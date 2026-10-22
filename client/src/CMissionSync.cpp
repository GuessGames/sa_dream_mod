#include "stdafx.h"
#include "CMissionSync.h"
#include "COpCodeSync.h"
#include <network/packets/blips.h>
#include <CPedPlacement.h>
#include <CCutsceneMgr.h>
#include <CEntryExitManager.h>
#include <CEntryExitTransitionSync.h>

namespace
{
// ---------------------------------------------------------------- locate cylinders
struct SArea
{
    float fromX, fromY, toX, toY, z;
    uint32_t seenAt;
};

std::vector<SArea> g_hostAreas;    // host: seen recently in its mission
std::vector<SArea> g_remoteAreas;  // clients: received from the host
uint32_t g_lastAreasSent = 0;
std::string g_lastAreasKey;

constexpr uint32_t AREA_TIMEOUT_MS = 600;  // scripts may check a locate only every few frames

bool SameArea(const SArea& a, float fromX, float fromY, float toX, float toY)
{
    return fabsf(a.fromX - fromX) < 0.5f && fabsf(a.fromY - fromY) < 0.5f && fabsf(a.toX - toX) < 0.5f &&
           fabsf(a.toY - toY) < 0.5f;
}

// all 14 callers of CTheScripts::HighlightImportantArea are the IS_*_IN_AREA / LOCATE_* script commands
void __cdecl HighlightImportantArea_Hook(int markerId, float fromX, float fromY, float toX, float toY, float z)
{
    CTheScripts::HighlightImportantArea(markerId, fromX, fromY, toX, toY, z);

    if (!CNetwork::m_bAuthenticated || !CLocalPlayer::m_bIsHost)
        return;
    if (!COpCodeSync::IsGenericMissionScript(COpCodeSync::GetCurrentScript()))
        return;

    for (auto& area : g_hostAreas)
    {
        if (SameArea(area, fromX, fromY, toX, toY))
        {
            area.seenAt = GetTickCount();
            area.z = z;
            return;
        }
    }
    if (g_hostAreas.size() < Packets::Blips::MissionAreas::MAX_AREAS)
        g_hostAreas.push_back({fromX, fromY, toX, toY, z, GetTickCount()});
}

void SendAreasIfChanged()
{
    uint32_t now = GetTickCount();
    g_hostAreas.erase(std::remove_if(g_hostAreas.begin(), g_hostAreas.end(),
                          [now](const SArea& a) { return now - a.seenAt > AREA_TIMEOUT_MS; }),
        g_hostAreas.end());

    std::string key;
    for (auto& a : g_hostAreas)
        key += std::to_string((int)a.fromX) + "," + std::to_string((int)a.fromY) + "," + std::to_string((int)a.toX) + "," +
               std::to_string((int)a.toY) + ";";

    // on change, plus a refresh every 5 s for players that joined later
    if (key == g_lastAreasKey && now - g_lastAreasSent < 5000)
        return;

    g_lastAreasKey = key;
    g_lastAreasSent = now;

    Packets::Blips::MissionAreas packet{};
    for (auto& a : g_hostAreas)
    {
        auto& p = packet.areas[packet.count++];
        p.fromX = a.fromX;
        p.fromY = a.fromY;
        p.toX = a.toX;
        p.toY = a.toY;
        p.z = a.z;
    }
    GetPacketFactory().Send(packet);
}

// ---------------------------------------------------------------- entity blips
uint32_t g_lastBlipsSent = 0;
std::string g_lastBlipsKey;

void SendEntityBlipsIfChanged()
{
    Packets::Blips::MissionEntityBlips packet{};
    std::string key;

    // adapted missions show their blips per player with Coop.* commands themselves
    bool missionActive = false;
    if (CTheScripts::IsPlayerOnAMission())
    {
        missionActive = true;
        for (CRunningScript* script = CTheScripts::pActiveScripts; script; script = script->m_pNext)
        {
            if (script->m_bIsMission && COpCodeSync::IsScriptAdapted(script))
                missionActive = false;
        }
    }
    for (int i = 0; i < MAX_RADAR_TRACES && missionActive; i++)
    {
        const tRadarTrace& trace = CRadar::ms_RadarTrace[i];
        if (!trace.m_bInUse ||
            (trace.m_nBlipType != BLIP_CAR && trace.m_nBlipType != BLIP_CHAR && trace.m_nBlipType != BLIP_OBJECT))
            continue;
        if (packet.count >= Packets::Blips::MissionEntityBlips::MAX_BLIPS)
            break;

        Packets::Blips::_MissionEntityBlip blip{};
        if (trace.m_nBlipType == BLIP_OBJECT)
        {
            CObject* object = CPools::GetObject(trace.m_nEntityHandle);
            CNetworkObject* networkObject = object ? CNetworkObjectManager::GetObject(object) : nullptr;
            if (!networkObject)
                continue;
            blip.isObject = 1;
            blip.entityId = networkObject->m_nObjectId;
        }
        else if (trace.m_nBlipType == BLIP_CAR)
        {
            CVehicle* vehicle = CPools::GetVehicle(trace.m_nEntityHandle);
            CNetworkVehicle* networkVehicle = vehicle ? CNetworkVehicleManager::GetVehicle(vehicle) : nullptr;
            if (!networkVehicle || networkVehicle->m_nVehicleId < 0)
                continue;
            blip.isVehicle = 1;
            blip.entityId = networkVehicle->m_nVehicleId;
        }
        else
        {
            CPed* ped = CPools::GetPed(trace.m_nEntityHandle);
            CNetworkPed* networkPed = ped ? CNetworkPedManager::GetPed(ped) : nullptr;
            if (!networkPed || networkPed->m_nPedId < 0)
                continue;  // players and peds that are not networked
            blip.isVehicle = 0;
            blip.entityId = networkPed->m_nPedId;
        }
        blip.colour = static_cast<uint8_t>(std::min<uint32_t>(trace.m_nColour, 15));
        blip.display = trace.m_nBlipDisplay;
        blip.scale = trace.m_nBlipSize;
        blip.friendly = trace.m_bFriendly;
        packet.blips[packet.count++] = blip;

        key += std::to_string(blip.isVehicle) + std::to_string(blip.isObject) + ":" + std::to_string(blip.entityId) + ":" + std::to_string(blip.colour) + ":" +
               std::to_string(blip.display) + ":" + std::to_string(blip.scale) + ";";
    }

    uint32_t now = GetTickCount();
    if (key == g_lastBlipsKey && now - g_lastBlipsSent < 5000)
        return;

    g_lastBlipsKey = key;
    g_lastBlipsSent = now;
    GetPacketFactory().Send(packet);
}

void ClearRemoteBlip(int& handle, bool& missionBlip)
{
    if (missionBlip && handle != -1)
        CRadar::ClearBlip(handle);
    handle = -1;
    missionBlip = false;
}

void SetupBlip(int handle, const Packets::Blips::_MissionEntityBlip& b)
{
    CRadar::ChangeBlipColour(handle, b.colour);
    CRadar::ChangeBlipScale(handle, b.scale);
    CRadar::ChangeBlipDisplay(handle, static_cast<eBlipDisplay>(b.display));
    CRadar::SetBlipFriendly(handle, b.friendly != 0);
}
}  // namespace

