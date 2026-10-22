#include "network/packet_types.h"
#include "stdafx.h"
#include "network/packet_handler.h"
#include "network/packets/objects.h"

PACKET_HANDLER(ePacketType::OBJECT_SPAWN, Packets::Objects::ObjectSpawn* pObjectSpawn, CNetworkPlayer* pNetworkPlayer)
{
    if (Packets::Objects::GetObjectIdCreator(pObjectSpawn->objectid) != pNetworkPlayer->m_iPlayerId)
    {
        logger::warn("[obj] SPAWN id=%d from %s rejected: not his id", pObjectSpawn->objectid,
            pNetworkPlayer->GetName().c_str());
        return;
    }

    if (CNetworkObjectManager::GetObject(pObjectSpawn->objectid) != nullptr)
    {
        logger::warn("[obj] SPAWN id=%d from %s rejected: id in use", pObjectSpawn->objectid,
            pNetworkPlayer->GetName().c_str());
        return;
    }

    logger::info("[obj] SPAWN id=%d model=%d by %s pos=(%.1f %.1f %.1f) total=%d", pObjectSpawn->objectid,
        pObjectSpawn->modelid, pNetworkPlayer->GetName().c_str(), pObjectSpawn->pos.x, pObjectSpawn->pos.y,
        pObjectSpawn->pos.z, (int)CNetworkObjectManager::m_pObjects.size() + 1);
    GetPacketFactory().SendToAll(*pObjectSpawn, pNetworkPlayer);

    CNetworkObject* object = new CNetworkObject(*pObjectSpawn);
    object->m_pSyncer = pNetworkPlayer;
    CNetworkObjectManager::Add(object);
}

PACKET_HANDLER(
    ePacketType::OBJECT_REMOVE, Packets::Objects::ObjectRemove* pObjectRemove, CNetworkPlayer* pNetworkPlayer)
{
    if (auto object = CNetworkObjectManager::GetObject(pObjectRemove->objectid))
    {
        if (object->m_pSyncer != pNetworkPlayer)
        {
            logger::warn("[obj] REMOVE id=%d from %s rejected: not the syncer", pObjectRemove->objectid,
                pNetworkPlayer->GetName().c_str());
            return;
        }

        logger::info("[obj] REMOVE id=%d by %s", pObjectRemove->objectid, pNetworkPlayer->GetName().c_str());
        GetPacketFactory().SendToAll(*pObjectRemove, pNetworkPlayer);
        CNetworkObjectManager::Remove(object);
        delete object;
    }
}

PACKET_HANDLER(
    ePacketType::OBJECT_UPDATE, Packets::Objects::ObjectUpdate* pObjectUpdate, CNetworkPlayer* pNetworkPlayer)
{
    if (auto object = CNetworkObjectManager::GetObject(pObjectUpdate->objectid))
    {
        if (object->m_pSyncer != pNetworkPlayer)
            return;

        object->m_vecPosition = pObjectUpdate->pos;
        object->m_vecRight = pObjectUpdate->right;
        object->m_vecUp = pObjectUpdate->up;
        object->m_lastUpdate = *pObjectUpdate;
        object->m_bHasUpdate = true;
        GetPacketFactory().SendToAll(*pObjectUpdate, pNetworkPlayer);
    }
}

PACKET_HANDLER(ePacketType::OBJECT_DAMAGE_REQUEST, Packets::Objects::ObjectDamageRequest* pObjectDamageRequest,
    CNetworkPlayer* pNetworkPlayer)
{
    if (auto object = CNetworkObjectManager::GetObject(pObjectDamageRequest->objectid))
    {
        if (object->m_pSyncer == nullptr || object->m_pSyncer == pNetworkPlayer)
            return;

        // only the syncer applies damage
        pObjectDamageRequest->senderPlayerid.value = pNetworkPlayer->m_iPlayerId;
        GetPacketFactory().Send(*pObjectDamageRequest, object->m_pSyncer);
    }
}

PACKET_HANDLER(ePacketType::WORLD_OBJECT_DAMAGE, Packets::Objects::WorldObjectDamage* pWorldObjectDamage,
    CNetworkPlayer* pNetworkPlayer)
{
    pWorldObjectDamage->senderPlayerid.value = pNetworkPlayer->m_iPlayerId;
    GetPacketFactory().SendToAll(*pWorldObjectDamage, pNetworkPlayer);
}

PACKET_HANDLER(
    ePacketType::OBJECT_DAMAGE, Packets::Objects::ObjectDamage* pObjectDamage, CNetworkPlayer* pNetworkPlayer)
{
    if (auto object = CNetworkObjectManager::GetObject(pObjectDamage->objectid))
    {
        if (object->m_pSyncer != pNetworkPlayer)
            return;

        object->m_damageState = pObjectDamage->state;
        GetPacketFactory().SendToAll(*pObjectDamage, pNetworkPlayer);
    }
}
