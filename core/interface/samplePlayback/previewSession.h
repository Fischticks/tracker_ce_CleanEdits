#pragma once
// Preview session: the small mechanism shared by Sample Boundary Capture and Lazy Chop.
//
// A session ties a feature's pending data to ONE preview note: its note source (pad / preview key), the
// instrument that was playing, and the voice (sequencer track == instrumentPlayer[] index) that plays it.
// The session never decides what the feature does with its data; it only answers "is this still the
// note I was started for" and reads the playhead. Each feature owns its own PreviewSession instance, so
// the two features can never see or commit each other's pending state.
//
// Compiled only when a feature that uses it is enabled.

#if defined(MOD_LAZY_CHOP) || defined(MOD_SAMPLE_BOUNDARY_CAPTURE)

#include <stdint.h>

struct PreviewSession
{
    bool active = false;
    uint8_t voiceIndex = 0;      // sequencer track == playerEngine index
    uint8_t instrumentIndex = 0; // commit target, fixed at begin()
    int16_t noteSource = -1;     // pad number / 12 / INTERFACE_BUTTON_PREVIEW

    // Call right after sequencer.handleNoteOn() for `source`. Binds to the voice that really got the note:
    // the sequencer track must hold an open manual note with the same source and instrument and the voice
    // must be sounding. Returns false (session stays inactive) otherwise.
    bool begin(int16_t source);

    // Sequencer binding still the same, voice still sounding, instrument still the one on screen.
    bool stillValid() const;

    bool isReversed() const;
    uint16_t position() const; // absolute normalised playhead 0..65535, only meaningful while stillValid()

    void clear() { *this = PreviewSession(); }
};

#endif
