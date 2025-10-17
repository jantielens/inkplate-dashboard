# Inkplate Dashboard

A multi-board dashboard firmware for Inkplate e-ink displays that periodically downloads and displays images from a web URL.

## Supported Devices

- ✅ **Inkplate 2** (212x104, 1-bit black & white) - _No physical button_
- ✅ **Inkplate 5 V2** (1280x720, 3-bit grayscale)
- ✅ **Inkplate 10** (1200x825, 1-bit black & white)
- ✅ **Inkplate 6 Flick** (1024x758, 3-bit grayscale)

## Features

- 📱 **Easy Setup**: WiFi configuration via captive portal on first boot
- 🖼️ **Image Display**: Downloads PNG images from any public URL (HTTP/HTTPS)
- ⚡ **Power Efficient**: Deep sleep between updates to maximize battery life
- 🔄 **Configurable Refresh**: Set update interval (default: 5 minutes)
- 🌐 **Web Interface**: Beautiful, responsive configuration portal
- 💾 **Persistent Config**: Settings saved to device flash memory
- 🔧 **Button Config Mode**: Long press button to reconfigure settings anytime (2-minute timeout)
- ⚡ **Manual Refresh**: Short press button for immediate image update
- 🗑️ **Factory Reset**: Web-based factory reset to erase all settings and start fresh
- ⬆️ **OTA Updates**: Upload new firmware directly via web interface
- 🏠 **Home Assistant Integration**: Optional MQTT battery voltage reporting with auto-discovery

## Quick Start


### 1. First Time Setup

```powershell
# Install Arduino CLI and dependencies
.\setup.ps1

# Build firmware for your device
\.\build.ps1 inkplate2        # or inkplate5v2, inkplate10, inkplate6flick

# Upload to device (replace COM7 with your port)
\.\upload.ps1 -board inkplate2 -port COM7
\.\upload.ps1 -board inkplate6flick -port COM7
```

### 2. Two-Step Onboarding Flow

**Step 1: WiFi Setup (Boot Mode)**
1. Device boots and creates WiFi access point:
  - Network: `inkplate-dashb-XXXXXX`
  - No password required
2. Connect to the Inkplate WiFi network
3. Open browser to `http://192.168.4.1`
4. Enter **only your WiFi network name and password** (no image URL or refresh rate yet)
5. Save – device restarts and connects to your WiFi

**Step 2: Dashboard Configuration (Auto Config Mode)**
1. Device detects WiFi is configured but no image URL
2. Automatically enters config mode (shows device IP on screen)
3. Connect to your WiFi network
4. Open browser to the device IP (shown on screen)
5. Enter **image URL** (PNG) and **refresh rate**
6. Save – configuration is complete, device starts normal operation

**Subsequent Configuration (Button-Triggered Config Mode)**
- Long press button (2.5+ seconds) to enter config mode at any time
- Device connects to WiFi and shows config page URL
- All settings (WiFi, image URL, refresh rate, factory reset) can be updated
- Config mode times out after 2 minutes

### 3. Normal Operation

The device will:
1. Connect to your WiFi network
2. Download the image from your URL (HTTP/HTTPS supported)
3. Display it on the e-ink screen
4. Enter deep sleep for the configured refresh interval
5. Wake up automatically and repeat

**Button Controls:**

**Short Press** (quick tap): Immediately refresh the display
  - Wakes device from sleep
  - Downloads and displays latest image
  - Returns to normal sleep schedule

**Long Press** (hold 2.5+ seconds): Enter Config Mode
  - Device connects to WiFi and shows configuration URL on screen
  - Access web interface for 2 minutes to update settings (all fields available)
  - Device automatically restarts or returns to sleep after timeout

> **Note for Inkplate 2**: This device does not have a physical button. Configuration must be done during the initial setup or by performing a factory reset through the web interface while in config mode.

## Project Structure

