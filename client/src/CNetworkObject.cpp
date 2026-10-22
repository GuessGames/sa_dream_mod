#include "stdafx.h"
#include "CNetworkObject.h"
#include <CObject.h>

CNetworkObject::CNetworkObject(const Packets::Objects::ObjectSpawn& packet)
{
    m_nObjectId = packet.objectid;
    m_bSyncing = false;
    m_nModelId = packet.modelid;
    m_lastSentState = packet.damageState;

    if (!CreateObject(packet))
    {
        logger::warn("[obj] SPAWN id=%d model=%d: could not create the object", packet.objectid, packet.modelid);
    }
}

bool CNetworkObject::CreateObject(const Packets::Objects::ObjectSpawn& packet)
{
    int modelid = packet.modelid;
    if (modelid >= 20000 || CModelInfo::ms_modelInfoPtrs[modelid] == nullptr)
    {
        return false;
    }

    unsigned char oldFlags = CStreaming::ms_aInfoForModel[modelid].m_nFlags;
    CStreaming::RequestModel(modelid, GAME_REQUIRED);
    CStreaming::LoadAllRequestedModels(false);

    if (!(oldFlags & GAME_REQUIRED))
    {
        CStreaming::SetModelIsDeletable(modelid);
        CStreaming::SetModelTxdIsDeletable(modelid);
    }

    if (CStreaming::ms_aInfoForModel[modelid].m_nLoadState != LOADSTATE_LOADED)
    {
        return false;
    }

    CModelInfo::ms_modelInfoPtrs[modelid]->m_nAlpha = 255;

    // CObject::Create(int modelId, bool bUnused)
    m_pObject = plugin::CallAndReturn<CObject*, 0x5A1F60, int, bool>(modelid, false);
    if (m_pObject == nullptr)
    {
        return false;
    }

    m_pObject->RegisterReference((CEntity**)&m_pObject);

    m_pObject->m_nObjectType = OBJECT_MISSION;
    SetMatrix(m_pObject, packet.pos, packet.right, packet.up);

    if (CModelInfo::ms_modelInfoPtrs[modelid]->AsLodAtomicModelInfoPtr() != nullptr)
    {
        m_pObject->SetupBigBuilding();
    }

    m_pObject->m_nAreaCode = packet.areaCode;
    ApplyDamageState(m_pObject, packet.damageState);

    // not a hooked call site, so it doesn't become a hosted object
    CWorld::Add(m_pObject);
    return true;
}

CNetworkObject::~CNetworkObject()
{
    if (m_bMissionBlip && m_nBlipHandle != -1)
    {
        CRadar::ClearBlip(m_nBlipHandle);
    }
    m_nBlipHandle = -1;

    if (m_pObject != nullptr)
    {
        m_pObject->CleanUpOldReference((CEntity**)&m_pObject);
    }

    if (m_bSyncing)
    {
        // the game owns (and already deleted) our hosted object, only tell the others
        Packets::Objects::ObjectRemove packet{};
        packet.objectid = m_nObjectId;
        GetPacketFactory().Send(packet);
    }
    else if (m_pObject != nullptr && m_pObject->IsVTableValid())
    {
        CWorld::Remove(m_pObject);
        CWorld::RemoveReferencesToDeletedObject(m_pObject);
        delete m_pObject;
    }
    m_pObject = nullptr;
}

bool CNetworkObject::HasChangedSinceLastUpdate()
{
    CMatrix* matrix = m_pObject->m_matrix;
    if (matrix == nullptr)
    {
        return false;
    }

    if (m_pObject->m_pAttachedTo != m_pLastAttachedTo)
    {
        return true;
    }

    if (m_pObject->m_pAttachedTo != nullptr)
    {
        const CQuaternion& rotation = m_pObject->m_qAttachedEntityRotation;
        if (DistanceBetweenPoints(m_pObject->m_vecAttachOffset, m_vecLastAttachOffset) > 0.001f ||
            DistanceBetweenPoints(m_pObject->m_vecAttachedEntityPosn, m_vecLastAttachTilt) > 0.001f ||
            DistanceBetweenPoints(rotation.imag, m_lastAttachRotation.imag) > 0.001f ||
            std::abs(rotation.real - m_lastAttachRotation.real) > 0.001f)
        {
            return true;
        }

        // the others position it from its parent themselves
        if (IsAttachedToNetworkEntity(m_pObject))
        {
            return false;
        }
    }

    return DistanceBetweenPoints(matrix->pos, m_vecLastPos) > 0.01f ||
           DistanceBetweenPoints(matrix->right, m_vecLastRight) > 0.001f ||
           DistanceBetweenPoints(matrix->up, m_vecLastUp) > 0.001f ||
           DistanceBetweenPoints(m_pObject->m_vecMoveSpeed, m_vecLastMoveSpeed) > 0.0005f ||
           DistanceBetweenPoints(m_pObject->m_vecTurnSpeed, m_vecLastTurnSpeed) > 0.0005f;
}

