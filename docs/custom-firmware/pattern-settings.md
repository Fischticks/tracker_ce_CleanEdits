# Shift + Pattern → Pattern Settings — `MOD_PATTERN_SETTINGS`

## Source files

**New (feature-specific, compiled only with the flag):**

| File | Purpose |
|---|---|
| `core/interface/shiftPattern/shiftPattern.h` | `cShiftPattern` module class (the Shift + Pattern screen) |
| `core/interface/shiftPattern/fShiftPattern.cpp` | input handling, access to the underlying settings |
| `core/interface/shiftPattern/gShiftPattern.cpp` | display controls and drawing |
| `docs/custom-firmware/pattern-settings.md` | this document |

**Modified (existing Tracker CE files):**

| File | Change | Guard |
|---|---|---|
| `core/interface/core/interfaceDefs.h` | new event `eventActivateShiftPattern` | `MOD_PATTERN_SETTINGS` |
| `core/interface/core/interface.cpp` | include, and the handler for that event (deactivate the caller, activate the screen) | `MOD_PATTERN_SETTINGS` |
| `core/interface/patternEditor/fPatternEdit.cpp` | `functPattern()`: Shift + Pattern raises the event. `setActualPatternLength()` split in two (see Deviations) | the key handler is guarded; the split is not |
| `core/interface/patternEditor/patternEditor.h` | declaration of `applyActualPatternLength()` | not guarded |
| `docs/custom-firmware/README.md` | overview updated | – |

**Removed:** none. (The old Pattern BPM control on Pattern Editor Page 4 does not exist in this repository, so there is nothing to remove.)

## Summary

Shift + Pattern, pressed from the Pattern Editor, opens a category screen laid out like the Sample Recorder config screen.
The category list is in column 0; the other seven columns operate the selected category. The only category today is
**Pattern Settings**, with three value sliders:

| Column | Control | Range | Underlying setting (no copy is kept) |
|---|---|---|---|
| 1 | Length | 1–128 steps | active pattern length (`track[].length`, stored zero based) through the Pattern Editor's own setter |
| 2 | Divider | 1–16 | project setting `mtProject.values.patternDiv` (stored zero based, default 4) |
| 3–6 | empty | – | reserved for future controls |
| 7 | BPM | OFF, 10–400 | per-pattern Pattern BPM (`Sequencer::getPatternBpm/setPatternBpm`) |

The original Pattern Editor Length control (Page 1) and the Settings › Pattern divider item are unchanged, and both read and
write the same state, so a change made in either place shows up in the other.

## Behaviour

- Each control shows its name, the value directly under it and a bar. The value text is exact (integers).
- Buttons 0–7 select a column (empty columns are ignored). Pressing button 0 while it is selected steps to the next category.
  Left/Right move between the selectable columns (no wrap-around). Up/Down and the encoder change the selected value by 1 per
  step; in column 0 they walk the category list. Pressing the Pattern button (or any other module button) leaves the screen.
- The category selection is kept while the firmware runs (it is a member of the module object), so returning to the screen
  shows the category that was last used.
- The screen polls its three values, so a pattern change while playing (or any other change) is reflected without reopening.
- Pattern BPM: OFF is its own state. From OFF the first step up starts at the Project BPM; stepping down from 10 returns to
  OFF. Changes mark the pattern as changed. Tempo priority (Tempo FX, then Pattern BPM, then Project BPM), Tempo FX behaviour
  and external-clock behaviour are those of the engine (`pattern-bpm.md`) and are not touched here.
- Pattern divider: the Pattern Editor reads `patternDiv` into its cached `trackerPattern.stepDevider` every time it is
  opened, so the divider drawn in the editor is current when you return to it; no reload is needed.

## Build and dependencies

Definition: `MOD_PATTERN_SETTINGS`. Whether it is part of the default firmware is set in `custom-firmware.mk` (see
[README.md](README.md)); this document does not repeat the default. Temporary combinations (run `make clean` first; an override
replaces the whole default list for that build):
```
make clean && make plus CUSTOM_DEFINES="-DMOD_PATTERN_SETTINGS"                          # Length + Divider
make clean && make plus CUSTOM_DEFINES="-DMOD_PATTERN_SETTINGS -DMOD_PATTERN_BPM"        # + BPM slider (pattern file v8)
```

- `MOD_PATTERN_SETTINGS` is independent. The BPM slider exists only when `MOD_PATTERN_BPM` is also defined; the engine does
  not depend on this screen, and this screen does not require the engine.
- Flag off: the new files compile to nothing and the guarded edits disappear; behaviour is stock Tracker CE.

## Category order and Quantize

Quantize (Live Record) is a separate feature that is not part of this firmware yet. The intended order is
**Quantize, then Pattern Settings**. The category table is deliberately a one-line list (`categoryNames[]` in
`gShiftPattern.cpp`, `Category` enum in `shiftPattern.h`): adding Quantize means inserting it at index 0 and
branching on `category` where the screen draws and handles its columns. No placeholder entry is shown today.

## Integration points and future Tracker CE updates

- One new directory, three new files; no existing screen's layout, handlers or tables were modified. The module is not added
  to `Interface::modules[]` (that table only maps hardware buttons to screens); it is entered through a new event, in the
  same way as the Image Viewer.
- Existing source edits are small, bracketed by `#ifdef MOD_PATTERN_SETTINGS` where possible, and independent of each other:
  an event constant, an event case, a three-line key-handler hook, and one method split.
- Unavoidable couplings: the `functPattern()` hook in `fPatternEdit.cpp`, and `applyActualPatternLength()`, which must be
  kept in step with `setActualPatternLength()` if upstream changes what that function does.

## Deviations from the specification

1. **Length is applied through `applyActualPatternLength()`, not `setActualPatternLength()` directly.** The specification says to
   use the exact-set method. That method also redraws Pattern Editor controls (`showLength()`, `refreshPattern()`), which are
   not valid while another screen is active (display controls are rebuilt on every screen change). The method was therefore
   split: `applyActualPatternLength()` contains all of its data effects (all eight sample tracks, cached length, cursor
   clamp, pattern-changed flag) and `setActualPatternLength()` now calls it and then redraws. Callers of the original method
   behave exactly as before.
2. **Labels are "Length", "Divider" and "BPM"**, not "Pattern Divider" / "Pattern BPM". The columns are 94 px wide and the
   two-line labels use the small font; the longer names are unlikely to fit. Not verified on hardware; they are three string
   constants in `gShiftPattern.cpp`.
3. **Only Pattern Settings is present as a category** (see above), so it is currently the first and only entry rather than the
   second.
4. **No Page 4 BPM UI to relocate or remove**: this repository never had one.
5. Values are shown in decimal. The Pattern Editor's own length display follows the hex/decimal numbering option; this screen
   does not.
6. Not implemented because the specification does not ask for them: transport (Play/Stop) and pad handling on this screen.

## Verification

See the pull request description for the builds and checks that were actually run. Behaviour on hardware has not been tested.
