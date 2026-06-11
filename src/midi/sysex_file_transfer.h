#pragma once

#include <cstddef>
#include <cstdint>

#include "hid/midi.h"

namespace major_midi
{

class SysExFileTransfer
{
  public:
    using RefreshMediaFn = void (*)(void* context);
    using CompleteFn    = void (*)(bool success, void* context);

    void Init(RefreshMediaFn refresh_media, CompleteFn complete, void* context);

    bool HandleUsbMidiEvent(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);

  private:
    static constexpr uint8_t kManufacturerId = 0x7D;
    static constexpr uint8_t kMagic0         = 'M';
    static constexpr uint8_t kMagic1         = 'M';
    static constexpr size_t  kMaxFilename    = 48;
    static constexpr size_t  kMaxRawChunk    = 63;

    enum class Command : uint8_t
    {
        Start  = 0x01,
        Data   = 0x02,
        End    = 0x03,
        Cancel = 0x04,
        Ack    = 0x7F,
    };

    enum class Status : uint8_t
    {
        Ok           = 0x00,
        Invalid      = 0x01,
        Busy         = 0x02,
        FsError      = 0x03,
        Sequence     = 0x04,
        Filename     = 0x05,
        Decode       = 0x06,
        NotReceiving = 0x07,
    };

    bool IsTransferMessage(const daisy::MidiEvent& msg) const;
    void HandleStart(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleData(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleEnd(const daisy::MidiEvent& msg, daisy::MidiUsbHandler& usb_midi);
    void HandleCancel(daisy::MidiUsbHandler& usb_midi);

    void SendAck(daisy::MidiUsbHandler& usb_midi,
                 Command               request,
                 Status                status,
                 uint16_t              value = 0);
    void AbortTransfer(bool delete_partial);
    bool NormalizeMidiFilename(const uint8_t* src,
                               size_t         src_len,
                               char*          out,
                               size_t         out_sz) const;
    bool DecodePacked7Bit(const uint8_t* src,
                          size_t         src_len,
                          uint8_t*       out,
                          size_t         raw_len) const;

    void*          refresh_media_context_ = nullptr;
    RefreshMediaFn refresh_media_         = nullptr;
    CompleteFn     complete_              = nullptr;
    bool           active_                = false;
    uint16_t       expected_sequence_     = 0;
    char           active_path_[96]{};
};

} // namespace major_midi
