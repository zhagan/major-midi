# Major MIDI Dev Resources

## Build

Major MIDI firmware builds with the libDaisy Make workflow.

```sh
make
```

That produces the Patch SM firmware image using the current `Makefile` target configuration.

Useful commands:

```sh
make clean
```

Build context:

| Item | Notes |
| --- | --- |
| Build system | `make` via libDaisy |
| App target | `BOOT_QSPI` |
| Main firmware entry | `src/main.cpp` |
| Project config | `Makefile` |

## Source Layout

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | App setup, transport loop, media reload, save flow |
| `src/ui/` | Input scanning, event translation, UI state changes, OLED rendering |
| `src/midi/` | MIDI file playback, routing, media browser, scheduler |
| `src/synth/` | SF2 synth integration |
| `src/cv/` | CV/gate behavior |
| `src/persist/` | Boot state and song-scoped persistence |
| `docs/` | Generated site output and CSS |

## Runtime Flow

Most of the application lifecycle is coordinated from `src/main.cpp`.

Key entry points:

| Function | Role |
| --- | --- |
| `LoadSelectedMedia()` | Reloads MIDI and/or SF2, restores song-scoped settings, reapplies runtime state |
| `SaveAllSettings()` | Writes config back to disk and reopens the current song safely |
| `ApplyAppSettings()` | Pushes `AppState` into transport, synth, routing, and CV/gate behavior |
| main loop | Samples controls, translates UI events, applies pending loads/saves, renders display |

## UI Architecture

The UI is split into three layers:

| File | Role |
| --- | --- |
| `src/ui/ui_input.cpp` | Reads raw hardware state from buttons, encoder, knobs, and sync switch |
| `src/ui/ui_controller.cpp` | Converts UI events into `AppState` changes |
| `src/ui/ui_renderer.cpp` | Draws the current state to the OLED |

When changing front-panel behavior, start in `UiController`. When changing only wording or layout, start in `UiRenderer`.

## Media And Transport

The playback path is spread across the MIDI and synth modules:

| Area | Notes |
| --- | --- |
| `src/midi/smf_player.*` | MIDI file parsing, playback state, embedded Major MIDI metadata |
| `src/midi/mixer_transport.*` | Channel state, mixing, timing, and transport-facing playback logic |
| `src/midi/media_library.*` | SD-card scanning and file browser model |
| `src/midi/scheduler.*` | Scheduled MIDI output timing |
| `src/synth/synth_tsf.*` | SoundFont loading and synth voice handling |

If a change affects how a song loads, loops, or routes events, read `LoadSelectedMedia()`, `smf_player`, and `mixer_transport` together.

## Persistence

Major MIDI has two persistence scopes:

| Scope | File | Purpose |
| --- | --- |
| Boot/global | `0:/major_midi_boot.cfg` | Last boot MIDI selection and general UI preferences |
| Per-song | `<selected-midi>.cfg` | Song-scoped SF2, routing, CV/gate, and channel settings |

Relevant code:

| File | Role |
| --- | --- |
| `src/persist/boot_state_persist.*` | Boot-state save/load |
| `src/persist/song_config_persist.*` | Per-song config save/load |
| `src/persist/midi_routing_persist.*` | MIDI routing serialization helpers |
| `src/persist/cv_gate_persist.*` | CV/gate serialization helpers |

`Song > Save To MIDI` is separate from these config files. It writes embedded Major MIDI metadata back into the `.mid` itself.

## Development Notes

- `AppState` in `src/app_state.h` is the central shared state model.
- UI, persistence, transport, and CV/gate all meet through `ApplyAppSettings()`.
- The worktree may be dirty; avoid broad reverts and isolate changes carefully.

## Next Step

For repo-level context, design intent, and maintenance notes, use [README.md](../README.md).
