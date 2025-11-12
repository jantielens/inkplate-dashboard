# Configuration Portal Guide

## Overview

The Inkplate Dashboard configuration portal allows users to set up their device on first boot through a simple web interface.

## How It Works

### First Boot Sequence

1. **Device boots for the first time** (no configuration stored)
2. **Access Point starts automatically**:
   - Network Name: `inkplate-dashb-XXXXXX` (last 6 chars of MAC address)
   - Security: Open (no password required)
   - IP Address: `192.168.4.1` (default AP IP)

3. **Web server starts** on port 80
4. **Display shows instructions**:
   ```
   Configuration Mode
   1. Connect to WiFi: inkplate-dashb-XXXXXX
   2. Open browser to: http://192.168.4.1
   3. Enter your settings
   ```

### User Configuration Steps

1. **Connect to the Inkplate WiFi network** using phone/laptop
2. **Open web browser** and navigate to `http://192.168.4.1`
3. **Fill out the configuration form**:
   - WiFi Network Name (SSID) - Required
   - WiFi Password - Optional (leave empty for open networks)
   - Device Name - Optional (custom friendly name for MQTT/Home Assistant)
   - Dashboard Images - Required (at least 1 image URL with display interval)
   - Additional images - Optional (add up to 10 images total for carousel mode)

4. **Click "Save Configuration"**
5. **Device saves settings and restarts**
6. **Device connects to configured WiFi** and enters normal operation

## Configuration Form Fields

### WiFi Network Name (SSID)
- **Required**: Yes
- **Type**: Text
- **Description**: The name of your WiFi network
- **Example**: `MyHomeWiFi`

### WiFi Password
- **Required**: No
- **Type**: Password
- **Description**: Password for your WiFi network (leave empty if open network)
- **Security**: Stored securely in ESP32 NVS

### Device Name (Friendly Name)
- **Required**: No
- **Type**: Text (max 24 characters)
- **Description**: Optional custom name for your device
- **Display behavior**: 
  - Your input is used as-is for the Home Assistant device name (preserves spaces and capitalization)
  - A sanitized version is used for MQTT topics, entity IDs, and hostname
- **Validation**: Automatically sanitized to: lowercase a-z, digits 0-9, hyphens (-)
- **Real-time feedback**: Preview shows sanitized result as you type
- **Default**: If empty, uses `Inkplate Dashboard <MAC>` format (e.g., `Inkplate Dashboard a886e694`)
- **Examples**: 
  - Input: `Bedroom Lis` → Device: "Bedroom Lis", Entities: `sensor.bedroomlis_*`
  - Input: `living-room` → Device: "living-room", Entities: `sensor.living-room_*`
- **⚠️ Warning**: Changing this creates a new device in Home Assistant (old entities stop updating)

### Network Configuration
- **Required**: No
- **Default**: DHCP (Automatic)
- **Type**: Radio selection (DHCP / Static IP)
- **Description**: Choose between automatic IP assignment or manual static IP configuration
- **Performance**: Static IP reduces wake time by 0.5-2 seconds per cycle

#### DHCP Mode (Automatic)
- **Default**: Yes
- **Description**: Router automatically assigns IP address
- **Use when**: Simple setup, device moves between networks, or no specific IP requirements

#### Static IP Mode (Manual)
- **Description**: Manually configure fixed IP address for faster, more consistent connections
- **Use when**: Battery-powered operation, frequent updates, or predictable IP address needed
- **All fields required** when Static IP is selected

##### IP Address
- **Required**: Yes (if Static IP selected)
- **Type**: IPv4 address
- **Format**: `xxx.xxx.xxx.xxx`
- **Validation**: Each octet must be 0-255
- **Example**: `192.168.1.100`
- **Note**: Must be outside router's DHCP range to avoid conflicts

##### Gateway
- **Required**: Yes (if Static IP selected)
- **Type**: IPv4 address
- **Format**: `xxx.xxx.xxx.xxx`
- **Validation**: Each octet must be 0-255
- **Example**: `192.168.1.1`
- **Description**: Your router's IP address
- **Typical values**: `192.168.1.1`, `192.168.0.1`, `10.0.0.1`

