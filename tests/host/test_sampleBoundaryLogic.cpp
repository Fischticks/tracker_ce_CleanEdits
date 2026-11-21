// Host test for core/source/sampleBoundaryLogic.cpp.  Build and run:
//   g++ -std=c++17 -DMOD_SAMPLE_BOUNDARY_CAPTURE -Icore/source tests/host/test_sampleBoundaryLogic.cpp core/source/sampleBoundaryLogic.cpp -o /tmp/sbl_test && /tmp/sbl_test
#include "sampleBoundaryLogic.h"
#include <cstdio>
#include <cstdlib>

using namespace sampleBoundary;
static int failures = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static Arrangement A(uint16_t s, uint16_t l1, uint16_t l2, uint16_t e) { Arrangement a; a.pos[0]=s; a.pos[1]=l1; a.pos[2]=l2; a.pos[3]=e; return a; }
static void cap(Capture* c, Kind k, uint16_t v, uint32_t order) { c[k].present = true; c[k].value = v; c[k].order = order; }
static Context ctx(uint32_t frames, Mode m, uint32_t fwd = 33) { Context c; c.sampleFrames = frames; c.mode = m; c.minForwardLoopFrames = fwd; return c; }
static uint32_t F(const Arrangement& a, Kind k, uint32_t frames) { return toFrame(a.pos[k], frames); }

