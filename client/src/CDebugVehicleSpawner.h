#pragma once
class CDebugVehicleSpawner
{
public:
	// model 400 + index
	static const char* GetVehicleName(int index);
	static constexpr int NUM_VEHICLES = 212;
public:
	static void Process();
};

