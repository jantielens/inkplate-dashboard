# Changelog

## [Unreleased]

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

[Unreleased]: https://github.com/jantielens/inkplate-dashboard/compare/v0.3.1...HEAD
[0.3.1]: https://github.com/jantielens/inkplate-dashboard/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/jantielens/inkplate-dashboard/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/jantielens/inkplate-dashboard/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/jantielens/inkplate-dashboard/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/jantielens/inkplate-dashboard/compare/v0.0.1...v0.1.0
[0.0.1]: https://github.com/jantielens/inkplate-dashboard/releases/tag/v0.0.1

All notable changes to the Inkplate Dashboard project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).