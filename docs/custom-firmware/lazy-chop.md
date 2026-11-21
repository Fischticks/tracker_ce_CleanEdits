# Lazy Chop — `MOD_LAZY_CHOP`

## Source files

**New:**

| File | Purpose |
|---|---|
| `core/interface/samplePlayback/lazyChop.h`, `lazyChop.cpp` | session handling, pending positions, commit, UI refresh |
| `core/interface/samplePlayback/previewSession.h`, `previewSession.cpp` | preview-session mechanism shared with Boundary Capture |
| `core/interface/samplePlayback/sampleMarkers.h` | hook surface used by `fSamplePlay.cpp` (shared) |
| `tests/host/test_sliceInsert.cpp`, `tests/host/run_sliceInsert.sh` | host test of the batch insert, run against the real function text |
| `docs/custom-firmware/lazy-chop.md` | this document |

**Modified:**

| File | Change | Guard |
|---|---|---|
| `core/source/mtSliceManager.h`, `mtSliceManager.cpp` | new `insertSlicesAtPositions()`; `addSlice()` untouched | `MOD_LAZY_CHOP` |
| `core/AudioEngine/VoiceStreamManager.h`, `VoiceStreamManager.cpp` | `requestInstrumentSliceCacheRefresh()` + `servicePendingSliceCacheRefresh()` called from `update()` | `MOD_LAZY_CHOP` |
| `core/AudioEngine/mtAudioEngine.h`, `mtAudioEngine.cpp` | `isSamplePlaying()`, `isSampleReversed()` (shared) | either feature |
| `core/libs/Audio/play_memory.h` | read-only `isReverse()` (shared) | either feature |
| `core/source/mtSequencer.h`, `mtSequencer.cpp` | read-only `getManualNoteBinding()` (shared) | either feature |
| `core/interface/samplePlayback/fSamplePlay.cpp` | the same one-line `sampleMarkers::` calls as Boundary Capture, plus the Add Slice call in `functAddSlice()` | no-op inlines without flags |
| `docs/custom-firmware/README.md` | overview | – |

**Removed:** none. **Format:** no instrument/project format change.

## Original vs. this implementation

| | Original (`jamartinez4991/Tracker_Claude_Backup`, `mod/lazy-chop`, doc `claude/features/lazy-chop/IMPLEMENTATION.md`) | This PR |
|---|---|---|
| Baseline | older upstream (no `kNoInstrument`, `readRaw`, `voiceIsStereo`) | current Fischticks `main`; the old patch does not apply, nothing copied wholesale |
| Session state | a `LazyChopSession` struct inside `cSamplePlayback` | `PreviewSession` (shared helper) + feature state in `lazyChop.cpp`; `samplePlayback.h` is **not** modified |
| Hooks | `#ifdef` blocks inside `fSamplePlay.cpp` | single-line calls through `sampleMarkers.h` |
| `insertSlicesAtPositions()` | original | **same algorithm**, re-read against this baseline's `addSlice()` (zero boundary, 48 limit, selection) and host-tested |
| Deferred streamed-cache refresh | original | **same design** re-applied to this baseline's `VoiceStreamManager` (checked: `voices[].active/instrIdx`, `currentJob.inProgress`, `isAsyncReadBusy()` exist) |
| Behaviour | as below | unchanged |

## Behaviour (preserved)

* A session starts when an eligible preview note starts: Slice or Beat Slice mode, normal layout, a free session, the sequencer track
  really holds the open manual note for that source and instrument, the voice is playing **forward** (reversed notes never start a
  session).
* Add Slice (button 2 or Insert, both through `functAddSlice()`) during a session stores the absolute playhead position as pending;
  nothing else changes. Exact duplicates are ignored; existing boundaries + pending never exceed 48. A reversed voice consumes the press
  without capturing. If the binding became invalid the session is finalised and the press falls through to the ordinary midpoint Add.
  No session → ordinary Add, unchanged.
* Finalisation (idempotent): initiating pad released; natural end, cut or replacement of the voice; instrument change; layout toggle;
  transport play; play-mode change; leaving the screen. It always commits to the instrument bound at start (an out-of-range index
  discards). Pending markers are never dropped just because the audition was interrupted.
* Commit: `insertSlicesAtPositions()` keeps existing boundaries, sorts, rejects exact duplicates and 65535, caps at 48, selects the last
  inserted marker, leaves the instrument untouched if nothing was inserted. Close but distinct positions are allowed.
* Streamed samples: the cache refresh is requested, not executed; it runs from `VoiceStreamManager::update()` only when no streaming job
  is in progress, no asynchronous SD read is outstanding, and no active voice uses that instrument. (Conservative: any in-progress job
  blocks every instrument.) Ordinary Add/Remove still refresh immediately, as before.

## Independence from Boundary Capture

Own `PreviewSession`, own pending list, own commit. Lazy Chop only starts in Slice/Beat Slice, Boundary Capture only in the four
non-slice modes, so a note can belong to at most one of them; neither reads or commits the other's state.

## Limitations / not verified

* Not run on hardware or a simulator: lifecycle cases (interruption, voice replacement, layout change), streamed samples and the UI
  redraw are verified by reading the code and compiling only.
* Notes from an external MIDI keyboard that bypass `cSamplePlayback::noteOnHandle()` do not start a session.
* Slice-table writes are not synchronised with the audio ISR beyond what ordinary Add already does.
* Pending markers are not drawn until committed.

## Verification

Host: `tests/host/run_sliceInsert.sh` (13 checks: ordering, zero boundary, duplicates inside the batch and against existing, 65535,
closely spaced, 48 cap, full table, selection unchanged when nothing inserted). Builds: see the PR description.
