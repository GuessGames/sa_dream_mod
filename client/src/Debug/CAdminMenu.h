#pragma once

// In-game admin menu / trainer (F8): missions, player, vehicles, teleports, world.
// Mission and world controls are host-only while connected: the host runs the mission scripts
// and its time/weather are synced to everyone.
class CAdminMenu
{
public:
    static void Init();
    static void DrawUI();

    static inline bool ms_bOpen = false;

private:
    static void Process();
    static void DrawMissionsTab();
    static void DrawPlayerTab();
    static void DrawVehicleTab();
    static void DrawTeleportTab();
    static void DrawWorldTab();
};
