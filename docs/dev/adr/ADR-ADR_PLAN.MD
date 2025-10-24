# Inkplate Dashboard

The InkPlate Dashboard is a project that allow InkPlate devices to periodically download an image and display the image on the screen. For now it supports InkPlate 5 gen 2 and InkPlate 10, in the future it will support more devices.

## First usage
When the Inkplate Dashboard firmware is flashed on a device, it should create a wi-fi access point. It should serve a basic web page in which the users can enter:
- wi-fi credentials
- the URL of the image to download
- the refresh rate (in minutes, default: 5 minutes)

**Access Point Details:**
- AP Name: "inkplate-dashb-XXXXXXX" (where XXXXXXX is a unique device identifier)
- Security: Open network (no password)
- No reset functionality needed for now

Once the user submits the form, the device should connect to the specified wi-fi network and start downloading the image from the specified URL at the specified refresh rate.

The settings are saved on the board's memory using the Preferences library.

## Normal operation
Once the device is configured, it should connect to the specified wi-fi network and start downloading the image from the specified URL at the specified refresh rate. After the image is displayed, the device should go to deep sleep for the specified refresh rate.

If the device fails to connect to the wifi network, or if it fails to download the image, it should display a meaningful error message on the screen and go to deep sleep (for the specified refresh rate). So next time it boots, it will try again to connect to the wifi network and download the image.

**Image Specifications:**
- Format: PNG or JPEG (baseline DCT with Huffman coding only - progressive JPEG not supported)
- Resolution: Must match screen resolution (provided correctly, no resizing needed)
- Processing: No rotation or other image processing
- Source: Public web server (HTTP/HTTPS, no authentication)
- Size: No file size limits

**Error Handling:**
- Display meaningful error messages on screen (WiFi connection failed, image download failed, invalid image format, etc.)
- No error logging
- Retry on next wake cycle

## Implementation details

- Use Arduino CLI as build platform
- Share as much code as possible between the different devices
- Keep the structure of the project as simple as possible
- No OTA updates for now
- No battery monitoring for now

## Implementation steps

### Phase 1: Project Setup & Core Structure
✅ DONE

### Phase 2: Configuration Storage & Retrieval
4. [✅] Implement configuration storage using Preferences library
5. [✅] Create configuration data structure (WiFi SSID, password, image URL, refresh rate)
6. [✅] Implement functions to save and load configuration
7. [✅] Add first-boot detection mechanism

### Phase 3: WiFi Access Point & Configuration Portal
8. [✅] Implement WiFi AP mode with device-specific name (inkplate-dashb-XXXXXXX)
9. [✅] Create HTML configuration page (responsive, simple form)
10. [✅] Implement web server for configuration portal
11. [✅] Handle form submission and save configuration
12. [✅] Add success page and automatic reboot after configuration

### Phase 4: WiFi Client Connection
13. [✅] Implement WiFi client mode connection with stored credentials
14. [✅] Add connection timeout and error handling
15. [✅] Display connection status/errors on screen
15a. [OBSOLETE - See Phase 6] ~~Keep exposing the web server so the user can update the WiFi credentials if needed, image URL and refresh rate~~ (Replaced by button-triggered config mode)

### Phase 5: Image Download & Display
16. [✅] Implement HTTP/HTTPS client for PNG image download
17. [SKIP] Add image validation (PNG format, basic checks)
18. [✅] Display image on InkPlate screen, use the drawImage function from the InkPlate library
19. [✅] Add error handling for download failures
20. [✅] Display meaningful error messages on screen for different failure scenarios

### Phase 6: Deep Sleep & Power Management
21. [✅] Implement deep sleep functionality with dual wake sources (timer + button)
22. [✅] Calculate sleep duration based on refresh rate (convert minutes to microseconds)
23. [✅] Configure button (GPIO_NUM_36 for inkplate5v2) as ext0 wake source
24. [✅] Detect wake cause on boot (timer vs button press)
25. [✅] **Config Mode (Button Triggered):**
    - Display "Config Mode" message on screen with instructions
    - Connect to configured WiFi (client mode, not AP)
    - Start web server for 2 minutes (hardcoded timeout)
    - Allow configuration updates via web interface
    - Return to deep sleep after timeout or configuration update
27. [✅] **Normal Mode (Timer Wake):**
    - No web server running
    - Connect to WiFi, download & display image
    - Go to deep sleep immediately after display
28. [✅] Ensure proper shutdown sequence before sleep (WiFi disconnect, display sleep)

### Phase 7: Refresh by button press
**Implementation Note:** ESP32 ext0 wake source triggers on signal level, not press duration. Detection must happen after wake, not during sleep.

