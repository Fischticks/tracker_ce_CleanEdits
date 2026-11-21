// Sample Playback Boundary Capture - audio-side helpers (MOD_SAMPLE_BOUNDARY_CAPTURE).
// Everything here is guarded; without the flag this translation unit is empty.
#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE

#include "Audio.h"
#include "play_memory.h"
#include "sampleBoundaryLogic.h"
#include <cmath>

// Smallest Loop Start -> Loop End distance (frames) for which the Forward Loop's single overshoot
// correction ("iPitchCounter = loopPoint1 + (iPitchCounter - loopEnd)") can never land outside the loop.
//
// After one output sample iPitchCounter advances by floor(pitch) or floor(pitch) + 1 (fraction carry),
// so the overshoot past the loop end is at most floor(pitch) + 1 - 1; one extra frame keeps the
// interpolation read (+1) inside the loop. The largest increment the note table can produce is
// notes[MAX_NOTE - 1] plus, at most, the finetune step up to the next semitone (which for the top note is
// bounded by the last semitone interval), so the value follows the table instead of a hard-coded number.
uint32_t AudioPlayMemory::getForwardLoopMinFrames()
{
    const float top = notes[MAX_NOTE - 1];
    const float lastSemitone = notes[MAX_NOTE - 1] - notes[MAX_NOTE - 2];
    const float maxIncrement = top + lastSemitone;
    return (uint32_t)ceilf(maxIncrement) + 1;
}

// Applies new Loop Start / Loop End to a voice that is playing `instrIdx`, using the exact frame
// positions (no snap-to-zero). The caller holds AudioNoInterrupts() and has already stored the new
// values in the instrument. Start/End of a sounding voice are never changed (the engine only reads
// them at note-on), so a loop that no longer fits inside the voice's own region is left for the next
// note-on instead of being forced. Streamed voices are left alone as well: their look-ahead buffer was
// planned with the loop points of the note-on.
bool AudioPlayMemory::applyCapturedBoundaries(uint8_t instrIdx, uint16_t loopPoint1, uint16_t loopPoint2)
{
    if (!playing || currentInstrIdx != instrIdx || currentIsStreamed)
        return false;
    if (!((currentPlayMode == playModeLoopForward) || (currentPlayMode == playModeLoopBackward) ||
          (currentPlayMode == playModePingpong)))
        return false;

    uint32_t loop1;
    uint32_t loop2;
    if (samplePointsSource != nullptr)
    {
        loop1 = samplePointsSource->pointsInSamples.loop1;
        loop2 = samplePointsSource->pointsInSamples.loop2;
    }
    else
    {
        loop1 = sampleBoundary::toFrame(loopPoint1, (uint32_t)currentSampleLength);
        loop2 = sampleBoundary::toFrame(loopPoint2, (uint32_t)currentSampleLength);
    }

    if (loop1 < pointsInSamples.start || loop2 >= pointsInSamples.end || loop2 <= loop1)
        return false;

    pointsInSamples.loop1 = loop1;
    pointsInSamples.loop2 = loop2;
    constrainsInSamples.loopPoint1 = loop1 - pointsInSamples.start;
    constrainsInSamples.loopPoint2 = loop2 - pointsInSamples.start;
    constrainsInSamples.loopLength = loop2 - loop1;
    return true;
}

#endif // MOD_SAMPLE_BOUNDARY_CAPTURE
