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
make clean && make plus CUSTOM_DEFINES="-DMOD_PATTERN_BPM -DMOD_PATTERN_SETTINGS -DMOD_SAMPLE_BOUNDARY_CAPTURE -DMOD_LAZY_CHOP"   # explicitly select all defaults
make clean && make plus CUSTOM_DEFINES=""                          # no optional features (stock behaviour)
```

### Policy for optional features

1. Implement each feature behind its own dedicated definition and document the definition and what it does.
2. Add completed and accepted features to the default `CUSTOM_DEFINES` list. Keep a feature disabled by default only for a
   documented reason, such as experimental status, compatibility concerns, or resource constraints.
3. Keep feature definitions centralized in `custom-firmware.mk`; do not duplicate defaults in GitHub Actions or other makefiles.
4. Feature documents name the definition, its dependencies, and whether it is included in the default build.
5. The PR describes how to test the feature both enabled and disabled using command-line overrides.

## Features

| Feature | Definition | Default build | Requires | Document |
|---|---|---|---|
| Pattern BPM (engine, pattern file v8) | `MOD_PATTERN_BPM` | Enabled | – | [pattern-bpm.md](pattern-bpm.md) |
| Shift + Pattern screen: Pattern Settings (Length, Divider, BPM) | `MOD_PATTERN_SETTINGS` | Enabled | optional: `MOD_PATTERN_BPM` (adds the BPM slider) | [pattern-settings.md](pattern-settings.md) |
| Sample Playback Boundary Capture | `MOD_SAMPLE_BOUNDARY_CAPTURE` | Enabled | – | [sample-boundary-capture.md](sample-boundary-capture.md) |
| Lazy Chop | `MOD_LAZY_CHOP` | Enabled | – | [lazy-chop.md](lazy-chop.md) |
| Shift + Pattern screen: Quantize (live-record quantize, Step Quantize, Note Off, Bypass) | `MOD_SHIFT_PATTERN_QUANTIZE` | Enabled (approved by the user) | `MOD_PATTERN_SETTINGS`; optional `MOD_PATTERN_BPM` | [shift-pattern-quantize.md](shift-pattern-quantize.md) |

All five features are enabled in the default build for Original Tracker, Tracker+ and Tracker Mini. The exact default list is defined only in `custom-firmware.mk`.

**File-format warning:** with `MOD_PATTERN_BPM` the firmware saves pattern file version 8. Older stock firmware may not load projects
saved that way; keep a backup of `Projects/` before using such a build on existing projects.

## Dependencies

- Pattern BPM has no prerequisites.
- Boundary Capture and Lazy Chop have no prerequisites and are independent of each other and of the Pattern features. When either is
  enabled, `samplePlayback/previewSession.*` is compiled in as shared support.
- Pattern Settings works on its own (Length and Divider sliders). With `MOD_PATTERN_BPM` it also shows the BPM slider that edits
  the Pattern BPM. The engine does not depend on the screen.
- Quantize (`MOD_SHIFT_PATTERN_QUANTIZE`) adds the Quantize category to the Pattern Settings screen and therefore requires
  `MOD_PATTERN_SETTINGS`. It is independent of `MOD_PATTERN_BPM` and of Sample Playback Markers.

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
| `core/source/mtSequencer.cpp` | Pattern BPM, Quantize |
| `core/source/mtSequencer.h` | Pattern BPM, Quantize |
| `core/source/mtSequencer_patternEdit.cpp` | Quantize (`setSelectionNote`) |
| `core/FileManager/WorkspaceProjectLoad.cpp` | Quantize (settings reset) |
| `core/interface/shiftPattern/*` | Pattern Settings, Quantize (category list and dispatch) |
| `core/source/mtStructs.h` | Pattern BPM |
| `core/FileManager/fileManagerPattern.cpp` | Pattern BPM |
| `core/interface/patternEditor/fPatternEdit.cpp`, `patternEditor.h` | Pattern Settings, Quantize (`fPatternEdit.cpp` cursor advance) |
| `core/interface/core/interface.cpp`, `interfaceDefs.h` | Pattern Settings |

Sample Playback Markers (Boundary Capture and Lazy Chop have independent definitions; both compile `samplePlayback/previewSession.*` when either is on):

| File | Features |
|---|---|
| `core/interface/samplePlayback/fSamplePlay.cpp` | Boundary Capture, Lazy Chop (one `sampleMarkers::` hook surface; Add Slice call is Lazy Chop only) |
| `core/source/mtSequencer.h/.cpp` | Boundary Capture, Lazy Chop (shared `getManualNoteBinding()`), also Pattern BPM (different regions) |
| `core/AudioEngine/mtAudioEngine.h/.cpp` | Boundary Capture, Lazy Chop (shared `isSamplePlaying/isSampleReversed`; capture adds two more) |
| `core/libs/Audio/play_memory.h` | Boundary Capture, Lazy Chop (shared `isReverse()`) |
| `core/source/mtSliceManager.*`, `core/AudioEngine/VoiceStreamManager.*` | Lazy Chop |

The Pattern features are not modified by Sample Playback Markers (`mtSequencer.*` is a shared file, edited in different regions).

No file is modified by both Pattern features. Pattern Settings reads the Pattern BPM only through the accessors Pattern BPM adds to
`mtSequencer.h`.
