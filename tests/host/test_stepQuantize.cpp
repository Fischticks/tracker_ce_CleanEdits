// Host test of the Step Quantize cursor chain: the same sequence of quantizeGrid calls that
// Sequencer::stepQuantizeTarget / takeStepQuantizeAdvance perform (the sequencer itself is not linked here).
// Build: g++ -std=c++17 -Wall -Icore/source tests/host/test_stepQuantize.cpp -o /tmp/tsq && /tmp/tsq
#include <stdio.h>

#include "quantizeGrid.h"

using namespace quantizeGrid;

static int failures = 0;
static long checks = 0;
#define CHECK(c)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        checks++;                                                                                  \
        if (!(c))                                                                                  \
        {                                                                                          \
            if (failures++ < 10)                                                                   \
                printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c);                                 \
        }                                                                                          \
    } while (0)

int main()
{
    for (int q = kQuarterMin; q <= kQuarterMax; q++)
    {
        for (int d = 0; d < divisionCount; d++)
        {
            for (int len = 1; len <= 128; len += (len < 16 ? 1 : 7))
            {
                const int32_t total = len * kTicksPerStep;
                const int32_t interval = intervalTicks(d, q);
                // start the chain from every cursor step
                for (int start = 0; start < len; start++)
                {
                    int32_t p = nearest(start * kTicksPerStep, interval, total).wrapped;
                    int32_t prev = -1;
                    int steps = 0;
                    int32_t cursor = start;
                    for (int n = 0; n < 4 * (total / interval + 2); n++)
                    {
                        // validity check of the sequencer: the remembered position belongs to the cursor step
                        CHECK(stepOf(p) == cursor || n == 0);
                        CHECK(p >= 0 && p < total);
                        // a micromove must replay at exactly the offset inside the step
                        const int32_t off = offsetOf(p);
                        const int8_t mm = micromoveForOffset(off);
                        CHECK(micromoveDelayTicks(mm) == off);
                        // grid positions advance monotonically until the wrap
                        if (prev >= 0 && p != 0)
                            CHECK(p - prev == interval || (p - prev) > 0);
                        prev = p;
                        const int32_t nx = next(p, interval, total);
                        // the cursor goes to the step holding nx; re-validating must reproduce nx
                        cursor = stepOf(nx);
                        p = nx;
                        steps++;
                    }
                    CHECK(steps > 0);
                }
            }
        }
    }
    // Remembered position (RememberedPosition): stale positions are never reused.
    {
        const int32_t total = 28 * kTicksPerStep;
        const int32_t i16d = intervalTicks(div1_16D, 4); // a different grid
        const int32_t i8t = intervalTicks(div1_8T, 4);
        RememberedPosition r;
        const int32_t p = next(0, i8t, total); // a fractional position
        r.set(p, i8t, total);
        CHECK(r.usableAt(stepOf(p), i8t, total));
        // Division change (different interval) -> not reusable even on the same step
        CHECK(!r.usableAt(stepOf(p), i16d, total));
        // Quarter Note Length change -> not reusable
        CHECK(!r.usableAt(stepOf(p), intervalTicks(div1_8T, 5), total));
        // Pattern length change -> not reusable
        CHECK(!r.usableAt(stepOf(p), i8t, 20 * kTicksPerStep));
        // Manual cursor move invalidates; returning to the same step stays invalid
        r.invalidate();
        CHECK(!r.usableAt(stepOf(p) + 1, i8t, total));
        CHECK(!r.usableAt(stepOf(p), i8t, total));
        // A valid position is not reusable on another step
        r.set(p, i8t, total);
        CHECK(!r.usableAt(stepOf(p) + 1, i8t, total));
        CHECK(!r.usableAt(stepOf(p) - 1, i8t, total));
    }

    // Realtime Instrument-pad Step Quantize uses the nearest grid point at full strength,
    // independent of the Live Record Quantize Strength setting.
    for (int q = kQuarterMin; q <= kQuarterMax; q++)
    {
        for (int d = 0; d < divisionCount; d++)
        {
            const int32_t interval = intervalTicks(d, q);
            const int patternLengths[] = {1, 4, 16, 28, 30, 64, 100, 128};
            for (int len : patternLengths)
            {
                const int32_t total = len * kTicksPerStep;
                for (int32_t physical = 0; physical < total; physical++)
                {
                    const Nearest target = nearest(physical, interval, total);
                    const int32_t destination = target.wrapped;
                    CHECK(destination >= 0 && destination < total);
                    CHECK(destination == target.target || target.target == total);
                    CHECK(destination % interval == 0 || destination == 0);
                    const int32_t destinationStep = stepOf(destination);
                    const int8_t destinationMicromove = micromoveForOffset(offsetOf(destination));
                    CHECK(destinationStep >= 0 && destinationStep < len);
                    CHECK(micromoveDelayTicks(destinationMicromove) == offsetOf(destination));
                    // A full-strength Step Quantize target is exactly the nearest grid point.
                    CHECK(applyStrength(physical, target.target, 100, total) == destination);
                }
            }
        }
    }
    // Exact pattern-end targets wrap to step zero and retain their forward-boundary identity.
    {
        const int32_t total = 16 * kTicksPerStep;
        const int32_t interval = intervalTicks(div1_16, 4);
        const Nearest target = nearest(total - 1, interval, total);
        CHECK(target.target == total);
        CHECK(target.wrapped == 0);
        CHECK(stepOf(target.wrapped) == 0);
        CHECK(micromoveForOffset(offsetOf(target.wrapped)) == -1);
    }

    printf("%ld checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
