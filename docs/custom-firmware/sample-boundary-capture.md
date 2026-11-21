# Sample Playback Boundary Capture — `MOD_SAMPLE_BOUNDARY_CAPTURE`

> **Default build:** `MOD_SAMPLE_BOUNDARY_CAPTURE` is enabled by default for Original Tracker, Tracker+ and Tracker Mini through the root `custom-firmware.mk`. See [the custom firmware overview](README.md) for configuration and temporary overrides.

## Source files

**New (feature-specific; every file is wholly guarded and compiles to nothing without the flag):**

| File | Purpose |
|---|---|
| `core/source/sampleBoundaryLogic.h`, `core/source/sampleBoundaryLogic.cpp` | pure logic: frame conversion, minimum spacing, validation, last-capture-wins resolution. No firmware dependencies |
| `core/interface/samplePlayback/boundaryCapture.h`, `boundaryCapture.cpp` | session handling, pending captures, the atomic commit, UI refresh |
| `core/interface/samplePlayback/previewSession.h`, `previewSession.cpp` | small preview-session mechanism shared with Lazy Chop (compiled if either feature is on) |
| `core/interface/samplePlayback/sampleMarkers.h` | the one hook surface used by `fSamplePlay.cpp` (empty inlines without the flags) |
| `core/libs/Audio/play_memory_boundary_capture.cpp` | audio side: `getForwardLoopMinFrames()` and `applyCapturedBoundaries()` |
| `tests/host/test_sampleBoundaryLogic.cpp` | host unit test of the logic |
| `docs/custom-firmware/sample-boundary-capture.md` | this document |

**Modified (existing Tracker CE files):**

| File | Change | Guard |
|---|---|---|
| `core/interface/samplePlayback/fSamplePlay.cpp` | include + 11 one-line calls to `sampleMarkers::` (update, stop, note on/off, layout toggle, transport, play-mode change, Add Slice, function keys 1–4) | calls are no-op inlines without the flags |
| `core/libs/Audio/play_memory.h` | declarations (`isReverse()` shared with Lazy Chop, `getForwardLoopMinFrames()`, `applyCapturedBoundaries()`) | `MOD_SAMPLE_BOUNDARY_CAPTURE` (`isReverse()`: either feature) |
| `core/AudioEngine/mtAudioEngine.h`, `mtAudioEngine.cpp` | `playerEngine::isSamplePlaying()`, `isSampleReversed()` (shared), `getForwardLoopMinFrames()`, `applyCapturedBoundaries()` | same |
| `core/source/mtSequencer.h`, `mtSequencer.cpp` | read-only `getManualNoteBinding()` (shared with Lazy Chop) | either feature |
| `docs/custom-firmware/README.md` | overview | – |

**Removed:** none. **Pattern/instrument file format:** unchanged (see Format).

## Summary

While a sample is auditioned from a pad, the existing Start, Loop Start, Loop End and End function keys (buttons 1–4 of the
Sample Playback screen) also record the current playhead. Nothing changes during the audition. When the initiating pad is released
(or the note ends / is replaced, or the screen state changes) the recorded positions are resolved against the instrument's current
boundaries and written in one atomic step.

## Behaviour

* **Modes:** One-Shot, Forward Loop, Backward Loop, Pingpong. Slice, Beat Slice, Wavetable and Granular never start a session.
  Playback direction does not matter (a reversed note is captured like any other).
* **Capture:** a function key press while a session is active stores `getWavePosition()` (absolute normalised 0..65535) as the
  pending value for that boundary. The key keeps its existing meaning (it still selects that point for the encoder); capture is
  additive. In One-Shot only Start and End are captured: the Loop keys do nothing extra, as Loop Start/End are not active there.
* **Repeats:** pressing a key again replaces its pending value and gives it a new, higher submission number.
* **Commit:** pad release of the initiating source; natural end of the note, cut or voice replacement; instrument change (commits to
  the instrument bound at session start); leaving the screen; transport play; play-mode change; layout toggle. A session whose
  instrument's play mode differs from the one at session start is discarded.
* **No zero-crossing snapping** anywhere in the capture or commit path.

## Resolution (deterministic, "last capture wins")

Implemented in `sampleBoundaryLogic.cpp`, exercised by `tests/host/test_sampleBoundaryLogic.cpp`.

1. Pending captures are applied in ascending submission number, whatever order the keys were physically pressed in.
2. Each capture keeps its value. Neighbours that would violate the ordering or spacing are pushed outward
   (upwards: `Start → Loop Start → Loop End → End`; downwards in the opposite direction) until the whole chain is valid.
   Boundaries farther away that already satisfy the rules are not touched.
3. If a capture cannot be satisfied anywhere (a boundary would have to leave 0..65535, or the sample is too short), that capture
   is discarded, the arrangement from before it is kept, and the remaining captures are still applied. The result is never a
   partially applied capture: each capture is applied to a copy and accepted only when the whole copy validates.
4. Boundaries that are not part of the arrangement in the current mode (Loop Start/End in One-Shot) are neither validated nor
   written.

