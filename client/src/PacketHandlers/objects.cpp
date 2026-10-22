#include "network/packets/objects.h"
#include "network/packet_types.h"
#include "stdafx.h"

PACKET_HANDLER(ePacketType::OBJECT_SPAWN, Packets::Objects::ObjectSpawn* pObjectSpawn)
{
    logger::info("[obj] SPAWN id=%d model=%d pos=(%.1f %.1f %.1f)", pObjectSpawn->objectid, pObjectSpawn->modelid,
        pObjectSpawn->pos.x, pObjectSpawn->pos.y, pObjectSpawn->pos.z);

    if (CNetworkObject* pExisting = CNetworkObjectManager::GetObject(pObjectSpawn->objectid))
    {
        if (pExisting->m_bSyncing)
        {
            return;
        }

        CNetworkObjectManager::Remove(pExisting);
        delete pExisting;
    }

    CNetworkObjectManager::Add(new CNetworkObject(*pObjectSpawn));
}

PACKET_HANDLER(ePacketType::OBJECT_REMOVE, Packets::Objects::ObjectRemove* pObjectRemove)
{
    CNetworkObject* pNetworkObject = CNetworkObjectManager::GetObject(pObjectRemove->objectid);
    logger::info("[obj] REMOVE id=%d%s", pObjectRemove->objectid, pNetworkObject ? "" : " (unknown locally)");
    if (pNetworkObject != nullptr && !pNetworkObject->m_bSyncing)
    {
        CNetworkObjectManager::Remove(pNetworkObject);
        delete pNetworkObject;
    }
}

PACKET_HANDLER(ePacketType::OBJECT_UPDATE, Packets::Objects::ObjectUpdate* pObjectUpdate)
{
    CNetworkObject* pNetworkObject = CNetworkObjectManager::GetObject(pObjectUpdate->objectid);
    if (pNetworkObject == nullptr || pNetworkObject->m_bSyncing)
    {
        return;
    }

    CObject* pObject = pNetworkObject->m_pObject;
    if (pObject == nullptr || !pObject->IsVTableValid())
    {
        return;
    }

    if (CNetworkObject::ApplyAttachState(pObject, pObjectUpdate->attach))
    {
        return;
    }

    CWorld::Remove(pObject);
    CNetworkObject::SetMatrix(pObject, pObjectUpdate->pos, pObjectUpdate->right, pObjectUpdate->up);
    CWorld::Add(pObject);

    pObject->m_vecMoveSpeed = pObjectUpdate->moveSpeed;
    pObject->m_vecTurnSpeed = pObjectUpdate->turnSpeed;
}

PACKET_HANDLER(ePacketType::OBJECT_DAMAGE_REQUEST, Packets::Objects::ObjectDamageRequest* pObjectDamageRequest)
{
    CNetworkObject* pNetworkObject = CNetworkObjectManager::GetObject(pObjectDamageRequest->objectid);
    if (pNetworkObject == nullptr || !pNetworkObject->m_bSyncing || pNetworkObject->m_pObject == nullptr)
    {
        return;
    }

    CNetworkPlayer* pNetworkPlayer = CNetworkPlayerManager::GetPlayer(pObjectDamageRequest->senderPlayerid);
    if (pNetworkPlayer == nullptr || pNetworkPlayer->m_pPed == nullptr || !pNetworkPlayer->m_pPed->IsVTableValid())
    {
        return;
    }

    CEntity* pDamager = pNetworkPlayer->m_pPed;
    if (pObjectDamageRequest->bInVehicle && pNetworkPlayer->m_pPed->m_pVehicle != nullptr)
    {
        pDamager = pNetworkPlayer->m_pPed->m_pVehicle;
    }

    CObject* pObject = pNetworkObject->m_pObject;
    CVector colPos = pObjectDamageRequest->bHasColPos ? (CVector)pObjectDamageRequest->colPos : pObject->GetPosition();
    CVector colNormal =
        pObjectDamageRequest->bHasColNormal ? pObjectDamageRequest->colNormal : CVector(0.0f, 0.0f, 1.0f);
    eWeaponType weapon = (eWeaponType)pObjectDamageRequest->weapon;

    Packets::Objects::SObjectDamageState oldState{};
    CNetworkObject::GetDamageState(pObject, oldState);

    // the request comes from a player, so `bOnlyDamagedByPlayer` is satisfied
    CNetworkObject::CallObjectDamage(pObject, pObjectDamageRequest->damage, &colPos, &colNormal, pDamager, weapon, true);

    pNetworkObject->SendDamageIfChanged(oldState, pObjectDamageRequest->damage, &colPos, &colNormal, pDamager, weapon);
}

