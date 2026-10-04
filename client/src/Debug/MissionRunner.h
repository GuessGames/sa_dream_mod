#pragma once

class MissionRunner
{
public:
	static bool DrawUI();
	static int GetMissionCount();
	static const char* GetMissionName(int index);
	static int GetMissionId(int index);
};
