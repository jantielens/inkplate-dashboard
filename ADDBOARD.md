# Adding a New Inkplate Board

This guide describes the steps required to add support for a new Inkplate board to the firmware.

## Overview

Adding a new board involves creating board-specific configuration files and updating the build system to recognize the new board. The firmware uses a shared codebase with board-specific configurations to support multiple Inkplate devices.

## Step-by-Step Guide

### 1. Create Board Directory Structure

Create a new directory under `boards/` for your board:

```bash
mkdir boards/inkplate-newboard
```

### 2. Create Board Configuration File

Create `boards/inkplate-newboard/board_config.h` with the following template:

```cpp
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board identification
#define BOARD_NAME "Inkplate NewBoard"
#define BOARD_TYPE INKPLATE_NEWBOARD

// Display specifications
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BOARD_ROTATION 0  // Default rotation (not used - rotation is user-configurable)

// Display mode
// For standard Inkplate boards (5, 6, 10): Use INKPLATE_1BIT or INKPLATE_3BIT
// For Inkplate 2: Use DISPLAY_MODE_INKPLATE2 (no parameter in constructor)
#define DISPLAY_MODE INKPLATE_3BIT

// Board-specific features
#define HAS_TOUCHSCREEN false
#define HAS_FRONTLIGHT false
#define HAS_BATTERY true
#define HAS_BUTTON true  // Set to false if no physical button

// Board-specific pins
#define WAKE_BUTTON_PIN 36  // GPIO pin for wake button (if HAS_BUTTON is true)

// Battery monitoring pins
#define BATTERY_ADC_PIN 35  // GPIO35 - ADC pin for battery voltage reading

// Board-specific settings
#define DISPLAY_TIMEOUT_MS 10000  // Timeout in milliseconds

// Font sizes for text hierarchy
// Adjust these based on screen size (larger screens use larger fonts)
#define FONT_HEADING1 6   // Large headings (e.g., "Dashboard", screen titles)
#define FONT_HEADING2 4   // Medium headings (e.g., section titles)
#define FONT_NORMAL 2     // Normal text (e.g., descriptions, status messages)

// Line spacing (pixels between lines of text)
// Adjust based on screen size and desired text density
#define LINE_SPACING 10

// Margins
#define MARGIN 10          // General margin (left, top)
#define INDENT_MARGIN 30   // Indentation for nested content

#endif // BOARD_CONFIG_H
```

**Important Notes:**
- Adjust `SCREEN_WIDTH` and `SCREEN_HEIGHT` to match your board's display resolution in landscape orientation
- `BOARD_ROTATION` is defined but not used - rotation is user-configurable via web portal
- For Inkplate 2, use `DISPLAY_MODE_INKPLATE2` instead of `DISPLAY_MODE`
- Scale `FONT_HEADING1`, `FONT_HEADING2`, and `FONT_NORMAL` appropriately for the screen size
- Smaller screens should use smaller fonts and tighter `LINE_SPACING`
- Set `MARGIN` to 0 for very small screens to maximize display space

**Font Scaling Guidelines:**
- **Large screens (10", 9.7")**: HEADING1=6-8, HEADING2=4-5, NORMAL=2-3
- **Medium screens (5"-6")**: HEADING1=6, HEADING2=4, NORMAL=2
- **Small screens (2"-3")**: HEADING1=1, HEADING2=1, NORMAL=1

### 3. Create Arduino Sketch File

Create `boards/inkplate-newboard/inkplate-newboard.ino`:

```cpp
// Board-specific sketch for Inkplate NewBoard
// This file serves as the entry point and includes board validation

#include "board_config.h"

// Validate board type at compile time
#if !defined(BOARD_TYPE) || BOARD_TYPE != INKPLATE_NEWBOARD
#error "This sketch is for Inkplate NewBoard only! Check board selection in Arduino IDE."
#endif

// Include the common main sketch implementation
#include <src/main_sketch.ino.inc>
```

**Note:** Replace `INKPLATE_NEWBOARD` with the appropriate board type constant. Check the Inkplate library for the correct constant name.

### 4. Update Build Scripts

