#include "sysex_remote_control.h"

#include <cstdio>
#include <cstring>

namespace major_midi
{

void SysExRemoteControl::Init(AppState*       state,
                              MediaLibrary*   library,
                              SyncLoopStateFn sync_loop_state,
                              void*           context)
{
    state_             = state;
    library_           = library;
    sync_loop_state_   = sync_loop_state;
    sync_loop_context_ = context;
}

bool SysExRemoteControl::HandleUsbMidiEvent(const daisy::MidiEvent& msg,
                                            daisy::MidiUsbHandler& usb_midi)
{
    if(!IsRemoteMessage(msg) || state_ == nullptr || library_ == nullptr)
        return false;

    const auto command = static_cast<Command>(msg.sysex_data[3]);
    switch(command)
    {
        case Command::GetStatus: HandleGetStatus(usb_midi); break;
        case Command::GetSelectedMidiPath: HandleGetSelectedMidiPath(usb_midi); break;
        case Command::GetSelectedSf2Path: HandleGetSelectedSf2Path(usb_midi); break;
        case Command::LoadMidi: HandleLoadMidi(msg, usb_midi); break;
        case Command::LoadSf2: HandleLoadSf2(msg, usb_midi); break;
        case Command::Transport: HandleTransport(msg, usb_midi); break;
        case Command::GetChannelState: HandleGetChannelState(msg, usb_midi); break;
        case Command::SetChannelState: HandleSetChannelState(msg, usb_midi); break;
        case Command::GetSongState: HandleGetSongState(usb_midi); break;
        case Command::SetSongState: HandleSetSongState(msg, usb_midi); break;
        case Command::SaveSongSettings: HandleSaveSongSettings(usb_midi); break;
        case Command::GetMidiDirCount: HandleGetMidiDirCount(msg, usb_midi); break;
        case Command::GetMidiDirEntry: HandleGetMidiDirEntry(msg, usb_midi); break;
        case Command::GetSf2DirCount: HandleGetSf2DirCount(msg, usb_midi); break;
        case Command::GetSf2DirEntry: HandleGetSf2DirEntry(msg, usb_midi); break;
        default:
            SendReply(usb_midi, command, Status::Invalid);
            break;
    }
    return true;
}

bool SysExRemoteControl::IsRemoteMessage(const daisy::MidiEvent& msg) const
{
    return msg.type == daisy::MidiMessageType::SystemCommon
           && msg.sc_type == daisy::SystemCommonType::SystemExclusive
           && msg.sysex_message_len >= 4 && msg.sysex_data[0] == kManufacturerId
           && msg.sysex_data[1] == kMagic0 && msg.sysex_data[2] == kMagic1
           && msg.sysex_data[3] >= static_cast<uint8_t>(Command::GetStatus)
           && msg.sysex_data[3] <= static_cast<uint8_t>(Command::GetSf2DirEntry);
}

bool SysExRemoteControl::ReadPathArg(const daisy::MidiEvent& msg,
                                     size_t                  offset,
                                     char*                   out_path,
                                     size_t                  out_sz)
{
    if(out_path == nullptr || out_sz == 0)
        return false;
    out_path[0] = '\0';

    if(msg.sysex_message_len <= offset)
        return false;

    const uint8_t len = msg.sysex_data[offset];
    if(len == 0)
        return true; // empty path == root, valid

    if(msg.sysex_message_len < offset + 1 + len)
        return false;

    const size_t copy_len
        = static_cast<size_t>(len) < (out_sz - 1) ? len : (out_sz - 1);
    std::memcpy(out_path, msg.sysex_data + offset + 1, copy_len);
    out_path[copy_len] = '\0';
    return true;
}

void SysExRemoteControl::HandleGetStatus(daisy::MidiUsbHandler& usb_midi)
{
    uint8_t payload[8]{};
    size_t  pos    = 0;
    payload[pos++] = state_->transport_playing ? 1 : 0;
    pos += Write14(payload + pos, static_cast<uint16_t>(state_->bpm));
    pos += Write14(payload + pos, state_->current_measure);
    payload[pos++] = static_cast<uint8_t>(state_->current_beat & 0x7F);
    SendReply(usb_midi, Command::GetStatus, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleGetSelectedMidiPath(daisy::MidiUsbHandler& usb_midi)
{
    const char*  path     = state_->selected_midi_path;
    const size_t path_len = std::strlen(path);
    uint8_t      payload[1 + MediaLibrary::kPathMax]{};
    size_t       pos      = 0;
    payload[pos++] = static_cast<uint8_t>(path_len & 0x7F);
    std::memcpy(payload + pos, path, path_len);
    pos += path_len;
    SendReply(usb_midi, Command::GetSelectedMidiPath, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleGetSelectedSf2Path(daisy::MidiUsbHandler& usb_midi)
{
    const char*  path     = state_->selected_sf2_path;
    const size_t path_len = std::strlen(path);
    uint8_t      payload[1 + MediaLibrary::kPathMax]{};
    size_t       pos      = 0;
    payload[pos++] = static_cast<uint8_t>(path_len & 0x7F);
    std::memcpy(payload + pos, path, path_len);
    pos += path_len;
    SendReply(usb_midi, Command::GetSelectedSf2Path, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleLoadMidi(const daisy::MidiEvent& msg,
                                        daisy::MidiUsbHandler& usb_midi)
{
    char path[MediaLibrary::kPathMax]{};
    if(!ReadPathArg(msg, 4, path, sizeof(path)))
    {
        SendReply(usb_midi, Command::LoadMidi, Status::Invalid);
        return;
    }
    if(path[0] == '\0' || !library_->MidiFileExists(path))
    {
        SendReply(usb_midi, Command::LoadMidi, Status::Range);
        return;
    }

    std::snprintf(
        state_->selected_midi_path, sizeof(state_->selected_midi_path), "%s", path);
    state_->pending_midi_load = true;
    state_->ui_mode           = UiMode::Performance;
    state_->menu_page         = MenuPage::Main;
    state_->menu_page_cursor  = 0;
    state_->menu_root_cursor  = 0;
    state_->menu_editing      = false;
    SendReply(usb_midi, Command::LoadMidi, Status::Ok);
}

void SysExRemoteControl::HandleLoadSf2(const daisy::MidiEvent& msg,
                                       daisy::MidiUsbHandler& usb_midi)
{
    char path[MediaLibrary::kPathMax]{};
    if(!ReadPathArg(msg, 4, path, sizeof(path)))
    {
        SendReply(usb_midi, Command::LoadSf2, Status::Invalid);
        return;
    }
    if(path[0] == '\0' || !library_->SoundFontFileExists(path))
    {
        SendReply(usb_midi, Command::LoadSf2, Status::Range);
        return;
    }

    std::snprintf(
        state_->selected_sf2_path, sizeof(state_->selected_sf2_path), "%s", path);
    state_->pending_sf2_load = true;
    state_->ui_mode          = UiMode::Performance;
    state_->menu_page        = MenuPage::Main;
    state_->menu_page_cursor = 0;
    state_->menu_root_cursor = 0;
    state_->menu_editing     = false;
    SendReply(usb_midi, Command::LoadSf2, Status::Ok);
}

void SysExRemoteControl::HandleTransport(const daisy::MidiEvent& msg,
                                         daisy::MidiUsbHandler& usb_midi)
{
    if(msg.sysex_message_len < 5)
    {
        SendReply(usb_midi, Command::Transport, Status::Invalid);
        return;
    }

    switch(msg.sysex_data[4])
    {
        case 0x00: state_->transport_playing = false; break;
        case 0x01: state_->transport_playing = true; break;
        case 0x02: state_->transport_playing = !state_->transport_playing; break;
        default:
            SendReply(usb_midi, Command::Transport, Status::Invalid);
            return;
    }

    uint8_t payload[1]{static_cast<uint8_t>(state_->transport_playing ? 1 : 0)};
    SendReply(usb_midi, Command::Transport, Status::Ok, payload, 1);
}

void SysExRemoteControl::HandleGetChannelState(const daisy::MidiEvent& msg,
                                               daisy::MidiUsbHandler& usb_midi)
{
    if(msg.sysex_message_len < 6)
    {
        SendReply(usb_midi, Command::GetChannelState, Status::Invalid);
        return;
    }

    const uint16_t index = Read14(msg.sysex_data + 4);
    if(index >= 16)
    {
        SendReply(usb_midi, Command::GetChannelState, Status::Range);
        return;
    }

    const auto& ch = state_->channels[index];
    uint8_t payload[16]{};
    size_t  pos = 0;
    pos += Write14(payload + pos, index);
    payload[pos++] = ch.volume & 0x7F;
    payload[pos++] = ch.pan & 0x7F;
    payload[pos++] = ch.reverb_send & 0x7F;
    payload[pos++] = ch.chorus_send & 0x7F;
    payload[pos++] = ch.muted ? 1 : 0;
    pos += Write14(payload + pos, ch.program_override >= 0
                                      ? static_cast<uint16_t>(ch.program_override)
                                      : 128u);
    payload[pos++] = ch.current_program & 0x7F;
    SendReply(usb_midi, Command::GetChannelState, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleSetChannelState(const daisy::MidiEvent& msg,
                                               daisy::MidiUsbHandler& usb_midi)
{
    if(msg.sysex_message_len < 12)
    {
        SendReply(usb_midi, Command::SetChannelState, Status::Invalid);
        return;
    }

    const uint16_t index = Read14(msg.sysex_data + 4);
    if(index >= 16)
    {
        SendReply(usb_midi, Command::SetChannelState, Status::Range);
        return;
    }

    const uint8_t volume      = msg.sysex_data[6];
    const uint8_t pan         = msg.sysex_data[7];
    const uint8_t reverb_send = msg.sysex_data[8];
    const uint8_t chorus_send = msg.sysex_data[9];
    const uint8_t muted       = msg.sysex_data[10];
    const uint16_t program    = Read14(msg.sysex_data + 11);
    if(volume > 127 || pan > 127 || reverb_send > 127 || chorus_send > 127
       || muted > 1 || program > 128)
    {
        SendReply(usb_midi, Command::SetChannelState, Status::Range);
        return;
    }

    auto& ch            = state_->channels[index];
    ch.volume           = volume;
    ch.pan              = pan;
    ch.reverb_send      = reverb_send;
    ch.chorus_send      = chorus_send;
    ch.muted            = muted != 0;
    ch.program_override = program == 128 ? -1 : static_cast<int8_t>(program);
    if(ch.program_override >= 0)
        ch.current_program = static_cast<uint8_t>(ch.program_override);
    state_->settings_dirty = true;

    uint8_t payload[2]{};
    Write14(payload, index);
    SendReply(usb_midi, Command::SetChannelState, Status::Ok, payload, 2);
}

void SysExRemoteControl::HandleGetSongState(daisy::MidiUsbHandler& usb_midi)
{
    uint8_t payload[16]{};
    size_t  pos = 0;
    pos += Write14(payload + pos, state_->song_bpm_override);
    payload[pos++] = state_->song_loop_enabled ? 1 : 0;
    pos += Write28(payload + pos, state_->loop_start_tick);
    pos += Write28(payload + pos, state_->loop_length_ticks);
    SendReply(usb_midi, Command::GetSongState, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleSetSongState(const daisy::MidiEvent& msg,
                                            daisy::MidiUsbHandler& usb_midi)
{
    if(msg.sysex_message_len < 14)
    {
        SendReply(usb_midi, Command::SetSongState, Status::Invalid);
        return;
    }

    const uint16_t bpm_override = Read14(msg.sysex_data + 4);
    const uint8_t  loop_enabled = msg.sysex_data[6];
    const uint32_t loop_start   = Read28(msg.sysex_data + 7);
    const uint32_t loop_length  = Read28(msg.sysex_data + 11);
    if(bpm_override > 300 || loop_enabled > 1 || loop_length == 0)
    {
        SendReply(usb_midi, Command::SetSongState, Status::Range);
        return;
    }

    state_->song_bpm_override = bpm_override;
    state_->song_loop_enabled = loop_enabled != 0;
    state_->loop_start_tick   = loop_start;
    state_->loop_length_ticks = loop_length;
    state_->settings_dirty    = true;
    if(sync_loop_state_ != nullptr)
        sync_loop_state_(sync_loop_context_);

    SendReply(usb_midi, Command::SetSongState, Status::Ok);
}

void SysExRemoteControl::HandleSaveSongSettings(daisy::MidiUsbHandler& usb_midi)
{
    state_->pending_save_settings = true;
    SendReply(usb_midi, Command::SaveSongSettings, Status::Ok);
}

void SysExRemoteControl::HandleGetMidiDirCount(const daisy::MidiEvent& msg,
                                               daisy::MidiUsbHandler& usb_midi)
{
    char dir_path[MediaLibrary::kPathMax]{};
    if(!ReadPathArg(msg, 4, dir_path, sizeof(dir_path)))
    {
        SendReply(usb_midi, Command::GetMidiDirCount, Status::Invalid);
        return;
    }

    const uint16_t count
        = static_cast<uint16_t>(library_->MidiDirEntryCount(dir_path));
    uint8_t payload[2]{};
    const size_t pos = Write14(payload, count);
    SendReply(usb_midi, Command::GetMidiDirCount, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleGetMidiDirEntry(const daisy::MidiEvent& msg,
                                               daisy::MidiUsbHandler& usb_midi)
{
    char dir_path[MediaLibrary::kPathMax]{};
    if(!ReadPathArg(msg, 4, dir_path, sizeof(dir_path)))
    {
        SendReply(usb_midi, Command::GetMidiDirEntry, Status::Invalid);
        return;
    }

    const size_t index_offset = 5 + std::strlen(dir_path);
    if(msg.sysex_message_len < index_offset + 2)
    {
        SendReply(usb_midi, Command::GetMidiDirEntry, Status::Invalid);
        return;
    }
    const uint16_t index = Read14(msg.sysex_data + index_offset);

    bool is_dir = false;
    char name[MediaLibrary::kNameMax]{};
    if(!library_->MidiDirEntryAt(dir_path, index, is_dir, name, sizeof(name)))
    {
        SendReply(usb_midi, Command::GetMidiDirEntry, Status::Range);
        return;
    }

    const size_t name_len = std::strlen(name);
    uint8_t      payload[2 + 1 + 1 + MediaLibrary::kNameMax]{};
    size_t       pos = 0;
    pos += Write14(payload + pos, index);
    payload[pos++] = is_dir ? 1 : 0;
    payload[pos++] = static_cast<uint8_t>(name_len & 0x7F);
    std::memcpy(payload + pos, name, name_len);
    pos += name_len;
    SendReply(usb_midi, Command::GetMidiDirEntry, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleGetSf2DirCount(const daisy::MidiEvent& msg,
                                              daisy::MidiUsbHandler& usb_midi)
{
    char dir_path[MediaLibrary::kPathMax]{};
    if(!ReadPathArg(msg, 4, dir_path, sizeof(dir_path)))
    {
        SendReply(usb_midi, Command::GetSf2DirCount, Status::Invalid);
        return;
    }

    const uint16_t count
        = static_cast<uint16_t>(library_->SoundFontDirEntryCount(dir_path));
    uint8_t payload[2]{};
    const size_t pos = Write14(payload, count);
    SendReply(usb_midi, Command::GetSf2DirCount, Status::Ok, payload, pos);
}

void SysExRemoteControl::HandleGetSf2DirEntry(const daisy::MidiEvent& msg,
                                              daisy::MidiUsbHandler& usb_midi)
{
    char dir_path[MediaLibrary::kPathMax]{};
    if(!ReadPathArg(msg, 4, dir_path, sizeof(dir_path)))
    {
        SendReply(usb_midi, Command::GetSf2DirEntry, Status::Invalid);
        return;
    }

    const size_t index_offset = 5 + std::strlen(dir_path);
    if(msg.sysex_message_len < index_offset + 2)
    {
        SendReply(usb_midi, Command::GetSf2DirEntry, Status::Invalid);
        return;
    }
    const uint16_t index = Read14(msg.sysex_data + index_offset);

    bool is_dir = false;
    char name[MediaLibrary::kNameMax]{};
    if(!library_->SoundFontDirEntryAt(dir_path, index, is_dir, name, sizeof(name)))
    {
        SendReply(usb_midi, Command::GetSf2DirEntry, Status::Range);
        return;
    }

    const size_t name_len = std::strlen(name);
    uint8_t      payload[2 + 1 + 1 + MediaLibrary::kNameMax]{};
    size_t       pos = 0;
    pos += Write14(payload + pos, index);
    payload[pos++] = is_dir ? 1 : 0;
    payload[pos++] = static_cast<uint8_t>(name_len & 0x7F);
    std::memcpy(payload + pos, name, name_len);
    pos += name_len;
    SendReply(usb_midi, Command::GetSf2DirEntry, Status::Ok, payload, pos);
}

void SysExRemoteControl::SendReply(daisy::MidiUsbHandler& usb_midi,
                                   Command               command,
                                   Status                status,
                                   const uint8_t*        payload,
                                   size_t                payload_size)
{
    uint8_t reply[kReplyMax]{};
    size_t  pos = 0;
    reply[pos++] = 0xF0;
    reply[pos++] = kManufacturerId;
    reply[pos++] = kMagic0;
    reply[pos++] = kMagic1;
    reply[pos++] = static_cast<uint8_t>(command);
    reply[pos++] = static_cast<uint8_t>(status);
    for(size_t i = 0; i < payload_size && pos < (kReplyMax - 1); i++)
        reply[pos++] = static_cast<uint8_t>(payload[i] & 0x7F);
    reply[pos++] = 0xF7;
    usb_midi.SendMessage(reply, pos);
}

uint16_t SysExRemoteControl::Read14(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 7));
}

uint32_t SysExRemoteControl::Read28(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0])
           | (static_cast<uint32_t>(data[1]) << 7)
           | (static_cast<uint32_t>(data[2]) << 14)
           | (static_cast<uint32_t>(data[3]) << 21);
}

size_t SysExRemoteControl::Write14(uint8_t* out, uint16_t value)
{
    out[0] = static_cast<uint8_t>(value & 0x7F);
    out[1] = static_cast<uint8_t>((value >> 7) & 0x7F);
    return 2;
}

size_t SysExRemoteControl::Write28(uint8_t* out, uint32_t value)
{
    out[0] = static_cast<uint8_t>(value & 0x7F);
    out[1] = static_cast<uint8_t>((value >> 7) & 0x7F);
    out[2] = static_cast<uint8_t>((value >> 14) & 0x7F);
    out[3] = static_cast<uint8_t>((value >> 21) & 0x7F);
    return 4;
}

} // namespace major_midi
