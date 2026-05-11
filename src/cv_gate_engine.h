#pragma once

#include "app_state.h"
#include "daisy_patch_sm.h"
#include "mixer_transport.h"

namespace major_midi
{

class CvGateEngine
{
  public:
    void Init(daisy::patch_sm::DaisyPatchSM& hw, float sample_rate);
    void Update(const AppState& state, MixerTransport& transport);
    int  EffectiveBpm(const AppState& state) const;

  private:
    float ReadCvInput(size_t index) const;
    float PitchVoltageForChannel(const MixerTransport& transport,
                                 const CvOutputConfig& config) const;
    float CcVoltageForChannel(const MixerTransport& transport,
                              const CvOutputConfig& config) const;
    bool  SyncGateHigh(uint64_t cycle_sample,
                       int      bpm,
                       SyncResolution resolution) const;
    bool  SyncGateHighInBlock(uint64_t block_start_sample,
                              uint64_t block_end_sample,
                              int      bpm,
                              SyncResolution resolution) const;
    bool  ResetGateHigh(uint64_t cycle_sample,
                        int      bpm,
                        int      time_sig_num,
                        int      time_sig_den) const;
    bool  ResetGateHighInBlock(uint64_t block_start_sample,
                               uint64_t block_end_sample,
                               int      bpm,
                               int      time_sig_num,
                               int      time_sig_den) const;

    daisy::patch_sm::DaisyPatchSM* hw_          = nullptr;
    float                          sample_rate_ = 48000.0f;
    int                            live_bpm_    = 0;
    bool                           was_playing_ = false;
    uint8_t                        active_gate_note_[2]{};
    uint8_t                        active_gate_channel_[2]{};
    bool                           gate_note_active_[2]{};
    uint32_t                       gate_out_pulse_remaining_[2]{};
};

} // namespace major_midi
