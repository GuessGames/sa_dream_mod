#include "stdafx.h"
#include "CAdminMenu.h"
#include "CImGui.h"
#include "MissionRunner.h"
#include "CPacketTimeline.h"
#include <imgui.h>
#include <CCheat.h>
#include <CDebugVehicleSpawner.h>
#include <fstream>
#include <sstream>

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
// mission cleanup only "releases" mission cars, so a restarted mission would add a second one next to the old
struct SLeftoverVehicle
{
    CVehicle* vehicle;
    int ref;
};
std::vector<SLeftoverVehicle> leftoverVehicles;

void RememberMissionVehicles()
{
    leftoverVehicles.clear();
    for (auto* networkVehicle : CNetworkVehicleManager::m_pVehicles)
    {
        CVehicle* vehicle = networkVehicle->m_pVehicle;
        if (networkVehicle->m_bSyncing && vehicle && networkVehicle->m_nCreatedBy == MISSION_VEHICLE)
            leftoverVehicles.push_back({vehicle, CPools::GetVehicleRef(vehicle)});
    }
}

void RemoveLeftoverMissionVehicles()
{
    int removed = 0;
    for (auto& v : leftoverVehicles)
    {
        // still the same, empty vehicle: nobody is driving or sitting in it
        if (CPools::GetVehicle(v.ref) != v.vehicle || v.vehicle->m_pDriver || v.vehicle->m_nNumPassengers > 0)
            continue;
        Command<Commands::DELETE_CAR>(v.ref);
        removed++;
    }
    leftoverVehicles.clear();
    if (removed)
        logger::info("[admin] removed %d leftover mission vehicle(s)", removed);
}

void ReplaceMission(int id)
{
    if (IsMissionActive())
    {
        RememberMissionVehicles();
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

void Teleport(CVector pos, bool findGround, int interior = -1)
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    int ref = CPools::GetPedRef(ped);
    if (interior >= 0 && interior != CGame::currArea)
    {
        // cars do not belong in interiors: a passenger or a driver going inside leaves the car
        if (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle && (interior != 0 || ped->m_pVehicle->m_pDriver != ped))
            Command<Commands::WARP_CHAR_FROM_CAR_TO_COORD>(ref, pos.x, pos.y, pos.z);
        Command<Commands::SET_AREA_VISIBLE>(interior);
        Command<Commands::SET_CHAR_AREA_VISIBLE>(ref, interior);
    }
    else if (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle && ped->m_pVehicle->m_pDriver != ped)
    {
        Command<Commands::WARP_CHAR_FROM_CAR_TO_COORD>(ref, pos.x, pos.y, pos.z);
    }

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

// ------------------------------------------------------------------ wardrobe: everything the shops sell
// read from the game's own data\shopping.dat (clothes, haircuts and tattoos price sections)
struct SClothesItem
{
    std::string texture, model, label;
    int part;
};
std::vector<SClothesItem> clothesItems;
bool bClothesLoaded = false;

std::string GxtToString(const char* text)
{
    std::string out;
    for (const char* c = text; c && *c; c++)
    {
        if (*c == '~')  // skip ~x~ colour codes
        {
            const char* end = strchr(c + 1, '~');
            if (!end)
                break;
            c = end;
            continue;
        }
        out += (*c >= 32 && *c < 127) ? *c : '?';
    }
    return out;
}

void LoadClothes()
{
    bClothesLoaded = true;
    std::ifstream file("data\\shopping.dat");
    std::string line;
    int mode = 0;  // 1 clothes, 2 haircuts, 3 tattoos
    bool inPrices = false;
    while (std::getline(file, line))
    {
        size_t hash = line.find('#');
        if (hash != std::string::npos)
            line.resize(hash);
        std::istringstream in(line);
        std::string first;
        if (!(in >> first))
            continue;

        if (first == "section")
        {
            std::string name;
            in >> name;
            if (name == "prices") inPrices = true;
            else if (name == "shops") inPrices = false;
            else if (inPrices) mode = name == "Clothes" ? 1 : name == "Haircuts" ? 2 : name == "Tattoos" ? 3 : 0;
            continue;
        }
        if (first == "end")
        {
            mode = 0;
            continue;
        }
        if (!mode)
            continue;

        SClothesItem item;
        std::string nametag, model, type;
        item.texture = first;
        if (mode == 3)
        {
            if (!(in >> nametag >> type))
                continue;
        }
        else
        {
            if (!(in >> nametag >> model >> type))
                continue;
            item.model = model;
        }
        item.part = atoi(type.c_str());
        if (item.part < 0 || item.part > 17)
            continue;
        std::string label = GxtToString(TheText.Get(nametag.c_str()));
        item.label = label.empty() || label == nametag ? item.texture : label + "  (" + item.texture + ")";
        clothesItems.push_back(item);
    }
    // the default face/hair is not sold anywhere
    clothesItems.push_back({"player_face", "head", "Default hair (player_face)", 1});
    logger::info("[admin] wardrobe: %d items from shopping.dat", (int)clothesItems.size());
}

void ApplyClothes(const char* texture, const char* model, int part)
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped || !ped->m_pPlayerData || !ped->m_pPlayerData->m_pPedClothesDesc)
        return;
    ped->m_pPlayerData->m_pPedClothesDesc->SetTextureAndModel(texture, model, part);
    // the rebuild sends the new look to the other players (CPed::Dress hook)
    CClothes::RebuildPlayer(ped, false);
    SetStatus(texture ? std::string("Вдягнено: ") + texture : "Знято");
}