24. [✅] Implement button hold detection after wake:
    - On button wake, immediately check if button is still pressed (GPIO LOW)
    - If button is held, wait 2-3 seconds and check again
    - If still held after timeout → Enter Config Mode (existing behavior)
    - If released (short press) → Trigger Normal Update
25. [✅] Route short button press to normal update cycle:
    - Call existing `performNormalUpdate()` function
    - Connect to WiFi, download & display image
    - Return to deep sleep with timer wake source
26. [✅] Update PowerManager to return button press type (short vs long)
27. [✅] Update main_sketch.ino.inc to handle both button press types appropriately


### Phase 8: Testing & Refinement
27. [ ] Test first-time configuration flow
28. [ ] Test normal operation cycle (download, display, sleep)
29. [ ] Test error scenarios (WiFi failure, download failure, invalid image)
30. [ ] Test on both InkPlate 5 Gen 2 and InkPlate 10 devices
31. [ ] Optimize memory usage and performance
32. [ ] Document build and flash instructions

### Phase 9: CI/CD Setup
33. [✅] Set up GitHub Actions workflow for automated builds
34. [✅] Configure build matrix for both device types
35. [✅] Add firmware artifact uploads
36. [✅] Add build caching for faster CI runs

### Phase 10: Versioning & Release Management
37. [✅] Add semantic versioning to firmware (MAJOR.MINOR.PATCH)
38. [✅] Store version in dedicated version.h file
39. [✅] Display version on config portal footer
40. [✅] Report version to Home Assistant via MQTT discovery
41. [✅] Create CHANGELOG.md with versioning guidelines
42. [✅] Add GitHub Release workflow to create releases from tags
43. [✅] Include firmware binaries in GitHub Releases automatically
44. [✅] Add version check workflow for PRs (warns if version not incremented)
45. [✅] Update build scripts to include version in binary filenames
46. [✅] Update README.md with versioning and release process documentation

## Feature: rework configuration UI

### Overview
Currently we have a UI that's being displayed during first boot (boot mode, when no wifi info is available) and during config mode. Currently both boot mode and config mode UI's have the same fields. We should rework the UI to have different fields for boot mode and config mode to create a two-step onboarding flow.

### UI Field Requirements

**Boot Mode (First Boot - AP Mode):**
- WiFi SSID (required)
- WiFi Password (optional)
- No Image URL or refresh rate fields
- No factory reset button

**Config Mode (Button-Triggered or Auto-Entered):**
- WiFi SSID (required)
- WiFi Password (optional)
- Image URL (required)
- Refresh rate (optional, default 5 minutes)
- Factory reset button

### User Flow

1. **First Boot (Boot Mode - AP Mode):**
   - User connects to device AP
   - Enters only WiFi credentials
   - Device saves WiFi credentials and reboots

2. **After First Boot:**
   - Device detects WiFi configured but no Image URL
   - Automatically enters Config Mode (connected to WiFi, not AP)
   - User connects to their WiFi network
   - Opens browser to device IP
   - Enters Image URL and refresh rate
   - Configuration is complete

3. **Subsequent Config Mode (Button-Triggered):**
   - User long-presses button
   - Device enters Config Mode with full form
   - Can update any settings including factory reset

### Implementation Tasks

- [✅] Add mode parameter to ConfigPortal (BOOT_MODE vs CONFIG_MODE)
- [✅] Update generateConfigPage() to conditionally show fields based on mode
- [✅] Modify handleSubmit() validation to allow submission without Image URL in boot mode
- [✅] Add ConfigManager methods to distinguish "has WiFi" vs "fully configured"
- [✅] Update main_sketch.ino.inc to detect partial configuration and auto-enter config mode
- [✅] Update enterAPMode() to pass BOOT_MODE to portal
- [✅] Update enterConfigMode() to pass CONFIG_MODE to portal

## Feature: Report battery voltage to Home Assistant
The device should calculate battery voltage and report it to Home Assistant using MQTT. Every time the device wakes up (timer wake or button-triggered normal update), it should publish the battery voltage to MQTT topics using Home Assistant auto-discovery.

The Inkplate should appear as a single device entity with the battery voltage as a sensor. This allows additional sensors to be added in the future.

### Battery Voltage Reading
**Hardware Configuration:**
- ADC Pin: GPIO 35 (both Inkplate 5 V2 and Inkplate 10)
- Attenuation: 12dB
- Voltage Divider: 1:2 ratio (multiply ADC reading by 2)
- MOSFET Control: Enable battery reading MOSFET before sampling, disable after to save power

