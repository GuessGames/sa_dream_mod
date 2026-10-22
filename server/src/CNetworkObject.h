#pragma once

#include "network/packets/objects.h"

class CNetworkPlayer;
class CNetworkObject
{
public:
    CNetworkObject(const Packets::Objects::ObjectSpawn& packet);

    int m_nObjectId;
    CNetworkPlayer* m_pSyncer = nullptr;
    unsigned short m_nModelId;
    CVector m_vecPosition;
    CVector m_vecRight;
    CVector m_vecUp;
    uint8_t m_nAreaCode;
    Packets::Objects::SObjectDamageState m_damageState;

    // the latest movement/attachment, replayed to late joiners
    Packets::Objects::ObjectUpdate m_lastUpdate{};
    bool m_bHasUpdate = false;
};
