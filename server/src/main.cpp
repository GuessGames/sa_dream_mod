#include "stdafx.h"

#if defined(_WIN32)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#endif

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <fstream>
#include <deque>
#pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__)
#include <unistd.h>
#endif

#if defined(_WIN32)
// SA Dream Mod: the server had no crash handler. Writes <game>\CoopAndreas_crashes\server_<time>.log (+ .dmp),
// which the launcher picks up and offers to report; the crash watcher symbolizes it with the release PDB.
static LONG WINAPI ServerCrashHandler(EXCEPTION_POINTERS* info)
{
    time_t now = time(nullptr);
    tm t{};
    localtime_s(&t, &now);
    CreateDirectoryA("..\\CoopAndreas_crashes", nullptr);
    char base[MAX_PATH];
    sprintf_s(base, "..\\CoopAndreas_crashes\\server_%04d-%02d-%02d_%02d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec);

    std::ofstream out(std::string(base) + ".log");
    DWORD address = (DWORD)(uintptr_t)info->ExceptionRecord->ExceptionAddress;
    HMODULE module = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(uintptr_t)address, &module);
    char moduleName[MAX_PATH] = "unknown";
    if (module)
        GetModuleFileNameA(module, moduleName, MAX_PATH);
    const char* shortName = strrchr(moduleName, '\\') ? strrchr(moduleName, '\\') + 1 : moduleName;

    char line[512];
    sprintf_s(line, "SA Dream Mod server crash, CoopAndreas %s %s %s\nUnhandled exception 0x%08X at 0x%08X in %s (+0x%08X)\n",
        COOPANDREAS_VERSION, __DATE__, __TIME__, (unsigned)info->ExceptionRecord->ExceptionCode, (unsigned)address, shortName,
        (unsigned)(address - (DWORD)(uintptr_t)module));
    out << line;
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        sprintf_s(line, "Access violation: %s 0x%08X\n", info->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
            (unsigned)info->ExceptionRecord->ExceptionInformation[1]);
        out << line;
    }

    // backtrace as module+offset (symbolized offline with the release PDB)
    out << "\nBacktrace:\n";
    CONTEXT ctx = *info->ContextRecord;
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = ctx.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 32; i++)
    {
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &ctx, nullptr, SymFunctionTableAccess64,
                SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0)
            break;
        HMODULE frameModule = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)(uintptr_t)frame.AddrPC.Offset, &frameModule);
        char frameName[MAX_PATH] = "unknown";
        if (frameModule)
            GetModuleFileNameA(frameModule, frameName, MAX_PATH);
        const char* frameShort = strrchr(frameName, '\\') ? strrchr(frameName, '\\') + 1 : frameName;
        sprintf_s(line, "   0x%08X in %s (+0x%08X)\n", (unsigned)frame.AddrPC.Offset, frameShort,
            (unsigned)(frame.AddrPC.Offset - (DWORD64)(uintptr_t)frameModule));
        out << line;
    }
    SymCleanup(process);

    // what the server did right before
    fflush(stdout);
    std::ifstream log("..\\CoopAndreas\\logs\\server.log");
    std::deque<std::string> last;
    std::string text;
    while (std::getline(log, text))
    {
        last.push_back(text);
        if (last.size() > 60)
            last.pop_front();
    }
    out << "\nLast log lines:\n";
    for (auto& l : last)
        out << "   " << l << "\n";
    out.close();

    HANDLE dump = CreateFileA((std::string(base) + ".dmp").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dump != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION mdei{GetCurrentThreadId(), info, FALSE};
        MiniDumpWriteDump(process, GetCurrentProcessId(), dump,
            (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo), &mdei, nullptr, nullptr);
        CloseHandle(dump);
    }
    printf("[!] : The server crashed, report saved to %s.log\n", base);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

std::filesystem::path GetExecutableDir()
{
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    return std::filesystem::current_path();
#endif
}

int main(int argc, char* argv[])
{
#if defined(_WIN32)
    SetConsoleTitleW(L"CoopAndreas Server");
    SetUnhandledExceptionFilter(ServerCrashHandler);
#endif

    // unbuffered output so the log is visible live when stdout is redirected (manager log view)
    setvbuf(stdout, nullptr, _IONBF, 0);
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--no-colors") == 0)
            logger::ms_bColors = false;
    }

    std::filesystem::path exeDir = GetExecutableDir();
    std::error_code ec;
    std::filesystem::current_path(exeDir, ec);

    printf("[!] : Support:\n");
    printf("- https://github.com/Tornamic/CoopAndreas\n");
    printf("- https://discord.gg/TwQsR4qxVx\n");
    printf("- coopandreasmod@gmail.com\n\n");

    printf("[!] : CoopAndreas Server \n");
#ifdef _DEBUG
    char config[] = "Debug";
#else
    char config[] = "Release";
#endif

    printf("[!] : Version : %s, %s %s %s %s\n", COOPANDREAS_VERSION, config, sizeof(void*) == 8 ? "x64" : "x86",
        __DATE__, __TIME__);
#if defined(_WIN32)
    printf("[!] : Platform : Microsoft Windows \n");
#else
    printf("[!] : Platform : GNU/Linux | BSD \n");
#endif

    CConfigManager::Init();
    CServerTime::Init();

    CNetwork::Init(CConfigManager::GetConfigPort());
    return 0;
}