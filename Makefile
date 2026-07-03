TARGET = SF2MidiPlayer

# Build for Daisy Bootloader (store/execute app from QSPI, not internal FLASH)
APP_TYPE = BOOT_QSPI

# DEBUG = 0
# OPT   = -Os
# LTO   = 1
USE_FATFS = 1
USE_DAISYSP_LGPL = 1

C_INCLUDES += -Isrc -Isrc/persist -Isrc/midi -Isrc/ui -Isrc/synth -Isrc/cv -Isrc/sd

CPP_SOURCES = \
  src/main.cpp \
  src/sd/sd_mount.cpp \
  src/synth/synth_tsf.cpp \
  src/midi/smf_player.cpp \
  src/midi/major_midi_settings.cpp \
  src/midi/media_library.cpp \
  src/midi/sysex_remote_control.cpp \
  src/midi/sysex_file_transfer.cpp \
  src/midi/scheduler.cpp \
  src/clock_sync.cpp \
  src/persist/boot_state_persist.cpp \
  src/persist/persist_file.cpp \
  src/persist/cv_gate_persist.cpp \
  src/persist/midi_routing_persist.cpp \
  src/persist/performance_persist.cpp \
  src/persist/song_config_persist.cpp \
  src/cv/cv_gate_engine.cpp \
  src/ui/ui_input.cpp \
  src/ui/ui_controller.cpp \
  src/ui/ui_renderer.cpp \
  src/midi/mixer_transport.cpp

LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR  = ../../DaisySP/

# Generate a link map to inspect size/symbol pulls
# LDFLAGS += -Wl,-Map=build/$(TARGET).map,--cref
LDSCRIPT = ./alt_sram.lds

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
