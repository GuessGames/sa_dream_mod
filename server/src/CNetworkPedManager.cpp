#include "stdafx.h"
#include <network/packets/peds.h>

std::vector<CNetworkPed*> CNetworkPedManager::m_pPeds;

void CNetworkPedManager::Add(CNetworkPed* ped)
{
    m_pPeds.push_back(ped);
}

void CNetworkPedManager::Remove(CNetworkPed* ped)
{
    auto it = std::find(m_pPeds.begin(), m_pPeds.end(), ped);
    // std::find()
    if (it != m_pPeds.end())
    {
        m_pPeds.erase(it);
    }
}

CNetworkPed* CNetworkPedManager::GetPed(int pedid)
{
    for (int i = 0; i != m_pPeds.size(); i++)
    {
        if (m_pPeds[i]->m_nPedId == pedid)
        {
            return m_pPeds[i];
        }
    }
    return nullptr;
}

int CNetworkPedManager::GetFreeId()
{
    // don't hand out a just freed id again: late packets of the old ped would be applied to the new one
    static int nextId = 0;
    for (int i = 0; i < Config::MAX_SERVER_PEDS; i++)
    {
        int id = (nextId + i) % Config::MAX_SERVER_PEDS;
        if (CNetworkPedManager::GetPed(id) == nullptr)
        {
            nextId = (id + 1) % Config::MAX_SERVER_PEDS;
            return id;
        }
    }

    return -1;
}

void CNetworkPedManager::AssignSyncer(CNetworkPed* ped, CNetworkPlayer* newSyncer, bool notifyOld)
{
    Packets::Peds::AssignPedSyncer packet{};
    packet.pedid = ped->m_nPedId;

    if (notifyOld && ped->m_pSyncer && ped->m_pSyncer != newSyncer)
    {
        packet.syncing = false;
        GetPacketFactory().Send(packet, ped->m_pSyncer);
    }

    if (newSyncer)
    {
        packet.syncing = true;
        GetPacketFactory().Send(packet, newSyncer);
    }

    logger::info("[ped] syncer of id=%d: %s -> %s", ped->m_nPedId,
        ped->m_pSyncer ? ped->m_pSyncer->GetName().c_str() : "none", newSyncer ? newSyncer->GetName().c_str() : "none");
    ped->m_pSyncer = newSyncer;
}

void CNetworkPedManager::MigrateAllHosted(CNetworkPlayer* from, bool removeIfNobody)
{
    for (auto it = m_pPeds.begin(); it != m_pPeds.end();)
    {
        CNetworkPed* ped = *it;
        if (ped->m_pSyncer != from)
        {
            ++it;
            continue;
        }

        if (CNetworkPlayer* to = CNetworkPlayerManager::PickSyncer(from, ped->m_vecPos, removeIfNobody))
        {
            // a disconnected player can't receive packets anymore
            AssignSyncer(ped, to, !removeIfNobody);
            ++it;
        }
        else if (removeIfNobody)
        {
            Packets::Peds::PedRemove packet{};
            packet.pedid = ped->m_nPedId;
            GetPacketFactory().SendToAll(packet, from);
            delete ped;
            it = m_pPeds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CNetworkPedManager::RemoveAllHostedAndNotify(CNetworkPlayer* player)
{
    Packets::Peds::PedRemove packet{};

    for (auto it = CNetworkPedManager::m_pPeds.begin(); it != CNetworkPedManager::m_pPeds.end();)
    {
        if ((*it)->m_pSyncer == player)
        {
            packet.pedid = (*it)->m_nPedId;
            GetPacketFactory().SendToAll(packet, player);
            delete *it;
            it = CNetworkPedManager::m_pPeds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}