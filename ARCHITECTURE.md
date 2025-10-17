# Architecture Documentation

## Overview

This project implements a dashboard application for multiple Inkplate e-ink display boards. The architecture uses a multi-board design where common code is shared across all board variants, while board-specific configurations are isolated to individual board directories.

## Multi-Board Approach

### Supported Boards

The project supports the following Inkplate boards:
- **Inkplate 2** - 2.13" monochrome display
- **Inkplate 5 V2** - 5.2" monochrome display  
- **Inkplate 6 Flick** - 6" color display
- **Inkplate 10** - 10.1" monochrome display

### Directory Structure

```
inkplate-dashboard/
├── boards/              # Board-specific code
│   ├── inkplate2/
│   │   ├── inkplate2.ino      # Board sketch
│   │   └── board_config.h     # Board configuration
│   ├── inkplate5v2/
│   ├── inkplate6flick/
│   └── inkplate10/
└── common/              # Shared code
    ├── library.properties
    ├── examples/
    └── src/             # Common source files
```

### How It Works

Each board has its own `.ino` sketch file that:
1. Validates the correct board is selected (compile-time check)
2. Includes its board-specific `board_config.h`
3. Includes the shared implementation from `common/src/main_sketch.ino.inc`

**Example: inkplate2.ino**
```cpp
#ifndef ARDUINO_INKPLATE2
#error "Wrong board selection for this sketch"
#endif

#include "board_config.h"
#include <src/main_sketch.ino.inc>
```

### Board Configuration

Each `board_config.h` defines board-specific constants:
- **BOARD_NAME** - Human-readable board name
- **SCREEN_WIDTH** / **SCREEN_HEIGHT** - Display dimensions
- **DISPLAY_MODE** - Color mode (INKPLATE_1BIT or INKPLATE_3BIT)
- **HAS_BUTTON** - Whether the board has a wake button
- **WAKE_BUTTON_PIN** - GPIO pin for wake button
- **UI Layout Constants** - Font sizes, margins, spacing

### Build System

The build system (`build.ps1` for Windows, `build.sh` for Linux/CI) handles compilation:

1. **Copies common `.cpp` files** from `common/src/` to each board's sketch directory
2. **Compiles** each board with:
   - Arduino CLI
   - Board-specific FQBN (Fully Qualified Board Name)
   - Include paths to find both common headers and board config
   - Compiler flag to auto-include `board_config.h`
3. **Cleans up** temporary `.cpp` files after compilation
4. **Produces** versioned firmware binaries (e.g., `inkplate5v2-v0.6.0.bin`)

## Common Components (common/src/)

### Core Implementation

#### main_sketch.ino.inc
The main program logic shared by all boards. Contains:
- **setup()** - Initialization, wake reason detection, mode selection
- **loop()** - Event handling for AP mode and config mode
- **Mode Selection Logic** - Determines whether to enter AP mode, config mode, or normal operation

### System Managers

#### ConfigManager (`config_manager.h/cpp`)
Manages persistent configuration storage using ESP32 Preferences API.

**Responsibilities:**
- Load/save WiFi credentials
- Load/save image URL and refresh rate
- Load/save MQTT broker settings
- Load/save optional settings (debug mode, CRC32 checking)
- Validate configuration completeness

**Key Methods:**
- `begin()` - Initialize preferences
- `loadConfig()` - Load all settings into DashboardConfig struct
- `saveWiFiConfig()` - Save WiFi SSID/password
- `saveImageConfig()` - Save image URL and refresh rate
- `hasWiFiConfig()` / `isFullyConfigured()` - Check configuration state

#### WiFiManager (`wifi_manager.h/cpp`)
Handles WiFi connectivity.

**Responsibilities:**
- Connect to configured WiFi network
- Set hostname based on device MAC
- Enable persistent credentials and auto-reconnect
- Disable WiFi sleep mode for faster wake cycles
- Measure signal strength (RSSI)

**Key Methods:**
- `connectToWiFi()` - Connect using saved credentials
- `disconnect()` - Disconnect and power down WiFi

#### PowerManager (`power_manager.h/cpp`)
Manages power states, wake sources, and watchdog protection.

**Responsibilities:**
- Detect wake-up reason (timer, button, reset, first boot)
- Detect button press type (short vs long press)
- Configure deep sleep with multiple wake sources
- Enable/disable hardware watchdog timer for operation protection
- Track device running state for battery calculations

**Wake Reasons:**
- `WAKEUP_FIRST_BOOT` - Initial power-on
- `WAKEUP_TIMER` - Scheduled wake from deep sleep
- `WAKEUP_BUTTON` - Wake button pressed
- `WAKEUP_RESET_BUTTON` - Hardware reset button (button-less boards)

