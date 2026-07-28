@echo off
REM Demo Mode - Runs NUUB for 10 seconds for testing
REM Then exits automatically

setlocal enabledelayedexpansion

cd /d "%~dp0NUUB-USB-Installer\bin"
set NUUB_LIB_DIR=%cd%\lib
set PATH=%NUUB_LIB_DIR%;%PATH%

echo.
echo ════════════════════════════════════════════════════════════
echo                      NUUB - DEMO MODE
echo ════════════════════════════════════════════════════════════
echo.
echo Starting NUUB in DEMO mode (will auto-exit in 10 seconds)
echo.

timeout /t 3 /nobreak

REM Start NUUB in background and capture PID
start /b nuub.exe

REM Get the PID of the last started process
for /f "tokens=2" %%A in ('tasklist ^| find /I "nuub.exe"') do (
    set PID=%%A
    goto found
)

:found
if defined PID (
    echo.
    echo ✓ NUUB started successfully (PID: %PID%)
    echo.
    echo Running for 10 seconds...
    timeout /t 10 /nobreak
    
    echo.
    echo Stopping NUUB Demo...
    taskkill /PID %PID% /F >nul 2>&1
    echo ✓ Demo closed
) else (
    echo ERROR: Failed to start NUUB
    pause
)

echo.
echo ════════════════════════════════════════════════════════════
echo Demo completed. For full agent mode, run:
echo   C:\Users\Asus\AppData\Roaming\NUUB\bin\launcher.bat
echo ════════════════════════════════════════════════════════════
echo.

endlocal