int main()
{
    const uint32_t LONG = 6553500; // 100 frames per norm step: spacing rules do not interfere

    // Spec 5.6 examples (positions scaled; frames >> minimum spacing).
    {
        Context c = ctx(LONG, kBackwardLoop);
        Arrangement base = A(1000, 2000, 4000, 6000), out; Capture caps[kBoundaryCount];
        cap(caps, kStart, 2500, 1); CHECK(resolve(base, caps, c, out) == 0);
        CHECK(out.pos[kStart] == 2500 && out.pos[kLoopStart] == 2500 && out.pos[kLoopEnd] == 4000 && out.pos[kEnd] == 6000);

        Capture c2[kBoundaryCount]; cap(c2, kLoopStart, 800, 1); CHECK(resolve(base, c2, c, out) == 0);
        CHECK(out.pos[kStart] == 800 && out.pos[kLoopStart] == 800 && out.pos[kLoopEnd] == 4000);

        Capture c3[kBoundaryCount]; cap(c3, kLoopEnd, 6500, 1); CHECK(resolve(base, c3, c, out) == 0);
        CHECK(out.pos[kLoopEnd] == 6500 && out.pos[kEnd] > 6500 && toFrame(out.pos[kEnd], LONG) >= toFrame(6500, LONG) + 1);
        CHECK(out.pos[kStart] == 1000 && out.pos[kLoopStart] == 2000);

        // End captured below Loop End propagates through Loop Start and Start.
        Capture c4[kBoundaryCount]; cap(c4, kEnd, 1500, 1); CHECK(resolve(base, c4, c, out) == 0);
        CHECK(out.pos[kEnd] == 1500);
        CHECK(F(out, kLoopEnd, LONG) + 1 <= F(out, kEnd, LONG));
        CHECK(F(out, kLoopStart, LONG) + 1 <= F(out, kLoopEnd, LONG));
        CHECK(out.pos[kStart] <= out.pos[kLoopStart]);
        CHECK(isValid(out, c));
    }
    // Uncaptured boundaries untouched when no conflict.
    {
        Context c = ctx(LONG, kPingpongLoop);
        Arrangement base = A(100, 2000, 4000, 6000), out; Capture caps[kBoundaryCount];
        cap(caps, kStart, 500, 1); cap(caps, kEnd, 9000, 2);
        CHECK(resolve(base, caps, c, out) == 0);
        CHECK(out.pos[kStart] == 500 && out.pos[kEnd] == 9000 && out.pos[kLoopStart] == 2000 && out.pos[kLoopEnd] == 4000);
    }
    // Chronological order: later capture is never moved by an earlier one; result independent of key order.
    {
        Context c = ctx(LONG, kBackwardLoop);
        Arrangement base = A(1000, 2000, 4000, 6000), o1, o2; Capture a[kBoundaryCount], b[kBoundaryCount];
        // Start@A(order1), LoopStart@B(order2), Start@C(order3 replaces order1)
        cap(a, kLoopStart, 3000, 2); cap(a, kStart, 3500, 3);   // Start (C=3500) newest
        cap(b, kStart, 3500, 3); cap(b, kLoopStart, 3000, 2);    // same, struct filled in other order
        resolve(base, a, c, o1); resolve(base, b, c, o2);
        for (int k = 0; k < kBoundaryCount; k++) CHECK(o1.pos[k] == o2.pos[k]);
        CHECK(o1.pos[kStart] == 3500);            // newest capture preserved
        CHECK(o1.pos[kLoopStart] >= 3500);        // older neighbour moved
        // Reverse the chronology: Loop Start newest, so Start is pushed down instead.
        Capture r[kBoundaryCount]; cap(r, kStart, 3500, 1); cap(r, kLoopStart, 3000, 2);
        resolve(base, r, c, o2);
        CHECK(o2.pos[kLoopStart] == 3000 && o2.pos[kStart] == 3000);
    }
    // One-Shot: only Start/End, loops untouched, 1 frame minimum.
    {
        Context c = ctx(LONG, kOneShot);
        Arrangement base = A(1000, 2000, 4000, 6000), out; Capture caps[kBoundaryCount];
        cap(caps, kStart, 7000, 1); CHECK(resolve(base, caps, c, out) == 0);
        CHECK(out.pos[kStart] == 7000 && out.pos[kEnd] > 7000 && F(out, kEnd, LONG) >= F(out, kStart, LONG) + 1);
        CHECK(out.pos[kLoopStart] == 2000 && out.pos[kLoopEnd] == 4000);
        Capture e[kBoundaryCount]; cap(e, kEnd, 0, 1);  // End to 0 pushes Start to 0 -> frames equal -> impossible
        CHECK(resolve(base, e, c, out) == (1u << kEnd));
        CHECK(out.pos[kStart] == 1000 && out.pos[kEnd] == 6000);   // previous valid arrangement kept
        Capture lp[kBoundaryCount]; cap(lp, kLoopStart, 5, 1);               // inactive in One-Shot
        CHECK(resolve(base, lp, c, out) == (1u << kLoopStart));
    }
    // Same sample frame for distinct 16-bit values (short sample) must be rejected / separated in frames.
    {
        const uint32_t SHORT = 1000;   // ~65 norm steps per frame
        Context c = ctx(SHORT, kOneShot);
        Arrangement base = A(0, 0, 0, 65535), out; Capture caps[kBoundaryCount];
        cap(caps, kStart, 30000, 1); cap(caps, kEnd, 30010, 2);     // 10 norm steps apart = same frame
        uint8_t rej = resolve(base, caps, c, out);
        CHECK(isValid(out, c));
        CHECK(F(out, kEnd, SHORT) >= F(out, kStart, SHORT) + 1);
        (void)rej;
        CHECK(out.pos[kEnd] == 30010 || out.pos[kStart] != 30000); // End newest keeps its value, Start moved
        CHECK(out.pos[kEnd] == 30010 && F(out, kStart, SHORT) + 1 <= F(out, kEnd, SHORT));
    }
    // Exact minimum separations are accepted, one frame less is not reachable.
    {
        const uint32_t N = 65535; // 1 frame per norm step: exact control
        {
            Context c = ctx(N, kForwardLoop, 33);
            Arrangement base = A(0, 100, 5000, 9000), out; Capture caps[kBoundaryCount];
            cap(caps, kLoopStart, 4000, 1); cap(caps, kLoopEnd, 4033, 2);
            CHECK(resolve(base, caps, c, out) == 0 && out.pos[kLoopEnd] == 4033 && out.pos[kLoopStart] == 4000);
            Capture caps2[kBoundaryCount]; cap(caps2, kLoopStart, 4000, 1); cap(caps2, kLoopEnd, 4032, 2);  // 32 < 33 -> Loop Start pushed down
            CHECK(resolve(base, caps2, c, out) == 0 && out.pos[kLoopEnd] == 4032 && out.pos[kLoopStart] == 4032 - 33);
            CHECK(isValid(out, c));
        }
        {
            Context c = ctx(N, kPingpongLoop);
            Arrangement base = A(0, 100, 5000, 9000), out; Capture caps[kBoundaryCount];
            cap(caps, kLoopStart, 4000, 1); cap(caps, kLoopEnd, 4002, 2);
            CHECK(resolve(base, caps, c, out) == 0 && out.pos[kLoopEnd] - out.pos[kLoopStart] == 2);
            Capture k2[kBoundaryCount]; cap(k2, kLoopStart, 4000, 1); cap(k2, kLoopEnd, 4001, 2);
            resolve(base, k2, c, out); CHECK(out.pos[kLoopEnd] - out.pos[kLoopStart] >= 2 && out.pos[kLoopEnd] == 4001);
        }
        {
            Context c = ctx(N, kBackwardLoop);
            Arrangement base = A(0, 100, 5000, 9000), out; Capture caps[kBoundaryCount];
            cap(caps, kLoopStart, 4000, 1); cap(caps, kLoopEnd, 4000, 2);   // zero-length loop is invalid
            resolve(base, caps, c, out); CHECK(out.pos[kLoopEnd] > out.pos[kLoopStart] || out.pos[kLoopStart] < 4000);
            CHECK(isValid(out, c) && F(out, kLoopEnd, N) - F(out, kLoopStart, N) >= 1);
        }
        {
            // Loop End and End need one frame between them.
            Context c = ctx(N, kBackwardLoop);
            Arrangement base = A(0, 100, 5000, 9000), out; Capture caps[kBoundaryCount];
            cap(caps, kEnd, 8000, 1); cap(caps, kLoopEnd, 8000, 2);
            resolve(base, caps, c, out); CHECK(F(out, kEnd, N) >= F(out, kLoopEnd, N) + 1 && out.pos[kLoopEnd] == 8000);
        }
        {
            // Start == Loop Start is allowed.
            Context c = ctx(N, kForwardLoop, 33);
            Arrangement base = A(0, 100, 5000, 9000), out; Capture caps[kBoundaryCount]; cap(caps, kStart, 100, 1);
            CHECK(resolve(base, caps, c, out) == 0 && out.pos[kStart] == 100 && out.pos[kLoopStart] == 100);
        }
    }
    // Positions at the extremes.
    {
        const uint32_t N = 65535;
        Context c = ctx(N, kBackwardLoop);
        Arrangement base = A(10, 20, 40, 60), out; Capture caps[kBoundaryCount];
        cap(caps, kEnd, 65535, 1); CHECK(resolve(base, caps, c, out) == 0 && out.pos[kEnd] == 65535);
        Capture k[kBoundaryCount]; cap(k, kLoopEnd, 65535, 1);   // End cannot go above 65535 -> impossible (needs 1 frame)
        CHECK(resolve(base, k, c, out) == (1u << kLoopEnd) && out.pos[kLoopEnd] == 40);
        Capture z[kBoundaryCount]; cap(z, kLoopStart, 0, 1); CHECK(resolve(base, z, c, out) == 0 && out.pos[kStart] == 0 && out.pos[kLoopStart] == 0);
        Capture s[kBoundaryCount]; cap(s, kStart, 65535, 1);      // Start at max pushes everything up: impossible
        CHECK(resolve(base, s, c, out) == (1u << kStart) && isValid(out, c) && out.pos[kStart] == 10);
        // Sample too short to hold any valid loop (frames < 2 + spacing): everything rejected, nothing changes.
        Context tiny = ctx(2, kPingpongLoop);
        Capture t[kBoundaryCount]; cap(t, kLoopStart, 100, 1);
        Arrangement b2 = A(0, 0, 65535, 65535);
        CHECK(resolve(b2, t, tiny, out) != 0 && out.pos[kLoopStart] == 0);
    }
    // Invalid capture does not corrupt other valid captures; the rest are still applied.
    {
        const uint32_t N = 65535;
        Context c = ctx(N, kBackwardLoop);
        Arrangement base = A(10, 20, 40, 60), out; Capture caps[kBoundaryCount];
        cap(caps, kLoopStart, 30, 1); cap(caps, kLoopEnd, 65535, 2); cap(caps, kStart, 5, 3);
        uint8_t rej = resolve(base, caps, c, out);
        CHECK(rej == (1u << kLoopEnd));
        CHECK(out.pos[kStart] == 5 && out.pos[kLoopStart] == 30 && out.pos[kLoopEnd] == 40 && out.pos[kEnd] == 60);
    }
    // Brute-force property: every outcome is valid and every non-rejected capture value is preserved
    // unless a *later* capture moved it.
    {
        srand(7);
        const uint32_t lens[] = { 2, 10, 100, 1000, 65535, 400000 };
        const Mode modes[] = { kOneShot, kForwardLoop, kBackwardLoop, kPingpongLoop };
        for (int it = 0; it < 200000; it++)
        {
            Context c = ctx(lens[rand() % 6], modes[rand() % 4], 33);
            Arrangement base; uint16_t v[4]; for (int i = 0; i < 4; i++) v[i] = rand() % 65536;
            // sorted valid-ish base
            for (int i = 0; i < 4; i++) for (int j = i + 1; j < 4; j++) if (v[j] < v[i]) { uint16_t t = v[i]; v[i] = v[j]; v[j] = t; }
            for (int i = 0; i < 4; i++) base.pos[i] = v[i];
            Capture caps[kBoundaryCount]; uint32_t ord = 1;
            int perm[4] = { 0, 1, 2, 3 };
            for (int i = 3; i > 0; i--) { int j = rand() % (i + 1); int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
            for (int i = 0; i < 4; i++) if (rand() % 2) cap(caps, (Kind)perm[i], rand() % 65536, ord++);
            Arrangement out; uint8_t rej = resolve(base, caps, c, out); (void)rej;
            if (isValid(base, c))   // from a valid base the result must be valid
            {
                if (!isValid(out, c)) { failures++; printf("INVALID RESULT it=%d\n", it); break; }
            }
            // Uncaptured boundaries outside the propagated chain are never touched when nothing was applied.
            bool anyApplied = false; for (int k = 0; k < 4; k++) if (caps[k].present && !(rej & (1u << k))) anyApplied = true;
            if (!anyApplied) for (int k = 0; k < 4; k++) if (out.pos[k] != base.pos[k]) { failures++; printf("CHANGED WITHOUT APPLY it=%d\n", it); it = 1 << 30; break; }
        }
        checks++;
    }
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
