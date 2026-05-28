#pragma once
#include <cstdint>
#include <cstddef>

#include "hid/midi.h"

enum class EvType : uint8_t
{
    NoteOn,
    NoteOff,
    Program,
    ControlChange,
    PitchBend,
    AllSoundOff,
    AllNotesOff,
};

struct MidiEv
{
    uint64_t atSample = 0;
    EvType   type     = EvType::NoteOn;
    uint8_t  ch       = 0;
    uint8_t  a        = 0; // note/program
    uint8_t  b        = 0; // velocity
};

template <size_t N>
class EventQueue
{
  public:
    bool Push(const MidiEv& e)
    {
        const size_t next = (head_ + 1) % N;
        if(next == tail_)
            return false; // full
        buf_[head_] = e;
        head_       = next;
        return true;
    }

    bool Peek(MidiEv& out) const
    {
        if(tail_ == head_)
            return false;
        out = buf_[tail_];
        return true;
    }

    bool Pop(MidiEv& out)
    {
        if(tail_ == head_)
            return false;
        out   = buf_[tail_];
        tail_ = (tail_ + 1) % N;
        return true;
    }

    bool Empty() const { return tail_ == head_; }

    void Clear() { head_ = tail_ = 0; }
    bool IsFull() const { return ((head_ + 1) % N) == tail_; }
    size_t Size() const
    {
        if(head_ >= tail_)
            return head_ - tail_;
        return N - (tail_ - head_);
    }

    template <typename Fn>
    void Transform(Fn&& fn)
    {
        size_t idx = tail_;
        while(idx != head_)
        {
            fn(buf_[idx]);
            idx = (idx + 1) % N;
        }
    }

  private:
    MidiEv buf_[N]{};
    size_t head_ = 0;
    size_t tail_ = 0;
};

namespace major_midi
{

enum class MidiOutputKind : uint8_t
{
    Notes,
    Ccs,
    Programs,
    Transport,
    Clock,
};

struct ScheduledMidiOutPacket
{
    uint64_t at_sample       = 0;
    uint64_t uart_due_sample = 0;
    bool     send_usb        = false;
    bool     send_uart       = false;
    bool     usb_sent        = false;
    bool     uart_sent       = false;
    uint8_t  bytes[3]{};
    uint8_t  size = 0;
    EvType   type = EvType::NoteOn;
    uint8_t  ch   = 0;
    uint8_t  a    = 0;
    uint8_t  b    = 0;
};

using MidiMonitorEventFn
    = void (*)(const MidiEv& ev, void* context);
using MidiBlockEventFn
    = bool (*)(const MidiEv& ev, void* context);
using MidiEvToRawBytesFn = bool (*)(const MidiEv&   ev,
                                    uint8_t         out[3],
                                    size_t&         size,
                                    MidiOutputKind& kind,
                                    void*           context);
using MidiPreparePacketFn = bool (*)(bool           to_usb,
                                     MidiOutputKind kind,
                                     const uint8_t* bytes,
                                     size_t         size,
                                     uint8_t        out[3],
                                     size_t&        out_size,
                                     void*          context);
using MidiSendPacketFn
    = void (*)(bool to_usb, const uint8_t* bytes, size_t size, void* context);

bool BuildRawMidiFromIncomingEvent(const daisy::MidiEvent& msg,
                                   uint8_t                 out[3],
                                   size_t&                 size,
                                   MidiOutputKind&         kind);

bool BuildRawMidiFromScheduledEvent(const MidiEv&         ev,
                                    uint8_t               out[3],
                                    size_t&               size,
                                    MidiOutputKind&       kind);

class ScheduledMidiOutputScheduler
{
  public:
    static constexpr size_t kQueueSize = 64;

    void Reset();
    bool Empty() const;

    void ForwardScheduledMidiOut(const MidiEv&          ev,
                                 float                  sample_rate,
                                 MidiMonitorEventFn     monitor_fn,
                                 MidiBlockEventFn       block_fn,
                                 MidiEvToRawBytesFn     to_raw_fn,
                                 MidiPreparePacketFn    prepare_fn,
                                 void*                  context);

    void FlushScheduledMidiOut(uint64_t            now_sample,
                               MidiPreparePacketFn prepare_fn,
                               MidiSendPacketFn    send_fn,
                               void*               context);

  private:
    bool     Enqueue(const ScheduledMidiOutPacket& packet);
    bool     Peek(ScheduledMidiOutPacket& packet, size_t& index) const;
    void     MarkSent(size_t index, bool usb_sent, bool uart_sent);
    uint32_t MidiUartWireSamplesForPacket(float sample_rate, size_t size) const;

    ScheduledMidiOutPacket queue_[kQueueSize]{};
    size_t                 head_                = 0;
    size_t                 tail_                = 0;
    uint64_t               uart_next_tx_sample_ = 0;
};

} // namespace major_midi
