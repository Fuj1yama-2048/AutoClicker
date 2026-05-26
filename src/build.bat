@echo off
cd /d "%~dp0"

:: Auto-load MSVC environment
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto :check_compiler
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto :check_compiler
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    goto :check_compiler
)

:check_compiler
echo ============================================
echo   Auto Clicker - Build
echo ============================================
echo.

if not exist "..\bin" mkdir "..\bin"
if exist "*.obj" del /q "*.obj" 2>nul
if exist "*.res" del /q "*.res" 2>nul

echo   Killing old process...
taskkill /f /im AutoClicker.exe >nul 2>&1

where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [MSVC detected]
    echo   Compiling resources...
    rc /nologo /fo resources.res resources.rc
    if %ERRORLEVEL% NEQ 0 (
        echo   Resource compile failed!
        goto :end
    )
    echo   Compiling code...
    cl /utf-8 /EHsc /O2 /MT /nologo /Fe:..\bin\AutoClicker.exe main.cpp resources.res user32.lib gdi32.lib shell32.lib comdlg32.lib advapi32.lib winmm.lib
    if %ERRORLEVEL% EQU 0 (
        if exist "*.obj" del /q "*.obj" 2>nul
        if exist "*.res" del /q "*.res" 2>nul
        echo.
        echo ============================================
        echo   Build OK! Output: ..\bin\AutoClicker.exe
        echo ============================================
        echo.
        echo   Starting...
        start "" "..\bin\AutoClicker.exe"
    ) else (
        echo.
        echo ============================================
        echo   Build FAILED
        echo ============================================
    )
    goto :end
)

where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [MinGW detected]
    echo   Compiling resources...
    windres resources.rc resources.o
    g++ -O2 -std=c++17 -o ..\bin\AutoClicker.exe main.cpp resources.o -static -lgdi32 -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lwinmm -mwindows
    if %ERRORLEVEL% EQU 0 (
        if exist "*.o" del /q "*.o" 2>nul
        echo.
        echo ============================================
        echo   Build OK! Output: ..\bin\AutoClicker.exe
        echo ============================================
        echo.
        echo   Starting...
        start "" "..\bin\AutoClicker.exe"
    ) else (
        echo.
        echo ============================================
        echo   Build FAILED
        echo ============================================
    )
    goto :end
)

echo [ERROR] No C++ compiler found!
echo.
echo Install either:
echo   1. Visual Studio (MSVC)
echo   2. MinGW-w64 (https://winlibs.com)
echo.
pause
:end
