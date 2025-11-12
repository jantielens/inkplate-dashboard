# Download GFXfonts from multiple sources
# - FreeSans7pt7b from jmchiappa/GFXFonts (for Inkplate2)
# - Roboto fonts from rusconi/gfxFonts (for larger boards)

$destDir = Join-Path $PSScriptRoot "..\common\src\fonts"

# Create fonts directory if it doesn't exist
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir | Out-Null
    Write-Host "Created directory: $destDir"
}

# Font files to download with their sources
$fonts = @(
    @{
        Name = "FreeSans7pt7b.h"
        Url = "https://raw.githubusercontent.com/jmchiappa/GFXFonts/master/Fonts/FreeSans7pt7b.h"
        Description = "FreeSans 7pt (for Inkplate2 - 16px height)"
    },
    @{
        Name = "Roboto_Bold24pt7b.h"
        Url = "https://raw.githubusercontent.com/rusconi/gfxFonts/main/Roboto_Bold24pt7b.h"
        Description = "Roboto Bold 24pt (for large headings)"
    },
    @{
        Name = "Roboto_Bold20pt7b.h"
        Url = "https://raw.githubusercontent.com/rusconi/gfxFonts/main/Roboto_Bold20pt7b.h"
        Description = "Roboto Bold 20pt (for medium headings)"
    },
    @{
        Name = "Roboto_Regular12pt7b.h"
        Url = "https://raw.githubusercontent.com/rusconi/gfxFonts/main/Roboto_Regular12pt7b.h"
        Description = "Roboto Regular 12pt (for normal text)"
    }
)

Write-Host "Downloading fonts from multiple sources..."
Write-Host ""

foreach ($font in $fonts) {
    $destPath = Join-Path $destDir $font.Name
    
    try {
        Write-Host "Downloading $($font.Name)..." -NoNewline
        Invoke-WebRequest -Uri $font.Url -OutFile $destPath -ErrorAction Stop
        Write-Host " Done" -ForegroundColor Green
        Write-Host "  -> $($font.Description)" -ForegroundColor Gray
    }
    catch {
        Write-Host " Failed" -ForegroundColor Red
        Write-Host "Error: $_" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "All fonts downloaded successfully to: $destDir" -ForegroundColor Green