```
inkplate-dashboard-new/
├── boards/              # Board-specific sketches
│   ├── inkplate2/      # Inkplate 2 specific code
│   ├── inkplate5v2/    # Inkplate 5 V2 specific code
│   └── inkplate10/     # Inkplate 10 specific code
├── common/             # Shared code library
│   ├── src/           # Source files
│   └── examples/      # Documentation & examples
├── build/             # Build artifacts (generated)
├── build.ps1          # Build script
├── upload.ps1         # Upload script
└── setup.ps1          # Setup script
```

## Build System

This project uses a custom PowerShell build system that compiles Arduino sketches while sharing 95% of code between boards.

**Key Commands:**
```powershell
.\build.ps1 all                              # Build all boards
.\build.ps1 inkplate5v2                      # Build specific board
.\upload.ps1 -board inkplate5v2 -port COM7   # Upload to device
```

**Documentation:**
- 📘 [Build System Guide](BUILD_SYSTEM.md) - Detailed explanation
- 📝 [Quick Reference](BUILD_QUICK_REFERENCE.md) - Cheat sheet

## Development Status

### ✅ Completed Phases

- **Phase 1**: Project Setup & Core Structure
- **Phase 2**: Configuration Storage & Retrieval
- **Phase 3**: WiFi Access Point & Configuration Portal
- **Phase 4**: WiFi Client Connection
- **Phase 5**: Image Download & Display (HTTP/HTTPS)
- **Phase 6**: Deep Sleep & Power Management
- **Phase 7**: Refresh by Button Press (Short vs Long Press)

### 🚧 In Progress / Upcoming

- **Phase 8**: Testing & Refinement
- **Phase 9**: CI/CD Setup

See [plan.md](plan.md) for detailed implementation roadmap.

## Image Requirements

- **Format**: PNG only
- **Resolution**: Must match your screen:
  - Inkplate 5 V2: 1280x720 pixels
  - Inkplate 10: 1200x825 pixels
- **Source**: Public web server (HTTP/HTTPS)
- **Processing**: No resizing or rotation (provide correct dimensions)

## Configuration

### WiFi Credentials
- Stored securely in ESP32 NVS (Non-Volatile Storage)
- Encrypted by ESP32 hardware


### Settings
- **Boot Mode (First Boot):**
  - WiFi SSID: Your network name (required)
  - WiFi Password: Your network password (optional)
  - No image URL or refresh rate fields

- **Config Mode (Auto or Button-Triggered):**
  - WiFi SSID: Your network name (required)
  - WiFi Password: Your network password (optional)
  - Image URL: Full URL to PNG image (required)
  - Refresh Rate: Minutes between updates (default: 5, minimum: 1)
  - MQTT Broker: Optional Home Assistant MQTT broker URL (format: `mqtt://host:port`)
  - MQTT Username: Optional MQTT authentication username
  - MQTT Password: Optional MQTT authentication password
  - Factory Reset button (button-triggered config mode only)

### Factory Reset
You can reset your device to factory defaults through the web interface:

1. **Long press the button** (2.5+ seconds) to enter Config Mode
2. **Open the configuration page** in your browser
3. **Scroll to the bottom** to the "Danger Zone" section
4. **Click "Factory Reset"** and confirm
5. **Device erases all settings** and reboots into setup mode

This will permanently erase all configuration including WiFi credentials. The device will create a new access point for fresh setup.

## Home Assistant Integration (Optional)

The device can optionally report battery voltage to Home Assistant using MQTT with auto-discovery.

### Setup

1. **Configure MQTT Broker** (in Config Mode):
   - MQTT Broker URL: `mqtt://your-broker-ip:1883`
   - MQTT Username: Your broker username (optional)
   - MQTT Password: Your broker password (optional)

2. **Automatic Discovery**:
   - Device appears in Home Assistant as "Inkplate Dashboard XXXXXX"
   - Battery voltage sensor is auto-discovered
   - No manual YAML configuration needed

### What Gets Reported

- **Battery Voltage**: Published every time device wakes up (timer or button press)
- **Device Information**: Board type, manufacturer, unique device ID
- **Sensor Details**: Voltage in Volts with 3 decimal precision

