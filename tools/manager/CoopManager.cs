// CoopAndreas Manager - install / uninstall / repair / update / launch / logs
// Built with the .NET Framework 4.x csc.exe (C# 5), no external dependencies. See build.cmd.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

[assembly: System.Reflection.AssemblyTitle("CoopAndreas Manager")]
[assembly: System.Reflection.AssemblyVersion("1.0.0.0")]

namespace CoopManager
{
    // ------------------------------------------------------------------ localization
    static class L
    {
        public static bool Uk = true;
        static readonly Dictionary<string, string[]> S = new Dictionary<string, string[]>
        {
            {"title", new[]{"CoopAndreas — менеджер", "CoopAndreas Manager"}},
            {"tabInstall", new[]{"Встановлення", "Install"}},
            {"tabLaunch", new[]{"Запуск", "Launch"}},
            {"tabLogs", new[]{"Логи", "Logs"}},
            {"tabSettings", new[]{"Шляхи", "Paths"}},
            {"lang", new[]{"English", "Українська"}},
            {"gameDir", new[]{"Тека гри:", "Game folder:"}},
            {"srcDir", new[]{"Код моду (git):", "Mod source (git):"}},
            {"addZip", new[]{"Архів additional:", "Additional archive:"}},
            {"browse", new[]{"Огляд…", "Browse…"}},
            {"save", new[]{"Зберегти", "Save"}},
            {"status", new[]{"Стан", "Status"}},
            {"refresh", new[]{"Оновити стан", "Refresh status"}},
            {"install", new[]{"Встановити / оновити файли", "Install / update files"}},
            {"uninstall", new[]{"Видалити мод", "Uninstall mod"}},
            {"repair", new[]{"Перевірити і виправити", "Verify and repair"}},
            {"build", new[]{"Зібрати з коду", "Build from source"}},
            {"update", new[]{"Оновити з git + зібрати + встановити", "Git pull + build + install"}},
            {"exeOk", new[]{"gta_sa.exe: версія 1.0 US (сумісна)", "gta_sa.exe: version 1.0 US (compatible)"}},
            {"exeBad", new[]{"gta_sa.exe: НЕСУМІСНА версія (буде замінена під час встановлення)", "gta_sa.exe: INCOMPATIBLE version (will be replaced on install)"}},
            {"exeMissing", new[]{"gta_sa.exe не знайдено", "gta_sa.exe not found"}},
            {"modInstalled", new[]{"Мод встановлено: {0}", "Mod installed: {0}"}},
            {"modNotInstalled", new[]{"Мод не встановлено", "Mod is not installed"}},
            {"buildFound", new[]{"Зібрані файли: {0}", "Built files: {0}"}},
            {"buildMissing", new[]{"Зібраних файлів немає — натисніть «Зібрати з коду»", "No build output — press \"Build from source\""}},
            {"backupFound", new[]{"Резервна копія оригіналів: є", "Backup of original files: present"}},
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
            {"testHint", new[]{"Вікна ставляться поруч. У грі виберіть «Start Game» → підключення до 127.0.0.1 (нік і IP вже прописані).", "Windows are placed side by side. In game choose \"Start Game\" → connect to 127.0.0.1 (nick and IP are pre-filled)."}},
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
            {"fixPerms", new[]{"Надати права на теку гри", "Grant access to game folder"}},
            {"notWritable", new[]{"Немає прав на запис у теку гри (вона захищена як Program Files). Натисніть «Так», щоб надати вашому користувачу права на зміну цієї теки (з'явиться запит адміністратора Windows).", "The game folder is not writable (protected like Program Files). Press \"Yes\" to grant your user modify rights on this folder (a Windows administrator prompt will appear)."}},
            {"writableNo", new[]{"Тека гри: немає прав на запис — натисніть «Надати права на теку гри»", "Game folder: not writable — press \"Grant access to game folder\""}},
            {"repo", new[]{"Наш приватний репозиторій", "Our private repository"}},
            {"repoUrl", new[]{"URL репозиторію:", "Repository URL:"}},
            {"branch", new[]{"Гілка:", "Branch:"}},
            {"clone", new[]{"Клонувати", "Clone"}},
            {"checkUpdates", new[]{"Перевірити оновлення", "Check for updates"}},
            {"updateRepo", new[]{"Оновити (git pull) і встановити", "Update (git pull) and install"}},
            {"publish", new[]{"Опублікувати збірку (для розробника)", "Publish build (developer)"}},
            {"srcDist", new[]{"Ставити готові файли з репозиторію (dist)", "Install prebuilt files from the repo (dist)"}},
            {"srcBuild", new[]{"Ставити мою локальну збірку (розробник)", "Install my local build (developer)"}},
            {"repoInfo", new[]{"Remote: {0}   гілка: {1}   коміт: {2}", "Remote: {0}   branch: {1}   commit: {2}"}},
            {"repoMissing", new[]{"Репозиторій не знайдено — вкажіть URL на вкладці «Шляхи» і натисніть «Клонувати»", "Repository not found — set the URL on the Paths tab and press \"Clone\""}},
            {"updatesAvail", new[]{"Доступно оновлень: {0} коміт(ів)", "Updates available: {0} commit(s)"}},
            {"upToDate", new[]{"Встановлена остання версія", "Up to date"}},
            {"unsafeOrigin", new[]{"Remote 'origin' не налаштований або вказує на оригінальний репозиторій Tornamic. Оновлення дозволені тільки з нашого приватного репозиторію.", "Remote 'origin' is not set or points to the original Tornamic repository. Updates are only allowed from our private repository."}},
            {"dirtyTree", new[]{"У репозиторії є незбережені зміни (git status). Закомітьте або скасуйте їх перед оновленням.", "The repository has uncommitted changes (git status). Commit or discard them before updating."}},
            {"confirmPublish", new[]{"Скопіювати поточну збірку в dist/, зробити коміт і запушити в наш приватний репозиторій ({0})?", "Copy the current build to dist/, commit and push to our private repository ({0})?"}},
            {"noAdditional", new[]{"Не знайдено архів/теку additional.", "The additional archive/folder was not found."}},
        };
        public static string T(string key)
        {
            string[] v;
            if (!S.TryGetValue(key, out v)) return key;
            return Uk ? v[0] : v[1];
        }
        public static string F(string key, params object[] args) { return string.Format(T(key), args); }
    }