##### Subnet Mask
- **Required**: Yes (if Static IP selected)
- **Type**: IPv4 address
- **Format**: `xxx.xxx.xxx.xxx`
- **Validation**: Each octet must be 0-255
- **Default**: `255.255.255.0`
- **Example**: `255.255.255.0`
- **Description**: Network subnet mask
- **Note**: `255.255.255.0` is typical for home networks

##### Primary DNS
- **Required**: Yes (if Static IP selected)
- **Type**: IPv4 address
- **Format**: `xxx.xxx.xxx.xxx`
- **Validation**: Each octet must be 0-255
- **Default**: `8.8.8.8` (Google DNS)
- **Example**: `8.8.8.8`
- **Alternatives**: `1.1.1.1` (Cloudflare), `9.9.9.9` (Quad9)
- **Description**: Primary DNS server for resolving domain names
- **Important**: Required for fetching images from URLs

##### Secondary DNS
- **Required**: No
- **Type**: IPv4 address
- **Format**: `xxx.xxx.xxx.xxx`
- **Validation**: Each octet must be 0-255 (if provided)
- **Example**: `8.8.4.4` (Google secondary)
- **Alternatives**: `1.0.0.1` (Cloudflare secondary)
- **Description**: Backup DNS server (used if primary fails)

### Dashboard Images (Carousel Support)
- **Required**: Yes (at least 1 image)
- **Maximum**: 10 images
- **Type**: Multiple URL + interval pairs
- **Description**: Configure single image or multi-image carousel
- **Format**: PNG or JPEG (baseline encoding only, not progressive JPEG)
- **Resolution**: Must match screen resolution:
  - Inkplate 2: 212x104
  - Inkplate 5 V2: 960x540
  - Inkplate 6 Flick: 1448x1072
  - Inkplate 10: 1200x825
- **Mode Detection**: Automatic based on number of images configured
  - **1 image** = Single image mode (refreshes periodically)
  - **2+ images** = Carousel mode (rotates through images)

#### Image URL (per image)
- **Required**: Yes for first image, optional for others
- **Type**: URL
- **Example**: `https://example.com/dashboard1.png`
- **Validation**: Must start with `http://` or `https://`
- **Length**: Maximum 250 characters

#### Display Interval (per image)
- **Required**: Yes (when image URL provided)
- **Type**: Number
- **Unit**: Minutes
- **Default**: 5 minutes (auto-filled when URL entered)
- **Minimum**: 1 minute
- **Description**: How long to display this image before moving to next (or refreshing in single image mode)
- **Example**: Image 1 for 10 minutes, Image 2 for 5 minutes, Image 3 for 15 minutes

#### Progressive Disclosure UI
- Form shows **2 image slots by default** (required + optional)
- Click **"➕ Add Another Image"** to reveal slots 3-10 on demand
- Button hides when all 10 slots are visible
- Battery life estimator calculates average interval across all images

### Screen Rotation
- **Required**: No
- **Type**: Dropdown
- **Default**: 0° (Landscape)
- **Options**: 0° (Landscape), 90° (Portrait), 180° (Landscape Inverted), 270° (Portrait Inverted)
- **Description**: Display orientation for wall mounting
- **Note**: **Images must be pre-rotated to match your chosen orientation.** For example, if you select 90° for portrait mounting, provide portrait-oriented images (e.g., 720×1280 instead of 1280×720 for Inkplate 5 V2). The device does not rotate images automatically.

### MQTT Broker (Optional - Home Assistant Integration)
- **Required**: No
- **Type**: URL
- **Description**: MQTT broker URL for Home Assistant integration
- **Format**: `mqtt://hostname:port`
- **Example**: `mqtt://192.168.1.30:1883`
- **Note**: Leave empty to disable MQTT reporting

### MQTT Username (Optional)
- **Required**: No
- **Type**: Text
- **Description**: Username for MQTT broker authentication
- **Note**: Only needed if your broker requires authentication

### MQTT Password (Optional)
- **Required**: No
- **Type**: Password
- **Description**: Password for MQTT broker authentication
- **Note**: Only needed if your broker requires authentication
- **Security**: Stored securely in ESP32 NVS

