# Major MIDI

Major MIDI is a Daisy Patch SM firmware for playing Standard MIDI Files from SD card through a SoundFont 2 synth engine, with front-panel mixing, live MIDI input, internal or external sync, saved song settings, MIDI routing, and assignable CV/gate I/O.

## Quick Start

1. Copy one or more `.mid` files into `0:/midi`.
2. Copy one or more `.sf2` files into `0:/soundfonts`.
3. Insert the SD card and power the module.
4. Open the menu with a long encoder press.
5. Load a MIDI file from `Load MIDI`.
6. Load a SoundFont from `Load SF2`.
7. Return to the performance screen.
8. Press `Play`.

If the sync switch is set to external, playback will wait for external clock instead of free-running.

## SD Card Layout

Major MIDI scans these folders recursively:

```text
0:/midi
0:/soundfonts
```

Example:

```text
0:/midi/set1/track01.mid
0:/midi/loops/arp.mid
0:/soundfonts/general/microgm.sf2
0:/soundfonts/drums/clubkit.sf2
```

Hidden files and AppleDouble files are ignored.

## Boot And Loading

On boot, Major MIDI:

| Step | Behavior |
| --- | --- |
| SD scan | Scans `0:/midi` and `0:/soundfonts` |
| MIDI restore | Reloads the last saved MIDI selection when available |
| SF2 selection | Starts from the first SF2, then may switch to the song's saved SF2 |
| Song config | Loads `<midi-file>.cfg` if present |
| UI prefs | Loads general UI settings from `0:/major_midi_boot.cfg` |

If both a MIDI file and an SF2 are available, the unit loads them automatically at startup.

## Front Panel

| Control | Role |
| --- | --- |
| `B1..B4` | Bank buttons and view combos |
| `K1..K4` | Per-channel controls for the visible bank |
| `Play` | Transport and back/cancel in menus |
| Encoder turn | Page selection, menu navigation, loop edits, BPM edit |
| Encoder press | Shift, confirm, enter/exit edit modes |
| OLED | Current mode, transport, file names, and parameter values |

The sync source is a hardware switch. Internal sync free-runs from the current BPM. External sync follows incoming MIDI clock or configured gate sync.

## Performance View

Performance mode is the default screen.

Top-line fields show:

| Field | Meaning |
| --- | --- |
| `STP` or `PLY` | Stopped or playing |
| BPM | Current transport BPM |
| Measure and beat | Current musical position |
| `B1..B4` | Active bank |
| `V`, `P`, `R`, `C`, `G`, `M`, `BPM` | Active knob page |

The screen also shows the current MIDI file, the current SoundFont, and four visible channels from the selected bank.

## Banks And Knob Pages

The 16 channels are shown in four banks:

| Bank | Channels |
| --- | --- |
| `B1` | 1-4 |
| `B2` | 5-8 |
| `B3` | 9-12 |
| `B4` | 13-16 |

Press `B1..B4` to select a bank.

Turn the encoder in performance mode to cycle knob pages:

| Page | Function |
| --- | --- |
| `V` | Volume |
| `P` | Pan |
| `R` | Reverb send |
| `C` | Chorus send |
| `G` | Program override |
| `M` | Mute |
| `BPM` | Tempo edit page |

`K1..K4` always control the four visible channels for the active page.

With `Knobs` set to `Pickup`, a knob must cross the stored value before it takes control. With `Knobs` set to `Instant`, the value jumps immediately.

## BPM Editing

The encoder does not always change tempo directly.

To edit tempo:

1. Turn the encoder until the active page is `BPM`.
2. Tap the encoder to unlock BPM editing.
3. Turn the encoder to set BPM from `20` to `300`.
4. Tap the encoder again to lock BPM.

If a song BPM override is saved for the current MIDI file, that override is applied when the song loads.

## Channel Focus

Long-press one of `B1..B4` in performance mode to focus that visible channel.

Channel focus shows:

| Item | Meaning |
| --- | --- |
| Channel and bank | Which channel is selected |
| Program | Current program or override |
| Program name | Current GM/SF2 program label when available |
| Volume and pan | Current mix values |
| Reverb and chorus | Current send values |
| Mute state | Per-channel mute status |

