@echo off
rem Builds both editions with the C# compiler that ships with Windows (.NET Framework 4.x):
rem   build\manager\CoopAndreasManager.exe  - developer manager (build, publish, tests, logs)
rem   build\manager\SADreamLauncher.exe     - simple launcher for players (install/update, play)
cd /d "%~dp0"
if not exist ..\..\build\manager mkdir ..\..\build\manager
set CSC="%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /optimize+ /nowarn:162,429 /win32icon:..\..\launcher\icon.ico /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll
%CSC% /out:..\..\build\manager\CoopAndreasManager.exe CoopManager.cs || exit /b 1
%CSC% /define:PLAYER /out:..\..\build\manager\SADreamLauncher.exe CoopManager.cs || exit /b 1
