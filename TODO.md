# Major MIDI — Todo List

1. **CV fine-tuning: make it global, not per-song.**
   Currently CV output calibration/fine-tuning lives in the per-song config
   (`src/persist/song_config_persist.cpp`). Change it to a single value set
   once at device initialization, not stored per-song. Default value: 102.8.

2. **Smooth tempo transitions on user tempo changes.**
   When the user increases/decreases tempo during playback, the MIDI file
   should smoothly speed up/slow down rather than jumping abruptly to the
   new tempo. Likely touches `src/midi/scheduler.*` and
   `src/midi/mixer_transport.*`. this will be difficult

3. **Improve the web remote control page (`docs/remote.html`).**
   Expose more of the SysEx remote-control protocol's functionality and
   improve overall UX/visual design.

4. **[Experimental] Double the OLED SSD130x driver's write-chunk size.**
   `libDaisy/src/dev/oled_ssd130x.h` — increase the data payload size per
   write chunk to 2x current, to test display update throughput.

5. **Finalize documentation, especially the user manual.**
   Bring `site/USER.md` / `docs/user.html` up to date and complete, and add
   a PDF export of the user manual (not currently produced by
   `docs/generate_docs.py`).
