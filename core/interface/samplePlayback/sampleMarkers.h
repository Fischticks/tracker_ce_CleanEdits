#pragma once
// Single hook surface between cSamplePlayback (fSamplePlay.cpp) and the two Sample Playback Markers
// features. Without either flag every function is an empty inline, so the hook sites in fSamplePlay.cpp
// need no #ifdef of their own and compile to nothing.
//
//   MOD_SAMPLE_BOUNDARY_CAPTURE  -> samplePlayback/boundaryCapture.*
//   MOD_LAZY_CHOP                -> samplePlayback/lazyChop.*
// The two never share pending state; each owns its own PreviewSession (samplePlayback/previewSession.h).

#include <stdint.h>

#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE
#include "samplePlayback/boundaryCapture.h"
#endif
#ifdef MOD_LAZY_CHOP
#include "samplePlayback/lazyChop.h"
#endif

namespace sampleMarkers
{
inline void onNoteOn([[maybe_unused]] int16_t source)
{
#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE
    boundaryCapture::onNoteOn(source);
#endif
#ifdef MOD_LAZY_CHOP
    lazyChop::onNoteOn(source);
#endif
}

inline void onNoteOff([[maybe_unused]] int16_t source)
{
#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE
    boundaryCapture::onNoteOff(source);
#endif
#ifdef MOD_LAZY_CHOP
    lazyChop::onNoteOff(source);
#endif
}

inline void update()
{
#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE
    boundaryCapture::update();
#endif
#ifdef MOD_LAZY_CHOP
    lazyChop::update();
#endif
}

// Transport start, play-mode change, layout change, leaving the screen: commit whatever is pending.
inline void finalizeAll([[maybe_unused]] bool refreshUi = true)
{
#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE
    boundaryCapture::finalize(refreshUi);
#endif
#ifdef MOD_LAZY_CHOP
    lazyChop::finalize(refreshUi);
#endif
}

// Add Slice pressed. True = consumed by a Lazy Chop session.
inline bool captureSliceMarker()
{
#ifdef MOD_LAZY_CHOP
    return lazyChop::capture();
#else
    return false;
#endif
}

// Boundary function key pressed: 0 Start, 1 Loop Start, 2 Loop End, 3 End. Never consumes the key.
inline void captureBoundary([[maybe_unused]] uint8_t kind)
{
#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE
    boundaryCapture::capture(kind);
#endif
}
} // namespace sampleMarkers
