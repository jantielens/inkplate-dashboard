
# Architecture Decision Record: Adding Support for New Inkplate Boards

## Status
Accepted

## Context
The project must support new Inkplate board variants as they are released, while maintaining a shared codebase and minimizing duplication.

## Decision
Define a process for adding new boards: create a new board directory, add a board-specific configuration header, and update the build system to recognize the new board. Board-specific code is isolated, while shared logic remains in the common directory.

## Consequences
- New boards can be added with minimal changes to the shared codebase.
- The build system and configuration files must be updated for each new board.
- Ensures maintainability and scalability as the hardware lineup grows.

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
#define BOARD_TYPE INKPLATE_NEWBOARD  // Use appropriate constant from Inkplate library

// Display specifications
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BOARD_ROTATION 0  // 0, 1, 2, or 3 (rotation is user-configurable via web portal)

// Display mode
// For standard Inkplate boards (5, 6, 10): Use INKPLATE_1BIT or INKPLATE_3BIT
// For Inkplate 2: Use DISPLAY_MODE_INKPLATE2 (no parameter in constructor)
#define DISPLAY_MODE INKPLATE_3BIT

// Display performance optimization flags
#define DISPLAY_FAST_REFRESH true    // true = fast refresh (<5s), false = slow refresh (20+s)
#define DISPLAY_MINIMAL_UI false     // true = skip logos and reduce intermediate screens

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
- `BOARD_ROTATION` default value (0-3) is defined but rotation is user-configurable via web portal
- `BOARD_TYPE` should match the constant from the Inkplate library (e.g., `INKPLATE_2`, `INKPLATE_5V2`, `INKPLATE_10`, `INKPLATE_6FLICK`)
- For Inkplate 2, use `DISPLAY_MODE_INKPLATE2` instead of `DISPLAY_MODE`
- **Display Optimization Flags** (new in v1.3.2):
  - `DISPLAY_FAST_REFRESH`: Set to `false` for slow displays (20+ seconds refresh), `true` for fast displays
  - `DISPLAY_MINIMAL_UI`: Set to `true` to skip logos and intermediate screens on small/slow displays
  - Example: Inkplate 2 uses `DISPLAY_FAST_REFRESH=false` and `DISPLAY_MINIMAL_UI=true` for optimal UX
- Scale `FONT_HEADING1`, `FONT_HEADING2`, and `FONT_NORMAL` appropriately for the screen size
- Smaller screens should use smaller fonts and tighter `LINE_SPACING`
- Set `MARGIN` to 0 for very small screens to maximize display space

**Display Optimization Guidelines:**
- **Fast displays (5-10 second refresh)**: `DISPLAY_FAST_REFRESH=true`, `DISPLAY_MINIMAL_UI=false`
- **Slow displays (20+ second refresh)**: `DISPLAY_FAST_REFRESH=false`, `DISPLAY_MINIMAL_UI=true` 
- **Small screens (<300px width)**: Consider `DISPLAY_MINIMAL_UI=true` regardless of refresh speed

**Font Scaling Guidelines:**
- **Large screens (10", 9.7")**: HEADING1=6-8, HEADING2=4-5, NORMAL=2-3
- **Medium screens (5"-6")**: HEADING1=6, HEADING2=4, NORMAL=2
- **Small screens (2"-3")**: HEADING1=1, HEADING2=1, NORMAL=1

### 3. Create Arduino Sketch File

Create `boards/inkplate-newboard/inkplate-newboard.ino`:

```cpp
// Board-specific sketch for Inkplate NewBoard
// This file serves as the entry point and includes board validation

// Board validation - check Arduino board definition
#ifndef ARDUINO_INKPLATENEWBOARD
#error "Wrong board selection for this sketch, please select Soldered InkplateNewBoard in the boards menu."
#endif

// Include board configuration first (required by shared code)
#include "board_config.h"

// Include shared implementation
#include <src/main_sketch.ino.inc>
```

**Note:** Replace `ARDUINO_INKPLATENEWBOARD` with the appropriate Arduino board constant. These follow the pattern `ARDUINO_INKPLATE<NAME>` (e.g., `ARDUINO_INKPLATE2`, `ARDUINO_INKPLATE5V2`, `ARDUINO_INKPLATE10`, `ARDUINO_INKPLATE6FLICK`).

### 4. Update Build Scripts

#### 4.1 Update `build.sh` (Linux/macOS/GitHub Actions)

Add your board to the `BOARDS` associative array:

```bash
# Board configurations
declare -A BOARDS=(
    [inkplate2]="Inkplate 2|Inkplate_Boards:esp32:Inkplate2|boards/inkplate2"
    [inkplate5v2]="Inkplate 5 V2|Inkplate_Boards:esp32:Inkplate5V2|boards/inkplate5v2"
    [inkplate10]="Inkplate 10|Inkplate_Boards:esp32:Inkplate10|boards/inkplate10"
    [inkplate6flick]="Inkplate 6 Flick|Inkplate_Boards:esp32:Inkplate6Flick|boards/inkplate6flick"
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
    'inkplate6flick' = @{
        Name = "Inkplate 6 Flick"
        FQBN = "Inkplate_Boards:esp32:Inkplate6Flick"
        Path = "boards/inkplate6flick"
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
    [ValidateSet('inkplate2', 'inkplate5v2', 'inkplate10', 'inkplate6flick', 'inkplate-newboard', 'all')]
    [string]$Board = "inkplate5v2"
)
```

#### 4.3 Update `upload.ps1` (Windows)

Add your board to the `ValidateSet` parameter:

```powershell
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('inkplate2', 'inkplate5v2', 'inkplate10', 'inkplate6flick', 'inkplate-newboard')]
    [string]$Board,
    
    [Parameter(Mandatory=$false)]
    [string]$Port = ""
)
```