Tap the encoder to leave channel focus and return to the normal bank view.

## Mute Page

On the `M` knob page:

| Control | Result |
| --- | --- |
| `K1..K4` | Set mute state for the visible channels |
| `B1..B4` | Toggle mute for the visible channels directly |

To switch banks while on the mute page, hold the encoder and press a bank button.

## Extra Views

Press the button combos below to open alternate views:

| Combo | View |
| --- | --- |
| `B1 + B2` | MIDI monitor |
| `B1 + B3` | Transport/song info |
| `B1 + B4` | Loop edit |

### MIDI Monitor

The MIDI monitor shows the most recent note, pitch bend, and CC activity for each channel.

Controls:

| Control | Result |
| --- | --- |
| Encoder turn | Scroll channel list |
| `Play` | Clear monitor data |
| Encoder tap or long press | Exit |

### Transport View

The transport view shows the current MIDI file, measure and beat, loop range, current tick, active voice count, and time signature.

Press `Play`, tap the encoder, or long-press the encoder to exit back to performance mode.

## Loop Edit

Loop edit gives direct access to the current loop range.

Editable fields:

| Field | Meaning |
| --- | --- |
| `Active` | Enable or disable looping |
| `St M` | Start measure |
| `St B` | Start beat |
| `St T` | Start tick |
| `Ln M` | Loop length in measures |
| `Ln B` | Additional beats |
| `Ln T` | Absolute length in ticks |

Controls:

| Control | Result |
| --- | --- |
| Encoder turn | Move between fields |
| Encoder press | Toggle edit mode for the selected field |
| `Play` | Exit loop edit |
| Encoder long press | Exit loop edit |

## Menu Basics

Open or close the main menu with either:

| Action | Result |
| --- | --- |
| Long encoder press | Toggle menu |
| Hold encoder and press `Play` | Toggle menu |

Inside menus:

| Control | Result |
| --- | --- |
| Encoder turn | Move cursor |
| Encoder press | Enter page, confirm selection, or toggle page edit mode |
| `Play` | Back out one level or leave the menu |

Main menu pages:

| Page | Purpose |
| --- | --- |
| `Load MIDI` | Browse and load `.mid` files |
| `Load SF2` | Browse and load `.sf2` files |
| `General` | UI preferences |
| `FX` | Global synth FX tuning |
| `Song` | Song-level loop and tempo settings |
| `SF2` | Synth and per-channel settings |
| `MIDI` | USB/UART output routing |
| `CV/Gate` | CV and gate assignment |
| `Save All` | Write config files safely |

## Load MIDI And Load SF2

The file browsers support subdirectories.

Behavior:

| Item | Behavior |
| --- | --- |
| Directory | Enter it |
| `[..]` | Go up one level |
| File | Load it |

Loading a MIDI file also reloads the song-scoped config file when `<song>.cfg` exists next to that MIDI file.

## General

General settings:

| Item | Meaning |
| --- | --- |
| `Saver` | Screen saver timeout |
| `Knobs` | `Pickup` or `Instant` |
| `Enc` | Encoder direction |
| `OLED X` | Horizontal OLED column offset |

## FX

FX settings are global, not per-channel.

| Item | Meaning |
| --- | --- |
| `Rev Time` | Reverb time |
| `Rev LPF` | Reverb low-pass filter |
| `Rev HPF` | Reverb high-pass filter |
| `Ch Depth` | Chorus depth |
| `Ch Speed` | Chorus speed |

Per-channel reverb and chorus amounts stay on the performance pages and in the `SF2` menu.

## Song

Song settings:

| Item | Meaning |
| --- | --- |
| `BPM Ovr` | Saved BPM override |
| `Loop` | Loop enable |
| `St M`, `St B`, `St T` | Loop start |
| `Ln M`, `Ln B`, `Ln T` | Loop length |
| `Save To MIDI` | Write embedded Major MIDI song metadata |

`Save To MIDI` updates the current `.mid` file. It does not replace `Save All`.

## SF2

SF2 settings:

