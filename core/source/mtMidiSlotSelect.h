#ifndef SOURCE_MTMIDISLOTSELECT_H_
#define SOURCE_MTMIDISLOTSELECT_H_

#include <stdint.h>

// Configuration storage is unconditional so every build variant uses the same
// global MIDI configuration layout.
void midiSlotSelectResetConfig();
void midiSlotSelectCheckConfig();

#ifdef MOD_MIDI_SLOT_SELECT
bool midiSlotSelectContextActive();
int16_t midiSlotSelectMapNote(uint8_t rawNote);

extern const char* const midiSlotSelectNoteLabels[128];
extern const char* const midiSlotSelectOctaveLabels[11];
void midiSlotSelectRefreshLabels();
#endif

#endif /* SOURCE_MTMIDISLOTSELECT_H_ */
