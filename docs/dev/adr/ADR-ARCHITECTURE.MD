
# Architecture Decision Record: Multi-Board Architecture

## Status
Accepted

## Context
The Inkplate Dashboard project must support multiple e-ink display boards (Inkplate 2, 5 V2, 6 Flick, 10) with both shared and board-specific code, while minimizing code duplication and maintenance overhead.

## Decision
Adopt a multi-board architecture where:
- Common code is placed in a shared `common/` directory.
- Each board has its own directory under `boards/` with a board-specific sketch and configuration header.
- The build system copies shared code into each board's build context and compiles with board-specific settings.

## Consequences
- Enables support for multiple Inkplate boards with minimal duplication.
- Board-specific features and configuration are isolated, while core logic is shared.
- The build system is more complex but flexible for future board additions.

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
- Read battery voltage and calculate percentage from Li-ion discharge curve

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
- `readBatteryVoltage()` - Read battery voltage in volts
- `calculateBatteryPercentage()` - Static method to calculate percentage from voltage (Li-ion curve, 5% granularity)
- `enableWatchdog()` - Enable hardware watchdog timer (optional timeout parameter, uses board config default if not specified)
- `disableWatchdog()` - Disable hardware watchdog timer
- `prepareForSleep()` - Disconnect WiFi and prepare for deep sleep
- `enterDeepSleep()` - Enter deep sleep with timer and button wake sources

#### ImageManager (`image_manager.h/cpp`)
Handles image download and display with optional overlay rendering.

**Responsibilities:**
- Download PNG images from HTTP/HTTPS URLs
- Display images on e-ink screen
- Render status overlay on top of images (if enabled)
- Store/retrieve last image CRC32 for change detection
- Provide download progress and error reporting

**Key Methods:**
- `downloadAndDisplay()` - Download and show image from URL with optional overlay
- `checkCRC32Changed()` - Fetch and compare CRC32 hash
- `saveCRC32()` / `loadCRC32()` - Persist CRC32 value
- `getLastError()` - Get error message from last operation
- `setOverlayManager()` - Wire overlay manager for status rendering

**CRC32 Feature:**
The server can provide a `.crc32` file alongside the image (e.g., `image.png.crc32`) containing a hex CRC32 hash. The device checks this before downloading to skip unchanged images, saving battery and bandwidth.

**Overlay Integration:**
After successful image display, ImageManager renders an optional status overlay using OverlayManager (see below).

#### OverlayManager (`overlay_manager.h/cpp`)
Renders configurable status overlays on dashboard images.

**Purpose:** Display battery level, update time, and cycle time information on top of dashboard images for at-a-glance status monitoring.

**Responsibilities:**
- Render battery icon with fill level (rounded corners for medium/large sizes)
- Render battery percentage text
- Render update time (24-hour format, timezone-aware)
- Render cycle time (performance metric)
- Position overlay at configured corner
- Respect user color and size preferences

**Key Methods:**
- `renderOverlay()` - Main rendering method, respects DashboardConfig settings
- `drawBatteryIcon()` - Public helper for drawing battery icon at arbitrary positions (used by UI components)
- `getFontForSize()` - Get GFXfont based on size setting (Small/Medium/Large)
- `calculateOverlayPosition()` - Calculate X/Y coordinates for configured position

**Configuration (via DashboardConfig):**
- **Position**: Top Left, Top Right, Bottom Left, Bottom Right
- **Size**: Small (7pt), Medium (12pt), Large (20pt)
- **Color**: Black (0), Dark Gray (2), Light Gray (5), White (7)
- **Elements**: Battery icon, battery %, update time, cycle time (all toggleable)

**Technical Details:**
- Uses GFXfont baseline positioning for text
- Battery icon uses proportional scaling (3:5 aspect ratio)
- Respects `DISPLAY_MINIMAL_UI` board flag
- Rotation-aware positioning
- Grayscale color values (0-7 for 3-bit displays)

#### MQTTManager (`mqtt_manager.h/cpp`)
Manages MQTT connectivity and Home Assistant integration with optimized battery-powered operation.

