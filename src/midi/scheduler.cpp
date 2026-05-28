#include "scheduler.h"

#include "util/scopedirqblocker.h"

using namespace daisy;

namespace major_midi
{

bool BuildRawMidiFromIncomingEvent(const daisy::MidiEvent& msg,
                                   uint8_t                 out[3],
                                   size_t&                 size,
                                   MidiOutputKind&         kind)
{
    switch(msg.type)
    {
        case daisy::MidiMessageType::NoteOn:
            out[0] = static_cast<uint8_t>(0x90 | (msg.channel & 0x0F));
            out[1] = msg.data[0];
            out[2] = msg.data[1];
            size   = 3;
            kind   = MidiOutputKind::Notes;
            return true;

        case daisy::MidiMessageType::NoteOff:
            out[0] = static_cast<uint8_t>(0x80 | (msg.channel & 0x0F));
            out[1] = msg.data[0];
            out[2] = msg.data[1];
            size   = 3;
            kind   = MidiOutputKind::Notes;
            return true;

        case daisy::MidiMessageType::ControlChange:
            out[0] = static_cast<uint8_t>(0xB0 | (msg.channel & 0x0F));
            out[1] = msg.data[0];
            out[2] = msg.data[1];
            size   = 3;
            kind   = MidiOutputKind::Ccs;
            return true;

        case daisy::MidiMessageType::ProgramChange:
            out[0] = static_cast<uint8_t>(0xC0 | (msg.channel & 0x0F));
            out[1] = msg.data[0];
            size   = 2;
            kind   = MidiOutputKind::Programs;
            return true;

        case daisy::MidiMessageType::ChannelMode:
            if(msg.cm_type == daisy::ChannelModeType::AllNotesOff
               || msg.cm_type == daisy::ChannelModeType::AllSoundOff)
            {
                out[0] = static_cast<uint8_t>(0xB0 | (msg.channel & 0x0F));
                out[1] = msg.cm_type == daisy::ChannelModeType::AllSoundOff ? 120
                                                                            : 123;
                out[2] = 0;
                size   = 3;
                kind   = MidiOutputKind::Ccs;
                return true;
            }
            break;

        case daisy::MidiMessageType::SystemRealTime:
            switch(msg.srt_type)
            {
                case daisy::SystemRealTimeType::TimingClock:
                    out[0] = 0xF8;
                    size   = 1;
                    kind   = MidiOutputKind::Clock;
                    return true;
                case daisy::SystemRealTimeType::Start:
                    out[0] = 0xFA;
                    size   = 1;
                    kind   = MidiOutputKind::Transport;
                    return true;
                case daisy::SystemRealTimeType::Continue:
                    out[0] = 0xFB;
                    size   = 1;
                    kind   = MidiOutputKind::Transport;
                    return true;
                case daisy::SystemRealTimeType::Stop:
                    out[0] = 0xFC;
                    size   = 1;
                    kind   = MidiOutputKind::Transport;
                    return true;
                default: break;
            }
            break;

        default: break;
    }

    return false;
}

bool BuildRawMidiFromScheduledEvent(const MidiEv&   ev,
                                    uint8_t         out[3],
                                    size_t&         size,
                                    MidiOutputKind& kind)
{
    switch(ev.type)
    {
        case EvType::NoteOn:
            out[0] = static_cast<uint8_t>(0x90 | (ev.ch & 0x0F));
            out[1] = ev.a;
            out[2] = ev.b;
            size   = 3;
            kind   = MidiOutputKind::Notes;
            return true;
        case EvType::NoteOff:
            out[0] = static_cast<uint8_t>(0x80 | (ev.ch & 0x0F));
            out[1] = ev.a;
            out[2] = 0;
            size   = 3;
            kind   = MidiOutputKind::Notes;
            return true;
        case EvType::Program:
            out[0] = static_cast<uint8_t>(0xC0 | (ev.ch & 0x0F));
            out[1] = ev.a;
            size   = 2;
            kind   = MidiOutputKind::Programs;
            return true;
        case EvType::ControlChange:
            out[0] = static_cast<uint8_t>(0xB0 | (ev.ch & 0x0F));
            out[1] = ev.a;
            out[2] = ev.b;
            size   = 3;
            kind   = MidiOutputKind::Ccs;
            return true;
        case EvType::AllSoundOff:
            out[0] = static_cast<uint8_t>(0xB0 | (ev.ch & 0x0F));
            out[1] = 120;
            out[2] = 0;
            size   = 3;
            kind   = MidiOutputKind::Ccs;
            return true;
        case EvType::AllNotesOff:
            out[0] = static_cast<uint8_t>(0xB0 | (ev.ch & 0x0F));
            out[1] = 123;
            out[2] = 0;
            size   = 3;
            kind   = MidiOutputKind::Ccs;
            return true;
        case EvType::PitchBend:
        default: break;
    }
    return false;
}

void ScheduledMidiOutputScheduler::Reset()
{
    ScopedIrqBlocker lock;
    head_                = 0;
    tail_                = 0;
    uart_next_tx_sample_ = 0;
}

bool ScheduledMidiOutputScheduler::Empty() const
{
    ScopedIrqBlocker lock;
    return tail_ == head_;
}

void ScheduledMidiOutputScheduler::ForwardScheduledMidiOut(
    const MidiEv&       ev,
    float               sample_rate,
    MidiMonitorEventFn  monitor_fn,
    MidiBlockEventFn    block_fn,
    MidiEvToRawBytesFn  to_raw_fn,
    MidiPreparePacketFn prepare_fn,
    void*               context)
{
    if(monitor_fn != nullptr)
        monitor_fn(ev, context);
    if(block_fn != nullptr && block_fn(ev, context))
        return;
    if(to_raw_fn == nullptr || prepare_fn == nullptr)
        return;

    ScheduledMidiOutPacket packet{};
    packet.at_sample = ev.atSample;
    packet.type      = ev.type;
    packet.ch        = ev.ch;
    packet.a         = ev.a;
    packet.b         = ev.b;

    size_t         size = 0;
    MidiOutputKind kind = MidiOutputKind::Notes;
    if(!to_raw_fn(ev, packet.bytes, size, kind, context))
        return;

    uint8_t routed[3]{};
    size_t  routed_size = 0;
    packet.send_usb = prepare_fn(
        true, kind, packet.bytes, size, routed, routed_size, context);
    packet.send_uart = prepare_fn(
        false, kind, packet.bytes, size, routed, routed_size, context);
    if(!packet.send_usb && !packet.send_uart)
        return;

    packet.size = static_cast<uint8_t>(size);
    if(packet.send_uart)
    {
        const uint64_t event_sample = ev.atSample;
        if(uart_next_tx_sample_ < event_sample)
            uart_next_tx_sample_ = event_sample;
        packet.uart_due_sample = uart_next_tx_sample_;
        uart_next_tx_sample_ += MidiUartWireSamplesForPacket(sample_rate, packet.size);
    }

    Enqueue(packet);
}

void ScheduledMidiOutputScheduler::FlushScheduledMidiOut(
    uint64_t            now_sample,
    MidiPreparePacketFn prepare_fn,
    MidiSendPacketFn    send_fn,
    void*               context)
{
    if(prepare_fn == nullptr || send_fn == nullptr)
        return;

    while(true)
    {
        ScheduledMidiOutPacket packet{};
        size_t                 packet_index = 0;
        if(!Peek(packet, packet_index))
            break;

        const bool usb_due  = packet.send_usb && !packet.usb_sent;
        const bool uart_due = packet.send_uart && !packet.uart_sent
                              && now_sample >= packet.uart_due_sample;
        if(!usb_due && !uart_due)
            break;

        uint8_t routed[3]{};
        size_t  routed_size = 0;
        const auto kind     = [&]() {
            switch(packet.type)
            {
                case EvType::ControlChange:
                case EvType::AllSoundOff:
                case EvType::AllNotesOff: return MidiOutputKind::Ccs;
                case EvType::Program: return MidiOutputKind::Programs;
                case EvType::NoteOn:
                case EvType::NoteOff:
                case EvType::PitchBend: return MidiOutputKind::Notes;
            }
            return MidiOutputKind::Notes;
        }();

        if(usb_due
           && prepare_fn(true,
                         kind,
                         packet.bytes,
                         packet.size,
                         routed,
                         routed_size,
                         context))
            send_fn(true, routed, routed_size, context);
        if(uart_due
           && prepare_fn(false,
                         kind,
                         packet.bytes,
                         packet.size,
                         routed,
                         routed_size,
                         context))
            send_fn(false, routed, routed_size, context);

        MarkSent(packet_index, usb_due, uart_due);
    }
}

bool ScheduledMidiOutputScheduler::Enqueue(const ScheduledMidiOutPacket& packet)
{
    ScopedIrqBlocker lock;
    const size_t     next = (head_ + 1) % kQueueSize;
    if(next == tail_)
        return false;
    queue_[head_] = packet;
    head_         = next;
    return true;
}

bool ScheduledMidiOutputScheduler::Peek(ScheduledMidiOutPacket& packet,
                                        size_t&                 index) const
{
    ScopedIrqBlocker lock;
    if(tail_ == head_)
        return false;
    index  = tail_;
    packet = queue_[index];
    return true;
}

void ScheduledMidiOutputScheduler::MarkSent(size_t index,
                                            bool   usb_sent,
                                            bool   uart_sent)
{
    ScopedIrqBlocker lock;
    if(tail_ == head_ || index != tail_)
        return;

    auto& packet = queue_[index];
    if(usb_sent)
        packet.usb_sent = true;
    if(uart_sent)
        packet.uart_sent = true;

    if((!packet.send_usb || packet.usb_sent)
       && (!packet.send_uart || packet.uart_sent))
        tail_ = (tail_ + 1) % kQueueSize;
}

uint32_t ScheduledMidiOutputScheduler::MidiUartWireSamplesForPacket(
    float  sample_rate,
    size_t size) const
{
    if(size == 0)
        return 0;

    const double bytes_on_wire = static_cast<double>(size);
    const double samples
        = (bytes_on_wire * 10.0 * static_cast<double>(sample_rate)) / 31250.0;
    return static_cast<uint32_t>(samples > 1.0 ? samples : 1.0);
}

} // namespace major_midi
