# Build System Documentation

## Overview

This project uses a custom PowerShell-based build system that compiles Arduino sketches for multiple Inkplate boards while sharing common code between them. This document explains the architecture, workflow, and design decisions.

## Quick Reference

```powershell
# Build specific board
.\build.ps1 inkplate5v2
.\build.ps1 inkplate10

# Build all boards
.\build.ps1 all

# Upload to device
.\upload.ps1 -board inkplate5v2 -port COM7
```

## Project Structure

```
inkplate-dashboard-new/
├── boards/                    # Board-specific sketches
│   ├── inkplate5v2/
│   │   ├── inkplate5v2.ino   # Minimal board-specific entry point
│   │   └── board_config.h     # Board-specific constants
│   └── inkplate10/
│       ├── inkplate10.ino     # Minimal board-specific entry point
│       └── board_config.h     # Board-specific constants
│
├── common/                    # Shared code library
│   ├── library.properties     # Arduino library metadata
│   ├── src/
│   │   ├── main_sketch.ino.inc      # Shared setup() and loop()
│   │   ├── config_manager.h/cpp     # Configuration storage
│   │   ├── wifi_manager.h/cpp       # WiFi management
│   │   ├── config_portal.h/cpp      # Web configuration portal
│   │   ├── display_manager.h/cpp    # Display abstraction
│   │   ├── utils.h/cpp              # Utility functions
│   │   └── ...more features...      # Future: image_downloader.h/cpp,
│   │                                #         png_decoder.h/cpp,
│   │                                #         power_manager.h/cpp, etc.
│   └── examples/              # Documentation and examples
│
├── build/                     # Build artifacts (generated)
│   ├── inkplate5v2/          # Inkplate 5 V2 build output
│   └── inkplate10/           # Inkplate 10 build output
│
├── build.ps1                  # Main build script
├── upload.ps1                 # Upload script
└── setup.ps1                  # Environment setup
```

## Architecture Rationale

### Why This Structure?

The project supports **multiple hardware boards** (Inkplate 5 V2 and Inkplate 10) that share **95% of the same code** but differ in:
- Screen resolution (1280x720 vs 1200x825)
- Display mode (3-bit vs 1-bit grayscale)
- Hardware features (touchscreen availability)

**Design Goals:**
1. ✅ **DRY Principle**: Write shared code once
2. ✅ **Board-Specific Customization**: Easy to configure per-board settings
3. ✅ **Maintainability**: Single place to update core logic
4. ✅ **Arduino CLI Compatibility**: Works with standard tooling

### Why Not Standard Arduino Library?

**Standard Arduino Library Pattern:**
```
MyLibrary/
├── library.properties
├── src/
│   ├── MyLibrary.h
│   └── MyLibrary.cpp
└── examples/
    └── BasicExample/
        └── BasicExample.ino  # Separate sketch with setup()/loop()
```

**Problem:** This pattern expects:
- Libraries provide **classes/functions**
- Sketches provide **setup() and loop()**
- You can't share `setup()` and `loop()` code easily

**Our Solution:**
- Use `.ino.inc` include file for shared `setup()` and `loop()`
- Board-specific `.ino` files just include this shared implementation
- Common code acts as both a library AND shared sketch code

## Build Process Deep Dive

### Step-by-Step Workflow

When you run `.\build.ps1 inkplate5v2`:

#### 1. **Initialize Paths**
```powershell
$WORKSPACE_PATH = (Get-Location).Path
$COMMON_PATH = Join-Path $WORKSPACE_PATH "common"
$SKETCH_PATH = "boards/inkplate5v2"
$BUILD_DIR = "build/inkplate5v2"
```

#### 2. **Copy `.cpp` Files to Sketch Directory**
```powershell
# Copy: common/src/*.cpp → boards/inkplate5v2/*.cpp
$commonSrcFiles = Get-ChildItem -Path (Join-Path $COMMON_PATH "src") -Filter "*.cpp"
foreach ($file in $commonSrcFiles) {
    Copy-Item -Path $file.FullName -Destination $SKETCH_PATH
}
```

**Files Copied:**
- `config_manager.cpp`
- `config_portal.cpp`
- `display_manager.cpp`
- `utils.cpp`
- `wifi_manager.cpp`
- ...and any other `.cpp` files in `common/src/`

