# Custom firmware features — overview

Optional features added on top of Tracker CE. Each is a build-time flag, off by default; with every flag off the firmware is
unchanged Tracker CE. Each feature has its own document in this folder.

## Building with features

Flags are plain `-D` defines passed through the existing `CUSTOM_DEFINES` hook (see `docs/developer_guide.md`):

```
make plus CUSTOM_DEFINES="-DMOD_PATTERN_BPM"                         # also: original, mini
make clean                                                           # required when the flags change
```

## Features

| Feature | Flag | Default | Requires | Document |
|---|---|---|---|---|
| Pattern BPM (engine, pattern file v8) | `MOD_PATTERN_BPM` | off | – | [pattern-bpm.md](pattern-bpm.md) |
| Shift + Pattern screen: Pattern Settings (Length, Divider, BPM) | `MOD_PATTERN_SETTINGS` | off | optional: `MOD_PATTERN_BPM` (adds the BPM slider) | [pattern-settings.md](pattern-settings.md) |

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
