# LXR Drum POC

**Status:** Initial `.psyn` build-path and multi-part voice scaffold. Not yet source-faithful LXR DSP.

## What this prototype establishes

- A C++ `polysynth::InstrumentVoice` plugin registered through the official `.psyn` bridge.
- One Tracker host voice (`default_voices=1`, `max_voices=1`).
- Three independent internal percussion states, triggered by MIDI notes 36, 38, and 42.
- Overlapping one-shot tails mixed inside that single host voice.
- Fixed parameter IDs and three editor sections.
- A build target included in `make psyn`, so the existing GitHub Actions packaging step collects `LXRD.psyn` alongside the factory and CHIP plugins.

## Important limitation

The first renderer is an intentionally small dual-oscillator/FM approximation used to prove the SDK registration, metadata, routing, render, and packaging seams. It does **not** yet contain the audited LXR oscillator/wavetable, pitch/amplitude envelopes, nonlinear ZDF resonant filter, transient generator, or source InitKit values. Do not describe this build as a faithful LXR-02 port.

The next implementation pass should replace the provisional renderer with the source-audited DrumVoice DSP dependency closure, preserving the three-part host architecture. The first build/CI result will determine whether the SDK integration itself is sound before expanding the DSP.

## Build

From the repository root:

```sh
make psyn
```

Expected plugin output: `build/Synths/LXRD.psyn`.

The root Makefile also copies the LXR GPL-3.0 license and NOTICE into `build/Synths/` so the existing CI SD-card artifact carries the applicable notices.

## Trigger adaptation

| MIDI note | Internal part |
|---:|---|
| 36 | Drum 1 |
| 38 | Drum 2 |
| 42 | Drum 3 |

These are provisional Tracker-side note assignments, not original LXR sequencer routing.

## License and naming

The prototype is distributed under GPL-3.0-only with the upstream LXR NOTICE terms retained. The engine is an independent community adaptation; it must not be presented as an official Erica Synths product. See [LICENSE-LXR-GPL-3.0.txt](LICENSE-LXR-GPL-3.0.txt) and [NOTICE-LXR.md](NOTICE-LXR.md).
