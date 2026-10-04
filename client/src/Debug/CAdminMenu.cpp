#include "stdafx.h"
#include "CAdminMenu.h"
#include "CImGui.h"
#include "MissionRunner.h"
#include <imgui.h>
#include <CCheat.h>
#include <CDebugVehicleSpawner.h>

namespace
{
const char* WEATHER_NAMES[] = {"EXTRASUNNY_LA", "SUNNY_LA", "EXTRASUNNY_SMOG_LA", "SUNNY_SMOG_LA", "CLOUDY_LA", "SUNNY_SF",
    "EXTRASUNNY_SF", "CLOUDY_SF", "RAINY_SF", "FOGGY_SF", "SUNNY_VEGAS", "EXTRASUNNY_VEGAS", "CLOUDY_VEGAS",
    "EXTRASUNNY_COUNTRYSIDE", "SUNNY_COUNTRYSIDE", "CLOUDY_COUNTRYSIDE", "RAINY_COUNTRYSIDE", "EXTRASUNNY_DESERT",
    "SUNNY_DESERT", "SANDSTORM_DESERT", "UNDERWATER", "EXTRACOLOURS_1", "EXTRACOLOURS_2"};

bool bGodMode = false;
bool bNeverWanted = false;
int nPendingMissionId = -1;  // launched as soon as the current mission has been cleaned up
uint32_t nPendingSince = 0;
int nLastLaunchedMissionId = -1;
CVector vecSavedPos{};
bool bHasSavedPos = false;
char szMissionFilter[64] = "";
char szVehicleFilter[32] = "";
std::string sStatus;

void SetStatus(const std::string& text)
{
    sStatus = text;
    logger::info("[admin] %s", text.c_str());
}

// while connected only the host may touch missions/world
bool CanControlWorld()
{
    return !CNetwork::m_bAuthenticated || CLocalPlayer::m_bIsHost;
}

bool IsMissionActive()
{
    return CTheScripts::IsPlayerOnAMission() || CTheScripts::bAlreadyRunningAMissionScript;
}

const char* GetActiveMissionScriptName()
{
    for (CRunningScript* script = CTheScripts::pActiveScripts; script; script = script->m_pNext)
    {
        if (script->m_bIsMission)
            return script->m_szName;
    }
    return nullptr;
}

const char* GetMissionNameById(int id)
{
    for (int i = 0; i < MissionRunner::GetMissionCount(); i++)
    {
        if (MissionRunner::GetMissionId(i) == id)
            return MissionRunner::GetMissionName(i);
    }
    return "?";
}

bool CanLaunchHere(std::string& reason)
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
    {
        reason = "Гравець недоступний";
        return false;
    }
    if (CGame::currArea != AREA_MAIN_MAP || ped->m_nAreaCode != AREA_MAIN_MAP)
    {
        reason = "Вийдіть з інтер'єру, щоб запустити місію";
        return false;
    }
    return true;
}

void LaunchMission(int id)
{
    if (id < 0 || id >= CTheScripts::NumberOfMissionScripts)
        return;

    nLastLaunchedMissionId = id;
    Command<Commands::LOAD_AND_LAUNCH_MISSION_INTERNAL>(id);
    SetStatus(std::string("Запущено місію: ") + GetMissionNameById(id));
}

// fails the running mission (its own cleanup runs) and launches `id` once it is gone
void ReplaceMission(int id)
{
    if (IsMissionActive())
    {
        Command<Commands::FAIL_CURRENT_MISSION>();
        nPendingMissionId = id;
        nPendingSince = GetTickCount();
        SetStatus(std::string("Поточну місію провалено, далі: ") + GetMissionNameById(id));
    }
    else
    {
        LaunchMission(id);
    }
}

void Teleport(CVector pos, bool findGround)
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    Command<Commands::REQUEST_COLLISION>(pos.x, pos.y);
    Command<Commands::LOAD_SCENE>(pos.x, pos.y, pos.z);
    if (findGround)
    {
        float groundZ = 0.0f;
        Command<Commands::GET_GROUND_Z_FOR_3D_COORD>(pos.x, pos.y, 1000.0f, &groundZ);
        pos.z = groundZ + 1.0f;
    }

    if (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle && ped->m_pVehicle->m_pDriver == ped)
        Command<Commands::SET_CAR_COORDINATES>(CPools::GetVehicleRef(ped->m_pVehicle), pos.x, pos.y, pos.z);
    else
        Command<Commands::SET_CHAR_COORDINATES>(CPools::GetPedRef(ped), pos.x, pos.y, pos.z);

    SetStatus("Телепорт: " + std::to_string((int)pos.x) + ", " + std::to_string((int)pos.y) + ", " + std::to_string((int)pos.z));
}

