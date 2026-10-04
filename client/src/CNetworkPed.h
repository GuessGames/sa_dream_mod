#pragma once
class CNetworkPed
{
private:
	CNetworkPed() {}
public:
	int m_nPedId = -1;
	CPed* m_pPed = nullptr;
	bool m_bSyncing = false;
	unsigned char m_nTempId = 255;
	ePedType m_nPedType;
	unsigned char m_nCreatedBy;
	CVector m_vecVelocity{0.0f, 0.0f, 0.0f};
	float m_fAimingRotation = 0.0f;
	float m_fCurrentRotation = 0.0f;
	float m_fLookDirection = 0.0f;
	eMoveState m_nMoveState = eMoveState::PEDMOVE_NONE;
	float m_fMoveBlendRatio = 0.0f;
	CAutoPilot m_autoPilot;
	float m_fGasPedal = 0.0f;
	float m_fBreakPedal = 0.0f;
	float m_fSteerAngle = 0.0f;
	float m_fHealth = 100.0f;
	int m_nBlipHandle = -1;
	bool m_bMissionBlip = false; // m_nBlipHandle was created by the generic mission sync
	bool m_bClaimOnRelease = false;
	// pool handle at creation: detects a pool slot reused by another ped (pointer alone is not enough)
	int m_nPoolRef = -1;
	// the game removed our hosted ped before the server confirmed its id
	bool m_bRemovedBeforeConfirm = false;
	// a random ped temporarily marked as mission so the population code doesn't delete it next to another player
	bool m_bKeptAlive = false;
	// real AI settings, puppets (peds synced by someone else) get them disabled
	int m_nOrigDmType = -1;
	float m_fOrigHearingRange = 30.0f;
	float m_fOrigSeeingRange = 30.0f;
	unsigned int m_nOrigDmNumPedsToScan = 0;
	float m_fOrigDmRadius = 0.0f;

	static CNetworkPed* CreateHosted(CPed* ped);
	void WarpIntoVehicleDriver(CVehicle* vehicle);
	void WarpIntoVehiclePassenger(CVehicle* vehicle, int seatid);
	void RemoveFromVehicle(CVehicle* vehicle);
	void ClaimOnRelease();
	void CancelClaim();

	void ApplyWeaponSnapshot(Packets::Players::SWeaponSnapshot& weaponSnapshot);
	bool IsPedValid();
	// explicit ownership change from the server; taking over a puppet gives it its AI back
	void SetSyncing(bool syncing);
	void SetKeptAlive(bool keep);

	CNetworkPed(int pedid, int modelId, ePedType pedType, CVector pos, unsigned char createdBy, char specialModelName[]);
	~CNetworkPed();
};

