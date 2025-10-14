# Build System Explained with Examples

This document walks through actual examples of what happens during the build process.

## Example: Building Inkplate 5 V2

### Before Build

**Project Structure:**
```
inkplate-dashboard-new/
├── boards/
│   └── inkplate5v2/
│       ├── inkplate5v2.ino      ✓ Exists
│       └── board_config.h        ✓ Exists
│
└── common/
    └── src/
        ├── main_sketch.ino.inc   ✓ Exists
        ├── config_manager.h      ✓ Exists
        ├── config_manager.cpp    ✓ Exists
        ├── wifi_manager.h        ✓ Exists
        ├── wifi_manager.cpp      ✓ Exists
        ├── config_portal.h       ✓ Exists
        ├── config_portal.cpp     ✓ Exists
        ├── display_manager.h     ✓ Exists
        ├── display_manager.cpp   ✓ Exists
        ├── utils.h               ✓ Exists
        ├── utils.cpp             ✓ Exists
        └── ...future files...    (image_downloader.cpp, png_decoder.cpp, etc.)
```

**Note:** As features are implemented, more `.cpp` files will be added to `common/src/`.
The build system automatically handles all `.cpp` files in this directory.

### Step 1: Copy Phase

**Command:** `.\build.ps1 inkplate5v2`

**What Happens:**
```powershell
# Script copies ALL .cpp files from common/src/:
common/src/config_manager.cpp    → boards/inkplate5v2/config_manager.cpp
common/src/wifi_manager.cpp      → boards/inkplate5v2/wifi_manager.cpp
common/src/config_portal.cpp     → boards/inkplate5v2/config_portal.cpp
common/src/display_manager.cpp   → boards/inkplate5v2/display_manager.cpp
common/src/utils.cpp             → boards/inkplate5v2/utils.cpp
# ...and any future .cpp files automatically
```

**Console Output:**
```
Copied config_manager.cpp to sketch directory
Copied config_portal.cpp to sketch directory
Copied display_manager.cpp to sketch directory
Copied utils.cpp to sketch directory
Copied wifi_manager.cpp to sketch directory
# ...future files will appear here as they're added
```
Copied wifi_manager.cpp to sketch directory
```

**Temporary State:**
```
boards/inkplate5v2/
├── inkplate5v2.ino          ✓ Original
├── board_config.h            ✓ Original
├── config_manager.cpp        ← COPIED (temporary)
├── config_portal.cpp         ← COPIED (temporary)
├── display_manager.cpp       ← COPIED (temporary)
├── utils.cpp                 ← COPIED (temporary)
└── wifi_manager.cpp          ← COPIED (temporary)
```

### Step 2: Compilation Phase

**Arduino CLI Command:**
```powershell
arduino-cli compile `
    --fqbn Inkplate_Boards:esp32:Inkplate5V2 `
    --build-path build/inkplate5v2 `
    --library common `
    --build-property "compiler.cpp.extra_flags=-I\"C:\dev\inkplate-dashboard-new\common\" -I\"C:\dev\inkplate-dashboard-new\common\src\"" `
    boards/inkplate5v2
```

**What Arduino CLI Does:**

1. **Processes .ino file:**
   ```cpp
   // inkplate5v2.ino gets converted to inkplate5v2.ino.cpp
   #include <Arduino.h>  // Auto-added
   
   // Original content:
   #include "board_config.h"
   #include <src/main_sketch.ino.inc>
   ```

2. **Resolves includes:**
   ```
   board_config.h           → boards/inkplate5v2/board_config.h
   src/main_sketch.ino.inc  → common/src/main_sketch.ino.inc (via -I flag)
   src/config_manager.h     → common/src/config_manager.h (via -I flag)
   src/wifi_manager.h       → common/src/wifi_manager.h (via -I flag)
   etc.
   ```

