#pragma once

#include <unordered_map>

// Syncs pickups dropped in the world (money and weapons of dead peds).
// Only the owner of a ped creates its drops and tells the others; whoever collects a pickup removes it everywhere.
class CPickupSync
{
public:
    static void InjectHooks();

    static void OnPickupCreate(uint32_t netId, uint16_t modelId, uint8_t pickupType, uint32_t ammo,
        uint16_t moneyPerDay, const CVector& pos);
    static void OnPickupRemove(uint32_t netId);

    // network id -> local pickup handle (refIndex << 16 | slot)
    static inline std::unordered_map<uint32_t, int> ms_netToHandle;
    static inline uint32_t ms_nextNetId = 1;
};
