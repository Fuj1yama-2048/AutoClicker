@echo off
echo ============================================
echo   Clear AutoClicker Registry Settings
echo ============================================
echo.
echo   Will delete: HKCU\Software\AutoClicker
echo   Next launch will use default settings.
echo.
set /p confirm="   Continue? (Y/N): "
if /i "%confirm%" neq "Y" (
    echo   Cancelled.
    goto :end
)
reg delete "HKCU\Software\AutoClicker" /f >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   Cleared!
) else (
    echo   Key not found (already clean).
)
:end
pause
