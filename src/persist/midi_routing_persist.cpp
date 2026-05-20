#include "midi_routing_persist.h"
#include "persist_file.h"

namespace major_midi
{
namespace
{
static constexpr uint8_t kMagic[4] = {'M', 'M', 'M', 'R'};
static constexpr uint8_t kVersion  = 2;
static constexpr size_t  kFileSize = 41;

uint8_t PackChannelOutput(const MidiChannelOutputRouting& routing)
{
    return (routing.notes ? 0x01 : 0x00) | (routing.ccs ? 0x02 : 0x00)
           | (routing.programs ? 0x04 : 0x00);
}

void UnpackChannelOutput(uint8_t packed, MidiChannelOutputRouting& routing)
{
    routing.notes     = (packed & 0x01) != 0;
    routing.ccs       = (packed & 0x02) != 0;
    routing.programs  = (packed & 0x04) != 0;
}

void WriteConfig(uint8_t* out, const MidiRoutingConfig& config)
{
    out[0] = kMagic[0];
    out[1] = kMagic[1];
    out[2] = kMagic[2];
    out[3] = kMagic[3];
    out[4] = kVersion;
    out[5] = (config.usb.transport ? 0x01 : 0x00) | (config.usb.clock ? 0x02 : 0x00);
    out[6] = (config.uart.transport ? 0x01 : 0x00) | (config.uart.clock ? 0x02 : 0x00);
    for(size_t ch = 0; ch < 16; ch++)
        out[7 + ch] = PackChannelOutput(config.usb.channels[ch]);
    for(size_t ch = 0; ch < 16; ch++)
        out[23 + ch] = PackChannelOutput(config.uart.channels[ch]);
    out[7 + 16 + 16] = config.usb_in_to_uart ? 1 : 0;
    out[8 + 16 + 16] = config.uart_in_to_usb ? 1 : 0;
}

bool ReadConfig(const uint8_t* in, MidiRoutingConfig& config)
{
    if(in[0] != kMagic[0] || in[1] != kMagic[1] || in[2] != kMagic[2]
       || in[3] != kMagic[3])
        return false;

    if(in[4] == 1)
    {
        if((in[5] & ~0x1F) != 0 || (in[6] & ~0x1F) != 0 || in[7] > 1 || in[8] > 1)
            return false;

        const uint8_t usb_packed  = in[5];
        const uint8_t uart_packed = in[6];
        config.usb.transport      = (usb_packed & 0x08) != 0;
        config.usb.clock          = (usb_packed & 0x10) != 0;
        config.uart.transport     = (uart_packed & 0x08) != 0;
        config.uart.clock         = (uart_packed & 0x10) != 0;
        for(size_t ch = 0; ch < 16; ch++)
        {
            config.usb.channels[ch].notes    = (usb_packed & 0x01) != 0;
            config.usb.channels[ch].ccs      = (usb_packed & 0x02) != 0;
            config.usb.channels[ch].programs = (usb_packed & 0x04) != 0;
            config.uart.channels[ch].notes    = (uart_packed & 0x01) != 0;
            config.uart.channels[ch].ccs      = (uart_packed & 0x02) != 0;
            config.uart.channels[ch].programs = (uart_packed & 0x04) != 0;
        }
        config.usb_in_to_uart = in[7] != 0;
        config.uart_in_to_usb = in[8] != 0;
        return true;
    }

    if(in[4] != kVersion)
        return false;

    if((in[5] & ~0x03) != 0 || (in[6] & ~0x03) != 0 || in[37] > 1 || in[38] > 1)
        return false;

    config.usb.transport  = (in[5] & 0x01) != 0;
    config.usb.clock      = (in[5] & 0x02) != 0;
    config.uart.transport = (in[6] & 0x01) != 0;
    config.uart.clock     = (in[6] & 0x02) != 0;
    for(size_t ch = 0; ch < 16; ch++)
    {
        if((in[7 + ch] & ~0x07) != 0 || (in[23 + ch] & ~0x07) != 0)
            return false;
        UnpackChannelOutput(in[7 + ch], config.usb.channels[ch]);
        UnpackChannelOutput(in[23 + ch], config.uart.channels[ch]);
    }
    config.usb_in_to_uart = in[37] != 0;
    config.uart_in_to_usb = in[38] != 0;
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
    if(read_result != FR_OK || close_result != FR_OK || read < 9)
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
    if(write_result != FR_OK)
    {
        if(result_code != nullptr)
            *result_code = static_cast<int>(write_result);
    }
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
