# Performance FX & Track Play Modes

This guide describes the retained Performance FX behavior and the Shift + Perform snapshot/recall shortcut in Tracker CE.

## Overview

Tracker CE separates the Performance screen from the lifetime of Performance FX:

- **Track FX opt-in is track-owned.** Each track can retain its Performance FX configuration independently of whether the Performance screen is currently open.
- **Track opt-in persists** when navigating to another screen and when changing patterns.
- **Pattern Play Mode is track-owned.** A track's selected Performance Play Mode continues to govern that track outside the Performance screen.
- **Performance track length remains screen-mode scoped.** Leaving the Performance screen does not keep the Performance-specific track-length behavior active.
- **Pattern Length is the only independently active global Performance FX.** It can remain active when no individual track is opted in.

This allows a Performance setup to remain part of the playing project without requiring the Performance screen to stay open.

## Track FX opt-in and retained behavior

Opting a track into Performance FX enables the configured Performance FX for that track. The state is retained across screen navigation and pattern changes.

When the Performance screen is left:

- Retained track FX remain active.
- Track-owned Pattern Play Modes remain active.
- Performance-specific track length is no longer applied outside Performance Mode.
- Position LFO rate ownership is released because that control is scoped to the Performance screen.

Active Performance FX and their MIDI CC controls remain available outside the Performance screen. Track-pattern MIDI CC controls remain screen-dependent.

A Performance FX filter takes ownership of the active filter even if the instrument's own filter-enable setting is off. In that case, the cutoff envelope and Filter LFO can still modulate the Performance-owned filter. When Performance filter ownership ends, the underlying cutoff and any active cutoff envelope are restored.

## Shift + Perform: snapshot and recall

**Shift + Perform** is a global shortcut. It works from any screen and recognizes either button order:

- Hold **Shift**, then press **Perform**.
- Press **Perform**, then hold **Shift**.

The shortcut consumes the Perform action; it does not also open the Performance screen or switch its track bank.

### When Performance FX are active

If at least one track is opted in or Pattern Length is active, Shift + Perform:

1. Saves the current opt-in state of every track and the current Pattern Length setting.
2. Turns off all Performance FX, including Pattern Length.
3. Keeps the saved configuration in a temporary recall buffer.

The snapshot represents the complete configuration, including tracks that were opted out and the exact Pattern Length state.

### When no Performance FX are active

If the recall buffer contains a valid snapshot, Shift + Perform restores the saved track opt-ins and Pattern Length, then consumes the snapshot.

If there is no valid snapshot, Shift + Perform opts all tracks into Performance FX. Pattern Length remains disabled.

### Starting a different configuration

The recall buffer is invalidated when a new configuration is manually started from the fully inactive state. This includes manually opting a track in or manually enabling Pattern Length while no Performance FX are active.

Manual changes made while a Performance FX configuration is already active do not rewrite the suspended snapshot.

The buffer is temporary runtime state; it is not a saved project preset and is cleared when a different project is loaded or a new project is created.

## Pattern Play Modes and Song playback

Pattern Play Mode belongs to each track rather than to the global Performance screen. Track-owned modes—including Backward, Random, and Custom modes—continue operating after leaving the Performance screen.

Song playback also continues through pattern boundaries while these modes are active. Pattern preloading is based on the sequencer's absolute cycle position, not the displayed step position, because Backward, Random, and Custom modes can move or jump the displayed position while the pattern cycle continues sequentially.

This preserves normal song transitions without requiring a track's displayed position to move forward through every step.

## Reset and project changes

- Explicitly shutting down Performance FX clears the retained configuration.
- Replacing the current project or creating a new project clears retained Performance FX and the Shift + Perform recall buffer.
- Leaving the Performance screen alone does not clear retained track FX or Pattern Length.

## Quick behavior reference

| Action | Result |
| --- | --- |
| Leave the Performance screen with track FX active | Track FX remain active; Performance track-length behavior ends |
| Change patterns with track FX opted in | Track opt-in remains in effect |
| Use Shift + Perform with any FX active | Snapshot all track opt-ins and Pattern Length, then disable FX |
| Use Shift + Perform with FX inactive and a valid snapshot | Restore the saved configuration and consume the snapshot |
| Use Shift + Perform with FX inactive and no snapshot | Opt all tracks in; leave Pattern Length off |
| Manually start a configuration while all FX are off | Invalidate the suspended snapshot |
| Load or create another project | Clear retained FX and the recall buffer |

