#include "stdafx.h"
#include "CCutsceneSkipVote.h"
#include <set>
#include <CCutsceneMgr.h>
#include "CHudColours.h"

// differences from upstream: works offline (the game's own skip), AFK/paused players don't block the skip
// (they can't press the button), the host repeats the state so a late cutscene start shows the right count

namespace
{
// shared state, shown to everyone
int g_nVotes = 0;
int g_nVoters = 0;
bool g_bVoted = false;

// host only
bool g_bSkipped = false;
std::set<int> g_voters;  // the players connected when the cutscene started (and the host)
std::set<int> g_votes;
uint32_t g_nLastBroadcast = 0;

bool IsActiveVoter(int playerid)
{
    if (playerid == CNetworkPlayerManager::m_nMyId)
        return true;

    CNetworkPlayer* player = CNetworkPlayerManager::GetPlayer(playerid);
    return player != nullptr && !player->m_bPaused && !player->m_bAfk;
}

// host: recount, tell everyone when it changed (or every second), true when everyone active voted
bool UpdateHostState()
{
    int votes = 0;
    int voters = 0;
    for (int playerid : g_voters)
    {
        if (!IsActiveVoter(playerid))
            continue;
        voters++;
        if (g_votes.count(playerid))
            votes++;
    }

    if (votes != g_nVotes || voters != g_nVoters || GetTickCount() - g_nLastBroadcast > 1000)
    {
        g_nVotes = votes;
        g_nVoters = voters;
        g_nLastBroadcast = GetTickCount();

        Packets::Scripts::CutsceneSkipVoteState packet{};
        packet.votes = votes;
        packet.voters = voters;
        GetPacketFactory().Send(packet);
    }

    return votes > 0 && votes == voters;
}

bool CCutsceneMgr__IsCutsceneSkipButtonBeingPressed_Hook()
{
    bool bPressed = plugin::CallAndReturn<bool, 0x4D5D10>();  // CCutsceneMgr::IsCutsceneSkipButtonBeingPressed

    if (!CNetwork::m_bAuthenticated)
        return bPressed;

    if (bPressed && !g_bVoted)
    {
        g_bVoted = true;
        if (CLocalPlayer::m_bIsHost)
        {
            g_votes.insert(CNetworkPlayerManager::m_nMyId);
        }
        else
        {
            GetPacketFactory().Send(Packets::Scripts::CutsceneSkipVote{});
        }
        logger::info("[mission] voted to skip the cutscene");
    }

    // the others skip when the host's script clears the cutscene
    if (!CLocalPlayer::m_bIsHost)
        return false;

    if (!g_bSkipped && UpdateHostState())
    {
        g_bSkipped = true;
        logger::info("[mission] everyone voted, skipping the cutscene (%d/%d)", g_nVotes, g_nVoters);
    }
    return g_bSkipped;
}

void CCutsceneMgr__StartCutscene_Hook()
{
    g_nVotes = 0;
    g_nVoters = 0;
    g_bVoted = false;
    g_bSkipped = false;
    g_votes.clear();
    g_voters.clear();
    g_nLastBroadcast = 0;

    if (CNetwork::m_bAuthenticated && CLocalPlayer::m_bIsHost)
    {
        for (auto* player : CNetworkPlayerManager::m_pPlayers)
        {
            g_voters.insert(player->m_iPlayerId);
        }
        g_voters.insert(CNetworkPlayerManager::m_nMyId);
        UpdateHostState();
    }

    CCutsceneMgr::StartCutscene();
}

uintptr_t DrawWidescreen_ptr = 0x0;
void* DrawWidescreen_Hook(int a1)
{
    if (CNetwork::m_bAuthenticated && CCutsceneMgr::ms_running && g_nVoters > 0 && (g_nVotes > 0 || g_bVoted))
    {
        char szText[64];
        sprintf_s(szText, "Skip? (%d/%d)%s", g_nVotes, g_nVoters, g_bVoted ? " - Voted" : "");

        CFont::SetOrientation(eFontAlignment::ALIGN_RIGHT);
        CFont::SetEdge(1);
        CFont::SetColor(HudColour.GetRGBA(HUD_COLOUR_GREY));
        CFont::SetScale(CUtil::HUD_X(0.3f), CUtil::HUD_Y(0.4f));
        CFont::SetProportional(true);
        CFont::SetBackground(false, false);
        CFont::SetFontStyle(FONT_SUBTITLES);
        CFont::SetDropColor(CRGBA(0, 0, 0, 255));
        CFont::SetJustify(false);

        CFont::PrintString(CUtil::HUD_X(640.0f - 10.0f), CUtil::HUD_Y(448.0f - 32.0f), szText);
    }

    return plugin::CallAndReturnDyn<void*, int>(DrawWidescreen_ptr, a1);
}
}  // namespace

PACKET_HANDLER(ePacketType::CUTSCENE_SKIP_VOTE, Packets::Scripts::CutsceneSkipVote* pCutsceneSkipVote)
{
    if (CLocalPlayer::m_bIsHost && g_voters.count(pCutsceneSkipVote->senderPlayerid.value))
    {
        g_votes.insert(pCutsceneSkipVote->senderPlayerid.value);
    }
}

PACKET_HANDLER(ePacketType::CUTSCENE_SKIP_VOTE_STATE, Packets::Scripts::CutsceneSkipVoteState* pCutsceneSkipVoteState)
{
    g_nVotes = pCutsceneSkipVoteState->votes;
    g_nVoters = pCutsceneSkipVoteState->voters;
}

void CCutsceneSkipVote::InjectHooks()
{
    patch::RedirectCall(0x48072B, CCutsceneMgr__StartCutscene_Hook);
    patch::RedirectCall(0x5B1947, CCutsceneMgr__IsCutsceneSkipButtonBeingPressed_Hook);

    DrawWidescreen_ptr = injector::GetBranchDestination(0x53E2BC).as_int();
    patch::RedirectCall(0x53E2BC, DrawWidescreen_Hook);
}
