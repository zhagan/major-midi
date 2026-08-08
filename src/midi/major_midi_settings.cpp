#include "major_midi_settings.h"

namespace major_midi
{
void MajorMidiSettings::Reset()
{
    master_volume_max = 127;
    expression_max    = 127;
    reverb_max        = 127;
    chorus_max        = 127;
    transpose         = 0;
    bpm_override      = 0;
    loop_enabled      = false;
    loop_start_tick   = 0;
    loop_length_ticks = 1920;
    loop_start_measure = 1;
    loop_start_beat   = 1;
    loop_start_sub    = 1;
    loop_length_beats = 0;
    for(uint8_t i = 0; i < kChannelCount; i++)
    {
        program_override[i] = kNoOverride;
        pan_override[i]     = kNoOverride;
        volume[i]           = 100;
        reverb_send[i]      = 0;
        chorus_send[i]      = 0;
        muted[i]            = false;
    }
}

bool HasMajorMidiBpmOverride(const MajorMidiSettings& settings)
{
    return settings.bpm_override > 0;
}

uint32_t MajorMidiTempoUsecPerQuarter(const MajorMidiSettings& settings)
{
    if(!HasMajorMidiBpmOverride(settings))
        return 500000;
    if(settings.bpm_override == 0)
        return 500000;
    return 60000000u / settings.bpm_override;
}
} // namespace major_midi
