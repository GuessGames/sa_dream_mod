#include "CServerTime.h"
#include "network/packet.h"
#include "network/packet_types.h"
#include "network/packets/system.h"
#include "stdafx.h"
#include "CPacketBuffer.h"

void CPacketBuffer::Receive(Packet* pPacket)
{
    if (pPacket->GetChannel() == ePacketChannel::SYSTEM)
    {
        GetPacketHandler().ProcessPacket(pPacket);
        delete pPacket;
        return;
    }

    if (pPacket->GetChannel() != ePacketChannel::SYNC)
    {
        m_events.push_back({pPacket, GetTickCount()});
        return;
    }

    if (m_packets.empty() || pPacket->serverTime >= m_packets.back()->serverTime)
    {
        m_packets.push_back(pPacket);
        return;
    }

    auto it = std::upper_bound(m_packets.begin(), m_packets.end(), pPacket->serverTime,
        [](server_time_t time, const Packet* snapshot) { return time < snapshot->serverTime; });

    m_packets.insert(it, pPacket);
}

void CPacketBuffer::ProcessOne(Packet* pPacket, uint32_t renderTime)
{
    GetPacketHandler().ProcessPacket(pPacket);

    SPacketRecord packetRecord{};
    packetRecord.m_nSize = pPacket->GetBytesRead();
    packetRecord.m_bInbound = true;
    packetRecord.m_bOutbound = false;
    packetRecord.m_packetType = pPacket->GetType();
    packetRecord.m_serverTime = pPacket->serverTime;
    packetRecord.m_sContent = pPacket->ToString();
    GetPacketFactory().AddPacketRecord(packetRecord, renderTime);

    delete pPacket;
}

void CPacketBuffer::Process()
{
    uint32_t renderTime = GetRenderTime(g_serverTime);
    uint32_t now = GetTickCount();

    // merge both queues by time; events keep their relative (arrival) order
    while (true)
    {
        bool eventDue = !m_events.empty() && (m_events.front().packet->serverTime <= renderTime ||
                                                 now - m_events.front().arrivedAt >= MAX_EVENT_HOLD_MS);
        bool syncDue = !m_packets.empty() && m_packets.front()->serverTime <= renderTime;

        if (!eventDue && !syncDue)
            break;

        if (eventDue && (!syncDue || m_events.front().packet->serverTime <= m_packets.front()->serverTime))
        {
            Packet* pPacket = m_events.front().packet;
            m_events.pop_front();
            ProcessOne(pPacket, renderTime);
        }
        else
        {
            Packet* pPacket = m_packets.front();
            m_packets.pop_front();
            ProcessOne(pPacket, renderTime);
        }
    }
}
