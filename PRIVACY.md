# Privacy / Приватність

SA Dream Mod does not collect, track or send any data in the background.
SA Dream Mod нічого не збирає, не відстежує і не відправляє у фоні.

## What the mod sends / Що мод відправляє
* **To the game server you join** — what co-op needs: your nickname, position, actions, vehicles, chat. Nothing else.
  The server keeps no logs beyond its own console log on the host's PC.
* **No beta keys, no PC IDs, no accounts.** The upstream beta key system (a key bound to a PC ID handed out by a Discord bot)
  was removed.
* **No Discord Rich Presence**, no analytics; crash reports are sent only with your consent (see below).

## Crash reports / Звіти про збої
When the game or the server crashes, a report (`*.log`) and a small memory dump (`*.dmp`) are saved **only on your PC** in
`<game folder>\CoopAndreas_crashes`. They contain technical data: versions, the error address, registers, the call stack,
loaded modules, the last lines of the mod's log (nicknames of players and positions can appear there).

The first time the game crashes, the launcher **asks** whether reports may be sent automatically.
* **Yes:** from then on the launcher sends each new report (the `.log` and the small `.dmp`, plus your nickname and the mod
  version) to the developer's **private** storage. It is not published anywhere. You can turn it off by setting
  `AutoSendCrashes=0` in `%LOCALAPPDATA%\CoopAndreasManager\CoopLauncher.ini`.
* **No:** the launcher asks each time; if you agree, a GitHub page opens with the report filled in, and it is published
  only when **you** press "Submit new issue" with your own GitHub account.
Nothing is sent without your consent.

## The launcher / Лаунчер
* Downloads the mod only from the public release repository on GitHub (`GuessGames/sa_dream_mod_coop`).
* The server panel asks `api.ipify.org` for your public IP address (to show it to you as an address for friends).
* Settings are stored locally in `%LOCALAPPDATA%\CoopAndreasManager`.