**Responsibilities:**
- Connect to MQTT broker with authentication
- Publish Home Assistant MQTT Discovery messages (conditional based on wake reason)
- Publish telemetry (battery voltage, battery percentage, WiFi signal, WiFi BSSID, loop time, loop time breakdown)
- Publish log messages for monitoring
- Batch publishing for single-session efficiency

**Key Features:**
- **Conditional Discovery**: Only publishes discovery on first boot and hardware reset (skips normal timer wakes)
- **Single Session Publishing**: `publishAllTelemetry()` batches all MQTT operations into one connection
- **Fire-and-Forget States**: State messages don't wait for ACK (faster publishing)
- **Discovery Result Checking**: Discovery messages still verify publish success for debugging
- **Optimized Timeouts**: Socket timeout 2s, keep-alive 5s for faster connections

**Key Methods:**
- `begin()` - Initialize MQTT configuration
- `connect()` - Connect to broker with retry logic
- `publishAllTelemetry()` - Batch publish all telemetry in single session (optimized, preferred)
- `publishDiscovery()` - Send Home Assistant auto-discovery configs (legacy)
- `publishBatteryVoltage()` - Send battery voltage (legacy)
- `publishBatteryPercentage()` - Send battery percentage (legacy)
- `publishWiFiSignal()` - Send WiFi RSSI (legacy)
- `publishLoopTime()` - Send execution duration (legacy)
- `publishLastLog()` - Send log message (legacy)
- `shouldPublishDiscovery()` - Determine if discovery should be published based on wake reason
- `buildDeviceInfoJSON()` - Build device info JSON for discovery messages

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

The UI layer provides reusable display components for different screens and messages. All UI classes inherit from UIBase for shared functionality.

#### UIBase (`ui_base.h/cpp`)
Base class providing common UI rendering functionality.

**Purpose:** Eliminate code duplication across UI components by centralizing shared patterns.

**Members:**
- `displayManager` - Display abstraction layer
- `overlayManager` - Overlay rendering manager (for battery icons on logo screens)

**Methods:**
- `setOverlayManager()` - Wire overlay manager for battery icon rendering
- `drawBatteryIconBottomLeft()` - Render medium-sized black battery icon at bottom-left (used on logo screens)

**Inheritance:** Extended by UIMessages, UIError, and UIStatus.

#### UIMessages (`ui_messages.h/cpp`)
General purpose message rendering. Inherits from UIBase.

**Methods:**
- `showSplashScreen()` - Boot splash with board name and resolution
- `showHeading()` - Large centered heading text
- `showSubheading()` - Medium centered subheading
- `showNormalText()` - Regular text with word wrapping
- `showConfigInitError()` - Configuration initialization failure

#### UIError (`ui_error.h/cpp`)
Error screen displays. Inherits from UIBase.

**Methods:**
- `showWiFiError()` - WiFi connection failed
- `showImageError()` - Image download/display failed
- `showAPStartError()` - Access point startup failed
- `showPortalError()` - Config portal startup failed
- `showConfigLoadError()` - Cannot load configuration
- `showConfigModeFailure()` - Config mode entry failed

#### UIStatus (`ui_status.h/cpp`)
Status and progress screens. Inherits from UIBase.

**Methods:**
- `showAPModeSetup()` - AP mode with SSID and IP
- `showConfigModeSetup()` - Config mode with network info
- `showConfigModeConnecting()` - Connecting to WiFi in config mode
- `showDownloading()` - Image download in progress
- `showManualRefresh()` - Manual refresh triggered
- `showWiFiConfigured()` - WiFi credentials saved (AP mode)
- `showSettingsUpdated()` - Configuration updated (config mode)
- `showDebugStatus()` - Debug info (WiFi, refresh rate)

**Battery Icon Feature:** All logo screens (splash, setup screens, manual refresh) automatically display a medium-sized black battery icon at bottom-left, independent of user overlay configuration.

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

**Purpose:** Execute the main dashboard update cycle with hardware watchdog protection and hourly scheduling support.

**Watchdog Protection:**
- Enabled at the start of the update cycle to protect against lockups
- Monitors WiFi connection, MQTT publishing, image download, and display operations
- If any step hangs longer than the timeout, ESP32 forces device into deep sleep for automatic recovery
- Timeout is board-specific: 30 seconds default, 60 seconds for Inkplate2 (slower display)
- Disabled before entering sleep to allow device to complete the sleep cycle safely
- NOT active during config mode or AP mode (user configuration must never be interrupted)

