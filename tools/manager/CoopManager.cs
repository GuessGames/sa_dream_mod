// CoopAndreas Manager - install / uninstall / repair / update / launch / logs
// Built with the .NET Framework 4.x csc.exe (C# 5), no external dependencies. See build.cmd.
//
// Two modes:
//  * player    - downloads the prebuilt package from the PUBLIC release repo (no git needed) and installs it
//  * developer - assembles the package from the local xmake build of the PRIVATE source repo, can publish releases
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

[assembly: System.Reflection.AssemblyTitle("CoopAndreas Manager")]
[assembly: System.Reflection.AssemblyVersion("1.1.0.0")]

namespace CoopManager
{
    // ------------------------------------------------------------------ localization
    static class L
    {
        public static bool Uk = true;
        static readonly Dictionary<string, string[]> S = new Dictionary<string, string[]>
        {
            {"title", new[]{"SA Dream Mod — менеджер розробника", "SA Dream Mod — developer manager"}},
            {"tabInstall", new[]{"Встановлення", "Install"}},
            {"tabLaunch", new[]{"Запуск", "Launch"}},
            {"tabLogs", new[]{"Логи", "Logs"}},
            {"tabSettings", new[]{"Налаштування", "Settings"}},
            {"lang", new[]{"English", "Українська"}},
            {"gameDir", new[]{"Тека гри:", "Game folder:"}},
            {"srcDir", new[]{"Код моду (git, розробник):", "Mod source (git, developer):"}},
            {"addZip", new[]{"Архів з gta_sa.exe 1.0 US:", "Archive with gta_sa.exe 1.0 US:"}},
            {"releaseRepo", new[]{"Репозиторій релізу (owner/repo):", "Release repository (owner/repo):"}},
            {"releaseDir", new[]{"Локальна копія релізу (розробник):", "Local release clone (developer):"}},
            {"browse", new[]{"Огляд…", "Browse…"}},
            {"save", new[]{"Зберегти", "Save"}},
            {"status", new[]{"Стан", "Status"}},
            {"refresh", new[]{"Оновити стан", "Refresh status"}},
            {"install", new[]{"Встановити / перевстановити", "Install / reinstall"}},
            {"uninstall", new[]{"Видалити мод", "Uninstall mod"}},
            {"repair", new[]{"Перевірити і виправити", "Verify and repair"}},
            {"build", new[]{"Зібрати з коду", "Build from source"}},
            {"fixPerms", new[]{"Надати права на теку гри", "Grant access to game folder"}},
            {"exeOk", new[]{"gta_sa.exe: версія 1.0 US (сумісна)", "gta_sa.exe: version 1.0 US (compatible)"}},
            {"exeBad", new[]{"gta_sa.exe: НЕСУМІСНА версія (буде замінена під час встановлення)", "gta_sa.exe: INCOMPATIBLE version (will be replaced on install)"}},
            {"exeMissing", new[]{"gta_sa.exe не знайдено", "gta_sa.exe not found"}},
            {"modInstalled", new[]{"Мод встановлено: версія {0}", "Mod installed: version {0}"}},
            {"modNotInstalled", new[]{"Мод не встановлено", "Mod is not installed"}},
            {"pkgReady", new[]{"Пакет для встановлення: версія {0}", "Package to install: version {0}"}},
            {"pkgMissing", new[]{"Пакета немає — натисніть «Оновити»", "No package yet — press \"Update\""}},
            {"buildMissing", new[]{"Немає локальної збірки — натисніть «Зібрати з коду»", "No local build — press \"Build from source\""}},
            {"backupFound", new[]{"Резервна копія оригінальних файлів: є", "Backup of original files: present"}},
            {"writableNo", new[]{"Тека гри: немає прав на запис — натисніть «Надати права на теку гри»", "Game folder: not writable — press \"Grant access to game folder\""}},
            {"notWritable", new[]{"Немає прав на запис у теку гри (вона захищена як Program Files). Натисніть «Так», щоб надати вашому користувачу права на зміну цієї теки (з'явиться запит адміністратора Windows).", "The game folder is not writable (protected like Program Files). Press \"Yes\" to grant your user modify rights on this folder (a Windows administrator prompt will appear)."}},
            {"updates", new[]{"Оновлення", "Updates"}},
            {"modePlayer", new[]{"Гравець: оновлення з репозиторію релізу", "Player: updates from the release repository"}},
            {"releaseAccess", new[]{"Немає доступу до репозиторію релізу. Увійдіть у GitHub у вікні, яке відкриє git, і переконайтеся, що вас додано в колаборатори.", "No access to the release repository. Log in to GitHub in the window git opens and make sure you were added as a collaborator."}},
            {"needGit", new[]{"Потрібен Git for Windows: https://git-scm.com/download/win", "Git for Windows is required: https://git-scm.com/download/win"}},
            {"modeDev", new[]{"Розробник: моя локальна збірка", "Developer: my local build"}},
            {"checkUpdates", new[]{"Перевірити оновлення", "Check for updates"}},
            {"update", new[]{"Оновити і встановити", "Update and install"}},
            {"publish", new[]{"Опублікувати реліз", "Publish release"}},
            {"remoteVersion", new[]{"Доступна версія: {0}", "Available version: {0}"}},
            {"upToDate", new[]{"Встановлена остання версія ({0})", "Up to date ({0})"}},
            {"newVersion", new[]{"Є нова версія: {0} (встановлено: {1})", "New version available: {0} (installed: {1})"}},
            {"devInfo", new[]{"Код: {0}   гілка: {1}   коміт: {2}", "Source: {0}   branch: {1}   commit: {2}"}},
            {"repoMissing", new[]{"Репозиторій з кодом не знайдено (потрібен тільки розробнику)", "Source repository not found (developers only)"}},
            {"unsafeOrigin", new[]{"Remote 'origin' не налаштований або вказує на оригінальний репозиторій Tornamic. Дозволено працювати тільки з нашими репозиторіями.", "Remote 'origin' is not set or points to the original Tornamic repository. Only our own repositories are allowed."}},
            {"dirtyTree", new[]{"У репозиторії з кодом є незакомічені зміни. Закомітьте їх перед публікацією, щоб реліз відповідав коду.", "The source repository has uncommitted changes. Commit them before publishing so the release matches the source."}},
            {"confirmPublish", new[]{"Опублікувати нову версію в репозиторій релізу {0}?", "Publish a new version to the release repository {0}?"}},
            {"notPushed", new[]{"Спочатку запуште коміти в репозиторій з кодом: реліз має відповідати опублікованому коду.", "Push your commits to the source repository first: the release must match the published source."}},
            {"releaseDirMissing", new[]{"Локальна копія релізного репозиторію не знайдена: {0}", "Local release clone not found: {0}"}},
            {"selfUpdated", new[]{"Менеджер оновлено — перезапустіть його.", "The manager was updated — please restart it."}},
            {"serial", new[]{"Серійний ключ бета-тесту", "Beta test serial key"}},
            {"pcid", new[]{"ID вашого ПК:", "Your PC ID:"}},
            {"copyCmd", new[]{"Копіювати команду", "Copy command"}},
            {"discord", new[]{"Відкрити Discord", "Open Discord"}},
            {"serialHint", new[]{"Надішліть команду в будь-який канал Discord-сервера CoopAndreas, бот видасть ключ. Вставте його нижче.", "Send the command in any channel of the CoopAndreas Discord server, the bot replies with a key. Paste it below."}},
            {"serialKey", new[]{"Ключ:", "Key:"}},
            {"play", new[]{"Звичайна гра", "Normal play"}},
            {"nick", new[]{"Нікнейм:", "Nickname:"}},
            {"ip", new[]{"IP сервера:", "Server IP:"}},
            {"port", new[]{"Порт:", "Port:"}},
            {"launchGame", new[]{"Запустити гру", "Launch game"}},
            {"server", new[]{"Локальний сервер", "Local server"}},
            {"startServer", new[]{"Запустити сервер", "Start server"}},
            {"stopServer", new[]{"Зупинити сервер", "Stop server"}},
            {"serverRunning", new[]{"Сервер працює (PID {0})", "Server running (PID {0})"}},
            {"serverStopped", new[]{"Сервер зупинено", "Server stopped"}},
            {"test", new[]{"Тест у два вікна (на цьому ПК)", "Two-window test (this PC)"}},
            {"nick1", new[]{"Нік вікна 1:", "Window 1 nick:"}},
            {"nick2", new[]{"Нік вікна 2:", "Window 2 nick:"}},
            {"launchTest", new[]{"Сервер + 2 вікна гри", "Server + 2 game windows"}},
            {"killGames", new[]{"Закрити всі вікна гри", "Close all game windows"}},
            {"testHint", new[]{"Вікна ставляться поруч і самі підключаються до 127.0.0.1 (нік і IP вже прописані). Роздільність у грі — 800×600.", "Windows are placed side by side and connect to 127.0.0.1 automatically (nick and IP are pre-filled). In-game resolution 800×600."}},
            {"logFile", new[]{"Файл:", "File:"}},
            {"filter", new[]{"Фільтр:", "Filter:"}},
            {"onlyWarn", new[]{"Тільки попередження/помилки", "Warnings/errors only"}},
            {"follow", new[]{"Автопрокрутка", "Auto-scroll"}},
            {"openFolder", new[]{"Відкрити теку логів", "Open logs folder"}},
            {"reload", new[]{"Перечитати", "Reload"}},
            {"needSerial", new[]{"Спочатку вкажіть серійний ключ на вкладці «Запуск».", "Enter the serial key on the Launch tab first."}},
            {"needInstall", new[]{"Мод не встановлено. Спочатку встановіть його.", "The mod is not installed. Install it first."}},
            {"confirmUninstall", new[]{"Видалити CoopAndreas і відновити оригінальні файли гри?", "Remove CoopAndreas and restore the original game files?"}},
            {"done", new[]{"Готово.", "Done."}},
            {"busy", new[]{"Зачекайте, виконується інша операція…", "Please wait, another operation is running…"}},
            {"failed", new[]{"Помилка: {0}", "Error: {0}"}},
            {"gameRunning", new[]{"Гра запущена — закрийте її перед цією операцією.", "The game is running — close it before this operation."}},
            {"pTitle", new[]{"SA Dream Mod", "SA Dream Mod"}},
            {"pSubtitle", new[]{"Кооператив GTA San Andreas · на основі CoopAndreas", "GTA San Andreas co-op · based on CoopAndreas"}},
            {"pGame", new[]{"Гра", "Game"}},
            {"pFolder", new[]{"Тека гри", "Game folder"}},
            {"pChange", new[]{"Змінити…", "Change…"}},
            {"pExeNeeded", new[]{"Архів additional — потрібен, якщо гра не версії 1.0 US", "Additional archive — needed if your game isn't 1.0 US"}},
            {"pNoArchive", new[]{"не вказано", "not set"}},
            {"pArchiveRequired", new[]{"Ваша гра не версії 1.0 US. Вкажіть архів additional (або теку з gta_sa.exe 1.0 US) — лаунчер встановить усе з нього сам.", "Your game is not version 1.0 US. Choose the additional archive (or a folder with gta_sa.exe 1.0 US) — the launcher installs everything from it."}},
            {"pChooseGame", new[]{"Вкажіть теку, де встановлена GTA San Andreas.", "Choose the folder where GTA San Andreas is installed."}},
            {"pArchiveBad", new[]{"У вибраному архіві/теці немає gta_sa.exe версії 1.0 US.", "The chosen archive/folder has no gta_sa.exe version 1.0 US."}},
            {"pChooseArchive", new[]{"Вказати…", "Choose…"}},
            {"pInstall", new[]{"Встановити", "Install"}},
            {"pUpdateTo", new[]{"Оновити до {0}", "Update to {0}"}},
            {"pCheck", new[]{"Перевірити оновлення", "Check for updates"}},
            {"pChecking", new[]{"Перевіряю оновлення…", "Checking for updates…"}},
            {"pNotInstalled", new[]{"Мод ще не встановлено", "The mod is not installed yet"}},
            {"pInstalledV", new[]{"Встановлено: {0}", "Installed: {0}"}},
            {"pLatest", new[]{"У вас остання версія", "You have the latest version"}},
            {"pNewAvail", new[]{"Доступна нова версія: {0}", "New version available: {0}"}},
            {"pOffline", new[]{"Не вдалося перевірити оновлення", "Could not check for updates"}},
            {"pPlayer", new[]{"Гравець", "Player"}},
            {"pIpHint", new[]{"IP дає той, хто запускає сервер", "The server host gives you the IP"}},
            {"pKey", new[]{"Ключ бета-тесту", "Beta key"}},
            {"pHowKey", new[]{"Як отримати?", "How to get it?"}},
            {"pHowKeyText", new[]{"1. Команду «{0}» скопійовано в буфер обміну.\n2. Зараз відкриється Discord CoopAndreas — вставте команду в будь-який канал.\n3. Бот надішле ключ — вставте його в поле «Ключ».", "1. The command \"{0}\" was copied to the clipboard.\n2. The CoopAndreas Discord opens now — paste the command into any channel.\n3. The bot sends you a key — paste it into the \"Key\" field."}},
            {"pPlay", new[]{"ГРАТИ", "PLAY"}},
            {"pLogs", new[]{"Логи", "Logs"}},
            {"pRepair", new[]{"Виправити", "Repair"}},
            {"pWorking", new[]{"Зачекайте…", "Please wait…"}},
            {"pReady", new[]{"Готово", "Ready"}},
            {"svTitle", new[]{"Сервер", "Server"}},
            {"svSubtitle", new[]{"Запустіть сервер, щоб друзі могли підключитися до вас", "Start the server so your friends can join you"}},
            {"svRunning", new[]{"● Сервер працює", "● Server is running"}},
            {"svStopped", new[]{"● Сервер зупинено", "● Server is stopped"}},
            {"svNotInstalled", new[]{"Сервер ще не встановлено — спочатку встановіть мод.", "The server is not installed yet — install the mod first."}},
            {"svStart", new[]{"Запустити сервер", "Start server"}},
            {"svStop", new[]{"Зупинити", "Stop"}},
            {"svPort", new[]{"Порт (UDP):", "Port (UDP):"}},
            {"svAddresses", new[]{"Адреси для друзів", "Addresses for your friends"}},
            {"svCopy", new[]{"Копіювати IP", "Copy IP"}},
            {"svCopied", new[]{"IP скопійовано: {0}", "IP copied: {0}"}},
            {"svUseLocal", new[]{"Я теж граю тут (IP 127.0.0.1)", "I play here too (IP 127.0.0.1)"}},
            {"svPlayers", new[]{"Гравці онлайн: {0}", "Players online: {0}"}},
            {"svLog", new[]{"Лог сервера", "Server log"}},
            {"svInternet", new[]{"Інтернет, порт {0} UDP", "Internet, port {0} UDP"}},
            {"svLan", new[]{"Локальна мережа", "Local network"}},
            {"svOpenPanel", new[]{"Панель сервера", "Server panel"}},
            {"svPaused", new[]{" (пауза)", " (paused)"}},
            {"noExe", new[]{"Потрібен gta_sa.exe версії 1.0 US: вкажіть архів або теку з ним на вкладці «Налаштування».", "gta_sa.exe 1.0 US is required: set the archive or folder containing it on the Settings tab."}},
        };
        public static string T(string key)
        {
            string[] v;
            if (!S.TryGetValue(key, out v)) return key;
            return Uk ? v[0] : v[1];
        }
        public static string F(string key, params object[] args) { return string.Format(T(key), args); }
    }

