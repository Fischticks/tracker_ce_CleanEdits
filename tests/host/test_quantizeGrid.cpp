// Host test for core/source/quantizeGrid.h. Build: g++ -std=c++17 -Wall -Icore/source tests/host/test_quantizeGrid.cpp
#include "quantizeGrid.h"

#include <cstdio>
#include <cstdlib>

using namespace quantizeGrid;

static int failures = 0;
static int checks = 0;
#define CHECK(c)                                                           \
    do                                                                     \
    {                                                                      \
        checks++;                                                          \
        if (!(c))                                                          \
        {                                                                  \
            failures++;                                                    \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);            \
        }                                                                  \
    } while (0)

int main()
{
    // 1. interval table: spec examples at Q = 4 and Q = 16 (in steps * 48)
    const int32_t q4[divisionCount] = { 6 * 48, 4 * 48, 3 * 48, 128, 2 * 48, 72, 64, 48 };
    const int32_t q16[divisionCount] = { 24 * 48, 16 * 48, 12 * 48, 512, 8 * 48, 6 * 48, 256, 4 * 48 };
    for (int d = 0; d < divisionCount; d++)
    {
        CHECK(intervalTicks(d, 4) == q4[d]);
        CHECK(intervalTicks(d, 16) == q16[d]);
        for (int q = 4; q <= 16; q++)
            CHECK(intervalTicks(d, q) >= 48); // never finer than one step
    }
    // 1/8T at Q=4 is 1 1/3 steps, 1/4T 2 2/3, 1/16D 1 1/2
    CHECK(intervalTicks(div1_8T, 4) == 64 && intervalTicks(div1_4T, 4) == 128 && intervalTicks(div1_16D, 4) == 72);
    CHECK(intervalTicks(div1_16, 7) == 84 && intervalTicks(div1_16D, 5) == 90);

    // 2. micromove <-> delay: every offset 1..47 has an exact micromove; 0 means none
    CHECK(micromoveForOffset(0) == -1);
    for (int o = 1; o <= 47; o++)
    {
        int8_t m = micromoveForOffset(o);
        CHECK(m >= 0 && m <= 100);
        CHECK(micromoveDelayTicks(m) == o);
    }
    // matches the sequencer's recording formula: m = map(uStep, 1, 48, 0, 100) = (uStep-1)*100/47
    for (int u = 2; u <= 48; u++)
        CHECK(micromoveDelayTicks((u - 1) * 100 / 47) >= u - 1 && micromoveDelayTicks((u - 1) * 100 / 47) <= u);

    // 3. nearest: exhaustive over several pattern lengths and every division / quarter value
    const int lens[] = { 1, 4, 16, 28, 30, 64, 100, 128 };
    for (int len : lens)
    {
        const int32_t total = len * 48;
        for (int d = 0; d < divisionCount; d++)
            for (int q = 4; q <= 16; q++)
            {
                const int32_t I = intervalTicks(d, q);
                for (int32_t p = 0; p < total; p++)
                {
                    Nearest n = nearest(p, I, total);
                    CHECK(n.wrapped >= 0 && n.wrapped < total);
                    // target is on the grid (or the pattern end)
                    CHECK(n.target == total || n.target % I == 0);
                    // on-grid input stays put (strength 100 reaches the grid exactly)
                    if (p % I == 0)
                        CHECK(n.target == p);
                    // nearest: no grid point (or pattern end) is closer; ties go forward
                    int32_t dist = abs(n.target - p);
                    int32_t lo = (p / I) * I, hi = lo + I > total ? total : lo + I;
                    CHECK(dist <= abs(lo - p) && dist <= abs(hi - p));
                    if (p - lo == hi - p)
                        CHECK(n.target == hi);
                    // strength endpoints
                    CHECK(applyStrength(p, n.target, 0, total) == p);
                    CHECK(applyStrength(p, n.target, 100, total) == n.wrapped);
                    int32_t half = applyStrength(p, n.target, 50, total);
                    CHECK(half >= 0 && half < total);
                }
            }
    }

    // 4. strength interpolation, rounding away from zero on exact half ticks
    CHECK(applyStrength(100, 96, 50, 768) == 98);   // -2
    CHECK(applyStrength(100, 97, 50, 768) == 98);   // -1.5 -> -2
    CHECK(applyStrength(100, 103, 50, 768) == 102); // +1.5 -> +2
    CHECK(applyStrength(100, 96, 25, 768) == 99);   // -1
    CHECK(applyStrength(100, 96, 75, 768) == 97);   // -3
    CHECK(applyStrength(5, 0, 100, 768) == 0);
    // monotone in strength
    for (int32_t p = 0; p < 768; p += 7)
    {
        Nearest n = nearest(p, 128, 768);
        int32_t prev = p;
        for (int s = 0; s <= 100; s++)
        {
            int32_t r = applyStrength(p, n.target, s, 768);
            if (n.target >= p && n.target < 768) // (a target at the pattern end wraps to 0 by design)
                CHECK(r >= prev);
            prev = r;
        }
    }

    // 5. wrap: pattern end is a candidate, the grid restarts at 0
    CHECK(nearest(1400, 192, 1440).target == 1440 && nearest(1400, 192, 1440).wrapped == 0); // 28-step... 30 steps
    CHECK(nearest(1344, 192, 1440).target == 1344);
    CHECK(next(1344, 192, 1440) == 0);              // 1536 would be outside: wraps to the start
    CHECK(next(0, 192, 1440) == 192);
    CHECK(next(100, 192, 1440) == 192);
    // 100% on a note just before the end lands on step 0 of the pattern
    CHECK(applyStrength(1430, 1440, 100, 1440) == 0);
    CHECK(applyStrength(1430, 1440, 50, 1440) == 1435);

    // 6. next() steps through every grid position exactly once per loop
    for (int d = 0; d < divisionCount; d++)
    {
        const int32_t total = 16 * 48, I = intervalTicks(d, 4);
        int32_t p = 0, count = 0;
        do
        {
            int32_t n = next(p, I, total);
            CHECK(n == 0 || n - p == I);
            p = n;
            count++;
        } while (p != 0 && count < 1000);
        CHECK(count == (total + I - 1) / I);
    }

    // 7. player position helper
    CHECK(positionFromPlayer(0, 1) == 0 && positionFromPlayer(3, 25) == 3 * 48 + 24 && positionFromPlayer(2, 48) == 143);
    CHECK(stepOf(143) == 2 && offsetOf(143) == 47);

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