// ---------------------------------------------------------------- teleports of the host during a mission
// Missions move the player with SET_CHAR_COORDINATES / SET_CAR_COORDINATES / WARP_* / SET_AREA_VISIBLE,
// which only the host runs, so the others stayed behind; the admin menu teleport too. Any jump of the host that
// its speed can't explain is such a teleport: the others are brought next to the host. Enex doors are left out,
// the others walk through them themselves.
namespace
{
constexpr float TELEPORT_MIN_DIST = 30.0f;  // smaller moves are placements, the others are close anyway
constexpr float BRING_SKIP_DIST = 15.0f;    // already near the new position (adapted missions bring themselves)

constexpr uint32_t BRING_SETTLE_MS = 500;    // missions often move the host twice in a row (into an interior, out)

CVector g_vecLastHostPos{};
int g_nLastArea = -1;
bool g_bBringPending = false;
uint32_t g_nLastJumpAt = 0;

bool IsHostOnMission() { return CNetwork::m_bAuthenticated && CLocalPlayer::m_bIsHost && CTheScripts::IsPlayerOnAMission(); }

CVector FindSpotNearHost(const CVector& hostPos, float heading, int index)
{
    // behind and beside the host, a wider ring for more players
    float dist = 2.0f + 1.0f * (index / 6);
    float angle = heading + 3.14159f + ((index % 6) - 2.5f) * 0.6f;
    CVector spot = hostPos + CVector(-sinf(angle), cosf(angle), 0.0f) * dist;
    if (CPedPlacement::FindZCoorForPed(&spot) &&
        !CWorld::TestSphereAgainstWorld(spot, 0.35f, nullptr, true, true, true, true, true, false) &&
        CWorld::GetIsLineOfSightClear(hostPos, spot, true, true, false, true, true, false, false))
    {
        return spot;
    }
    return hostPos;
}

void BringPlayersToHost()
{
    CPlayerPed* hostPed = FindPlayerPed(0);
    if (!hostPed)
        return;

    CVector hostPos = hostPed->GetPosition();
    float heading = hostPed->m_fCurrentRotation;
    int index = 0;
    for (CNetworkPlayer* player : CNetworkPlayerManager::m_pPlayers)
    {
        if (!player->m_pPed || !player->m_pPed->IsVTableValid())
            continue;
        if (player->m_pPed->m_nAreaCode == CGame::currArea &&
            DistanceBetweenPoints(player->m_pPed->GetPosition(), hostPos) < BRING_SKIP_DIST)
            continue;

        Packets::Players::PlayerBring packet{};
        packet.targetid = player->m_iPlayerId;
        packet.pos = FindSpotNearHost(hostPos, heading, index++);
        packet.interior = CGame::currArea;
        packet.heading = heading;
        packet.bMission = true;
        GetPacketFactory().Send(packet);
        logger::info("[mission] host teleported, bringing %s to (%.1f %.1f %.1f) area %d", player->GetName().c_str(),
            packet.pos.x, packet.pos.y, packet.pos.z, packet.interior);
    }
}

bool IsInEnexTransition()
{
    return CEntryExitManager::ms_exitEnterState != 0 || CEntryExitTransitionSync::IsLocalTransitionActive();
}

void DetectHostTeleport()
{
    CPlayerPed* ped = FindPlayerPed(0);
    // a respawn after wasted/busted is not a mission teleport
    if (!ped || !IsHostOnMission() || CWorld::Players[0].m_nPlayerState != PLAYERSTATE_PLAYING)
    {
        g_bBringPending = false;
        g_nLastArea = -1;
        return;
    }

    CVector pos = ped->GetPosition();
    if (g_nLastArea != -1 && !IsInEnexTransition())
    {
        // the farthest the host could have moved on his own this frame
        CPhysical* mover = ped->m_nPedFlags.bInVehicle && ped->m_pVehicle ? (CPhysical*)ped->m_pVehicle : (CPhysical*)ped;
        float ownMove = mover->m_vecMoveSpeed.Magnitude() * CTimer::ms_fTimeStep * 2.0f;
        float jump = DistanceBetweenPoints(pos, g_vecLastHostPos);
        // an area change alone is a cutscene switching the visible area; interiors are far away from the map
        if (jump > TELEPORT_MIN_DIST + ownMove)
        {
            logger::info("[mission] host jumped %.0f m (area %d -> %d)", jump, g_nLastArea, (int)CGame::currArea);
            g_bBringPending = true;
            g_nLastJumpAt = GetTickCount();
        }
    }
    g_vecLastHostPos = pos;
    g_nLastArea = CGame::currArea;

    // during a cutscene, a fade or right after a jump the script is still placing the host:
    // bring them once, to where he ends up
    if (g_bBringPending && !CCutsceneMgr::ms_running && !TheCamera.m_bFading &&
        GetTickCount() - g_nLastJumpAt > BRING_SETTLE_MS)
    {
        g_bBringPending = false;
        BringPlayersToHost();
    }
}
}  // namespace
void CMissionSync::InjectHooks()
{
    Events::gameProcessEvent.after += DetectHostTeleport;

    const int callers[] = {0x467989, 0x467A96, 0x467FA0, 0x46805E, 0x4801D7, 0x48704F, 0x4873A4, 0x4876B2, 0x4879B2,
        0x487CA9, 0x487F13, 0x488E6A, 0x489111, 0x489378};
    for (int address : callers)
        patch::RedirectCall(address, HighlightImportantArea_Hook);
}

