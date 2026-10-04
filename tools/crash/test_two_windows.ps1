# Reproduces crashes: a separate server on port 6768 + two game windows (profiles 1 and 2) that connect by themselves.
# The host (first to connect) starts mission -mission <id> by itself (-1 = none), e.g. 13 = Tagging Up Turf.
# Never touches a running server of the user (own copy, own port) and aborts if a game is already running.
# Prints the new crash reports and the important log lines, then closes everything it started.
#   powershell -ExecutionPolicy Bypass -File tools\crash\test_two_windows.ps1 -mission 13 -runSeconds 120
param([int]$mission = 13, [int]$runSeconds = 90, [string]$gameDir = "D:\Grand Theft Auto San Andreas", [string]$serverExe = "")
$work = Join-Path $env:TEMP "sadream_crash_test"
$g = $gameDir
if (-not $serverExe) { $serverExe = "$g\CoopAndreasServer\server.exe" }
$port = 6768

if (Get-Process gta_sa -ErrorAction SilentlyContinue) { "ABORT: a game is running (somebody is playing)"; return }

$srv = "$work\srv"
New-Item -ItemType Directory -Force $srv | Out-Null
Copy-Item $serverExe "$srv\server.exe" -Force
Set-Content "$srv\server-config.ini" "port = $port`nmaxplayers = 8" -Encoding ascii
$srvProc = Start-Process "$srv\server.exe" -ArgumentList "--no-colors" -WorkingDirectory $srv -WindowStyle Hidden -RedirectStandardOutput "$srv\server.log" -RedirectStandardError "$srv\server.err" -PassThru
Start-Sleep 2

# same serial/PC id the launcher uses (HKCU\Software\CoopAndreas)
$k = Get-ItemProperty "HKCU:\Software\CoopAndreas"
$md5 = [Security.Cryptography.MD5]::Create()
$pcid = ([BitConverter]::ToString($md5.ComputeHash([Text.Encoding]::ASCII.GetBytes($k.pcid)), 0, 4)).Replace("-", "")
$userDir = Join-Path ([Environment]::GetFolderPath("MyDocuments")) "GTA San Andreas User Files"
$games = @()
$before = @(Get-ChildItem "$g\CoopAndreas_crashes\*.log" -ErrorAction SilentlyContinue).Count

foreach ($p in 1, 2) {
    $ini = "$userDir\coopandreas_$p.ini"
    $lines = if (Test-Path $ini) { Get-Content $ini } else { @("[config]") }
    $lines = $lines | Where-Object { $_ -notmatch '^(nickname|ip|port)=' }
    $lines += "nickname=Tester$p", "ip=127.0.0.1", "port=$port"
    Set-Content $ini $lines -Encoding ascii
    $extra = if ($p -eq 1 -and $mission -ge 0) { " -testmission $mission" } else { "" }
    $games += Start-Process "$g\gta_sa.exe" -PassThru -ArgumentList "--coop -id $pcid -serial $($k.Serialkey) -profile $p --coopd$($p-1) -autoconnect$extra" -WorkingDirectory $g
    Start-Sleep 2
}

$deadline = (Get-Date).AddSeconds($runSeconds)
while ((Get-Date) -lt $deadline) {
    Start-Sleep 3
    if (@($games | Where-Object { -not $_.HasExited }).Count -lt 2) { "a game window closed or crashed"; Start-Sleep 3; break }
}
$after = @(Get-ChildItem "$g\CoopAndreas_crashes\*.log" -ErrorAction SilentlyContinue)
"crash reports: before $before, after $($after.Count)"
$after | Sort-Object LastWriteTime | Select-Object -Last ([Math]::Max(0, $after.Count - $before)) | ForEach-Object { "NEW CRASH: " + $_.FullName }

# only the windows this script started
$games | Where-Object { -not $_.HasExited } | ForEach-Object { Stop-Process -Id $_.Id -Force }
Stop-Process -Id $srvProc.Id -Force -ErrorAction SilentlyContinue
"--- server"; Get-Content "$srv\server.log" -ErrorAction SilentlyContinue | Select-String "freeId|disconnected" | ForEach-Object { $_.Line }
foreach ($n in 1, 2) { "--- client_$n"; Get-Content "$g\CoopAndreas\logs\client_$n.log" -ErrorAction SilentlyContinue | Select-String "\[admin\]|\[mission\] replay|\[crashfix\]|host now|error|window|pause" | Select-Object -Last 10 | ForEach-Object { $_.Line } }
