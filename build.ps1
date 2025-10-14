# Arduino CLI Build Script for Multiple Inkplate Boards
# This script compiles the Arduino sketch using Arduino CLI

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet('inkplate5v2', 'inkplate10', 'all')]
    [string]$Board = "inkplate5v2"
)

# Get absolute path to workspace
$WORKSPACE_PATH = (Get-Location).Path
$COMMON_PATH = Join-Path $WORKSPACE_PATH "common"

# Extract firmware version from version.h
$VERSION_FILE = Join-Path $COMMON_PATH "src\version.h"
if (Test-Path $VERSION_FILE) {
    $versionLine = Get-Content $VERSION_FILE | Where-Object { $_ -match '#define FIRMWARE_VERSION "' }
    if ($versionLine -match '"([^"]+)"') {
        $FIRMWARE_VERSION = $matches[1]
        Write-Host "Firmware version: $FIRMWARE_VERSION" -ForegroundColor Cyan
    } else {
        Write-Host "Warning: Could not parse version from version.h" -ForegroundColor Yellow
        $FIRMWARE_VERSION = "unknown"
    }
} else {
    Write-Host "Warning: version.h not found, using 'unknown' as version" -ForegroundColor Yellow
    $FIRMWARE_VERSION = "unknown"
}

# Board configurations
$boards = @{
    'inkplate5v2' = @{
        Name = "Inkplate 5 V2"
        FQBN = "Inkplate_Boards:esp32:Inkplate5V2"
        Path = "boards/inkplate5v2"
    }
    'inkplate10' = @{
        Name = "Inkplate 10"
        FQBN = "Inkplate_Boards:esp32:Inkplate10"
        Path = "boards/inkplate10"
    }
}

function Build-Board {
    param(
        [string]$BoardKey
    )
    
    $config = $boards[$BoardKey]
    $SKETCH_PATH = $config.Path
    $BOARD_FQBN = $config.FQBN
    $BUILD_DIR = "build/$BoardKey"
    
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Building: $($config.Name)" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    
    # Create build directory if it doesn't exist
    if (!(Test-Path $BUILD_DIR)) {
        New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
    }
    
    # Copy common source files directly to sketch directory for Arduino to compile
    $commonSrcFiles = Get-ChildItem -Path (Join-Path $COMMON_PATH "src") -Filter "*.cpp"
    foreach ($file in $commonSrcFiles) {
        $destPath = Join-Path $SKETCH_PATH $file.Name
        Copy-Item -Path $file.FullName -Destination $destPath -Force
        Write-Host "Copied $($file.Name) to sketch directory" -ForegroundColor Gray
    }
    
    # Compile the sketch with custom library path and build properties
    Write-Host "Compiling $SKETCH_PATH..." -ForegroundColor Yellow
    Write-Host "Including common libraries from: $COMMON_PATH" -ForegroundColor Gray
    
    arduino-cli compile --fqbn $BOARD_FQBN --build-path $BUILD_DIR --library $COMMON_PATH --build-property "compiler.cpp.extra_flags=-I`"$COMMON_PATH`" -I`"$COMMON_PATH\src`"" $SKETCH_PATH
    
    # Clean up copied files after build
    foreach ($file in $commonSrcFiles) {
        $destPath = Join-Path $SKETCH_PATH $file.Name
        Remove-Item -Path $destPath -Force -ErrorAction SilentlyContinue
    }
    
    if ($LASTEXITCODE -eq 0) {
        # Rename binary to include version
        $ORIGINAL_BIN = Join-Path $BUILD_DIR "$BoardKey.ino.bin"
        $VERSIONED_BIN = Join-Path $BUILD_DIR "$BoardKey-v$FIRMWARE_VERSION.bin"
        
        if (Test-Path $ORIGINAL_BIN) {
            Copy-Item -Path $ORIGINAL_BIN -Destination $VERSIONED_BIN -Force
            Write-Host "Build successful!" -ForegroundColor Green
            Write-Host "Build artifacts: $BUILD_DIR" -ForegroundColor Cyan
            Write-Host "Firmware binary: $BoardKey-v$FIRMWARE_VERSION.bin" -ForegroundColor Cyan
        } else {
            Write-Host "Build successful (binary not found for renaming)!" -ForegroundColor Green
            Write-Host "Build artifacts: $BUILD_DIR" -ForegroundColor Cyan
        }
        return $true
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        return $false
    }
}

# Main execution
Write-Host "=== Multi-Board Build System ===" -ForegroundColor Cyan

$success = $true

if ($Board -eq "all") {
    Write-Host "Building all boards..." -ForegroundColor Yellow
    foreach ($boardKey in $boards.Keys) {
        if (!(Build-Board $boardKey)) {
            $success = $false
        }
    }
} else {
    if (!(Build-Board $Board)) {
        $success = $false
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($success) {
    Write-Host "All builds completed successfully!" -ForegroundColor Green
} else {
    Write-Host "Some builds failed!" -ForegroundColor Red
    exit 1
}
Write-Host "========================================" -ForegroundColor Cyan