void CMissionSync::Process()
{
    if (!CNetwork::m_bAuthenticated || !CLocalPlayer::m_bIsHost)
        return;

    // 4 times per second is plenty for blips and markers
    static uint32_t lastRun = 0;
    if (GetTickCount() - lastRun < 250)
        return;
    lastRun = GetTickCount();

    SendAreasIfChanged();
    SendEntityBlipsIfChanged();
}

void CMissionSync::DrawAreas()
{
    if (!CNetwork::m_bAuthenticated || CLocalPlayer::m_bIsHost)
        return;

    int id = 0x7A00;
    for (auto& a : g_remoteAreas)
        CTheScripts::HighlightImportantArea(id++, a.fromX, a.fromY, a.toX, a.toY, a.z);
}

void CMissionSync::ApplyAreas(const Packets::Blips::MissionAreas& packet)
{
    if (packet.count != g_remoteAreas.size())
        logger::info("[mission] locate areas: %d", packet.count);
    g_remoteAreas.clear();
    for (int i = 0; i < packet.count; i++)
    {
        auto& p = packet.areas[i];
        g_remoteAreas.push_back({p.fromX, p.fromY, p.toX, p.toY, p.z, 0});
    }
}

void CMissionSync::ApplyEntityBlips(const Packets::Blips::MissionEntityBlips& packet)
{
    static int lastCount = -1;
    if (packet.count != lastCount)
        logger::info("[mission] entity blips: %d", packet.count);
    lastCount = packet.count;

    // remove the blips that are not in the snapshot anymore
    for (auto* networkPed : CNetworkPedManager::m_pPeds)
    {
        if (!networkPed->m_bMissionBlip)
            continue;
        bool keep = false;
        for (int i = 0; i < packet.count && !keep; i++)
            keep = !packet.blips[i].isVehicle && !packet.blips[i].isObject &&
                   packet.blips[i].entityId == networkPed->m_nPedId;
        if (!keep)
            ClearRemoteBlip(networkPed->m_nBlipHandle, networkPed->m_bMissionBlip);
    }
    for (auto* networkVehicle : CNetworkVehicleManager::m_pVehicles)
    {
        if (!networkVehicle->m_bMissionBlip)
            continue;
        bool keep = false;
        for (int i = 0; i < packet.count && !keep; i++)
            keep = packet.blips[i].isVehicle && packet.blips[i].entityId == networkVehicle->m_nVehicleId;
        if (!keep)
            ClearRemoteBlip(networkVehicle->m_nBlipHandle, networkVehicle->m_bMissionBlip);
    }
    for (auto* networkObject : CNetworkObjectManager::m_pObjects)
    {
        if (!networkObject->m_bMissionBlip)
            continue;
        bool keep = false;
        for (int i = 0; i < packet.count && !keep; i++)
            keep = packet.blips[i].isObject && packet.blips[i].entityId == networkObject->m_nObjectId;
        if (!keep)
            ClearRemoteBlip(networkObject->m_nBlipHandle, networkObject->m_bMissionBlip);
    }

    // create/update the rest (blips made by adapted missions via Coop.* commands are left alone)
    for (int i = 0; i < packet.count; i++)
    {
        const auto& b = packet.blips[i];
        if (b.isObject)
        {
            CNetworkObject* networkObject = CNetworkObjectManager::GetObject(b.entityId);
            if (!networkObject || !networkObject->m_pObject || networkObject->m_bSyncing)
                continue;
            if (networkObject->m_nBlipHandle == -1)
            {
                networkObject->m_nBlipHandle = CRadar::SetEntityBlip(
                    BLIP_OBJECT, CPools::GetObjectRef(networkObject->m_pObject), 0, static_cast<eBlipDisplay>(b.display));
                networkObject->m_bMissionBlip = true;
            }
            if (networkObject->m_bMissionBlip)
                SetupBlip(networkObject->m_nBlipHandle, b);
        }
        else if (b.isVehicle)
        {
            CNetworkVehicle* networkVehicle = CNetworkVehicleManager::GetVehicle(b.entityId);
            if (!networkVehicle || !networkVehicle->m_pVehicle)
                continue;
            if (networkVehicle->m_nBlipHandle == -1)
            {
                networkVehicle->m_nBlipHandle = CRadar::SetEntityBlip(
                    BLIP_CAR, CPools::GetVehicleRef(networkVehicle->m_pVehicle), 0, static_cast<eBlipDisplay>(b.display));
                networkVehicle->m_bMissionBlip = true;
            }
            if (networkVehicle->m_bMissionBlip)
                SetupBlip(networkVehicle->m_nBlipHandle, b);
        }
        else
        {
            CNetworkPed* networkPed = CNetworkPedManager::GetPed(b.entityId);
            if (!networkPed || !networkPed->m_pPed)
                continue;
            if (networkPed->m_nBlipHandle == -1)
            {
                networkPed->m_nBlipHandle = CRadar::SetEntityBlip(
                    BLIP_CHAR, CPools::GetPedRef(networkPed->m_pPed), 0, static_cast<eBlipDisplay>(b.display));
                networkPed->m_bMissionBlip = true;
            }
            if (networkPed->m_bMissionBlip)
                SetupBlip(networkPed->m_nBlipHandle, b);
        }
    }
}