### Spacing rules (frames, verified against this baseline)

All checks use the frame positions the engine will derive, `(uint32_t)((float)norm * ((float)length / 65535))`, which is the
non-snapping branch of `calculatePointPositionAndSnapToZero()`. The captured norm values are what is stored, so converting back
cannot undo the validated arrangement. Distinct norms that resolve to the same frame are therefore treated as the same frame.

| Pair | Minimum (frames) | Source of the requirement |
|---|---|---|
| One-Shot Start → End | 1 | `playSingleShot()` rejects `start >= end` |
| Start → Loop Start (loop modes) | 0 (equal allowed) | loop note-on only rejects `start > loop1` |
| Loop Start → Loop End, Backward | 1 | positive length; `keepInLoop()` is modulo based |
| Loop Start → Loop End, Pingpong | 2 | reverse path uses `loopPoint1 ? loopPoint1 : 1` / `+ 1` as lower bound |
| Loop Start → Loop End, Forward | `ceil(maxIncrement) + 1`, **33** on this baseline | single overshoot correction `loop1 + (counter - loopEnd)` |
| Loop End → End | 1 | `playLoopForward()` clamps `loop2 >= end` to `end - 1` |

**Forward Loop value.** `AudioPlayMemory::getForwardLoopMinFrames()` computes it from the note table:
`notes[MAX_NOTE-1] + (notes[MAX_NOTE-1] - notes[MAX_NOTE-2])` (top note plus the largest finetune step) → ≈ 31.9 → `ceil` 32, `+1`
for the interpolation read = 33. The earlier audit's "about 30" refers to the pitch alone. `MOD_PITCH_RANGE` does not exist on this
baseline (no occurrence in the tree); the value follows the table, so a future wider range is picked up automatically. Not covered: pitch
that envelopes or LFOs could push beyond the note table (the engine clamps glide at `notes[MAX_NOTE-1]` but I did not audit every modulation path).

## Atomic commit

`boundaryCapture.cpp: commit()`:

1. `AudioNoInterrupts()` (the mechanism `playSingleShot()`/`playLoopForward()` already use around their parameter reads).
2. Store Start/End (and Loop Start/End in loop modes) in the instrument.
3. Loop modes: `playerEngine::applyCapturedBoundaries()` for all 8 players → `AudioPlayMemory::applyCapturedBoundaries()` for each
   channel (channel 0 first; a second channel copies its source's points exactly as `setLP1/2` do).
4. `AudioInterrupts()`, then `newFileManager.setInstrumentStructChanged()` and a UI refresh if that instrument is on screen.

The existing editors (`modStartPoint` and friends) are not called: they preserve loop length, can shift neighbours based on
intermediate state, and set refresh bits that re-snap loop points to zero crossings.

**What a sounding voice receives.** Loop Start/End are applied to the voice immediately, using exact frames, only if the new loop
fits inside the start/end that voice was triggered with. Start/End of a sounding voice are never changed (the engine reads them at
note-on only; the same is true of the ordinary editors). **Streamed samples:** instrument values are updated, but live voices are
left alone because the streaming look-ahead was planned at note-on with the old loop points; the next note uses the new values.

## Parameters and file format

No new parameters, no instrument or project format change, no migration. Only existing `startPoint`, `endPoint`, `loopPoint1`,
`loopPoint2` are written.

## Known limitations

* **Snap to zero at the next note-on.** The setting defaults to *on* in new projects. A later note-on converts the stored values with
  zero-crossing snapping (existing engine behaviour, not changed here), which can move a point by up to ~128 frames and, for loops
  close to the minimum, defeat the spacing guarantee. The engine's own checks (`setLP1/2`, note-on ordering) still apply. A
  strictly snap-proof arrangement would need the engine's note-on path changed; that is outside this feature.
* Notes from an external MIDI keyboard that do not go through `cSamplePlayback::noteOnHandle()` do not start a session.
* Pending captures are not drawn until committed.

## Integration and rebase notes

* All feature logic lives in new files. The only edits to existing code are one-line calls and guarded declarations/accessors.
* `fSamplePlay.cpp` hooks all go through `sampleMarkers.h`; to rebase, re-add those lines.
* Shared with Lazy Chop (both additive): `previewSession.*`, `sampleMarkers.h`, `play_memory.h` (`isReverse()`), `mtAudioEngine.*`
  (`isSamplePlaying/isSampleReversed`), `mtSequencer.*` (`getManualNoteBinding`). The two features never share pending state.

## Verification

See the PR description for the builds actually run. Host test: `g++ -std=c++17 -DMOD_SAMPLE_BOUNDARY_CAPTURE -Icore/source
tests/host/test_sampleBoundaryLogic.cpp core/source/sampleBoundaryLogic.cpp -o /tmp/sbl_test && /tmp/sbl_test` (49 checks incl. a
200 000-case randomised property check that every outcome is valid). **Not run:** anything on hardware or a simulator (capture timing,
audio behaviour at minimum loops and maximum pitch, streamed samples, stereo samples, UI redraw).