### Frontlight Configuration (Inkplate 6 Flick Only)

**Note**: These fields only appear when using an Inkplate 6 Flick device with built-in frontlight support.

#### Frontlight Duration
- **Required**: No
- **Type**: Number
- **Unit**: Seconds
- **Default**: 0 (disabled)
- **Range**: 0-255 seconds
- **Description**: How long to keep the frontlight on after a manual button press update
- **Behavior**:
  - **0 seconds** = Frontlight never activates (default, backward compatible)
  - **1-255 seconds** = Frontlight turns on after successful image display and stays on for the configured duration
- **Use Cases**:
  - **Quick check** (15-30 seconds) - Brief illumination to verify the update
  - **Extended viewing** (60-300 seconds) - Comfortable reading time in low light conditions
- **Note**: Frontlight only activates for manual button press updates, not automatic timer-based refreshes

#### Frontlight Brightness
- **Required**: No
- **Type**: Number
- **Unit**: Brightness level
- **Default**: 63 (maximum brightness)
- **Range**: 0-63
- **Description**: Brightness level for the frontlight
- **Values**:
  - **0** = Minimum brightness (essentially off)
  - **31** = Medium brightness
  - **63** = Maximum brightness (default)
- **Recommendation**: Start with maximum (63) and reduce if too bright
- **Note**: Only used when Frontlight Duration > 0

## Web Interface

The configuration portal features a modern, card-based design optimized for mobile and desktop:

### Visual Design (v0.16.0+)
- **Gradient background** (purple theme)
- **Card-based sections** with white backgrounds, shadows, and rounded corners
- **Blue gradient section headers** for clear visual hierarchy
- **Floating battery life estimator badge** that follows scroll position
- **Mobile-friendly** responsive layout
- **Real-time validation** and feedback
- **Success/Error feedback** with color-coded messages
- **Auto-redirect** after submission

### Layout Organization

Configuration options are organized into clearly separated sections:

1. **📶 WiFi Network** - Network credentials
2. **🖼️ Dashboard Images** - Image URLs with individual display intervals (carousel support)
3. **📡 MQTT / Home Assistant** - Optional MQTT integration
4. **🕐 Scheduling** - Update hours, CRC32 change detection, battery estimator

Each section features:
- Clear icon-based headers
- Grouped related settings
- Contextual help text
- Visual hierarchy with proper spacing

### Pages

1. **Main Configuration Page** (`/`)
   - Device information card (device name, board type, IP address)
   - Collapsible configuration sections
   - Floating battery life estimator (shows estimated days of operation)
   - Field validation with inline error messages
   - Success/Error feedback
   - OTA firmware update button (full-width secondary button, only shown in CONFIG_MODE)
   - Reboot device button (full-width secondary button, only shown in CONFIG_MODE)
   - Factory reset option (danger zone, only shown on configured devices)
   - VCOM management button (advanced, only shown on devices with TPS65186 PMIC)

2. **Success Page** (`/submit` - POST success)
   - Confirmation message
   - Auto-redirects after 5 seconds
   - Device restarts automatically

3. **Error Page** (`/submit` - POST error)
   - Error message display
   - Auto-redirects back to form after 3 seconds

4. **Factory Reset Page** (`/factory-reset` - POST success)
   - Confirmation of reset completion
   - Device reboots automatically

5. **OTA Update Page** (`/ota` - GET)
   - **Option 1: GitHub Releases Update**
     - Check for latest firmware from GitHub
     - Automatic board-specific asset matching
     - Version comparison (current vs. latest)
     - One-click installation
     - Progress display on both web and e-ink screen
   - **Option 2: Manual File Upload**
     - File upload interface for `.bin` firmware files
     - Real-time progress bar during upload
     - Visual feedback on e-ink display
   - Warning banner with safety instructions
   - Only accessible in CONFIG_MODE

6. **OTA Update Handlers**
   - `/ota/check` (GET) - Query GitHub Releases API
     - Returns JSON with version info and download URL
     - Compares current version with latest release
     - Finds matching asset for board type
   - `/ota/install` (POST) - Download and install from GitHub
     - Streams firmware from GitHub
     - Shows progress on display
     - Automatic reboot after success
   - `/ota` (POST) - Manual file upload handler
     - Receives and flashes firmware binary
     - Shows progress on device display
     - Success: device reboots with new firmware
     - Error: displays error message