void CAdminMenu::TeleportLocalPlayer(CVector pos, int interior)
{
    Teleport(pos, false, interior);
}

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

    // -testmission N: started by the host 15 s after connecting
    static uint32_t authenticatedAt = 0;
    if (!CNetwork::m_bAuthenticated)
        authenticatedAt = 0;
    else if (!authenticatedAt)
        authenticatedAt = GetTickCount();
    if (CCore::ms_bTestOutfit && authenticatedAt && GetTickCount() - authenticatedAt > 5000 && !ped->m_nPedFlags.bInVehicle)
    {
        CCore::ms_bTestOutfit = false;
        CStats::SetStatValue(STAT_FAT, 995.0f);
        CStats::SetStatValue(STAT_MUSCLE, 10.0f);
        CPedClothesDesc* desc = ped->m_pPlayerData->m_pPedClothesDesc;
        desc->SetTextureAndModel("player_torso", "torso", 0);
        desc->SetTextureAndModel("afrotash", "afro", 1);
        desc->SetTextureAndModel("tracktrwhstr", "tracktr", 2);
        desc->SetTextureAndModel("sandalsock", "flipflop", 3);
        desc->SetTextureAndModel("11grove3", nullptr, 11);
        desc->SetTextureAndModel("neckropeg", "neck2", 13);
        desc->SetTextureAndModel("watchgno", "watch", 14);
        desc->SetTextureAndModel("groucho", "grouchos", 15);
        CClothes::RebuildPlayer(ped, false);
        SetStatus("-testoutfit: test clothes and fat 995 applied");
    }

    if (CCore::ms_nTestMission >= 0 && authenticatedAt && CLocalPlayer::m_bIsHost && GetTickCount() - authenticatedAt > 15000 &&
        !IsMissionActive() && !FrontEndMenuManager.m_bMenuActive)
    {
        int id = CCore::ms_nTestMission;
        CCore::ms_nTestMission = -1;
        LaunchMission(id);
    }

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
            RemoveLeftoverMissionVehicles();
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
            if (ImGui::BeginTabItem("Гардероб")) { DrawWardrobeTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Світ")) { DrawWorldTab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Розробка"))
            {
                // network debugging for developers (was in the upstream D1212 debug window)
                static bool bPacketTimeline = false;
                ImGui::Checkbox("Packet timeline", &bPacketTimeline);
                if (bPacketTimeline)
                    CPacketTimeline::DrawUI();
                ImGui::EndTabItem();
            }
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

    ImGui::SeparatorText("Статура (бачать усі гравці)");
    static float fat = -1.0f, muscle = -1.0f;
    if (fat < 0.0f || ImGui::Button("Взяти з гри", ImVec2(-1.0f, 0.0f)))
    {
        fat = CStats::GetStatValue(STAT_FAT);
        muscle = CStats::GetStatValue(STAT_MUSCLE);
    }
    ImGui::SliderFloat("Жир", &fat, 0.0f, 1000.0f, "%.0f");
    ImGui::SliderFloat("М'язи", &muscle, 0.0f, 1000.0f, "%.0f");
    struct SBody { const char* name; float fat, muscle; };
    static const SBody bodies[] = {{"Худий", 0.0f, 0.0f}, {"Звичайний", 200.0f, 200.0f}, {"М'язистий", 0.0f, 1000.0f}, {"Товстий", 1000.0f, 0.0f}};
    float w4 = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 3.0f) / 4.0f;
    for (int i = 0; i < 4; i++)
    {
        if (i) ImGui::SameLine();
        if (ImGui::Button(bodies[i].name, ImVec2(w4, 0.0f)))
        {
            fat = bodies[i].fat;
            muscle = bodies[i].muscle;
        }
    }
    bool inVehicle = ped->m_nPedFlags.bInVehicle;
    ImGui::BeginDisabled(inVehicle);
    if (ImGui::Button("Застосувати статуру", ImVec2(-1.0f, 0.0f)))
    {
        CStats::SetStatValue(STAT_FAT, fat);
        CStats::SetStatValue(STAT_MUSCLE, muscle);
        CClothes::RebuildPlayer(ped, false);
        SetStatus("Статура: жир " + std::to_string((int)fat) + ", м'язи " + std::to_string((int)muscle));
    }
    ImGui::EndDisabled();
    if (inVehicle)
        ImGui::TextDisabled("Статуру й одяг можна змінити, коли ви не в транспорті");
}

