#include "stdafx.h"
#include "CCrashLog.h"
#include "Commands/CCustomCommandRegistrar.h"
#include "CDiscordRPC.h"
#include <COpCodeSync.h>
#include <CCustomMenuManager.h>
#include <winuser.h>
#include <CLaunchManager.h>
#include <Debug/CImGui.h>

semver_t CCore::Version;

WNDPROC prevWndProc;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CChat::WndProc(hWnd, message, wParam, lParam);
    CCustomMenuManager::WndProc(hWnd, message, wParam, lParam);
    if (CImGui::WndProc(hWnd, message, wParam, lParam))
    {
        return TRUE;
    }
    return CallWindowProcW(prevWndProc, hWnd, message, wParam, lParam);
}

void InitWndProc()
{
    HWND hWnd = *(HWND*)0xC97C1C;

    SetWindowTextW(hWnd, L"CoopAndreas");
    RsGlobal.appName = "CoopAndreas";

    if (hWnd)
    {
        prevWndProc = (WNDPROC)GetWindowLongW(hWnd, GWL_WNDPROC);
        SetWindowLongW(hWnd, GWL_WNDPROC, (LONG)WindowProc);
        return;
    }
}

uintptr_t WinMain_AfterWindowInit_ptr = 0x0;
void WinMain_AfterWindowInit()
{
    InitWndProc();
    plugin::CallDyn(WinMain_AfterWindowInit_ptr);
}

void CCore::Init()
{
    CCrashLog::ms_lpPreviousFilter = SetUnhandledExceptionFilter(CCrashLog::ExceptionHandler);

    LPTSTR cmd = GetCommandLine();
    if (strstr(cmd, "--coopd0") != nullptr)
    {
        ms_bDebug = true;
        ms_nRunIndex = 0;
    }
    if (strstr(cmd, "--coopd1") != nullptr)
    {
        ms_bDebug = true;
        ms_nRunIndex = 1;
    }

    // -profile N: separate config (nickname/ip) and log file, needed to run 2 instances on one PC
    if (const char* profileArg = strstr(cmd, "-profile "))
    {
        ms_nProfile = std::clamp(atoi(profileArg + 9), 0, 9);
    }

    ms_bAutoConnect = strstr(cmd, "-autoconnect") != nullptr;

    gvm.Detect();
    // log to CoopAndreas\logs\client[_N].log (read by the manager); -console shows a console window instead
    if (strstr(cmd, "-console") != nullptr)
        CCore::AllocateConsole();
    else
        CCore::RedirectOutputToLogFile();
    CImGui::Init();
    CLaunchManager::CollectCommandLineArgs();
    WinMain_AfterWindowInit_ptr = injector::GetBranchDestination(0x748995).as_int();
    patch::RedirectCall(0x748995, WinMain_AfterWindowInit);
    CCustomMenuManager::Init();
    CPatch::ApplyPatches();
    CHook::Init();
    CCustomCommandRegistrar::Register();
    CDXFont::Init();
    CDiscordRPC::Init();
    COpCodeSync::Init();
    Events::initGameEvent.after += []
    {
        CPatch::TemporaryPatches();
        // init CNetworking async
        CreateThread(NULL, NULL, CNetwork::InitAsync, NULL, NULL, NULL);
    };
    Events::initRwEvent += [] { CPatch::PatchFramerate(); };

    /*Events::initScriptsEvent.after += []
    {
        CPatch::TemporaryPatches();
    };*/
    gameShutdownEvent.before += []
    {
        // disconnect from server
        CDiscordRPC::Destroy();
        CNetwork::Disconnect();
    };
    semver_parse(COOPANDREAS_VERSION, &CCore::Version);
}

std::string CCore::GetProfileSuffix()
{
    return ms_nProfile > 0 ? "_" + std::to_string(ms_nProfile) : "";
}

void CCore::RedirectOutputToLogFile()
{
    CreateDirectoryA("CoopAndreas", nullptr);
    CreateDirectoryA("CoopAndreas\\logs", nullptr);

    std::string path = "CoopAndreas\\logs\\client" + GetProfileSuffix() + ".log";
    std::string oldPath = "CoopAndreas\\logs\\client" + GetProfileSuffix() + ".old.log";
    MoveFileExA(path.c_str(), oldPath.c_str(), MOVEFILE_REPLACE_EXISTING);

    if (freopen(path.c_str(), "w", stdout))
    {
        setvbuf(stdout, nullptr, _IONBF, 0);
        logger::ms_bColors = false;
        logger::info("CoopAndreas %s client log, profile %d", COOPANDREAS_VERSION, ms_nProfile);
    }
}

void CCore::AllocateConsole()
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);

    DWORD dwMode = 0;
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hOutput, &dwMode);
    dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOutput, dwMode);

    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_SHOWMINIMIZED);
}
