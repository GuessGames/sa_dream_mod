@echo off
rem Builds build\manager\CoopAndreasManager.exe with the C# compiler that ships with Windows (.NET Framework 4.x)
cd /d "%~dp0"
if not exist ..\..\build\manager mkdir ..\..\build\manager
"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /optimize+ ^
  /out:..\..\build\manager\CoopAndreasManager.exe /win32icon:..\..\launcher\icon.ico ^
  /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll ^
  CoopManager.cs
