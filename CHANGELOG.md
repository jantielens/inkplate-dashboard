# Changelog

All notable changes to the Inkplate Dashboard project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/jantielens/inkplate-dashboard/compare/v0.0.1...HEAD
[0.0.1]: https://github.com/jantielens/inkplate-dashboard/releases/tag/v0.0.1
