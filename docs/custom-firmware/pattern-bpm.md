# Pattern BPM (engine) — `MOD_PATTERN_BPM`

## Source files

**Modified (existing Tracker CE files, all changes behind `#ifdef MOD_PATTERN_BPM`):**

| File | Change |
|---|---|
| `core/source/mtStructs.h` | `PATTERN_FILE_VERSION` is 8 when the flag is set (6 otherwise) |
| `core/FileManager/fileManagerPattern.cpp` | `checkVersionAndTranslate()`: files older than v8 get Pattern BPM = OFF |
| `core/source/mtSequencer.h` | `kPatternBpmMin/Max`, `getPatternBpm()`, `setPatternBpm()` |
| `core/source/mtSequencer.cpp` | `getCurrentTempo()` falls back to the Pattern BPM; FX Off (`!`) cancels an active Tempo FX |

**New:** `docs/custom-firmware/pattern-bpm.md` (this file).
**Removed:** none.

## Summary

Each pattern can store its own starting BPM. OFF (the default) means the pattern uses the Project BPM. This change is the
engine and storage only: **it has no user interface**. The control that edits the value is provided by Pattern Settings
(`docs/custom-firmware/pattern-settings.md`), which depends on this flag.

## Behaviour

- Range 10–400 BPM in 1 BPM steps, plus OFF. Stored as a 16-bit little-endian value in the previously unused
  `strPattern::rezerwa2` (low byte) and `rezerwa3` (high byte). `0` = OFF.
- Tempo priority: active Tempo FX → Pattern BPM of the playing pattern → Project BPM.
- FX Off (`!`) cancels an active Tempo FX. The starting tempo returns automatically; neither the saved Pattern BPM nor the
  Project BPM is modified.
- Switching patterns keeps the stock behaviour: `switchRamPatternsNow()` already calls `cancelFxes()`, which resets the Tempo
  FX, so no additional reset was added. Looping the same pattern keeps the stock Tempo FX persistence.
- External clock sync ignores the Pattern BPM exactly as it ignores the Project BPM (`getCurrentTempo()` only consults it on
  the internal-clock / render / selection paths).

## File format and migration

- Pattern file version **8** when built with the flag; stock is 6. Version 7 is deliberately skipped and stays reserved for a
  possible Master Multiplier port (it would use `rezerwa1`).
- `checkVersionAndTranslate()` sets `rezerwa2/3 = 0` for every file below v8, after the existing translation chain and the
  existing `limitTrackLengths()` handling. The current upstream migration code was not replaced.
- Checked against the source: pattern size validation (`core/include/patternFileValidation.h`) is size-based for versions
  above 5, so v8 files are accepted. A flag-off build reads a v8 file, ignores the BPM bytes and keeps them in the reserved
  fields. A flag-on build never loses data when loading older files.
- Master Multiplier subset concern: if a v8 file written by this build were later loaded by a build that also carries a
  multiplier migration for `< v7`, that migration would be skipped. This is harmless today because stock firmware never
  writes `rezerwa1` (it is initialised to 0 and no other code touches it), so the multiplier's default (×1 = 0) is already
  what such files contain. Nothing in this repository uses `rezerwa1`.
- Files saved with the flag cannot be read by older stock firmware that rejects newer versions. Keep a backup of
  `Projects/` before testing.

## Build

```
make plus CUSTOM_DEFINES="-DMOD_PATTERN_BPM"     # also: original, mini
```
Run `make clean` when changing `CUSTOM_DEFINES`. Flag off = stock behaviour and file format.

## Dependencies

None. Required by: Pattern Settings (`MOD_PATTERN_SETTINGS`).

## Shared source files

`mtSequencer.cpp`, `mtSequencer.h`, `mtStructs.h` and `fileManagerPattern.cpp` are ordinary Tracker CE files that any feature
may touch. All edits here are small, bracketed by `#ifdef MOD_PATTERN_BPM`, and easy to reapply.

## Verification

See the pull request description for the builds that were actually run.

## Origin

Ported from the earlier Pattern BPM implementation (`SNAPSHOT/features-docs/pattern-bpm/IMPLEMENTATION.md` in the backup
repository). Differences: no Page 4 UI (it never existed in this repository), no redundant Tempo FX reset in
`switchRamPatternsNow()`, and the version/migration code was re-applied onto the current upstream migration chain.