void CNetworkObject::StoreLastUpdate()
{
    CMatrix* matrix = m_pObject->m_matrix;
    if (matrix == nullptr)
    {
        return;
    }

    m_vecLastPos = matrix->pos;
    m_vecLastRight = matrix->right;
    m_vecLastUp = matrix->up;
    m_vecLastMoveSpeed = m_pObject->m_vecMoveSpeed;
    m_vecLastTurnSpeed = m_pObject->m_vecTurnSpeed;
    m_pLastAttachedTo = m_pObject->m_pAttachedTo;
    m_vecLastAttachOffset = m_pObject->m_vecAttachOffset;
    m_vecLastAttachTilt = m_pObject->m_vecAttachedEntityPosn;
    m_lastAttachRotation = m_pObject->m_qAttachedEntityRotation;
}

void CNetworkObject::GetAttachState(CObject* object, Packets::Objects::SObjectAttachState& state)
{
    state = Packets::Objects::SObjectAttachState();
    if (object->m_pAttachedTo == nullptr)
    {
        return;
    }

    state.bAttached = true;
    state.attachedTo.SetEntity(object->m_pAttachedTo);
    state.offset = object->m_vecAttachOffset;
    state.tilt = object->m_vecAttachedEntityPosn;
    state.rotationImag = object->m_qAttachedEntityRotation.imag;
    state.rotationReal = object->m_qAttachedEntityRotation.real;
}

bool CNetworkObject::IsAttachedToNetworkEntity(CObject* object)
{
    if (object->m_pAttachedTo == nullptr)
    {
        return false;
    }

    CNetworkEntitySerializer parent{};
    parent.SetEntity(object->m_pAttachedTo);
    return parent.GetEntity() == object->m_pAttachedTo;
}

bool CNetworkObject::ApplyAttachState(CObject* object, const Packets::Objects::SObjectAttachState& state)
{
    CNetworkEntitySerializer attachedTo = state.attachedTo;
    CEntity* parent = state.bAttached ? attachedTo.GetEntity() : nullptr;

    if (parent == nullptr || parent == object || !parent->IsVTableValid() || parent->m_nType < ENTITY_TYPE_VEHICLE ||
        parent->m_nType > ENTITY_TYPE_OBJECT)
    {
        if (object->m_pAttachedTo != nullptr)
        {
            object->DettachEntityFromEntity(0.0f, 0.0f, 0.0f, false);
        }
        return false;
    }

    if (object->m_pAttachedTo != parent)
    {
        if (object->m_pAttachedTo != nullptr)
        {
            object->DettachEntityFromEntity(0.0f, 0.0f, 0.0f, false);
        }

        // CPhysical::AttachEntityToEntity(CPhysical*, CVector offset, CVector tilt), registers the reference
        plugin::CallMethodAndReturn<CPhysical*, 0x54D570, CPhysical*, CPhysical*, CVector, CVector>(
            object, (CPhysical*)parent, state.offset, state.tilt);
    }

    object->m_vecAttachOffset = state.offset;
    object->m_vecAttachedEntityPosn = state.tilt;
    object->m_qAttachedEntityRotation.imag = state.rotationImag;
    object->m_qAttachedEntityRotation.real = state.rotationReal;

    plugin::CallMethod<0x546FF0, CPhysical*>(object);  // CPhysical::PositionAttachedEntity
    return true;
}

CNetworkObject* CNetworkObject::CreateHosted(CObject* pObject)
{
    int objectid = CNetworkObjectManager::AllocateObjectId();
    if (objectid == Packets::Objects::OBJECT_ID_INVALID)
    {
        logger::warn("[obj] no free object id, model %d is not synced", pObject->m_nModelIndex);
        return nullptr;
    }

    if (pObject->m_matrix == nullptr)
    {
        pObject->AllocateMatrix();
        pObject->m_placement.UpdateMatrix(pObject->m_matrix);
    }

    CNetworkObject* networkObject = new CNetworkObject();

    networkObject->m_pObject = pObject;
    pObject->RegisterReference((CEntity**)&networkObject->m_pObject);

    networkObject->m_nObjectId = objectid;
    networkObject->m_bSyncing = true;
    networkObject->m_nModelId = pObject->m_nModelIndex;
    networkObject->StoreLastUpdate();
    networkObject->m_nLastUpdateTime = GetTickCount();
    GetDamageState(pObject, networkObject->m_lastSentState);

    CNetworkObjectManager::Add(networkObject);

    Packets::Objects::ObjectSpawn packet{};
    packet.objectid = networkObject->m_nObjectId;
    packet.modelid = pObject->m_nModelIndex;
    packet.pos = pObject->m_matrix->pos;
    packet.right = pObject->m_matrix->right;
    packet.up = pObject->m_matrix->up;
    packet.areaCode = pObject->m_nAreaCode;
    packet.damageState = networkObject->m_lastSentState;
    GetPacketFactory().Send(packet);

    logger::info("[obj] hosted SPAWN id=%d model=%d pos=(%.1f %.1f %.1f)", networkObject->m_nObjectId,
        pObject->m_nModelIndex, packet.pos.x, packet.pos.y, packet.pos.z);
    return networkObject;
}

