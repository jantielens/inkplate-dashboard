# Arduino CLI Setup Script for Multiple Inkplate Boards
# This script ensures all prerequisites are installed

# Configuration
$BOARD_URL = "https://raw.githubusercontent.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE/master/package_Dasduino_Boards_index.json"
$BOARD_PACKAGE = "Inkplate_Boards:esp32"
$REQUIRED_BOARDS = @("Inkplate2", "Inkplate5V2", "Inkplate10", "Inkplate6Flick")

Write-Host "=== Arduino CLI Setup for Multiple Inkplate Boards ===" -ForegroundColor Cyan

# Check if Arduino CLI is installed
Write-Host "`nChecking Arduino CLI installation..." -ForegroundColor Yellow
$arduinoCli = Get-Command arduino-cli -ErrorAction SilentlyContinue
if (!$arduinoCli) {
    Write-Host "ERROR: Arduino CLI is not installed!" -ForegroundColor Red
    Write-Host "Please install it from: https://arduino.github.io/arduino-cli/latest/installation/" -ForegroundColor Yellow
    exit 1
}
Write-Host "✓ Arduino CLI found: $($arduinoCli.Version)" -ForegroundColor Green

# Initialize config if needed
Write-Host "`nChecking Arduino CLI configuration..." -ForegroundColor Yellow
$configCheck = arduino-cli config dump 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Initializing Arduino CLI configuration..." -ForegroundColor Yellow
    arduino-cli config init
}
Write-Host "✓ Configuration OK" -ForegroundColor Green

# Check and add board manager URL
Write-Host "`nChecking board manager URLs..." -ForegroundColor Yellow
$currentUrls = arduino-cli config dump | ConvertFrom-Json | Select-Object -ExpandProperty board_manager -ErrorAction SilentlyContinue | Select-Object -ExpandProperty additional_urls -ErrorAction SilentlyContinue

if ($currentUrls -notcontains $BOARD_URL) {
    Write-Host "Adding Inkplate board definitions URL..." -ForegroundColor Yellow
    arduino-cli config add board_manager.additional_urls $BOARD_URL
    Write-Host "✓ Board URL added" -ForegroundColor Green
} else {
    Write-Host "✓ Board URL already configured" -ForegroundColor Green
}

# Update board index
Write-Host "`nUpdating board index..." -ForegroundColor Yellow
arduino-cli core update-index
Write-Host "✓ Board index updated" -ForegroundColor Green

# Check if Inkplate board package is installed
Write-Host "`nChecking Inkplate board package..." -ForegroundColor Yellow
$installedCores = arduino-cli core list | Out-String
if ($installedCores -match "Inkplate_Boards:esp32") {
    Write-Host "✓ Inkplate board package already installed" -ForegroundColor Green
    
    # Check for updates
    Write-Host "Checking for board package updates..." -ForegroundColor Yellow
    arduino-cli core upgrade $BOARD_PACKAGE
} else {
    Write-Host "Installing Inkplate board package..." -ForegroundColor Yellow
    arduino-cli core install $BOARD_PACKAGE
    Write-Host "✓ Inkplate board package installed" -ForegroundColor Green
}

# Verify the specific boards are available
Write-Host "`nVerifying Inkplate boards..." -ForegroundColor Yellow
$allBoards = arduino-cli board listall inkplate | Out-String

$allFound = $true
foreach ($board in $REQUIRED_BOARDS) {
    if ($allBoards -match $board) {
        Write-Host "✓ $board is available" -ForegroundColor Green
    } else {
        Write-Host "✗ WARNING: $board not found!" -ForegroundColor Red
        $allFound = $false
    }
}

if (!$allFound) {
    Write-Host "`nAvailable Inkplate boards:" -ForegroundColor Yellow
    arduino-cli board listall inkplate
    Write-Host "`nSome boards are missing, but you can still use the available ones." -ForegroundColor Yellow
}

# Check for required libraries (you can add more as needed)
Write-Host "`nChecking required libraries..." -ForegroundColor Yellow
$requiredLibraries = @(
    "PubSubClient"  # MQTT client library for Home Assistant integration
)

foreach ($lib in $requiredLibraries) {
    $libCheck = arduino-cli lib list | Select-String $lib
    if ($libCheck) {
        Write-Host "✓ Library '$lib' is installed" -ForegroundColor Green
    } else {
        Write-Host "Installing library '$lib'..." -ForegroundColor Yellow
        arduino-cli lib install $lib
    }
}
}

Write-Host "`n=== Setup Complete ===" -ForegroundColor Green
Write-Host "You can now run .\build.ps1 to compile your sketch" -ForegroundColor Cyan
