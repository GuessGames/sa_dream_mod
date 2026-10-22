#pragma once

// a mission object (CREATE_OBJECT of a synced mission script). Ported from upstream CoopAndreas df03bbf:
// the creator (the host) is the syncer: it sends the movement, attachment and damage state,
// the others only show it and ask the syncer to apply their damage.
class CNetworkObject
{
private:
    CNetworkObject() {}

public:
    int m_nObjectId = Packets::Objects::OBJECT_ID_INVALID;
    CObject* m_pObject = nullptr;  // registered reference: the game nulls it when it deletes the object
    int m_nModelId = 0;
    bool m_bSyncing = false;
    int m_nBlipHandle = -1;
    bool m_bMissionBlip = false;  // m_nBlipHandle was created by the generic mission sync

    CVector m_vecLastPos{};
    CVector m_vecLastRight{};
    CVector m_vecLastUp{};
    CVector m_vecLastMoveSpeed{};
    CVector m_vecLastTurnSpeed{};
    CPhysical* m_pLastAttachedTo = nullptr;
    CVector m_vecLastAttachOffset{};
    CVector m_vecLastAttachTilt{};
    CQuaternion m_lastAttachRotation{};
    uint32_t m_nLastUpdateTime = 0;
    Packets::Objects::SObjectDamageState m_lastSentState{};

    inline static bool ms_bApplyingNetworkDamage = false;

    ~CNetworkObject();
    CNetworkObject(const Packets::Objects::ObjectSpawn& packet);
    bool CreateObject(const Packets::Objects::ObjectSpawn& packet);
    bool HasChangedSinceLastUpdate();
    void StoreLastUpdate();
    void SendStateIfChanged();
    void SendDamageIfChanged(const Packets::Objects::SObjectDamageState& oldState, float damage, CVector* colPos,
        CVector* colNormal, CEntity* damager, eWeaponType weapon);
    static CNetworkObject* CreateHosted(CObject* object);
    static void CallObjectDamage(CObject* object, float damage, CVector* colPos, CVector* colNormal, CEntity* damager,
        eWeaponType weapon, bool bIgnoreOnlyDamagedByPlayer);
    static void SetMatrix(CObject* object, const CVector& pos, const CVector& right, const CVector& up);
    static void GetAttachState(CObject* object, Packets::Objects::SObjectAttachState& state);
    static bool ApplyAttachState(CObject* object, const Packets::Objects::SObjectAttachState& state);
    static bool IsAttachedToNetworkEntity(CObject* object);
    static void GetDamageState(CObject* object, Packets::Objects::SObjectDamageState& state);
    static void ApplyDamageState(CObject* object, const Packets::Objects::SObjectDamageState& state);
    // map objects: the same on every PC (where the map placed it, even if it was pushed since)
    static CVector GetMapPosition(CObject* object);
    static CObject* FindMapObject(int modelid, const CVector& mapPos);
};
