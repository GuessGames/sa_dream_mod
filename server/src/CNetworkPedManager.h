#pragma once

#include <vector>

class CNetworkPlayer;
class CNetworkPed;

class CNetworkPedManager
{
public:
    static std::vector<CNetworkPed*> m_pPeds;
    static void Add(CNetworkPed* ped);
    static void Remove(CNetworkPed* ped);
    static CNetworkPed* GetPed(int pedid);
    static int GetFreeId();
    static void RemoveAllHostedAndNotify(CNetworkPlayer* player);
    // explicit ownership change: old syncer gets syncing=false, new one syncing=true
    static void AssignSyncer(CNetworkPed* ped, CNetworkPlayer* newSyncer, bool notifyOld = true);
    // moves every ped hosted by `from` to the nearest other active player; removes them if there is nobody
    static void MigrateAllHosted(CNetworkPlayer* from, bool removeIfNobody);
};