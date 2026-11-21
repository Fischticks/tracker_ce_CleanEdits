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

## Dependencies

- Pattern BPM has no prerequisites. A feature that edits the value (Pattern Settings) lists it as an optional dependency.

## Pattern file versions

| Version | Meaning |
|---|---|
| 6 | stock Tracker CE |
| 7 | reserved (Master Multiplier, not part of this firmware) |
| 8 | `MOD_PATTERN_BPM`: `strPattern::rezerwa2/3` hold the Pattern BPM |

## Source files touched by more than one feature

None yet. This table is updated whenever a second feature edits a file another feature already modifies.

| File | Features |
|---|---|
| `core/source/mtSequencer.cpp` | Pattern BPM |
| `core/source/mtSequencer.h` | Pattern BPM |
| `core/source/mtStructs.h` | Pattern BPM |
| `core/FileManager/fileManagerPattern.cpp` | Pattern BPM |