PACKET_HANDLER(ePacketType::WORLD_OBJECT_DAMAGE, Packets::Objects::WorldObjectDamage* pWorldObjectDamage)
{
    // not streamed in here: nothing to show, the map recreates it intact anyway
    CObject* pObject = CNetworkObject::FindMapObject(pWorldObjectDamage->modelid, pWorldObjectDamage->mapPos);
    if (pObject == nullptr || !pObject->m_bIsVisible || pObject->m_pRwObject == nullptr)
    {
        return;
    }

    CEntity* pDamager = nullptr;
    CNetworkPlayer* pNetworkPlayer = CNetworkPlayerManager::GetPlayer(pWorldObjectDamage->senderPlayerid);
    if (pNetworkPlayer != nullptr && pNetworkPlayer->m_pPed != nullptr && pNetworkPlayer->m_pPed->IsVTableValid())
    {
        pDamager = pNetworkPlayer->m_pPed;
        if (pWorldObjectDamage->bInVehicle && pNetworkPlayer->m_pPed->m_pVehicle != nullptr)
        {
            pDamager = pNetworkPlayer->m_pPed->m_pVehicle;
        }
    }

    CVector colPos = pWorldObjectDamage->bHasColPos ? (CVector)pWorldObjectDamage->colPos : pObject->GetPosition();
    CVector colNormal =
        pWorldObjectDamage->bHasColNormal ? pWorldObjectDamage->colNormal : CVector(0.0f, 0.0f, 1.0f);

    int ref = CPools::GetObjectRef(pObject);
    CNetworkObject::CallObjectDamage(pObject, pWorldObjectDamage->damage, &colPos, &colNormal, pDamager,
        (eWeaponType)pWorldObjectDamage->weapon, true);

    // the replayed hit may roll differently (or be too weak here): take the sender's result
    if (CPools::GetObject(ref) == pObject && !pWorldObjectDamage->state.bHasBeenShattered)
    {
        CNetworkObject::ApplyDamageState(pObject, pWorldObjectDamage->state);
    }
}

PACKET_HANDLER(ePacketType::OBJECT_DAMAGE, Packets::Objects::ObjectDamage* pObjectDamage)
{
    CNetworkObject* pNetworkObject = CNetworkObjectManager::GetObject(pObjectDamage->objectid);
    if (pNetworkObject == nullptr || pNetworkObject->m_bSyncing || pNetworkObject->m_pObject == nullptr)
    {
        return;
    }

    CObject* pObject = pNetworkObject->m_pObject;

    // replay the hit for its effects (glass, fragments), then take the syncer's result
    if (!pObjectDamage->bStateOnly && pObject->m_bIsVisible && pObject->m_pRwObject != nullptr)
    {
        CVector colPos = pObjectDamage->bHasColPos ? (CVector)pObjectDamage->colPos : pObject->GetPosition();
        CVector colNormal = pObjectDamage->bHasColNormal ? pObjectDamage->colNormal : CVector(0.0f, 0.0f, 1.0f);

        CNetworkObject::CallObjectDamage(pObject, pObjectDamage->damage, &colPos, &colNormal,
            pObjectDamage->damager.GetEntity(), (eWeaponType)pObjectDamage->weapon, true);
    }

    if (pNetworkObject->m_pObject != nullptr)
    {
        CNetworkObject::ApplyDamageState(pNetworkObject->m_pObject, pObjectDamage->state);
    }
}