**Reading Sequence:**
1. Enable battery read MOSFET
2. Wait 5ms for voltage to stabilize
3. Sample ADC value from GPIO 35
4. Disable battery read MOSFET
5. Apply voltage divider compensation (multiply by 2)

### MQTT Configuration
**Configuration Parameters (added to Config Mode):**
- MQTT Broker URL (format: `mqtt://broker.example.com:1883`)
- MQTT Username (optional)
- MQTT Password (optional)

**Storage:** Add to existing Preferences namespace "dashboard"

### Home Assistant Auto-Discovery
**Device Information:**
- Device Name: "Inkplate Dashboard XXXXXX" (where XXXXXX is device ID)
- Manufacturer: "Soldered Electronics"
- Model: Board type ("Inkplate 5 V2" or "Inkplate 10")
- Identifiers: Device unique ID

**Sensor Configuration:**
- Sensor Type: Voltage only
- Unit: Volts (V)
- Precision: 3 decimal places
- Device Class: `voltage`

**MQTT Topics:**
- Discovery: `homeassistant/sensor/inkplate-XXXXXX/battery_voltage/config`
- State: `homeassistant/sensor/inkplate-XXXXXX/battery_voltage/state`

### Publishing Behavior
- Publish on every wake-up (timer wake and normal button-triggered updates)
- Do NOT publish during config mode
- Silently fail if MQTT broker is unreachable (continue normal operation)
- Skip MQTT if broker URL is not configured

### Implementation Steps
- [✅] Add MQTT configuration fields to `DashboardConfig` struct (broker URL, username, password)
- [✅] Update `ConfigManager` to store/retrieve MQTT settings
- [✅] Add MQTT fields to Config Mode web UI
- [✅] Install PubSubClient library dependency
- [✅] Create `MQTTManager` class with methods:
  - `begin()` - Initialize MQTT client
  - `connect()` - Connect to broker with credentials
  - `publishBatteryVoltage()` - Read and publish battery voltage
  - `publishDiscovery()` - Send Home Assistant auto-discovery config
  - `disconnect()` - Clean disconnect
