#pragma once
// Lazy Chop (MOD_LAZY_CHOP): mark slice positions by pressing Add Slice while auditioning a sliced
// sample from a pad. Positions are held pending and inserted in one batch when the session ends.
// Re-implementation of the earlier Lazy Chop (jamartinez4991/Tracker_Claude_Backup, mod/lazy-chop) on the
// current baseline; see docs/custom-firmware/lazy-chop.md.

#ifdef MOD_LAZY_CHOP

#include <stdint.h>

namespace lazyChop
{
void onNoteOn(int16_t source);  // after sequencer.handleNoteOn()
void onNoteOff(int16_t source); // initiating source released
void update();                  // natural end / cut / voice replacement / instrument change
void finalize(bool refreshUi);  // idempotent; always commits to the instrument bound at start

// Add Slice pressed. True = consumed by an active session (the ordinary midpoint Add must be skipped).
bool capture();
} // namespace lazyChop

#endif
