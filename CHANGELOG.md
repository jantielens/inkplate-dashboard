
# Changelog

## [Unreleased]

## [0.16.0] - 2025-10-24

### Changed
- **Config Portal UI Redesign** - Complete overhaul for better visual hierarchy and user experience
  - Card-based section layout with white backgrounds and shadows
  - Blue gradient section headers for clear visual separation
  - Floating battery life estimator badge that follows scroll position
  - Improved mobile responsiveness and touch-friendly controls
  - Enhanced visual styling for warnings, errors, and success messages
  
## [0.15.0] - 2025-10-23

### Added
- Manual OTA firmware updates from GitHub Releases (issue #35)
  - Check for latest firmware version directly from config portal
  - Download and install updates from GitHub with one click
  - Progress indicators on both web UI and e-ink display
  - Uses unauthenticated GitHub API (60 requests/hour limit)
  - Error handling for network failures, rate limits, and invalid firmware

### Technical
- New `GitHubOTA` class for GitHub Releases API integration
- New endpoints: `/ota/check` (GET) and `/ota/install` (POST)
- Semantic versioning comparison for update detection
- Board name to asset prefix mapping (e.g., "Inkplate 2" → "inkplate2")
- Streaming download with progress callbacks
- Enhanced OTA page with dual update options

## [0.14.0] - 2025-10-22

### Added
- Web-based firmware flasher using ESP Web Tools
  - Flash firmware directly from browser (Chrome/Edge) without installing software
  - Support for all 4 Inkplate boards (2, 5 V2, 6 Flick, 10)
  - Complete 3-part flashing: bootloader, partition table, and firmware
  - Automated manifest generation via GitHub Actions on release
  - Local testing environment for development
  - Responsive UI with dark mode support and comprehensive troubleshooting guide

### Changed
- Build system now generates versioned bootloader and partition binaries for web flasher support
- Release workflow updated to automatically generate and publish ESP Web Tools manifests

## [0.13.0] - 2025-10-21

### Added
- VCOM (Common Voltage) management system for e-ink display calibration
  - Read current panel VCOM value from TPS65186 PMIC via I2C
  - Display VCOM value at startup in serial logs for diagnostics
  - Program new VCOM values to TPS65186 EEPROM with safety validation
  - Dedicated VCOM management page in configuration portal
  - Explicit warnings and user confirmation required before programming
  - Range validation: -5.0V to 0V with automatic conversion to raw register values
  - Comprehensive diagnostics logging (both serial console and web UI)
  - Automatic verification: power cycle and readback from EEPROM after programming
  - Polling-based wait for EEPROM programming completion (bit 6 auto-clear detection)
  - Integrated into "Danger Zone" section with styled warning button
  - Helps users optimize display contrast and image quality safely

### Changed
- Configuration portal danger zone layout improved with dedicated VCOM warning

## [0.12.1] - 2025-10-19

### Added
- New 200x200 Inkplate logo implemented as a packed 4bpp bitmap for better splash branding

### Fixed
- Build error in `config_portal.cpp` due to missing include for logo symbols (now resolved)


## [0.12.0] - 2025-10-19

### Added
- Battery life estimator in configuration portal (issue #43)
  - Real-time battery life calculation based on user configuration
  - Interactive estimator updates instantly as settings change (refresh rate, CRC32, hourly schedule, battery capacity, daily changes)
  - User-configurable "Expected Image Changes Per Day" parameter for accurate CRC32 savings calculations
  - Displays estimated battery life in days and months with color-coded status indicators
  - Four status levels: Excellent (180+ days), Good (90-179 days), Moderate (45-89 days), Poor (<45 days)
  - Detailed metrics: daily power consumption, wake-ups per day, active time, sleep time
  - Visual progress bar with color-coded gradients matching status
  - Context-aware tips for optimizing battery life based on current configuration
  - Battery capacity selector: 600mAh, 1200mAh (default), 1800mAh, 3000mAh, 6000mAh
  - Informational banner clarifying estimator is information-only (battery capacity setting not persisted)
  - Transparent calculation assumptions disclosed to users
  - Pure client-side JavaScript implementation - zero ESP32 memory overhead
  - Well-commented power constants for easy refinement based on real-world measurements
  - Helps users make informed decisions about power vs. convenience tradeoffs

## [0.11.0] - 2025-10-19

### Added
- Screen rotation configuration parameter (issue #41)
  - New user-configurable setting to rotate display for portrait/landscape mounting
  - Four rotation options: 0° (Landscape), 90° (Portrait), 180° (Inverted Landscape), 270° (Portrait Inverted)
  - Dropdown selector in web configuration portal with help text
  - Setting persists across reboots in NVS storage
  - Default: 0° (landscape) for backward compatibility with existing devices
  - Applied to essential UI screens (errors, setup instructions, confirmation messages)
  - Images must be pre-rotated to match the selected orientation
  - Documentation updated with rotation configuration guide and image dimension tables
  - Validation ensures only valid rotation values (0-3) are accepted

### Changed
- `DisplayManager::init()` now accepts optional rotation parameter
- Display coordinate system selectively adjusts rotation based on screen type
- Image size requirements tables updated to show both landscape and portrait dimensions
- Configuration portal includes rotation field after timezone offset setting
- Image rendering temporarily resets rotation to 0 for performance (images expected to be pre-rotated)

### Performance
- **Selective rotation optimization**: Rotation is enabled for nearly all screens (errors, setup, splash, confirmations, manual refresh), with only 2 extremely transient debug screens skipping rotation for speed
- Normal operation cycle time: ~6 seconds without debug mode (no text screens shown), ~6 seconds with debug mode (transient screens skip rotation)
- Only 2 transient screens skip rotation: `showDebugStatus()` (debug mode only) and `showDownloading()` (immediately replaced by image)
- All user-facing screens display at configured rotation for optimal readability (15+ screens including errors, setup, splash, manual refresh, confirmations)
- Added rotation state caching to avoid redundant `setRotation()` calls
- See `ROTATION_PERFORMANCE_OPTIMIZATION.md` for detailed analysis and implementation notes

## [0.10.0] - 2025-10-19

### Added
- Battery percentage reporting via MQTT alongside voltage
  - New `battery_percentage` sensor with Home Assistant auto-discovery
  - Calculates percentage from voltage using lithium-ion discharge curve
  - Uses 5% granularity (100%, 95%, 90%, etc.) to reduce MQTT update frequency
  - Typical Li-ion/LiPo battery characteristics for ESP32 devices
  - Voltage range: 4.2V (100%) to 3.0V (0%) with non-linear interpolation
  - Static calculation method `PowerManager::calculateBatteryPercentage()`
  - Published in all MQTT telemetry updates
- Image CRC32 checksum reporting via MQTT
  - New `image_crc32` sensor with Home Assistant auto-discovery
  - Publishes hexadecimal CRC32 of currently displayed image (e.g., `0xABCD1234`)
  - Published with every MQTT telemetry update
  - Allows monitoring of which image version is currently displayed
  - Useful for tracking display state across device reboots

### Changed
- MQTT telemetry now includes battery percentage in addition to voltage
- Battery percentage displayed in serial logs alongside voltage
- Home Assistant device now shows both battery voltage and percentage sensors
- MQTT sensor names updated for clarity (e.g., "Battery Voltage" instead of "Battery")
- MQTT state messages now published with retain flag
  - Ensures Home Assistant receives telemetry values even on first boot timing issues
  - State messages persist on broker for reliability across reconnections
  - Matches discovery message behavior (both now use retained messages)

## [0.9.0] - 2025-10-19

### Added
- Hourly scheduling feature (issue #3)
  - 24-hour per-hour control via web configuration portal
  - Schedule stored as 3-byte (24-bit) bitmask in NVS (hours 0-23)
  - Interactive checkbox grid showing which hours are enabled for updates
  - Device skips entire disabled periods using smart sleep calculation
  - Updates only occur during configured enabled hours
  - Default: all hours enabled for backward compatibility
- Timezone offset support with DST awareness (issue #3)
  - New configuration option to set timezone offset (-12 to +14 hours)
  - Timezone offset applied to NTP-synchronized UTC time for hour calculations
  - DST (Daylight Saving Time) support through manual offset adjustment
  - Help text warns about manual DST adjustment requirements
- NTP time synchronization (issue #3)
  - Automatic time sync via NTP after WiFi connection
  - Uses pool.ntp.org and time.nist.gov servers
  - Time required for hourly schedule verification

### Changed
- Normal update cycle now includes hourly schedule check
  - Occurs before CRC32 check if hour is disabled
  - Disabled hours skip image download entirely to save battery
  - Enabled hours proceed with normal update flow (CRC32, download, display)
- Battery optimization on disabled hours
  - Device calculates sleep until next enabled hour (not just refresh interval)
  - Skips multiple refresh cycles during disabled periods
  - Estimated 17% battery savings with 8-hour night disable window
- Wake source handling
  - Hourly schedule only enforced on `WAKEUP_TIMER` (deep sleep wakeups)
  - Manual button press (short or long) bypasses hourly schedule check
  - Config mode entry bypasses hourly schedule check
  - First boot (after restart) bypasses hourly schedule check
- NVS optimization
  - `markDeviceRunning` moved to first boot only (not on every normal wake)
  - Reduces NVS write cycles on every update

### Fixed
- Code duplication in hour checking and timezone calculations
  - Extracted `isHourEnabledInBitmask()` static helper for single-point maintenance
  - Extracted `applyTimezoneOffset()` static helper for timezone calculation
  - Reduced duplicated code by 17 lines
  - Improved code clarity and testability

### Documentation
- Updated README features list to include hourly scheduling
- Added "Update Hours" configuration option to README
- Comprehensive "Update Hours" section added to USING.md with examples
- Added "Timezone Offset" section to USING.md with DST explanation
- Created HOURLY_SCHEDULING.md with complete feature documentation
- Created HOURLY_SCHEDULING_IMPLEMENTATION.md with technical implementation details
- Updated ARCHITECTURE.md with hourly scheduling component overview

### Performance Impact
- Battery life improvement: 17% savings on typical 8-hour disabled window
- No performance impact on update operations (only affects wake-up decision)
- Time sync adds ~1 second per enabled hour wake
- Smart sleep calculation only runs if hour is disabled

## [0.8.1] - 2025-10-18

### Added
- LogBox automatic timing display (issue #29)
  - All LogBox instances now automatically show elapsed time in milliseconds in the bottom border
  - Format: `╰──────(NNNNms)───────` (always in ms, never converted to seconds)
  - Static variable tracks time from `begin()` to `end()` with zero overhead
  - No changes required at call sites - timing is automatic for all LogBox usages
- Deep sleep full loop time reporting (issue #29)
  - `enterDeepSleep()` now accepts optional `loopTimeMs` parameter
  - Displays total loop time from main loop start to deep sleep entry
  - Format: `Full loop time: NNNNms` shown in deep sleep LogBox when provided
  - Helps monitor total wake cycle duration for battery optimization
- MQTT telemetry value logging
  - Serial log now shows actual values being published to MQTT
  - Displays: Battery voltage, WiFi signal strength, Loop time, Last log message
  - Format: `Battery: 4.202 V`, `WiFi Signal: -51 dBm`, `Loop Time: 6.08 s`, `Last Log: [INFO] message`
  - Improves debugging and verification of MQTT integration

### Changed
- Success messages added to MQTT telemetry for all normal operations
  - `"Image updated successfully"` when CRC32 detects new image
  - `"Image displayed successfully"` when image displayed without CRC32 change or forced refresh
  - Ensures MQTT always receives status updates for successful operations
  - Complements existing error and CRC32 skip messages

### Fixed
- Removed unnecessary 1-second delay after MQTT publishing in success path
  - Improves battery efficiency by ~16% on image download cycles
  - Loop time accuracy improved from 84% to 96%+ (difference reduced from 1.3s to 0.25s)
  - MQTT messages still fully transmitted (proper disconnect + WiFi shutdown already handle flushing)
  - Image download cycles now complete in ~6.3s instead of ~8.3s
  - CRC32 skip cycles remain ultra-fast at ~0.4s

### Performance Impact
- Wake cycle time reduced by 1 second on all image download operations
- Loop time reporting accuracy improved from 84% to 96%+ 
- Battery life improved due to faster overall cycle times
- Serial output provides better visibility into timing and MQTT values

## [0.8.0] - 2025-10-18

### Added
- MQTT optimization for battery-powered operation (issue #21)
  - New `publishAllTelemetry()` batch method publishes all MQTT data in a single session
  - Conditional discovery publishing based on wake reason (only on first boot and hardware reset)
  - Discovery messages skip normal timer wakes, reducing MQTT traffic by ~2KB per cycle
  - Helper methods `buildDeviceInfoJSON()` and `shouldPublishDiscovery()` for cleaner code
  - Wake reason passed through to MQTT manager for intelligent discovery decisions

### Changed
- MQTT session optimization: reduced from 2 sessions to 1 per normal cycle (50% reduction)
  - Battery voltage now collected before WiFi connection for faster telemetry gathering
  - All sensor data (battery, RSSI, loop time, log message) passed to single MQTT publish call
  - MQTT publishing moved to end of cycle (after image display) in all paths
- MQTT timeout optimization for faster connections
  - Socket timeout reduced from 10s to 2s
  - Keep-alive interval reduced from 15s to 5s
- Fire-and-forget optimization for state messages (no ACK wait)
  - Discovery messages still check publish results for debugging (rare events)
  - State messages use fire-and-forget for maximum speed (frequent events)
- Normal mode controller refactoring for improved maintainability
  - Removed excessive debug logging (323 lines → 171 lines, 47% reduction)
  - Eliminated redundant wrapper methods (`connectWiFi`, simplified `downloadAndDisplayImage`)
  - Extracted `publishMQTTTelemetry()` helper to eliminate code duplication
  - Simplified CRC32 save logic with clear boolean conditions
  - Streamlined error handlers with reduced LogBox overhead

### Performance Impact
- MQTT cycle time reduced from 3-5 seconds to 1-2 seconds (60-67% faster)
- Discovery traffic eliminated on normal timer wakes (100% reduction, ~2KB saved per cycle)
- Improved battery life due to faster wake cycles and reduced network activity
- Time to display image significantly reduced (MQTT no longer blocks image rendering)

### Technical Details
- Discovery published only on `WAKEUP_FIRST_BOOT` and `WAKEUP_RESET_BUTTON`
- Discovery skipped on `WAKEUP_TIMER` (normal refresh) and `WAKEUP_BUTTON` (manual refresh)
- Home Assistant entities created on first boot, preserved across normal wake cycles
- Backward compatible with existing deployments (no breaking changes)

## [0.7.0] - 2025-10-17

### Added
- Watchdog timer implementation (issue #18)
  - Protects normal operation against lockups during WiFi, MQTT, image download, or display steps
  - Forces device into deep sleep on timeout to enable battery-powered automatic recovery
  - Board-specific timeout configuration: 30 seconds default, 60 seconds for Inkplate2 (slower display)
  - Disabled during config and AP modes to allow uninterrupted user configuration
  - Uses ESP32 hardware watchdog (`esp_task_wdt`) with automatic panic recovery

### Changed
- Major code refactoring to improve maintainability and readability
  - Extracted UI components into dedicated classes (UIMessages, UIError, UIStatus)
  - Extracted mode logic into controller classes (APModeController, ConfigModeController, NormalModeController)
  - Reduced main_sketch.ino.inc from 779 lines to ~250 lines (68% reduction)
  - Improved code organization with clear separation of concerns
  - Added comprehensive architecture documentation (ARCHITECTURE.md)

### Fixed
- Image display not refreshing on manual refresh (pre-existing bug from v0.3.1)
  - Added missing display() call after drawImage() in ImageManager
  - Manual refresh now properly shows updated image on e-ink screen

## [0.6.0] - 2025-10-17

### Added
- Structured serial logging with ASCII box-drawing characters (issue #20)
  - New `LogBox` utility class for consistent, visually organized log output
  - ASCII box-drawing format with `+--` borders and `|   ` content lines
  - Three logging methods: `begin(title)`, `line(message)`, `linef(format, ...)`, `end()`
  - Terminal-compatible characters for universal display support
  - Centralized logging pattern eliminates code duplication
- Deferred CRC32 save implementation (issue #56 improvement)
  - CRC32 checksums now saved only after successful image display, preventing checksum/image desynchronization
  - Guarantees no missed image updates while maintaining battery optimization

### Changed
- Refactored all serial logging across entire codebase to use LogBox
- Replaced mixed `Serial.println()`/`Serial.printf()` calls with consistent LogBox pattern
- Improved log readability with structured formatting
- All numeric values now use printf-style formatting via `linef()` for consistency
- ImageManager API enhanced: `checkCRC32Changed()` with optional output parameter and new `saveCRC32()` method
- WiFi connection optimizations for faster and more reliable connections (issue #22)
  - Disabled WiFi sleep mode during wake cycles to reduce network latency by 200-400ms
  - Enabled persistent WiFi credentials for faster reconnection (2-5 seconds faster on subsequent wakes)
  - Enabled automatic WiFi reconnection for better resilience to transient network issues
  - Set device hostname for better network identification (e.g., "inkplate-A1B2C3")

### Technical Details
- logger.h: Header with LogBox class definition
- logger.cpp: Implementation with static methods for structured output
- No behavior changes - only formatting improvements to serial output
- CRC32 deferred save: see IMPLEMENTATION_NOTES.md for details

## [0.5.0] - 2025-10-17

### Added
- CRC32-based image change detection feature (issue #56)
  - Optional feature to skip image downloads when content hasn't changed
  - Downloads small `.crc32` checksum file (10 bytes) before full image
  - Compares checksum with stored value from last successful download
  - Skips image download if checksum matches, saving battery and time
  - Automatically updates stored CRC32 when image changes
  - Graceful fallback to full image download if `.crc32` file is missing or invalid
  - Configuration checkbox in web portal to enable/disable
  - Persistent storage of last CRC32 value in preferences
  - Compatible with [@jantielens/ha-screenshotter](https://github.com/jantielens/ha-screenshotter) add-on
  - Comprehensive documentation in CRC32_GUIDE.md with server setup examples
  - MQTT logging for CRC32 check results (unchanged/changed)


### Changed
- ImageManager now accepts ConfigManager reference for CRC32 storage
- Normal update flow now checks CRC32 before image download when enabled
- Documentation updated with battery life impact analysis

### Performance Impact
- Wake duration reduced from 11s to 4.5s when image unchanged
- Daily power consumption reduced by 60.7% with once-daily image changes
- Battery life increased from ~13 days to ~33 days (1200mAh battery, 5-minute refresh)
- 287 out of 288 daily wakes skip full image download (assuming once-daily changes)

## [0.4.0] - 2025-10-17

### Added
- Web-based OTA (Over-The-Air) firmware update feature (issue #6)
  - New `/ota` route accessible from configuration portal (CONFIG_MODE only)
  - File upload interface for `.bin` firmware files
  - Real-time upload progress bar (0-100%) in web interface
  - Visual feedback on e-ink display during firmware installation
  - Display shows "Firmware Update", "Installing firmware...", "Device will reboot when complete", "Do not power off!"
  - Automatic device reboot after successful firmware flash
  - Watchdog timer disabled during upload to prevent interruption
  - Warning banner with safety instructions
  - Success/error feedback with user-friendly messages
  - Mobile-friendly UI matching existing portal design
  - Full-width green button prominently placed below configuration form

### Changed
- Config mode timeout increased from 2 minutes to 5 minutes
  - Provides sufficient time for firmware uploads which can take 1-2 minutes
  - Display and serial messages now dynamically show correct timeout value
- Build system now uses "Minimal SPIFFS" partition scheme (1.9MB APP with OTA/190KB SPIFFS)
  - Previous "Huge APP" partition scheme did not support OTA updates
  - Both build.ps1 and build.sh updated with `--board-options "PartitionScheme=min_spiffs"`
  - All board configurations now include OTA partition support

### Fixed
- Display messages now consistently use MARGIN constant instead of hardcoded y=100
- Timeout messages now dynamically calculated from CONFIG_MODE_TIMEOUT_MS constant

### Technical Details
- ESP32 Update library integration for OTA flashing
- Watchdog timer management: `disableCore0WDT()` on upload start, `enableCore0WDT()` on failure/abort
- Partition size calculation using `ESP.getFreeSketchSpace()`
- Upload handler with chunked write operations
- DisplayManager integration for on-screen firmware update progress
- Proper error handling and Serial logging for debugging
- CSS styles for progress bar, warning banner, file input, and secondary button

## [0.3.1] - 2025-10-16

### Added
- Visual feedback for manual refresh button press (issue #15)
  - Display now shows "Manual Refresh" message when button is pressed for immediate update
  - Message "Button pressed - updating..." displayed for 1.5 seconds before update begins
  - Provides immediate user confirmation that button press was detected
  - Works on all devices with wake button (Inkplate 5 V2, Inkplate 10, Inkplate 6 Flick)
- USING.md user guide for end users
  - Complete setup and configuration instructions
  - Troubleshooting guide
  - FAQ section
  - Device-specific button operation details

### Improved
- Configuration portal mobile experience (issue #17)
  - Refactored CSS into separate header file (config_portal_css.h) for better maintainability
  - Input font size increased to 16px to prevent iOS auto-zoom on focus
  - Added touch-action: manipulation to all interactive elements for better mobile responsiveness
  - Checkbox size increased to 20px with custom styling for easier tapping
  - Modal centering improved with transform-based positioning instead of margin
  - Added -webkit-overflow-scrolling: touch for smooth iOS scrolling
  - Responsive padding adjustments for mobile screens (< 600px width)
  - Fixed body layout by removing flexbox centering in favor of margin-based approach
  - Added word-break for device info to prevent text overflow
  - Better touch feedback with proper hover/active state handling

## [0.3.0] - 2025-10-15

### Added
- Inkplate 6 Flick board support with dedicated configuration

## [0.2.0] - 2025-10-15

### Added
- Inkplate 2 board support with dedicated configuration
- MODES.md documentation explaining device modes and configuration behavior
- Board-specific font sizes (FONT_HEADING1, FONT_HEADING2, FONT_NORMAL) for different display sizes
- Board-specific line spacing (LINE_SPACING) for optimal text layout
- Configurable margins (MARGIN, INDENT_MARGIN) in board configurations
- NVS-based device running detection to distinguish between reset button and power-on
- Conditional button handling for boards with/without wake button capability
- Debug logging for wake/button detection and button-absent flows

### Changed
- DisplayManager now uses board-specific font sizes and layout positioning
- Common UI elements now use configurable font sizes and margins instead of hard-coded values
- PowerManager enhanced with Preferences/NVS handling and markDeviceRunning() functionality
- Wake source reconfiguration before deep sleep
- Build, upload, and setup scripts updated to include Inkplate 2
- README updated with Inkplate 2 information and example commands

### Fixed
- Text overlap issue on small displays (text lines appearing on top of each other)
- Incorrect top margin calculation causing text to not start at the actual top margin
- Version label now respects board-specific MARGIN setting

### Technical Details
- ESP reset reason detection using esp_reset_reason()
- RTC running flag set before deep sleep
- Build toolchain updated to include Inkplate 2 in include paths

## [0.1.1] - 2025-10-15

### Added
- Added Debug option in the configuration portal to enable/disable debug messages on the screen (disabled by default)
- On-screen firmware version label rendered during display refreshes
- Dev container setup for consistent local development environment
- Display cycle documentation describing normal operation flow

### Changed
- Version label now shows the full "Firmware" prefix for clarity

## [0.1.0] - 2025-10-14

### Added
- Automatic retry mechanism for image download failures
  - Up to 3 total attempts (initial + 2 retries) with 20-second delays between attempts
  - Silent retries without user feedback during retry attempts
  - Error message displayed only after all retries are exhausted
  - RTC memory persistence of retry count across deep sleep cycles
  - Treats all error types the same (network, HTTP, format errors)

### Changed
- PowerManager now supports fractional minute sleep durations via float overload
- Image download failures now sleep for 20 seconds before retry instead of full refresh rate
- Retry count automatically resets on successful download

## [0.0.1] - 2025-10-14

### Added
- Initial release of Inkplate Dashboard firmware
- WiFi configuration via Access Point mode on first boot
- Two-step onboarding flow (WiFi credentials first, then dashboard config)
- Image download and display from URL (PNG format, HTTP/HTTPS)
- Configurable refresh rate (default: 5 minutes)
- Deep sleep power management between updates
- Button-triggered config mode (long press) for reconfiguration
- Button-triggered refresh (short press) for immediate update
- MQTT integration with Home Assistant auto-discovery
- Battery voltage reporting to Home Assistant
- WiFi signal strength (RSSI) reporting to Home Assistant
- Loop time measurement and reporting
- Last log message reporting via MQTT text sensor
- Factory reset functionality
- Web-based configuration portal with responsive UI
- Support for Inkplate 5 V2 and Inkplate 10 devices
- Automated build system with Arduino CLI
- GitHub Actions CI/CD for automated builds
- Semantic versioning with version display in config portal
- Software version reporting to Home Assistant

### Technical Details
- ESP32-based firmware using Arduino framework
- PubSubClient for MQTT communication
- Inkplate Library for e-paper display control
- Preferences library for persistent configuration storage
- Shared codebase between different Inkplate devices

All notable changes to the Inkplate Dashboard project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).