3. **Compiles source files:**
   ```
   ✓ inkplate5v2.ino.cpp        (generated from .ino)
   ✓ config_manager.cpp         (copied to sketch dir)
   ✓ wifi_manager.cpp           (copied to sketch dir)
   ✓ config_portal.cpp          (copied to sketch dir)
   ✓ display_manager.cpp        (copied to sketch dir)
   ✓ utils.cpp                  (copied to sketch dir)
   ✓ ...all other .cpp files    (from common/src/)
   ✓ InkplateLibrary sources    (from installed library)
   ✓ ESP32 core sources         (from Arduino core)
   ```
   
   **Note:** Any new `.cpp` files added to `common/src/` are automatically compiled.

4. **Links into binary:**
   ```
   All .o files → inkplate5v2.ino.elf → inkplate5v2.ino.bin
   ```

**Console Output:**
```
Compiling boards/inkplate5v2...
Including common libraries from: C:\dev\inkplate-dashboard-new\common

Sketch uses 234567 bytes (17%) of program storage space.
Global variables use 12345 bytes (3%) of dynamic memory.

Build successful!
```

### Step 3: Cleanup Phase

**What Happens:**
```powershell
# Script removes copied files:
Remove-Item boards/inkplate5v2/config_manager.cpp
Remove-Item boards/inkplate5v2/wifi_manager.cpp
Remove-Item boards/inkplate5v2/config_portal.cpp
Remove-Item boards/inkplate5v2/display_manager.cpp
Remove-Item boards/inkplate5v2/utils.cpp
```

**After Cleanup:**
```
boards/inkplate5v2/
├── inkplate5v2.ino          ✓ Original (unchanged)
└── board_config.h            ✓ Original (unchanged)
```

**Build Output Created:**
```
build/inkplate5v2/
├── inkplate5v2.ino.bin      ← Flash this!
├── inkplate5v2.ino.elf      ← Debugging symbols
├── inkplate5v2.ino.map      ← Memory map
├── compile_commands.json    ← IDE integration
├── core/                    ← Compiled core files
└── libraries/               ← Compiled library files
```

## Example: What Gets Compiled

### Source File: inkplate5v2.ino

**Original:**
```cpp
#ifndef ARDUINO_INKPLATE5V2
#error "Wrong board selection"
#endif

#include "board_config.h"
#include <src/main_sketch.ino.inc>
```

**After Arduino Preprocessor:**
```cpp
#include <Arduino.h>  // Auto-added by Arduino

#ifndef ARDUINO_INKPLATE5V2
#error "Wrong board selection"
#endif

// Content of board_config.h expanded here:
#define BOARD_NAME "Inkplate 5 V2"
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define DISPLAY_MODE INKPLATE_3BIT
// ... etc

// Content of main_sketch.ino.inc expanded here:
#include "Inkplate.h"
#include <src/config_manager.h>
#include <src/wifi_manager.h>
// ... more includes

Inkplate display(DISPLAY_MODE);  // Uses INKPLATE_3BIT from config
ConfigManager configManager;
WiFiManager wifiManager(&configManager);
// ... more globals

void setup() {
    Serial.println("Starting " + String(BOARD_NAME));  // Uses "Inkplate 5 V2"
    // ... setup code uses SCREEN_WIDTH, SCREEN_HEIGHT, etc.
}

void loop() {
    // ... loop code
}
```

### Compilation Units

When building, these separate files are compiled:

**1. inkplate5v2.ino.cpp** (generated)
```cpp
// Everything from above preprocessed file
```

**2. config_manager.cpp**
```cpp
#include "config_manager.h"
// ... implementation
```

**3. wifi_manager.cpp**
```cpp
#include "wifi_manager.h"
// ... implementation
```

**4. config_portal.cpp**
```cpp
#include "config_portal.h"
// ... implementation
```

**5. display_manager.cpp**
```cpp
#include "display_manager.h"
// ... implementation
```

**6. utils.cpp**
```cpp
#include "utils.h"
// ... implementation
```

Each compiled separately into `.o` files, then linked together.

## Example: Include Path Resolution

### Scenario: Compiler sees `#include <src/config_manager.h>`

**Search Process:**

1. **Check sketch directory:**
   ```
   boards/inkplate5v2/src/config_manager.h  ✗ Not found
   ```

2. **Check library paths (--library common):**
   ```
   common/src/config_manager.h  ✓ Found!
   ```

3. **Use this file**

### With Custom Include Flags