| Item | Meaning |
| --- | --- |
| `Voices` | Max synth voices |
| `Channel` | Channel being edited |
| `Mute` | Mute for that channel |
| `Volume` | Channel volume |
| `Pan` | Channel pan |
| `RevSend` | Channel reverb send |
| `ChoSend` | Channel chorus send |
| `Program` | Program override or file-follow mode |
| `Trans` | Global transpose |

Program behavior:

| Value | Result |
| --- | --- |
| `Program File` | Follow program changes from the MIDI file |
| `Program 000..127` | Force a manual program override |

Higher voice counts increase CPU load. If playback becomes unstable, reduce `Voices`.

## MIDI

The `MIDI` page controls USB and UART output routing.

Available output modes:

| Mode | Meaning |
| --- | --- |
| `Off` | No channel output |
| `Notes` | Forward notes only |
| `Nt+CC` | Forward notes and CCs |
| `N+C+P` | Forward notes, CCs, and programs |
| `Matrix` | Per-channel routing matrix |

Matrix controls let you choose:

| Item | Meaning |
| --- | --- |
| `Mtx Port` | USB or UART |
| `Mtx Src` | Source channel |
| `Mtx Dst` | Destination channel |
| `Mtx Nt` | Forward notes |
| `Mtx CC` | Forward CCs |
| `Mtx Prg` | Forward program changes |

There are also dedicated toggles for:

| Item | Meaning |
| --- | --- |
| `USB Trn` / `UART Trn` | Forward transport messages |
| `USB Clk` / `UART Clk` | Forward clock |
| `USB>UART` | Pass USB input to UART output |
| `UART>USB` | Pass UART input to USB output |

## CV/Gate

CV/Gate assignments are fully configurable per song.

Input CV modes:

| Mode | Meaning |
| --- | --- |
| `Off` | Disabled |
| `MasterVol` | Master volume control |
| `BPM` | Tempo control |
| `Ch Pitch` | Channel pitch control |
| `Ch CC` | Channel CC control |
| `NotePitch` | Note pitch control |

Gate input modes:

| Mode | Meaning |
| --- | --- |
| `Off` | Disabled |
| `Sync` | External gate sync input |
| `NoteTrig` | Trigger notes on a channel |

Gate output modes:

| Mode | Meaning |
| --- | --- |
| `Off` | Disabled |
| `Sync` | Clock output |
| `Reset` | Reset pulse output |
| `Ch Gate` | Gate output from a channel |

CV output modes:

| Mode | Meaning |
| --- | --- |
| `Off` | Disabled |
| `Pitch` | Output channel pitch |
| `CC` | Output a channel CC value |

Each CV/gate page also exposes the related channel, CC number, sync resolution, trigger style, or note priority when that mode needs it.

## Sync

Major MIDI has two sync behaviors:

| Sync source | Behavior |
| --- | --- |
| Internal | Plays at the current BPM |
| External | Waits for incoming clock before advancing |

External sync can follow:

| Source | Notes |
| --- | --- |
| MIDI clock | Via incoming MIDI transport clock |
| Gate sync | Via configured gate input sync pulses |

If external sync is selected and no valid clock arrives, `Play` will arm transport but the song will not move.

## Saving And Recall

There are two save paths:

| Action | What it writes |
| --- | --- |
| `Song > Save To MIDI` | Embedded Major MIDI metadata inside the current `.mid` |
| `Save All` | `<current-midi>.cfg` plus boot/UI state |

`Save All` stores:

| Saved state |
| --- |
| Selected SF2 for the current song |
| Channel mix, mute, and program override state |
| Song loop data |
| MIDI routing |
| CV/gate assignments |
| General UI settings and last boot MIDI selection |

For reliability, use `Save All` only while playback is stopped.

## Troubleshooting

| Problem | Check |
| --- | --- |
| No playback when pressing `Play` | Sync switch may be set to external with no incoming clock |
| No files in browser | Confirm files are under `0:/midi` or `0:/soundfonts` |
| Wrong instrument loads with a song | Check the song's saved SF2 and per-channel program overrides |
| Knobs do not respond immediately | `Knobs` may be set to `Pickup` |
| Audio overload or glitches | Lower `Voices`, reduce dense arrangements, or use a lighter SF2 |