- [✅] Add battery voltage reading function (using Inkplate library's readBattery method)
- [✅] Integrate MQTT publishing into `performNormalUpdate()` in main_sketch.ino.inc
- [✅] Define board-specific battery pins in board_config.h
- [✅] Test MQTT publishing with and without Home Assistant
- [✅] Test error handling (unreachable broker, invalid credentials)

## Feature: reduce log messages on screen during normal operation
During normal operation (timer wake or button-triggered normal update), the device should minimize log messages displayed on the screen. Only critical errors should be shown. Informational messages should be suppressed to provide a cleaner user experience.

Add a new configuration option to enable/disable loggin on the screen during normal operation. This option should be available in Config Mode (defaults to off).

### Implementation Steps
- [ ] ...
- [ ] ...

## Feature: additional values to report to Home Assistant
In addition to battery voltage, the device should report the following values to Home Assistant using MQTT:

### Loop time
The device should measure the time taken to complete the normal update cycle (from wake to going back to sleep) and report this time to Home Assistant using MQTT as a **sensor entity**. This allows users to monitor the performance of their device.

**Sensor Configuration:**
- Device Class: `duration`
- Unit: seconds (s)
- Precision: 2 decimal places
- MQTT Topics:
  - Discovery: `homeassistant/sensor/inkplate-XXXXXX/loop_time/config`
  - State: `homeassistant/sensor/inkplate-XXXXXX/loop_time/state`

### WiFi signal strength
The device should measure the WiFi signal strength (RSSI) when it connects to the WiFi network and report this value to Home Assistant using MQTT as a **sensor entity**. This allows users to monitor the quality of their WiFi connection.

**Sensor Configuration:**
- Device Class: `signal_strength`
- Unit: dBm
- Precision: 0 decimal places (integer)
- MQTT Topics:
  - Discovery: `homeassistant/sensor/inkplate-XXXXXX/wifi_signal/config`
  - State: `homeassistant/sensor/inkplate-XXXXXX/wifi_signal/state`

### Log messages
The device should send log messages to Home Assistant using MQTT as a **text sensor entity** that stores the last log message. This allows users to monitor the status of their device and create automations or display the messages in the UI. Only log messages that are either outside of the normal operation (e.g. during config mode) or error messages during normal operation should be sent to Home Assistant. Informational messages during normal operation should not be sent.

**Implementation Note:** MQTT Events do not automatically create Home Assistant logbook entries - they are designed for button presses. For log messages, a simple text sensor is easier for users.

**Sensor Configuration:**
- Sensor Type: Text sensor
- Discovery Topic: `homeassistant/sensor/inkplate-XXXXXX/last_log/config`
- State Topic: `homeassistant/sensor/inkplate-XXXXXX/last_log/state`
- Payload format: Plain text with severity prefix (e.g., "[INFO] Config mode entered")
- Icon: `mdi:message-text`

**Messages to Send:**
- **Config Mode:**
  - Config mode entered (info)
  - Config mode timeout (info)
  - Settings updated (info)
- **Error Messages (Normal Operation):**
  - WiFi connection failed (error)
  - Image download failed (error)
  - MQTT connection failed (warning)
- **Do NOT Send:**
  - Normal timer wake (skip)
  - Successful image download (skip)
  - Going to sleep (skip)
  - Other routine informational messages (skip)

### Implementation Steps
- [✅] Add `publishLoopTime()` method to MQTTManager
- [✅] Add `publishWiFiSignal()` method to MQTTManager
- [✅] Add `publishLogEvent()` method to MQTTManager with severity parameter
- [✅] Update `publishDiscovery()` to include loop_time and wifi_signal sensor configs
- [✅] Add loop time measurement in `performNormalUpdate()`:
  - Record `startTime = millis()` at the beginning
  - Calculate duration before `enterDeepSleep()`
  - Publish loop time to MQTT
- [✅] Add WiFi signal measurement after `wifiManager.connectToWiFi()`:
  - Call `WiFi.RSSI()` to get signal strength
  - Publish RSSI to MQTT
- [✅] Integrate log event publishing at appropriate points:
  - `enterConfigMode()` - "Config mode entered"
  - Config timeout in `loop()` - "Config mode timeout"
  - Config received - "Settings updated"
  - WiFi connection failure - "WiFi connection failed: [SSID]"
  - Image download failure - "Image download failed: [error]"
  - MQTT connection failure - "MQTT connection failed" (warning, not error)
- [x] Test all three new sensors/events appear correctly in Home Assistant
- [x] Verify events appear in Home Assistant Logbook with correct device attribution

## Feature: version numbering and changelog
The firmware should use semantic versioning (MAJOR.MINOR.PATCH) to track changes and releases. The version number should be stored in a dedicated `version.h` file and included in the firmware build. The version should be displayed on the configuration portal page footer.
The version should also be reported to Home Assistant using MQTT as part of the device information during auto-discovery.
A `CHANGELOG.md` file should be maintained to document changes for each version, following the "Keep a Changelog" format.
The generated firmwares should be tagged with the version number in the GitHub repository. (and their filenames should include the version number)
A workflow should be created to check if the version is increased when a pr is merged into the main branch.

### Versioning Guidelines
Follow semantic versioning standards:
- **MAJOR**: Breaking changes (e.g., config format changes requiring factory reset, incompatible API changes)
- **MINOR**: New features (backward compatible additions)
- **PATCH**: Bug fixes (backward compatible fixes)

### Technical Decisions
- **Version file location**: `common/src/version.h`
- **MQTT field**: Add `"sw_version":"0.0.1"` to device object in Home Assistant discovery
- **Config portal display**: Footer on all portal pages
- **Firmware naming**: `inkplate5v2-v0.0.1.bin` and `inkplate10-v0.0.1.bin`
- **GitHub releases**: Automatic on tag push (e.g., `v0.0.1`), includes both board binaries and CHANGELOG excerpt
- **Version check workflow**: Warns on PR if version not incremented (does not block merge)
- **Initial version**: `0.0.1`

### Implementation Steps
- [✅] Create `common/src/version.h` with version constants (FIRMWARE_VERSION, VERSION_MAJOR/MINOR/PATCH)
- [✅] Update `mqtt_manager.cpp` `publishDiscovery()` to include `sw_version` in device object
- [✅] Update `config_portal.cpp` `generateConfigPage()` to display version in footer on all pages
- [✅] Modify `build.sh` and `build.ps1` to rename output binaries with version number
- [✅] Create `CHANGELOG.md` following "Keep a Changelog" format with initial v0.0.1 entry
- [✅] Create `.github/workflows/release.yml` to:
  - Trigger on version tags (e.g., `v*.*.*`)
  - Build both board variants (inkplate5v2 and inkplate10)
  - Create GitHub Release with CHANGELOG excerpt for that version
  - Upload both firmware binaries as release assets with versioned filenames
- [✅] Create `.github/workflows/version-check.yml` to:
  - Run on PRs to main branch
  - Parse version.h from base and head branches
  - Verify version was incremented
  - Add warning comment to PR if version unchanged (does not block merge)
- [✅] Update README.md with versioning information and release process