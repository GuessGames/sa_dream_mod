# CoopAndreas — наша приватна збірка / our private build

Приватний форк [Tornamic/CoopAndreas](https://github.com/Tornamic/CoopAndreas) (GPL-3.0) з виправленнями синхронізації та менеджером встановлення.
Оригінальний README — [README.md](README.md). Аналіз проблем і план — [docs/SYNC_ANALYSIS.md](docs/SYNC_ANALYSIS.md).

## Для гравця
1. Встановіть [Git for Windows](https://git-scm.com/download/win) (потрібен для оновлень).
2. Склонуйте цей репозиторій (git clone https://github.com/GuessGames/sa_dream_mod.git) (наприклад, у `D:\CoopAndreasDev\src`) і запустіть `dist\CoopAndreasManager.exe`.
3. На вкладці **Шляхи** вкажіть теку гри і архів `additional.zip` (у ньому є `gta_sa.exe` 1.0 US, у git його немає).
4. **Встановлення → Встановити / оновити файли.** Якщо тека гри захищена, менеджер попросить права адміністратора.
5. **Запуск:** серійний ключ (`/gen <ID>` у Discord CoopAndreas), нік, IP, кнопка «Запустити гру».
6. Оновлення: **Встановлення → Оновити (git pull) і встановити**. Оновлення беруться **тільки** з цього репозиторію (`origin`).

## Для розробника
- Збірка: `xmake f -p windows -a x86 -m release --toolchain=msvc`, потім `xmake -y` (з PowerShell/cmd; потрібні VS 2022 Build Tools і xmake).
- Менеджер: `tools\manager\build.cmd` → `dist\CoopAndreasManager.exe`.
- Перемикач «Ставити мою локальну збірку» встановлює з `build\windows\x86\release` замість `dist\bin`.
- **Опублікувати збірку** копіює збірку в `dist\`, комітить і пушить в `origin`, і гравці отримують її через «Оновити».
- Remote `upstream` (Tornamic) — тільки для читання: push-URL навмисно вимкнено. Оновлення від автора: `git fetch upstream` + `git merge upstream/main`.

## Структура
| Шлях | Що це |
|---|---|
| `client/ server/ shared/ proxy/ launcher/` | код моду |
| `scm/` | скомпільований `main.scm` + `script.img` |
| `dist/bin/` | готові бінарники для гравців |
| `dist/additional/` | ASI loader, WindowedMode, WidescreenFix, scrlog (без `gta_sa.exe`) |
| `dist/CoopAndreasManager.exe` | менеджер |
| `tools/manager/` | код менеджера |
| `docs/` | документація |

## Тест у два вікна на одному ПК
**Запуск → Сервер + 2 вікна гри.** Кожне вікно має свій профіль (`-profile 1/2`), конфіг `coopandreas_N.ini` і лог `client_N.log`. У грі поставте роздільність 800×600.
