#pragma once

class CImGui
{
public: 
	static void Init();
	static void SetActive(bool bActive);
	// active (mouse cursor, input captured) while the debug window or the admin menu is open
	static void UpdateActive();
	static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static inline bool ms_bActive = false;
	static inline bool ms_bDebugWindow = false;
};
