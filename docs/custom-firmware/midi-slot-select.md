# MIDI Slot Select — `MOD_MIDI_SLOT_SELECT`

**Status:** Implemented on `feature/midi-slot-select`; pull-request build verification pending.  
**Default build:** Enabled. The runtime setting defaults Off.  
**Dependencies:** None.

## Summary

MIDI Slot Select maps incoming USB or DIN MIDI pitches to instrument slots while the Pattern Editor is active, sequencer MIDI voice mode is selected, and the Instrument column is selected.

The feature emulates the existing physical-pad behavior in that column. Incoming MIDI pitch chooses the slot; the pattern note is the source-defined `Sequencer::STEP_NOTE_DEFAULT` (C5), not the incoming pitch.

## Settings

All four settings are flat entries in the existing MIDI Settings group and are stored globally in the MIDI configuration, not in project data.

| Setting | Values | Default |
|---|---|---|
| MIDI Slot Select | Off / On | Off |
| Slot Select Mode | Drum / Key / White Keys Only | Drum |
| Drum/Key Start | MIDI note 0–127 | 36 |
| White Key Octave | C0–C10 | C3 |

The Drum and Key modes use the same chromatic mapping. White Keys Only uses the configured C octave as its base and maps seven white keys per octave.

## Mapping

- **Drum and Key:** `slot = rawMidiNote - Drum/Key Start`. Each semitone advances one slot.
- **White Keys Only:** the twelve-semitone lookup is `{0,-1,1,-1,2,3,-1,4,-1,5,-1,6}`; black keys (`-1`) are invalid. Each octave advances seven slots.
- White Key Octave 0 begins at MIDI C0 (note 0); octave 10 begins at MIDI C10 (note 120).
- A note below the configured base, a black key in White Keys Only mode, an invalid mode, or a slot outside the current `kTotalInstrumentsCount` is rejected.
- The current target has 64 slots without `FEATURE_SYNTH` and 67 with it.

Mapping uses raw MIDI pitch before Middle C calibration. The calibration is not applied to the fixed C5 note passed through the pad-style recording path.

## Event behavior

The feature handles events only when all activation conditions are true: compile-time feature enabled, runtime setting On, sequencer MIDI voice mode, Pattern Editor active, and Instrument column selected. USB and DIN callbacks use the same mapping and dispatch logic. Outside that context, normal MIDI routing is unchanged.

For a valid Note-On:

1. Map the raw MIDI pitch to a slot and update `mtProject.values.lastUsedInstrument`.
2. If the recorder is active (`sequencer.isRec()`), use the existing pad-specific route:
   `handleNoteOn(GRID_INSIDE_PATTERN_INSTRUMENT, STEP_NOTE_DEFAULT, incomingVelocity, mappedSlot)`.
   Existing velocity configuration and recording/microtiming behavior remain in control.
3. If the recorder is inactive, reuse the stopped-edit physical-pad selection operation. On MIDI tracks, the existing pad conversion is `MIDI_INSTRUMENTS_START_IDX + (slot % kMidiInstrumentsCount)`; on other tracks, the mapped slot is passed directly to `setSelectionInstrument()`. Its native selected-step side effects are preserved.

For a valid Note-Off while the recorder is active, use the corresponding pad-specific `handleNoteOff(GRID_INSIDE_PATTERN_INSTRUMENT, STEP_NOTE_DEFAULT, 0, mappedSlot)` route. That route performs existing release/preview cleanup and suppresses pattern Note-Off marker insertion. When the recorder is inactive, the release event has no selection action, matching the stopped physical-pad path.

Invalid mappings are consumed while the feature is active; they do not fall through to ordinary MIDI note entry or select another instrument.

## Configuration compatibility

Six bytes are appended unconditionally to the global MIDI configuration: enable, mode, Drum/Key Start, White Key Octave, initialization marker, and reserved byte. Compile-time assertions require a 20-byte MIDI configuration structure and verify the extension offsets. The storage layout is the same with the feature flag on or off.

The marker is `0x5A`. Older size-prefixed configurations without the extension receive defaults when the marker is absent. The legacy recovery path's fixed 18-byte MIDI read explicitly invalidates the marker before configuration validation, preventing unrelated legacy bytes from being interpreted as initialized settings. `CONFIG_STRUCT_VER` is unchanged; the feature adds no project-data fields or project-file version change.

## Build and tests

The feature is enabled by default through `custom-firmware.mk`. To test the default configuration:

```sh
make clean
make all
```

Because command-line `CUSTOM_DEFINES` replaces the complete default list, this is the equivalent feature-disabled build while retaining the other current defaults:

```sh
make clean
make all CUSTOM_DEFINES="-DMOD_PATTERN_BPM -DMOD_PATTERN_SETTINGS -DMOD_SAMPLE_BOUNDARY_CAPTURE -DMOD_LAZY_CHOP -DMOD_SHIFT_PATTERN_QUANTIZE"
```

The pure mapping helper has a host test:

```sh
g++ -std=c++17 -Wall -Wextra -Icore/source tests/host/test_midiSlotSelectMapping.cpp -o /tmp/test_midiSlotSelectMapping
/tmp/test_midiSlotSelectMapping
```

The pull-request workflow is expected to build Original Tracker, Tracker+, and Tracker Mini with the default feature set. Hardware behavior has not been validated.

## Source files

| File | Responsibility |
|---|---|
| `core/source/mtMidiSlotSelectMapping.h` | Pure raw-pitch mapping function, shared with host tests |
| `core/source/mtMidiSlotSelect.h/.cpp` | Configuration defaults/validation, activation predicate, mapping wrapper, dynamic note labels |
| `core/source/mtStructs.h` | Unconditional global MIDI settings extension and layout assertions |
| `core/source/mtConfig.cpp` | Configuration initialization and legacy recovery marker invalidation |
| `core/source/mtMidi.cpp` | Shared USB/DIN interception before ordinary sequencer note handling |
| `core/interface/configEditor/configMenu.cpp`, `configMenuDefs.h` | Four flat MIDI Settings controls |
| `core/interface/configEditor/gConfigEdit.cpp` | Refresh calibrated note labels when the Settings editor opens |
| `custom-firmware.mk` | Default feature flag |
| `tests/host/test_midiSlotSelectMapping.cpp` | Mapping boundary and mode tests |

## Scope and limitations

- No feature-specific held-note ownership tracker or generalized MIDI source/channel ownership refactor is added. Pad-style Note-Off behavior uses the existing sequencer source semantics.
- No separate edit-mode microtiming system is added. Recorder-active events use the existing recording path.
- No pattern Note-Off marker is added for the pad-specific route.
- No sequencer-wide refactor or project-file format change is included.

Build, automated host-test, and hardware-validation status must be updated from actual results; a PR being open is not evidence that the firmware build passed.