**Hourly Scheduling:**
- After WiFi connection, device syncs time via NTP (pool.ntp.org, time.nist.gov)
- Checks if current hour is enabled in the 24-bit bitmask
- If hour is **disabled**: Calculates sleep duration until next enabled hour and enters deep sleep immediately
- If hour is **enabled**: Proceeds with normal update cycle
- Default: All 24 hours enabled (backward compatible)
- Enables significant battery savings by skipping entire disabled periods (e.g., midnight to 6 AM)

**Workflow:**
1. **Enable watchdog timer** (protects all operations below)
2. Load configuration
3. Collect telemetry data early (battery voltage, wake reason)
4. Connect to WiFi (capture RSSI)
5. Sync time via NTP and check hourly schedule
   - If hour disabled: Calculate sleep until next enabled hour and sleep immediately
   - If hour enabled: Continue to next step
6. Check CRC32 for image changes (if enabled)
   - If CRC32 matches on timer wake: Publish telemetry with "unchanged" message and sleep immediately
7. Download and display image
8. Save CRC32 after successful display (if enabled)
9. **Publish all MQTT telemetry in single session** (if configured)
   - Discovery messages: Only on first boot and hardware reset
   - State messages: Battery (voltage, percentage), WiFi (signal, BSSID), loop timing (total, WiFi, NTP, CRC32, Image), optional log message
10. **Disable watchdog timer** (before entering sleep)
11. Enter deep sleep until next refresh

**Special Cases:**
- **Manual Refresh** - Button press during sleep forces image download and ignores hourly schedule
- **Retry Mechanism** - Up to 3 retry attempts with 20-second delays on download failure
- **CRC32 Skip** - Timer wake with matching CRC32 skips download entirely
- **Hourly Skip** - Timer wake during disabled hour skips WiFi connection, image download, and sleeps until next enabled hour
- **Watchdog Timeout** - Device automatically enters deep sleep for recovery (prevents battery drain from lockups)

**Hourly Schedule Example:**
```
Configuration: Sleep 11 PM - 6 AM (hours 0-5 disabled, 6-23 enabled)

3 AM wake:    Hourly check fails → Sleep until 6 AM (3 hours)
2 PM wake:    Hourly check passes → Perform normal update → Sleep 5 minutes
```

**Key Methods:**
- `execute()` - Run complete update cycle (enables watchdog at start, disables before sleep)
- `calculateSleepUntilNextEnabledHour()` - Find next enabled hour and calculate sleep duration

**See Also:**
- [Hourly Scheduling Documentation](HOURLY_SCHEDULING.md) - Comprehensive guide to hourly scheduling feature

### Utilities

#### logger.h/cpp
Structured serial logging with the Logger class.

**Usage:**
```cpp
Logger::begin("Operation Name");
Logger::line("Simple message");
Logger::linef("Formatted: %d", value);
Logger::end();
```

**Features:**
- Indentation-based visual structure
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
    uint8_t updateHours[3];   // 24-bit bitmask for hourly scheduling (bits 0-23 = hours 0-23)
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
2. Add discovery message if Home Assistant integration desired (with appropriate device_class, unit_of_measurement, force_update)
3. Add parameter to `publishAllTelemetry()` to batch publish with other sensors
4. Call from appropriate controller during update cycle (e.g., `NormalModeController::execute()`)

**Example: Loop Time Breakdown (v1.2.0)**
- Added `LoopTimings` struct in `NormalModeController` to track WiFi, NTP, CRC32, Image operation durations
- Added timing parameters to `publishAllTelemetry()` signature
- Added discovery messages for `loop_time_wifi`, `loop_time_ntp`, `loop_time_crc`, `loop_time_image` with device_class "duration", unit "s", and force_update true
- All timing values always published (0 = operation skipped) for correlation via timestamps

### Adding UI Screens

1. Add method to appropriate UI class (`UIMessages`, `UIError`, or `UIStatus`)
2. Follow existing patterns for layout and font usage
3. Call from controllers where needed
