#pragma once
// Sample Playback Boundary Capture - pure logic (MOD_SAMPLE_BOUNDARY_CAPTURE).
//
// No firmware dependencies on purpose: this file is compiled unchanged by the host test in
// tests/host/test_sampleBoundaryLogic.cpp. The audio engine and the UI both call into it, so the
// frame positions that are validated here are the frame positions the engine will use.
//
// Boundaries are stored as normalised 16-bit positions (0..65535). All spacing rules are checked on
// the sample-frame positions those values resolve to, never on the 16-bit values alone.

#include <stdint.h>

namespace sampleBoundary
{
enum Kind : uint8_t
{
    kStart = 0,
    kLoopStart = 1,
    kLoopEnd = 2,
    kEnd = 3,
    kBoundaryCount = 4
};

enum Mode : uint8_t
{
    kOneShot,
    kForwardLoop,
    kBackwardLoop,
    kPingpongLoop
};

struct Arrangement
{
    uint16_t pos[kBoundaryCount];
};

// One pending capture. `order` is a strictly increasing submission counter: pressing a key again
// replaces the value and takes a new, higher order number.
struct Capture
{
    bool present = false;
    uint16_t value = 0;
    uint32_t order = 0;
};

struct Context
{
    uint32_t sampleFrames = 0;
    Mode mode = kOneShot;
    // Minimum Loop Start -> Loop End distance (frames) for Forward Loop. The caller derives it from
    // the engine's pitch table (see AudioPlayMemory::getForwardLoopMinFrames()).
    uint32_t minForwardLoopFrames = 0;
};

// Same expression and rounding as AudioPlayMemory::calculatePointPositionAndSnapToZero() uses when
// snap-to-zero is off: (uint32_t)((float)position * ((float)sampleLength / MAX_16BIT)).
uint32_t toFrame(uint16_t norm, uint32_t sampleFrames);

// Minimum Loop Start -> Loop End distance in frames for the mode (0 for One-Shot: not applicable).
uint32_t minLoopFrames(const Context& ctx);

// Whole-arrangement check on the resolved frame positions. In One-Shot only Start/End are examined.
bool isValid(const Arrangement& a, const Context& ctx);

// Apply every present capture in ascending `order`. Each one is applied with "last capture wins":
// the captured boundary keeps its value and neighbours are pushed outward (through Loop Start/Loop End
// and Start/End) until the arrangement is valid. A capture that cannot produce a valid arrangement is
// discarded and the arrangement before it is kept. Returns a bit mask (1 << Kind) of discarded captures.
// `out` is always written (it equals `current` when everything was discarded).
uint8_t resolve(const Arrangement& current,
                const Capture captures[kBoundaryCount],
                const Context& ctx,
                Arrangement& out);
} // namespace sampleBoundary