## Technical Details

### Classes Used

```cpp
ConfigPortal configPortal(&configManager, &wifiManager);
```

### Key Methods

```cpp
// Constructor with DisplayManager for OTA screen feedback
ConfigPortal configPortal(&configManager, &wifiManager, &displayManager);

// Start the web server
configPortal.begin();

// Handle incoming requests (call in loop)
configPortal.handleClient();

// Check if config was submitted
if (configPortal.isConfigReceived()) {
    ESP.restart();
}

// Stop the server
configPortal.stop();
```

### HTTP Routes

- `GET /` - Configuration form
- `POST /submit` - Form submission handler
- `POST /factory-reset` - Factory reset handler (CONFIG_MODE only)
- `POST /reboot` - Device reboot handler (CONFIG_MODE only)
- `GET /ota` - OTA firmware update page (CONFIG_MODE only)
- `POST /ota` - OTA firmware upload handler (CONFIG_MODE only)
- `GET /ota/check` - Check GitHub Releases for updates (CONFIG_MODE only)
- `POST /ota/install` - Install firmware from GitHub (CONFIG_MODE only)
- `404` - Not found (redirects to home)

### Access Point Configuration

- **SSID Format**: `inkplate-dashb-` + last 6 hex chars of MAC
- **Security**: Open (no password)
- **IP Address**: `192.168.4.1` (ESP32 default)
- **Timeout**: 5 minutes (AP_TIMEOUT_MS)

## Validation Rules

The portal validates:
1. **WiFi SSID** must not be empty
2. **At least 1 image URL** must be provided
3. **Each image URL** must start with `http://` or `https://`
4. **Each image URL** must not exceed 250 characters
5. **Display interval** must be provided for each image (minimum 1 minute)
6. **Screen rotation** must be 0-3 (defaults to 0 if invalid)
7. **Static IP fields** (if Static IP mode selected):
   - IP Address, Gateway, Subnet, and Primary DNS are required
   - All IP fields must match IPv4 format: `xxx.xxx.xxx.xxx`
   - Each octet must be in range 0-255
   - Secondary DNS is optional but validated if provided

## After Configuration

Once configured:
1. Settings are saved to ESP32 NVS (non-volatile storage)
2. Device marked as "configured"
3. Device restarts
4. On next boot, device skips AP mode
5. Device enters normal operation mode (connects to WiFi, downloads image)

## Firmware Updates (OTA)

The configuration portal includes two methods for updating device firmware:

### Option 1: Update from GitHub Releases (Recommended)

Automatically download and install the latest firmware directly from GitHub:

1. **Access OTA Page**:
   - Enter config mode (button press for 2.5+ seconds)
   - Navigate to `http://[device-ip]/ota`
   - Or click "⬆️ Firmware Update" button from main config page

2. **Automatic Update Check**:
   - Page automatically checks GitHub Releases API on load
   - Displays current version vs. latest available version
   - Shows matching firmware asset for your board type
   - "Install Update" button appears if new version is available

3. **Install Update** (if available):
   - Click "Install Update" button
   - Redirected to dedicated update status page
   - Device downloads firmware from GitHub in background
   - Real-time progress displayed (percentage and KB downloaded)
   - Progress shown on both web UI and e-ink screen
   - Device automatically reboots after successful installation

**Features**:
- Automatic update check on page load (no button click needed)
- Automatic board detection (finds correct firmware for your device)
- Version comparison (only shows install button if update available)
- No manual file download needed
- Real-time progress tracking with percentage and data transfer
- Connection loss detection (shows "Device is Rebooting" when device restarts)
- Safe: keeps current firmware if download fails

**Technical Details**:
- Uses unauthenticated GitHub API (60 requests/hour limit)
- Repository: `jantielens/inkplate-dashboard`
- Asset naming: `{board}-v{version}.bin` (e.g., `inkplate2-v0.15.0.bin`)
- Semantic version comparison (e.g., v0.15.0 > v0.14.0)
- Streaming download (no buffering entire file in memory)