**Note:** The build script automatically copies **all** `.cpp` files from `common/src/`, so as new features are added (like `image_downloader.cpp`, `png_decoder.cpp`, etc.), they will be automatically included in the build without modifying the build script.

**Why Copy?**
- Arduino CLI doesn't automatically compile `.cpp` files from libraries referenced with `--library`
- Copying ensures Arduino's build system sees and compiles them
- This is a **build-time workaround** for Arduino CLI's limitations

#### 3. **Compile with Arduino CLI**
```powershell
arduino-cli compile \
    --fqbn Inkplate_Boards:esp32:Inkplate5V2 \
    --build-path build/inkplate5v2 \
    --library common \
    --build-property "compiler.cpp.extra_flags=-I\"$COMMON_PATH\" -I\"$COMMON_PATH\src\"" \
    boards/inkplate5v2
```

**Arguments Explained:**

- **`--fqbn`** (Fully Qualified Board Name)
  - Specifies target hardware
  - Format: `{package}:{architecture}:{board}`
  - Examples:
    - `Inkplate_Boards:esp32:Inkplate5V2`
    - `Inkplate_Boards:esp32:Inkplate10`

- **`--build-path`**
  - Where to place compiled binaries
  - Separate folder per board prevents conflicts

- **`--library`**
  - Tells Arduino CLI where to find libraries
  - Makes `common/` discoverable as a library
  - Allows `#include <src/config_manager.h>` to work

- **`--build-property`**
  - Adds custom compiler flags
  - `-I"$COMMON_PATH"` - Include path to common/
  - `-I"$COMMON_PATH\src"` - Include path to common/src/
  - Allows both `#include <src/file.h>` and `#include "file.h"` patterns

- **Last Argument** (`boards/inkplate5v2`)
  - Path to the sketch to compile
  - Contains the `.ino` file

#### 4. **Compilation Happens**

Arduino CLI:
1. Reads `inkplate5v2.ino`
2. Sees `#include "board_config.h"` → loads board-specific config
3. Sees `#include <src/main_sketch.ino.inc>` → loads shared setup()/loop()
4. Finds copied `.cpp` files in sketch directory → compiles them
5. Finds `.h` files via include paths → resolves dependencies
6. Links everything into firmware binary

**What Gets Compiled:**
- `inkplate5v2.ino` (processed into `.cpp` by Arduino)
- `config_manager.cpp` (copied from common/src/)
- `config_portal.cpp` (copied from common/src/)
- `display_manager.cpp` (copied from common/src/)
- `utils.cpp` (copied from common/src/)
- `wifi_manager.cpp` (copied from common/src/)
- ...plus any future `.cpp` files added to `common/src/`
- All InkplateLibrary source files
- ESP32 Arduino core files

#### 5. **Cleanup Copied Files**
```powershell
foreach ($file in $commonSrcFiles) {
    Remove-Item -Path (Join-Path $SKETCH_PATH $file.Name) -Force
}
```

**Why Cleanup?**
- Keeps source directories clean
- Prevents confusion (single source of truth in `common/`)
- Only temporary files for build process

#### 6. **Copy Versioned Binaries**

For web flasher support, the build script creates versioned copies of all binaries:

```bash
# Copy and rename all binaries to include version
VERSIONED_BIN="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.bin"
VERSIONED_BOOTLOADER="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.bootloader.bin"
VERSIONED_PARTITIONS="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.partitions.bin"

cp "$ORIGINAL_BIN" "$VERSIONED_BIN"
cp "$ORIGINAL_BOOTLOADER" "$VERSIONED_BOOTLOADER"
cp "$ORIGINAL_PARTITIONS" "$VERSIONED_PARTITIONS"
```

**Why three binaries?**
- **Bootloader** - ESP32 first-stage bootloader (flashed at 0x1000)
- **Partitions** - Partition table defining memory layout (flashed at 0x8000)
- **Firmware** - Main application code (flashed at 0x10000)

This matches standard ESP32 flashing with `arduino-cli upload` or `esptool.py` and enables web-based flashing through ESP Web Tools.

See [WEB_FLASHER.md](WEB_FLASHER.md) for details on the web flasher implementation.

