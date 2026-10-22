#pragma once

#define OBJECT_PERIODIC_UPDATE_INTERVAL (1000)

class CNetworkObjectManager
{
public:
    static inline std::vector<CNetworkObject*> m_pObjects;

    static inline int ms_nNextObjectCounter = 1;

    static CNetworkObject* GetObject(int objectid);
    static CNetworkObject* GetObject(CEntity* object);
    static void Add(CNetworkObject* object);
    static void Remove(CNetworkObject* object);
    static int AllocateObjectId();
    static void ResetIdAllocator();
    // should the object the game just added to the world be synced (a mission object of the host's synced mission)
    static bool ShouldHost(CObject* object);
    static void RemoveHostedUnused();
    static void Update();
};
