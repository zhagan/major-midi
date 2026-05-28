#include "midi_routing_persist.h"
#include "persist_file.h"

namespace major_midi
{
namespace
{
static constexpr uint8_t kMagic[4] = {'M', 'M', 'M', 'R'};
static constexpr uint8_t kVersion  = 3;
static constexpr size_t  kFileSize = 73;

uint8_t PackChannelOutput(const MidiChannelOutputRouting& routing)
{
    return (routing.notes ? 0x01 : 0x00) | (routing.ccs ? 0x02 : 0x00)
           | (routing.programs ? 0x04 : 0x00);
}

void UnpackChannelOutput(uint8_t packed, MidiChannelOutputRouting& routing)
{
    routing.notes    = (packed & 0x01) != 0;
    routing.ccs      = (packed & 0x02) != 0;
    routing.programs = (packed & 0x04) != 0;
}

bool ValidMode(uint8_t raw)
{
    return raw <= static_cast<uint8_t>(MidiOutputMode::Matrix);
}

void WriteConfig(uint8_t* out, const MidiRoutingConfig& config)
{
    out[0] = kMagic[0];
    out[1] = kMagic[1];
    out[2] = kMagic[2];
    out[3] = kMagic[3];
    out[4] = kVersion;
    out[5] = static_cast<uint8_t>(config.usb.mode);
    out[6] = static_cast<uint8_t>(config.uart.mode);
    out[7] = (config.usb.transport ? 0x01 : 0x00)
             | (config.usb.clock ? 0x02 : 0x00);
    out[8] = (config.uart.transport ? 0x01 : 0x00)
             | (config.uart.clock ? 0x02 : 0x00);
    size_t offset = 9;
    for(size_t ch = 0; ch < 16; ch++)
        out[offset++] = PackChannelOutput(config.usb.channels[ch]);
    for(size_t ch = 0; ch < 16; ch++)
        out[offset++] = config.usb.channels[ch].destination_channel;
    for(size_t ch = 0; ch < 16; ch++)
        out[offset++] = PackChannelOutput(config.uart.channels[ch]);
    for(size_t ch = 0; ch < 16; ch++)
        out[offset++] = config.uart.channels[ch].destination_channel;
    out[offset++] = config.usb_in_to_uart ? 1 : 0;
    out[offset++] = config.uart_in_to_usb ? 1 : 0;
}

bool ReadConfig(const uint8_t* in, MidiRoutingConfig& config)
{
    if(in[0] != kMagic[0] || in[1] != kMagic[1] || in[2] != kMagic[2]
       || in[3] != kMagic[3] || in[4] != kVersion)
        return false;

    if(!ValidMode(in[5]) || !ValidMode(in[6]) || (in[7] & ~0x03) != 0
       || (in[8] & ~0x03) != 0)
        return false;

    config.usb.mode       = static_cast<MidiOutputMode>(in[5]);
    config.uart.mode      = static_cast<MidiOutputMode>(in[6]);
    config.usb.transport  = (in[7] & 0x01) != 0;
    config.usb.clock      = (in[7] & 0x02) != 0;
    config.uart.transport = (in[8] & 0x01) != 0;
    config.uart.clock     = (in[8] & 0x02) != 0;

    size_t offset = 9;
    for(size_t ch = 0; ch < 16; ch++)
    {
        if((in[offset] & ~0x07) != 0)
            return false;
        UnpackChannelOutput(in[offset++], config.usb.channels[ch]);
    }
    for(size_t ch = 0; ch < 16; ch++)
    {
        if(in[offset] > 15)
            return false;
        config.usb.channels[ch].destination_channel = in[offset++];
    }
    for(size_t ch = 0; ch < 16; ch++)
    {
        if((in[offset] & ~0x07) != 0)
            return false;
        UnpackChannelOutput(in[offset++], config.uart.channels[ch]);
    }
    for(size_t ch = 0; ch < 16; ch++)
    {
        if(in[offset] > 15)
            return false;
        config.uart.channels[ch].destination_channel = in[offset++];
    }
    config.usb_in_to_uart = in[offset++] != 0;
    config.uart_in_to_usb = in[offset++] != 0;
    return true;
}
} // namespace

bool LoadMidiRoutingConfig(const char* path, MidiRoutingConfig& config)
{
    uint8_t data[kFileSize]{};
    FIL&    file = SharedPersistFile();
    if(f_open(&file, path, FA_READ) != FR_OK)
        return false;

    UINT read = 0;
    const FRESULT read_result  = f_read(&file, data, kFileSize, &read);
    const FRESULT close_result = f_close(&file);
    if(read_result != FR_OK || close_result != FR_OK || read != kFileSize)
        return false;

    return ReadConfig(data, config);
}

bool SaveMidiRoutingConfig(const char* path,
                           const MidiRoutingConfig& config,
                           PersistWriteStage*       failed_stage,
                           int*                     result_code,
                           PersistProgressFn        progress_fn,
                           void*                    progress_ctx)
{
    uint8_t data[kFileSize];
    WriteConfig(data, config);

    if(result_code != nullptr)
        *result_code = -1;

    if(failed_stage != nullptr)
        *failed_stage = PersistWriteStage::Open;
    if(progress_fn != nullptr)
        progress_fn(PersistWriteStage::Open, progress_ctx);
    FIL&          file        = SharedPersistFile();
    const FRESULT open_result = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
    if(open_result != FR_OK)
    {
        if(result_code != nullptr)
            *result_code = static_cast<int>(open_result);
        return false;
    }

    if(failed_stage != nullptr)
        *failed_stage = PersistWriteStage::Write;
    if(progress_fn != nullptr)
        progress_fn(PersistWriteStage::Write, progress_ctx);
    UINT written = 0;
    const FRESULT write_result = f_write(&file, data, kFileSize, &written);
    if(write_result != FR_OK && result_code != nullptr)
        *result_code = static_cast<int>(write_result);
    if(failed_stage != nullptr)
        *failed_stage = PersistWriteStage::Close;
    if(progress_fn != nullptr)
        progress_fn(PersistWriteStage::Close, progress_ctx);
    const FRESULT close_result = f_close(&file);
    if(result_code != nullptr)
        *result_code = static_cast<int>(close_result != FR_OK ? close_result : write_result);
    if(failed_stage != nullptr)
        *failed_stage = PersistWriteStage::Done;
    if(progress_fn != nullptr)
        progress_fn(PersistWriteStage::Done, progress_ctx);
    return write_result == FR_OK && written == kFileSize && close_result == FR_OK;
}

} // namespace major_midi
