#include "stdafx.h"
#include "CNetworkObject.h"
#include "CNetworkObjectManager.h"
#include "COpCodeSync.h"

CNetworkObject* CNetworkObjectManager::GetObject(int objectid)
{
    if (objectid < 0)
        return nullptr;

    for (auto* networkObject : m_pObjects)
    {
        if (networkObject->m_nObjectId == objectid)
        {
            return networkObject;
        }
    }
    return nullptr;
}

CNetworkObject* CNetworkObjectManager::GetObject(CEntity* object)
{
    if (object == nullptr)
        return nullptr;

    for (auto* networkObject : m_pObjects)
    {
        if (networkObject->m_pObject == object)
        {
            return networkObject;
        }
    }
    return nullptr;
}

void CNetworkObjectManager::Add(CNetworkObject* object) { m_pObjects.push_back(object); }

void CNetworkObjectManager::Remove(CNetworkObject* object)
{
    auto it = std::find(m_pObjects.begin(), m_pObjects.end(), object);
    if (it != m_pObjects.end())
    {
        m_pObjects.erase(it);
    }
}

int CNetworkObjectManager::AllocateObjectId()
{
    // skip ids still in use after the 16 bit counter wrapped
    for (int i = 0; i < 0x10000; i++)
    {
        int counter = ms_nNextObjectCounter;
        ms_nNextObjectCounter = (ms_nNextObjectCounter + 1) & 0xFFFF;
        int objectid = Packets::Objects::MakeObjectId(CNetworkPlayerManager::m_nMyId, counter);
        if (GetObject(objectid) == nullptr)
        {
            return objectid;
        }
    }
    return Packets::Objects::OBJECT_ID_INVALID;
}

void CNetworkObjectManager::ResetIdAllocator() { ms_nNextObjectCounter = 1; }

bool CNetworkObjectManager::ShouldHost(CObject* object)
{
    if (!CNetwork::m_bAuthenticated || !CLocalPlayer::m_bIsHost || !COpCodeSync::ms_bSyncingEnabled ||
        object->m_nObjectType != OBJECT_MISSION || GetObject(object) != nullptr)
    {
        return false;
    }

    // the same scripts as the opcode sync: adapted missions and the generic sync of the others
    CRunningScript* script = COpCodeSync::GetCurrentScript();
    return script != nullptr && script->m_bIsMission &&
           (COpCodeSync::IsScriptAdapted(script) || COpCodeSync::IsGenericMissionScript(script));
}

// a hosted object is gone when the game deleted it (the reference is nulled). Not in the CWorld::Remove hook:
// the game also removes and re-adds objects to move them, that must not recreate them for everyone
void CNetworkObjectManager::RemoveHostedUnused()
{
    for (auto it = m_pObjects.begin(); it != m_pObjects.end();)
    {
        if ((*it)->m_bSyncing && (*it)->m_pObject == nullptr)
        {
            logger::info("[obj] hosted object id=%d removed locally, notifying others", (*it)->m_nObjectId);
            delete *it;
            it = m_pObjects.erase(it);
            continue;
        }
        ++it;
    }
}

void CNetworkObjectManager::Update()
{
    RemoveHostedUnused();

    uint32_t currentTime = GetTickCount();

    for (auto* pNetworkObject : m_pObjects)
    {
        CObject* pObject = pNetworkObject->m_pObject;
        if (pObject == nullptr || !pNetworkObject->m_bSyncing || pObject->m_matrix == nullptr)
            continue;

        pNetworkObject->SendStateIfChanged();

        if (!pNetworkObject->HasChangedSinceLastUpdate() &&
            currentTime - pNetworkObject->m_nLastUpdateTime < OBJECT_PERIODIC_UPDATE_INTERVAL)
            continue;

        Packets::Objects::ObjectUpdate packet{};
        packet.objectid = pNetworkObject->m_nObjectId;
        CNetworkObject::GetAttachState(pObject, packet.attach);
        packet.pos = pObject->m_matrix->pos;
        packet.right = pObject->m_matrix->right;
        packet.up = pObject->m_matrix->up;
        packet.moveSpeed = pObject->m_vecMoveSpeed;
        packet.turnSpeed = pObject->m_vecTurnSpeed;
        GetPacketFactory().Send(packet);

        pNetworkObject->StoreLastUpdate();
        pNetworkObject->m_nLastUpdateTime = currentTime;
    }
}