**Watchdog Timer:**
- Protects normal operation against lockups (WiFi, MQTT, image download, display)
- Uses ESP32 hardware watchdog with automatic panic recovery
- Board-specific timeout: 30 seconds default, 60 seconds for Inkplate2
- Enabled only during `NormalModeController::execute()`
- Disabled before entering sleep to avoid interrupting sleep cycles
- On timeout: Forces device into deep sleep for automatic recovery

**Key Methods:**
- `begin()` - Initialize with button pin
- `getWakeupReason()` - Detect why device woke up
- `detectButtonPressType()` - Check for short/long press
- `enableWatchdog()` - Enable hardware watchdog timer (optional timeout parameter, uses board config default if not specified)
- `disableWatchdog()` - Disable hardware watchdog timer
- `prepareForSleep()` - Disconnect WiFi and prepare for deep sleep
- `enterDeepSleep()` - Enter deep sleep with timer and button wake sources

#### ImageManager (`image_manager.h/cpp`)
Handles image download and display.

**Responsibilities:**
- Download PNG images from HTTP/HTTPS URLs
- Display images on e-ink screen
- Store/retrieve last image CRC32 for change detection
- Provide download progress and error reporting

**Key Methods:**
- `downloadAndDisplay()` - Download and show image from URL
- `checkCRC32Changed()` - Fetch and compare CRC32 hash
- `saveCRC32()` / `loadCRC32()` - Persist CRC32 value
- `getLastError()` - Get error message from last operation

**CRC32 Feature:**
The server can provide a `.crc32` file alongside the image (e.g., `image.png.crc32`) containing a hex CRC32 hash. The device checks this before downloading to skip unchanged images, saving battery and bandwidth.

#### MQTTManager (`mqtt_manager.h/cpp`)
Manages MQTT connectivity and Home Assistant integration.

**Responsibilities:**
- Connect to MQTT broker with authentication
- Publish Home Assistant MQTT Discovery messages
- Publish telemetry (battery, WiFi signal, loop time)
- Publish log messages for monitoring

**Key Methods:**
- `begin()` - Initialize MQTT configuration
- `connect()` - Connect to broker with retry logic
- `publishDiscovery()` - Send Home Assistant auto-discovery configs
- `publishBatteryVoltage()` - Send battery level
- `publishWiFiSignal()` - Send WiFi RSSI
- `publishLoopTime()` - Send execution duration
- `publishLastLog()` - Send log message

#### DisplayManager (`display_manager.h/cpp`)
Low-level display operations and power management.

**Responsibilities:**
- Initialize Inkplate display
- Clear display buffer
- Power cycle display (important for e-ink refresh)
- Handle board-specific display initialization

**Key Methods:**
- `init()` - Initialize display with optional splash screen
- `clear()` - Clear display buffer
- `powerCycle()` - Power off/on display for clean refresh

#### ConfigPortal (`config_portal.h/cpp`)
Web-based configuration interface.

**Responsibilities:**
- Run captive portal web server (AP mode or config mode)
- Serve configuration HTML forms
- Handle form submissions
- Support OTA firmware updates
- Provide WiFi network scanning

**Modes:**
- **AP Mode** - Initial setup, only WiFi credentials
- **Config Mode** - Full configuration (WiFi, image URL, MQTT, options)

**Key Methods:**
- `begin()` - Start web server
- `handleClient()` - Process HTTP requests
- `isConfigReceived()` - Check if user submitted config
- `getMode()` - Get current portal mode (AP or Config)

### UI Components (common/src/ui/)

The UI layer provides reusable display components for different screens and messages.

#### UIMessages (`ui_messages.h/cpp`)
General purpose message rendering.

**Methods:**
- `showSplashScreen()` - Boot splash with board name and resolution
- `showHeading()` - Large centered heading text
- `showSubheading()` - Medium centered subheading
- `showNormalText()` - Regular text with word wrapping
- `showConfigInitError()` - Configuration initialization failure

#### UIError (`ui_error.h/cpp`)
Error screen displays.

**Methods:**
- `showWiFiError()` - WiFi connection failed
- `showImageError()` - Image download/display failed
- `showAPStartError()` - Access point startup failed
- `showPortalError()` - Config portal startup failed
- `showConfigLoadError()` - Cannot load configuration
- `showConfigModeFailure()` - Config mode entry failed

#### UIStatus (`ui_status.h/cpp`)
Status and progress screens.

**Methods:**
- `showAPModeSetup()` - AP mode with SSID and IP
- `showConfigModeSetup()` - Config mode with network info
- `showConfigModeConnecting()` - Connecting to WiFi in config mode
- `showDownloading()` - Image download in progress
- `showManualRefresh()` - Manual refresh triggered
- `showWiFiConfigured()` - WiFi credentials saved (AP mode)
- `showSettingsUpdated()` - Configuration updated (config mode)
- `showDebugStatus()` - Debug info (WiFi, refresh rate)