### MQTT Topics

- Discovery: `homeassistant/sensor/inkplate-XXXXXX/battery_voltage/config`
- State: `homeassistant/sensor/inkplate-XXXXXX/battery_voltage/state`

### Behavior

- Publishing happens during normal updates (timer wake and short button press)
- Does NOT publish during config mode
- Gracefully continues if MQTT broker is unreachable
- Skips MQTT if broker URL is not configured

## Architecture

### Code Organization

**Board-Specific Files** (`boards/{board}/`):
- `{board}.ino` - Entry point (~10 lines, just includes)
- `board_config.h` - Board-specific constants

**Shared Code** (`common/src/`):
- `main_sketch.ino.inc` - Shared setup() and loop()
- `config_manager.*` - Configuration storage
- `wifi_manager.*` - WiFi AP and client modes
- `config_portal.*` - Web configuration interface
- `display_manager.*` - Display abstraction
- `image_manager.*` - Image download and display
- `power_manager.*` - Deep sleep and wake management
- `utils.*` - Utility functions

### Why This Structure?

- ✅ **DRY**: Write shared code once
- ✅ **Maintainable**: Single source of truth
- ✅ **Scalable**: Easy to add new boards
- ✅ **Testable**: Isolated components

See [BUILD_SYSTEM.md](BUILD_SYSTEM.md) for complete architecture explanation.

## Requirements

### Hardware
- Inkplate 5 V2 or Inkplate 10 device
- USB cable for programming
- WiFi network

### Software
- Windows PowerShell (for build scripts)
- Arduino CLI
- Inkplate board package

All installed automatically by `setup.ps1`.

### Local build prerequisites (Linux/macOS)

When running `./build.sh` outside of the PowerShell setup flow, make sure these manual steps are completed first:

1. Install Arduino CLI and put it on your `PATH`:
  ```bash
  cd /tmp
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh -o install-arduino-cli.sh
  sudo BINDIR=/usr/local/bin sh install-arduino-cli.sh
  ```
2. Register the Inkplate board package index:
  ```bash
  arduino-cli core update-index \
    --additional-urls https://raw.githubusercontent.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE/master/package_Dasduino_Boards_index.json
  ```
3. Install the Inkplate ESP32 core (includes toolchain and flashing utilities):
  ```bash
  arduino-cli core install Inkplate_Boards:esp32 \
    --additional-urls https://raw.githubusercontent.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE/master/package_Dasduino_Boards_index.json
  ```
4. Install required Arduino libraries:
  ```bash
  arduino-cli lib install "InkplateLibrary"
  arduino-cli lib install "PubSubClient"
  ```
5. Install the Python serial dependency used by `esptool.py` during compilation:
  ```bash
  python3 -m pip install --user pyserial
  ```

After these prerequisites are in place you can call `./build.sh` and the script will compile both Inkplate targets.

> ℹ️ GitHub Codespaces automatically runs these steps via `.devcontainer/postCreate.sh`, so builds work out of the box when using the provided dev container.

## Documentation

**📚 [Documentation Index](DOCS_INDEX.md) - Complete guide to all documentation**

**Quick Links:**
- 📘 [Build System Guide](BUILD_SYSTEM.md) - Complete build system explanation
- 📝 [Quick Reference](BUILD_QUICK_REFERENCE.md) - Commands and diagrams
- 🔍 [Build Examples](BUILD_EXAMPLES.md) - Step-by-step walkthroughs
- 🔧 [Configuration Portal Guide](common/examples/config_portal_guide.md)
- 💾 [ConfigManager Usage](common/examples/config_test.md)
- 📋 [Implementation Plan](plan.md)

## Troubleshooting

### Can't See WiFi Network
- Wait 30 seconds after device boot
- Check device display for exact network name
- Ensure device is powered on

### Can't Access Configuration Page
- Verify you're connected to Inkplate WiFi
- Try `http://192.168.4.1` (not https)
- Disable mobile data on phone

