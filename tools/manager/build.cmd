@echo off
rem Builds dist\CoopAndreasManager.exe with the C# compiler that ships with Windows (.NET Framework 4.x)
cd /d "%~dp0"
"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /optimize+ ^
  /out:..\..\dist\CoopAndreasManager.exe /win32icon:..\..\launcher\icon.ico ^
  /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll ^
  CoopManager.cs
