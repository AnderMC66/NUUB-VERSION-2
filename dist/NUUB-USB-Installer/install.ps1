$INSTALL_DIR = "$env:APPDATA\NUUB"
$USB_DIR = $PSScriptRoot
$BIN_SOURCE = "$USB_DIR\bin"

Write-Host ""
Write-Host "NUUB - USB Quick Installer" -ForegroundColor Cyan
Write-Host ""
Write-Host "Origen: $USB_DIR" -ForegroundColor Green
Write-Host "Destino: $INSTALL_DIR" -ForegroundColor Green
Write-Host ""

if (-not (Test-Path $INSTALL_DIR)) { New-Item -ItemType Directory -Path $INSTALL_DIR -Force | Out-Null }
if (-not (Test-Path "$INSTALL_DIR\bin")) { New-Item -ItemType Directory -Path "$INSTALL_DIR\bin" -Force | Out-Null }

Write-Host "Copiando archivos..." -ForegroundColor Yellow
Get-ChildItem -Path $BIN_SOURCE -Filter "*.exe" -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -Path $_.FullName -Destination "$INSTALL_DIR\bin\$($_.Name)" -Force
    Write-Host "  OK: $($_.Name)" -ForegroundColor Green
}

Write-Host ""
Write-Host "Creando acceso directo..." -ForegroundColor Yellow
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut("$env:APPDATA\Microsoft\Windows\Start Menu\Programs\NUUB.lnk")
$Shortcut.TargetPath = "$INSTALL_DIR\bin\nuub.exe"
$Shortcut.WorkingDirectory = $INSTALL_DIR
$Shortcut.Save()
Write-Host "  OK: Acceso directo creado" -ForegroundColor Green

Write-Host ""
Write-Host "INSTALACION COMPLETADA!" -ForegroundColor Green
Write-Host ""
Write-Host "Ubicacion: $INSTALL_DIR" -ForegroundColor Cyan
Write-Host "Puedes buscar NUUB en el Start Menu" -ForegroundColor Cyan
Write-Host ""
