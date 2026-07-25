#pragma once

#include <cstdint>

namespace major_midi
{
static constexpr uint8_t kChannelCount = 16;

struct MajorMidiSettings
{
    static constexpr int8_t kNoOverride = -1;

    uint8_t  master_volume_max = 127;
    uint8_t  expression_max    = 127;
    uint8_t  reverb_max        = 127;
    uint8_t  chorus_max        = 127;
    int8_t   transpose         = 0;
    uint16_t bpm_override      = 0;
    bool     loop_enabled      = false;
    uint32_t loop_start_tick   = 0;
    uint32_t loop_length_ticks = 1920;
    uint16_t loop_start_measure = 1;
    uint8_t  loop_start_beat   = 1;
    uint8_t  loop_start_sub    = 1;
    uint16_t loop_length_beats = 0;
    int8_t   program_override[kChannelCount];
    int8_t   pan_override[kChannelCount];
    uint8_t  volume[kChannelCount];
    uint8_t  reverb_send[kChannelCount];
    uint8_t  chorus_send[kChannelCount];
    bool     muted[kChannelCount];

    void Reset();
};

bool     HasMajorMidiBpmOverride(const MajorMidiSettings& settings);
uint32_t MajorMidiTempoUsecPerQuarter(const MajorMidiSettings& settings);

} // namespace major_midi
