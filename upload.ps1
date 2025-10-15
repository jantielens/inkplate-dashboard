# Arduino CLI Upload Script for Multiple Inkplate Boards
# This script uploads the Arduino sketch using Arduino CLI

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet('inkplate2', 'inkplate5v2', 'inkplate10')]
    [string]$Board = "inkplate5v2",
    
    [Parameter(Mandatory=$false)]
    [string]$Port = ""
)

# Board configurations
$boards = @{
    'inkplate2' = @{
        Name = "Inkplate 2"
        FQBN = "Inkplate_Boards:esp32:Inkplate2"
        Path = "boards/inkplate2"
    }
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

$config = $boards[$Board]
$SKETCH_PATH = $config.Path
$BOARD_FQBN = $config.FQBN
$BUILD_DIR = "build/$Board"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Uploading to: $($config.Name)" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

# Auto-detect port if not specified
if (!$Port) {
    Write-Host "`nAuto-detecting board..." -ForegroundColor Yellow
    $boardList = arduino-cli board list
    Write-Host $boardList
    
    # Try to find ESP32 board
    $detectedPort = $boardList | Select-String -Pattern "(COM\d+)" | ForEach-Object { $_.Matches[0].Value } | Select-Object -First 1
    
    if ($detectedPort) {
        $Port = $detectedPort
        Write-Host "✓ Detected board on port: $Port" -ForegroundColor Green
    } else {
        Write-Host "✗ ERROR: No board detected!" -ForegroundColor Red
        Write-Host "`nPlease ensure:" -ForegroundColor Yellow
        Write-Host "  1. The Inkplate is connected via USB" -ForegroundColor Yellow
        Write-Host "  2. The USB drivers are installed" -ForegroundColor Yellow
        Write-Host "  3. No other program is using the serial port" -ForegroundColor Yellow
        Write-Host "`nOr specify port manually: .\upload.ps1 -Board $Board -Port COM7" -ForegroundColor Cyan
        exit 1
    }
}

Write-Host "`nPort: $Port" -ForegroundColor Cyan
Write-Host "Board: $($config.Name)" -ForegroundColor Cyan

# Check if build exists
$binaryFile = "$BUILD_DIR\$Board.ino.bin"
if (Test-Path $binaryFile) {
    Write-Host "✓ Found existing build" -ForegroundColor Green
    Write-Host "`nUploading to $Port..." -ForegroundColor Yellow
    arduino-cli upload --fqbn $BOARD_FQBN --port $Port --input-dir $BUILD_DIR $SKETCH_PATH
} else {
    Write-Host "No existing build found, compiling first..." -ForegroundColor Yellow
    
    # Create build directory if it doesn't exist
    if (!(Test-Path $BUILD_DIR)) {
        New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
    }
    
    arduino-cli compile --fqbn $BOARD_FQBN --build-path $BUILD_DIR --upload --port $Port $SKETCH_PATH
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "✓ Upload successful!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
} else {
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "✗ Upload failed!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "`nTroubleshooting tips:" -ForegroundColor Yellow
    Write-Host "  1. Check that the board is connected and powered" -ForegroundColor Yellow
    Write-Host "  2. Try a different USB cable or port" -ForegroundColor Yellow
    Write-Host "  3. Close any serial monitors or other programs using the port" -ForegroundColor Yellow
    Write-Host "  4. Press the reset button on the Inkplate during upload" -ForegroundColor Yellow
    Write-Host "  5. Verify correct board: .\upload.ps1 -Board $Board" -ForegroundColor Yellow
    exit 1
}
