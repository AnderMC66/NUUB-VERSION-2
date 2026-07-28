@echo off
setlocal enabledelayedexpansion

set "INSTALL_DIR=%APPDATA%\NUUB\bin"

echo Agregando %INSTALL_DIR% a PATH...
setx PATH "!PATH!;%INSTALL_DIR%"

echo.
echo PATH actualizado. Abre una nueva terminal para verlo.
echo Ahora puedes escribir: nuub.exe desde cualquier carpeta
echo.
pause
