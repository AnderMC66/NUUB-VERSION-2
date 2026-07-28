@echo off
setlocal enabledelayedexpansion

set "INSTALL_DIR=%APPDATA%\NUUB"

echo.
echo Desinstalando NUUB de: %INSTALL_DIR%
echo.

REM Eliminar acceso directo
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\NUUB.lnk" 2>nul

REM Eliminar carpeta
if exist "%INSTALL_DIR%" (
    rmdir /S /Q "%INSTALL_DIR%"
    echo Desinstalacion completada.
) else (
    echo NUUB no se encontro instalado.
)

pause
