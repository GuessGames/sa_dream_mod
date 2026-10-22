# Reproduces crashes: a separate server on port 6768 + two game windows (profiles 1 and 2) that connect by themselves.
# The host (first to connect) starts mission -mission <id> by itself (-1 = none), e.g. 13 = Tagging Up Turf.
# -extraArgs "-testteleport": more client flags for every window.
# -windows N: N game windows (profiles 1..N, default 2), e.g. to test more players at once.
# -bots N: N fake players (build\...\botclient.exe) join after the windows and run around Grove Street.
# -outfitProfile 2: that window wears other clothes and fat 995 after connecting (clothes/cutscene tests).
# Never touches a running server of the user (own copy, own port) and aborts if a game is already running.
# Prints the new crash reports and the important log lines, then closes everything it started.
#   powershell -ExecutionPolicy Bypass -File tools\crash\test_two_windows.ps1 -mission 13 -runSeconds 120
param([int]$mission = 13, [int]$runSeconds = 90, [string]$gameDir = "D:\Grand Theft Auto San Andreas", [string]$serverExe = "", [int]$outfitProfile = 0, [int]$windows = 2, [string]$extraArgs = "", [int]$bots = 0)
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

$userDir = Join-Path ([Environment]::GetFolderPath("MyDocuments")) "GTA San Andreas User Files"
$games = @()
$before = @(Get-ChildItem "$g\CoopAndreas_crashes\*.log" -ErrorAction SilentlyContinue).Count

foreach ($p in 1..$windows) {
    $ini = "$userDir\coopandreas_$p.ini"
    # @(): a new profile has one line, a single string would be concatenated by += instead of extended
    $lines = @(if (Test-Path $ini) { Get-Content $ini } else { "[config]" })
    $lines = @($lines | Where-Object { $_ -notmatch '^(\[config\].*|nickname|ip|port|report-crashlogs)=?' })
    $lines = @("[config]", "report-crashlogs=true") + $lines + @("nickname=Tester$p", "ip=127.0.0.1", "port=$port")
    Set-Content $ini $lines -Encoding ascii
    # both get it: only the one that becomes the host launches it
    $extra = if ($mission -ge 0) { " -testmission $mission" } else { "" }
    if ($p -eq $outfitProfile) { $extra += " -testoutfit" }
    if ($extraArgs) { $extra += " $extraArgs" }
    $log = "$g\CoopAndreas\logs\client_$p.log"
    $games += Start-Process "$g\gta_sa.exe" -PassThru -ArgumentList "--coop -profile $p --coopd$([Math]::Min($p-1, 1)) -autoconnect$extra" -WorkingDirectory $g
    # windows that initialise at the same time sometimes quit silently: start the next one after this one is up
    $upBy = (Get-Date).AddSeconds(40)
    Start-Sleep 2
    while ((Get-Date) -lt $upBy -and -not $games[-1].HasExited -and -not (Select-String -Path $log -Pattern "autoconnect" -Quiet -ErrorAction SilentlyContinue)) { Start-Sleep 1 }
    if ($games[-1].HasExited) { "window $p quit during start" }
    Start-Sleep 2
}

$botProc = $null
if ($bots -gt 0) {
    $botExe = Join-Path (Split-Path $PSScriptRoot -Parent | Split-Path -Parent) "build\windows\x86\release\botclient.exe"
    $botProc = Start-Process $botExe -ArgumentList "--port $port --bots $bots --seconds $runSeconds --x 2495 --y -1670 --z 13.4 --radius 6" -WindowStyle Hidden -RedirectStandardOutput "$work\bots.log" -PassThru
}

$deadline = (Get-Date).AddSeconds($runSeconds)
while ((Get-Date) -lt $deadline) {
    Start-Sleep 3
    if (@($games | Where-Object { -not $_.HasExited }).Count -lt $windows) { "a game window closed or crashed"; Start-Sleep 3; break }
}
$after = @(Get-ChildItem "$g\CoopAndreas_crashes\*.log" -ErrorAction SilentlyContinue)
"crash reports: before $before, after $($after.Count)"
$after | Sort-Object LastWriteTime | Select-Object -Last ([Math]::Max(0, $after.Count - $before)) | ForEach-Object { "NEW CRASH: " + $_.FullName }

# only the windows this script started
$games | Where-Object { -not $_.HasExited } | ForEach-Object { Stop-Process -Id $_.Id -Force }
if ($botProc -and -not $botProc.HasExited) { Stop-Process -Id $botProc.Id -Force }
Stop-Process -Id $srvProc.Id -Force -ErrorAction SilentlyContinue
if ($bots -gt 0) { "--- bots"; Get-Content "$work\bots.log" -ErrorAction SilentlyContinue }
"--- server"; Get-Content "$srv\server.log" -ErrorAction SilentlyContinue | Select-String "freeId|disconnected" | ForEach-Object { $_.Line }
foreach ($n in 1..$windows) { "--- client_$n"; Get-Content "$g\CoopAndreas\logs\client_$n.log" -ErrorAction SilentlyContinue | Select-String "\[admin\]|\[mission\] replay|\[crashfix\]|\[obj\]|teleport|bringing|host jumped|host now|error|pause" | Select-Object -Last 25 | ForEach-Object { $_.Line } }
