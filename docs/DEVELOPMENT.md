# SA Dream Mod — розробка / development

## Репозиторії
| Репозиторій | Що містить |
|---|---|
| [`GuessGames/sa_dream_mod`](https://github.com/GuessGames/sa_dream_mod) | код, інструменти, документація |
| [`GuessGames/sa_dream_mod_coop`](https://github.com/GuessGames/sa_dream_mod_coop) | тільки готові файли для гравців (без файлів Rockstar) |

Remote `upstream` (Tornamic/CoopAndreas) — тільки для читання: push-URL навмисно вимкнено.
Оновлення від автора: `git fetch upstream` + `git merge upstream/main`.

## Збірка
Потрібні VS 2022 Build Tools (C++ x86) і [xmake](https://xmake.io). Збирати з PowerShell або cmd (у Git Bash xmake підхоплює MinGW).

```
xmake f -p windows -a x86 -m release --toolchain=msvc
xmake -y
tools\manager\build.cmd
```

Результат:
- `build\windows\x86\release\` — `CoopAndreasSA.dll`, `proxy.dll` (→ `eax.dll`), `server.exe`;
- `build\manager\CoopAndreasManager.exe` — менеджер розробника;
- `build\manager\SADreamLauncher.exe` — простий лаунчер для гравців (той самий код з `/define:PLAYER`).

## Менеджер розробника
- **Встановлення:**
  - встановлення з локальної збірки, перевірка й виправлення, видалення;
  - «Зібрати з коду»;
  - «Оновити і встановити» (git pull + збірка + встановлення).
- **Опублікувати реліз:**
  1. збирає пакет у `..\release` (клон `sa_dream_mod_coop`);
  2. пише README з посиланням на коміт коду;
  3. комітить і пушить.

  Перед публікацією коміт коду має бути запушений.
- **Сервер:** панель сервера (та сама, що в лаунчері гравця) — запуск/зупинка, гравці онлайн, адреси для друзів, лог. Сервер запускається окремим процесом і працює після закриття менеджера. Окремо: `CoopAndreasManager.exe --server` (ярлик «SA Dream Mod - Server»).
- **Запуск:** звичайна гра, локальний сервер, **тест у два вікна** (сервер + 2 клієнти з `-profile 1/2` і `-autoconnect`).
- **Логи:** `server.log`, `client.log`, `client_1.log`, `client_2.log` у `<гра>\CoopAndreas\logs`, з фільтром і живим оновленням.
- CLI: `CoopAndreasManager.exe --status | --install | --repair | --uninstall | --verify | --update | --assemble <dir> | --publish | --test | --stop` (лог: `%LOCALAPPDATA%\CoopAndreasManager\manager_cli.log`).

## Параметри клієнта
| Параметр | Що робить |
|---|---|
| `-profile N` | окремий конфіг `coopandreas_N.ini` і лог `client_N.log` (два вікна на одному ПК) |
| `--coopd0` / `--coopd1` | ставить вікна поруч |
| `-autoconnect` | сам натискає «почати гру» з ніком/IP з конфігу |
| `-console` | консоль замість лог-файлу |

## Структура
| Шлях | Що це |
|---|---|
| `client/ server/ shared/ proxy/ launcher/` | код моду |
| `scm/` | скомпільований `main.scm` + `script.img` |
| `dist/additional/` | ASI loader, WindowedMode, WidescreenFix, scrlog |
| `tools/manager/` | менеджер і лаунчер |
| `docs/` | документація |