void CAdminMenu::DrawWardrobeTab()
{
    CPlayerPed* ped = FindPlayerPed(0);
    if (!ped)
        return;
    if (!bClothesLoaded)
        LoadClothes();

    struct SPart { const char* name; int part; bool removable; };
    static const SPart parts[] = {
        {"Торс", 0, false}, {"Зачіска", 1, false}, {"Ноги", 2, false}, {"Взуття", 3, false},
        {"Ланцюжок", 13, true}, {"Годинник", 14, true}, {"Окуляри", 15, true}, {"Головний убір", 16, true},
        {"Костюм", 17, true}, {"Тату: ліве плече", 4, true}, {"Тату: ліве передпліччя", 5, true},
        {"Тату: праве плече", 6, true}, {"Тату: праве передпліччя", 7, true}, {"Тату: спина", 8, true},
        {"Тату: груди зліва", 9, true}, {"Тату: груди справа", 10, true}, {"Тату: живіт", 11, true},
        {"Тату: поперек", 12, true},
    };
    static int selected = 0;
    static char filter[32] = "";

    bool inVehicle = ped->m_nPedFlags.bInVehicle;
    if (inVehicle)
        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Вийдіть з транспорту, щоб перевдягнутися");
    ImGui::TextWrapped("Усе, що продають магазини одягу, перукарні й тату-салони. Інші гравці бачать зміни.");

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::BeginCombo("##part", parts[selected].name))
    {
        for (int i = 0; i < IM_ARRAYSIZE(parts); i++)
            if (ImGui::Selectable(parts[i].name, i == selected))
                selected = i;
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##clothes_filter", "Пошук", filter, sizeof(filter));

    const SPart& part = parts[selected];
    ImGui::BeginDisabled(inVehicle);
    if (part.removable && ImGui::Button("Зняти", ImVec2(-1.0f, 0.0f)))
        ApplyClothes(nullptr, nullptr, part.part);

    ImGui::BeginChild("##clothes", ImVec2(0.0f, 0.0f), true);
    unsigned int current = ped->m_pPlayerData && ped->m_pPlayerData->m_pPedClothesDesc ? ped->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[part.part] : 0;
    int shown = 0;
    for (const auto& item : clothesItems)
    {
        if (item.part != part.part || !ContainsNoCase(item.label.c_str(), filter))
            continue;
        shown++;
        bool worn = current && CKeyGen::GetUppercaseKey(item.texture.c_str()) == current;
        ImGui::PushID(&item);
        if (ImGui::Selectable(item.label.c_str(), worn))
            ApplyClothes(item.texture.c_str(), item.model.empty() ? nullptr : item.model.c_str(), item.part);
        ImGui::PopID();
    }
    if (!shown)
        ImGui::TextDisabled(clothesItems.empty() ? "Не вдалося прочитати data\\shopping.dat" : "Нічого не знайдено");
    ImGui::EndChild();
    ImGui::EndDisabled();
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

    ImGui::SeparatorText("Гравці");
    bool any = false;
    for (auto* player : CNetworkPlayerManager::m_pPlayers)
    {
        if (!player || !player->m_pPed)
            continue;
        any = true;
        ImGui::PushID(player->m_iPlayerId);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(player->GetName().c_str());
        if (player->m_bPaused || player->m_bAfk)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "AFK");
        }
        float bw = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (ImGui::Button("До нього", ImVec2(bw, 0.0f)))
        {
            CVector pos = player->m_pPed->GetPosition();
            pos.x += 2.0f;
            Teleport(pos, false, player->m_pPed->m_nAreaCode);
        }
        ImGui::SameLine();
        if (ImGui::Button("До себе", ImVec2(bw, 0.0f)))
        {
            Packets::Players::PlayerBring packet{};
            packet.targetid = player->m_iPlayerId;
            packet.pos = ped->GetPosition() + ped->GetForward() * 2.0f;
            packet.interior = ped->m_nAreaCode;
            GetPacketFactory().Send(packet);
            SetStatus(player->GetName() + " переміщується до вас");
        }
        ImGui::PopID();
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