#### 4.4 Update `setup.ps1` (Windows)

Add your board to the required boards list:

```powershell
$REQUIRED_BOARDS = @("Inkplate2", "Inkplate5V2", "Inkplate10", "Inkplate6Flick", "InkplateNewBoard")
```

### 5. Update GitHub Actions Workflows

#### 5.1 Update `.github/workflows/build.yml`

Add your board to the workflow dispatch options:

```yaml
workflow_dispatch:
  inputs:
    board:
      description: 'Board to build (inkplate2, inkplate5v2, inkplate10, inkplate6flick, inkplate-newboard, or all)'
      required: false
      default: 'all'
      type: choice
      options:
        - all
        - inkplate2
        - inkplate5v2
        - inkplate10
        - inkplate6flick
        - inkplate-newboard
```

And update the build matrix in the `prepare` job:

```yaml
- name: Set build matrix
  id: set-matrix
  run: |
    if [ "${{ github.event_name }}" = "workflow_dispatch" ]; then
      if [ "${{ github.event.inputs.board }}" = "all" ]; then
        echo 'matrix=["inkplate2","inkplate5v2","inkplate10","inkplate6flick","inkplate-newboard"]' >> $GITHUB_OUTPUT
      else
        echo 'matrix=["${{ github.event.inputs.board }}"]' >> $GITHUB_OUTPUT
      fi
    else
      echo 'matrix=["inkplate2","inkplate5v2","inkplate10","inkplate6flick","inkplate-newboard"]' >> $GITHUB_OUTPUT
    fi
```

#### 5.2 Update `.github/workflows/release.yml`

Add your board to the build matrix:

```yaml
strategy:
  matrix:
    board: [inkplate2, inkplate5v2, inkplate10, inkplate6flick, inkplate-newboard]
```

### 6. Update Web Flasher

#### 6.1 Update `scripts/generate_manifests.sh`

Add your board's display name to the script (used for manifest generation):

```bash
# Friendly display names for boards
declare -A NAMES
NAMES[inkplate2]="Inkplate 2"
NAMES[inkplate5v2]="Inkplate 5 V2"
NAMES[inkplate10]="Inkplate 10"
NAMES[inkplate6flick]="Inkplate 6 Flick"
NAMES[inkplate-newboard]="Inkplate NewBoard"
```

**Note:** The script automatically processes all board binaries in the artifacts directory during release builds, so this is the only change needed for manifest generation.

#### 6.2 Update `flasher/index.html`

Add your board as an option in the HTML form. Find the board selection section and add:

```html
<div class="board-option">
  <input type="radio" name="board" id="inkplate-newboard" value="inkplate-newboard" />
  <label for="inkplate-newboard">
    <div class="board-icon">📱</div>
    <div class="board-name">Inkplate NewBoard</div>
    <div class="board-specs">1280×720, grayscale</div>
  </label>
</div>
```

Replace the icon, name, and specs as appropriate. Common icons used:
- 📱 for medium/small boards
- 🖥️ for large boards
- 📟 for very small boards

**Important:** The `value` attribute must match the board key used in build scripts (e.g., `inkplate-newboard`).

### 7. Update Documentation

#### 7.1 Update `README.md`

Add your board to the supported boards list:

```markdown
## Supported Devices

- **Inkplate 2**
- **Inkplate 5 V2**
- **Inkplate 6 Flick**
- **Inkplate 10**
- **Inkplate NewBoard** *(example - your new board)*
```

Add build and upload examples for your board:

```bash
# Build firmware for Inkplate NewBoard
./build.ps1 inkplate-newboard

# Upload to Inkplate NewBoard
./upload.ps1 inkplate-newboard -port COM7
```

#### 7.2 Update `CHANGELOG.md`

Document the addition of the new board in the `[Unreleased]` section:

```markdown
## [Unreleased]

### Added
- Inkplate NewBoard support with dedicated configuration
```

## Web Flasher Integration

The web flasher automatically supports any board that's included in the release workflow. Here's how it works:

1. **During Release Build**: When you create a release tag (e.g., `v1.2.3`), the GitHub Actions release workflow:
   - Builds firmware for all boards in the matrix
   - Generates bootloader and partition binaries for each board
   - Runs `scripts/generate_manifests.sh` to create ESP Web Tools manifest files
   - Publishes all files to the `gh-pages` branch

2. **Manifest Files**: The script creates `manifest_<board>.json` files (e.g., `manifest_inkplate-newboard.json`) with:
   - Board-specific firmware URLs
   - Bootloader and partition table URLs
   - ESP32 memory offsets (0x1000, 0x8000, 0x10000)
   - Board name and version information

3. **Web UI**: The `flasher/index.html` page:
   - Displays available board options
   - Loads the appropriate manifest when a user selects a board
   - Uses ESP Web Tools to flash all three parts to the device

**No additional configuration is needed** beyond the steps in sections 6.1 and 6.2 above. The build and release automation handles everything else.

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
- [ ] Updated `scripts/generate_manifests.sh` with board display name
- [ ] Updated `flasher/index.html` with board option (icon, name, specs)
- [ ] Updated `README.md` with board information
- [ ] Updated `CHANGELOG.md` with addition
- [ ] Tested build process locally
- [ ] Tested firmware on actual hardware
- [ ] Verified all display modes work correctly
- [ ] Confirmed text layout is readable and properly spaced
- [ ] Verified web flasher shows new board option (after release)

## Questions?

If you encounter issues or have questions about adding a new board, please:
1. Check existing board configurations for reference
2. Review the troubleshooting section above
3. Open an issue on GitHub with details about your board and the problem
