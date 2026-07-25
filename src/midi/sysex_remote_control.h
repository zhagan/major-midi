#pragma once

#include "app_state.h"
#include "hid/midi.h"
#include "media_library.h"

namespace major_midi
{

class SysExRemoteControl
{
  public:
    using SyncLoopStateFn = void (*)(void* context);

    void Init(AppState* state,
              MediaLibrary* library,
              SyncLoopStateFn sync_loop_state,
              void* context);
    bool HandleUsbMidiEvent(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);

  private:
    static constexpr uint8_t kManufacturerId = 0x7D;
    static constexpr uint8_t kMagic0         = 'M';
    static constexpr uint8_t kMagic1         = 'M';
    static constexpr size_t  kReplyMax       = 120;

    enum class Command : uint8_t
    {
        GetStatus           = 0x10,
        GetSelectedMidiPath = 0x11,
        GetSelectedSf2Path  = 0x12,
        LoadMidi            = 0x13,
        LoadSf2             = 0x14,
        Transport           = 0x15,
        GetChannelState     = 0x16,
        SetChannelState     = 0x17,
        GetSongState        = 0x18,
        SetSongState        = 0x19,
        SaveSongSettings    = 0x1A,
        GetMidiDirCount     = 0x1B,
        GetMidiDirEntry     = 0x1C,
        GetSf2DirCount      = 0x1D,
        GetSf2DirEntry      = 0x1E,
    };

    enum class Status : uint8_t
    {
        Ok = 0x00,
        Invalid = 0x01,
        Range = 0x02,
    };

    bool IsRemoteMessage(const daisy::MidiEvent& msg) const;
    void HandleGetStatus(daisy::MidiUsbHandler& usb_midi);
    void HandleGetSelectedMidiPath(daisy::MidiUsbHandler& usb_midi);
    void HandleGetSelectedSf2Path(daisy::MidiUsbHandler& usb_midi);
    void HandleLoadMidi(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleLoadSf2(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleTransport(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleGetChannelState(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleSetChannelState(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleGetSongState(daisy::MidiUsbHandler& usb_midi);
    void HandleSetSongState(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleSaveSongSettings(daisy::MidiUsbHandler& usb_midi);
    void HandleGetMidiDirCount(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleGetMidiDirEntry(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleGetSf2DirCount(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleGetSf2DirEntry(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);

    static bool ReadPathArg(const daisy::MidiEvent& msg,
                            size_t                  offset,
                            char*                   out_path,
                            size_t                  out_sz);

    void SendReply(daisy::MidiUsbHandler& usb_midi,
                   Command               command,
                   Status                status,
                   const uint8_t*        payload = nullptr,
                   size_t                payload_size = 0);
    static uint16_t Read14(const uint8_t* data);
    static uint32_t Read28(const uint8_t* data);
    static size_t   Write14(uint8_t* out, uint16_t value);
    static size_t   Write28(uint8_t* out, uint32_t value);

    AppState*       state_             = nullptr;
    MediaLibrary*   library_           = nullptr;
    SyncLoopStateFn sync_loop_state_   = nullptr;
    void*           sync_loop_context_ = nullptr;
};

} // namespace major_midi
