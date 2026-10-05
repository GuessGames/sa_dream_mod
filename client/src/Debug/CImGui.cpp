#include "CImGui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx9.h"
#include "CAdminMenu.h"

ImFont* pFont;

void InitFonts()
{
    char windowsDir[MAX_PATH];
    GetWindowsDirectoryA(windowsDir, MAX_PATH);
    std::string fontsDir = std::string(windowsDir) + "\\Fonts\\";

    ImGuiIO& io = ImGui::GetIO();
    pFont = nullptr;

    std::string segoePath = fontsDir + "segoeui.ttf";
    if (FileExists(segoePath.c_str()))
    {
        pFont = io.Fonts->AddFontFromFileTTF(segoePath.c_str(), 16.0f);
    }

    if (!pFont)
    {
        pFont = io.Fonts->AddFontDefault();
    }
}

void InitStyles()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);

    style.Colors[ImGuiCol_WindowBg] = ImColor(13, 19, 33, 255);
    style.Colors[ImGuiCol_ChildBg] = ImColor(24, 31, 47, 255);
    style.ChildRounding = 3.0f;
}

// while the admin/debug menu is open the mouse belongs to it: no camera turning, shooting or weapon switching
// (the keyboard still moves the player). CPad::UpdateMouse fills the PC mouse state at 0xB73404.
static void __fastcall CPad__UpdateMouse_Hook(CPad* This)
{
    plugin::CallMethod<0x53F3C0, CPad*>(This);
    if (CImGui::ms_bActive && !FrontEndMenuManager.m_bMenuActive)
        memset(reinterpret_cast<void*>(0xB73404), 0, 0x14);
}

void CImGui::SetActive(bool bActive)
{
    ms_bActive = bActive;

    if (ms_bActive)
    {
        CPad::GetPad(0)->DisablePlayerControls |= 0x200;
    }
    else
    {
        CPad::GetPad(0)->DisablePlayerControls &= ~0x200;
    }

    static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice())->ShowCursor(ms_bActive);
    ImGui::GetIO().MouseDrawCursor = ms_bActive;
}

void CImGui::UpdateActive()
{
    bool bActive = CAdminMenu::ms_bOpen;
    if (bActive != ms_bActive)
        SetActive(bActive);
}

void CImGui::Init()
{
    CAdminMenu::Init();
    patch::RedirectCall(0x541DD7, CPad__UpdateMouse_Hook);

    Events::initRwEvent += []
    {
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(RsGlobal.ps->window);
        ImGui_ImplWin32_EnableDpiAwareness();

        ImGui_ImplDX9_Init(static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice()));

        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr;

        InitStyles();
        InitFonts();

        ImGui_ImplDX9_InvalidateDeviceObjects();
        ImGui_ImplDX9_CreateDeviceObjects();
    };

    Events::drawAfterFadeEvent += []
    {
        if (CImGui::ms_bActive && !FrontEndMenuManager.m_bMenuActive)
        {
            // readable on any resolution: 1.0 at 1080p, ~1.33 at 1440p, 2.0 at 4K
            ImGui::GetStyle().FontScaleMain = std::max(1.0f, RsGlobal.maximumHeight / 1080.0f);

            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
            {
                CPad::GetPad(0)->DisablePlayerControls |= 0x200;
            }
            else
            {
                CPad::GetPad(0)->DisablePlayerControls &= ~0x200;
            }

            ImGui::PushFont(pFont);
            CAdminMenu::DrawUI();
            ImGui::PopFont();

            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

            ImGui_ImplDX9_InvalidateDeviceObjects();
        }
    };
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CImGui::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
}
