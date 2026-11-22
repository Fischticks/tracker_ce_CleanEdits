// Host tests for core/source/mtMidiSlotSelectMapping.h.
// Build: g++ -std=c++17 -Wall -Wextra -Icore/source tests/host/test_midiSlotSelectMapping.cpp -o /tmp/test_midiSlotSelectMapping && /tmp/test_midiSlotSelectMapping
#include "mtMidiSlotSelectMapping.h"

#include <cstdio>

static int failures = 0;
static int checks = 0;

#define CHECK(condition)                                                                  \
    do                                                                                    \
    {                                                                                     \
        checks++;                                                                         \
        if (!(condition))                                                                 \
        {                                                                                 \
            failures++;                                                                   \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);              \
        }                                                                                 \
    } while (0)

int main()
{
    // Drum and Key are intentionally identical chromatic maps.
    for (int note = 0; note <= 127; note++)
    {
        CHECK(midiSlotSelectMapRawNote(note, midiSlotSelectModeDrum, 36, 3, 64) ==
              midiSlotSelectMapRawNote(note, midiSlotSelectModeKey, 36, 3, 64));
    }
    CHECK(midiSlotSelectMapRawNote(36, midiSlotSelectModeDrum, 36, 3, 64) == 0);
    CHECK(midiSlotSelectMapRawNote(37, midiSlotSelectModeKey, 36, 3, 64) == 1);
    CHECK(midiSlotSelectMapRawNote(35, midiSlotSelectModeDrum, 36, 3, 64) == -1);
    CHECK(midiSlotSelectMapRawNote(99, midiSlotSelectModeDrum, 36, 3, 64) == 63);
    CHECK(midiSlotSelectMapRawNote(100, midiSlotSelectModeDrum, 36, 3, 64) == -1);
    CHECK(midiSlotSelectMapRawNote(102, midiSlotSelectModeDrum, 36, 3, 67) == 66);
    CHECK(midiSlotSelectMapRawNote(102, midiSlotSelectModeDrum, 36, 3, 64) == -1);
    CHECK(midiSlotSelectMapRawNote(127, midiSlotSelectModeDrum, 127, 3, 64) == 0);

    // White-key sequence: C, D, E, F, G, A, B; each octave advances seven slots.
    const uint8_t whiteNotes[7] = { 36, 38, 40, 41, 43, 45, 47 };
    const uint8_t blackNotes[5] = { 37, 39, 42, 44, 46 };
    for (int slot = 0; slot < 7; slot++)
    {
        CHECK(midiSlotSelectMapRawNote(whiteNotes[slot],
                                       midiSlotSelectModeWhiteKeysOnly,
                                       36,
                                       3,
                                       64) == slot);
        CHECK(midiSlotSelectMapRawNote(whiteNotes[slot] + 12,
                                       midiSlotSelectModeWhiteKeysOnly,
                                       36,
                                       3,
                                       64) == slot + 7);
    }
    for (uint8_t note : blackNotes)
        CHECK(midiSlotSelectMapRawNote(note, midiSlotSelectModeWhiteKeysOnly, 36, 3, 64) == -1);

    CHECK(midiSlotSelectMapRawNote(35, midiSlotSelectModeWhiteKeysOnly, 36, 3, 64) == -1);
    CHECK(midiSlotSelectMapRawNote(48, midiSlotSelectModeWhiteKeysOnly, 36, 3, 64) == 7);

    // Octave limits and the top of the MIDI note range.
    CHECK(midiSlotSelectMapRawNote(0, midiSlotSelectModeWhiteKeysOnly, 36, 0, 64) == 0);
    CHECK(midiSlotSelectMapRawNote(12, midiSlotSelectModeWhiteKeysOnly, 36, 0, 64) == 7);
    CHECK(midiSlotSelectMapRawNote(120, midiSlotSelectModeWhiteKeysOnly, 36, 10, 64) == 0);
    CHECK(midiSlotSelectMapRawNote(127, midiSlotSelectModeWhiteKeysOnly, 36, 10, 64) == 4);
    CHECK(midiSlotSelectMapRawNote(119, midiSlotSelectModeWhiteKeysOnly, 36, 10, 64) == -1);

    // Invalid inputs and unavailable instrument slots are rejected.
    CHECK(midiSlotSelectMapRawNote(128, midiSlotSelectModeDrum, 36, 3, 64) == -1);
    CHECK(midiSlotSelectMapRawNote(36, midiSlotSelectModeCount, 36, 3, 64) == -1);
    CHECK(midiSlotSelectMapRawNote(36, midiSlotSelectModeDrum, 36, 3, 0) == -1);

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
