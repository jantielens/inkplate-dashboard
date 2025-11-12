# Download GFXfonts from Inkplate Arduino Library
# Fonts are sourced from: https://github.com/SolderedElectronics/Inkplate-Arduino-library/tree/master/Fonts

$fontsBaseUrl = "https://raw.githubusercontent.com/SolderedElectronics/Inkplate-Arduino-library/master/Fonts"
$destDir = Join-Path $PSScriptRoot "..\common\src\fonts"

# Create fonts directory if it doesn't exist
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
    Write-Host "Created directory: $destDir"
}

# Font files to download
$fonts = @(
    "Picopixel.h",
    "FreeSans9pt7b.h",
    "FreeSans12pt7b.h",
    "FreeSansBold18pt7b.h",
    "FreeSansBold24pt7b.h"
)

Write-Host "Downloading fonts from Inkplate library..."
Write-Host ""

foreach ($font in $fonts) {
    $url = "$fontsBaseUrl/$font"
    $destPath = Join-Path $destDir $font
    
    try {
        Write-Host "Downloading $font..." -NoNewline
        Invoke-WebRequest -Uri $url -OutFile $destPath -ErrorAction Stop
        Write-Host " Done" -ForegroundColor Green
    }
    catch {
        Write-Host " Failed" -ForegroundColor Red
        Write-Host "Error: $_" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "All fonts downloaded successfully to: $destDir" -ForegroundColor Green