#### 4.1 Update `build.sh` (Linux/macOS/GitHub Actions)

Add your board to the `BOARDS` associative array:

```bash
# Board configurations
declare -A BOARDS=(
    [inkplate2]="Inkplate 2|Inkplate_Boards:esp32:Inkplate2|boards/inkplate2"
    [inkplate5v2]="Inkplate 5 V2|Inkplate_Boards:esp32:Inkplate5V2|boards/inkplate5v2"
    [inkplate10]="Inkplate 10|Inkplate_Boards:esp32:Inkplate10|boards/inkplate10"
    [inkplate-newboard]="Inkplate NewBoard|Inkplate_Boards:esp32:InkplateNewBoard|boards/inkplate-newboard"
)
```

**Format:** `[board-key]="Board Name|FQBN|sketch-path"`
- `board-key`: Lowercase identifier used in build commands
- `Board Name`: Human-readable board name (for display purposes)
- `FQBN`: Fully Qualified Board Name from the Inkplate boards package
- `sketch-path`: Path to the board's sketch directory

#### 4.2 Update `build.ps1` (Windows)

Add your board to the `$boards` hashtable:

```powershell
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
    'inkplate-newboard' = @{
        Name = "Inkplate NewBoard"
        FQBN = "Inkplate_Boards:esp32:InkplateNewBoard"
        Path = "boards/inkplate-newboard"
    }
}
```

And update the `ValidateSet` parameter:

```powershell
param(
    [Parameter(Mandatory=$false)]
    [ValidateSet('inkplate2', 'inkplate5v2', 'inkplate10', 'inkplate-newboard', 'all')]
    [string]$Board = "inkplate5v2"
)
```

#### 4.3 Update `upload.ps1` (Windows)

Add your board to the `ValidateSet` parameter:

```powershell
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('inkplate2', 'inkplate5v2', 'inkplate10', 'inkplate-newboard')]
    [string]$Board,
    
    [Parameter(Mandatory=$false)]
    [string]$Port = ""
)
```

#### 4.4 Update `setup.ps1` (Windows)

Add your board to the required boards list:

```powershell
$REQUIRED_BOARDS = @("Inkplate2", "Inkplate5V2", "Inkplate10", "InkplateNewBoard")
```

### 5. Update GitHub Actions Workflows

#### 5.1 Update `.github/workflows/build.yml`

Add your board to the workflow dispatch options:

```yaml
workflow_dispatch:
  inputs:
    board:
      description: 'Board to build (inkplate2, inkplate5v2, inkplate10, inkplate-newboard, or all)'
      required: false
      default: 'all'
      type: choice
      options:
        - all
        - inkplate2
        - inkplate5v2
        - inkplate10
        - inkplate-newboard
```

And update the build matrix in the `prepare` job:

```yaml
- name: Set build matrix
  id: set-matrix
  run: |
    if [ "${{ github.event_name }}" = "workflow_dispatch" ]; then
      if [ "${{ github.event.inputs.board }}" = "all" ]; then
        echo 'matrix=["inkplate2","inkplate5v2","inkplate10","inkplate-newboard"]' >> $GITHUB_OUTPUT
      else
        echo 'matrix=["${{ github.event.inputs.board }}"]' >> $GITHUB_OUTPUT
      fi
    else
      echo 'matrix=["inkplate2","inkplate5v2","inkplate10","inkplate-newboard"]' >> $GITHUB_OUTPUT
    fi
```

#### 5.2 Update `.github/workflows/release.yml`

Add your board to the build matrix:

```yaml
strategy:
  matrix:
    board: [inkplate2, inkplate5v2, inkplate10, inkplate-newboard]
```

### 6. Update Documentation

#### 6.1 Update `README.md`

Add your board to the supported boards list:

```markdown
## Supported Boards

- ✅ Inkplate 2 (212x104, monochrome)
- ✅ Inkplate 5 V2 (1280x720, grayscale)
- ✅ Inkplate 10 (1200x825, grayscale)
- ✅ Inkplate 6 Flick (1024x758, grayscale)
```

Add build and upload examples for your board:

