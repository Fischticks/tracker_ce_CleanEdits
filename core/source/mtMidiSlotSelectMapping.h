#ifndef SOURCE_MTMIDISLOTSELECTMAPPING_H_
#define SOURCE_MTMIDISLOTSELECTMAPPING_H_

#include <stdint.h>

enum enMidiSlotSelectMode : uint8_t
{
    midiSlotSelectModeDrum = 0,
    midiSlotSelectModeKey,
    midiSlotSelectModeWhiteKeysOnly,
    midiSlotSelectModeCount
};

static constexpr uint8_t kMidiSlotSelectConfigMarker = 0x5A;
static constexpr uint8_t kMidiSlotSelectDefaultStartNote = 36;
static constexpr uint8_t kMidiSlotSelectDefaultWhiteKeyOctave = 3;

// Pure mapping helper shared by firmware and host tests. Returns -1 for an
// invalid MIDI pitch, unsupported mode, black key, or unavailable instrument.
inline int16_t midiSlotSelectMapRawNote(uint8_t rawNote,
                                        uint8_t mode,
                                        uint8_t startNote,
                                        uint8_t whiteKeyOctave,
                                        int instrumentCount)
{
    if (rawNote > 127 || instrumentCount <= 0)
        return -1;

    int slot = -1;
    if (mode == midiSlotSelectModeWhiteKeysOnly)
    {
        static constexpr int8_t whiteKeyIndex[12] = {
            0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6
        };

        const int baseNote = 12 * whiteKeyOctave;
        if (rawNote < baseNote)
            return -1;

        const int offset = rawNote - baseNote;
        const int whiteIndex = whiteKeyIndex[offset % 12];
        if (whiteIndex < 0)
            return -1;

        slot = (offset / 12) * 7 + whiteIndex;
    }
    else if (mode == midiSlotSelectModeDrum || mode == midiSlotSelectModeKey)
    {
        if (rawNote < startNote)
            return -1;

        // Drum and Key intentionally share the same chromatic mapping.
        slot = rawNote - startNote;
    }
    else
    {
        return -1;
    }

    if (slot < 0 || slot >= instrumentCount)
        return -1;

    return (int16_t)slot;
}

#endif /* SOURCE_MTMIDISLOTSELECTMAPPING_H_ */