    // ------------------------------------------------------------------ settings (%LOCALAPPDATA%\CoopAndreasManager)
    class Settings
    {
        public string GameDir = @"C:\Program Files (x86)\Rockstar Games\GTA San Andreas";
        public string SourceDir = "";
        public string AdditionalZip = "";
        public string ReleaseRepo = "GuessGames/sa_dream_mod_coop";
        public string ReleaseDir = "";
        public bool Ukrainian = true;
        public bool PlayerMode = true;
        public string Nick = "Player";
        public string Ip = "127.0.0.1";
        public int Port = 6767;
        public string Nick1 = "Tester1";
        public string Nick2 = "Tester2";

        public static string AppDataDir { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "CoopAndreasManager"); } }
        static string FilePath { get { return Path.Combine(AppDataDir, Program.IsPlayerEdition ? "CoopLauncher.ini" : "CoopManager.ini"); } }

        public static Settings Load()
        {
            var s = new Settings();
            if (File.Exists(FilePath))
            {
                foreach (var line in File.ReadAllLines(FilePath, Encoding.UTF8))
                {
                    int eq = line.IndexOf('=');
                    if (eq <= 0) continue;
                    string k = line.Substring(0, eq).Trim().TrimStart('\uFEFF'), v = line.Substring(eq + 1).Trim();
                    switch (k)
                    {
                        case "GameDir": s.GameDir = v; break;
                        case "SourceDir": s.SourceDir = v; break;
                        case "AdditionalZip": s.AdditionalZip = v; break;
                        case "ReleaseRepo": if (NormalizeRepo(v).Length > 0) s.ReleaseRepo = NormalizeRepo(v); break;
                        case "ReleaseDir": s.ReleaseDir = v; break;
                        case "Ukrainian": s.Ukrainian = v == "1"; break;
                        case "PlayerMode": s.PlayerMode = v != "0"; break;
                        case "InstallFromDist": s.PlayerMode = v != "0"; break; // pre-1.1 name
                        case "Nick": s.Nick = v; break;
                        case "Ip": s.Ip = v; break;
                        case "Port": int.TryParse(v, out s.Port); break;
                        case "Nick1": s.Nick1 = v; break;
                        case "Nick2": s.Nick2 = v; break;
                    }
                }
            }
            // developer machine: the exe was built inside the source repo
            if (s.SourceDir.Length == 0 && !Program.IsPlayerEdition)
            {
                string repo = FindRepoRoot(Path.GetDirectoryName(Application.ExecutablePath));
                if (repo != null) { s.SourceDir = repo; s.PlayerMode = false; }
            }
            if (Program.IsPlayerEdition) { s.PlayerMode = true; s.SourceDir = ""; }
            if (!IsGameFolder(s.GameDir))
            {
                string found = FindGameDir();
                if (found != null) s.GameDir = found;
            }
            if (!File.Exists(FilePath) && s.Nick == "Player") s.Nick = Environment.UserName;
            if (s.ReleaseDir.Length == 0 && s.SourceDir.Length > 0)
                s.ReleaseDir = Path.Combine(Path.GetDirectoryName(s.SourceDir.TrimEnd('\\')), "release");
            return s;
        }

        // first run: look for the game in the usual places (Rockstar Launcher, Steam, retail, drive roots)
        static string FindGameDir()
        {
            var candidates = new List<string>();
            try
            {
                using (var k = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\WOW6432Node\Rockstar Games\GTA San Andreas\Installation"))
                    if (k != null && k.GetValue("ExePath") is string) candidates.Add(Path.GetDirectoryName(((string)k.GetValue("ExePath")).Trim('"')));
                using (var k = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\WOW6432Node\Valve\Steam"))
                    if (k != null && k.GetValue("InstallPath") is string) candidates.Add(Path.Combine((string)k.GetValue("InstallPath"), @"steamapps\common\Grand Theft Auto San Andreas"));
            }
            catch { }
            foreach (var drive in DriveInfo.GetDrives().Where(d => d.DriveType == DriveType.Fixed && d.IsReady))
            {
                string root = drive.RootDirectory.FullName;
                foreach (var rel in new[] { "Grand Theft Auto San Andreas", "GTA San Andreas", @"Games\Grand Theft Auto San Andreas",
                    @"Games\GTA San Andreas", @"Program Files (x86)\Rockstar Games\GTA San Andreas", @"Program Files\Rockstar Games\GTA San Andreas" })
                    candidates.Add(Path.Combine(root, rel));
            }
            return candidates.FirstOrDefault(IsGameFolder);
        }

        // a GTA SA folder: any known exe name (retail/RGL gta_sa.exe, Steam gta-sa.exe) or the main archive
        public static bool IsGameFolder(string dir)
        {
            try
            {
                return !string.IsNullOrEmpty(dir) && (File.Exists(Path.Combine(dir, "gta_sa.exe")) ||
                       File.Exists(Path.Combine(dir, "gta-sa.exe")) || File.Exists(Path.Combine(dir, @"models\gta3.img")));
            }
            catch { return false; }
        }

        // "https://github.com/owner/repo(.git)(/)" or "owner/repo" -> "owner/repo"
        public static string NormalizeRepo(string value)
        {
            string v = (value ?? "").Trim();
            v = Regex.Replace(v, @"^(https?://)?(www\.)?github\.com/", "", RegexOptions.IgnoreCase);
            v = Regex.Replace(v, @"(\.git)?/*$", "", RegexOptions.IgnoreCase);
            var parts = v.Split('/');
            return parts.Length >= 2 ? parts[0] + "/" + parts[1] : v;
        }

        public static string FindRepoRoot(string dir)
        {
            for (var d = new DirectoryInfo(dir); d != null; d = d.Parent)
                if (Directory.Exists(Path.Combine(d.FullName, ".git"))) return d.FullName;
            return null;
        }

        public void Save()
        {
            var sb = new StringBuilder();
            sb.AppendLine("GameDir=" + GameDir);
            sb.AppendLine("SourceDir=" + SourceDir);
            sb.AppendLine("AdditionalZip=" + AdditionalZip);
            sb.AppendLine("ReleaseRepo=" + ReleaseRepo);
            sb.AppendLine("ReleaseDir=" + ReleaseDir);
            sb.AppendLine("Ukrainian=" + (Ukrainian ? "1" : "0"));
            sb.AppendLine("PlayerMode=" + (PlayerMode ? "1" : "0"));
            sb.AppendLine("Nick=" + Nick);
            sb.AppendLine("Ip=" + Ip);
            sb.AppendLine("Port=" + Port);
            sb.AppendLine("Nick1=" + Nick1);
            sb.AppendLine("Nick2=" + Nick2);
            Directory.CreateDirectory(AppDataDir);
            File.WriteAllText(FilePath, sb.ToString(), new UTF8Encoding(false));
        }
    }

    // ------------------------------------------------------------------ package manifest (manifest.txt)
    //   version=2026.10.04-1cbe9b2
    //   file=bin/CoopAndreasSA.dll|<sha256>|<size>
    class Manifest
    {
        public string Version = "";
        public readonly Dictionary<string, string> Meta = new Dictionary<string, string>();
        public readonly SortedDictionary<string, string> Files = new SortedDictionary<string, string>(StringComparer.OrdinalIgnoreCase); // path -> sha256

        public static Manifest Parse(string text)
        {
            var m = new Manifest();
            foreach (var raw in text.Replace("\r", "").Split('\n'))
            {
                int eq = raw.IndexOf('=');
                if (eq <= 0) continue;
                string k = raw.Substring(0, eq), v = raw.Substring(eq + 1);
                if (k == "file")
                {
                    var parts = v.Split('|');
                    if (parts.Length >= 2) m.Files[parts[0]] = parts[1];
                }
                else
                {
                    m.Meta[k] = v;
                    if (k == "version") m.Version = v;
                }
            }
            return m;
        }

        public static Manifest Load(string dir)
        {
            string p = Path.Combine(dir, "manifest.txt");
            return File.Exists(p) ? Parse(File.ReadAllText(p)) : null;
        }

        // hashes every file of the package directory (except manifest.txt / .git / source) and writes manifest.txt
        public static Manifest Write(string dir, string version, Dictionary<string, string> meta)
        {
            var m = new Manifest { Version = version };
            foreach (var f in Directory.GetFiles(dir, "*", SearchOption.AllDirectories))
            {
                string rel = f.Substring(dir.TrimEnd('\\').Length + 1).Replace('\\', '/');
                if (rel == "manifest.txt" || rel.StartsWith(".git") || rel.StartsWith("source/") || rel.StartsWith("dev/") || rel == "README.md") continue;
                m.Files[rel] = Installer.Sha256(f);
            }
            var sb = new StringBuilder();
            sb.AppendLine("version=" + version);
            foreach (var kv in meta) sb.AppendLine(kv.Key + "=" + kv.Value);
            foreach (var kv in m.Files) sb.AppendLine("file=" + kv.Key + "|" + kv.Value + "|" + new FileInfo(Path.Combine(dir, kv.Key)).Length);
            File.WriteAllText(Path.Combine(dir, "manifest.txt"), sb.ToString(), new UTF8Encoding(false));
            return m;
        }
    }

    // ------------------------------------------------------------------ install logic
    class Installer
    {
        public const string BackupDirName = "_coop_backup";
        public const string ManifestName = "install_manifest.txt";
        public const long Exe10UsSize = 14383616;
        public const long AsiLoaderVorbisSize = 18944; // vorbisFile.dll of the ASI loader

        readonly Settings s;
        readonly Action<string> log;
        public Installer(Settings settings, Action<string> logger) { s = settings; log = logger; }

        public string BuildDir { get { return Path.Combine(s.SourceDir, @"build\windows\x86\release"); } }
        public string LauncherBuildPath { get { return Path.Combine(s.SourceDir, @"build\manager\" + Program.LauncherFileName); } }
        public string PlayerPackageDir { get { return Path.Combine(Settings.AppDataDir, "package"); } }
        public string DevPackageDir { get { return Path.Combine(Settings.AppDataDir, "dev_package"); } }
        public string PackageDir { get { return s.PlayerMode ? PlayerPackageDir : DevPackageDir; } }
        public string BackupDir { get { return Path.Combine(s.GameDir, BackupDirName); } }
        public string ManifestPath { get { return Path.Combine(BackupDir, ManifestName); } }
        public string LogsDir { get { return Path.Combine(s.GameDir, @"CoopAndreas\logs"); } }
        public string ServerDir { get { return Path.Combine(s.GameDir, "CoopAndreasServer"); } }

        // files whose originals we keep in the backup folder
        static readonly string[] OriginalsToBackup = { "gta_sa.exe", "eax.dll", "vorbisFile.dll" };

        public static string Sha256(string path)
        {
            using (var sha = SHA256.Create())
            using (var f = File.OpenRead(path))
                return BitConverter.ToString(sha.ComputeHash(f)).Replace("-", "");
        }

        public bool IsInstalled { get { return File.Exists(ManifestPath); } }

        public string InstalledVersion
        {
            get
            {
                if (!IsInstalled) return "";
                var first = File.ReadAllLines(ManifestPath).FirstOrDefault(l => l.StartsWith("# version "));
                return first == null ? "?" : first.Substring(10);
            }
        }

        public bool IsGameDirWritable()
        {
            try
            {
                string probe = Path.Combine(s.GameDir, ".coop_write_test");
                File.WriteAllText(probe, "x");
                File.Delete(probe);
                return true;
            }
            catch { return false; }
        }

        // runs icacls elevated (UAC prompt) to give the current user modify rights on the game folder
        public bool GrantGameDirAccess()
        {
            string user = Environment.UserDomainName + "\\" + Environment.UserName;
            var psi = new ProcessStartInfo("icacls.exe", "\"" + s.GameDir + "\" /grant \"" + user + "\":(OI)(CI)M /T /Q")
            {
                Verb = "runas", UseShellExecute = true, WindowStyle = ProcessWindowStyle.Hidden,
            };
            try
            {
                using (var p = Process.Start(psi)) { p.WaitForExit(); }
            }
            catch (System.ComponentModel.Win32Exception) { return false; } // UAC declined
            return IsGameDirWritable();
        }

        public string ExeState()
        {
            string exe = Path.Combine(s.GameDir, "gta_sa.exe");
            if (!File.Exists(exe)) return L.T("exeMissing");
            return new FileInfo(exe).Length == Exe10UsSize ? L.T("exeOk") : L.T("exeBad");
        }

        // gta_sa.exe 1.0 US: from the release package (exe\) or from the archive/folder set in the settings
        public string GetCompatibleExe()
        {
            string packaged = Path.Combine(PackageDir, @"exe\gta_sa.exe");
            if (File.Exists(packaged) && new FileInfo(packaged).Length == Exe10UsSize) return packaged;
            return GetCompatibleExeFromSettings();
        }

        public string GetCompatibleExeFromSettings()
        {
            string p = s.AdditionalZip;
            if (string.IsNullOrEmpty(p)) return null;
            if (Directory.Exists(p))
            {
                return Directory.GetFiles(p, "gta_sa.exe", SearchOption.AllDirectories)
                    .FirstOrDefault(f => new FileInfo(f).Length == Exe10UsSize);
            }
            if (!File.Exists(p)) return null;
            string tmp = Path.Combine(Path.GetTempPath(), "CoopManager_exe");
            Directory.CreateDirectory(tmp);
            using (var zip = ZipFile.OpenRead(p))
            {
                var e = zip.Entries.FirstOrDefault(x => x.Name.Equals("gta_sa.exe", StringComparison.OrdinalIgnoreCase) && x.Length == Exe10UsSize);
                if (e == null) return null;
                string dst = Path.Combine(tmp, "gta_sa.exe");
                e.ExtractToFile(dst, true);
                return dst;
            }
        }

        // developer: builds a package directory from the local build of the source repo
        public void AssembleFromBuild(string target, bool clean)
        {
            if (!File.Exists(Path.Combine(BuildDir, "CoopAndreasSA.dll"))) throw new Exception(L.T("buildMissing"));
            if (clean)
            {
                foreach (var sub in new[] { "bin", "additional", "scm", "exe" })
                    if (Directory.Exists(Path.Combine(target, sub))) Directory.Delete(Path.Combine(target, sub), true);
            }
            Directory.CreateDirectory(Path.Combine(target, "bin"));
            Directory.CreateDirectory(Path.Combine(target, "additional"));
            Directory.CreateDirectory(Path.Combine(target, "scm"));
            foreach (var name in new[] { "CoopAndreasSA.dll", "proxy.dll", "LaunchCoopAndreas.exe", "LaunchCoopAndreas.exe.manifest", "server.exe" })
                File.Copy(Path.Combine(BuildDir, name), Path.Combine(target, "bin", name), true);
            // the release is public: Rockstar files (gta_sa.exe, the original vorbisFile.dll) are never included,
            // vorbisHooked.dll is recreated from the player's own game and gta_sa.exe comes from his archive
            foreach (var f in Directory.GetFiles(Path.Combine(s.SourceDir, @"dist\additional")))
            {
                if (Path.GetFileName(f).Equals("vorbisHooked.dll", StringComparison.OrdinalIgnoreCase)) continue;
                File.Copy(f, Path.Combine(target, "additional", Path.GetFileName(f)), true);
            }
            foreach (var name in new[] { "main.scm", "script.img" })
                File.Copy(Path.Combine(s.SourceDir, "scm", name), Path.Combine(target, "scm", name), true);
            foreach (var old in new[] { "CoopAndreasManager.exe" })
                if (File.Exists(Path.Combine(target, old))) File.Delete(Path.Combine(target, old));
            if (!File.Exists(LauncherBuildPath)) throw new Exception("build the launcher first: tools\\manager\\build.cmd");
            File.Copy(LauncherBuildPath, Path.Combine(target, Program.LauncherFileName), true);

            var git = new Git(s);
            string head = git.Get("rev-parse --short HEAD");
            string version = DateTime.Now.ToString("yyyy.MM.dd.HHmm") + "-" + (head.Length > 0 ? head : "local");
            var meta = new Dictionary<string, string> { { "commit", head }, { "built", DateTime.Now.ToString("yyyy-MM-dd HH:mm") } };
            Manifest.Write(target, version, meta);
            log("package " + version + " -> " + target);
        }

        // map: destination path relative to game dir -> source absolute path
        Dictionary<string, string> BuildFileMap(string pkg)
        {
            var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (var f in Directory.GetFiles(Path.Combine(pkg, "additional")))
            {
                string name = Path.GetFileName(f);
                if (name.EndsWith(".ini", StringComparison.OrdinalIgnoreCase)) continue; // copied only when missing
                map[name] = f;
            }
            map["eax.dll"] = Path.Combine(pkg, @"bin\proxy.dll");
            map["CoopAndreasSA.dll"] = Path.Combine(pkg, @"bin\CoopAndreasSA.dll");
            map["LaunchCoopAndreas.exe"] = Path.Combine(pkg, @"bin\LaunchCoopAndreas.exe");
            map["LaunchCoopAndreas.exe.manifest"] = Path.Combine(pkg, @"bin\LaunchCoopAndreas.exe.manifest");
            map[@"CoopAndreas\main.scm"] = Path.Combine(pkg, @"scm\main.scm");
            map[@"CoopAndreas\script.img"] = Path.Combine(pkg, @"scm\script.img");
            map[@"CoopAndreasServer\server.exe"] = Path.Combine(pkg, @"bin\server.exe");
            return map;
        }

        void EnsureGameNotRunning()
        {
            if (Process.GetProcessesByName("gta_sa").Length > 0) throw new Exception(L.T("gameRunning"));
        }

        public void Install(bool onlyChanged)
        {
            EnsureGameNotRunning();
            if (!Directory.Exists(s.GameDir)) throw new Exception("Game folder not found: " + s.GameDir);
            if (!s.PlayerMode) AssembleFromBuild(DevPackageDir, true);
            string pkg = PackageDir;
            var pm = Manifest.Load(pkg);
            if (pm == null) throw new Exception(L.T("pkgMissing"));

            var map = BuildFileMap(pkg);
            string gameExe = Path.Combine(s.GameDir, "gta_sa.exe");
            if (!File.Exists(gameExe) || new FileInfo(gameExe).Length != Exe10UsSize)
            {
                string exe = GetCompatibleExe();
                if (exe == null) throw new Exception(L.T("noExe"));
                map["gta_sa.exe"] = exe;
            }
            Directory.CreateDirectory(BackupDir);

            // 1. back up the originals once (first install only)
            foreach (var name in OriginalsToBackup)
            {
                string src = Path.Combine(s.GameDir, name), dst = Path.Combine(BackupDir, name);
                if (File.Exists(src) && !File.Exists(dst) && !IsInstalled)
                {
                    File.Copy(src, dst);
                    log("backup: " + name);
                }
            }

            // 2. the original eax.dll must stay reachable as eax_orig.dll for the proxy
            string eaxOrig = Path.Combine(s.GameDir, "eax_orig.dll");
            string eaxBackup = Path.Combine(BackupDir, "eax.dll");
            if (!File.Exists(eaxOrig) && File.Exists(eaxBackup))
            {
                File.Copy(eaxBackup, eaxOrig);
                log("eax.dll -> eax_orig.dll");
            }

            // 3. the ASI loader replaces vorbisFile.dll and loads the original one as vorbisHooked.dll
            string hooked = Path.Combine(s.GameDir, "vorbisHooked.dll");
            if (!File.Exists(hooked))
            {
                string orig = new[] { Path.Combine(BackupDir, "vorbisFile.dll"), Path.Combine(s.GameDir, "vorbisFile.dll") }
                    .FirstOrDefault(f => File.Exists(f) && new FileInfo(f).Length != AsiLoaderVorbisSize);
                if (orig != null) { File.Copy(orig, hooked); log("original vorbisFile.dll -> vorbisHooked.dll"); }
                else log("! vorbisHooked.dll not found: the original vorbisFile.dll of the game is missing");
            }

            // 4. copy files
            var manifest = new List<string>();
            foreach (var kv in map)
            {
                string dst = Path.Combine(s.GameDir, kv.Key);
                if (!File.Exists(kv.Value)) { log("! missing source: " + kv.Value); continue; }
                string srcHash = Sha256(kv.Value);
                bool same = File.Exists(dst) && Sha256(dst) == srcHash;
                if (!same)
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(dst));
                    File.Copy(kv.Value, dst, true);
                    log("copy: " + kv.Key);
                }
                // gta_sa.exe is restored from the backup on uninstall, so it is not tracked as a mod file
                if (!kv.Key.Equals("gta_sa.exe", StringComparison.OrdinalIgnoreCase)) manifest.Add(kv.Key + "|" + srcHash);
            }

            // 5. default ini files, never overwritten
            foreach (var f in Directory.GetFiles(Path.Combine(pkg, "additional"), "*.ini"))
            {
                string dst = Path.Combine(s.GameDir, Path.GetFileName(f));
                if (!File.Exists(dst)) { File.Copy(f, dst); log("copy (default config): " + Path.GetFileName(f)); }
                manifest.Add(Path.GetFileName(f) + "|config");
            }
            manifest.Add("eax_orig.dll|generated");
            manifest.Add("vorbisHooked.dll|generated");
            // e.g. the Steam version has gta-sa.exe only: our gta_sa.exe has no original to restore, remove it on uninstall
            if (!File.Exists(Path.Combine(BackupDir, "gta_sa.exe"))) manifest.Add("gta_sa.exe|generated");

            Directory.CreateDirectory(LogsDir);
            File.WriteAllLines(ManifestPath, new[] { "# installed " + DateTime.Now.ToString("yyyy-MM-dd HH:mm"), "# version " + pm.Version }.Concat(manifest));
            log(L.T("done") + " " + pm.Version);
        }

        public List<string> Verify()
        {
            var problems = new List<string>();
            if (!IsInstalled) { problems.Add(L.T("modNotInstalled")); return problems; }
            foreach (var line in File.ReadAllLines(ManifestPath))
            {
                if (line.StartsWith("#")) continue;
                var parts = line.Split('|');
                if (parts.Length != 2) continue;
                string dst = Path.Combine(s.GameDir, parts[0]);
                if (!File.Exists(dst)) { problems.Add("missing: " + parts[0]); continue; }
                if (parts[1].Length == 64 && Sha256(dst) != parts[1]) problems.Add("changed: " + parts[0]);
            }
            string exe = Path.Combine(s.GameDir, "gta_sa.exe");
            if (File.Exists(exe) && new FileInfo(exe).Length != Exe10UsSize) problems.Add("gta_sa.exe is not 1.0 US");
            return problems;
        }

        public void Repair()
        {
            var problems = Verify();
            foreach (var p in problems) log("! " + p);
            if (problems.Count == 0) { log("OK — no problems found"); return; }
            Install(true);
        }

        public void Uninstall()
        {
            EnsureGameNotRunning();
            if (!IsInstalled) { log(L.T("modNotInstalled")); return; }
            foreach (var line in File.ReadAllLines(ManifestPath))
            {
                if (line.StartsWith("#")) continue;
                var parts = line.Split('|');
                if (parts.Length != 2 || parts[1] == "config") continue; // keep user configs
                string dst = Path.Combine(s.GameDir, parts[0]);
                if (File.Exists(dst)) { File.Delete(dst); log("delete: " + parts[0]); }
            }
            foreach (var name in OriginalsToBackup)
            {
                string b = Path.Combine(BackupDir, name);
                if (File.Exists(b)) { File.Copy(b, Path.Combine(s.GameDir, name), true); log("restore: " + name); }
            }
            if (Directory.Exists(ServerDir) && !Directory.EnumerateFileSystemEntries(ServerDir).Any()) Directory.Delete(ServerDir);
            File.Delete(ManifestPath);
            log(L.T("done") + " (logs kept in CoopAndreas\\logs)");
        }
    }

    // ------------------------------------------------------------------ public release repo (download without git)
    class Release
    {
        readonly Settings s;
        readonly Action<string> log;
        public Release(Settings settings, Action<string> logger) { s = settings; log = logger; }

        static WebClient Client()
        {
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
            var wc = new WebClient();
            wc.Headers[HttpRequestHeader.UserAgent] = "CoopAndreasManager";
            return wc;
        }

        // the commit sha of the release branch, so that manifest and files are read from the same snapshot.
        // Asked through git's own ref discovery: no API rate limit (60/hour per IP) and no CDN cache;
        // a branch name is never used because raw.githubusercontent.com caches branches for minutes,
        // which mixed an old manifest with new files ("checksum mismatch")
        string HeadSha()
        {
            Exception last = null;
            try
            {
                using (var wc = Client())
                {
                    wc.Headers[HttpRequestHeader.UserAgent] = "git/2.0";
                    string refs = wc.DownloadString("https://github.com/" + s.ReleaseRepo + ".git/info/refs?service=git-upload-pack");
                    var m = Regex.Match(refs, "([0-9a-f]{40}) refs/heads/main");
                    if (m.Success) return m.Groups[1].Value;
                }
            }
            catch (Exception ex) { last = ex; }
            try
            {
                using (var wc = Client())
                {
                    string json = wc.DownloadString("https://api.github.com/repos/" + s.ReleaseRepo + "/commits/main");
                    var m = Regex.Match(json, "\"sha\"\\s*:\\s*\"([0-9a-f]{40})\"");
                    if (m.Success) return m.Groups[1].Value;
                }
            }
            catch (Exception ex) { last = ex; }
            // name the repo: a 404 almost always means a wrong release repository in the settings
            throw new Exception("cannot reach the release repository '" + s.ReleaseRepo + "' on GitHub" + (last != null ? ": " + last.Message : ""));
        }

        string RawUrl(string sha, string path) { return "https://raw.githubusercontent.com/" + s.ReleaseRepo + "/" + sha + "/" + path; }

        public Manifest FetchRemote(out string sha)
        {
            sha = HeadSha();
            using (var wc = Client())
            {
                wc.Encoding = Encoding.UTF8;
                return Manifest.Parse(wc.DownloadString(RawUrl(sha, "manifest.txt")));
            }
        }

        // downloads changed files into the player package directory; returns true when the manager itself changed
        public bool Download(string pkgDir)
        {
            string sha;
            var remote = FetchRemote(out sha);
            log("release " + remote.Version + " (" + (sha.Length > 7 ? sha.Substring(0, 7) : sha) + ")");
            Directory.CreateDirectory(pkgDir);
            bool managerChanged = false;
            using (var wc = Client())
            {
                foreach (var kv in remote.Files)
                {
                    string dst = Path.Combine(pkgDir, kv.Key.Replace('/', '\\'));
                    if (File.Exists(dst) && Installer.Sha256(dst) == kv.Value) continue;
                    Directory.CreateDirectory(Path.GetDirectoryName(dst));
                    string tmp = dst + ".download";
                    wc.DownloadFile(RawUrl(sha, kv.Key), tmp);
                    if (Installer.Sha256(tmp) != kv.Value) { File.Delete(tmp); throw new Exception("checksum mismatch: " + kv.Key); }
                    if (File.Exists(dst)) File.Delete(dst);
                    File.Move(tmp, dst);
                    log("download: " + kv.Key);
                    if (kv.Key == Program.LauncherFileName) managerChanged = true;
                }
            }
            // files that are no longer part of the release
            var local = Manifest.Load(pkgDir);
            if (local != null)
                foreach (var old in local.Files.Keys.Where(k => !remote.Files.ContainsKey(k)))
                {
                    string p = Path.Combine(pkgDir, old.Replace('/', '\\'));
                    if (File.Exists(p)) File.Delete(p);
                }
            using (var wc = Client())
            {
                wc.Encoding = Encoding.UTF8;
                File.WriteAllText(Path.Combine(pkgDir, "manifest.txt"), wc.DownloadString(RawUrl(sha, "manifest.txt")), new UTF8Encoding(false));
            }
            // only the player launcher updates itself; the developer manager is built from source
            return managerChanged && Program.IsPlayerEdition && SelfUpdate(Path.Combine(pkgDir, Program.LauncherFileName));
        }

        // a running exe cannot be overwritten but can be renamed: swap it and ask for a restart
        static bool SelfUpdate(string newExe)
        {
            string me = Application.ExecutablePath;
            if (!File.Exists(newExe) || Installer.Sha256(newExe) == Installer.Sha256(me)) return false;
            string old = me + ".old";
            if (File.Exists(old)) File.Delete(old);
            File.Move(me, old);
            File.Copy(newExe, me);
            return true;
        }
    }

    // ------------------------------------------------------------------ git (developer only; never the upstream repo)
    class Git
    {
        readonly Settings s;
        public Git(Settings settings) { s = settings; }

        public bool RepoExists { get { return s.SourceDir.Length > 0 && Directory.Exists(Path.Combine(s.SourceDir, ".git")); } }

        public static string Exe
        {
            get
            {
                foreach (var p in new[] { @"C:\Program Files\Git\cmd\git.exe", @"C:\Program Files (x86)\Git\cmd\git.exe" })
                    if (File.Exists(p)) return p;
                return "git";
            }
        }

        public int Run(string args, out string output, string workDir = null)
        {
            var psi = new ProcessStartInfo(Exe, args)
            {
                WorkingDirectory = workDir ?? s.SourceDir, UseShellExecute = false, CreateNoWindow = true,
                RedirectStandardOutput = true, RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8,
            };
            psi.EnvironmentVariables["GIT_TERMINAL_PROMPT"] = "0";
            try
            {
                using (var p = Process.Start(psi))
                {
                    var err = p.StandardError.ReadToEndAsync();
                    string o = p.StandardOutput.ReadToEnd();
                    p.WaitForExit();
                    output = (o + err.Result).Trim();
                    return p.ExitCode;
                }
            }
            catch (System.ComponentModel.Win32Exception) { output = L.T("needGit"); return -1; }
            catch (Exception ex) { output = ex.Message; return -1; }
        }

        public string Get(string args, string workDir = null)
        {
            string o;
            return Run(args, out o, workDir) == 0 ? o : "";
        }

        public string OriginUrl(string workDir = null) { return Get("remote get-url origin", workDir); }

        public static bool IsUpstream(string url) { return url.IndexOf("Tornamic/CoopAndreas", StringComparison.OrdinalIgnoreCase) >= 0; }

        public bool IsDirty(string workDir = null) { return Get("status --porcelain", workDir).Trim().Length > 0; }
    }

    // ------------------------------------------------------------------ serial / pc id (same as launcher/pcid.h)
    static class Serial
    {
        const string Key = @"Software\CoopAndreas";

        public static string GetPcId()
        {
            using (var k = Registry.CurrentUser.CreateSubKey(Key))
            {
                string pcid = k.GetValue("pcid") as string;
                if (string.IsNullOrEmpty(pcid))
                {
                    pcid = Guid.NewGuid().ToString("D").ToUpperInvariant();
                    k.SetValue("pcid", pcid);
                }
                using (var md5 = MD5.Create())
                {
                    byte[] h = md5.ComputeHash(Encoding.ASCII.GetBytes(pcid));
                    return BitConverter.ToString(h, 0, 4).Replace("-", "");
                }
            }
        }

        public static string GetSerial()
        {
            using (var k = Registry.CurrentUser.CreateSubKey(Key)) return (k.GetValue("Serialkey") as string) ?? "";
        }

        public static void SetSerial(string v)
        {
            using (var k = Registry.CurrentUser.CreateSubKey(Key)) k.SetValue("Serialkey", v ?? "");
        }
    }

    static class Native
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        public static extern bool WritePrivateProfileString(string section, string key, string value, string file);
    }

    // ------------------------------------------------------------------ game / server launching shared by GUI and CLI
    static class GameLauncher
    {
        // writes nickname/ip/port into the client config of the given profile
        public static void WriteClientConfig(Settings s, int profile, string nick, string ip)
        {
            string userDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "GTA San Andreas User Files");
            Directory.CreateDirectory(userDir);
            string file = Path.Combine(userDir, profile > 0 ? "coopandreas_" + profile + ".ini" : "coopandreas.ini");
            Native.WritePrivateProfileString("config", "nickname", nick, file);
            Native.WritePrivateProfileString("config", "ip", ip, file);
            Native.WritePrivateProfileString("config", "port", s.Port.ToString(), file);
        }

        // profile 0 = normal play; windowIndex 0/1 = side-by-side placement, -1 = none; returns the logged command line
        public static string Launch(Settings s, int profile, string nick, string ip, int windowIndex, bool autoConnect, string extraArgs = "")
        {
            string serial = Serial.GetSerial();
            if (string.IsNullOrEmpty(serial)) throw new Exception(L.T("needSerial"));
            WriteClientConfig(s, profile, nick, ip);

            string args = "--coop -id " + Serial.GetPcId() + " -serial " + serial;
            if (profile > 0) args += " -profile " + profile;
            if (windowIndex >= 0) args += " --coopd" + windowIndex;
            if (autoConnect) args += " -autoconnect";
            if (!string.IsNullOrEmpty(extraArgs)) args += " " + extraArgs;
            Process.Start(new ProcessStartInfo(Path.Combine(s.GameDir, "gta_sa.exe"), args) { WorkingDirectory = s.GameDir, UseShellExecute = false });
            return "gta_sa.exe " + args.Replace(serial, "***");
        }

        // server writing straight into CoopAndreas\logs\server.log, independent of the manager process
        public static void StartDetachedServer(Installer inst)
        {
            string exe = Path.Combine(inst.ServerDir, "server.exe");
            if (!File.Exists(exe)) throw new Exception(L.T("needInstall"));
            StopServers(inst);
            Directory.CreateDirectory(inst.LogsDir);
            string log = Path.Combine(inst.LogsDir, "server.log");
            if (File.Exists(log)) File.Copy(log, Path.Combine(inst.LogsDir, "server.old.log"), true);
            Process.Start(new ProcessStartInfo("cmd.exe", "/c \"\"" + exe + "\" --no-colors > \"" + log + "\" 2>&1\"")
            {
                WorkingDirectory = inst.ServerDir, UseShellExecute = false, CreateNoWindow = true,
            });
        }

        public static Process FindServerProcess(Installer inst)
        {
            string exe = Path.Combine(inst.ServerDir, "server.exe");
            foreach (var pr in Process.GetProcessesByName("server"))
            {
                try { if (string.Equals(pr.MainModule.FileName, exe, StringComparison.OrdinalIgnoreCase)) return pr; } catch { }
            }
            return null;
        }

        // server.exe reads its port from server-config.ini next to it
        public static void WriteServerPort(Installer inst, int port)
        {
            Directory.CreateDirectory(inst.ServerDir);
            string cfg = Path.Combine(inst.ServerDir, "server-config.ini");
            var lines = File.Exists(cfg) ? File.ReadAllLines(cfg).Where(l => !l.TrimStart().StartsWith("port")).ToList() : new List<string> { "maxplayers = 8" };
            lines.Insert(0, "port = " + port);
            File.WriteAllLines(cfg, lines);
        }

        public static void StopServers(Installer inst)
        {
            string exe = Path.Combine(inst.ServerDir, "server.exe");
            foreach (var pr in Process.GetProcessesByName("server"))
            {
                try { if (string.Equals(pr.MainModule.FileName, exe, StringComparison.OrdinalIgnoreCase)) { pr.Kill(); pr.WaitForExit(3000); } } catch { }
            }
        }

        public static void StopGames()
        {
            foreach (var pr in Process.GetProcessesByName("gta_sa")) { try { pr.Kill(); } catch { } }
        }
    }

    // ------------------------------------------------------------------ developer operations shared by GUI and CLI
    class DevOps
    {
        readonly Settings s;
        readonly Action<string> log;
        readonly Func<string, string, string, int> runTool;
        public DevOps(Settings settings, Action<string> logger, Func<string, string, string, int> tool) { s = settings; log = logger; runTool = tool; }

        public bool Build()
        {
            string xm = File.Exists(@"C:\Program Files\xmake\xmake.exe") ? @"C:\Program Files\xmake\xmake.exe" : "xmake";
            if (!File.Exists(Path.Combine(s.SourceDir, @".xmake\windows\x86\xmake.conf")))
                runTool(xm, "f -p windows -a x86 -m release --toolchain=msvc -y", s.SourceDir);
            if (runTool(xm, "-y", s.SourceDir) != 0) return false;
            string cmd = Path.Combine(s.SourceDir, @"tools\manager\build.cmd");
            return runTool("cmd.exe", "/c \"" + cmd + "\"", Path.GetDirectoryName(cmd)) == 0;
        }

        // assembles the package into the local clone of the public release repo, commits and pushes it
        public void Publish(Func<string, bool> confirm)
        {
            var git = new Git(s);
            if (!git.RepoExists) throw new Exception(L.T("repoMissing"));
            if (!Directory.Exists(Path.Combine(s.ReleaseDir, ".git"))) throw new Exception(L.F("releaseDirMissing", s.ReleaseDir));
            string relOrigin = git.OriginUrl(s.ReleaseDir);
            if (relOrigin.Length == 0 || Git.IsUpstream(relOrigin) || relOrigin.IndexOf(s.ReleaseRepo, StringComparison.OrdinalIgnoreCase) < 0)
                throw new Exception(L.T("unsafeOrigin") + " (" + relOrigin + ")");
            if (git.IsDirty()) throw new Exception(L.T("dirtyTree"));

            // GPL-3: the published binaries must match source that is publicly available,
            // so the exact commit has to be pushed to our (public) source repo first
            string fetchOutput;
            git.Run("fetch origin", out fetchOutput);
            if (git.Get("rev-list --count origin/main..HEAD") != "0") throw new Exception(L.T("notPushed"));
            if (!confirm(relOrigin)) return;

            var inst = new Installer(s, log);
            inst.AssembleFromBuild(s.ReleaseDir, true);

            string srcDir = Path.Combine(s.ReleaseDir, "source");
            if (Directory.Exists(srcDir)) Directory.Delete(srcDir, true);
            File.Copy(Path.Combine(s.SourceDir, "LICENSE"), Path.Combine(s.ReleaseDir, "LICENSE"), true);

            // the developer manager is published next to the launcher (not part of the player package/manifest)
            string devManager = Path.Combine(s.SourceDir, @"build\manager\CoopAndreasManager.exe");
            if (File.Exists(devManager))
            {
                Directory.CreateDirectory(Path.Combine(s.ReleaseDir, "dev"));
                File.Copy(devManager, Path.Combine(s.ReleaseDir, @"dev\CoopAndreasManager.exe"), true);
            }

            string head = git.Get("rev-parse HEAD");
            string sourceUrl = Regex.Replace(git.OriginUrl(), @"\.git$", "");
            string readme = Path.Combine(s.ReleaseDir, "README.md");
            File.WriteAllText(readme,
                "# SA Dream Mod — release\n\n" +
                "Ready-to-install files of **SA Dream Mod** (a CoopAndreas based co-op for GTA San Andreas).\n" +
                "Готові файли **SA Dream Mod** (кооператив GTA San Andreas на основі CoopAndreas).\n\n" +
                "## ⬇️ [Download the launcher / Завантажити лаунчер](https://github.com/" + s.ReleaseRepo + "/raw/main/" + Program.LauncherFileName + ")\n\n" +
                "## ⬇️ [Download the developer manager / Завантажити менеджер розробника](https://github.com/" + s.ReleaseRepo + "/raw/main/dev/CoopAndreasManager.exe)\n\n" +
                "Or the latest one on the [Releases](https://github.com/" + s.ReleaseRepo + "/releases/latest) page / або на сторінці Releases.\n\n" +
                "## How to play / Як грати\n\n" +
                "1. Download `" + Program.LauncherFileName + "` (link above) / завантажте лаунчер (посилання вище).\n" +
                "2. Run it, check the game folder, press **Install** / запустіть, перевірте теку гри, натисніть **Встановити**.\n" +
                "3. Enter nickname, server IP and the beta key, press **PLAY** / введіть нік, IP і ключ, натисніть **ГРАТИ**.\n\n" +
                "`gta_sa.exe` 1.0 US is not included (Rockstar file) — the launcher asks for it if your game needs it.\n" +
                "`gta_sa.exe` 1.0 US не входить у реліз (файл Rockstar) — лаунчер попросить його, якщо потрібно.\n\n" +
                "- Source code of this version (GPL-3.0): " + sourceUrl + "/tree/" + head + "\n" +
                "- Based on [CoopAndreas](https://github.com/Tornamic/CoopAndreas) by Tornamic and contributors (GPL-3.0).\n",
                new UTF8Encoding(false));

            string version = Manifest.Load(s.ReleaseDir).Version;
            // files must be stored byte for byte, the manifest hashes them (no CRLF/LF conversion)
            File.WriteAllText(Path.Combine(s.ReleaseDir, ".gitattributes"), "* -text\n", new UTF8Encoding(false));
            if (runTool("git", "add -A --renormalize", s.ReleaseDir) != 0) return;
            if (runTool("git", "add -A", s.ReleaseDir) != 0) return;
            if (runTool("git", "commit -m \"release " + version + "\"", s.ReleaseDir) != 0) return;
            if (runTool("git", "push origin HEAD:main", s.ReleaseDir) != 0) return;
            CreateGitHubRelease(version, head);
        }

        static string FindGh()
        {
            foreach (var p in new[] { @"C:\Program Files\GitHub CLI\gh.exe", @"C:\Program Files (x86)\GitHub CLI\gh.exe",
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), @"Programs\GitHub CLI\gh.exe") })
                if (File.Exists(p)) return p;
            return null;
        }

        // a GitHub Release with the launcher attached: players just press "download" on the Releases page
        public void CreateGitHubRelease(string version, string sourceCommit)
        {
            string gh = FindGh();
            if (gh == null)
            {
                log("GitHub CLI (gh) is not installed: the files are published, the Releases page is skipped");
                return;
            }
            string tag = "v" + version;
            string notes = Path.Combine(Path.GetTempPath(), "sadream_release_notes.md");
            File.WriteAllText(notes,
                "**Download `" + Program.LauncherFileName + "` below, run it, press Install, then PLAY.**\n" +
                "**Завантажте `" + Program.LauncherFileName + "` нижче, запустіть, натисніть «Встановити», потім «ГРАТИ».**\n\n" +
                "If you already have the launcher, just press Update in it / якщо лаунчер уже є — натисніть «Оновити».\n\n" +
                "Source / код: https://github.com/" + Regex.Replace(new Git(s).OriginUrl(), @"^https://github\.com/|\.git$", "") + "/tree/" + sourceCommit + "\n",
                new UTF8Encoding(false));
            runTool(gh, "release create " + tag + " \"" + Path.Combine(s.ReleaseDir, Program.LauncherFileName) + "\" --repo " + s.ReleaseRepo +
                " --title \"SA Dream Mod " + version + "\" --notes-file \"" + notes + "\" --latest", s.ReleaseDir);
        }
    }

    // ------------------------------------------------------------------ main window
    class DevForm : Form
    {
        readonly Settings settings = Settings.Load();
        Installer installer;
        readonly Dictionary<Control, string> texts = new Dictionary<Control, string>();
        volatile bool busy;

        TabControl tabs;
        TextBox tbGame, tbSrc, tbZip, tbRelRepo, tbRelDir, tbNick, tbIp, tbPort, tbNick1, tbNick2, tbSerial, tbFilter, tbOutput, tbLog;
        Label lbExe, lbMod, lbPkg, lbBackup, lbPcId, lbServer, lbDev, lbUpdates;
        RadioButton rbPlayer, rbDev;
        Button btnBuild, btnPublish;
        ComboBox cbLogFile;
        CheckBox chkWarn, chkFollow;
        Button btnLang;
        System.Windows.Forms.Timer logTimer;
        long logPos;
        string logCurrentFile;
        readonly List<string> logLines = new List<string>();

        public DevForm()
        {
            L.Uk = settings.Ukrainian;
            installer = new Installer(settings, Log);
            Font = new Font("Segoe UI", 9.5f);
            ClientSize = new Size(900, 720);
            MinimumSize = new Size(760, 600);
            StartPosition = FormStartPosition.CenterScreen;
            try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

            var header = Theme.HeaderPanel(this, "SA Dream Mod · Developer", "build · install · test · publish", 66);
            btnLang = new Button { Anchor = AnchorStyles.Top | AnchorStyles.Right, Size = new Size(110, 30), Location = new Point(ClientSize.Width - 126, 18), Tag = "header" };
            Theme.StyleButton(btnLang, Color.FromArgb(52, 56, 70), Color.White);
            btnLang.Click += delegate { L.Uk = !L.Uk; settings.Ukrainian = L.Uk; settings.Save(); ApplyTexts(); RefreshStatus(); };
            Reg(btnLang, "lang");
            header.Controls.Add(btnLang);

            ClientSize = new Size(ClientSize.Width, ClientSize.Height + 34);
            tabs = new TabControl { Location = new Point(6, 72), Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right };
            tabs.Size = new Size(ClientSize.Width - 12, ClientSize.Height - 78);
            Controls.Add(tabs);

            BuildInstallTab();
            BuildLaunchTab();
            BuildLogsTab();
            BuildSettingsTab();

            ApplyTexts();
            RefreshStatus();

            logTimer = new System.Windows.Forms.Timer { Interval = 500 };
            logTimer.Tick += delegate { PollLog(); };
            logTimer.Start();
            // the server keeps running after the manager is closed (stop it in the server panel)
            BackColor = Theme.Back;
            Theme.Apply(this);
            Shown += delegate { if (settings.PlayerMode) RunBusy(() => CheckUpdates()); };
        }

        // ---- helpers
        void Reg(Control c, string key) { texts[c] = key; }
        void ApplyTexts()
        {
            Text = L.T("title");
            foreach (var kv in texts) kv.Key.Text = L.T(kv.Value);
        }
        Label Lbl(Control parent, string key, int x, int y, int w = 150)
        {
            var l = new Label { Location = new Point(x, y + 3), AutoSize = false, Size = new Size(w, 22) };
            if (key != null) Reg(l, key);
            parent.Controls.Add(l);
            return l;
        }
        static readonly string[] PrimaryKeys = { "install", "update", "launchTest", "startServer", "save" };
        static readonly string[] PlayKeys = { "launchGame" };
        static readonly string[] DangerKeys = { "uninstall", "killGames", "stopServer" };

        Button Btn(Control parent, string key, int x, int y, int w, EventHandler click)
        {
            var b = new Button { Location = new Point(x, y), Size = new Size(w, 32) };
            if (PrimaryKeys.Contains(key)) b.Tag = "primary";
            else if (PlayKeys.Contains(key)) b.Tag = "play";
            else if (DangerKeys.Contains(key)) b.Tag = "danger";
            Reg(b, key);
            b.Click += click;
            parent.Controls.Add(b);
            return b;
        }
        GroupBox Group(Control parent, string key, int x, int y, int w, int h)
        {
            var g = new GroupBox { Location = new Point(x, y), Size = new Size(w, h), Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
            Reg(g, key);
            parent.Controls.Add(g);
            return g;
        }
        TabPage Page(string key)
        {
            var p = new TabPage { Padding = new Padding(8), UseVisualStyleBackColor = true };
            Reg(p, key);
            tabs.TabPages.Add(p);
            return p;
        }

        void Log(string line)
        {
            if (tbOutput == null) return;
            if (tbOutput.InvokeRequired) { tbOutput.BeginInvoke(new Action<string>(Log), line); return; }
            tbOutput.AppendText(DateTime.Now.ToString("HH:mm:ss") + "  " + line + Environment.NewLine);
        }

        void RunBusy(Action work)
        {
            if (busy) { MessageBox.Show(L.T("busy")); return; }
            busy = true;
            UseWaitCursor = true;
            var t = new Thread(() =>
            {
                try { work(); }
                catch (Exception ex) { Log(L.F("failed", ex.Message)); }
                finally
                {
                    busy = false;
                    BeginInvoke(new Action(() => { UseWaitCursor = false; RefreshStatus(); }));
                }
            });
            t.IsBackground = true;
            t.Start();
        }

        // runs a console tool and streams its output to the log box; returns the exit code
        int RunTool(string exe, string args, string workDir)
        {
            Log("> " + exe + " " + args);
            var psi = new ProcessStartInfo(exe, args)
            {
                WorkingDirectory = workDir, UseShellExecute = false, CreateNoWindow = true,
                RedirectStandardOutput = true, RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8,
            };
            psi.EnvironmentVariables["XMAKE_COLORTERM"] = "nocolor";
            using (var p = new Process { StartInfo = psi })
            {
                DataReceivedEventHandler h = (o, e) => { if (e.Data != null) Log(StripAnsi(e.Data)); };
                p.OutputDataReceived += h;
                p.ErrorDataReceived += h;
                p.Start();
                p.BeginOutputReadLine();
                p.BeginErrorReadLine();
                p.WaitForExit();
                Log("exit code " + p.ExitCode);
                return p.ExitCode;
            }
        }

        static string StripAnsi(string s) { return Regex.Replace(s, @"\x1B\[[0-9;]*[A-Za-z]", ""); }

        DevOps Dev { get { return new DevOps(settings, Log, RunTool); } }

        // ---- Install tab
        void BuildInstallTab()
        {
            var p = Page("tabInstall");
            var g = Group(p, "status", 8, 6, 860, 130);
            lbExe = Lbl(g, null, 12, 22, 830);
            lbMod = Lbl(g, null, 12, 46, 830);
            lbPkg = Lbl(g, null, 12, 70, 830);
            lbBackup = Lbl(g, null, 12, 94, 830);

            var gu = Group(p, "updates", 8, 142, 860, 150);
            lbUpdates = Lbl(gu, null, 12, 20, 830);
            lbDev = Lbl(gu, null, 12, 42, 830);
            rbPlayer = new RadioButton { Location = new Point(12, 68), Size = new Size(410, 24), Checked = settings.PlayerMode };
            rbDev = new RadioButton { Location = new Point(430, 68), Size = new Size(410, 24), Checked = !settings.PlayerMode };
            Reg(rbPlayer, "modePlayer");
            Reg(rbDev, "modeDev");
            rbPlayer.CheckedChanged += delegate { settings.PlayerMode = rbPlayer.Checked; settings.Save(); RefreshStatus(); };
            gu.Controls.Add(rbPlayer);
            gu.Controls.Add(rbDev);
            Btn(gu, "checkUpdates", 12, 104, 200, delegate { RunBusy(() => CheckUpdates()); });
            Btn(gu, "update", 220, 104, 300, delegate { if (EnsureWritable()) RunBusy(() => UpdateAndInstall()); });
            btnPublish = Btn(gu, "publish", 528, 104, 316, delegate
            {
                RunBusy(() => Dev.Publish(url => (bool)Invoke(new Func<bool>(() =>
                    MessageBox.Show(L.F("confirmPublish", url), Text,
                        MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes))));
            });

            int y = 302;
            Btn(p, "install", 8, y, 280, delegate { if (EnsureWritable()) RunBusy(() => installer.Install(false)); });
            Btn(p, "repair", 296, y, 280, delegate { if (EnsureWritable()) RunBusy(() => installer.Repair()); });
            Btn(p, "uninstall", 584, y, 280, delegate
            {
                if (EnsureWritable() && MessageBox.Show(L.T("confirmUninstall"), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                    RunBusy(() => installer.Uninstall());
            });
            y += 38;
            btnBuild = Btn(p, "build", 8, y, 280, delegate { RunBusy(() => Dev.Build()); });
            Btn(p, "fixPerms", 296, y, 280, delegate { EnsureWritable(); });
            Btn(p, "refresh", 584, y, 280, delegate { RefreshStatus(); });

            tbOutput = new TextBox
            {
                Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both, WordWrap = false,
                Location = new Point(8, y + 40), Font = new Font("Consolas", 9f),
                Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right,
                BackColor = Color.FromArgb(24, 24, 28), ForeColor = Color.Gainsboro,
            };
            tbOutput.Size = new Size(856, 260);
            p.Controls.Add(tbOutput);
        }

        void CheckUpdates()
        {
            if (!settings.PlayerMode) return;
            string sha;
            var remote = new Release(settings, Log).FetchRemote(out sha);
            string installed = installer.InstalledVersion;
            string msg = installed == remote.Version ? L.F("upToDate", installed) : L.F("newVersion", remote.Version, installed.Length > 0 ? installed : "—");
            Log(msg);
            BeginInvoke(new Action(() => { lbUpdates.Text = msg; }));
        }

        void UpdateAndInstall()
        {
            if (settings.PlayerMode)
            {
                bool restart = new Release(settings, Log).Download(installer.PlayerPackageDir);
                installer.Install(true);
                if (restart) BeginInvoke(new Action(() => MessageBox.Show(L.T("selfUpdated"))));
            }
            else
            {
                var git = new Git(settings);
                if (!git.RepoExists) { Log(L.T("repoMissing")); return; }
                string origin = git.OriginUrl();
                if (origin.Length == 0 || Git.IsUpstream(origin)) { Log(L.T("unsafeOrigin")); return; }
                if (git.IsDirty()) { Log(L.T("dirtyTree")); return; }
                if (RunTool("git", "pull --ff-only origin main", settings.SourceDir) != 0) return;
                if (!Dev.Build()) return;
                installer.Install(true);
            }
        }

        bool EnsureWritable()
        {
            if (installer.IsGameDirWritable()) return true;
            if (MessageBox.Show(L.T("notWritable"), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes) return false;
            bool ok = installer.GrantGameDirAccess();
            RefreshStatus();
            return ok;
        }

        void RefreshStatus()
        {
            if (lbExe == null) return;
            installer = new Installer(settings, Log);
            lbExe.Text = installer.ExeState();
            lbExe.ForeColor = lbExe.Text == L.T("exeOk") ? Color.DarkGreen : Color.DarkRed;
            lbMod.Text = installer.IsInstalled ? L.F("modInstalled", installer.InstalledVersion) : L.T("modNotInstalled");
            if (settings.PlayerMode)
            {
                var pm = Manifest.Load(installer.PlayerPackageDir);
                lbPkg.Text = pm != null ? L.F("pkgReady", pm.Version) : L.T("pkgMissing");
            }
            else
            {
                string dll = Path.Combine(installer.BuildDir, "CoopAndreasSA.dll");
                lbPkg.Text = File.Exists(dll) ? L.F("pkgReady", "build " + File.GetLastWriteTime(dll).ToString("yyyy-MM-dd HH:mm")) : L.T("buildMissing");
            }
            lbBackup.Text = Directory.Exists(installer.BackupDir) ? L.T("backupFound") : "";
            lbBackup.ForeColor = SystemColors.ControlText;
            if (Directory.Exists(settings.GameDir) && !installer.IsGameDirWritable()) { lbBackup.Text = L.T("writableNo"); lbBackup.ForeColor = Color.DarkRed; }

            var git = new Git(settings);
            bool dev = git.RepoExists;
            rbDev.Enabled = dev;
            btnBuild.Enabled = dev && !settings.PlayerMode;
            btnPublish.Enabled = dev && !settings.PlayerMode;
            lbDev.Text = dev ? L.F("devInfo", git.OriginUrl(), git.Get("rev-parse --abbrev-ref HEAD"), git.Get("log -1 --format=\"%h %s\"")) : "";
            UpdateServerLabel();
        }

        // ---- Launch tab
        void BuildLaunchTab()
        {
            var p = Page("tabLaunch");
            var gs = Group(p, "serial", 8, 6, 860, 130);
            Lbl(gs, "pcid", 12, 22, 120);
            lbPcId = new Label { Location = new Point(135, 25), AutoSize = true, Font = new Font("Consolas", 10f, FontStyle.Bold) };
            gs.Controls.Add(lbPcId);
            try { lbPcId.Text = "/gen " + Serial.GetPcId(); } catch (Exception ex) { lbPcId.Text = ex.Message; }
            Btn(gs, "copyCmd", 330, 18, 170, delegate { Clipboard.SetText(lbPcId.Text); });
            Btn(gs, "discord", 510, 18, 150, delegate { Process.Start("https://discord.gg/Z3ugSgFJMU"); });
            var hint = Lbl(gs, "serialHint", 12, 52, 830);
            hint.Height = 38;
            Lbl(gs, "serialKey", 12, 94, 120);
            tbSerial = new TextBox { Location = new Point(135, 94), Width = 200, Text = Serial.GetSerial() };
            tbSerial.TextChanged += delegate { Serial.SetSerial(tbSerial.Text.Trim()); };
            gs.Controls.Add(tbSerial);

            var gp = Group(p, "play", 8, 142, 420, 150);
            gp.Anchor = AnchorStyles.Top | AnchorStyles.Left;
            Lbl(gp, "nick", 12, 24, 110); tbNick = new TextBox { Location = new Point(125, 24), Width = 200, Text = settings.Nick }; gp.Controls.Add(tbNick);
            Lbl(gp, "ip", 12, 54, 110); tbIp = new TextBox { Location = new Point(125, 54), Width = 140, Text = settings.Ip }; gp.Controls.Add(tbIp);
            Lbl(gp, "port", 272, 54, 50); tbPort = new TextBox { Location = new Point(325, 54), Width = 70, Text = settings.Port.ToString() }; gp.Controls.Add(tbPort);
            Btn(gp, "launchGame", 12, 100, 200, delegate { SaveLaunchFields(); LaunchGame(0, settings.Nick, settings.Ip, -1); });

            var gsv = Group(p, "server", 440, 142, 428, 150);
            lbServer = Lbl(gsv, null, 12, 26, 400);
            Btn(gsv, "startServer", 12, 60, 190, delegate { StartServer(); });
            Btn(gsv, "stopServer", 212, 60, 190, delegate { StopServer(); });
            Btn(gsv, "svOpenPanel", 12, 100, 390, delegate
            {
                using (var panel = new ServerPanel(settings, installer, ip => { tbIp.Text = ip; SaveLaunchFields(); }))
                    panel.ShowDialog(this);
                UpdateServerLabel();
            });

            var gt = Group(p, "test", 8, 300, 860, 170);
            Lbl(gt, "nick1", 12, 26, 120); tbNick1 = new TextBox { Location = new Point(135, 26), Width = 160, Text = settings.Nick1 }; gt.Controls.Add(tbNick1);
            Lbl(gt, "nick2", 320, 26, 120); tbNick2 = new TextBox { Location = new Point(443, 26), Width = 160, Text = settings.Nick2 }; gt.Controls.Add(tbNick2);
            Btn(gt, "launchTest", 12, 64, 280, delegate
            {
                SaveLaunchFields();
                if (!StartServer()) return;
                if (!LaunchGame(1, settings.Nick1, "127.0.0.1", 0, true)) return;
                Thread.Sleep(1500);
                LaunchGame(2, settings.Nick2, "127.0.0.1", 1, true);
            });
            Btn(gt, "killGames", 302, 64, 240, delegate
            {
                foreach (var pr in Process.GetProcessesByName("gta_sa")) { try { pr.Kill(); } catch { } }
            });
            var th = Lbl(gt, "testHint", 12, 104, 830);
            th.Height = 44;
        }

        void SaveLaunchFields()
        {
            settings.Nick = tbNick.Text.Trim();
            settings.Ip = tbIp.Text.Trim();
            int port; if (int.TryParse(tbPort.Text.Trim(), out port)) settings.Port = port;
            settings.Nick1 = tbNick1.Text.Trim();
            settings.Nick2 = tbNick2.Text.Trim();
            settings.Save();
        }

        // profile 0 = normal play; windowIndex 0/1 = side-by-side debug placement, -1 = none
        bool LaunchGame(int profile, string nick, string ip, int windowIndex, bool autoConnect = false)
        {
            if (!installer.IsInstalled) { MessageBox.Show(L.T("needInstall")); return false; }
            if (string.IsNullOrEmpty(Serial.GetSerial())) { tabs.SelectedIndex = 1; MessageBox.Show(L.T("needSerial")); return false; }
            try { Log("launch: " + GameLauncher.Launch(settings, profile, nick, ip, windowIndex, autoConnect)); return true; }
            catch (Exception ex) { MessageBox.Show(L.F("failed", ex.Message)); return false; }
        }

        bool ServerRunning { get { return GameLauncher.FindServerProcess(installer) != null; } }

        void UpdateServerLabel()
        {
            if (lbServer == null) return;
            var pr = GameLauncher.FindServerProcess(installer);
            lbServer.Text = pr != null ? L.F("serverRunning", pr.Id) : L.T("serverStopped");
            lbServer.ForeColor = pr != null ? Theme.Ok : Theme.Muted;
        }

        // detached like in the player launcher: it survives closing the manager and writes server.log itself
        bool StartServer()
        {
            if (ServerRunning) return true;
            try
            {
                GameLauncher.WriteServerPort(installer, settings.Port);
                GameLauncher.StartDetachedServer(installer);
                Log("server started, log: " + Path.Combine(installer.LogsDir, "server.log"));
            }
            catch (Exception ex) { MessageBox.Show(L.F("failed", ex.Message)); return false; }
            UpdateServerLabel();
            return true;
        }

        void StopServer()
        {
            GameLauncher.StopServers(installer);
            UpdateServerLabel();
        }

        // ---- Logs tab
        void BuildLogsTab()
        {
            var p = Page("tabLogs");
            Lbl(p, "logFile", 8, 8, 60);
            cbLogFile = new ComboBox { Location = new Point(70, 8), Width = 220, DropDownStyle = ComboBoxStyle.DropDownList };
            cbLogFile.DropDown += delegate { FillLogList(); };
            cbLogFile.SelectedIndexChanged += delegate { OpenLog(); };
            p.Controls.Add(cbLogFile);
            Lbl(p, "filter", 300, 8, 60);
            tbFilter = new TextBox { Location = new Point(362, 8), Width = 150 };
            tbFilter.TextChanged += delegate { RenderLog(); };
            p.Controls.Add(tbFilter);
            chkWarn = new CheckBox { Location = new Point(522, 8), Size = new Size(230, 24) };
            Reg(chkWarn, "onlyWarn");
            chkWarn.CheckedChanged += delegate { RenderLog(); };
            p.Controls.Add(chkWarn);
            chkFollow = new CheckBox { Location = new Point(522, 34), Size = new Size(150, 24), Checked = true };
            Reg(chkFollow, "follow");
            p.Controls.Add(chkFollow);
            Btn(p, "reload", 8, 36, 140, delegate { OpenLog(); });
            Btn(p, "openFolder", 156, 36, 200, delegate
            {
                Directory.CreateDirectory(installer.LogsDir);
                Process.Start("explorer.exe", installer.LogsDir);
            });
            tbLog = new TextBox
            {
                Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Both, WordWrap = false,
                Location = new Point(8, 74), Font = new Font("Consolas", 9f),
                Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right,
                BackColor = Color.FromArgb(24, 24, 28), ForeColor = Color.Gainsboro,
            };
            tbLog.Size = new Size(856, 560);
            p.Controls.Add(tbLog);
            FillLogList();
        }

        void FillLogList()
        {
            string cur = cbLogFile.SelectedItem as string;
            cbLogFile.Items.Clear();
            var names = new List<string> { "server.log", "client.log", "client_1.log", "client_2.log" };
            if (Directory.Exists(installer.LogsDir))
                foreach (var f in Directory.GetFiles(installer.LogsDir, "*.log"))
                    if (!names.Contains(Path.GetFileName(f), StringComparer.OrdinalIgnoreCase)) names.Add(Path.GetFileName(f));
            foreach (var n in names) cbLogFile.Items.Add(n);
            cbLogFile.SelectedItem = cur ?? "server.log";
        }

        void OpenLog()
        {
            logCurrentFile = cbLogFile.SelectedItem == null ? null : Path.Combine(installer.LogsDir, (string)cbLogFile.SelectedItem);
            logPos = 0;
            logLines.Clear();
            tbLog.Clear();
            PollLog();
        }

        void PollLog()
        {
            UpdateServerLabel();
            if (logCurrentFile == null || !File.Exists(logCurrentFile)) return;
            try
            {
                using (var fs = new FileStream(logCurrentFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                {
                    if (fs.Length < logPos) { logPos = 0; logLines.Clear(); tbLog.Clear(); } // file recreated
                    if (fs.Length == logPos) return;
                    fs.Seek(logPos, SeekOrigin.Begin);
                    var reader = new StreamReader(fs, Encoding.UTF8);
                    string chunk = reader.ReadToEnd();
                    logPos = fs.Length;
                    var newLines = chunk.Replace("\r", "").Split('\n').Where(x => x.Length > 0).ToList();
                    logLines.AddRange(newLines);
                    if (logLines.Count > 20000) logLines.RemoveRange(0, logLines.Count - 20000);
                    var visible = newLines.Where(Matches).ToList();
                    if (visible.Count > 0) tbLog.AppendText(string.Join(Environment.NewLine, visible) + Environment.NewLine);
                    if (chkFollow.Checked) { tbLog.SelectionStart = tbLog.TextLength; tbLog.ScrollToCaret(); }
                }
            }
            catch (IOException) { }
        }

        bool Matches(string line)
        {
            if (chkWarn.Checked && line.IndexOf("[warn]", StringComparison.OrdinalIgnoreCase) < 0 &&
                line.IndexOf("[error]", StringComparison.OrdinalIgnoreCase) < 0)
                return false;
            string f = tbFilter.Text;
            return f.Length == 0 || line.IndexOf(f, StringComparison.OrdinalIgnoreCase) >= 0;
        }

        void RenderLog()
        {
            tbLog.Text = string.Join(Environment.NewLine, logLines.Where(Matches)) + Environment.NewLine;
            tbLog.SelectionStart = tbLog.TextLength;
            tbLog.ScrollToCaret();
        }

        // ---- Settings tab
        void BuildSettingsTab()
        {
            var p = Page("tabSettings");
            int y = 12;
            tbGame = PathRow(p, "gameDir", ref y, settings.GameDir, 1);
            tbZip = PathRow(p, "addZip", ref y, settings.AdditionalZip, 2);
            tbRelRepo = PathRow(p, "releaseRepo", ref y, settings.ReleaseRepo, 0);
            tbSrc = PathRow(p, "srcDir", ref y, settings.SourceDir, 1);
            tbRelDir = PathRow(p, "releaseDir", ref y, settings.ReleaseDir, 1);
            Btn(p, "save", 230, y + 6, 160, delegate
            {
                settings.GameDir = tbGame.Text.Trim();
                settings.AdditionalZip = tbZip.Text.Trim();
                settings.ReleaseRepo = Settings.NormalizeRepo(tbRelRepo.Text);
                tbRelRepo.Text = settings.ReleaseRepo;
                settings.SourceDir = tbSrc.Text.Trim();
                settings.ReleaseDir = tbRelDir.Text.Trim();
                settings.Save();
                RefreshStatus();
                FillLogList();
            });
        }

        // kind: 0 = plain text, 1 = folder, 2 = zip file or folder
        TextBox PathRow(Control p, string key, ref int y, string value, int kind)
        {
            Lbl(p, key, 8, y, 220);
            var tb = new TextBox { Location = new Point(230, y), Width = 500, Text = value };
            p.Controls.Add(tb);
            if (kind > 0)
            {
                Btn(p, "browse", 740, y - 3, 110, delegate
                {
                    if (kind == 1)
                    {
                        using (var d = new FolderBrowserDialog { SelectedPath = tb.Text }) if (d.ShowDialog() == DialogResult.OK) tb.Text = d.SelectedPath;
                    }
                    else
                    {
                        using (var d = new OpenFileDialog { Filter = "zip / exe|*.zip;gta_sa.exe|*.*|*.*" })
                            if (d.ShowDialog() == DialogResult.OK) tb.Text = d.FileName.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) ? Path.GetDirectoryName(d.FileName) : d.FileName;
                    }
                });
            }
            y += 40;
            return tb;
        }
    }

    // ------------------------------------------------------------------ look & feel shared by both editions
    static class Theme
    {
        public static readonly Color Header = Color.FromArgb(28, 30, 38);
        public static readonly Color HeaderText = Color.White;
        public static readonly Color HeaderSub = Color.FromArgb(170, 176, 190);
        public static readonly Color Back = Color.FromArgb(243, 244, 247);
        public static readonly Color Card = Color.White;
        public static readonly Color Text = Color.FromArgb(30, 32, 40);
        public static readonly Color Muted = Color.FromArgb(105, 110, 125);
        public static readonly Color Accent = Color.FromArgb(0, 103, 192);
        public static readonly Color Play = Color.FromArgb(16, 137, 62);
        public static readonly Color Danger = Color.FromArgb(196, 43, 28);
        public static readonly Color Neutral = Color.FromArgb(226, 229, 235);
        public static readonly Color Ok = Color.FromArgb(16, 124, 16);
        public static readonly Color Warn = Color.FromArgb(202, 80, 16);

        public static Font Base(float size = 10f, FontStyle style = FontStyle.Regular) { return new Font("Segoe UI", size, style); }
        public static Font Semi(float size) { return new Font("Segoe UI Semibold", size); }

        public static void StyleButton(Button b, Color back, Color fore)
        {
            b.FlatStyle = FlatStyle.Flat;
            b.FlatAppearance.BorderSize = 0;
            b.FlatAppearance.MouseOverBackColor = ControlPaint.Light(back, 0.15f);
            b.FlatAppearance.MouseDownBackColor = ControlPaint.Dark(back, 0.05f);
            b.BackColor = back;
            b.ForeColor = fore;
            b.Cursor = Cursors.Hand;
            b.UseVisualStyleBackColor = false;
        }

        // flat neutral buttons + readable fonts everywhere; buttons tagged "primary"/"play"/"danger" get colors
        public static void Apply(Control root)
        {
            foreach (Control c in root.Controls)
            {
                var b = c as Button;
                if (b != null)
                {
                    string tag = b.Tag as string;
                    if (tag == "primary") StyleButton(b, Accent, Color.White);
                    else if (tag == "play") StyleButton(b, Play, Color.White);
                    else if (tag == "danger") StyleButton(b, Danger, Color.White);
                    else if (tag != "header") StyleButton(b, Neutral, Text);
                }
                else if (c is GroupBox) { c.ForeColor = Text; c.Font = Semi(10f); foreach (Control cc in c.Controls) cc.Font = Base(9.75f); }
                else if (c is TabPage) c.BackColor = Back;
                Apply(c);
            }
        }

        public static Panel HeaderPanel(Form f, string title, string subtitle, int height)
        {
            var h = new Panel { Dock = DockStyle.Top, Height = height, BackColor = Header };
            h.Controls.Add(new Label { Text = title, ForeColor = HeaderText, Font = Semi(17f), AutoSize = true, Location = new Point(18, 10), BackColor = Header });
            h.Controls.Add(new Label { Text = subtitle, ForeColor = HeaderSub, Font = Base(9.5f), AutoSize = true, Location = new Point(20, 44), BackColor = Header });
            f.Controls.Add(h);
            return h;
        }
    }

    // ------------------------------------------------------------------ server panel (both editions)
    // The server runs detached (cmd → server.log), so it keeps running when the launcher is closed;
    // the panel just tails the log and re-attaches to a running server.
    class ServerPanel : Form
    {
        readonly Settings settings;
        readonly Installer installer;
        readonly Action<string> useLocalIp;
        Label lbState, lbPortCap, lbAddrCap, lbPlayersCap, lbLogCap, lbInfo;
        Button btnStart, btnStop, btnCopy, btnUseLocal;
        TextBox tbPort, tbLog;
        ListBox lbAddresses, lbPlayers;
        System.Windows.Forms.Timer timer;
        long logPos;
        readonly List<string> players = new List<string>();
        readonly HashSet<string> paused = new HashSet<string>();
        string publicIp;

        public ServerPanel(Settings s, Installer inst, Action<string> onUseLocalIp)
        {
            settings = s;
            installer = inst;
            useLocalIp = onUseLocalIp;
            Text = L.T("svTitle") + " — SA Dream Mod";
            Font = Theme.Base();
            BackColor = Theme.Back;
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterParent;
            ClientSize = new Size(620, 680);
            try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }
            Theme.HeaderPanel(this, L.T("svTitle"), L.T("svSubtitle"), 72);

            int x = 18, w = ClientSize.Width - 36;
            var top = Card(x, 88, w, 96);
            lbState = new Label { Location = new Point(14, 12), Size = new Size(300, 26), Font = Theme.Semi(13f) };
            top.Controls.Add(lbState);
            lbPortCap = new Label { Location = new Point(14, 56), Size = new Size(90, 24), ForeColor = Theme.Muted, Text = L.T("svPort") };
            top.Controls.Add(lbPortCap);
            tbPort = new TextBox { Location = new Point(104, 53), Width = 70, Text = settings.Port.ToString(), Font = Theme.Base(10.5f), BorderStyle = BorderStyle.FixedSingle };
            top.Controls.Add(tbPort);
            btnStart = new Button { Location = new Point(w - 330, 22), Size = new Size(316, 48), Tag = "play", Font = Theme.Semi(11.5f), Text = L.T("svStart") };
            btnStart.Click += delegate { StartServer(); };
            top.Controls.Add(btnStart);
            btnStop = new Button { Location = new Point(w - 330, 22), Size = new Size(316, 48), Tag = "danger", Font = Theme.Semi(11.5f), Text = L.T("svStop") };
            btnStop.Click += delegate { GameLauncher.StopServers(installer); RefreshState(); };
            top.Controls.Add(btnStop);

            var mid = Card(x, 196, w, 196);
            lbAddrCap = new Label { Location = new Point(14, 10), AutoSize = true, Font = Theme.Semi(11f), Text = L.T("svAddresses") };
            mid.Controls.Add(lbAddrCap);
            lbAddresses = new ListBox { Location = new Point(14, 38), Size = new Size(w - 200, 110), Font = Theme.Base(10f), BorderStyle = BorderStyle.FixedSingle, IntegralHeight = false };
            mid.Controls.Add(lbAddresses);
            btnCopy = new Button { Location = new Point(w - 176, 38), Size = new Size(162, 34), Tag = "primary", Text = L.T("svCopy") };
            btnCopy.Click += delegate { CopySelectedIp(); };
            mid.Controls.Add(btnCopy);
            btnUseLocal = new Button { Location = new Point(w - 176, 80), Size = new Size(162, 50), Text = L.T("svUseLocal") };
            btnUseLocal.Click += delegate { if (useLocalIp != null) useLocalIp("127.0.0.1"); lbInfo.Text = "127.0.0.1"; };
            mid.Controls.Add(btnUseLocal);
            lbInfo = new Label { Location = new Point(14, 156), Size = new Size(w - 28, 24), ForeColor = Theme.Ok };
            mid.Controls.Add(lbInfo);

            var bottom = Card(x, 404, w, 258);
            lbPlayersCap = new Label { Location = new Point(14, 10), AutoSize = true, Font = Theme.Semi(11f) };
            bottom.Controls.Add(lbPlayersCap);
            lbPlayers = new ListBox { Location = new Point(14, 38), Size = new Size(180, 206), Font = Theme.Base(10f), BorderStyle = BorderStyle.FixedSingle, IntegralHeight = false };
            bottom.Controls.Add(lbPlayers);
            lbLogCap = new Label { Location = new Point(208, 10), AutoSize = true, Font = Theme.Semi(11f), Text = L.T("svLog") };
            bottom.Controls.Add(lbLogCap);
            tbLog = new TextBox
            {
                Location = new Point(208, 38), Size = new Size(w - 222, 206), Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical,
                Font = new Font("Consolas", 8.75f), BackColor = Color.FromArgb(24, 24, 28), ForeColor = Color.Gainsboro, WordWrap = false,
            };
            bottom.Controls.Add(tbLog);

            Theme.Apply(this);
            FillAddresses();
            ReloadLog();
            RefreshState();
            timer = new System.Windows.Forms.Timer { Interval = 1000 };
            timer.Tick += delegate { TailLog(); RefreshState(); };
            timer.Start();
            FormClosed += delegate { timer.Stop(); };
            new Thread(FetchPublicIp) { IsBackground = true }.Start();
        }

        Panel Card(int x, int y, int w, int h)
        {
            var c = new Panel { Location = new Point(x, y), Size = new Size(w, h), BackColor = Theme.Card };
            Controls.Add(c);
            return c;
        }

        string LogPath { get { return Path.Combine(installer.LogsDir, "server.log"); } }

        void RefreshState()
        {
            bool installed = File.Exists(Path.Combine(installer.ServerDir, "server.exe"));
            bool running = GameLauncher.FindServerProcess(installer) != null;
            lbState.Text = !installed ? L.T("svNotInstalled") : running ? L.T("svRunning") : L.T("svStopped");
            lbState.ForeColor = running ? Theme.Ok : installed ? Theme.Muted : Theme.Danger;
            if (!installed) lbState.Font = Theme.Base(10f);
            // only the button that makes sense right now is shown
            btnStart.Visible = !running;
            btnStart.Enabled = installed;
            btnStop.Visible = running;
            tbPort.Enabled = !running;
            if (!running && players.Count > 0) { players.Clear(); paused.Clear(); }
            UpdatePlayers();
        }

        void StartServer()
        {
            int port;
            if (!int.TryParse(tbPort.Text.Trim(), out port) || port < 1 || port > 65535) { tbPort.Focus(); return; }
            settings.Port = port;
            settings.Save();
            try
            {
                GameLauncher.WriteServerPort(installer, port);
                GameLauncher.StartDetachedServer(installer);
            }
            catch (Exception ex) { MessageBox.Show(L.F("failed", ex.Message), Text); return; }
            players.Clear();
            paused.Clear();
            logPos = 0;
            tbLog.Clear();
            FillAddresses();
            Thread.Sleep(300);
            RefreshState();
        }

        // IPv4 addresses of active adapters, VPN adapters first (that is what friends usually use)
        void FillAddresses()
        {
            lbAddresses.Items.Clear();
            var items = new List<KeyValuePair<int, string>>();
            try
            {
                foreach (var ni in System.Net.NetworkInformation.NetworkInterface.GetAllNetworkInterfaces())
                {
                    if (ni.OperationalStatus != System.Net.NetworkInformation.OperationalStatus.Up) continue;
                    if (ni.NetworkInterfaceType == System.Net.NetworkInformation.NetworkInterfaceType.Loopback) continue;
                    foreach (var ua in ni.GetIPProperties().UnicastAddresses)
                    {
                        if (ua.Address.AddressFamily != System.Net.Sockets.AddressFamily.InterNetwork) continue;
                        string ip = ua.Address.ToString();
                        if (ip.StartsWith("169.254.")) continue;
                        string name = ni.Description + " " + ni.Name;
                        string kind; int order;
                        if (name.IndexOf("Radmin", StringComparison.OrdinalIgnoreCase) >= 0) { kind = "Radmin VPN"; order = 0; }
                        else if (name.IndexOf("ZeroTier", StringComparison.OrdinalIgnoreCase) >= 0) { kind = "ZeroTier"; order = 0; }
                        else if (name.IndexOf("Hamachi", StringComparison.OrdinalIgnoreCase) >= 0) { kind = "Hamachi"; order = 0; }
                        else if (name.IndexOf("Tailscale", StringComparison.OrdinalIgnoreCase) >= 0) { kind = "Tailscale"; order = 0; }
                        else if (name.IndexOf("Virtual", StringComparison.OrdinalIgnoreCase) >= 0 || name.IndexOf("vEthernet", StringComparison.OrdinalIgnoreCase) >= 0) continue;
                        else { kind = L.T("svLan"); order = 1; }
                        items.Add(new KeyValuePair<int, string>(order, kind + " — " + ip));
                    }
                }
            }
            catch { }
            foreach (var it in items.OrderBy(i => i.Key)) lbAddresses.Items.Add(it.Value);
            if (!string.IsNullOrEmpty(publicIp)) lbAddresses.Items.Add(L.F("svInternet", settings.Port) + " — " + publicIp);
            if (lbAddresses.Items.Count > 0) lbAddresses.SelectedIndex = 0;
        }

        void FetchPublicIp()
        {
            try
            {
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                using (var wc = new WebClient())
                {
                    string ip = wc.DownloadString("https://api.ipify.org").Trim();
                    if (Regex.IsMatch(ip, @"^\d+\.\d+\.\d+\.\d+$"))
                    {
                        publicIp = ip;
                        BeginInvoke(new Action(FillAddresses));
                    }
                }
            }
            catch { }
        }

        void CopySelectedIp()
        {
            var item = lbAddresses.SelectedItem as string;
            if (item == null) return;
            string ip = item.Substring(item.LastIndexOf(' ') + 1);
            try { Clipboard.SetText(ip); lbInfo.Text = L.F("svCopied", ip); } catch { }
        }

        void ReloadLog()
        {
            logPos = 0;
            players.Clear();
            paused.Clear();
            tbLog.Clear();
            if (GameLauncher.FindServerProcess(installer) != null) TailLog();
        }

        void TailLog()
        {
            if (!File.Exists(LogPath)) return;
            try
            {
                using (var fs = new FileStream(LogPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                {
                    if (fs.Length < logPos) { logPos = 0; tbLog.Clear(); players.Clear(); paused.Clear(); }
                    if (fs.Length == logPos) return;
                    fs.Seek(logPos, SeekOrigin.Begin);
                    string chunk = new StreamReader(fs, Encoding.UTF8).ReadToEnd();
                    logPos = fs.Length;
                    var lines = chunk.Replace("\r", "").Split('\n').Where(l => l.Length > 0).ToList();
                    foreach (var line in lines) ParsePlayers(line);
                    // keep the log box short: the entity spam is in the file, the panel shows what a host cares about
                    var shown = lines.Where(l => l.IndexOf("] SPAWN", StringComparison.Ordinal) < 0 && l.IndexOf("] REMOVE", StringComparison.Ordinal) < 0 &&
                                                 l.IndexOf("syncer of id", StringComparison.Ordinal) < 0 && l.IndexOf("someone else's", StringComparison.Ordinal) < 0).ToList();
                    if (shown.Count > 0) tbLog.AppendText(string.Join(Environment.NewLine, shown) + Environment.NewLine);
                    if (tbLog.Lines.Length > 400) tbLog.Lines = tbLog.Lines.Skip(tbLog.Lines.Length - 300).ToArray();
                    tbLog.SelectionStart = tbLog.TextLength;
                    tbLog.ScrollToCaret();
                }
            }
            catch (IOException) { }
        }

        static readonly Regex ReJoin = new Regex(@"freeId \d+ name (.+?) version");
        static readonly Regex ReLeave = new Regex(@"\[net\] (.+?) disconnected");
        static readonly Regex RePause = new Regex(@"\[net\] (.+?) (opened|closed) the pause menu");

        void ParsePlayers(string line)
        {
            var m = ReJoin.Match(line);
            if (m.Success) { if (!players.Contains(m.Groups[1].Value)) players.Add(m.Groups[1].Value); return; }
            m = ReLeave.Match(line);
            if (m.Success) { players.Remove(m.Groups[1].Value); paused.Remove(m.Groups[1].Value); return; }
            m = RePause.Match(line);
            if (m.Success) { if (m.Groups[2].Value == "opened") paused.Add(m.Groups[1].Value); else paused.Remove(m.Groups[1].Value); }
        }

        void UpdatePlayers()
        {
            lbPlayersCap.Text = L.F("svPlayers", players.Count);
            var items = players.Select(p => paused.Contains(p) ? p + L.T("svPaused") : p).ToArray();
            if (!items.SequenceEqual(lbPlayers.Items.Cast<string>()))
            {
                lbPlayers.Items.Clear();
                lbPlayers.Items.AddRange(items);
            }
        }
    }

    // ------------------------------------------------------------------ simple launcher for players
    class PlayerForm : Form
    {
        readonly Settings settings = Settings.Load();
        Installer installer;
        volatile bool busy;
        string remoteVersion;

        Label lbTitle, lbSub, lbFolderCap, lbFolder, lbExeCap, lbArchive, lbModState, lbUpdate, lbStatus, lbPlayerCap, lbKeyCap, lbNick, lbIp, lbPort, lbKey, lbIpHint;
        Button btnLang, btnFolder, btnExe, btnMain, btnPlay, btnHowKey, btnLogs, btnRepair, btnUninstall, btnServer;
        TextBox tbNick, tbIp, tbPort, tbKey;
        Panel cardGame, cardPlayer;
        readonly List<Button> actionButtons = new List<Button>();

        public PlayerForm()
        {
            L.Uk = settings.Ukrainian;
            installer = new Installer(settings, SetStatus);
            Text = "SA Dream Mod";
            Font = Theme.Base();
            BackColor = Theme.Back;
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(540, 672);
            try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

            var header = Theme.HeaderPanel(this, L.T("pTitle"), L.T("pSubtitle"), 72);
            lbTitle = (Label)header.Controls[0];
            lbSub = (Label)header.Controls[1];
            btnLang = new Button { Size = new Size(96, 30), Location = new Point(ClientSize.Width - 112, 20), Tag = "header", Font = Theme.Base(9.5f) };
            Theme.StyleButton(btnLang, Color.FromArgb(52, 56, 70), Color.White);
            btnLang.Click += delegate { L.Uk = !L.Uk; settings.Ukrainian = L.Uk; settings.Save(); ApplyTexts(); RefreshState(); };
            header.Controls.Add(btnLang);

            int x = 18, w = ClientSize.Width - 36;

            // --- game card
            cardGame = Card(x, 88, w, 172);
            lbFolderCap = Caption(cardGame, 14, 10);
            lbFolder = new Label { Location = new Point(14, 34), Size = new Size(w - 130, 22), AutoEllipsis = true, ForeColor = Theme.Text };
            cardGame.Controls.Add(lbFolder);
            btnFolder = SmallButton(cardGame, w - 108, 30, 94);
            btnFolder.Click += delegate { ChooseFolder(); };
            lbExeCap = new Label { Location = new Point(14, 64), Size = new Size(w - 130, 20), ForeColor = Theme.Muted, Font = Theme.Base(9f) };
            cardGame.Controls.Add(lbExeCap);
            lbArchive = new Label { Location = new Point(14, 84), Size = new Size(w - 130, 22), AutoEllipsis = true, ForeColor = Theme.Text };
            cardGame.Controls.Add(lbArchive);
            btnExe = SmallButton(cardGame, w - 108, 76, 94);
            btnExe.Click += delegate { ChooseExeSource(); };
            lbModState = new Label { Location = new Point(14, 116), Size = new Size(w - 28, 22), Font = Theme.Semi(10.5f) };
            lbUpdate = new Label { Location = new Point(14, 140), Size = new Size(w - 28, 22), ForeColor = Theme.Muted };
            cardGame.Controls.Add(lbModState);
            cardGame.Controls.Add(lbUpdate);

            btnMain = new Button { Location = new Point(x, 270), Size = new Size(w, 46), Tag = "primary", Font = Theme.Semi(12f) };
            btnMain.Click += delegate { OnMainButton(); };
            Controls.Add(btnMain);
            actionButtons.Add(btnMain);

            // --- player card
            cardPlayer = Card(x, 328, w, 196);
            lbPlayerCap = Caption(cardPlayer, 14, 10);
            lbNick = FieldLabel(cardPlayer, 14, 40);
            tbNick = Field(cardPlayer, 150, 37, 220, settings.Nick);
            lbIp = FieldLabel(cardPlayer, 14, 74);
            tbIp = Field(cardPlayer, 150, 71, 170, settings.Ip);
            lbPort = new Label { Location = new Point(330, 74), Size = new Size(50, 24), ForeColor = Theme.Muted };
            cardPlayer.Controls.Add(lbPort);
            tbPort = Field(cardPlayer, 382, 71, 80, settings.Port.ToString());
            lbIpHint = new Label { Location = new Point(150, 100), Size = new Size(320, 20), ForeColor = Theme.Muted, Font = Theme.Base(8.75f) };
            cardPlayer.Controls.Add(lbIpHint);
            lbKeyCap = Caption(cardPlayer, 14, 128);
            lbKey = FieldLabel(cardPlayer, 14, 158);
            tbKey = Field(cardPlayer, 150, 155, 170, Serial.GetSerial());
            tbKey.TextChanged += delegate { Serial.SetSerial(tbKey.Text.Trim()); };
            btnHowKey = SmallButton(cardPlayer, 330, 153, 132);
            btnHowKey.Click += delegate { HowToGetKey(); };

            btnPlay = new Button { Location = new Point(x, 536), Size = new Size(w, 56), Tag = "play", Font = Theme.Semi(15f) };
            btnPlay.Click += delegate { Play(); };
            Controls.Add(btnPlay);
            actionButtons.Add(btnPlay);

            // --- footer
            lbStatus = new Label { Location = new Point(x, 604), Size = new Size(w, 22), ForeColor = Theme.Muted, AutoEllipsis = true };
            Controls.Add(lbStatus);
            btnLogs = SmallButton(this, x, 630, 110);
            btnLogs.Click += delegate { Directory.CreateDirectory(installer.LogsDir); Process.Start("explorer.exe", installer.LogsDir); };
            btnRepair = SmallButton(this, x + 118, 630, 110);
            btnRepair.Click += delegate { if (EnsureWritable()) RunBusy(() => installer.Repair()); };
            btnServer = SmallButton(this, x + 236, 630, 120);
            btnServer.Tag = "primary";
            btnServer.Click += delegate
            {
                SaveFields();
                using (var panel = new ServerPanel(settings, installer, ip => { tbIp.Text = ip; SaveFields(); }))
                    panel.ShowDialog(this);
            };
            btnUninstall = SmallButton(this, x + w - 138, 630, 138);
            btnUninstall.Tag = "danger";
            btnUninstall.Click += delegate
            {
                if (EnsureWritable() && MessageBox.Show(L.T("confirmUninstall"), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                    RunBusy(() => installer.Uninstall());
            };
            actionButtons.AddRange(new[] { btnRepair, btnUninstall });

            Theme.Apply(this);
            ApplyTexts();
            RefreshState();
            Shown += delegate { RunBusy(CheckUpdates); };
            FormClosing += delegate { SaveFields(); };
        }

        Panel Card(int x, int y, int w, int h)
        {
            var c = new Panel { Location = new Point(x, y), Size = new Size(w, h), BackColor = Theme.Card };
            Controls.Add(c);
            return c;
        }
        Label Caption(Control parent, int x, int y)
        {
            var l = new Label { Location = new Point(x, y), AutoSize = true, Font = Theme.Semi(11f), ForeColor = Theme.Text };
            parent.Controls.Add(l);
            return l;
        }
        Label FieldLabel(Control parent, int x, int y)
        {
            var l = new Label { Location = new Point(x, y), Size = new Size(134, 24), ForeColor = Theme.Muted };
            parent.Controls.Add(l);
            return l;
        }
        TextBox Field(Control parent, int x, int y, int w, string value)
        {
            var t = new TextBox { Location = new Point(x, y), Width = w, Text = value, Font = Theme.Base(10.5f), BorderStyle = BorderStyle.FixedSingle };
            parent.Controls.Add(t);
            return t;
        }
        Button SmallButton(Control parent, int x, int y, int w)
        {
            var b = new Button { Location = new Point(x, y), Size = new Size(w, 30), Font = Theme.Base(9.5f) };
            parent.Controls.Add(b);
            return b;
        }

        void ApplyTexts()
        {
            lbTitle.Text = L.T("pTitle");
            lbSub.Text = L.T("pSubtitle");
            btnLang.Text = L.T("lang");
            lbFolderCap.Text = L.T("pFolder");
            btnFolder.Text = L.T("pChange");
            lbExeCap.Text = L.T("pExeNeeded");
            btnExe.Text = L.T("pChooseArchive");
            lbPlayerCap.Text = L.T("pPlayer");
            lbNick.Text = L.T("nick");
            lbIp.Text = L.T("ip");
            lbPort.Text = L.T("port");
            lbIpHint.Text = L.T("pIpHint");
            lbKeyCap.Text = L.T("pKey");
            lbKey.Text = L.T("serialKey");
            btnHowKey.Text = L.T("pHowKey");
            btnPlay.Text = L.T("pPlay");
            btnLogs.Text = L.T("pLogs");
            btnRepair.Text = L.T("pRepair");
            btnServer.Text = L.T("svTitle");
            btnUninstall.Text = L.T("uninstall");
        }

        void SetStatus(string line)
        {
            if (lbStatus == null) return;
            if (lbStatus.InvokeRequired) { lbStatus.BeginInvoke(new Action<string>(SetStatus), line); return; }
            lbStatus.Text = line;
        }

        bool IsGameFolder(string dir) { return Settings.IsGameFolder(dir); }

        // the game needs gta_sa.exe 1.0 US from the archive and none is available yet
        bool NeedsArchive()
        {
            string exe = Path.Combine(settings.GameDir, "gta_sa.exe");
            bool is10us = File.Exists(exe) && new FileInfo(exe).Length == Installer.Exe10UsSize;
            return !is10us && installer.GetCompatibleExe() == null;
        }

        void RefreshState()
        {
            installer = new Installer(settings, SetStatus);
            bool gameFound = IsGameFolder(settings.GameDir);
            lbFolder.Text = gameFound ? settings.GameDir : settings.GameDir + "  —  " + L.T("exeMissing");
            lbFolder.ForeColor = gameFound ? Theme.Text : Theme.Danger;

            bool needArchive = gameFound && NeedsArchive();
            lbArchive.Text = string.IsNullOrEmpty(settings.AdditionalZip) ? L.T("pNoArchive") : settings.AdditionalZip;
            lbArchive.ForeColor = needArchive ? Theme.Warn : Theme.Text;
            lbExeCap.ForeColor = needArchive ? Theme.Warn : Theme.Muted;

            bool installed = installer.IsInstalled;
            lbModState.Text = installed ? L.F("pInstalledV", installer.InstalledVersion) : L.T("pNotInstalled");
            lbModState.ForeColor = installed ? Theme.Ok : Theme.Warn;

            if (remoteVersion == null) lbUpdate.Text = busy ? L.T("pChecking") : "";
            else if (remoteVersion.Length == 0) lbUpdate.Text = L.T("pOffline");
            else lbUpdate.Text = installed && installer.InstalledVersion == remoteVersion ? L.T("pLatest") : L.F("pNewAvail", remoteVersion);

            if (!installed) btnMain.Text = L.T("pInstall");
            else if (!string.IsNullOrEmpty(remoteVersion) && remoteVersion != installer.InstalledVersion) btnMain.Text = L.F("pUpdateTo", remoteVersion);
            else btnMain.Text = L.T("pCheck");

            // buttons stay clickable: a click explains what is missing instead of doing nothing
            foreach (var b in actionButtons) b.Enabled = !busy;
            btnPlay.Enabled = !busy && installed;
            btnRepair.Enabled = btnUninstall.Enabled = !busy && installed;
            UseWaitCursor = busy;
        }

        void RunBusy(Action work)
        {
            if (busy) return;
            busy = true;
            SetStatus(L.T("pWorking"));
            RefreshState();
            var t = new Thread(() =>
            {
                try { work(); if (lbStatus.Text == L.T("pWorking")) SetStatus(L.T("pReady")); }
                catch (Exception ex) { SetStatus(L.F("failed", ex.Message)); }
                finally
                {
                    busy = false;
                    BeginInvoke(new Action(RefreshState));
                }
            });
            t.IsBackground = true;
            t.Start();
        }

        void CheckUpdates()
        {
            try
            {
                string sha;
                remoteVersion = new Release(settings, s => { }).FetchRemote(out sha).Version;
            }
            catch { remoteVersion = ""; }
        }

        void OnMainButton()
        {
            if (installer.IsInstalled && (string.IsNullOrEmpty(remoteVersion) || remoteVersion == installer.InstalledVersion))
            {
                RunBusy(CheckUpdates);
                return;
            }
            if (!IsGameFolder(settings.GameDir))
            {
                MessageBox.Show(L.T("pChooseGame"), Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
                ChooseFolder();
                if (!IsGameFolder(settings.GameDir)) return;
            }
            if (NeedsArchive())
            {
                MessageBox.Show(L.T("pArchiveRequired"), Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
                ChooseExeSource();
                if (NeedsArchive()) return;
            }
            if (!EnsureWritable()) return;
            RunBusy(() =>
            {
                bool restart = new Release(settings, SetStatus).Download(installer.PlayerPackageDir);
                installer.Install(true);
                CheckUpdates();
                if (restart)
                {
                    BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show(L.T("selfUpdated"), Text);
                        Process.Start(Application.ExecutablePath);
                        Close();
                    }));
                }
            });
        }

        bool EnsureWritable()
        {
            if (installer.IsGameDirWritable()) return true;
            if (MessageBox.Show(L.T("notWritable"), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes) return false;
            return installer.GrantGameDirAccess();
        }

        void ChooseFolder()
        {
            using (var d = new FolderBrowserDialog { SelectedPath = settings.GameDir })
            {
                if (d.ShowDialog() != DialogResult.OK) return;
                settings.GameDir = d.SelectedPath;
                settings.Save();
                RefreshState();
            }
        }

        void ChooseExeSource()
        {
            using (var d = new OpenFileDialog { Filter = "additional.zip / gta_sa.exe|*.zip;gta_sa.exe|*.*|*.*" })
            {
                if (d.ShowDialog() != DialogResult.OK) return;
                settings.AdditionalZip = d.FileName.EndsWith(".exe", StringComparison.OrdinalIgnoreCase) ? Path.GetDirectoryName(d.FileName) : d.FileName;
                settings.Save();
                RefreshState();
                if (new Installer(settings, s => { }).GetCompatibleExeFromSettings() == null)
                    MessageBox.Show(L.T("pArchiveBad"), Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        void HowToGetKey()
        {
            string cmd = "/gen " + Serial.GetPcId();
            try { Clipboard.SetText(cmd); } catch { }
            MessageBox.Show(L.F("pHowKeyText", cmd), L.T("pKey"), MessageBoxButtons.OK, MessageBoxIcon.Information);
            Process.Start("https://discord.gg/Z3ugSgFJMU");
        }

        void SaveFields()
        {
            settings.Nick = tbNick.Text.Trim();
            settings.Ip = tbIp.Text.Trim();
            int port;
            if (int.TryParse(tbPort.Text.Trim(), out port)) settings.Port = port;
            settings.Save();
        }

        void Play()
        {
            SaveFields();
            if (string.IsNullOrEmpty(Serial.GetSerial())) { MessageBox.Show(L.T("needSerial"), Text); tbKey.Focus(); return; }
            if (settings.Nick.Length == 0) { tbNick.Focus(); return; }
            try { SetStatus(GameLauncher.Launch(settings, 0, settings.Nick, settings.Ip, -1, false)); }
            catch (Exception ex) { MessageBox.Show(L.F("failed", ex.Message), Text); }
        }
    }

    static class Program
    {
#if PLAYER
        public const bool IsPlayerEdition = true;
#else
        public const bool IsPlayerEdition = false;
#endif
        public const string LauncherFileName = "SADreamLauncher.exe";

        [DllImport("kernel32.dll")]
        static extern bool AttachConsole(int pid);

        // command line mode: --install --repair --uninstall --verify --status --update --publish --assemble <dir> --test --stop
        static int RunCli(string[] args)
        {
            AttachConsole(-1);
            var settings = Settings.Load();
            L.Uk = false;
            Directory.CreateDirectory(Settings.AppDataDir);
            string logPath = Path.Combine(Settings.AppDataDir, "manager_cli.log");
            using (var w = new StreamWriter(logPath, false, new UTF8Encoding(false)) { AutoFlush = true })
            {
                Action<string> log = line => { Console.WriteLine(line); w.WriteLine(line); };
                Func<string, string, string, int> tool = (exe, a, dir) =>
                {
                    log("> " + exe + " " + a);
                    var psi = new ProcessStartInfo(exe, a) { WorkingDirectory = dir, UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
                    using (var p = Process.Start(psi))
                    {
                        var err = p.StandardError.ReadToEndAsync();
                        log(p.StandardOutput.ReadToEnd().TrimEnd());
                        p.WaitForExit();
                        log(err.Result.TrimEnd());
                        return p.ExitCode;
                    }
                };
                var inst = new Installer(settings, log);
                try
                {
                    switch (args[0])
                    {
                        case "--install": inst.Install(false); break;
                        case "--repair": inst.Repair(); break;
                        case "--uninstall": inst.Uninstall(); break;
                        case "--verify":
                            var problems = inst.Verify();
                            foreach (var p in problems) log("! " + p);
                            log(problems.Count == 0 ? "OK" : problems.Count + " problem(s)");
                            return problems.Count == 0 ? 0 : 2;
                        case "--status":
                            var git = new Git(settings);
                            log("mode: " + (settings.PlayerMode ? "player" : "developer") + "  source: " + settings.SourceDir + "  origin: " + git.OriginUrl());
                            log("release repo: " + settings.ReleaseRepo + "  local clone: " + settings.ReleaseDir);
                            log("installed: " + inst.IsInstalled + " version: " + inst.InstalledVersion);
                            break;
                        case "--update":
                            if (settings.PlayerMode) { new Release(settings, log).Download(inst.PlayerPackageDir); inst.Install(true); }
                            else inst.Install(true);
                            break;
                        case "--test":
                            // --test [missionId]: the host launches that mission by itself (-testmission)
                            string extra = args.Length > 1 ? "-testmission " + args[1] : "";
                            GameLauncher.StartDetachedServer(inst);
                            Thread.Sleep(1000);
                            log(GameLauncher.Launch(settings, 1, settings.Nick1, "127.0.0.1", 0, true, extra));
                            Thread.Sleep(1500);
                            log(GameLauncher.Launch(settings, 2, settings.Nick2, "127.0.0.1", 1, true, extra));
                            break;
                        case "--stop":
                            GameLauncher.StopGames();
                            GameLauncher.StopServers(inst);
                            log("stopped");
                            break;
                        case "--assemble":
                            inst.AssembleFromBuild(args.Length > 1 ? args[1] : inst.DevPackageDir, true);
                            break;
                        case "--gh-release":
                            // Releases page for the version that is already in the release repo
                            var rm = Manifest.Load(settings.ReleaseDir);
                            if (rm == null) { log("nothing published yet"); return 1; }
                            string commit;
                            rm.Meta.TryGetValue("commit", out commit);
                            new DevOps(settings, log, tool).CreateGitHubRelease(rm.Version, new Git(settings).Get("rev-parse " + commit));
                            break;
                        case "--publish":
                            new DevOps(settings, log, tool).Publish(url => { log("publishing to " + url); return true; });
                            break;
                        default: log("unknown command " + args[0]); return 1;
                    }
                }
                catch (Exception ex) { log("ERROR: " + ex.Message); return 1; }
            }
            return 0;
        }

        [STAThread]
        static int Main(string[] args)
        {
            // leftover from a self-update
            try { File.Delete(Application.ExecutablePath + ".old"); } catch { }
            if (args.Length > 0 && args[0] == "--server")
            {
                // standalone server panel (desktop shortcut "SA Dream Mod — Server")
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                var ss = Settings.Load();
                L.Uk = ss.Ukrainian;
                Application.Run(new ServerPanel(ss, new Installer(ss, line => { }), null));
                return 0;
            }
            if (args.Length > 0) return RunCli(args);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
#if PLAYER
            Application.Run(new PlayerForm());
#else
            Application.Run(new DevForm());
#endif
            return 0;
        }
    }
}