void CNetworkObject::SetMatrix(CObject* object, const CVector& pos, const CVector& right, const CVector& up)
{
    if (object->m_matrix == nullptr)
    {
        object->AllocateMatrix();
    }

    object->m_matrix->right = right;
    object->m_matrix->up = up;
    object->m_matrix->at.Cross(right, up);
    object->m_matrix->pos = pos;

    plugin::CallMethod<0x446F90, CEntity*>(object);  // CEntity::UpdateRW
    object->UpdateRwFrame();
}

// script changes (SET_OBJECT_VISIBLE, SET_OBJECT_COLLISION, BREAK_OBJECT, SET_OBJECT_HEALTH...): the opcodes are
// replayed, but the server only learns the result from here, which late joiners need
void CNetworkObject::SendStateIfChanged()
{
    Packets::Objects::ObjectDamage packet{};
    GetDamageState(m_pObject, packet.state);
    if (packet.state == m_lastSentState)
    {
        return;
    }

    packet.objectid = m_nObjectId;
    packet.bStateOnly = true;
    GetPacketFactory().Send(packet);
    m_lastSentState = packet.state;
}

void CNetworkObject::SendDamageIfChanged(const Packets::Objects::SObjectDamageState& oldState, float damage,
    CVector* colPos, CVector* colNormal, CEntity* damager, eWeaponType weapon)
{
    Packets::Objects::ObjectDamage packet{};
    if (m_pObject != nullptr)
    {
        GetDamageState(m_pObject, packet.state);
    }

    if (m_pObject == nullptr || packet.state == oldState)
    {
        return;
    }

    packet.objectid = m_nObjectId;
    packet.damage = damage;
    packet.weapon = (uint8_t)weapon;
    packet.bHasColPos = colPos != nullptr;
    if (colPos != nullptr)
    {
        packet.colPos = *colPos;
    }
    packet.bHasColNormal = colNormal != nullptr;
    if (colNormal != nullptr)
    {
        packet.colNormal = *colNormal;
    }
    packet.damager.SetEntity(damager);
    GetPacketFactory().Send(packet);
    m_lastSentState = packet.state;
}

void CNetworkObject::CallObjectDamage(CObject* object, float damage, CVector* colPos, CVector* colNormal,
    CEntity* damager, eWeaponType weapon, bool bIgnoreOnlyDamagedByPlayer)
{
    bool oldApplying = ms_bApplyingNetworkDamage;
    bool oldOnlyDamagedByPlayer = object->m_nPhysicalFlags.bInvulnerable;  // bOnlyDamagedByPlayer (0x400000)

    if (bIgnoreOnlyDamagedByPlayer)
    {
        object->m_nPhysicalFlags.bInvulnerable = false;
    }

    ms_bApplyingNetworkDamage = true;
    plugin::CallMethod<0x5A0D90, CObject*, float, CVector*, CVector*, CEntity*, eWeaponType>(
        object, damage, colPos, colNormal, damager, weapon);
    ms_bApplyingNetworkDamage = oldApplying;

    // CObject::ObjectDamage can delete the object (the reference is nulled then)
    if (object->IsVTableValid())
    {
        object->m_nPhysicalFlags.bInvulnerable = oldOnlyDamagedByPlayer;
    }
}

void CNetworkObject::GetDamageState(CObject* object, Packets::Objects::SObjectDamageState& state)
{
    state.health = object->m_fHealth;
    state.lastDamagedWeaponType = object->m_nLastWeaponDamage;
    state.bRenderDamaged = object->m_bRenderDamaged;
    state.bIsVisible = object->m_bIsVisible;
    state.bUsesCollision = object->m_bUsesCollision;
    state.bHasBeenShattered = object->m_nObjectFlags.bIsBroken;
    state.bHasExploded = object->m_nObjectFlags.bIsExploded;
}

void CNetworkObject::ApplyDamageState(CObject* object, const Packets::Objects::SObjectDamageState& state)
{
    // the damaged model is picked when the RW object is created
    if (object->m_bRenderDamaged != state.bRenderDamaged && object->m_pRwObject != nullptr)
    {
        object->DeleteRwObject();
    }

    object->m_fHealth = state.health;
    object->m_nLastWeaponDamage = state.lastDamagedWeaponType;
    object->m_bRenderDamaged = state.bRenderDamaged;
    object->m_bIsVisible = state.bIsVisible;
    object->m_bUsesCollision = state.bUsesCollision;
    object->m_nObjectFlags.bIsBroken = state.bHasBeenShattered;
    object->m_nObjectFlags.bIsExploded = state.bHasExploded;
}
