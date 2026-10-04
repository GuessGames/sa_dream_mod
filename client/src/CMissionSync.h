#pragma once

namespace Packets::Blips
{
class MissionEntityBlips;
class MissionAreas;
}

// Generic sync for missions that were not adapted by hand (Coop.* commands in their scripts).
// The host mirrors what its mission shows: radar blips on peds/vehicles and the "locate" cylinders;
// texts, cutscenes, camera and ped tasks go through COpCodeSync.
class CMissionSync
{
public:
    static void InjectHooks();

    // host: builds and sends snapshots when they change
    static void Process();
    // clients: draws the host's locate cylinders every frame
    static void DrawAreas();

    static void ApplyEntityBlips(const Packets::Blips::MissionEntityBlips& packet);
    static void ApplyAreas(const Packets::Blips::MissionAreas& packet);
};
