#pragma once

class CWeatherSync
{
public:
	// host: sends the state when the weather changes and every 2 s
	static void Process();
	static void SyncCurrentState();
	static void HandlePacket(Packets::World::GameWeatherTime* pGameWeatherTime);
	// clients: keeps the host's weather every frame (the game would otherwise pick its own each hour)
	static void ApplyClient();
	static void ServerTimeRecalculated(server_time_t serverTime);

	static inline bool ms_bHasHostState = false;
	static inline short ms_nHostOld = 0, ms_nHostNew = 0, ms_nHostForced = -1;
};
