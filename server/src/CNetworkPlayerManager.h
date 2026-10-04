#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>

#include "CNetworkPlayer.h"

class CNetworkPlayerManager
{
public:
    static std::vector<CNetworkPlayer*> m_pPlayers;
    static void Add(CNetworkPlayer* player);
    static void Remove(CNetworkPlayer* player);
    static CNetworkPlayer* GetPlayer(int playerid);
    static CNetworkPlayer* GetPlayer(ENetPeer* peer);
    static int GetFreeID();
    static CNetworkPlayer* GetHost();
    static void AssignHostToFirstPlayer();
    // the best player to take over entities: nearest to `pos`, not paused, not `except`; nullptr if nobody
    // allowPaused: fall back to a paused player (better than deleting the entity, he simulates it once he resumes)
    static CNetworkPlayer* PickSyncer(CNetworkPlayer* except, const CVector& pos, bool allowPaused = false);
};