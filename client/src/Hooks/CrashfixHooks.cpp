#include "stdafx.h"
#include "CrashfixHooks.h"

// TODO: find the cause of those crashes, cuz they dont appear in the vanilla game

// i hope it will work
void __declspec(naked) CVehicleAnimGroup__ComputeAnimDoorOffsets_Hook()
{
    __asm
    {
        // ensure door id < 16
        mov edx, [esp + 8] // get door id
        cmp edx, 16 // compare
        jb cont // if lower than 16, good, exit the hook 

        // zero if out of range
        mov edx, 0
        mov[esp + 8], edx

        cont:
        // code cave
        mov     edx, [esp + 8]
        lea     eax, [edx + edx * 2]

        push 0x6E3D17
        ret
    }
}

static void __declspec(naked) CAnimManager__BlendAnimation_Hook()
{
    __asm
    {
        mov eax, [esp + 4] // get RpClump*
        test eax, eax // check for nullptr

        jnz cont // jump if ok

        retn // exit if not ok

        cont:
        // code cave
        sub esp, 0x14
        mov ecx, [esp + 0x14 + 0x4]

        push 0x4D4617
        ret
    }
}

// CAEStreamThread (radio / music streams): "play position % track length" with a length of 0 for a stream that
// is not open yet -> integer division by zero at 0x4F1464 (seen 4 times in crash reports). Position 0 instead.
static void __declspec(naked) CAEStreamThread__Service_DivZero_Hook()
{
    __asm
    {
        mov ecx, eax
        mov eax, [esp + 0x1C]
        xor edx, edx
        test ecx, ecx
        jz skip
        div ecx
    skip:
        push 0x4F1466
        ret
    }
}

// A script (or the population) creates a car whose model was streamed out after the script checked it: loading a
// cutscene frees memory, so a mission's CREATE_CAR right after the cutscene crashed in
// CVehicleModelInfo::CreateInstance (0x4C9691). The model is loaded again first.
static CVehicle* __cdecl CCarCtrl__GetNewVehicleDependingOnCarModel_Hook(int modelId, uint8_t createdBy)
{
    if (modelId > 0 && modelId < 20000 && CStreaming::ms_aInfoForModel[modelId].m_nLoadState != LOADSTATE_LOADED)
    {
        logger::warn("[crashfix] vehicle model %d is not loaded, loading it before creating the vehicle", modelId);
        unsigned char oldFlags = CStreaming::ms_aInfoForModel[modelId].m_nFlags;
        CStreaming::RequestModel(modelId, GAME_REQUIRED);
        CStreaming::LoadAllRequestedModels(false);
        if (!(oldFlags & GAME_REQUIRED))
            CStreaming::SetModelIsDeletable(modelId);
        if (CStreaming::ms_aInfoForModel[modelId].m_nLoadState != LOADSTATE_LOADED)
        {
            logger::error("[crashfix] vehicle model %d could not be loaded, the vehicle is not created", modelId);
            return nullptr;
        }
    }
    return plugin::CallAndReturn<CVehicle*, 0x421440, int, uint8_t>(modelId, createdBy);
}

void CrashfixHooks::InjectHooks()
{
    patch::RedirectJump(0x4F145C, CAEStreamThread__Service_DivZero_Hook);
    patch::RedirectCall(0x42C345, CCarCtrl__GetNewVehicleDependingOnCarModel_Hook);
    patch::RedirectCall(0x42C7D7, CCarCtrl__GetNewVehicleDependingOnCarModel_Hook);
    patch::RedirectCall(0x4306A1, CCarCtrl__GetNewVehicleDependingOnCarModel_Hook);
    patch::RedirectCall(0x43210E, CCarCtrl__GetNewVehicleDependingOnCarModel_Hook);
    patch::RedirectJump(0x6E3D10, CVehicleAnimGroup__ComputeAnimDoorOffsets_Hook);
    patch::RedirectJump(0x4D4610, CAnimManager__BlendAnimation_Hook);
}
