# Major MIDI — Todo List

1. **[DONE] CV fine-tuning: make it global, not per-song.**
   Done — `cv1_pitch_scale`/`cv2_pitch_scale` moved out of the per-song
   config into two global `AppState` fields (default 102.8), persisted in the
   boot config (`MMBT` v6->v7) and edited from the General settings menu.
   Song config dropped to its pre-pitch-scale layout (`MMSC` v12).

2. **[DONE] Smooth tempo transitions on user tempo changes.**
   Done — hand-driven tempo changes now glide via bounded per-`Update` steps
   (`kTempoRampBpmPerSecond = 240`) in `src/midi/mixer_transport.*` instead of
   snapping. The same change also fixed hung notes when tempo changed during a
   loop (flush loop-boundary notes before `ClearQueues()`).

3. **Improve the web remote control page (`docs/remote.html`).**
   Expose more of the SysEx remote-control protocol's functionality and
   improve overall UX/visual design.

4. **[Experimental] Double the OLED SSD130x driver's write-chunk size.**
   `libDaisy/src/dev/oled_ssd130x.h` — increase the data payload size per
   write chunk to 2x current, to test display update throughput.

5. **Finalize documentation, especially the user manual.**
   Ongoing — `site/USER.md` / `docs/user.html` have been refreshed against the
   current firmware. Still to do: a PDF export of the user manual (not
   currently produced by `docs/generate_docs.py`). Keep the manual in sync as
   features land.
