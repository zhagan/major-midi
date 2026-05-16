#include "song_config_persist.h"

#include <cstring>

#include "media_library.h"
#include "persist_file.h"

namespace major_midi
{
namespace
{
static constexpr uint8_t kMagic[4] = {'M', 'M', 'S', 'C'};
static constexpr uint8_t kVersion  = 5;
static constexpr size_t  kFileSize = 192;

uint32_t ReadUint32BE(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16)
           | (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

void WriteUint32BE(uint8_t* data, uint32_t value)
{
    data[0] = static_cast<uint8_t>((value >> 24) & 0xFFu);
    data[1] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    data[2] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    data[3] = static_cast<uint8_t>(value & 0xFFu);
}

bool ValidChannel(uint8_t ch)
{
    return ch <= 15;
}

bool ValidCc(uint8_t cc)
{
    return cc <= 127;
}

uint8_t PackChannelOutput(const MidiChannelOutputRouting& routing)
{
    return static_cast<uint8_t>((routing.notes ? 0x01 : 0x00)
                                | (routing.ccs ? 0x02 : 0x00)
                                | (routing.programs ? 0x04 : 0x00));
}

void UnpackChannelOutput(uint8_t packed, MidiChannelOutputRouting& routing)
{
    routing.notes    = (packed & 0x01) != 0;
    routing.ccs      = (packed & 0x02) != 0;
    routing.programs = (packed & 0x04) != 0;
}

void CopyNameField(char* out, size_t out_sz, const uint8_t* in, size_t field_sz)
{
    if(out == nullptr || out_sz == 0)
        return;
    const size_t copy_len = field_sz < (out_sz - 1) ? field_sz : (out_sz - 1);
    std::memcpy(out, in, copy_len);
    out[copy_len] = '\0';
}

void WriteConfig(uint8_t* out, const AppState& state, const char* sf2_name)
{
    size_t offset = 0;
    out[offset++] = kMagic[0];
    out[offset++] = kMagic[1];
    out[offset++] = kMagic[2];
    out[offset++] = kMagic[3];
    out[offset++] = kVersion;

    for(size_t i = 0; i < 2; i++)
    {
        out[offset++] = static_cast<uint8_t>(state.cv_gate.cv_in[i].mode);
        out[offset++] = state.cv_gate.cv_in[i].channel;
        out[offset++] = state.cv_gate.cv_in[i].cc;
    }
    for(size_t i = 0; i < 2; i++)
    {
        out[offset++] = static_cast<uint8_t>(state.cv_gate.gate_in[i].mode);
        out[offset++] = state.cv_gate.gate_in[i].channel;
    }
    for(size_t i = 0; i < 2; i++)
    {
        out[offset++] = static_cast<uint8_t>(state.cv_gate.gate_out[i].mode);
        out[offset++] = state.cv_gate.gate_out[i].channel;
        out[offset++] = static_cast<uint8_t>(state.cv_gate.gate_out[i].sync_resolution);
    }
    for(size_t i = 0; i < 2; i++)
    {
        out[offset++] = static_cast<uint8_t>(state.cv_gate.cv_out[i].mode);
        out[offset++] = state.cv_gate.cv_out[i].channel;
        out[offset++] = state.cv_gate.cv_out[i].cc;
        out[offset++] = static_cast<uint8_t>(state.cv_gate.cv_out[i].priority);
    }

    out[offset++] = (state.midi_routing.usb.transport ? 0x01 : 0x00)
                    | (state.midi_routing.usb.clock ? 0x02 : 0x00);
    out[offset++] = (state.midi_routing.uart.transport ? 0x01 : 0x00)
                    | (state.midi_routing.uart.clock ? 0x02 : 0x00);
    for(size_t ch = 0; ch < 16; ch++)
        out[offset++] = PackChannelOutput(state.midi_routing.usb.channels[ch]);
    for(size_t ch = 0; ch < 16; ch++)
        out[offset++] = PackChannelOutput(state.midi_routing.uart.channels[ch]);
    out[offset++] = state.midi_routing.usb_in_to_uart ? 1 : 0;
    out[offset++] = state.midi_routing.uart_in_to_usb ? 1 : 0;

    uint16_t mute_mask = 0;
    for(size_t i = 0; i < 16; i++)
    {
        const ChannelState& channel = state.channels[i];
        out[offset++]               = channel.volume;
        out[offset++]               = channel.pan;
        out[offset++]               = channel.reverb_send;
        out[offset++]               = channel.chorus_send;
        out[offset++]               = static_cast<uint8_t>(channel.program_override);
        if(channel.muted)
            mute_mask |= static_cast<uint16_t>(1u << i);
    }
    out[offset++] = static_cast<uint8_t>((mute_mask >> 8) & 0xFF);
    out[offset++] = static_cast<uint8_t>(mute_mask & 0xFF);
    out[offset++] = state.song_loop_enabled ? 1 : 0;
    WriteUint32BE(out + offset, state.loop_start_tick);
    offset += 4;
    WriteUint32BE(out + offset, state.loop_length_ticks > 0 ? state.loop_length_ticks : 1u);
    offset += 4;
    std::memset(out + offset, 0, MediaLibrary::kNameMax);
    if(sf2_name != nullptr && sf2_name[0] != '\0')
    {
        const size_t name_len = std::strlen(sf2_name);
        const size_t copy_len = name_len < (MediaLibrary::kNameMax - 1) ? name_len
                                                                         : (MediaLibrary::kNameMax - 1);
        std::memcpy(out + offset, sf2_name, copy_len);
    }
}

bool ReadConfig(const uint8_t* in, AppState& state, char* sf2_name, size_t sf2_name_sz)
{
    if(in[0] != kMagic[0] || in[1] != kMagic[1] || in[2] != kMagic[2] || in[3] != kMagic[3])
        return false;
    const uint8_t version = in[4];
    if(version < 2 || version > kVersion)
        return false;

    size_t offset = 5;
    for(size_t i = 0; i < 2; i++)
    {
        state.cv_gate.cv_in[i].mode    = static_cast<CvInMode>(in[offset++]);
        state.cv_gate.cv_in[i].channel = in[offset++];
        state.cv_gate.cv_in[i].cc      = in[offset++];
        if(state.cv_gate.cv_in[i].mode > CvInMode::NotePitch
           || !ValidChannel(state.cv_gate.cv_in[i].channel)
           || !ValidCc(state.cv_gate.cv_in[i].cc))
            return false;
    }
    for(size_t i = 0; i < 2; i++)
    {
        state.cv_gate.gate_in[i].mode = static_cast<GateInMode>(in[offset++]);
        if(version >= 2)
            state.cv_gate.gate_in[i].channel = in[offset++];
        else
            state.cv_gate.gate_in[i].channel = 0;
        if(state.cv_gate.gate_in[i].mode > GateInMode::NoteTrigger
           || !ValidChannel(state.cv_gate.gate_in[i].channel))
            return false;
    }
    for(size_t i = 0; i < 2; i++)
    {
        state.cv_gate.gate_out[i].mode = static_cast<GateOutMode>(in[offset++]);
        state.cv_gate.gate_out[i].channel = in[offset++];
        state.cv_gate.gate_out[i].sync_resolution = static_cast<SyncResolution>(in[offset++]);
        if(state.cv_gate.gate_out[i].mode > GateOutMode::ChannelGate
           || !ValidChannel(state.cv_gate.gate_out[i].channel)
           || state.cv_gate.gate_out[i].sync_resolution > SyncResolution::Div64)
            return false;
    }
    for(size_t i = 0; i < 2; i++)
    {
        state.cv_gate.cv_out[i].mode     = static_cast<CvOutMode>(in[offset++]);
        state.cv_gate.cv_out[i].channel  = in[offset++];
        state.cv_gate.cv_out[i].cc       = in[offset++];
        state.cv_gate.cv_out[i].priority = static_cast<NotePriority>(in[offset++]);
        if(state.cv_gate.cv_out[i].mode > CvOutMode::ChannelCc
           || !ValidChannel(state.cv_gate.cv_out[i].channel)
           || !ValidCc(state.cv_gate.cv_out[i].cc)
           || state.cv_gate.cv_out[i].priority > NotePriority::Lowest)
            return false;
    }

    const uint8_t usb_packed  = in[offset++];
    const uint8_t uart_packed = in[offset++];
    if(version < 5)
    {
        if((usb_packed & ~0x1F) != 0 || (uart_packed & ~0x1F) != 0)
            return false;
        state.midi_routing.usb.transport  = (usb_packed & 0x08) != 0;
        state.midi_routing.usb.clock      = (usb_packed & 0x10) != 0;
        state.midi_routing.uart.transport = (uart_packed & 0x08) != 0;
        state.midi_routing.uart.clock     = (uart_packed & 0x10) != 0;
        for(size_t ch = 0; ch < 16; ch++)
        {
            state.midi_routing.usb.channels[ch].notes    = (usb_packed & 0x01) != 0;
            state.midi_routing.usb.channels[ch].ccs      = (usb_packed & 0x02) != 0;
            state.midi_routing.usb.channels[ch].programs = (usb_packed & 0x04) != 0;
            state.midi_routing.uart.channels[ch].notes    = (uart_packed & 0x01) != 0;
            state.midi_routing.uart.channels[ch].ccs      = (uart_packed & 0x02) != 0;
            state.midi_routing.uart.channels[ch].programs = (uart_packed & 0x04) != 0;
        }
        state.midi_routing.usb_in_to_uart = in[offset++] != 0;
        state.midi_routing.uart_in_to_usb = in[offset++] != 0;
    }
    else
    {
        if((usb_packed & ~0x03) != 0 || (uart_packed & ~0x03) != 0)
            return false;
        state.midi_routing.usb.transport  = (usb_packed & 0x01) != 0;
        state.midi_routing.usb.clock      = (usb_packed & 0x02) != 0;
        state.midi_routing.uart.transport = (uart_packed & 0x01) != 0;
        state.midi_routing.uart.clock     = (uart_packed & 0x02) != 0;
        for(size_t ch = 0; ch < 16; ch++)
        {
            if((in[offset] & ~0x07) != 0)
                return false;
            UnpackChannelOutput(in[offset++], state.midi_routing.usb.channels[ch]);
        }
        for(size_t ch = 0; ch < 16; ch++)
        {
            if((in[offset] & ~0x07) != 0)
                return false;
            UnpackChannelOutput(in[offset++], state.midi_routing.uart.channels[ch]);
        }
        state.midi_routing.usb_in_to_uart = in[offset++] != 0;
        state.midi_routing.uart_in_to_usb = in[offset++] != 0;
    }

    for(size_t i = 0; i < 16; i++)
    {
        ChannelState& channel    = state.channels[i];
        channel.volume           = in[offset++];
        channel.pan              = in[offset++];
        channel.reverb_send      = in[offset++];
        channel.chorus_send      = in[offset++];
        channel.program_override = static_cast<int8_t>(in[offset++]);
        if(channel.volume > 127 || channel.pan > 127 || channel.reverb_send > 127
           || channel.chorus_send > 127 || channel.program_override > 127)
            return false;
    }
    const uint16_t mute_mask = (uint16_t(in[offset]) << 8) | in[offset + 1];
    offset += 2;
    for(size_t i = 0; i < 16; i++)
        state.channels[i].muted = ((mute_mask >> i) & 0x01u) != 0;

    if(version >= 3)
    {
        state.song_loop_enabled = in[offset++] != 0;
        state.loop_start_tick   = ReadUint32BE(in + offset);
        offset += 4;
        state.loop_length_ticks = ReadUint32BE(in + offset);
        offset += 4;
        if(state.loop_length_ticks < 1)
            state.loop_length_ticks = 1;
    }
    if(version >= 4)
        CopyNameField(sf2_name, sf2_name_sz, in + offset, MediaLibrary::kNameMax);
    else if(sf2_name != nullptr && sf2_name_sz > 0)
        sf2_name[0] = '\0';

    return true;
}
} // namespace

bool LoadSongConfig(const char* path, AppState& state, char* sf2_name, size_t sf2_name_sz)
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

    return ReadConfig(data, state, sf2_name, sf2_name_sz);
}

bool SaveSongConfig(const char* path,
                    const AppState& state,
                    const char*     sf2_name,
                    PersistWriteStage* failed_stage,
                    int*               result_code,
                    PersistProgressFn  progress_fn,
                    void*              progress_ctx)
{
    uint8_t data[kFileSize];
    WriteConfig(data, state, sf2_name);

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
