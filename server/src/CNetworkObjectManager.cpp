#include "stdafx.h"

std::vector<CNetworkObject*> CNetworkObjectManager::m_pObjects;

void CNetworkObjectManager::Add(CNetworkObject* object) { m_pObjects.push_back(object); }

void CNetworkObjectManager::Remove(CNetworkObject* object)
{
    auto it = std::find(m_pObjects.begin(), m_pObjects.end(), object);
    if (it != m_pObjects.end())
    {
        m_pObjects.erase(it);
    }
}

CNetworkObject* CNetworkObjectManager::GetObject(int objectid)
{
    if (objectid < 0)
        return nullptr;

    for (auto* object : m_pObjects)
    {
        if (object->m_nObjectId == objectid)
        {
            return object;
        }
    }
    return nullptr;
}

void CNetworkObjectManager::RemoveAllHostedAndNotify(CNetworkPlayer* player)
{
    int removed = 0;
    for (auto it = m_pObjects.begin(); it != m_pObjects.end();)
    {
        if ((*it)->m_pSyncer == player)
        {
            Packets::Objects::ObjectRemove packet{};
            packet.objectid = (*it)->m_nObjectId;
            GetPacketFactory().SendToAll(packet, player);

            delete *it;
            it = m_pObjects.erase(it);
            removed++;
        }
        else
        {
            ++it;
        }
    }

    if (removed)
    {
        logger::info("[obj] removed %d objects of %s", removed, player->GetName().c_str());
    }
}

void CNetworkObjectManager::SendAllTo(CNetworkPlayer* player)
{
    for (auto* object : m_pObjects)
    {
        Packets::Objects::ObjectSpawn packet{};
        packet.objectid = object->m_nObjectId;
        packet.modelid = object->m_nModelId;
        packet.pos = object->m_vecPosition;
        packet.right = object->m_vecRight;
        packet.up = object->m_vecUp;
        packet.areaCode = object->m_nAreaCode;
        packet.damageState = object->m_damageState;
        GetPacketFactory().Send(packet, player);

        if (object->m_bHasUpdate)
        {
            GetPacketFactory().Send(object->m_lastUpdate, player);
        }
    }
}