The build property adds these paths:
```
-I"C:\dev\inkplate-dashboard-new\common"
-I"C:\dev\inkplate-dashboard-new\common\src"
```

So these all work:
```cpp
#include <src/config_manager.h>      // common/src/config_manager.h
#include <config_manager.h>          // common/src/config_manager.h
#include <library.properties>        // common/library.properties
```

## Example: Why Headers Don't Need Copying

**Headers are INCLUDED, not COMPILED:**

```cpp
// In inkplate5v2.ino.cpp (compilation unit):
#include <src/config_manager.h>

// Preprocessor reads config_manager.h and pastes its content here
// No separate compilation needed for .h files
```

**Implementation files are COMPILED:**

```cpp
// config_manager.cpp needs to be compiled separately
// That's why we copy it to sketch directory
```

## Example: Build All Boards

**Command:** `.\build.ps1 all`

**Process:**
```
For each board in (inkplate5v2, inkplate10):
    1. Copy .cpp files to that board's directory
    2. Compile that board
    3. Clean up copied files
    4. Move to next board
```

**Timeline:**
```
[inkplate5v2] Copy files...
[inkplate5v2] Compiling...
[inkplate5v2] Build successful!
[inkplate5v2] Cleanup...

[inkplate10] Copy files...
[inkplate10] Compiling...
[inkplate10] Build successful!
[inkplate10] Cleanup...

All builds completed successfully!
```

**Output:**
```
build/
├── inkplate5v2/
│   └── inkplate5v2.ino.bin  (1.2 MB)
└── inkplate10/
    └── inkplate10.ino.bin   (1.2 MB)
```

## Example: What Happens Without File Copying

**If we DIDN'T copy .cpp files:**

```powershell
arduino-cli compile --library common boards/inkplate5v2
```

**Result:**
```
✓ inkplate5v2.ino compiled
✓ Headers found and included
✗ config_manager.cpp NOT compiled (not in sketch directory)
✗ wifi_manager.cpp NOT compiled (not in sketch directory)
✗ config_portal.cpp NOT compiled (not in sketch directory)
✗ display_manager.cpp NOT compiled (not in sketch directory)
✗ utils.cpp NOT compiled (not in sketch directory)
✗ ...any other .cpp files NOT compiled

Linker error: Undefined reference to ConfigManager::begin()
Linker error: Undefined reference to WiFiManager::startAccessPoint()
... etc
```

**Why?**
- Arduino CLI's `--library` flag makes **headers** discoverable
- But it doesn't automatically compile **implementation files** from those libraries
- That's an Arduino CLI limitation/design choice

**Our Solution:**
- Copy **all** `.cpp` files from `common/src/` to sketch directory
- Arduino's build system **always** compiles `.cpp` files in sketch directory
- After build, remove copied files to keep source clean
- This automatically handles any future `.cpp` files added to the project

## Example: Memory Usage

**Typical Build Output:**
```
Sketch uses 856,432 bytes (65%) of program storage space. Maximum is 1,310,720 bytes.
Global variables use 42,568 bytes (12%) of dynamic memory, leaving 285,112 bytes for local variables. Maximum is 327,680 bytes.
```

**What This Means:**
- **Program Storage (Flash):** 856 KB used out of 1.3 MB available
  - Your code, libraries, and constants
  - Stored permanently in flash

- **Dynamic Memory (RAM):** 42 KB used out of 327 KB available
  - Global variables, stack, heap
  - Used during runtime

**Memory Map Example (.map file):**
```
.text section (code):
    setup()                    1,234 bytes
    loop()                     567 bytes
    WiFiManager::start()       2,345 bytes
    ConfigPortal::handle()     3,456 bytes
    ...

.data section (initialized globals):
    display                    256 bytes
    configManager             128 bytes
    ...

.bss section (uninitialized globals):
    buffers                    4,096 bytes
    ...
```

## Summary

The build system:
1. **Temporarily copies** `.cpp` files to work around Arduino CLI limitation
2. **Uses include flags** to make headers discoverable
3. **Compiles everything** into a single firmware binary
4. **Cleans up** to keep source directories pristine

This allows us to maintain clean, shared code while working within Arduino's build constraints.
