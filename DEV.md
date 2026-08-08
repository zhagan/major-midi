# Major MIDI Dev Resources

## Build

Major MIDI firmware builds with the libDaisy Make workflow for the Daisy Patch SM (STM32H750, Cortex-M7).

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
| `src/main.cpp` | App setup, boot sequence, transport loop, media reload, save flow, USB/UART MIDI wiring |
| `src/app_state.h` | `AppState` — the single shared state struct for UI, transport, synth, routing, and CV/gate |
| `src/ui/` | Input scanning, event translation, UI state changes, OLED rendering |
| `src/midi/` | MIDI file playback, routing, media browser, scheduler, SysEx remote control and file transfer |
| `src/synth/` | SF2 synth integration (`synth_tsf.*`) plus the vendored `tsf.h` SoundFont renderer |
| `src/cv/` | CV/gate behavior |
| `src/persist/` | Boot state and per-song persistence |
| `src/sd/` | SD card mount |
| `src/clock_sync.*` | Internal/external sync arbitration (MIDI clock vs. gate sync) |
| `docs/` | Generated site output, CSS, and two hand-maintained standalone pages (`transfer.html`, `remote.html`) |

## Runtime Flow

Most of the application lifecycle is coordinated from `src/main.cpp`. The boot sequence, in order:

