#pragma once

#include <cstddef>

#include "app_state.h"

namespace major_midi
{

bool LoadBootState(AppState& state, char* midi_name, size_t midi_name_sz);
bool SaveBootState(const AppState& state, const char* midi_name);

} // namespace major_midi
