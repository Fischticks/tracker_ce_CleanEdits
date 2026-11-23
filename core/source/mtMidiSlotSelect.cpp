#include "mtMidiSlotSelect.h"

#include "mtMidiSlotSelectMapping.h"
#include "mtStructs.h"

#ifdef MOD_MIDI_SLOT_SELECT
#include "core/interface.h"
#include "patternEditor/patternEditor.h"
#include "scales.h"

#include <stdio.h>

extern cPatternEditor patternEditor;
#endif

void midiSlotSelectResetConfig()
{
    mtConfig.midi.midiSlotSelectEnabled = 0;
    mtConfig.midi.midiSlotSelectMode = midiSlotSelectModeDrum;
    mtConfig.midi.midiSlotSelectStartNote = kMidiSlotSelectDefaultStartNote;
    mtConfig.midi.midiSlotSelectWhiteKeyOctave = kMidiSlotSelectDefaultWhiteKeyOctave;
    mtConfig.midi.midiSlotSelectConfigMarker = kMidiSlotSelectConfigMarker;
    mtConfig.midi.midiSlotSelectReserved = 0;
}

void midiSlotSelectCheckConfig()
{
    if (mtConfig.midi.midiSlotSelectConfigMarker != kMidiSlotSelectConfigMarker)
    {
        midiSlotSelectResetConfig();
        return;
    }

    if (mtConfig.midi.midiSlotSelectEnabled > 1)
        mtConfig.midi.midiSlotSelectEnabled = 0;
    if (mtConfig.midi.midiSlotSelectMode >= midiSlotSelectModeCount)
        mtConfig.midi.midiSlotSelectMode = midiSlotSelectModeDrum;
    if (mtConfig.midi.midiSlotSelectStartNote > 127)
        mtConfig.midi.midiSlotSelectStartNote = kMidiSlotSelectDefaultStartNote;
    if (mtConfig.midi.midiSlotSelectWhiteKeyOctave > 10)
        mtConfig.midi.midiSlotSelectWhiteKeyOctave = kMidiSlotSelectDefaultWhiteKeyOctave;

    mtConfig.midi.midiSlotSelectReserved = 0;
}

#ifdef MOD_MIDI_SLOT_SELECT

bool midiSlotSelectContextActive()
{
    return mtConfig.midi.midiSlotSelectEnabled == 1 &&
           mtInterface.isPatternScreenActive() &&
           patternEditor.editParam == cPatternEditor::EditInstr;
}

int16_t midiSlotSelectMapNote(uint8_t rawNote)
{
    return midiSlotSelectMapRawNote(rawNote,
                                    mtConfig.midi.midiSlotSelectMode,
                                    mtConfig.midi.midiSlotSelectStartNote,
                                    mtConfig.midi.midiSlotSelectWhiteKeyOctave,
                                    kTotalInstrumentsCount);
}

static char noteLabelBuffer[128][12];

#define MIDI_SLOT_LABEL(i) noteLabelBuffer[i]
#define MIDI_SLOT_LABELS_8(i) \
    MIDI_SLOT_LABEL(i), MIDI_SLOT_LABEL(i + 1), MIDI_SLOT_LABEL(i + 2), MIDI_SLOT_LABEL(i + 3), \
      MIDI_SLOT_LABEL(i + 4), MIDI_SLOT_LABEL(i + 5), MIDI_SLOT_LABEL(i + 6), MIDI_SLOT_LABEL(i + 7)

const char* const midiSlotSelectNoteLabels[128] = {
    MIDI_SLOT_LABELS_8(0),   MIDI_SLOT_LABELS_8(8),   MIDI_SLOT_LABELS_8(16),
    MIDI_SLOT_LABELS_8(24),  MIDI_SLOT_LABELS_8(32),  MIDI_SLOT_LABELS_8(40),
    MIDI_SLOT_LABELS_8(48),  MIDI_SLOT_LABELS_8(56),  MIDI_SLOT_LABELS_8(64),
    MIDI_SLOT_LABELS_8(72),  MIDI_SLOT_LABELS_8(80),  MIDI_SLOT_LABELS_8(88),
    MIDI_SLOT_LABELS_8(96),  MIDI_SLOT_LABELS_8(104), MIDI_SLOT_LABELS_8(112),
    MIDI_SLOT_LABELS_8(120),
};

const char* const midiSlotSelectOctaveLabels[11] = {
    noteLabelBuffer[0], noteLabelBuffer[12], noteLabelBuffer[24], noteLabelBuffer[36],
    noteLabelBuffer[48], noteLabelBuffer[60], noteLabelBuffer[72], noteLabelBuffer[84],
    noteLabelBuffer[96], noteLabelBuffer[108], noteLabelBuffer[120],
};

#undef MIDI_SLOT_LABELS_8
#undef MIDI_SLOT_LABEL

void midiSlotSelectRefreshLabels()
{
    static uint8_t previousMiddleC = 0xFF;
    const uint8_t middleC = mtConfig.midi.middleC;
    if (previousMiddleC == middleC)
        return;

    const int transposition = 12 * ((int)middleC - 2);
    for (int note = 0; note < 128; note++)
    {
        int displayedNote = note + transposition;
        if (displayedNote < 0)
            displayedNote = 0;
        else if (displayedNote > 127)
            displayedNote = 127;

        snprintf(noteLabelBuffer[note],
                 sizeof(noteLabelBuffer[note]),
                 "%d - %s",
                 note,
                 allNotes[displayedNote]);
    }

    previousMiddleC = middleC;
}

#endif // MOD_MIDI_SLOT_SELECT
