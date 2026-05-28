# Major MIDI

Major MIDI is Daisy Patch SM firmware for playing Standard MIDI Files from SD card through a SoundFont 2 synth engine. The firmware also exposes live MIDI input, per-channel mixing, saved song state, MIDI routing, and CV/gate integration.

The user-facing guide lives in `USER.md`. The generated docs site in `docs/` is built from that file.

## Repo Layout

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | App bootstrap, media loading, transport loop, save/load flow |
| `src/ui/` | Input scanning, UI event translation, controller logic, OLED rendering |
| `src/midi/` | SMF playback, media browser, scheduling, routing, transport |
| `src/synth/` | SoundFont synth integration |
| `src/cv/` | CV/gate engine |
| `src/persist/` | Boot state and per-song persistence |
| `docs/` | Static site output plus the generator script |
| `USER.md` | Current operator guide and docs source |

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

The published docs site is a generated static page:

```sh
python3 docs/generate_docs.py
```

That command reads `USER.md` and rewrites `docs/index.html`.

## Persistence Model

Major MIDI persists two different layers of state:

| File | Purpose |
| --- | --- |
| `0:/major_midi_boot.cfg` | Last boot MIDI selection plus general UI preferences |
| `<selected-midi>.cfg` | Song-scoped settings, SF2 selection, MIDI routing, CV/gate, channel mix state |

`Song > Save To MIDI` writes embedded Major MIDI metadata into the `.mid` file. `Save All` writes the external config files and should be used when playback is stopped.

## Development Notes

- The repo may already contain local work; avoid broad reverts.
- The UI behavior is driven primarily by `src/ui/ui_controller.cpp` and `src/ui/ui_renderer.cpp`.
- The docs generator is intentionally simple Markdown-to-HTML code, not MkDocs.
