# Custom firmware features — overview

Optional features added on top of Tracker CE. Each is a compile-time definition (`-DMOD_…`). With a feature's definition absent, that
feature is compiled out and the firmware behaves like stock Tracker CE for it. Each feature has its own document in this folder.

## Configuration: `custom-firmware.mk`

`custom-firmware.mk` in the repository root is the **single source of truth** for which optional features are in the default firmware.
The root `Makefile` loads it and exports `CUSTOM_DEFINES`, so the same definitions reach every recursive build (core, platform and
synth) for **Original Tracker, Tracker+ and Tracker Mini**. GitHub Actions runs plain `make all`, so CI uses the same configuration
automatically; no workflow lists feature flags and none needs editing when a feature is added.

- **Enable** a feature: add its `-D` line to `CUSTOM_DEFINES` in `custom-firmware.mk`.
- **Disable** it: remove (or comment out) the line.
- **Always `make clean` after changing the definitions** (in the file or on the command line). The makefiles do not track compiler
  flags, so stale object files would produce an inconsistent mix of features.

```
make clean
make all            # all targets with the configuration in custom-firmware.mk
make clean
make plus           # one target (also: original, mini)
```

### Temporary overrides

A command-line `CUSTOM_DEFINES` replaces the whole default list for that build only and does not edit `custom-firmware.mk`
(note: it is not additive, and a `CUSTOM_DEFINES` set in your shell environment overrides the file as well):

```
make clean && make plus CUSTOM_DEFINES="-DMOD_PATTERN_SETTINGS"   # Settings without Pattern BPM
make clean && make plus CUSTOM_DEFINES=""                          # no optional features (stock behaviour)
```

### Policy for new optional features

1. Implement the feature behind its own dedicated definition and document the definition and what it does.
2. Add a **commented-out** entry for it to `custom-firmware.mk`. Only the project owner uncomments it; a feature PR never enables
   itself by default.
3. Do not add feature definitions to GitHub Actions, to other makefiles, or to another configuration file.
4. Feature documents name the definition and its dependencies but do not state a default; they point here.
5. The PR describes how to test the feature enabled and disabled (command-line overrides).

## Features

| Feature | Definition | Requires | Document |
|---|---|---|---|
| Pattern BPM (engine, pattern file v8) | `MOD_PATTERN_BPM` | – | [pattern-bpm.md](pattern-bpm.md) |
| Shift + Pattern screen: Pattern Settings (Length, Divider, BPM) | `MOD_PATTERN_SETTINGS` | optional: `MOD_PATTERN_BPM` (adds the BPM slider) | [pattern-settings.md](pattern-settings.md) |

Which of these are enabled by default is defined only in `custom-firmware.mk`.

**File-format warning:** with `MOD_PATTERN_BPM` the firmware saves pattern file version 8. Older stock firmware may not load projects
saved that way; keep a backup of `Projects/` before using such a build on existing projects.

## Dependencies

- Pattern BPM has no prerequisites.
- Pattern Settings works on its own (Length and Divider sliders). With `MOD_PATTERN_BPM` it also shows the BPM slider that edits
  the Pattern BPM. The engine does not depend on the screen.

## Pattern file versions

| Version | Meaning |
|---|---|
| 6 | stock Tracker CE |
| 7 | reserved (Master Multiplier, not part of this firmware) |
| 8 | `MOD_PATTERN_BPM`: `strPattern::rezerwa2/3` hold the Pattern BPM |

## Source files touched by more than one feature

The table is updated whenever another feature edits one of these files.

| File | Features |
|---|---|
| `core/source/mtSequencer.cpp` | Pattern BPM |
| `core/source/mtSequencer.h` | Pattern BPM |
| `core/source/mtStructs.h` | Pattern BPM |
| `core/FileManager/fileManagerPattern.cpp` | Pattern BPM |
| `core/interface/patternEditor/fPatternEdit.cpp`, `patternEditor.h` | Pattern Settings |
| `core/interface/core/interface.cpp`, `interfaceDefs.h` | Pattern Settings |

No file is modified by both features. Pattern Settings reads the Pattern BPM only through the accessors Pattern BPM adds to
`mtSequencer.h`.
