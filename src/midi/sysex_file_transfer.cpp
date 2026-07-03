#include "sysex_file_transfer.h"

#include <cctype>
#include <cstdio>
#include <cstring>

extern "C"
{
#include "ff.h"
}

namespace major_midi
{
namespace
{
FIL g_sysex_transfer_file;
}

void SysExFileTransfer::Init(RefreshMediaFn refresh_media, CompleteFn complete, void* context)
{
    refresh_media_         = refresh_media;
    complete_              = complete;
    refresh_media_context_ = context;
}

bool SysExFileTransfer::HandleUsbMidiEvent(const daisy::MidiEvent& msg,
                                           daisy::MidiUsbHandler& usb_midi)
{
    if(!IsTransferMessage(msg))
        return false;

    const auto command = static_cast<Command>(msg.sysex_data[3]);
    switch(command)
    {
        case Command::Start: HandleStart(msg, usb_midi); break;
        case Command::Data: HandleData(msg, usb_midi); break;
        case Command::End: HandleEnd(msg, usb_midi); break;
        case Command::Cancel: HandleCancel(usb_midi); break;
        default: SendAck(usb_midi, command, Status::Invalid); break;
    }
    return true;
}

bool SysExFileTransfer::IsTransferMessage(const daisy::MidiEvent& msg) const
{
    if(msg.type != daisy::MidiMessageType::SystemCommon
       || msg.sc_type != daisy::SystemCommonType::SystemExclusive
       || msg.sysex_message_len < 4 || msg.sysex_data[0] != kManufacturerId
       || msg.sysex_data[1] != kMagic0 || msg.sysex_data[2] != kMagic1)
        return false;

    switch(static_cast<Command>(msg.sysex_data[3]))
    {
        case Command::Start:
        case Command::Data:
        case Command::End:
        case Command::Cancel: return true;
        default: return false;
    }
}

void SysExFileTransfer::HandleStart(const daisy::MidiEvent& msg,
                                    daisy::MidiUsbHandler& usb_midi)
{
    if(msg.sysex_message_len < 5)
    {
        SendAck(usb_midi, Command::Start, Status::Invalid);
        return;
    }

    if(active_)
        AbortTransfer(true);

    const size_t filename_len = msg.sysex_data[4];
    if(filename_len == 0 || filename_len > kMaxFilename
       || (5 + filename_len) > msg.sysex_message_len)
    {
        SendAck(usb_midi, Command::Start, Status::Filename);
        return;
    }

    char filename[kMaxFilename + 5]{};
    if(!NormalizeMidiFilename(msg.sysex_data + 5,
                              filename_len,
                              filename,
                              sizeof(filename)))
    {
        SendAck(usb_midi, Command::Start, Status::Filename);
        return;
    }

    const FRESULT mkdir_result = f_mkdir("0:/midi");
    if(mkdir_result != FR_OK && mkdir_result != FR_EXIST)
    {
        SendAck(usb_midi, Command::Start, Status::FsError, mkdir_result);
        return;
    }

    std::snprintf(active_path_, sizeof(active_path_), "0:/midi/%s", filename);
    const FRESULT open_result
        = f_open(&g_sysex_transfer_file, active_path_, FA_CREATE_ALWAYS | FA_WRITE);
    if(open_result != FR_OK)
    {
        active_path_[0] = '\0';
        SendAck(usb_midi, Command::Start, Status::FsError, open_result);
        return;
    }

    active_            = true;
    expected_sequence_ = 0;
    SendAck(usb_midi, Command::Start, Status::Ok);
}

void SysExFileTransfer::HandleData(const daisy::MidiEvent& msg,
                                   daisy::MidiUsbHandler& usb_midi)
{
    if(!active_)
    {
        SendAck(usb_midi, Command::Data, Status::NotReceiving);
        return;
    }

    if(msg.sysex_message_len < 7)
    {
        SendAck(usb_midi, Command::Data, Status::Invalid);
        return;
    }

    const uint16_t sequence = static_cast<uint16_t>(msg.sysex_data[4])
                              | (static_cast<uint16_t>(msg.sysex_data[5]) << 7);
    const size_t raw_len = msg.sysex_data[6];
    if(sequence != expected_sequence_)
    {
        SendAck(usb_midi, Command::Data, Status::Sequence, expected_sequence_);
        return;
    }
    if(raw_len == 0 || raw_len > kMaxRawChunk)
    {
        SendAck(usb_midi, Command::Data, Status::Invalid, sequence);
        return;
    }

    uint8_t decoded[kMaxRawChunk]{};
    const uint8_t* packed     = msg.sysex_data + 7;
    const size_t   packed_len = msg.sysex_message_len - 7;
    if(!DecodePacked7Bit(packed, packed_len, decoded, raw_len))
    {
        SendAck(usb_midi, Command::Data, Status::Decode, sequence);
        return;
    }

    UINT          written      = 0;
    const FRESULT write_result = f_write(&g_sysex_transfer_file, decoded, raw_len, &written);
    if(write_result != FR_OK || written != raw_len)
    {
        AbortTransfer(true);
        if(complete_ != nullptr)
            complete_(false, refresh_media_context_);
        SendAck(usb_midi, Command::Data, Status::FsError, write_result);
        return;
    }

    expected_sequence_++;
    SendAck(usb_midi, Command::Data, Status::Ok, sequence);
}

void SysExFileTransfer::HandleEnd(const daisy::MidiEvent& msg,
                                  daisy::MidiUsbHandler& usb_midi)
{
    if(!active_)
    {
        SendAck(usb_midi, Command::End, Status::NotReceiving);
        return;
    }
    if(msg.sysex_message_len < 6)
    {
        SendAck(usb_midi, Command::End, Status::Invalid);
        return;
    }

    const uint16_t final_sequence = static_cast<uint16_t>(msg.sysex_data[4])
                                    | (static_cast<uint16_t>(msg.sysex_data[5]) << 7);
    if(final_sequence != expected_sequence_)
    {
        SendAck(usb_midi, Command::End, Status::Sequence, expected_sequence_);
        return;
    }

    const FRESULT sync_result  = f_sync(&g_sysex_transfer_file);
    const FRESULT close_result = f_close(&g_sysex_transfer_file);
    active_                    = false;
    expected_sequence_         = 0;
    active_path_[0]            = '\0';

    if(sync_result != FR_OK || close_result != FR_OK)
    {
        if(complete_ != nullptr)
            complete_(false, refresh_media_context_);
        SendAck(usb_midi,
                Command::End,
                Status::FsError,
                sync_result != FR_OK ? sync_result : close_result);
        return;
    }

    if(refresh_media_ != nullptr)
        refresh_media_(refresh_media_context_);
    if(complete_ != nullptr)
        complete_(true, refresh_media_context_);

    SendAck(usb_midi, Command::End, Status::Ok, final_sequence);
}

void SysExFileTransfer::HandleCancel(daisy::MidiUsbHandler& usb_midi)
{
    AbortTransfer(true);
    if(complete_ != nullptr)
        complete_(false, refresh_media_context_);
    SendAck(usb_midi, Command::Cancel, Status::Ok);
}

void SysExFileTransfer::SendAck(daisy::MidiUsbHandler& usb_midi,
                                Command               request,
                                Status                status,
                                uint16_t              value)
{
    uint8_t reply[]
        = {0xF0,
           kManufacturerId,
           kMagic0,
           kMagic1,
           static_cast<uint8_t>(Command::Ack),
           static_cast<uint8_t>(request),
           static_cast<uint8_t>(status),
           static_cast<uint8_t>(value & 0x7F),
           static_cast<uint8_t>((value >> 7) & 0x7F),
           0xF7};
    usb_midi.SendMessage(reply, sizeof(reply));
}

void SysExFileTransfer::AbortTransfer(bool delete_partial)
{
    if(active_)
        f_close(&g_sysex_transfer_file);
    if(delete_partial && active_path_[0] != '\0')
        f_unlink(active_path_);

    active_            = false;
    expected_sequence_ = 0;
    active_path_[0]    = '\0';
}

bool SysExFileTransfer::NormalizeMidiFilename(const uint8_t* src,
                                              size_t         src_len,
                                              char*          out,
                                              size_t         out_sz) const
{
    if(src == nullptr || out == nullptr || out_sz < 5 || src_len == 0)
        return false;

    size_t start = 0;
    for(size_t i = 0; i < src_len; i++)
    {
        if(src[i] == '/' || src[i] == '\\')
            start = i + 1;
    }
    if(start >= src_len)
        return false;

    size_t pos = 0;
    for(size_t i = start; i < src_len && pos + 1 < out_sz; i++)
    {
        char ch = static_cast<char>(src[i] & 0x7F);
        if(std::isalnum(static_cast<unsigned char>(ch)) || ch == '.'
           || ch == '_' || ch == '-' || ch == ' ')
        {
            out[pos++] = ch;
        }
        else
        {
            out[pos++] = '_';
        }
    }
    out[pos] = '\0';
    if(pos == 0)
        return false;

    while(out[0] == '.')
        std::memmove(out, out + 1, std::strlen(out));
    if(out[0] == '\0')
        return false;

    const char* dot = std::strrchr(out, '.');
    if(dot == nullptr)
    {
        if(pos + 4 >= out_sz)
            return false;
        std::strcat(out, ".mid");
        return true;
    }

    if(std::strcmp(dot, ".mid") == 0 || std::strcmp(dot, ".MID") == 0
       || std::strcmp(dot, ".Mid") == 0 || std::strcmp(dot, ".mId") == 0
       || std::strcmp(dot, ".miD") == 0 || std::strcmp(dot, ".mID") == 0
       || std::strcmp(dot, ".MId") == 0 || std::strcmp(dot, ".MiD") == 0)
        return true;

    if(std::strcmp(dot, ".midi") == 0 || std::strcmp(dot, ".MIDI") == 0
       || std::strcmp(dot, ".Midi") == 0)
    {
        const size_t stem_len = static_cast<size_t>(dot - out);
        if(stem_len + 4 >= out_sz)
            return false;
        out[stem_len] = '\0';
        std::strcat(out, ".mid");
        return true;
    }

    const size_t stem_len = static_cast<size_t>(dot - out);
    if(stem_len + 4 >= out_sz)
        return false;
    out[stem_len] = '\0';
    std::strcat(out, ".mid");
    return true;
}

bool SysExFileTransfer::DecodePacked7Bit(const uint8_t* src,
                                         size_t         src_len,
                                         uint8_t*       out,
                                         size_t         raw_len) const
{
    if(src == nullptr || out == nullptr || raw_len == 0)
        return false;

    size_t src_pos = 0;
    size_t out_pos = 0;
    while(out_pos < raw_len)
    {
        if(src_pos >= src_len)
            return false;

        const uint8_t msbs        = src[src_pos++];
        const size_t  block_count = (raw_len - out_pos) > 7 ? 7 : (raw_len - out_pos);
        if((src_pos + block_count) > src_len)
            return false;

        for(size_t i = 0; i < block_count; i++)
        {
            const uint8_t low = src[src_pos++];
            out[out_pos++]    = low | (((msbs >> i) & 0x01) << 7);
        }
    }

    return src_pos == src_len;
}

} // namespace major_midi