```bash
# Build firmware for Inkplate NewBoard
./build.ps1 inkplate-newboard

# Upload to Inkplate NewBoard
./upload.ps1 inkplate-newboard -port COM7
```

#### 6.2 Update `CHANGELOG.md`

Document the addition of the new board in the `[Unreleased]` section:

```markdown
## [Unreleased]

### Added
- Inkplate NewBoard support with dedicated configuration
```

## Testing Your Board

### 1. Build the Firmware

```bash
# Windows
.\build.ps1 inkplate-newboard

# Linux/macOS
./build.sh inkplate-newboard
```

### 2. Upload to Device

```bash
# Windows (replace COM7 with your port)
.\upload.ps1 inkplate-newboard -port COM7

# Linux/macOS (replace /dev/ttyUSB0 with your port)
arduino-cli upload -p /dev/ttyUSB0 --fqbn Inkplate_Boards:esp32:InkplateNewBoard boards/inkplate-newboard
```

### 3. Test All Modes

Test the following scenarios:
- ✅ **First boot** - Should enter AP mode automatically
- ✅ **WiFi configuration** - Should connect to configured WiFi
- ✅ **Image download and display** - Should download and display image
- ✅ **Config mode** (if HAS_BUTTON) - Hold button on boot to enter config mode
- ✅ **Text layout** - Verify no text overlap, proper margins, and readable fonts
- ✅ **Version label** - Should appear in bottom-right corner
- ✅ **Deep sleep and wake** - Should wake up after refresh interval
- ✅ **MQTT integration** (optional) - Should report to Home Assistant if configured

## Troubleshooting

### Text Overlap Issues

If you see text lines overlapping:
- Increase `LINE_SPACING` in `board_config.h`
- Reduce font sizes (`FONT_HEADING1`, `FONT_HEADING2`, `FONT_NORMAL`)

### Text Not at Top of Screen

If text doesn't start at the very top:
- Verify `MARGIN` is set correctly (0 for no margin, >0 for margin)
- Check that y-coordinate initialization starts at `MARGIN`, not `MARGIN + font height`

### Build Fails

- Verify the FQBN matches your board's definition in the Inkplate boards package
- Check that `BOARD_TYPE` constant exists in the Inkplate library
- Ensure all build scripts are updated with the correct board key

### Display Mode Issues

- **Inkplate 2**: Must use `DISPLAY_MODE_INKPLATE2` (no constructor parameter)
- **Other boards**: Use `DISPLAY_MODE INKPLATE_1BIT` or `DISPLAY_MODE INKPLATE_3BIT`

## Board-Specific Considerations

### Inkplate 2 Special Case

Inkplate 2 uses a different constructor than other boards:

```cpp
// In main_sketch.ino.inc
#ifdef DISPLAY_MODE_INKPLATE2
Inkplate display;  // No parameter
#else
Inkplate display(DISPLAY_MODE);  // With mode parameter
#endif
```

If your board requires a similar special case, add appropriate conditional compilation.

### Boards Without Physical Buttons

If your board has no physical button (like Inkplate 2):
- Set `HAS_BUTTON false` in `board_config.h`
- Users can enter config mode via reset button (ESP32 reset)
- Document this limitation in README.md

## Checklist

Before submitting your board addition:

- [ ] Created `boards/[board-name]/board_config.h` with correct settings
- [ ] Created `boards/[board-name]/[board-name].ino` sketch file
- [ ] Updated `build.sh` with board configuration
- [ ] Updated `build.ps1` with board configuration
- [ ] Updated `upload.ps1` with board option
- [ ] Updated `setup.ps1` with board requirement
- [ ] Updated `.github/workflows/build.yml` workflow
- [ ] Updated `.github/workflows/release.yml` workflow
- [ ] Updated `README.md` with board information
- [ ] Updated `CHANGELOG.md` with addition
- [ ] Tested build process locally
- [ ] Tested firmware on actual hardware
- [ ] Verified all display modes work correctly
- [ ] Confirmed text layout is readable and properly spaced

## Questions?

If you encounter issues or have questions about adding a new board, please:
1. Check existing board configurations for reference
2. Review the troubleshooting section above
3. Open an issue on GitHub with details about your board and the problem