1. `InitDefaultState()` and UI/splash init.
2. `SdMount()`.
3. `media_library.Scan()` — this only resets the two front-panel folder browsers to root; it does **not** walk the whole SD card into RAM (see the `media_library.*` note below).
4. If the SD card mounted: `LoadBootState()` restores the last MIDI filename and general UI prefs from `0:/major_midi_boot.cfg`.
5. Synth, transport, and CV/gate engine init.
6. Resolve `selected_midi_path`: if the restored filename still exists (`MidiFileExists`), adopt it; otherwise fall back to `FindFirstMidiFile()`. `selected_sf2_path` is always set via `FindFirstSoundFont()` (there's no persisted "last SF2" independent of the per-song config).
7. If the SD card mounted: `LoadSelectedMedia(true, true, now)` opens the SF2, opens the MIDI file, and — inside that same call — loads the per-song `.cfg` via `LoadSongConfig()`, which can override the SF2 selection made in step 6.
8. USB MIDI init and `StartReceive()`.
9. `sysex_file_transfer.Init(...)` and `sysex_remote_control.Init(&app_state, &media_library, SyncLoopStateForRemote, nullptr)` wire the two SysEx handlers into the USB MIDI receive path.
10. UART MIDI init, timer setup, then the main loop begins.

Key entry points used after boot:

| Function | Role |
| --- | --- |
| `LoadSelectedMedia()` | Reloads MIDI and/or SF2, restores song-scoped settings, reapplies runtime state |
| `SaveSelectedSongConfig()` | Quick-saves the current song's `.cfg` + boot state; used by `Song > Save Song CFG` |
| `SaveAllSettings()` | Stops the transport, then writes song `.cfg` and boot state with a staged progress overlay; used by `Save All` |
| `ApplyAppSettings()` | Pushes `AppState` into transport, synth, routing, and CV/gate behavior |
| main loop | Samples controls, translates UI events, applies pending loads/saves, drives the SysEx handlers, renders display |

## UI Architecture

The UI is split into three layers:

| File | Role |
| --- | --- |
| `src/ui/ui_input.cpp` | Reads raw hardware state from buttons, encoder, knobs, and sync switch; drives the B1-B4 activity LEDs via the MCP23017 |
| `src/ui/ui_controller.cpp` | Converts UI events into `AppState` changes |
| `src/ui/ui_renderer.cpp` | Draws the current state to the OLED |

When changing front-panel behavior, start in `UiController`. When changing only wording or layout, start in `UiRenderer`.

Notable UI details for anyone touching this code:

- Long-press threshold for the encoder and bank buttons is 700 ms (`ui_input.cpp`).
- `UiMode::Mute` is a dead enum value — never set anywhere. The live "mute page" UI is `KnobPage::Mute` inside `UiMode::Performance`, not a separate mode.
- Channel focus edit-field cycle is `Mute → Volume → Pan → Reverb → Chorus → Program`; a tap toggles edit for the current field, a long-press exits focus back to the bank view.

## Media And Transport

The playback path is spread across the MIDI and synth modules:

| Area | Notes |
| --- | --- |
| `src/midi/smf_player.*` | MIDI file parsing, playback state, transport |
| `src/midi/mixer_transport.*` | Channel state, mixing, timing, and transport-facing playback logic. Global transpose (`sf2_transpose`) explicitly skips the drum channel (index 9 / MIDI channel 10). Hand-driven tempo changes (encoder / web remote, not clock sync and not while looping) glide via bounded per-`Update` steps (`kTempoRampBpmPerSecond = 240`) reusing the instant-apply path (`SetTempoScale` / `RemapQueuedEventTimes` / `phase_start_ticks_` recalibration) with a small delta; the tempo-change-during-loop branch flushes loop-boundary notes before `ClearQueues()` so a sounding note's Note-Off is never dropped. |
| `src/midi/media_library.*` | Live, per-folder SD-card browsing — no whole-card scan or global file-count cap. Files are identified by relative path (e.g. `set1/track01.mid`), not index; `AppState::selected_midi_path`/`selected_sf2_path` carry that identity directly. A single folder's browser view still caps at `kMaxMidiBrowserEntries=128` / `kMaxSf2BrowserEntries=32` entries (front panel) — split a very large folder into subfolders if you hit that. `FindFirstMidiFile()`/`FindFirstSoundFont()` do an early-exit recursive walk, used only as a fallback when the previously-selected file is gone. |
| `src/midi/scheduler.*` | Scheduled MIDI output timing |
| `src/synth/synth_tsf.*` | SoundFont loading and synth voice handling. Voices field is `0`-`32`; `0` disables the synth entirely. FX (reverb/chorus) auto-bypass at ≥16 active voices and restore at ≤12 (hysteresis), to protect CPU headroom. |
| `src/midi/sysex_remote_control.*` | SysEx command protocol for the browser-based remote (`docs/remote.html`) — see below |
| `src/midi/sysex_file_transfer.*` | SysEx chunked file-upload protocol for MIDI transfer (`docs/transfer.html`) — see below |

If a change affects how a song loads, loops, or routes events, read `LoadSelectedMedia()`, `smf_player`, and `mixer_transport` together.

## SysEx Remote Control Protocol

`src/midi/sysex_remote_control.*` implements the protocol behind `docs/remote.html`. Manufacturer ID `0x7D`, magic bytes `'M' 'M'` (`0x4D 0x4D`). Every request/reply is a standard SysEx frame:

```text
Request: F0 7D 4D 4D <cmd> [payload...] F7
Reply:   F0 7D 4D 4D <cmd> <status> [payload...] F7
```

Status bytes: `0x00` OK, `0x01` Invalid packet, `0x02` Out of range. Multi-byte integers are packed 7-bit LSB-first: 14-bit values as 2 bytes, 28-bit values as 4 bytes. Paths/names are length-prefixed (1 byte length + raw bytes, ASCII only — same convention as elsewhere in the firmware). **Hard ceiling: every request and reply must fit in `SYSEX_BUFFER_LEN = 128` bytes total** (`libDaisy/src/hid/midi_parser.cpp`) — that's why file/directory selection is split into small single-value commands rather than one large batched reply, and why `kReplyMax = 120` in `sysex_remote_control.h` sits just under that ceiling.

There is no global flat file index anymore (see `media_library.*` above) — the remote enumerates one directory at a time by relative path, mirroring the front-panel browser, and loads/selects files by full relative path instead of index.

| Cmd | Name | Request payload | Reply payload |
| --- | --- | --- | --- |
| `0x10` | GetStatus | — | playing(1B), bpm(14b), measure(14b), beat(1B) |
| `0x11` | GetSelectedMidiPath | — | pathLen(1B), path bytes |
| `0x12` | GetSelectedSf2Path | — | pathLen(1B), path bytes |
| `0x13` | LoadMidi | pathLen(1B), path bytes | status only (`Range` if the path doesn't exist) |
| `0x14` | LoadSf2 | pathLen(1B), path bytes | status only |
| `0x15` | Transport | action(1B): `0x00` stop, `0x01` play, `0x02` toggle | resulting `transport_playing` (1B) |
| `0x16` | GetChannelState | channel(14b), `<16` | index(14b), volume, pan, reverb_send, chorus_send (1B each, 0-127), muted(1B), program_override(14b, `128`=none), current_program(1B) |
| `0x17` | SetChannelState | index(14b), volume, pan, reverb_send, chorus_send, muted, program(14b, `128` clears override) | index(14b) |
| `0x18` | GetSongState | — | bpmOverride(14b), loopEnabled(1B), loopStart(28b), loopLength(28b) |
| `0x19` | SetSongState | bpmOverride(14b, ≤300), loopEnabled(1B), loopStart(28b), loopLength(28b, nonzero) | status only |
| `0x1A` | SaveSongSettings | — | status only (sets `pending_save_settings`, same as `Song > Save Song CFG`) |
| `0x1B` | GetMidiDirCount | dirPathLen(1B), dir path bytes (empty = root) | count(14b) |
| `0x1C` | GetMidiDirEntry | dirPathLen(1B), dir path bytes, index(14b) | index(14b), isDir(1B), nameLen(1B), name bytes (leaf name only, ≤`kNameMax`=32) |
| `0x1D` | GetSf2DirCount | dirPathLen(1B), dir path bytes | count(14b) |
| `0x1E` | GetSf2DirEntry | dirPathLen(1B), dir path bytes, index(14b) | index(14b), isDir(1B), nameLen(1B), name bytes |

`LoadMidi`/`LoadSf2` push the selection into `pending_midi_load`/`pending_sf2_load` and force the UI back to Performance/Main menu. `SetSongState` calls back into `SyncLoopStateForRemote()` (wired in `main.cpp`) so loop display fields stay in sync. Directory entries are returned directories-first-then-files, in the same order `MediaLibrary::DirEntryAt()` walks them — matches the front panel's own folder browser ordering.

## SysEx File Transfer Protocol

`src/midi/sysex_file_transfer.*` implements the protocol behind `docs/transfer.html`. Same manufacturer ID/magic as above. Commands: `Start=0x01`, `Data=0x02`, `End=0x03`, `Cancel=0x04`; every reply is an Ack frame with command `0x7F`:

```text
Request: F0 7D 4D 4D <cmd> [payload...] F7
Ack:     F0 7D 4D 4D 7F <requestCmd> <status> <value low7> <value high7> F7
```

Status bytes: `0x00` OK, `0x01` Invalid packet, `0x02` Busy, `0x03` FS error, `0x04` Sequence mismatch, `0x05` Bad filename, `0x06` Decode error, `0x07` No active transfer.

| Cmd | Payload | Behavior |
| --- | --- | --- |
| `Start (0x01)` | filename length(1B), filename bytes (≤48) | Aborts any in-progress transfer, normalizes the filename (strips path, keeps alnum/`.`/`_`/`-`/space, forces a `.mid` extension), creates `0:/midi` if needed, opens `0:/midi/<name>` with create-always (this **overwrites** an existing file of the same name, with no rename/collision handling) |
| `Data (0x02)` | sequence(14b), raw length(1B, ≤63), 7-bit-packed data | Sequence must match the expected count or replies `Sequence` with the expected value; each chunk of ≤63 raw bytes is packed as one MSB byte + up to 7 data bytes |
| `End (0x03)` | final sequence(14b) | Must match the chunk count received; syncs and closes the file, then rescans the media library and reports the upload complete |
| `Cancel (0x04)` | — | Aborts the transfer and deletes the partial file |

`docs/remote.html` and `docs/transfer.html` are hand-authored static pages with the client side of these protocols implemented directly in embedded JavaScript (Web MIDI API with SysEx). They are **not** generated by `docs/generate_docs.py` — edit them directly, and keep their embedded command/status tables in sync with the two files above if the protocol changes.

## Persistence

Major MIDI has two active persistence scopes, both implemented directly in `src/persist/song_config_persist.cpp` and `src/persist/boot_state_persist.cpp`:

| Scope | File | Magic/Version | Notes |
| --- | --- | --- | --- |
| Boot/global | `0:/major_midi_boot.cfg` | `MMBT`, v7 | Last MIDI filename, `screen_saver_timeout_s`, `knob_pickup_mode`, `encoder_direction`, `oled_x_offset`, and (added in v7) the global `cv1_pitch_scale` / `cv2_pitch_scale` CV-output calibration |
| Per-song | `<selected-midi>.cfg` | `MMSC`, v12 | Both CV inputs, both gate inputs/outputs, both CV outputs, USB+UART MIDI output routing (mode, transport/clock flags, full 16-channel matrix), all 16 `ChannelState` entries + mute mask, loop state, `song_bpm_override`, SF2 FX-scaler caps, `sf2_transpose`, and the SF2 filename. v12 dropped the per-output pitch-scale fields (moved to the global boot config); it still loads v8-v11 files, skipping their legacy pitch-scale bytes |

Relevant code:

| File | Role |
| --- | --- |
| `src/persist/boot_state_persist.*` | Boot-state save/load |
| `src/persist/song_config_persist.*` | Per-song config save/load — this is where CV/gate and MIDI routing are actually serialized |
| `src/persist/persist_file.*` | One-function helper (`SharedPersistFile()`) so every persist module reuses a single shared `FIL` handle instead of declaring its own |

**Orphaned modules — do not assume these are live:** `src/persist/midi_routing_persist.*`, `src/persist/cv_gate_persist.*`, and `src/persist/performance_persist.*` each define their own standalone save/load functions with their own file magics (`MMMR`/`MMCV`/`MMPF`), but none of them are called anywhere outside their own source file. They compile and link (they're in the `Makefile`), but the actual routing/CV-gate persistence happens inline inside `song_config_persist.cpp`, and `performance_persist.*` has no caller at all. If you're extending persistence, either wire these up deliberately or remove them — don't assume `grep`-ing their names means they're part of the save flow.

`Song > Save Song CFG` and `Save All` both write the same per-song `.cfg` plus the boot-state file; the difference is `Save All` opens a confirmation page, forces the transport to stop first, and refuses to run while anything is playing or gating. See `USER.md` for the user-facing description.

## Development Notes

- `AppState` in `src/app_state.h` is the central shared state model.
- UI, persistence, transport, and CV/gate all meet through `ApplyAppSettings()`.
- The worktree may be dirty; avoid broad reverts and isolate changes carefully.
- The docs generator (`docs/generate_docs.py`) builds `index.html`/`user.html`/`dev.html`/`order.html` from `site/SPLASH.md`, `site/USER.md`, `DEV.md`, and `site/ORDER.md` respectively. `docs/transfer.html` and `docs/remote.html` are separate hand-written pages it does not touch.

## Next Step

For repo-level context, design intent, and maintenance notes, use [README.md](../README.md).
