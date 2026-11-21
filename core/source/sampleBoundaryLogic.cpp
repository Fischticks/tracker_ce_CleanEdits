#include "sampleBoundaryLogic.h"

#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE

namespace sampleBoundary
{
namespace
{
constexpr uint16_t kMaxNorm = 65535;

uint32_t gapFrames(const Context& ctx, Kind lower)
{
    if (ctx.mode == kOneShot)
    {
        return 1; // Start -> End
    }
    switch (lower)
    {
        case kStart: return 0; // Start == Loop Start is safe in every loop mode
        case kLoopStart: return minLoopFrames(ctx);
        case kLoopEnd: return 1; // at least one frame between Loop End and End
        default: return 0;
    }
}

// Active boundaries in ascending order.
uint8_t activeList(const Context& ctx, Kind list[kBoundaryCount])
{
    if (ctx.mode == kOneShot)
    {
        list[0] = kStart;
        list[1] = kEnd;
        return 2;
    }
    list[0] = kStart;
    list[1] = kLoopStart;
    list[2] = kLoopEnd;
    list[3] = kEnd;
    return 4;
}

// Smallest norm >= from whose frame is >= targetFrame. false if none exists.
bool smallestNormForFrame(uint16_t from, uint32_t targetFrame, uint32_t frames, uint16_t& result)
{
    if (toFrame(kMaxNorm, frames) < targetFrame)
    {
        return false;
    }
    uint32_t lo = from;
    uint32_t hi = kMaxNorm;
    while (lo < hi)
    {
        const uint32_t mid = (lo + hi) / 2;
        if (toFrame((uint16_t)mid, frames) >= targetFrame)
            hi = mid;
        else
            lo = mid + 1;
    }
    result = (uint16_t)lo;
    return true;
}

// Largest norm <= upTo whose frame is <= targetFrame. false if even norm 0 is too high.
bool largestNormForFrame(uint16_t upTo, uint32_t targetFrame, uint32_t frames, uint16_t& result)
{
    if (toFrame(0, frames) > targetFrame)
    {
        return false;
    }
    uint32_t lo = 0;
    uint32_t hi = upTo;
    while (lo < hi)
    {
        const uint32_t mid = (lo + hi + 1) / 2;
        if (toFrame((uint16_t)mid, frames) <= targetFrame)
            lo = mid;
        else
            hi = mid - 1;
    }
    result = (uint16_t)lo;
    return true;
}

bool applyOne(const Arrangement& base,
              Kind kind,
              uint16_t value,
              const Context& ctx,
              Arrangement& result)
{
    Kind list[kBoundaryCount];
    const uint8_t n = activeList(ctx, list);

    int idx = -1;
    for (uint8_t i = 0; i < n; i++)
    {
        if (list[i] == kind)
            idx = i;
    }
    if (idx < 0)
    {
        return false; // not an active boundary in this mode
    }

    result = base;
    result.pos[kind] = value;

    // Push the upper neighbours outward.
    for (int i = idx + 1; i < n; i++)
    {
        const uint16_t prev = result.pos[list[i - 1]];
        const uint32_t prevFrame = toFrame(prev, ctx.sampleFrames);
        const uint32_t target = prevFrame + gapFrames(ctx, list[i - 1]);
        const uint16_t cur = result.pos[list[i]];
        if (cur >= prev && toFrame(cur, ctx.sampleFrames) >= target)
        {
            continue; // already fine; farther boundaries need no change
        }
        uint16_t pushed = 0;
        if (!smallestNormForFrame(prev, target, ctx.sampleFrames, pushed))
        {
            return false;
        }
        result.pos[list[i]] = pushed;
    }

    // Push the lower neighbours outward.
    for (int i = idx - 1; i >= 0; i--)
    {
        const uint16_t next = result.pos[list[i + 1]];
        const uint32_t nextFrame = toFrame(next, ctx.sampleFrames);
        const uint32_t gap = gapFrames(ctx, list[i]);
        const uint16_t cur = result.pos[list[i]];
        if (cur <= next && toFrame(cur, ctx.sampleFrames) + gap <= nextFrame)
        {
            continue;
        }
        if (nextFrame < gap)
        {
            return false;
        }
        uint16_t pushed = 0;
        if (!largestNormForFrame(next, nextFrame - gap, ctx.sampleFrames, pushed))
        {
            return false;
        }
        result.pos[list[i]] = pushed;
    }

    return isValid(result, ctx);
}
} // namespace

uint32_t toFrame(uint16_t norm, uint32_t sampleFrames)
{
    return (uint32_t)((float)norm * ((float)sampleFrames / 65535.0f));
}

uint32_t minLoopFrames(const Context& ctx)
{
    switch (ctx.mode)
    {
        case kForwardLoop: return ctx.minForwardLoopFrames;
        case kBackwardLoop: return 1;
        case kPingpongLoop: return 2;
        default: return 0;
    }
}

bool isValid(const Arrangement& a, const Context& ctx)
{
    if (ctx.sampleFrames < 2)
    {
        return false;
    }

    Kind list[kBoundaryCount];
    const uint8_t n = activeList(ctx, list);

    for (uint8_t i = 0; i + 1 < n; i++)
    {
        const uint16_t lo = a.pos[list[i]];
        const uint16_t hi = a.pos[list[i + 1]];
        if (lo > hi)
        {
            return false;
        }
        if (toFrame(hi, ctx.sampleFrames) < toFrame(lo, ctx.sampleFrames) + gapFrames(ctx, list[i]))
        {
            return false;
        }
    }
    return true;
}

uint8_t resolve(const Arrangement& current,
                const Capture captures[kBoundaryCount],
                const Context& ctx,
                Arrangement& out)
{
    out = current;
    uint8_t rejected = 0;

    // Chronological processing: stable insertion sort of the present captures by `order`.
    Kind seq[kBoundaryCount];
    uint8_t count = 0;
    for (uint8_t k = 0; k < kBoundaryCount; k++)
    {
        if (!captures[k].present)
            continue;
        int j = count - 1;
        while (j >= 0 && captures[seq[j]].order > captures[k].order)
        {
            seq[j + 1] = seq[j];
            j--;
        }
        seq[j + 1] = (Kind)k;
        count++;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        const Kind k = seq[i];
        Arrangement candidate;
        if (applyOne(out, k, captures[k].value, ctx, candidate))
        {
            out = candidate;
        }
        else
        {
            rejected |= (uint8_t)(1u << k);
        }
    }
    return rejected;
}
} // namespace sampleBoundary

#endif // MOD_SAMPLE_BOUNDARY_CAPTURE
