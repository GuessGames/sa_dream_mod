#include "network/packet_types.h"
#include "stdafx.h"
#include "network/packet_handler.h"
#include "network/packets/world.h"

PACKET_HANDLER(
    ePacketType::GAME_WEATHER_TIME, Packets::World::GameWeatherTime* pGameWeatherTime, CNetworkPlayer* pNetworkPlayer)
{
	if (!pNetworkPlayer->m_bIsHost)
	{
		return;
	}

	GetPacketFactory().SendToAll(*pGameWeatherTime, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::ADD_EXPLOSION, Packets::World::AddExplosion* pAddExplosion, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pAddExplosion, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::TAG_UPDATE, Packets::World::TagUpdate* pTagUpdate, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pTagUpdate, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::UPDATE_ALL_TAGS, Packets::World::UpdateAllTags* pUpdateAllTags, CNetworkPlayer* pNetworkPlayer)
{
	if (pNetworkPlayer->m_bIsHost)
	{
		GetPacketFactory().SendToAll(*pUpdateAllTags, pNetworkPlayer);
	}
}

PACKET_HANDLER(ePacketType::UPDATE_MOON_SIZE, Packets::World::UpdateMoonSize* pUpdateMoonSize, CNetworkPlayer* pNetworkPlayer)
{
	GetPacketFactory().SendToAll(*pUpdateMoonSize, pNetworkPlayer);
}
PACKET_HANDLER(ePacketType::PICKUP_CREATE, Packets::World::PickupCreate* pPickupCreate, CNetworkPlayer* pNetworkPlayer)
{
    // the creator's player id lives in the top byte, so ids of different players never collide
    pPickupCreate->netId = (static_cast<uint32_t>(pNetworkPlayer->m_iPlayerId) << 24) | (pPickupCreate->netId & 0xFFFFFF);
    logger::info("[pickup] CREATE net=%08X model=%d type=%d by %s", pPickupCreate->netId, pPickupCreate->modelId,
        pPickupCreate->pickupType, pNetworkPlayer->GetName().c_str());
    GetPacketFactory().SendToAll(*pPickupCreate, pNetworkPlayer);
}

PACKET_HANDLER(ePacketType::PICKUP_REMOVE, Packets::World::PickupRemove* pPickupRemove, CNetworkPlayer* pNetworkPlayer)
{
    logger::info("[pickup] REMOVE net=%08X by %s", pPickupRemove->netId, pNetworkPlayer->GetName().c_str());
    GetPacketFactory().SendToAll(*pPickupRemove, pNetworkPlayer);
}
