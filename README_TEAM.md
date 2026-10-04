# CoopAndreas — наша приватна збірка / our private build

Приватний форк [Tornamic/CoopAndreas](https://github.com/Tornamic/CoopAndreas) (GPL-3.0) з виправленнями синхронізації та менеджером встановлення.
Оригінальний README — [README.md](README.md). Аналіз проблем і план — [docs/SYNC_ANALYSIS.md](docs/SYNC_ANALYSIS.md).

## Два репозиторії
| Репозиторій | Доступ | Що містить |
|---|---|---|
| `GuessGames/sa_dream_mod` (цей) | приватний | код, інструменти, документація |
| `GuessGames/sa_dream_mod_coop` | публічний | тільки готові файли + архів коду кожної версії (вимога GPL-3) |

## Для гравця (git не потрібен)
1. Завантажте `CoopAndreasManager.exe` з публічного релізного репозиторію і запустіть.
2. **Налаштування:** тека гри + архів або тека з `gta_sa.exe` 1.0 US (його немає в релізі).
3. **Встановлення → Оновити і встановити.** Менеджер завантажує тільки змінені файли, перевіряє SHA-256 і встановлює. Сам себе він теж оновлює.
4. **Запуск:** серійний ключ (`/gen <ID>` у Discord CoopAndreas), нік, IP, кнопка «Запустити гру».

## Для розробника
- Збірка: `xmake f -p windows -a x86 -m release --toolchain=msvc`, потім `xmake -y` (з PowerShell/cmd; потрібні VS 2022 Build Tools і xmake).
- Менеджер: `tools\manager\build.cmd` → `build\manager\CoopAndreasManager.exe` (у режимі розробника знаходить репозиторій сам).
- Режим «Розробник: моя локальна збірка» встановлює в гру з `build\`.
- **Опублікувати реліз:**
  1. збирає пакет у локальну копію релізного репозиторію (`..\release`);
  2. додає `source/*.zip` з кодом цієї версії;
  3. комітить і пушить. Гравці отримують його кнопкою «Оновити».
- CLI: `CoopAndreasManager.exe --status | --install | --repair | --uninstall | --verify | --update | --assemble <dir> | --publish`.
- Remote `upstream` (Tornamic) — тільки для читання: push-URL навмисно вимкнено. Оновлення від автора: `git fetch upstream` + `git merge upstream/main`.

## Структура
| Шлях | Що це |
|---|---|
| `client/ server/ shared/ proxy/ launcher/` | код моду |
| `scm/` | скомпільований `main.scm` + `script.img` |
| `dist/additional/` | ASI loader, WindowedMode, WidescreenFix, scrlog (без `gta_sa.exe`) |
| `tools/manager/` | код менеджера |
| `docs/` | документація |

## Тест у два вікна на одному ПК
**Запуск → Сервер + 2 вікна гри.** Кожне вікно має свій профіль (`-profile 1/2`), конфіг `coopandreas_N.ini` і лог `client_N.log`. У грі поставте роздільність 800×600.