### Mode Controllers (common/src/modes/)

Mode controllers encapsulate the logic for each operating mode.

#### APModeController (`ap_mode_controller.h/cpp`)
First-boot WiFi configuration mode.

**Purpose:** Create WiFi access point for initial setup when device has no WiFi credentials.

**Workflow:**
1. Create WiFi AP with name `Inkplate-XXXXXX`
2. Start config portal in AP mode
3. Display setup instructions (SSID, IP, password)
4. Wait for user to connect and submit WiFi credentials
5. Restart device to enter config mode

**Key Methods:**
- `begin()` - Start AP and portal
- `handleClient()` - Process portal requests
- `isConfigReceived()` - Check if WiFi configured

#### ConfigModeController (`config_mode_controller.h/cpp`)
Full configuration mode (triggered by button or auto-entry).

**Purpose:** Configure all dashboard settings (image URL, MQTT, options) after WiFi is set up.

**Workflow:**
1. Connect to configured WiFi
2. Start config portal in config mode
3. Display config instructions with timeout countdown
4. Wait for user to update settings (or timeout after 5 minutes)
5. On timeout: save partial config and enter deep sleep
6. On success: restart device

**Key Methods:**
- `begin()` - Connect WiFi and start portal
- `handleClient()` - Process portal requests
- `isConfigReceived()` - Check if config submitted
- `isTimedOut()` - Check 5-minute timeout
- `handleTimeout()` - Handle timeout gracefully

#### NormalModeController (`normal_mode_controller.h/cpp`)
Normal operation mode (image refresh cycle).

**Purpose:** Execute the main dashboard update cycle with hardware watchdog protection.

**Watchdog Protection:**
- Enabled at the start of the update cycle to protect against lockups
- Monitors WiFi connection, MQTT publishing, image download, and display operations
- If any step hangs longer than the timeout, ESP32 forces device into deep sleep for automatic recovery
- Timeout is board-specific: 30 seconds default, 60 seconds for Inkplate2 (slower display)
- Disabled before entering sleep to allow device to complete the sleep cycle safely
- NOT active during config mode or AP mode (user configuration must never be interrupted)

**Workflow:**
1. **Enable watchdog timer** (protects all operations below)
2. Load configuration
3. Connect to WiFi
4. Publish MQTT telemetry (if configured)
5. Check CRC32 for image changes (if enabled)
6. Download and display image
7. Save CRC32 after successful display
8. Publish loop time to MQTT
9. **Disable watchdog timer** (before entering sleep)
10. Enter deep sleep until next refresh

**Special Cases:**
- **Manual Refresh** - Button press during sleep forces image download even if CRC32 unchanged
- **Retry Mechanism** - Up to 3 retry attempts with 20-second delays on download failure
- **CRC32 Skip** - Timer wake with matching CRC32 skips download entirely
- **Watchdog Timeout** - Device automatically enters deep sleep for recovery (prevents battery drain from lockups)

**Key Methods:**
- `execute()` - Run complete update cycle (enables watchdog at start, disables before sleep)

### Utilities

#### logger.h/cpp
Structured serial logging with LogBox.

**Usage:**
```cpp
LogBox::begin("Operation Name");
LogBox::line("Simple message");
LogBox::linef("Formatted: %d", value);
LogBox::end();
```

**Features:**
- Box-drawing characters for visual structure
- Automatic line prefixing
- Printf-style formatting
- Persistent last log for MQTT publishing

#### utils.h/cpp
Utility functions.

**Functions:**
- `formatMillisToTime()` - Convert milliseconds to "Xm Ys" format
- `isHttps()` - Check if URL uses HTTPS

#### config.h
Shared configuration constants and structs.

**DashboardConfig Struct:**
```cpp
struct DashboardConfig {
    String wifiSSID;
    String wifiPassword;
    String imageURL;
    uint16_t refreshRate;     // Minutes
    String mqttBroker;
    String mqttUsername;
    String mqttPassword;
    bool useCRC32Check;
    bool debugMode;
};
```

## Operating Modes

### 1. AP Mode (First Boot)
**Trigger:** Device has no WiFi configuration
**Purpose:** Initial WiFi setup
**Display:** Shows AP SSID, IP, and password
**Exit:** User submits WiFi credentials → Restart → Config Mode

### 2. Config Mode
**Triggers:**
- WiFi configured but missing image URL (auto-entry)
- Long button press (2+ seconds)
- Hardware reset button on button-less boards (if fully configured)

**Purpose:** Configure or update all dashboard settings
**Display:** Shows network info, portal URL, 5-minute timeout countdown
**Exit:** 
- User submits config → Restart → Normal Operation
- 5-minute timeout → Deep Sleep

