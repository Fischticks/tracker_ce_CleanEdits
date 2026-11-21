#pragma once
// Sample Playback Boundary Capture (MOD_SAMPLE_BOUNDARY_CAPTURE).
//
// While a sample is auditioned from a pad, the Start / Loop Start / Loop End / End function keys record
// the current playhead as *pending* boundaries. Nothing changes while the pad is held; on release (or
// another safe finalization point) the pending set is resolved against the instrument's current
// boundaries (last capture wins, see core/source/sampleBoundaryLogic.h) and committed in one step.

#ifdef MOD_SAMPLE_BOUNDARY_CAPTURE

#include <stdint.h>

namespace boundaryCapture
{
void onNoteOn(int16_t source);  // after sequencer.handleNoteOn(): start a session if the note qualifies
void onNoteOff(int16_t source); // initiating source released: commit
void update();                  // natural end / voice cut / instrument change: commit
void finalize(bool refreshUi);  // idempotent; always commits to the instrument bound at start

// A boundary function key (0 = Start, 1 = Loop Start, 2 = Loop End, 3 = End) was pressed.
// Records the playhead when a session is active and the boundary is active in the playback mode.
// The key's normal behaviour (selecting the point) is NOT replaced; the return value only reports
// whether a capture was recorded.
bool capture(uint8_t kind);
} // namespace boundaryCapture

#endif
