@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "d:\C++p\AutoClicker\src"
rc /nologo /fo resources.res resources.rc
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
cl /utf-8 /EHsc /O2 /MT /nologo /Fe:..\bin\AutoClicker.exe main.cpp resources.res user32.lib gdi32.lib shell32.lib comdlg32.lib advapi32.lib winmm.lib