### 3. Normal Operation
**Trigger:** Device is fully configured
**Purpose:** Regular dashboard updates
**Display:** Downloaded dashboard image
**Exit:** Deep sleep until next refresh (or button wake)

## Wake-Up Behavior

### First Boot
- Enter AP Mode (if no WiFi) or Config Mode (if WiFi but no image URL)

### Timer Wake
- Execute normal update cycle
- Skip download if CRC32 unchanged (saves battery)

### Button Wake
**Short Press (<2 sec):**
- Force image refresh (ignores CRC32)
- Shows "Manual Refresh" message

**Long Press (≥2 sec):**
- Enter Config Mode

### Reset Button Wake (Button-less Boards)
- Enter Config Mode (if fully configured)
- Enter AP Mode (if not configured)

## Deep Sleep

The device spends most of its time in deep sleep to conserve battery.

**Wake Sources:**
- **Timer** - Wake at configured refresh interval
- **Button** - User presses wake button (on boards that have one)

**Sleep Duration:**
- Normal operation: User-configured refresh rate
- Config timeout: Same as refresh rate (or 5 min default)
- Download failure retry: 20 seconds (for 3 attempts)

## CRC32 Change Detection

An optional battery-saving feature to skip unchanged image downloads.

**How It Works:**
1. Server provides `<image-url>.crc32` file with 8-digit hex hash
2. Device fetches CRC32 before downloading image
3. If CRC32 matches stored value and wake was timer-based → Skip download
4. If CRC32 differs or wake was button-based → Download image
5. After successful display, save new CRC32 value

**Benefits:**
- Reduces WiFi usage
- Extends battery life
- Faster wake cycles when image unchanged

**Configuration:**
- Enable/disable in config portal
- Works with both HTTP and HTTPS

## Build Process

### Arduino CLI Compilation

**Requirements:**
- Arduino CLI
- ESP32 Arduino core (Inkplate boards package)
- Inkplate library

**Build Commands:**
```powershell
# Windows - Single board
.\build.ps1 inkplate5v2

# Windows - All boards
.\build.ps1 all

# Linux/CI - Single board
./build.sh inkplate5v2

# Linux/CI - All boards
./build.sh all
```

**Build Process:**
1. Extract firmware version from `common/src/version.h`
2. For each board:
   - Copy `common/src/*.cpp` to board sketch directory
   - Copy `common/src/ui/*.cpp` to board sketch directory
   - Copy `common/src/modes/*.cpp` to board sketch directory
   - Compile with Arduino CLI using board-specific FQBN
   - Clean up temporary `.cpp` files
   - Rename binary to include version (e.g., `inkplate5v2-v0.6.0.bin`)

**Include Path Resolution:**
- Common headers use `<src/...>` syntax
- Build system adds `common/` and `common/src/` to include path
- Each board directory is added for `board_config.h`
- Compiler auto-includes `board_config.h` via `-include` flag

## Memory Considerations

### RTC Memory
Variables marked with `RTC_DATA_ATTR` persist across deep sleep:
- `imageRetryCount` - Tracks download retry attempts

### Preferences (NVS)
Persistent storage for configuration (survives power loss):
- WiFi credentials
- Image URL and refresh rate
- MQTT settings
- Optional features (CRC32, debug mode)
- Last image CRC32 value

### Heap Memory
E-ink displays require significant RAM for framebuffer:
- Inkplate 2: ~5KB (212×104 pixels)
- Inkplate 5: ~38KB (960×540 pixels)
- Inkplate 6: ~60KB (800×600 pixels)
- Inkplate 10: ~150KB (1200×825 pixels)

Image decoding (PNG) also requires temporary buffers.

## Extension Points

### Adding a New Board

1. Create directory: `boards/inkplate_new/`
2. Create sketch: `inkplate_new.ino`
   ```cpp
   #ifndef ARDUINO_INKPLATE_NEW
   #error "Wrong board selection"
   #endif
   #include "board_config.h"
   #include <src/main_sketch.ino.inc>
   ```
3. Create `board_config.h` with constants
4. Add to build scripts (`build.ps1` and `build.sh`)
5. Test compilation and functionality

### Adding Configuration Options

1. Add field to `DashboardConfig` struct in `config.h`
2. Update `ConfigManager` load/save methods
3. Add form field to `ConfigPortal` HTML
4. Use new setting in relevant managers/controllers

### Adding MQTT Topics

1. Add publish method to `MQTTManager`
2. Add discovery message if Home Assistant integration desired
3. Call from appropriate controller during update cycle

### Adding UI Screens

1. Add method to appropriate UI class (`UIMessages`, `UIError`, or `UIStatus`)
2. Follow existing patterns for layout and font usage
3. Call from controllers where needed
