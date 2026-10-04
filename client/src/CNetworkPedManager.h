#pragma once
class CNetworkPedManager
{
public:
	static std::vector<CNetworkPed*> m_pPeds;
	static CNetworkPed* m_apTempPeds[255];

	static CNetworkPed* GetPed(int pedid);
	static CNetworkPed* GetPed(CEntity* entity);
	static void Add(CNetworkPed* ped);
	static void Remove(CNetworkPed* ped);
	static void Update();
	static void Process();
	static void AssignHost();
	static unsigned char AddToTempList(CNetworkPed* networkPed);
	static void RemoveHostedUnused();
	static bool IsNearRemotePlayer(const CVector& pos, float radius);
	static constexpr float KEEP_ALIVE_RADIUS = 100.0f;
};

