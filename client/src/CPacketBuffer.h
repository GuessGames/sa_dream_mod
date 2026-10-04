#pragma once

#include "network/packet.h"
#include <deque>

class CPacketBuffer
{
public:
    CPacketBuffer(uint32_t delay) : m_delay(delay) {}
    ~CPacketBuffer() {}

    void Receive(Packet* pPacket);
    void Process();

    uint32_t GetRenderTime(server_time_t serverTime) { return serverTime - m_delay; }

    // SYNC packets, sorted by server time (interpolation buffer)
    std::deque<Packet*> m_packets;

    // reliable EVENT/SCRIPT packets: kept in arrival order (the order the server relayed them),
    // sorting them by the sender's clock could swap e.g. a REMOVE of one player and a SPAWN of another
    struct SEvent
    {
        Packet* packet;
        uint32_t arrivedAt;
    };
    std::deque<SEvent> m_events;

    // an event never waits longer than this for its timestamp (protects against a sender with a wrong clock)
    static constexpr uint32_t MAX_EVENT_HOLD_MS = 300;

private:
    void ProcessOne(Packet* pPacket, uint32_t renderTime);

    uint32_t m_delay;
};

extern inline CPacketBuffer& GetPacketBuffer()
{
    static CPacketBuffer buffer(Config::INTERP_BUFFER_DELAY_MS);
    return buffer;
}
