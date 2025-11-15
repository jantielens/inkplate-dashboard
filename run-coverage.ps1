# Code Coverage Runner for Inkplate Dashboard Tests
# Uses OpenCppCoverage on Windows to generate HTML coverage reports

param(
    [switch]$Clean,
    [switch]$Install
)

$ErrorActionPreference = "Stop"

Write-Host "=== Inkplate Dashboard Code Coverage ===" -ForegroundColor Cyan
Write-Host ""

# Check if OpenCppCoverage is installed
$openCppCoverage = Get-Command "OpenCppCoverage.exe" -ErrorAction SilentlyContinue

if (-not $openCppCoverage) {
    Write-Host "ERROR: OpenCppCoverage not found" -ForegroundColor Red
    Write-Host ""
    Write-Host "To install OpenCppCoverage:" -ForegroundColor Yellow
    Write-Host "  1. Download from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases" -ForegroundColor Gray
    Write-Host "  2. Run installer and add to PATH" -ForegroundColor Gray
    Write-Host "  OR use Chocolatey: choco install opencppcoverage" -ForegroundColor Gray
    Write-Host ""
    
    if ($Install) {
        Write-Host "Attempting to install via winget..." -ForegroundColor Yellow
        winget install --id OpenCppCoverage.OpenCppCoverage -e
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Installation failed. Please install manually." -ForegroundColor Red
            exit 1
        }
        Write-Host "Please restart PowerShell and run this script again." -ForegroundColor Green
        exit 0
    }
    
    exit 1
}

Write-Host "OpenCppCoverage found: $($openCppCoverage.Source)" -ForegroundColor Green
Write-Host ""

# Navigate to test directory
$testDir = Join-Path $PSScriptRoot "test"
$buildDir = Join-Path $testDir "build"
$coverageDir = Join-Path $testDir "coverage"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

if ($Clean -and (Test-Path $coverageDir)) {
    Write-Host "Cleaning coverage directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $coverageDir
}

# Create directories
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

if (-not (Test-Path $coverageDir)) {
    New-Item -ItemType Directory -Path $coverageDir | Out-Null
}

Push-Location $buildDir

try {
    # Configure with Debug build for better coverage
    Write-Host "Configuring CMake (Debug build)..." -ForegroundColor Cyan
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    Write-Host ""
    
    # Build
    Write-Host "Building tests..." -ForegroundColor Cyan
    cmake --build . --config Debug
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    Write-Host ""
    
    # Get all test executables
    $testExecutables = @(
        "Debug\decision_tests.exe",
        "Debug\battery_tests.exe",
        "Debug\sleep_tests.exe",
        "Debug\config_tests.exe",
        "Debug\integration_tests.exe"
    )
    
    # Run coverage for each test executable
    Write-Host "Running tests with coverage..." -ForegroundColor Cyan
    Write-Host ""
    
    $commonSrcPath = Resolve-Path "..\..\..\common\src" | Select-Object -ExpandProperty Path
    $coverageOutput = Join-Path $PSScriptRoot "test\coverage"
    
    # Combine coverage from all test executables
    $firstTest = $true
    foreach ($testExe in $testExecutables) {
        $testName = [System.IO.Path]::GetFileNameWithoutExtension($testExe)
        Write-Host "  Running $testName..." -ForegroundColor Gray
        
        if ($firstTest) {
            # First test: create new coverage
            & OpenCppCoverage.exe `
                --sources "$commonSrcPath" `
                --export_type html:"$coverageOutput" `
                --export_type cobertura:"$coverageOutput\coverage.xml" `
                --excluded_sources "*\test\*" `
                --excluded_sources "*\mocks\*" `
                --excluded_sources "*\googletest\*" `
                -- $testExe --gtest_brief=1
            
            if ($LASTEXITCODE -ne 0) {
                throw "Coverage run failed for $testName"
            }
            $firstTest = $false
        } else {
            # Subsequent tests: merge with existing coverage
            & OpenCppCoverage.exe `
                --sources "$commonSrcPath" `
                --input_coverage "$coverageOutput\coverage.xml" `
                --export_type cobertura:"$coverageOutput\coverage.xml" `
                --excluded_sources "*\test\*" `
                --excluded_sources "*\mocks\*" `
                --excluded_sources "*\googletest\*" `
                -- $testExe --gtest_brief=1
            
            if ($LASTEXITCODE -ne 0) {
                throw "Coverage run failed for $testName"
            }
        }
    }
    
    Write-Host ""
    Write-Host "✅ Coverage analysis complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Coverage reports:" -ForegroundColor Cyan
    Write-Host "  HTML: $coverageOutput\index.html" -ForegroundColor Gray
    Write-Host "  XML:  $coverageOutput\coverage.xml" -ForegroundColor Gray
    Write-Host ""
    
    # Open HTML report
    $htmlReport = Join-Path $coverageOutput "index.html"
    if (Test-Path $htmlReport) {
        Write-Host "Opening coverage report in browser..." -ForegroundColor Yellow
        Start-Process $htmlReport
    }
    
} catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
    Pop-Location
    exit 1
} finally {
    Pop-Location
}

Write-Host "Coverage run complete" -ForegroundColor Cyan
