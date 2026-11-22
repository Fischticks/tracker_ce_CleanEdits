# Shift + Pattern → Quantize — `MOD_SHIFT_PATTERN_QUANTIZE`

**Status: implemented, compiled; NOT tested on hardware or in an emulator. Enabled by default** (the user approved enabling new
features by default on 2026-11-22; the flag stays separately switchable). Requires `MOD_PATTERN_SETTINGS` (compile-time `#error` otherwise).
Works with or without `MOD_PATTERN_BPM`.

## Source files

**New (feature-specific, compiled only with the flag):**

| File | Purpose |
|---|---|
| `core/source/quantizeGrid.h` | Pure grid math (no firmware dependencies, host-testable): division table, interval, nearest grid position, Strength, micromove conversion |
| `core/source/quantizeSettings.h` | Runtime-only settings (stored values) + the *effective* getters that apply Bypass |
| `core/interface/shiftPattern/gShiftPatternQuantize.cpp` | The Quantize category: controls, drawing, editing, Bypass dimming |
| `tests/host/test_quantizeGrid.cpp`, `tests/host/test_stepQuantize.cpp` | Host tests (see Tests) |
| `docs/custom-firmware/shift-pattern-quantize.md` | this document |

**Modified (existing Tracker CE files), all under `#ifdef MOD_SHIFT_PATTERN_QUANTIZE`:**

| File | Change |
|---|---|
| `core/source/mtSequencer.h` | `recQuantFwdSteps` in the player track; declarations of `quantizeNoteOn`, `stepQuantizeTarget`, `takeStepQuantizeAdvance`, `cancelStepQuantizeAdvance`, and the Step Quantize state |
| `core/source/mtSequencer.cpp` | live-record hook in `handleNoteOn` (computes destination step + micromove); Note Off marker gate in both `handleNoteOff` paths; forward-quantize guard in `getDestRecordingStep`; Step Quantize functions; Step Quantize in MIDI edit-mode note entry |
| `core/source/mtSequencer_patternEdit.cpp` | `setSelectionNote()` (pad / popup note entry) places the note through `stepQuantizeTarget` |
| `core/interface/patternEditor/fPatternEdit.cpp` | `moveCursorByStep()` / `moveCursorByStepParallel()` (no-argument forms) go to the next grid position after a quantized note; `cancelPopups()` drops the pending advance |
| `core/interface/shiftPattern/shiftPattern.h`, `fShiftPattern.cpp`, `gShiftPattern.cpp` | flag-aware category list (Quantize first), per-category redraw, dispatch to the new file |
| `core/FileManager/WorkspaceProjectLoad.cpp` | `quantizeSettings::resetToDefaults()` after a project is loaded |
| `custom-firmware.mk` | `-DMOD_SHIFT_PATTERN_QUANTIZE` added to the default `CUSTOM_DEFINES` |
| `docs/custom-firmware/README.md` | overview |

**Removed:** none.

## Screen (Sample Recorder layout)

Column 0 is the category selector (**Quantize**, **Pattern**; the screen title reads "Quantize" / "Pattern Settings"). Quantize
is the first category, so Pattern Settings is the second entry (default category without the flag is Pattern Settings, as before).
Changing the category redraws the whole screen. The selected category is kept while the firmware runs.

| Column | Control | Type | Range / default |
|---|---|---|---|
| 1 | Note Off | toggle | On |
| 2 | Quantize | toggle | On |
| 3 | Step Quantize (label "Step Quant") | toggle | Off |
| 4 | Quantize Division (label "Division") | list | 1/4D, 1/4, 1/8D, 1/4T, 1/8, 1/16D, 1/8T, 1/16; default 1/16 |
| 5 | Quantize Strength (label "Strength") | bar | 0–100, default 100 |
| 6 | Quarter Note Length (label "Quarter Len") | bar | 4–16, default 4 |
| 7 | Bypass | toggle | Off |

Up/Down/encoder edit the selected column; a toggle is set On by an increase and Off by a decrease. Bypass dims (theme inactive
colours) and locks Step Quantize, Division, Strength and Quarter Note Length; their stored values stay visible and unchanged.
Note Off and Quantize stay editable. Labels are abbreviated because the column is 100 px wide.

## Behaviour

* Grid: interval in ticks (48 ticks = one step) for division D and quarter length Q: straight `4Q/D`, triplet `8Q/(3D)`, dotted
  `6Q/D` steps; the grid starts at the pattern start and restarts at each loop. A note is stored as a step + micromove; every
  tick offset 1..47 is exactly representable.
* **Quantize (live recording):** the incoming position `48·step + (uStep−1)` moves Strength % toward the nearest grid position
  (ties go to the later one) and is stored with a micromove, independent of the Microtiming Detection option. Quantize off →
  the legacy path (`recMicrotiming()`) runs untouched. Beat-1 anticipation is not quantized. Occupied destination (note or FX)
  discards the write, as in the base firmware's recording path.
* **Note Off:** gates only the creation of Note Off markers; release/cleanup still run. Markers are not grid-quantized. A guard
  prevents a marker landing before a note-on that Quantize moved forward.
* **Step Quantize (manual note entry):** the note goes to the grid position nearest the cursor (ties forward) at full strength
  (Strength applies to live recording only); the cursor then advances to the step holding the next grid position instead of by
  Step Jump (wraps at the pattern end). The exact next position is remembered so that grids not aligned to steps stay exact.
  Covers pad/keyboard entry in the editor, the note popup commit and MIDI-keyboard entry in edit mode (chords use the same
  target on every track). Non-note entry (instrument, FX, volume, paste) is unchanged. An occupied target step is never
  overwritten (the note is discarded, the cursor still advances).
* **Bypass:** effective-value override only (`quantizeSettings::effective*`); stored values are never rewritten.
* **Persistence:** none. Runtime state, reset on project load/reload and at power-up. No file-format change.
* Existing pattern data, playback timing, BPM, pattern length and step duration are untouched.

## Deviations / decisions to review

* Occupied-destination rule is "discard the incoming write" (Addendum 1) for both recording and Step Quantize.
* Labels abbreviated ("Step Quant", "Division", "Strength", "Quarter Len") for the 100 px column.
* Step Quantize never overwrites an existing note, so retyping a note on an occupied target step does nothing under Step Quantize.
* Wrap-around of the category list is left as in Pattern Settings (the category button wraps; Up/Down clamp).

## Tests

* Host (`g++ -std=c++17 -Wall -Icore/source tests/host/test_quantizeGrid.cpp` and `test_stepQuantize.cpp`): grid math and the
  Step Quantize cursor chain over all divisions, Q = 4..16 and many pattern lengths.
* Firmware builds: see the PR description (compile/link only).
* **Not performed:** hardware, emulator, UI rendering, real MIDI/pad recording.

## Limitations

* Everything on-device is untested: category redraw, dimming, list rendering of the 8 divisions, real recording timing.
* Step Quantize on a multi-track selection or range paste is deliberately untouched.
* The default list (all five flags together) was not build-tested; each flag was built in the combinations listed in the PR.