#### 7. **Output Build Artifacts**
```
build/inkplate5v2/
├── inkplate5v2.ino.bin                    # Main firmware binary
├── inkplate5v2.ino.bootloader.bin         # Bootloader
├── inkplate5v2.ino.partitions.bin         # Partition table
├── inkplate5v2-v0.13.0.bin                # Versioned firmware (for releases)
├── inkplate5v2-v0.13.0.bootloader.bin     # Versioned bootloader (for releases)
├── inkplate5v2-v0.13.0.partitions.bin     # Versioned partitions (for releases)
├── inkplate5v2.ino.elf                    # Debugging symbols
├── inkplate5v2.ino.map                    # Memory map
└── partitions.csv                         # Partition table source
```

## How Code Sharing Works

### Board-Specific Entry Point

**`boards/inkplate5v2/inkplate5v2.ino`:**
```cpp
// Board validation - ensures correct board selected in Arduino
#ifndef ARDUINO_INKPLATE5V2
#error "Wrong board selection, please select Soldered Inkplate5V2"
#endif

// Include board configuration FIRST
#include "board_config.h"

// Include shared implementation (contains setup() and loop())
#include <src/main_sketch.ino.inc>
```

**Key Points:**
- Only 10 lines of code
- Board validation prevents compilation errors
- Loads board-specific config before shared code
- Includes shared `setup()` and `loop()`

### Board Configuration

**`boards/inkplate5v2/board_config.h`:**
```cpp
#define BOARD_NAME "Inkplate 5 V2"
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define DISPLAY_MODE INKPLATE_3BIT
#define HAS_TOUCHSCREEN false
```

These constants are used by shared code to adapt behavior.

### Shared Implementation

**`common/src/main_sketch.ino.inc`:**
```cpp
#include "Inkplate.h"
#include <src/config_manager.h>
#include <src/wifi_manager.h>
// ... more includes

// Uses BOARD_NAME, SCREEN_WIDTH, etc. from board_config.h
Inkplate display(DISPLAY_MODE);
ConfigManager configManager;

void setup() {
    Serial.println("Starting " + String(BOARD_NAME));
    // ... shared setup code
}

void loop() {
    // ... shared loop code
}
```

**Why `.ino.inc` Extension?**
- Not a standard `.ino` file (Arduino would try to compile it separately)
- Not a `.h` header (contains implementation, not declarations)
- `.inc` signals "include file" - common C/C++ convention
- Prevents Arduino IDE from auto-detecting it as a sketch

## Include Path Resolution

When code includes headers, Arduino CLI searches in this order:

```cpp
#include <src/config_manager.h>  // Search library paths
```

**Search Path:**
1. Sketch directory (`boards/inkplate5v2/`)
2. Libraries specified with `--library` (`common/`)
3. Installed Arduino libraries
4. Core libraries (ESP32)

**Result:** Finds `common/src/config_manager.h`

## Multi-Board Build (`all` Parameter)

```powershell
.\build.ps1 all
```

**Process:**
1. Loops through all boards in `$boards` hashtable
2. Calls `Build-Board` function for each
3. Tracks success/failure for each build
4. Reports final status

**Advantages:**
- Build all boards in one command
- Catch board-specific compilation errors early
- CI/CD friendly

## Build Artifacts

### Directory Structure
```
build/
├── inkplate5v2/
│   ├── inkplate5v2.ino.bin    # Flash this to device
│   ├── inkplate5v2.ino.elf    # For debugging
│   ├── inkplate5v2.ino.map    # Memory layout
│   ├── compile_commands.json  # For IDE integration
│   └── core/                  # Compiled core files
└── inkplate10/
    └── (same structure)
```

### What Each File Is

**`.bin` (Binary)**
- Final firmware ready to flash
- Use with `upload.ps1` or esptool
- This is what runs on the device

**`.elf` (Executable and Linkable Format)**
- Contains debugging symbols
- Used with debuggers (GDB)
- Maps binary addresses to source code

**`.map` (Memory Map)**
- Shows memory usage
- Lists function sizes
- Helpful for optimization

**`compile_commands.json`**
- LSP/IntelliSense integration
- Used by clangd, ccls
- Enables IDE code completion

## Upload Process

**`upload.ps1`:**
```powershell
.\upload.ps1 -board inkplate5v2 -port COM7
```

