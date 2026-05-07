#pragma once

#include "app_state.h"
#include "daisy_patch_sm.h"
#include "mixer_transport.h"

namespace major_midi
{

class CvGateEngine
{
  public:
    struct LoopSyncDebugEvent
    {
        bool     available = false;
        uint32_t gate_index = 0;
        uint32_t loop_index = 0;
        uint32_t expected = 0;
        uint32_t seen = 0;
        uint32_t wrap_tail = 0;
        uint32_t wrap_head = 0;
        bool     forced = false;
        uint64_t block_start_tick = 0;
        uint64_t block_end_tick = 0;
    };

    void Init(daisy::patch_sm::DaisyPatchSM& hw, float sample_rate);
    void Update(const AppState& state, MixerTransport& transport);
    int  EffectiveBpm(const AppState& state) const;
    bool ConsumeLoopSyncDebugEvent(LoopSyncDebugEvent& out);

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
    uint8_t                        active_gate_note_[2]{};
    uint8_t                        active_gate_channel_[2]{};
    bool                           gate_note_active_[2]{};
    uint32_t                       gate_out_pulse_remaining_[2]{};
    uint32_t                       sync_loop_pulse_count_[2]{};
    uint32_t                       sync_loop_index_[2]{};
    volatile bool                  debug_event_pending_ = false;
    LoopSyncDebugEvent             debug_event_{};
};

} // namespace major_midi
