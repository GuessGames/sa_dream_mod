#include "stdafx.h"
#include <network/packets/vehicles.h>

std::vector<CNetworkVehicle*> CNetworkVehicleManager::m_pVehicles;

void CNetworkVehicleManager::Add(CNetworkVehicle* vehicle)
{
    m_pVehicles.push_back(vehicle);
}

void CNetworkVehicleManager::Remove(CNetworkVehicle* vehicle)
{
    auto it = std::find(m_pVehicles.begin(), m_pVehicles.end(), vehicle);
    if (it != m_pVehicles.end())
    {
        m_pVehicles.erase(it);
    }
}

CNetworkVehicle* CNetworkVehicleManager::GetVehicle(int vehicleid)
{
    for (int i = 0; i != m_pVehicles.size(); i++)
    {
        if (m_pVehicles[i]->m_nVehicleId == vehicleid)
        {
            return m_pVehicles[i];
        }
    }
    return nullptr;
}

int CNetworkVehicleManager::GetFreeID()
{
    // don't hand out a just freed id again: late packets of the old vehicle would be applied to the new one
    static int nextId = 0;
    for (int i = 0; i < Config::MAX_SERVER_VEHICLES; i++)
    {
        int id = (nextId + i) % Config::MAX_SERVER_VEHICLES;
        if (CNetworkVehicleManager::GetVehicle(id) == nullptr)
        {
            nextId = (id + 1) % Config::MAX_SERVER_VEHICLES;
            return id;
        }
    }

    return -1;
}

void CNetworkVehicleManager::MigrateAllHosted(CNetworkPlayer* from, bool removeIfNobody)
{
    for (auto it = m_pVehicles.begin(); it != m_pVehicles.end();)
    {
        CNetworkVehicle* vehicle = *it;
        if (vehicle->m_pSyncer != from)
        {
            ++it;
            continue;
        }

        if (CNetworkPlayer* to = CNetworkPlayerManager::PickSyncer(from, vehicle->m_vecPosition, removeIfNobody))
        {
            vehicle->ReassignSyncer(to, !removeIfNobody);
            ++it;
        }
        else if (removeIfNobody)
        {
            Packets::Vehicles::VehicleRemove packet{};
            packet.vehicleid = vehicle->m_nVehicleId;
            GetPacketFactory().SendToAll(packet, from);
            delete vehicle;
            it = m_pVehicles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CNetworkVehicleManager::RemoveAllHostedAndNotify(CNetworkPlayer* player)
{
    for (auto it = CNetworkVehicleManager::m_pVehicles.begin(); it != CNetworkVehicleManager::m_pVehicles.end();)
    {
        if ((*it)->m_pSyncer == player)
        {
            Packets::Vehicles::VehicleRemove packet{};
            packet.vehicleid = (*it)->m_nVehicleId;
            GetPacketFactory().SendToAll(packet, player);

            delete *it;
            it = CNetworkVehicleManager::m_pVehicles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}