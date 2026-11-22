#ifndef QUANTIZE_GRID_H_
#define QUANTIZE_GRID_H_

// Shared quantization grid for Shift + Pattern -> Quantize (MOD_SHIFT_PATTERN_QUANTIZE).
// Pure integer arithmetic with no firmware dependencies, so it is host-testable (tests/host/test_quantizeGrid.cpp).
//
// Positions are absolute, pattern-relative, in ticks: 48 ticks per Tracker step, position 0 = start of the pattern.
// The grid is anchored at the pattern start and restarts at 0 after the pattern end (the pattern wraps).

#include <stdint.h>

namespace quantizeGrid
{
constexpr int32_t kTicksPerStep = 48;

// Division list, coarsest to finest interval (D = dotted, T = triplet).
enum Division : uint8_t
{
    div1_4D = 0,
    div1_4,
    div1_8D,
    div1_4T,
    div1_8,
    div1_16D,
    div1_8T,
    div1_16,
    divisionCount
};

constexpr uint8_t kDefaultDivision = div1_16;
constexpr uint8_t kDefaultStrength = 100;
constexpr uint8_t kDefaultQuarter = 4;
constexpr uint8_t kQuarterMin = 4;
constexpr uint8_t kQuarterMax = 16;

// Interval in ticks = coefficient * Quarter Note Length. Straight 4Q/D, triplet 8Q/(3D), dotted 6Q/D steps, times 48.
constexpr int32_t kIntervalPerQuarter[divisionCount] = { 72, 48, 36, 32, 24, 18, 16, 12 };

inline int32_t intervalTicks(uint8_t division, uint8_t quarter)
{
    if (division >= divisionCount)
        division = kDefaultDivision;
    if (quarter < kQuarterMin)
        quarter = kQuarterMin;
    if (quarter > kQuarterMax)
        quarter = kQuarterMax;
    return kIntervalPerQuarter[division] * quarter;
}

struct Nearest
{
    int32_t target; // unwrapped: may equal totalTicks (= position 0 of the next loop)
    int32_t wrapped; // target folded into [0, totalTicks)
};

// Nearest grid position to p (0 <= p < totalTicks). Exact ties go to the later position. When the last grid
// interval does not fit the pattern, the next candidate is the pattern end (= start of the next loop).
inline Nearest nearest(int32_t p, int32_t interval, int32_t totalTicks)
{
    int32_t lo = (p / interval) * interval;
    int32_t hi = lo + interval;
    if (hi > totalTicks)
        hi = totalTicks;
    Nearest n;
    n.target = (p - lo < hi - p) ? lo : hi;
    n.wrapped = (n.target >= totalTicks) ? n.target - totalTicks : n.target;
    return n;
}

// Next grid position strictly after p, wrapped to 0 at the pattern end.
inline int32_t next(int32_t p, int32_t interval, int32_t totalTicks)
{
    int32_t n = (p / interval + 1) * interval;
    return (n >= totalTicks) ? 0 : n;
}

// p moved strength% of the way to target; half ticks round away from zero. Result folded into [0, totalTicks).
inline int32_t applyStrength(int32_t p, int32_t target, uint8_t strength, int32_t totalTicks)
{
    if (strength > 100)
        strength = 100;
    const int32_t num = (int32_t)strength * (target - p);
    const int32_t delta = (num >= 0) ? (num + 50) / 100 : -((-num + 50) / 100);
    int32_t r = p + delta;
    if (r >= totalTicks)
        r -= totalTicks;
    if (r < 0)
        r += totalTicks;
    return r;
}

// Micromove (0..100, -1 = none) <-> delay in ticks after the step start, exactly as the sequencer plays it:
// delay = (m + 1) * 47 / 100 + 1 (integer arithmetic of map(m + 1, 0, 100, 1, 48)).
inline int32_t micromoveDelayTicks(int32_t m)
{
    return m < 0 ? 0 : ((m + 1) * 47) / 100 + 1;
}

// Micromove value that plays `offset` ticks (0..47) after the step start; -1 for offset 0 (no micromove).
inline int8_t micromoveForOffset(int32_t offset)
{
    if (offset <= 0)
        return -1;
    for (int32_t m = 0; m <= 100; m++)
    {
        if (micromoveDelayTicks(m) >= offset)
            return (int8_t)m;
    }
    return 100;
}

inline int32_t stepOf(int32_t p)
{
    return p / kTicksPerStep;
}
inline int32_t offsetOf(int32_t p)
{
    return p % kTicksPerStep;
}

// Position of an incoming event: step index and 1-based microstep (1..48) as kept by the player.
inline int32_t positionFromPlayer(int32_t step, int32_t uStep)
{
    return step * kTicksPerStep + (uStep - 1);
}

// Exact grid position remembered between two Step Quantize note entries. It is only reusable while it still belongs to
// the current grid (same interval and pattern length) and the cursor has not been moved since it was stored.
struct RememberedPosition
{
    bool valid = false;
    int32_t pos = 0;
    int32_t interval = 0;
    int32_t total = 0;

    void set(int32_t p, int32_t gridInterval, int32_t gridTotal)
    {
        valid = true;
        pos = p;
        interval = gridInterval;
        total = gridTotal;
    }
    void invalidate() { valid = false; }
    bool usableAt(int32_t cursorStep, int32_t gridInterval, int32_t gridTotal) const
    {
        return valid && interval == gridInterval && total == gridTotal && pos >= 0 && pos < gridTotal &&
               stepOf(pos) == cursorStep;
    }
};

} // namespace quantizeGrid

#endif /* QUANTIZE_GRID_H_ */
