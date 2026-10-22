#pragma once

#include "network/packet.h"
#include <unordered_map>

// the packets register themselves here (PacketRegistrar); the bots only need the prototypes to read with
class CPacketFactory
{
public:
    void RegisterPacket(Packet* pPacket) { m_map[pPacket->GetType()] = pPacket; }

    std::unordered_map<ePacketType, Packet*> m_map;
};

extern inline CPacketFactory& GetPacketFactory()
{
    static CPacketFactory factory;
    return factory;
}
