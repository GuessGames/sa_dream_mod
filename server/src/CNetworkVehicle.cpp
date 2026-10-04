#include "stdafx.h"
#include <network/packets/vehicles.h>

CNetworkVehicle::CNetworkVehicle(int vehicleid, unsigned short model, CVector pos, float rot)
{
    m_nVehicleId = vehicleid;
    m_nModelId = model;
    m_vecPosition = pos;
    m_vecRotation = CVector(0, 0, 0);
}

void CNetworkVehicle::ReassignSyncer(CNetworkPlayer* newSyncer, bool notifyOld)
{
    if (m_pSyncer != newSyncer)
    {
        logger::info("[veh] syncer of id=%d: %s -> %s", m_nVehicleId, m_pSyncer ? m_pSyncer->GetName().c_str() : "none",
            newSyncer->GetName().c_str());
        Packets::Vehicles::AssignVehicleSyncer packet{};
        packet.vehicleid = m_nVehicleId;

        // send to the old vehicle syncer
        if (m_pSyncer && notifyOld)
        {
            packet.syncing = false;
            GetPacketFactory().Send(packet, m_pSyncer);
        }

        // send to the new
        packet.syncing = true;
        GetPacketFactory().Send(packet, newSyncer);

        m_pSyncer = newSyncer;
    }
}

void CNetworkVehicle::SetOccupant(int8_t seatid, CNetworkPlayer* player)
{
    if (seatid < 0 || seatid > 7)
        return;

    if (this->m_pPlayers[seatid])
    {
        this->m_pPlayers[seatid]->m_nVehicleId = -1;
        this->m_pPlayers[seatid]->m_nSeatId = -1;
    }

    this->m_pPlayers[seatid] = player;

    if (player)
    {
        player->m_nVehicleId = this->m_nVehicleId;
        player->m_nSeatId = seatid;
    }
}
