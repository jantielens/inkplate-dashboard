# Changelog

## [Unreleased]

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