    // ------------------------------------------------------------------ settings (ini next to the exe)
    class Settings
    {
        public string GameDir = @"D:\Grand Theft Auto San Andreas";
        public string SourceDir = @"D:\CoopAndreasDev\src";
        public string AdditionalZip = @"D:\additional.zip";
        public bool Ukrainian = true;
        public string Nick = "Player";
        public string Ip = "127.0.0.1";
        public int Port = 6767;
        public string Nick1 = "Tester1";
        public string Nick2 = "Tester2";
        public string RepoUrl = "";
        public string Branch = "main";
        public bool InstallFromDist = true;

        public static string AppDataDir { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "CoopAndreasManager"); } }
        static string FilePath { get { return Path.Combine(AppDataDir, "CoopManager.ini"); } }

        public static Settings Load()
        {
            var s = new Settings();
            if (!File.Exists(FilePath)) return s;
            foreach (var line in File.ReadAllLines(FilePath, Encoding.UTF8))
            {
                int eq = line.IndexOf('=');
                if (eq <= 0) continue;
                string k = line.Substring(0, eq).Trim(), v = line.Substring(eq + 1).Trim();
                switch (k)
                {
                    case "GameDir": s.GameDir = v; break;
                    case "SourceDir": s.SourceDir = v; break;
                    case "AdditionalZip": s.AdditionalZip = v; break;
                    case "Ukrainian": s.Ukrainian = v == "1"; break;
                    case "Nick": s.Nick = v; break;
                    case "Ip": s.Ip = v; break;
                    case "Port": int.TryParse(v, out s.Port); break;
                    case "Nick1": s.Nick1 = v; break;
                    case "Nick2": s.Nick2 = v; break;
                    case "RepoUrl": s.RepoUrl = v; break;
                    case "Branch": s.Branch = v; break;
                    case "InstallFromDist": s.InstallFromDist = v != "0"; break;
                }
            }
            return s;
        }

        public void Save()
        {
            var sb = new StringBuilder();
            sb.AppendLine("GameDir=" + GameDir);
            sb.AppendLine("SourceDir=" + SourceDir);
            sb.AppendLine("AdditionalZip=" + AdditionalZip);
            sb.AppendLine("Ukrainian=" + (Ukrainian ? "1" : "0"));
            sb.AppendLine("Nick=" + Nick);
            sb.AppendLine("Ip=" + Ip);
            sb.AppendLine("Port=" + Port);
            sb.AppendLine("Nick1=" + Nick1);
            sb.AppendLine("Nick2=" + Nick2);
            sb.AppendLine("RepoUrl=" + RepoUrl);
            sb.AppendLine("Branch=" + Branch);
            sb.AppendLine("InstallFromDist=" + (InstallFromDist ? "1" : "0"));
            Directory.CreateDirectory(AppDataDir);
            File.WriteAllText(FilePath, sb.ToString(), Encoding.UTF8);
        }
    }

    // ------------------------------------------------------------------ install logic
    class Installer
    {
        public const string BackupDirName = "_coop_backup";
        public const string ManifestName = "install_manifest.txt";
        public const long Exe10UsSize = 14383616;

        readonly Settings s;
        readonly Action<string> log;
        public Installer(Settings settings, Action<string> logger) { s = settings; log = logger; }

        public string BuildDir { get { return Path.Combine(s.SourceDir, @"build\windows\x86\release"); } }
        public string DistDir { get { return Path.Combine(s.SourceDir, "dist"); } }
        // where the mod binaries are taken from: prebuilt dist/bin (players) or the local xmake output (developer)
        public string BinDir { get { return s.InstallFromDist ? Path.Combine(DistDir, "bin") : BuildDir; } }
        public string AdditionalDir { get { return Path.Combine(DistDir, "additional"); } }
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

        // gta_sa.exe 1.0 US is not stored in git (Rockstar binary): it comes from additional.zip (or a folder)
        public string GetCompatibleExe()
        {
            string p = s.AdditionalZip;
            if (Directory.Exists(p))
            {
                foreach (var f in Directory.GetFiles(p, "gta_sa.exe", SearchOption.AllDirectories)) return f;
                return null;
            }
            if (!File.Exists(p)) return null;
            string tmp = Path.Combine(Path.GetTempPath(), "CoopManager_exe");
            Directory.CreateDirectory(tmp);
            using (var zip = ZipFile.OpenRead(p))
            {
                var e = zip.Entries.FirstOrDefault(x => x.Name.Equals("gta_sa.exe", StringComparison.OrdinalIgnoreCase));
                if (e == null) return null;
                string dst = Path.Combine(tmp, "gta_sa.exe");
                e.ExtractToFile(dst, true);
                return dst;
            }
        }

        // map: destination path relative to game dir -> source absolute path
        public Dictionary<string, string> BuildFileMap()
        {
            var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (var f in Directory.GetFiles(AdditionalDir))
            {
                string name = Path.GetFileName(f);
                // user-tweakable ini files are only copied when missing, handled separately
                if (name.EndsWith(".ini", StringComparison.OrdinalIgnoreCase)) continue;
                map[name] = f;
            }
            map["eax.dll"] = Path.Combine(BinDir, "proxy.dll");
            map["CoopAndreasSA.dll"] = Path.Combine(BinDir, "CoopAndreasSA.dll");
            map["LaunchCoopAndreas.exe"] = Path.Combine(BinDir, "LaunchCoopAndreas.exe");
            map["LaunchCoopAndreas.exe.manifest"] = Path.Combine(BinDir, "LaunchCoopAndreas.exe.manifest");
            map[@"CoopAndreas\main.scm"] = Path.Combine(s.SourceDir, @"scm\main.scm");
            map[@"CoopAndreas\script.img"] = Path.Combine(s.SourceDir, @"scm\script.img");
            map[@"CoopAndreasServer\server.exe"] = Path.Combine(BinDir, "server.exe");
            return map;
        }

        public bool BuildExists()
        {
            return File.Exists(Path.Combine(BinDir, "CoopAndreasSA.dll")) && File.Exists(Path.Combine(BinDir, "proxy.dll"))
                && Directory.Exists(AdditionalDir);
        }

        void EnsureGameNotRunning()
        {
            if (Process.GetProcessesByName("gta_sa").Length > 0) throw new Exception(L.T("gameRunning"));
        }

        public void Install(bool onlyChanged)
        {
            EnsureGameNotRunning();
            if (!BuildExists()) throw new Exception(L.T("buildMissing"));
            if (!Directory.Exists(s.GameDir)) throw new Exception("Game folder not found: " + s.GameDir);

            var map = BuildFileMap();
            string gameExe = Path.Combine(s.GameDir, "gta_sa.exe");
            if (!File.Exists(gameExe) || new FileInfo(gameExe).Length != Exe10UsSize)
            {
                string exe = GetCompatibleExe();
                if (exe == null) throw new Exception(L.T("noAdditional"));
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

            // 3. copy files
            var manifest = new List<string>();
            foreach (var kv in map)
            {
                string dst = Path.Combine(s.GameDir, kv.Key);
                if (!File.Exists(kv.Value)) { log("! missing source: " + kv.Value); continue; }
                string srcHash = Sha256(kv.Value);
                bool same = File.Exists(dst) && Sha256(dst) == srcHash;
                if (!same || !onlyChanged)
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(dst));
                    if (!same)
                    {
                        File.Copy(kv.Value, dst, true);
                        log("copy: " + kv.Key);
                    }
                }
                // gta_sa.exe is restored from the backup on uninstall, so it is not tracked as a mod file
                if (!kv.Key.Equals("gta_sa.exe", StringComparison.OrdinalIgnoreCase)) manifest.Add(kv.Key + "|" + srcHash);
            }

            // 4. default ini files, never overwritten
            foreach (var f in Directory.GetFiles(AdditionalDir, "*.ini"))
            {
                string dst = Path.Combine(s.GameDir, Path.GetFileName(f));
                if (!File.Exists(dst)) { File.Copy(f, dst); log("copy (default config): " + Path.GetFileName(f)); }
                manifest.Add(Path.GetFileName(f) + "|config");
            }
            manifest.Add("eax_orig.dll|generated");

            Directory.CreateDirectory(LogsDir);
            File.WriteAllLines(ManifestPath, new[] { "# installed " + DateTime.Now.ToString("yyyy-MM-dd HH:mm") }.Concat(manifest));
            log(L.T("done"));
        }

        // returns list of problems; with fix=true reinstalls the broken files
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

    // ------------------------------------------------------------------ git (only our private repo, never upstream)
    class Git
    {
        readonly Settings s;
        public Git(Settings settings) { s = settings; }

        public bool RepoExists { get { return Directory.Exists(Path.Combine(s.SourceDir, ".git")); } }

        public int Run(string args, out string output, string workDir = null)
        {
            var psi = new ProcessStartInfo("git", args)
            {
                WorkingDirectory = workDir ?? s.SourceDir, UseShellExecute = false, CreateNoWindow = true,
                RedirectStandardOutput = true, RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8,
            };
            psi.EnvironmentVariables["GIT_TERMINAL_PROMPT"] = "0";
            using (var p = Process.Start(psi))
            {
                var err = p.StandardError.ReadToEndAsync();
                string o = p.StandardOutput.ReadToEnd();
                p.WaitForExit();
                output = (o + err.Result).Trim();
                return p.ExitCode;
            }
        }

        public string Get(string args)
        {
            string o;
            return Run(args, out o) == 0 ? o : "";
        }

        public string OriginUrl { get { return Get("remote get-url origin"); } }

        // updates/publishing must only ever talk to our private repo
        public bool OriginIsSafe
        {
            get
            {
                string url = OriginUrl;
                return url.Length > 0 && url.IndexOf("Tornamic/CoopAndreas", StringComparison.OrdinalIgnoreCase) < 0;
            }
        }

        public bool IsDirty(params string[] ignorePrefixes)
        {
            foreach (var line in Get("status --porcelain").Split('\n'))
            {
                string l = line.Trim();
                if (l.Length < 3) continue;
                string path = l.Substring(2).Trim().Replace('\\', '/');
                if (!ignorePrefixes.Any(x => path.StartsWith(x))) return true;
            }
            return false;
        }
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

    // ------------------------------------------------------------------ main window
    class MainForm : Form
    {
        readonly Settings settings = Settings.Load();
        Installer installer;
        readonly Dictionary<Control, string> texts = new Dictionary<Control, string>();
        volatile bool busy;
        Process serverProcess;
        StreamWriter serverLog;
        readonly object serverLogLock = new object();

        TabControl tabs;
        TextBox tbGame, tbSrc, tbZip, tbNick, tbIp, tbPort, tbNick1, tbNick2, tbSerial, tbFilter, tbOutput, tbLog;
        Label lbExe, lbMod, lbBuild, lbBackup, lbPcId, lbServer, lbRepo, lbUpdates;
        TextBox tbRepoUrl, tbBranch;
        RadioButton rbDist, rbBuild;
        ComboBox cbLogFile;
        CheckBox chkWarn, chkFollow;
        Button btnLang;
        System.Windows.Forms.Timer logTimer;
        long logPos;
        string logCurrentFile;
        readonly List<string> logLines = new List<string>();

        public MainForm()
        {
            L.Uk = settings.Ukrainian;
            installer = new Installer(settings, Log);
            Font = new Font("Segoe UI", 9.5f);
            ClientSize = new Size(900, 720);
            MinimumSize = new Size(760, 560);
            StartPosition = FormStartPosition.CenterScreen;
            try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

            btnLang = new Button { Anchor = AnchorStyles.Top | AnchorStyles.Right, Size = new Size(110, 26), Location = new Point(ClientSize.Width - 118, 4) };
            btnLang.Click += delegate { L.Uk = !L.Uk; settings.Ukrainian = L.Uk; settings.Save(); ApplyTexts(); RefreshStatus(); };
            Reg(btnLang, "lang");
            Controls.Add(btnLang);

            tabs = new TabControl { Location = new Point(6, 34), Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right };
            tabs.Size = new Size(ClientSize.Width - 12, ClientSize.Height - 40);
            Controls.Add(tabs);

            BuildInstallTab();
            BuildLaunchTab();
            BuildLogsTab();
            BuildPathsTab();

            ApplyTexts();
            RefreshStatus();

            logTimer = new System.Windows.Forms.Timer { Interval = 500 };
            logTimer.Tick += delegate { PollLog(); };
            logTimer.Start();
            FormClosing += delegate { StopServer(); };
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
        Button Btn(Control parent, string key, int x, int y, int w, EventHandler click)
        {
            var b = new Button { Location = new Point(x, y), Size = new Size(w, 30) };
            Reg(b, key);
            b.Click += click;
            parent.Controls.Add(b);
            return b;
        }
        GroupBox Group(Control parent, string key, int x, int y, int w, int h)
        {
            var g = new GroupBox { Location = new Point(x, y), Size = new Size(w, h) };
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
                WorkingDirectory = workDir,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8,
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

        static string StripAnsi(string s)
        {
            return System.Text.RegularExpressions.Regex.Replace(s, @"\x1B\[[0-9;]*[A-Za-z]", "");
        }

        string FindXmake()
        {
            string p = @"C:\Program Files\xmake\xmake.exe";
            return File.Exists(p) ? p : "xmake";
        }

        // ---- Install tab
        void BuildInstallTab()
        {
            var p = Page("tabInstall");
            var g = Group(p, "status", 8, 6, 860, 130);
            g.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            lbExe = Lbl(g, null, 12, 22, 830);
            lbMod = Lbl(g, null, 12, 46, 830);
            lbBuild = Lbl(g, null, 12, 70, 830);
            lbBackup = Lbl(g, null, 12, 94, 830);

            var gr = Group(p, "repo", 8, 142, 860, 150);
            gr.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            lbRepo = Lbl(gr, null, 12, 20, 830);
            lbUpdates = Lbl(gr, null, 12, 42, 830);
            rbDist = new RadioButton { Location = new Point(12, 68), Size = new Size(410, 24), Checked = settings.InstallFromDist };
            rbBuild = new RadioButton { Location = new Point(430, 68), Size = new Size(410, 24), Checked = !settings.InstallFromDist };
            Reg(rbDist, "srcDist");
            Reg(rbBuild, "srcBuild");
            EventHandler srcChanged = delegate { settings.InstallFromDist = rbDist.Checked; settings.Save(); RefreshStatus(); };
            rbDist.CheckedChanged += srcChanged;
            gr.Controls.Add(rbDist);
            gr.Controls.Add(rbBuild);
            Btn(gr, "checkUpdates", 12, 104, 200, delegate { RunBusy(() => CheckUpdates(true)); });
            Btn(gr, "updateRepo", 220, 104, 300, delegate
            {
                if (EnsureWritable()) RunBusy(() => UpdateFromRepo());
            });
            Btn(gr, "publish", 528, 104, 316, delegate { Publish(); });

            int y = 302;
            Btn(p, "install", 8, y, 280, delegate { if (EnsureWritable()) RunBusy(() => installer.Install(false)); });
            Btn(p, "repair", 296, y, 280, delegate { if (EnsureWritable()) RunBusy(() => installer.Repair()); });
            Btn(p, "uninstall", 584, y, 280, delegate
            {
                if (EnsureWritable() && MessageBox.Show(L.T("confirmUninstall"), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                    RunBusy(() => installer.Uninstall());
            });
            y += 38;
            Btn(p, "build", 8, y, 280, delegate { RunBusy(() => Build()); });
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

        // returns number of commits we are behind origin/<branch>, -1 on error
        int CheckUpdates(bool verbose)
        {
            var git = new Git(settings);
            if (!git.RepoExists) { Log(L.T("repoMissing")); return -1; }
            if (!git.OriginIsSafe) { Log(L.T("unsafeOrigin")); return -1; }
            string o;
            if (git.Run("fetch origin " + settings.Branch, out o) != 0) { Log(o); return -1; }
            int behind;
            int.TryParse(git.Get("rev-list --count HEAD..origin/" + settings.Branch), out behind);
            string msg = behind > 0 ? L.F("updatesAvail", behind) : L.T("upToDate");
            if (verbose) Log(msg);
            BeginInvoke(new Action(() => { lbUpdates.Text = msg; }));
            return behind;
        }

        void UpdateFromRepo()
        {
            var git = new Git(settings);
            if (!git.RepoExists) { Log(L.T("repoMissing")); return; }
            if (!git.OriginIsSafe) { Log(L.T("unsafeOrigin")); return; }
            // local build output in dist/ is regenerated by Publish, so only source changes block the update
            if (git.IsDirty()) { Log(L.T("dirtyTree")); return; }
            if (RunTool("git", "pull --ff-only origin " + settings.Branch, settings.SourceDir) != 0) return;
            if (!settings.InstallFromDist && !Build()) return;
            installer.Install(true);
            BuildManagerIntoDist(false);
        }

        // rebuilds the manager from tools/manager into dist/ (only when the developer publishes)
        bool BuildManagerIntoDist(bool force)
        {
            if (!force) return true;
            string cmd = Path.Combine(settings.SourceDir, @"tools\manager\build.cmd");
            return File.Exists(cmd) && RunTool("cmd.exe", "/c \"" + cmd + "\"", Path.GetDirectoryName(cmd)) == 0;
        }

        void Publish()
        {
            var git = new Git(settings);
            if (!git.RepoExists) { MessageBox.Show(L.T("repoMissing")); return; }
            if (!git.OriginIsSafe) { MessageBox.Show(L.T("unsafeOrigin")); return; }
            if (!File.Exists(Path.Combine(installer.BuildDir, "CoopAndreasSA.dll"))) { MessageBox.Show(L.T("buildMissing")); return; }
            if (MessageBox.Show(L.F("confirmPublish", git.OriginUrl), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
            RunBusy(() =>
            {
                string bin = Path.Combine(installer.DistDir, "bin");
                Directory.CreateDirectory(bin);
                foreach (var name in new[] { "CoopAndreasSA.dll", "proxy.dll", "LaunchCoopAndreas.exe", "LaunchCoopAndreas.exe.manifest", "server.exe" })
                {
                    File.Copy(Path.Combine(installer.BuildDir, name), Path.Combine(bin, name), true);
                    Log("dist/bin/" + name);
                }
                if (!BuildManagerIntoDist(true)) return;
                string head = git.Get("rev-parse --short HEAD");
                File.WriteAllText(Path.Combine(installer.DistDir, "BUILD_INFO.txt"),
                    "source commit: " + head + Environment.NewLine + "built: " + DateTime.Now.ToString("yyyy-MM-dd HH:mm") + Environment.NewLine);
                if (RunTool("git", "add dist", settings.SourceDir) != 0) return;
                if (RunTool("git", "commit -m \"dist: build of " + head + "\"", settings.SourceDir) != 0) return;
                RunTool("git", "push origin HEAD:" + settings.Branch, settings.SourceDir);
            });
        }

        bool EnsureWritable()
        {
            if (installer.IsGameDirWritable()) return true;
            if (MessageBox.Show(L.T("notWritable"), Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes) return false;
            bool ok = installer.GrantGameDirAccess();
            RefreshStatus();
            return ok;
        }

        bool Build()
        {
            string xm = FindXmake();
            if (!File.Exists(Path.Combine(settings.SourceDir, @".xmake\windows\x86\xmake.conf")))
                RunTool(xm, "f -p windows -a x86 -m release --toolchain=msvc -y", settings.SourceDir);
            return RunTool(xm, "-y", settings.SourceDir) == 0;
        }

        void RefreshStatus()
        {
            if (lbExe == null) return;
            installer = new Installer(settings, Log);
            lbExe.Text = installer.ExeState();
            lbExe.ForeColor = lbExe.Text == L.T("exeOk") ? Color.DarkGreen : Color.DarkRed;
            if (installer.IsInstalled)
                lbMod.Text = L.F("modInstalled", File.ReadAllLines(installer.ManifestPath).FirstOrDefault() ?? "");
            else
                lbMod.Text = L.T("modNotInstalled");
            string dll = Path.Combine(installer.BinDir, "CoopAndreasSA.dll");
            lbBuild.Text = File.Exists(dll) ? L.F("buildFound", File.GetLastWriteTime(dll).ToString("yyyy-MM-dd HH:mm") + "  (" + installer.BinDir + ")") : L.T("buildMissing");
            var git = new Git(settings);
            if (git.RepoExists)
            {
                lbRepo.Text = L.F("repoInfo", git.OriginUrl.Length > 0 ? git.OriginUrl : "—", git.Get("rev-parse --abbrev-ref HEAD"), git.Get("log -1 --format=%h %s"));
                lbRepo.ForeColor = git.OriginIsSafe ? SystemColors.ControlText : Color.DarkRed;
            }
            else
            {
                lbRepo.Text = L.T("repoMissing");
                lbRepo.ForeColor = Color.DarkRed;
            }
            lbBackup.Text = Directory.Exists(installer.BackupDir) ? L.T("backupFound") : "";
            if (!installer.IsGameDirWritable()) { lbBackup.Text = L.T("writableNo"); lbBackup.ForeColor = Color.DarkRed; }
            else lbBackup.ForeColor = SystemColors.ControlText;
            UpdateServerLabel();
        }

        // ---- Launch tab
        void BuildLaunchTab()
        {
            var p = Page("tabLaunch");
            var gs = Group(p, "serial", 8, 6, 860, 130);
            gs.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
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
            Lbl(gp, "nick", 12, 24, 110); tbNick = new TextBox { Location = new Point(125, 24), Width = 200, Text = settings.Nick }; gp.Controls.Add(tbNick);
            Lbl(gp, "ip", 12, 54, 110); tbIp = new TextBox { Location = new Point(125, 54), Width = 140, Text = settings.Ip }; gp.Controls.Add(tbIp);
            Lbl(gp, "port", 272, 54, 50); tbPort = new TextBox { Location = new Point(325, 54), Width = 70, Text = settings.Port.ToString() }; gp.Controls.Add(tbPort);
            Btn(gp, "launchGame", 12, 100, 200, delegate { SaveLaunchFields(); LaunchGame(0, settings.Nick, settings.Ip, -1); });

            var gsv = Group(p, "server", 440, 142, 428, 150);
            gsv.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            lbServer = Lbl(gsv, null, 12, 26, 400);
            Btn(gsv, "startServer", 12, 60, 190, delegate { StartServer(); });
            Btn(gsv, "stopServer", 212, 60, 190, delegate { StopServer(); });

            var gt = Group(p, "test", 8, 300, 860, 170);
            gt.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            Lbl(gt, "nick1", 12, 26, 120); tbNick1 = new TextBox { Location = new Point(135, 26), Width = 160, Text = settings.Nick1 }; gt.Controls.Add(tbNick1);
            Lbl(gt, "nick2", 320, 26, 120); tbNick2 = new TextBox { Location = new Point(443, 26), Width = 160, Text = settings.Nick2 }; gt.Controls.Add(tbNick2);
            Btn(gt, "launchTest", 12, 64, 280, delegate
            {
                SaveLaunchFields();
                if (!StartServer()) return;
                if (!LaunchGame(1, settings.Nick1, "127.0.0.1", 0)) return;
                Thread.Sleep(1500);
                LaunchGame(2, settings.Nick2, "127.0.0.1", 1);
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

        // writes nickname/ip/port into the client config of the given profile
        void WriteClientConfig(int profile, string nick, string ip)
        {
            string userDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "GTA San Andreas User Files");
            Directory.CreateDirectory(userDir);
            string file = Path.Combine(userDir, profile > 0 ? "coopandreas_" + profile + ".ini" : "coopandreas.ini");
            Native.WritePrivateProfileString("config", "nickname", nick, file);
            Native.WritePrivateProfileString("config", "ip", ip, file);
            Native.WritePrivateProfileString("config", "port", settings.Port.ToString(), file);
        }

        // profile 0 = normal play; windowIndex 0/1 = side-by-side debug placement, -1 = none
        bool LaunchGame(int profile, string nick, string ip, int windowIndex)
        {
            if (!installer.IsInstalled) { MessageBox.Show(L.T("needInstall")); return false; }
            string serial = Serial.GetSerial();
            if (string.IsNullOrEmpty(serial)) { tabs.SelectedIndex = 1; MessageBox.Show(L.T("needSerial")); return false; }
            WriteClientConfig(profile, nick, ip);

            string args = "--coop -id " + Serial.GetPcId() + " -serial " + serial;
            if (profile > 0) args += " -profile " + profile;
            if (windowIndex >= 0) args += " --coopd" + windowIndex;
            var psi = new ProcessStartInfo(Path.Combine(settings.GameDir, "gta_sa.exe"), args) { WorkingDirectory = settings.GameDir, UseShellExecute = false };
            try { Process.Start(psi); Log("launch: gta_sa.exe " + args.Replace(serial, "***")); return true; }
            catch (Exception ex) { MessageBox.Show(L.F("failed", ex.Message)); return false; }
        }

        bool ServerRunning { get { return serverProcess != null && !serverProcess.HasExited; } }

        void UpdateServerLabel()
        {
            if (lbServer == null) return;
            lbServer.Text = ServerRunning ? L.F("serverRunning", serverProcess.Id) : L.T("serverStopped");
            lbServer.ForeColor = ServerRunning ? Color.DarkGreen : Color.DimGray;
        }

        bool StartServer()
        {
            if (ServerRunning) return true;
            string exe = Path.Combine(installer.ServerDir, "server.exe");
            if (!File.Exists(exe)) { MessageBox.Show(L.T("needInstall")); return false; }
            // a server started outside of the manager would block the port
            foreach (var pr in Process.GetProcessesByName("server"))
            {
                try { if (string.Equals(pr.MainModule.FileName, exe, StringComparison.OrdinalIgnoreCase)) pr.Kill(); } catch { }
            }
            Directory.CreateDirectory(installer.LogsDir);
            string logPath = Path.Combine(installer.LogsDir, "server.log");
            if (File.Exists(logPath)) File.Copy(logPath, Path.Combine(installer.LogsDir, "server.old.log"), true);
            serverLog = new StreamWriter(new FileStream(logPath, FileMode.Create, FileAccess.Write, FileShare.ReadWrite), new UTF8Encoding(false)) { AutoFlush = true };

            var psi = new ProcessStartInfo(exe, "--no-colors")
            {
                WorkingDirectory = installer.ServerDir, UseShellExecute = false, CreateNoWindow = true,
                RedirectStandardOutput = true, RedirectStandardError = true,
            };
            serverProcess = new Process { StartInfo = psi, EnableRaisingEvents = true };
            DataReceivedEventHandler h = (o, e) =>
            {
                if (e.Data == null) return;
                lock (serverLogLock) { if (serverLog != null) serverLog.WriteLine(StripAnsi(e.Data)); }
            };
            serverProcess.OutputDataReceived += h;
            serverProcess.ErrorDataReceived += h;
            serverProcess.Exited += delegate { BeginInvoke(new Action(UpdateServerLabel)); };
            serverProcess.Start();
            serverProcess.BeginOutputReadLine();
            serverProcess.BeginErrorReadLine();
            Log("server started, log: " + logPath);
            UpdateServerLabel();
            return true;
        }

        void StopServer()
        {
            try { if (ServerRunning) serverProcess.Kill(); } catch { }
            lock (serverLogLock) { if (serverLog != null) { serverLog.Dispose(); serverLog = null; } }
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
            tbLog.Size = new Size(856, 480);
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
                line.IndexOf("[error]", StringComparison.OrdinalIgnoreCase) < 0 && line.IndexOf("[ERROR]") < 0)
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

        // ---- Paths tab
        void BuildPathsTab()
        {
            var p = Page("tabSettings");
            int y = 12;
            tbGame = PathRow(p, "gameDir", ref y, settings.GameDir, true);
            tbSrc = PathRow(p, "srcDir", ref y, settings.SourceDir, true);
            tbZip = PathRow(p, "addZip", ref y, settings.AdditionalZip, false);
            Lbl(p, "repoUrl", 8, y, 160);
            tbRepoUrl = new TextBox { Location = new Point(170, y), Width = 560, Text = settings.RepoUrl };
            p.Controls.Add(tbRepoUrl);
            Btn(p, "clone", 740, y - 3, 110, delegate
            {
                SavePaths();
                var git = new Git(settings);
                if (git.RepoExists || settings.RepoUrl.Length == 0) return;
                if (settings.RepoUrl.IndexOf("Tornamic/CoopAndreas", StringComparison.OrdinalIgnoreCase) >= 0) { MessageBox.Show(L.T("unsafeOrigin")); return; }
                string parent = Path.GetDirectoryName(settings.SourceDir.TrimEnd('\\'));
                Directory.CreateDirectory(parent);
                RunBusy(() => RunTool("git", "clone -b " + settings.Branch + " \"" + settings.RepoUrl + "\" \"" + settings.SourceDir + "\"", parent));
            });
            y += 40;
            Lbl(p, "branch", 8, y, 160);
            tbBranch = new TextBox { Location = new Point(170, y), Width = 200, Text = settings.Branch };
            p.Controls.Add(tbBranch);
            y += 40;
            Btn(p, "save", 170, y + 6, 160, delegate { SavePaths(); });
        }

        void SavePaths()
        {
            settings.GameDir = tbGame.Text.Trim();
            settings.SourceDir = tbSrc.Text.Trim();
            settings.AdditionalZip = tbZip.Text.Trim();
            settings.RepoUrl = tbRepoUrl.Text.Trim();
            settings.Branch = tbBranch.Text.Trim().Length > 0 ? tbBranch.Text.Trim() : "main";
            settings.Save();
            RefreshStatus();
            FillLogList();
        }

        TextBox PathRow(Control p, string key, ref int y, string value, bool folder)
        {
            Lbl(p, key, 8, y, 160);
            var tb = new TextBox { Location = new Point(170, y), Width = 560, Text = value };
            p.Controls.Add(tb);
            Btn(p, "browse", 740, y - 3, 110, delegate
            {
                if (folder)
                {
                    using (var d = new FolderBrowserDialog { SelectedPath = tb.Text }) if (d.ShowDialog() == DialogResult.OK) tb.Text = d.SelectedPath;
                }
                else
                {
                    using (var d = new OpenFileDialog { Filter = "zip|*.zip|*.*|*.*" }) if (d.ShowDialog() == DialogResult.OK) tb.Text = d.FileName;
                }
            });
            y += 40;
            return tb;
        }
    }

    static class Program
    {
        [DllImport("kernel32.dll")]
        static extern bool AttachConsole(int pid);

        // command line mode (used by scripts/automation): --install --repair --uninstall --verify
        static int RunCli(string[] args, string command)
        {
            AttachConsole(-1);
            var settings = Settings.Load();
            ApplyRepoArg(settings, args);
            L.Uk = false;
            Directory.CreateDirectory(Settings.AppDataDir);
            string logPath = Path.Combine(Settings.AppDataDir, "manager_cli.log");
            using (var w = new StreamWriter(logPath, false, new UTF8Encoding(false)) { AutoFlush = true })
            {
                Action<string> log = line => { Console.WriteLine(line); w.WriteLine(line); };
                var inst = new Installer(settings, log);
                try
                {
                    switch (command)
                    {
                        case "--install": inst.Install(false); break;
                        case "--status":
                            var git = new Git(settings);
                            log("source: " + settings.SourceDir + " origin: " + git.OriginUrl + " safe: " + git.OriginIsSafe);
                            log("bin: " + inst.BinDir + " exists: " + inst.BuildExists() + " installed: " + inst.IsInstalled);
                            break;
                        case "--repair": inst.Repair(); break;
                        case "--uninstall": inst.Uninstall(); break;
                        case "--verify":
                            var problems = inst.Verify();
                            foreach (var p in problems) log("! " + p);
                            log(problems.Count == 0 ? "OK" : problems.Count + " problem(s)");
                            return problems.Count == 0 ? 0 : 2;
                        default: log("unknown command " + command); return 1;
                    }
                }
                catch (Exception ex) { log("ERROR: " + ex.Message); return 1; }
            }
            return 0;
        }

        // finds the git repo root that contains the given directory (the exe lives in <repo>\dist)
        static string FindRepoRoot(string dir)
        {
            for (var d = new DirectoryInfo(dir); d != null; d = d.Parent)
                if (Directory.Exists(Path.Combine(d.FullName, ".git"))) return d.FullName;
            return null;
        }

        static string[] StripRepoArg(string[] args)
        {
            var list = args.ToList();
            int i = list.IndexOf("--repo");
            if (i >= 0) list.RemoveRange(i, Math.Min(2, list.Count - i));
            return list.ToArray();
        }

        static void ApplyRepoArg(Settings settings, string[] args)
        {
            int i = Array.IndexOf(args, "--repo");
            if (i >= 0 && i + 1 < args.Length && settings.SourceDir != args[i + 1])
            {
                settings.SourceDir = args[i + 1];
                settings.Save();
            }
        }

        [STAThread]
        static int Main(string[] args)
        {
            // when started from the repo (dist\CoopAndreasManager.exe) run a copy from %LOCALAPPDATA%,
            // otherwise `git pull` could not replace the running exe during an update
            string exe = Application.ExecutablePath;
            string repo = FindRepoRoot(Path.GetDirectoryName(exe));
            if (repo != null && Array.IndexOf(args, "--repo") < 0)
            {
                Directory.CreateDirectory(Settings.AppDataDir);
                string copy = Path.Combine(Settings.AppDataDir, "CoopAndreasManager.exe");
                try
                {
                    File.Copy(exe, copy, true);
                    var rest = string.Join(" ", args.Select(a => "\"" + a + "\""));
                    var p = Process.Start(new ProcessStartInfo(copy, "--repo \"" + repo + "\" " + rest) { UseShellExecute = false });
                    if (args.Length > 0) { p.WaitForExit(); return p.ExitCode; }
                    return 0;
                }
                catch (IOException) { } // a copy is already running: fall through and run in place
            }

            var cmdArgs = StripRepoArg(args);
            if (cmdArgs.Length > 0) return RunCli(args, cmdArgs[0]);
            var settings = Settings.Load();
            ApplyRepoArg(settings, args);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
            return 0;
        }
    }
}

