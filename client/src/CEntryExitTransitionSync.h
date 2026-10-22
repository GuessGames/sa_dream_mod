#pragma once

class CEntryExit;
class CPed;

namespace Packets::Players
{
class EnExTransition;
}

class CEntryExitTransitionSync
{
public:
    static void OnTransitionStarted(CEntryExit* pEntryExit, CPed* pPed);
    static void OnTransitionFinished(CEntryExit* pEntryExit, CPed* pPed);
    static void Receive(const Packets::Players::EnExTransition& packet);
    static void Process();
    // the local player is walking through an enex door
    static bool IsLocalTransitionActive() { return ms_pLocalAnimatedTransition != nullptr; }

private:
    static inline CEntryExit* ms_pLocalAnimatedTransition = nullptr;
};