**Workflow:**
1. Validates board parameter
2. Finds `.bin` file in `build/{board}/`
3. Calls `arduino-cli upload` with:
   - FQBN (board identifier)
   - Port (COM7, /dev/ttyUSB0, etc.)
   - Sketch path

**Under the Hood:**
- Arduino CLI uses **esptool.py**
- Uploads firmware to ESP32 flash
- Verifies upload succeeded

## Common Issues & Solutions

### Issue: "Wrong board selection" Error

**Cause:** Wrong board selected in Arduino IDE/CLI

**Solution:** 
- For Inkplate 5 V2: Select "Soldered Inkplate5V2"
- For Inkplate 10: Select "Soldered Inkplate10"
- Or use the build script (handles this automatically)

### Issue: Include Errors

**Symptom:** `fatal error: src/config_manager.h: No such file or directory`

**Cause:** Include paths not set correctly

**Solution:** Build script handles this with `--build-property` flags

### Issue: Duplicate Symbol Errors

**Symptom:** `multiple definition of 'setup'`

**Cause:** `.ino.inc` file compiled as separate sketch

**Solution:** Use `.inc` extension (not `.ino`) to prevent auto-compilation

### Issue: Library Not Found

**Symptom:** `InkplateLibrary.h: No such file or directory`

**Cause:** Inkplate boards not installed in Arduino CLI

**Solution:** Run `setup.ps1` to install board packages

## Comparison: Standard vs Our Approach

### Standard Arduino Multi-Board Project

```
Project/
├── Board1/
│   └── sketch.ino          # 200 lines (duplicated code)
├── Board2/
│   └── sketch.ino          # 200 lines (duplicated code)
└── SharedLib/
    └── src/
        ├── Library.h       # Only helper functions
        └── Library.cpp
```

**Pros:** Simple, follows Arduino conventions  
**Cons:** Code duplication, hard to maintain

### Our Approach

```
Project/
├── boards/
│   ├── Board1/
│   │   ├── sketch.ino      # 10 lines (just includes)
│   │   └── config.h        # Board-specific constants
│   └── Board2/
│       ├── sketch.ino      # 10 lines (just includes)
│       └── config.h
└── common/
    └── src/
        ├── main_sketch.ino.inc  # 200 lines (shared)
        └── (libraries)
```

**Pros:** No duplication, easy to maintain, single source of truth  
**Cons:** Non-standard, requires build script understanding

## Future Enhancements

### Possible Improvements

1. **PlatformIO Support**
   - Add `platformio.ini`
   - Use PlatformIO's native multi-env support
   - Would eliminate need for custom build script

2. **CMake Build**
   - ESP-IDF style build
   - More control over compilation
   - Better IDE integration

3. **GitHub Actions CI**
   - Automated builds on push
   - Test both boards
   - Generate release artifacts

4. **Dependency Management**
   - Version-lock libraries
   - Use `library_dependencies` in `library.properties`

## Best Practices

### When Adding New Boards

1. Create `boards/newboard/` directory
2. Add `newboard.ino` with validation and includes
3. Add `board_config.h` with constants
4. Update `$boards` hashtable in `build.ps1`
5. Test build with `.\build.ps1 newboard`

### When Adding New Features

1. Add code to `common/src/` (shared)
2. Use board-specific constants from `board_config.h`
3. Test on all boards: `.\build.ps1 all`
4. Update documentation

### Code Organization Rules

**Put in `common/src/`:**
- Code shared by all boards
- Classes, functions, utilities
- Main application logic

**Put in `boards/{board}/`:**
- Only `{board}.ino` entry point
- Only `board_config.h` constants
- Nothing else!

## Summary

The build system uses a **hybrid library + shared sketch pattern** to maximize code reuse across multiple hardware variants while working within Arduino CLI's constraints.

**Key Concepts:**
1. Board-specific `.ino` files are minimal (just validation + includes)
2. Shared `setup()` and `loop()` live in `.ino.inc` include file
3. `.cpp` files temporarily copied to sketch directory for compilation
4. Include paths set via compiler flags
5. Each board gets isolated build directory

This approach prioritizes **maintainability** and **DRY principles** over strict adherence to Arduino conventions, with the trade-off of requiring custom build scripts and documentation.
