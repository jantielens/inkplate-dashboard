# Test Runner Script for Inkplate Dashboard Unit Tests
# Builds and runs unit tests using CMake + Google Test

param(
    [switch]$Clean,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

Write-Host "=== Inkplate Dashboard Unit Test Runner ===" -ForegroundColor Cyan
Write-Host ""

# Check if CMake is installed
$cmakeVersion = cmake --version 2>$null
if (-not $cmakeVersion) {
    Write-Host "ERROR: CMake not found. Please install CMake:" -ForegroundColor Red
    Write-Host "  winget install Kitware.CMake" -ForegroundColor Yellow
    exit 1
}

Write-Host "CMake version:" -ForegroundColor Green
Write-Host "  $($cmakeVersion[0])" -ForegroundColor Gray
Write-Host ""

# Navigate to test directory
$testDir = Join-Path $PSScriptRoot "test"
$buildDir = Join-Path $testDir "build"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# Create build directory
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Push-Location $buildDir

try {
    # Configure
    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    $cmakeConfig = if ($Verbose) { "-DCMAKE_VERBOSE_MAKEFILE=ON" } else { "" }
    cmake .. $cmakeConfig
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    Write-Host ""
    
    # Build
    Write-Host "Building tests..." -ForegroundColor Cyan
    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    Write-Host ""
    
    # Run tests
    Write-Host "Running tests..." -ForegroundColor Cyan
    Write-Host ""
    ctest -C Release --output-on-failure --verbose
    $testResult = $LASTEXITCODE
    
    Write-Host ""
    if ($testResult -eq 0) {
        Write-Host "✅ All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "❌ Some tests failed" -ForegroundColor Red
        exit $testResult
    }
    
} catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
    Pop-Location
    exit 1
} finally {
    Pop-Location
}

Write-Host ""
Write-Host "Test run complete" -ForegroundColor Cyan
