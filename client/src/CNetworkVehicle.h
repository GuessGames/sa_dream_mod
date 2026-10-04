#pragma once
class CNetworkVehicle
{
private:
	CNetworkVehicle() {}
public:
	int m_nVehicleId = -1;
	CVehicle* m_pVehicle = nullptr;
	int m_nModelId = 0;
	char m_nPaintJob = -1;
	bool m_bSyncing = false;
	unsigned char m_nTempId = 255;
	unsigned char m_nCreatedBy;
	int m_nBlipHandle = -1;
	bool m_bMissionBlip = false; // m_nBlipHandle was created by the generic mission sync
	// pool handle at creation: detects a pool slot reused by another vehicle
	int m_nPoolRef = -1;
	// the game removed our hosted vehicle before the server confirmed its id
	bool m_bRemovedBeforeConfirm = false;
	// a random/parked vehicle temporarily marked as mission so it isn't deleted next to another player
	bool m_bKeptAlive = false;
	Packets::Vehicles::VehicleDriverUpdate m_playerDriverSnapshot{};
	CDamageManager m_oldDamageState{};

	~CNetworkVehicle();
	CNetworkVehicle(int vehicleid, int modelid, CVector pos, float rotation, unsigned char color1, unsigned char color2, unsigned char createdBy);
	bool CreateVehicle(int vehicleid, int modelid, CVector pos, float rotation, unsigned char color1, unsigned char color2);
	bool HasDriver();
	bool IsVehicleValid();
	void SetSyncing(bool syncing);
	void SetKeptAlive(bool keep);
	
	static CNetworkVehicle* CreateHosted(CVehicle* vehicle);
};

