#pragma once

class CNetworkObjectManager
{
public:
    static std::vector<CNetworkObject*> m_pObjects;
    static void Add(CNetworkObject* object);
    static void Remove(CNetworkObject* object);
    static CNetworkObject* GetObject(int objectid);
    // mission objects belong to the creator's mission script: nobody else would clean them up, so they go with him
    static void RemoveAllHostedAndNotify(CNetworkPlayer* player);
    // the full state of every object for a player that just connected
    static void SendAllTo(CNetworkPlayer* player);
};