### Option 2: Manual File Upload

Upload a firmware file from your computer:

1. **Download Firmware**:
   - Get `.bin` file from [GitHub Releases](https://github.com/jantielens/inkplate-dashboard/releases)
   - Ensure you download the correct file for your board type

2. **Upload**:
   - Access OTA page (`/ota`)
   - Scroll to "Option 2: Upload Local Firmware File"
   - Click "Choose File" and select the `.bin` file
   - Click "Upload & Install"
   - Wait for upload and installation to complete

3. **Reboot**:
   - Device automatically reboots after successful update

**When to use manual upload**:
- Testing custom builds
- No internet connection on device
- GitHub API rate limit reached
- Installing beta/development versions

### Safety Notes

⚠️ **Important**:
- Do not power off the device during firmware update
- Ensure stable WiFi connection for GitHub updates
- Only upload firmware built for your specific board type
- Device will automatically reboot after successful update
- If update fails, device keeps current firmware (safe)

### Troubleshooting Updates

**"GitHub API rate limit exceeded"**
- Wait 1 hour or use manual file upload
- Rate limit: 60 requests per hour per IP address

**"No firmware asset found for board"**
- Contact developer - asset missing from release
- Use manual upload as workaround

**"Network error" during GitHub check**
- Verify device has internet access
- Check if GitHub is accessible from your network
- Try again or use manual upload

**"Download incomplete" or "Update failed"**
- Check WiFi signal strength
- Ensure stable connection
- Retry the update
- Use manual upload if persistent issues

**"Already up to date"**
- You're running the latest version
- No action needed

## Factory Reset

### Via Web Interface (Recommended)

The easiest way to factory reset your device:

1. **Enter Config Mode**:
   - Long press the button (2.5+ seconds) if device is configured
   - Or connect to the AP if device is already in setup mode

2. **Access Configuration Page**:
   - Open browser to device IP address
   - For AP mode: `http://192.168.4.1`
   - For WiFi mode: Use device's local IP

3. **Locate Factory Reset**:
   - Scroll to bottom of configuration page
   - Find the red "Danger Zone" section
   - Only visible on devices that are already configured

4. **Perform Reset**:
   - Click "🗑️ Factory Reset" button
   - Confirm in the modal dialog
   - Device erases all settings and reboots

5. **Fresh Start**:
   - Device boots into unconfigured state
   - Creates new access point
   - Ready for initial setup

### Via Code (Advanced)

For programmatic factory reset:
```cpp
configManager.clearConfig();
ESP.restart();
```

### What Gets Erased

Factory reset permanently deletes:
- WiFi credentials (SSID and password)
- Image URL
- Refresh rate settings
- MQTT broker configuration (broker URL, username, password)
- Configuration status flag

The device will boot into AP mode and display setup instructions.

## Device Reboot

### Via Web Interface

You can reboot your device without changing any configuration:

1. **Enter Config Mode**:
   - Long press the button (2.5+ seconds) if device is configured
   - Or connect to the AP if device is already in setup mode

2. **Access Configuration Page**:
   - Open browser to device IP address
   - For AP mode: `http://192.168.4.1`
   - For WiFi mode: Use device's local IP

3. **Locate Reboot Button**:
   - Scroll to middle of configuration page
   - Find the blue "Reboot Device" button (below "Firmware Update" button)
   - Only visible in CONFIG_MODE

4. **Perform Reboot**:
   - Click "Reboot Device" button
   - Device confirms action and reboots
   - Device retains all current settings
   - Useful for troubleshooting connection issues

### Use Cases

- **WiFi connectivity issues**: Reboot to reset network state
- **Display refresh stuck**: Reboot to restart the update loop
- **General troubleshooting**: Soft reset without losing configuration
- **After network changes**: Reconnect to new WiFi after reboot

### Via Code (Advanced)

For programmatic reboot:
```cpp
ESP.restart();
```

## Security Considerations

- **Open AP**: No password required for initial setup
- **Local only**: Configuration portal only accessible when connected to device AP
- **No HTTPS**: Uses HTTP (sufficient for local setup)
- **Password storage**: WiFi password stored in encrypted NVS

## Troubleshooting

### Can't see the WiFi network
- Check device display for AP name
- Wait 10-30 seconds after boot
- Look for network starting with `inkplate-dashb-`

### Can't access web page
- Ensure connected to Inkplate WiFi
- Try `http://192.168.4.1` (not https)
- Some devices may require disabling mobile data

### Configuration not saving
- Check Serial monitor for error messages
- Ensure all required fields are filled
- Verify image URL is accessible


### Device keeps entering AP mode
- Configuration may not be saved properly
- Check NVS partition is not corrupted
- Try factory reset and reconfigure

## VCOM Management

**⚠️ WARNING: Advanced Feature - Use with Caution**

VCOM (Common Voltage) is a critical voltage setting that affects e-ink display contrast and image quality. Each e-ink panel has an optimal VCOM value typically printed on a label on the panel itself or specified by the manufacturer.

### What is VCOM?

VCOM is the common voltage used by the e-ink display controller (TPS65186 PMIC) to drive the e-ink panel. The correct VCOM value ensures:
- Optimal contrast
- Clear image reproduction
- Minimal ghosting
- Proper grayscale levels

### Accessing VCOM Management

1. Navigate to the Configuration Portal main page
2. Scroll to the **Danger Zone** section
3. Click the **⚠️ VCOM Management** button

### Reading VCOM

The VCOM Management page displays:
- **Current VCOM**: The voltage currently programmed in the TPS65186 EEPROM (e.g., "-1.53V")
- This value is automatically loaded by the PMIC when the device powers on
- The value is also displayed in serial logs at startup

### Programming VCOM

**⚠️ CRITICAL SAFETY INFORMATION:**
- Programming an incorrect VCOM value can damage your display
- Only change VCOM if you know the correct value for your specific panel
- The optimal value is typically on a label on the e-ink panel or in manufacturer documentation
- Valid range: -5.0V to 0V (typically between -1.0V and -3.0V)

**To program a new VCOM value:**

1. Enter the desired VCOM voltage in the input field (e.g., "-1.53")
2. Click **Program VCOM**
3. Read and acknowledge the warning dialog
4. The system will:
   - Validate the input range
   - Program the value to TPS65186 EEPROM (takes ~260ms)
   - Power cycle the display to reload from EEPROM
   - Verify the programmed value
   - Display detailed diagnostics

### Understanding Diagnostics

After programming, the page shows detailed diagnostics:
```
Initial VCOM Read: -1.53V (0x199)
Attempting to program: -1.53V (0x199)
Register 0x04 before programming: 0x01
Register 0x04 after programming: 0x41 (bit 6 set)
Bit 6 cleared after 260ms (EEPROM write complete)
Registers zeroed for verification
Power cycled for EEPROM reload
Register 0x04 after power cycle: 0x01
Verification VCOM Read: -1.53V (0x199)
✓ VCOM programming successful and verified!
```

**Key indicators:**
- **Bit 6 cleared after Xms**: Confirms EEPROM write completed successfully
- **Verification VCOM Read**: Must match the programmed value
- **✓ Success message**: Indicates the value persisted after power cycle

### When to Adjust VCOM

Consider adjusting VCOM if:
- Display images appear washed out or too dark
- Replacing the e-ink panel with a different model
- Manufacturer documentation specifies a different value
- Experiencing persistent ghosting issues

**Do NOT adjust VCOM:**
- As a troubleshooting step for connectivity issues
- Without knowing the correct value for your panel
- If the display is working properly

### Troubleshooting

**Programming fails:**
- Check serial logs for detailed I2C transaction information
- Ensure stable power supply during EEPROM write
- Verify the TPS65186 PMIC is responding (I2C address 0x48)

**Value doesn't persist:**
- Diagnostic logs should show "Bit 6 cleared" confirming EEPROM write
- Verification read should match programmed value after power cycle
- If verification fails, check hardware connections

**Display quality issues after programming:**
- If you programmed an incorrect value, reprogram with the correct value
- Check the panel label or manufacturer specs for the optimal VCOM
- Some panels may have tolerances (e.g., ±0.05V)