bool GetWaypoint(CVector& out)
{
    int handle = FrontEndMenuManager.m_nTargetBlipIndex;
    if (!handle || CRadar::GetActualBlipArrayIndex(handle) == -1)
        return false;

    out = CRadar::ms_RadarTrace[handle & 0xFFFF].m_vecPos;
    return true;
}

bool ContainsNoCase(const char* text, const char* filter)
{
    if (!filter[0])
        return true;
    std::string a(text), b(filter);
    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);
    return a.find(b) != std::string::npos;
}

void HostOnlyNote()
{
    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Це може робити тільки хост (той, хто підключився першим).");
}
}  // namespace

void CAdminMenu::Init()
{
    Events::gameProcessEvent += [] { Process(); };
}

void CAdminMenu::Process()
{
    // F8 toggles the menu (only while the game window has the focus)
    static bool bF8WasDown = false;
    bool bF8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0 && GetForegroundWindow() == RsGlobal.ps->window;
    if (bF8Down && !bF8WasDown && !FrontEndMenuManager.m_bMenuActive)
    {
        ms_bOpen = !ms_bOpen;
        CImGui::UpdateActive();
    }
    bF8WasDown = bF8Down;

    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    if (bGodMode)
    {
        ped->m_fHealth = ped->m_fMaxHealth;
        if (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle && ped->m_pVehicle->m_fHealth < 1000.0f)
            ped->m_pVehicle->m_fHealth = 1000.0f;
    }

    if (bNeverWanted && FindPlayerWanted(0) && FindPlayerWanted(0)->m_nWantedLevel > 0)
        Command<Commands::CLEAR_WANTED_LEVEL>(0);

    // waiting for the replaced mission to clean up (gives up after 15 s)
    if (nPendingMissionId >= 0)
    {
        if (!IsMissionActive())
        {
            int id = nPendingMissionId;
            nPendingMissionId = -1;
            LaunchMission(id);
        }
        else if (GetTickCount() - nPendingSince > 15000)
        {
            nPendingMissionId = -1;
            SetStatus("Місія не завершилась за 15 с, запуск скасовано");
        }
    }
}

