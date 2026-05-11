#include "boot_state_persist.h"

#include <cstring>

#include "media_library.h"
#include "persist_file.h"

namespace major_midi
{
namespace
{
static constexpr char    kBootStatePath[] = "0:/major_midi_boot.cfg";
static constexpr uint8_t kMagic[4]        = {'M', 'M', 'B', 'T'};
static constexpr uint8_t kVersion         = 2;
static constexpr size_t  kNameOffset      = 5;
static constexpr size_t  kTimeoutOffset   = kNameOffset + MediaLibrary::kNameMax;
static constexpr size_t  kKnobModeOffset  = kTimeoutOffset + 2;
static constexpr size_t  kFileSize        = kKnobModeOffset + 1;

uint16_t ReadUint16BE(const uint8_t* data)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

void WriteUint16BE(uint8_t* data, uint16_t value)
{
    data[0] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    data[1] = static_cast<uint8_t>(value & 0xFFu);
}

bool ValidKnobPickupMode(uint8_t raw)
{
    return raw <= static_cast<uint8_t>(KnobPickupMode::Jump);
}
} // namespace

bool LoadBootState(AppState& state, char* midi_name, size_t midi_name_sz)
{
    if(midi_name == nullptr || midi_name_sz == 0)
        return false;

    midi_name[0] = '\0';
    uint8_t data[kFileSize]{};
    FIL&    file = SharedPersistFile();
    if(f_open(&file, kBootStatePath, FA_READ) != FR_OK)
        return false;

    UINT read = 0;
    const FRESULT read_result  = f_read(&file, data, kFileSize, &read);
    const FRESULT close_result = f_close(&file);
    if(read_result != FR_OK || close_result != FR_OK || read < (5 + MediaLibrary::kNameMax))
        return false;
    if(data[0] != kMagic[0] || data[1] != kMagic[1] || data[2] != kMagic[2] || data[3] != kMagic[3])
        return false;

    const uint8_t version = data[4];
    if(version < 1 || version > kVersion)
        return false;

    const size_t copy_len = MediaLibrary::kNameMax < (midi_name_sz - 1) ? MediaLibrary::kNameMax
                                                                         : (midi_name_sz - 1);
    std::memcpy(midi_name, data + kNameOffset, copy_len);
    midi_name[copy_len] = '\0';

    if(version >= 2 && read >= kFileSize)
    {
        const uint16_t timeout_s = ReadUint16BE(data + kTimeoutOffset);
        state.screen_saver_timeout_s = timeout_s;
        if(ValidKnobPickupMode(data[kKnobModeOffset]))
            state.knob_pickup_mode = static_cast<KnobPickupMode>(data[kKnobModeOffset]);
    }
    return midi_name[0] != '\0';
}

bool SaveBootState(const AppState& state, const char* midi_name)
{
    if(midi_name == nullptr || midi_name[0] == '\0')
        return false;

    uint8_t data[kFileSize]{};
    data[0] = kMagic[0];
    data[1] = kMagic[1];
    data[2] = kMagic[2];
    data[3] = kMagic[3];
    data[4] = kVersion;

    const size_t name_len = std::strlen(midi_name);
    const size_t copy_len = name_len < (MediaLibrary::kNameMax - 1) ? name_len
                                                                    : (MediaLibrary::kNameMax - 1);
    std::memcpy(data + kNameOffset, midi_name, copy_len);
    WriteUint16BE(data + kTimeoutOffset, state.screen_saver_timeout_s);
    data[kKnobModeOffset] = static_cast<uint8_t>(state.knob_pickup_mode);

    FIL&          file        = SharedPersistFile();
    const FRESULT open_result = f_open(&file, kBootStatePath, FA_CREATE_ALWAYS | FA_WRITE);
    if(open_result != FR_OK)
        return false;

    UINT written = 0;
    const FRESULT write_result = f_write(&file, data, kFileSize, &written);
    const FRESULT close_result = f_close(&file);
    return write_result == FR_OK && close_result == FR_OK && written == kFileSize;
}

} // namespace major_midi
