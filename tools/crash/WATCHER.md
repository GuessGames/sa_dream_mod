# Crash watcher

Collects SA Dream Mod crashes, finds the cause, fixes it and ships the fix. Runs as a scheduled Claude task on the
developer PC (every 30 minutes while the Claude app is open); this file is its procedure.

## Where crashes come from

| Source | What | How it gets here |
|---|---|---|
| Developer PC | `D:\Grand Theft Auto San Andreas\CoopAndreas_crashes\*.log` + `*.dmp` (game) and `server_*.log/.dmp` (server) | written by the crash handlers |
| Friends' PCs | GitHub issues titled `[crash] …` in **GuessGames/sa_dream_mod** | the launcher offers to send a new report as a prefilled issue |

Read issues without a token: `curl -s "https://api.github.com/repos/GuessGames/sa_dream_mod/issues?state=open&per_page=50"` and keep
the ones whose title starts with `[crash]` (the label is only set when the reporter may set labels). The issue body
holds the summary (exception, backtrace, release, last log lines); there is no minidump.

## State

`D:\CoopAndreasDev\crash-watch\` (outside the repos):
* `processed.txt` — one line per handled report file name or `issue#N`; anything listed is skipped.
* `journal.md` — newest entry first: date, reports, signature, cause, fix commit/release or "not fixed: why".

## Procedure

1. List new reports (files and issues not in `processed.txt`). Nothing new → stop, write nothing.
2. Symbolize each: `python D:\CoopAndreasDev\src\tools\crash\analyze.py <report.log>`.
   It uses `D:\CoopAndreasDev\symbols\<release>\` (PDBs archived by every publish) and the `.dmp` next to the log
   (validated return addresses of the whole stack). For an issue, save the body to a temp `.log` file first.
3. Group by signature = crash address + first frames. Check `journal.md`: a signature already fixed in a release
   newer than the report's release is only marked processed.
4. Find the cause in the source at the commit of that release (`symbols\<release>\commit.txt`). Game addresses:
   plugin-sdk names (`third_party/plugin-sdk/plugin_sa/game_sa`), the disassembly helper approach (capstone is installed),
   our hooks that touch that code. Read the last log lines: they usually show what happened.
5. Fix only when the cause is understood. A guard that turns a crash into a logged warning is fine when the root
   cause is in game code (see `client/src/Hooks/CrashfixHooks.cpp`); log it with `[crashfix]`.
6. Build: `xmake build` in `D:\CoopAndreasDev\src` (configured for MSVC x86; reconfigure from PowerShell with
   `xmake f -p windows -a x86 -m release --toolchain=msvc -y` if needed). Build errors → fix or give up (step 9).
7. Test when possible: only if no `gta_sa.exe` runs (somebody plays) — copy the new
   `build\windows\x86\release\CoopAndreasSA.dll` into the game folder and run
   `tools\crash\test_two_windows.ps1 -mission <id> -runSeconds 120` (own server on port 6768, never the user's).
   Mission ids: `client/src/Debug/MissionRunner.cpp`. A reproduced crash must not come back.
8. Ship: commit in `D:\CoopAndreasDev\src` with `Fixes #N` for issues (closes them on push), push `origin main`, then
   publish: `D:\CoopAndreasDev\src\build\manager\CoopAndreasManager.exe --publish` (requires a clean, pushed tree;
   archives the symbols; log in `%LOCALAPPDATA%\CoopAndreasManager\manager_cli.log`). Players get it with "Update".
9. Not fixable now → no code change; write the analysis to `journal.md` so the developer sees it.
10. Add the handled reports to `processed.txt`, add the journal entry.

## Rules

* Never push to the upstream Tornamic/CoopAndreas repository; only `GuessGames/sa_dream_mod` (source) and the
  release repo through `--publish`.
* Never publish Rockstar files (`gta_sa.exe`, `vorbisHooked.dll`); `--publish` already excludes them.
* Never stop or restart a server or game you did not start; never send input to the desktop.
* Never use or ask for tokens or passwords. No GitHub comments (no auth): the commit message closes issues.
* Keep fixes small and in the style of the surrounding code; do not refactor while fixing a crash.
* One crash = one commit, with the crash signature and the cause in the message.