void CAdminMenu::DrawUI()
{
    if (!ms_bOpen)
        return;

    float scale = ImGui::GetStyle().FontScaleMain;
    ImGui::SetNextWindowSize(ImVec2(470.0f * scale, 560.0f * scale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(40.0f, 60.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("SA Dream Mod — адмін-меню (F8)", &ms_bOpen))
    {
        if (CNetwork::m_bAuthenticated)
            ImGui::TextDisabled(CLocalPlayer::m_bIsHost ? "Ви хост" : "Ви не хост: місії та світ керує хост");
        else
            ImGui::TextDisabled("Не підключено до сервера");

        if (ImGui::BeginTabBar("##admin_tabs"))
        {
            if (ImGui::BeginTabItem("Місії")) { DrawMissionsTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Гравець")) { DrawPlayerTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Транспорт")) { DrawVehicleTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Телепорт")) { DrawTeleportTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Світ")) { DrawWorldTab(); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }

        if (!sStatus.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("%s", sStatus.c_str());
        }
    }
    ImGui::End();

    if (!ms_bOpen)
        CImGui::UpdateActive();
}

void CAdminMenu::DrawMissionsTab()
{
    bool canControl = CanControlWorld();
    if (!canControl)
        HostOnlyNote();

    const char* active = GetActiveMissionScriptName();
    if (active || IsMissionActive())
        ImGui::Text("Активна місія: %s", active ? active : "(завантажується)");
    else
        ImGui::TextDisabled("Зараз немає активної місії");

    if (nPendingMissionId >= 0)
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Очікую завершення, потім: %s", GetMissionNameById(nPendingMissionId));

    ImGui::BeginDisabled(!canControl);

    ImGui::BeginDisabled(!IsMissionActive());
    if (ImGui::Button("Провалити / скасувати місію", ImVec2(-1.0f, 0.0f)))
    {
        Command<Commands::FAIL_CURRENT_MISSION>();
        SetStatus("Місію провалено");
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(nLastLaunchedMissionId < 0);
    std::string restart = "Перезапустити останню";
    if (nLastLaunchedMissionId >= 0)
        restart += std::string(": ") + GetMissionNameById(nLastLaunchedMissionId);
    if (ImGui::Button(restart.c_str(), ImVec2(-1.0f, 0.0f)))
        ReplaceMission(nLastLaunchedMissionId);
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextWrapped("Запуск місії (якщо інша вже йде — її буде провалено і замінено):");
    std::string reason;
    bool canLaunch = CanLaunchHere(reason);
    if (!canLaunch)
        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "%s", reason.c_str());

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##mission_filter", "Пошук місії…", szMissionFilter, sizeof(szMissionFilter));

    ImGui::BeginChild("##missions", ImVec2(0.0f, 0.0f), true);
    ImGui::BeginDisabled(!canLaunch);
    for (int i = 0; i < MissionRunner::GetMissionCount(); i++)
    {
        const char* name = MissionRunner::GetMissionName(i);
        if (!ContainsNoCase(name, szMissionFilter))
            continue;

        int id = MissionRunner::GetMissionId(i);
        bool valid = id >= 0 && id < CTheScripts::NumberOfMissionScripts;
        ImGui::BeginDisabled(!valid);
        ImGui::PushID(i);
        if (ImGui::Selectable(name))
            ReplaceMission(id);
        ImGui::PopID();
        ImGui::EndDisabled();
    }
    ImGui::EndDisabled();
    ImGui::EndChild();

    ImGui::EndDisabled();
}

void CAdminMenu::DrawPlayerTab()
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    if (ImGui::Button("Здоров'я 100%", ImVec2(w, 0.0f)))
    {
        ped->m_fHealth = ped->m_fMaxHealth;
        SetStatus("Здоров'я відновлено");
    }
    ImGui::SameLine();
    if (ImGui::Button("Броня 100", ImVec2(w, 0.0f)))
    {
        ped->m_fArmour = 100.0f;
        SetStatus("Броню видано");
    }

    if (ImGui::Checkbox("Безсмертя (здоров'я, вогонь, вибухи, падіння)", &bGodMode))
    {
        int ref = CPools::GetPedRef(ped);
        Command<Commands::SET_CHAR_PROOFS>(ref, bGodMode, bGodMode, bGodMode, bGodMode, bGodMode);
        SetStatus(bGodMode ? "Безсмертя увімкнено" : "Безсмертя вимкнено");
    }
    if (ImGui::Checkbox("Ніколи не розшукується", &bNeverWanted))
        SetStatus(bNeverWanted ? "Розшук вимкнено" : "Розшук знову працює");

    if (ImGui::Button("Зняти розшук", ImVec2(-1.0f, 0.0f)))
    {
        Command<Commands::CLEAR_WANTED_LEVEL>(0);
        SetStatus("Розшук знято");
    }

    ImGui::SeparatorText("Гроші");
    for (int amount : {1000, 10000, 100000, 1000000})
    {
        std::string label = "+$" + std::to_string(amount);
        if (ImGui::Button(label.c_str()))
        {
            CWorld::Players[CWorld::PlayerInFocus].m_nMoney += amount;
            SetStatus("Гроші " + label);
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();

    ImGui::SeparatorText("Зброя");
    if (ImGui::Button("Набір 1 (бандит)", ImVec2(w, 0.0f))) { CCheat::WeaponCheat1(); SetStatus("Видано набір зброї 1"); }
    ImGui::SameLine();
    if (ImGui::Button("Набір 2 (професіонал)", ImVec2(w, 0.0f))) { CCheat::WeaponCheat2(); SetStatus("Видано набір зброї 2"); }
    if (ImGui::Button("Набір 3 (важка)", ImVec2(w, 0.0f))) { CCheat::WeaponCheat3(); SetStatus("Видано набір зброї 3"); }
    ImGui::SameLine();
    if (ImGui::Button("Парашут", ImVec2(w, 0.0f))) { CCheat::ParachuteCheat(); SetStatus("Видано парашут"); }
    if (ImGui::Button("Джетпак", ImVec2(w, 0.0f))) { CCheat::JetpackCheat(); SetStatus("Видано джетпак"); }
    ImGui::SameLine();
    if (ImGui::Button("Максимальні навички", ImVec2(w, 0.0f)))
    {
        CCheat::WeaponSkillsCheat();
        CCheat::VehicleSkillsCheat();
        SetStatus("Навички зброї й водіння на максимумі");
    }
}

void CAdminMenu::DrawVehicleTab()
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    CVehicle* vehicle = ped->m_nPedFlags.bInVehicle ? ped->m_pVehicle : nullptr;
    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    ImGui::BeginDisabled(!vehicle);
    if (ImGui::Button("Відремонтувати", ImVec2(w, 0.0f)) && vehicle)
    {
        vehicle->Fix();
        vehicle->m_fHealth = 1000.0f;
        SetStatus("Транспорт відремонтовано");
    }
    ImGui::SameLine();
    if (ImGui::Button("Перевернути на колеса", ImVec2(w, 0.0f)) && vehicle)
    {
        Command<Commands::SET_CAR_HEADING>(CPools::GetVehicleRef(vehicle), vehicle->GetHeading() * 57.2958f);
        SetStatus("Транспорт поставлено на колеса");
    }
    ImGui::EndDisabled();
    if (!vehicle)
        ImGui::TextDisabled("Ремонт і переворот — коли ви в транспорті");

    ImGui::SeparatorText("Створити транспорт");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##vehicle_filter", "Пошук: INFERNUS, NRG500, HYDRA…", szVehicleFilter, sizeof(szVehicleFilter));
    ImGui::BeginChild("##vehicles", ImVec2(0.0f, 0.0f), true);
    for (int i = 0; i < CDebugVehicleSpawner::NUM_VEHICLES; i++)
    {
        const char* name = CDebugVehicleSpawner::GetVehicleName(i);
        if (!ContainsNoCase(name, szVehicleFilter))
            continue;

        std::string label = std::string(name) + "  (" + std::to_string(400 + i) + ")";
        if (ImGui::Selectable(label.c_str()))
        {
            CVehicle* spawned = CCheat::VehicleCheat(400 + i);
            if (spawned)
            {
                spawned->m_nVehicleFlags.bHasBeenOwnedByPlayer = true;
                SetStatus(std::string("Створено: ") + name);
            }
            else
            {
                SetStatus(std::string("Зараз не можна створити ") + name + " (ви в транспорті чи в інтер'єрі?)");
            }
        }
    }
    ImGui::EndChild();
}

void CAdminMenu::DrawTeleportTab()
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    CVector waypoint;
    bool hasWaypoint = GetWaypoint(waypoint);
    ImGui::BeginDisabled(!hasWaypoint);
    if (ImGui::Button("До мітки на карті", ImVec2(-1.0f, 0.0f)) && hasWaypoint)
        Teleport(waypoint, true);
    ImGui::EndDisabled();
    if (!hasWaypoint)
        ImGui::TextDisabled("Поставте мітку на карті (меню паузи → Карта)");

    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if (ImGui::Button("Запам'ятати позицію", ImVec2(w, 0.0f)))
    {
        vecSavedPos = ped->GetPosition();
        bHasSavedPos = true;
        SetStatus("Позицію збережено");
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!bHasSavedPos);
    if (ImGui::Button("Повернутися на неї", ImVec2(w, 0.0f)))
        Teleport(vecSavedPos, false);
    ImGui::EndDisabled();

    ImGui::SeparatorText("До гравця");
    bool any = false;
    for (auto* player : CNetworkPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPed)
            continue;
        any = true;
        std::string label = player->GetName();
        if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 0.0f)))
        {
            CVector pos = player->m_pPed->GetPosition();
            pos.x += 2.0f;
            Teleport(pos, false);
        }
    }
    if (!any)
        ImGui::TextDisabled("Інших гравців немає");

    ImGui::SeparatorText("Місця");
    struct SPlace { const char* name; CVector pos; };
    static const SPlace places[] = {
        {"Grove Street (дім CJ)", CVector(2495.0f, -1687.0f, 13.5f)},
        {"Лос-Сантос, аеропорт", CVector(1682.0f, -2286.0f, 13.5f)},
        {"Сан-Фієрро, гараж CJ", CVector(-2026.0f, 156.0f, 29.0f)},
        {"Лас-Вентурас, Strip", CVector(2027.0f, 1008.0f, 10.8f)},
        {"Гора Чіліад", CVector(-2320.0f, -1620.0f, 483.7f)},
        {"Зона 69", CVector(213.0f, 1867.0f, 17.6f)},
    };
    for (const SPlace& place : places)
    {
        if (ImGui::Button(place.name, ImVec2(-1.0f, 0.0f)))
            Teleport(place.pos, false);
    }
}

void CAdminMenu::DrawWorldTab()
{
    bool canControl = CanControlWorld();
    if (!canControl)
        HostOnlyNote();
    ImGui::TextWrapped("Час і погода хоста автоматично передаються всім гравцям.");

    ImGui::BeginDisabled(!canControl);
    static int hour = 12, minute = 0;
    ImGui::SliderInt("Година", &hour, 0, 23);
    ImGui::SliderInt("Хвилина", &minute, 0, 59);
    if (ImGui::Button("Встановити час", ImVec2(-1.0f, 0.0f)))
    {
        Command<Commands::SET_TIME_OF_DAY>(hour, minute);
        SetStatus("Час: " + std::to_string(hour) + ":" + (minute < 10 ? "0" : "") + std::to_string(minute));
    }

    static int weather = 0;
    ImGui::Combo("Погода", &weather, WEATHER_NAMES, IM_ARRAYSIZE(WEATHER_NAMES));
    if (ImGui::Button("Встановити погоду", ImVec2(-1.0f, 0.0f)))
    {
        Command<Commands::FORCE_WEATHER_NOW>(weather);
        SetStatus(std::string("Погода: ") + WEATHER_NAMES[weather]);
    }
    ImGui::EndDisabled();
}
