# Major MIDI

Major MIDI is Daisy Patch SM (STM32H750, Cortex-M7) firmware for playing Standard MIDI Files from SD card through a SoundFont 2 synth engine. The firmware also exposes live MIDI input, per-channel mixing, saved song state, MIDI routing, CV/gate integration, and a browser-based remote control / MIDI file transfer over USB SysEx.

The user-facing guide lives in `site/USER.md`. The generated docs site in `docs/` is built from that file (and three others — see Docs Workflow below).

## Repo Layout

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | App bootstrap, boot sequence, media loading, transport loop, save/load flow, USB/UART MIDI wiring |
| `src/app_state.h` | `AppState` — the shared state struct everything else reads and writes |
| `src/ui/` | Input scanning, UI event translation, controller logic, OLED rendering |
| `src/midi/` | SMF playback, media browser, scheduling, routing, transport, SysEx remote control and file transfer |
| `src/synth/` | SoundFont synth integration |
| `src/cv/` | CV/gate engine |
| `src/persist/` | Boot state and per-song persistence |
| `docs/` | Generated site output plus the generator script, and two hand-written pages (`transfer.html`, `remote.html`) that are not generated |
| `site/` | Markdown source for the generated pages (`SPLASH.md`, `USER.md`, `ORDER.md`) |
| `DEV.md` | Developer-facing build notes, source map, and protocol reference |

## Build

Major MIDI builds with the libDaisy Make-based workflow.

```sh
make
```

The current target is `BOOT_QSPI`, so the application is built for the Daisy bootloader/QSPI flow.

Clean artifacts with:

```sh
make clean
```

## Docs Workflow

The published docs site is generated from four Markdown sources:

```sh
python3 docs/generate_docs.py
```

| Source | Output |
| --- | --- |
| `site/SPLASH.md` | `docs/index.html` |
| `site/USER.md` | `docs/user.html` |
| `DEV.md` | `docs/dev.html` |
| `site/ORDER.md` | `docs/order.html` |

`docs/transfer.html` and `docs/remote.html` are separate hand-written pages (static HTML + embedded JS Web MIDI clients) that the generator does not touch — edit them directly.

## Persistence Model

Major MIDI persists two different layers of state, both handled in `src/persist/song_config_persist.cpp` and `src/persist/boot_state_persist.cpp`:

| File | Purpose |
| --- | --- |
| `0:/major_midi_boot.cfg` | Last boot MIDI selection plus general UI preferences |
| `<selected-midi>.cfg` | Song-scoped settings, SF2 selection, MIDI routing, CV/gate, channel mix state |

`Song > Save Song CFG` writes both files immediately, with no confirmation and without requiring playback to be stopped. `Save All` writes the same files behind a confirmation prompt and refuses to run while the transport is playing or a channel gate is active. See `DEV.md` for the exact field layout, and note that `src/persist/midi_routing_persist.*`, `src/persist/cv_gate_persist.*`, and `src/persist/performance_persist.*` are unused/orphaned modules — the real serialization lives in `song_config_persist.cpp`.

## Development Notes

- The repo may already contain local work; avoid broad reverts.
- The UI behavior is driven primarily by `src/ui/ui_controller.cpp` and `src/ui/ui_renderer.cpp`.
- The docs generator is intentionally simple Markdown-to-HTML code, not MkDocs.
