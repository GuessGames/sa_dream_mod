#pragma once

// a cutscene is skipped when every active player pressed the skip button (port of upstream e1765f3).
// The host counts the votes and skips, the others never skip on their own: the host's CLEAR_CUTSCENE reaches them.
class CCutsceneSkipVote
{
public:
    static void InjectHooks();
};