### WiFi Connection Failures
- Check WiFi credentials are correct (case-sensitive)
- Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Press button to enter Config Mode and update settings
- Device will retry on next wake cycle (configured refresh interval)

### Image Download Failures
- Verify image URL is accessible from your network
- Check image is PNG format with correct resolution
- Ensure URL is complete (include http:// or https://)
- For HTTPS, certificate validation is disabled for simplicity
- Device will retry on next wake cycle (configured refresh interval)
- Check Serial monitor for debugging information

### Build Failures
- Run `.\setup.ps1` to ensure dependencies installed
- Check Serial monitor for detailed error messages
- See [BUILD_SYSTEM.md](BUILD_SYSTEM.md) troubleshooting section

### Upload Failures
- Verify correct COM port
- Try different USB cable
- Press reset button on device before upload

### Config Mode (Button) Not Working
- Verify button is connected to GPIO 36
- Press button briefly (single press, not long hold)
- Device must be configured first (won't work on first boot)
- Check Serial monitor to verify wake reason detection
- Look for "Config Mode Active" message on screen

### Device Sleeping Immediately
- This is normal behavior after successful image display
- Device wakes automatically based on configured refresh rate
- Press button to enter Config Mode if changes needed
- First boot creates AP and doesn't sleep (waiting for configuration)

## Serial Debugging

Connect at 115200 baud to see detailed logs:
```powershell
# Windows
arduino-cli monitor -p COM7 -c baudrate=115200

# Linux/Mac
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Contributing

When adding features:
1. Add shared code to `common/src/`
2. Use board-specific constants from `board_config.h`
3. Test on all boards: `.\build.ps1 all`
4. Update documentation
5. Increment version in `common/src/version.h`
6. Update `CHANGELOG.md` with your changes

## Versioning & Releases

This project follows [Semantic Versioning](https://semver.org/):

- **MAJOR** (e.g., `1.0.0`): Breaking changes (config format changes requiring factory reset)
- **MINOR** (e.g., `0.1.0`): New features (backward compatible)
- **PATCH** (e.g., `0.0.1`): Bug fixes (backward compatible)

### Making a Release

1. **Update version** in `common/src/version.h`:
   ```cpp
   #define FIRMWARE_VERSION "0.0.2"
   ```

2. **Update CHANGELOG.md** with your changes:
   ```markdown
   ## [0.0.2] - 2025-10-15
   ### Added
   - New feature description
   ### Fixed
   - Bug fix description
   ```

3. **Commit and push** your changes to `main`:
   ```bash
   git add .
   git commit -m "Release v0.0.2"
   git push origin main
   ```

4. **Create and push a tag**:
   ```bash
   git tag v0.0.2
   git push origin v0.0.2
   ```

5. **Automated release**:
   - GitHub Actions automatically builds both board variants
   - Creates a GitHub Release with CHANGELOG excerpt
   - Attaches firmware binaries:
     - `inkplate5v2-v0.0.2.bin`
     - `inkplate10-v0.0.2.bin`

### Version Display

The firmware version is displayed:
- In the configuration portal footer
- Reported to Home Assistant via MQTT (device software version)
- In firmware binary filenames

### Pull Request Workflow

When you create a PR:
- **Version check workflow** runs automatically
- Compares version in your PR vs. main branch
- **Warns (but doesn't block)** if version unchanged
- Helpful reminder to increment version for code changes

See [CHANGELOG.md](CHANGELOG.md) for version history.

## License

[Add your license here]

## Credits

- Built with [Arduino CLI](https://arduino.github.io/arduino-cli/)
- Uses [Inkplate Library](https://github.com/SolderedElectronics/Inkplate-Arduino-library)
- For [Inkplate Devices](https://www.soldered.com/categories/inkplate/) by Soldered Electronics

## Support

- 📖 Documentation: See `docs/` directory
- 🐛 Issues: [Create an issue](your-repo/issues)
- 💬 Discussions: [Start a discussion](your-repo/discussions)
