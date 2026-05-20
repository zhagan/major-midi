#pragma once

#include <cstddef>

#include "app_state.h"

namespace major_midi
{

bool LoadSongConfig(const char* path, AppState& state, char* sf2_name = nullptr, size_t sf2_name_sz = 0);
bool SaveSongConfig(const char* path,
                    const AppState& state,
                    const char*     sf2_name      = nullptr,
                    PersistWriteStage* failed_stage = nullptr,
                    int*               result_code  = nullptr,
                    PersistProgressFn  progress_fn  = nullptr,
                    void*              progress_ctx = nullptr);

} // namespace major_midi
