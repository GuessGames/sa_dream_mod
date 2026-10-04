#pragma once
#include <semver.h>

class CCore
{
public:
	static void Init();
	static void AllocateConsole();
	static void RedirectOutputToLogFile();
	static std::string GetProfileSuffix();

	static inline bool ms_bDebug = false;
	static inline uint8_t ms_nRunIndex;
	static inline int ms_nProfile = 0;

	static semver_t Version;
	static inline injector::game_version_manager gvm{};
};

