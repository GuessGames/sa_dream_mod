#include "stdafx.h"
#include "CPickupSync.h"
#include <CPickups.h>
#include <network/packets/world.h>

static constexpr int NUM_PICKUPS = 620;  // CPickups::aPickUps

static int GetHandle(int slot)
{
    return (CPickups::aPickUps[slot].m_nReferenceIndex << 16) | slot;
}

static bool IsHandleAlive(int handle)
{
    int slot = handle & 0xFFFF;
    if (slot >= NUM_PICKUPS)
        return false;

    CPickup& pickup = CPickups::aPickUps[slot];
    return pickup.m_nPickupType != PICKUP_NONE && pickup.m_nReferenceIndex == (handle >> 16);
}

static void ForgetExpired()
{
    for (auto it = CPickupSync::ms_netToHandle.begin(); it != CPickupSync::ms_netToHandle.end();)
    {
        if (!IsHandleAlive(it->second))
            it = CPickupSync::ms_netToHandle.erase(it);
        else
            ++it;
    }
}

// the drops of a ped are created by whoever simulates that ped, everyone else gets them over the network
static bool IsDropOwner(CPed* ped)
{
    if (!CNetwork::m_bAuthenticated || ped == FindPlayerPed(0))
        return true;

    if (CNetworkPlayerManager::GetPlayer(ped))
        return false;  // a remote player: his own game creates the drops

    if (auto networkPed = CNetworkPedManager::GetPed(ped))
        return networkPed->m_bSyncing;

    return true;  // a local-only ped
}

// calls the original drop function and broadcasts every pickup it created
template <uintptr_t address>
static void __fastcall CPed__CreateDeadPedDrops_Hook(CPed* This, SKIP_EDX)
{
    if (!IsDropOwner(This))
        return;

    if (!CNetwork::m_bAuthenticated)
    {
        plugin::CallMethod<address, CPed*>(This);
        return;
    }

    static uint16_t refs[NUM_PICKUPS];
    static uint8_t types[NUM_PICKUPS];
    for (int i = 0; i < NUM_PICKUPS; i++)
    {
        refs[i] = CPickups::aPickUps[i].m_nReferenceIndex;
        types[i] = CPickups::aPickUps[i].m_nPickupType;
    }

    plugin::CallMethod<address, CPed*>(This);

    ForgetExpired();
    for (int i = 0; i < NUM_PICKUPS; i++)
    {
        CPickup& pickup = CPickups::aPickUps[i];
        if (pickup.m_nPickupType == PICKUP_NONE)
            continue;
        if (types[i] != PICKUP_NONE && refs[i] == pickup.m_nReferenceIndex)
            continue;  // existed before

        Packets::World::PickupCreate packet{};
        packet.netId = CPickupSync::ms_nextNetId++ & 0xFFFFFF;  // the server puts our player id in the top byte
        packet.modelId = pickup.m_nModelIndex;
        packet.pickupType = pickup.m_nPickupType;
        packet.ammo = pickup.m_nAmmo;
        packet.moneyPerDay = pickup.m_nMoneyPerDay;
        packet.pos = pickup.GetPosn();
        GetPacketFactory().Send(packet);

        // the server rewrites the top byte, remember the id the same way
        uint32_t netId = (static_cast<uint32_t>(CNetworkPlayerManager::m_nMyId) << 24) | packet.netId;
        CPickupSync::ms_netToHandle[netId] = GetHandle(i);
        logger::info("[pickup] dropped net=%08X model=%d type=%d ammo=%u", netId, packet.modelId, packet.pickupType,
            packet.ammo);
    }
}

// CPickup::Update returns true when the local player collected the pickup
static bool __fastcall CPickup__Update_Hook(CPickup* This, SKIP_EDX, CPlayerPed* playerPed, CVehicle* vehicle, int playerId)
{
    int slot = static_cast<int>(This - CPickups::aPickUps);
    int handle = GetHandle(slot);

    bool collected = plugin::CallMethodAndReturn<bool, 0x457410, CPickup*, CPlayerPed*, CVehicle*, int>(
        This, playerPed, vehicle, playerId);

    if (collected && CNetwork::m_bAuthenticated)
    {
        for (auto it = CPickupSync::ms_netToHandle.begin(); it != CPickupSync::ms_netToHandle.end(); ++it)
        {
            if (it->second == handle)
            {
                Packets::World::PickupRemove packet{};
                packet.netId = it->first;
                GetPacketFactory().Send(packet);
                logger::info("[pickup] collected net=%08X", it->first);
                CPickupSync::ms_netToHandle.erase(it);
                break;
            }
        }
    }
    return collected;
}

void CPickupSync::OnPickupCreate(
    uint32_t netId, uint16_t modelId, uint8_t pickupType, uint32_t ammo, uint16_t moneyPerDay, const CVector& pos)
{
    ForgetExpired();
    int handle = CPickups::GenerateNewOne(pos, modelId, pickupType, ammo, moneyPerDay, false, nullptr);
    if (handle < 0)
    {
        logger::warn("[pickup] CREATE net=%08X failed (no free pickup slot)", netId);
        return;
    }
    ms_netToHandle[netId] = handle;
    logger::info("[pickup] CREATE net=%08X model=%d type=%d ammo=%u", netId, modelId, pickupType, ammo);
}

void CPickupSync::OnPickupRemove(uint32_t netId)
{
    auto it = ms_netToHandle.find(netId);
    if (it == ms_netToHandle.end())
    {
        logger::info("[pickup] REMOVE net=%08X (unknown locally)", netId);
        return;
    }

    if (IsHandleAlive(it->second))
        CPickups::RemovePickUp(it->second);

    logger::info("[pickup] REMOVE net=%08X", netId);
    ms_netToHandle.erase(it);
}

void CPickupSync::InjectHooks()
{
    // CPed::CreateDeadPedWeaponPickups / CPed::CreateDeadPedMoney, called when a ped dies
    patch::RedirectCall(0x630755, CPed__CreateDeadPedDrops_Hook<0x4591D0>);
    patch::RedirectCall(0x63075C, CPed__CreateDeadPedDrops_Hook<0x4590F0>);

    // CPickups::Update -> CPickup::Update (player 1 and player 2)
    patch::RedirectCall(0x45902E, CPickup__Update_Hook);
    patch::RedirectCall(0x459095, CPickup__Update_Hook);
}
