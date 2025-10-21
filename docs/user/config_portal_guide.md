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
   - Image URL - Required (must be PNG format)
   - Refresh Rate - Optional (default: 5 minutes)

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

### Image URL
- **Required**: Yes
- **Type**: URL
- **Description**: Full URL to PNG image
- **Format**: Must be PNG
- **Resolution**: Must match screen resolution:
  - Inkplate 5 V2: 1280x720
  - Inkplate 10: 1200x825
- **Example**: `https://example.com/dashboard.png`

### Refresh Rate
- **Required**: No
- **Type**: Number
- **Default**: 5 minutes
- **Minimum**: 1 minute
- **Description**: How often to download and update the image

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

## Web Interface

The configuration portal features a modern, responsive design:

- **Gradient background** (purple theme)
- **Mobile-friendly** layout
- **Real-time validation**
- **Success/Error feedback**
- **Auto-redirect** after submission

### Pages

1. **Main Configuration Page** (`/`)
   - Shows device information (AP name, IP)
   - Configuration form
   - Field validation
   - Factory reset option (only shown on configured devices)
   - OTA firmware update button (full-width green button, only shown in CONFIG_MODE)

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
   - File upload interface for `.bin` firmware files
   - Real-time progress bar during upload in web browser
   - Visual feedback on e-ink display during installation
   - Warning banner with safety instructions
   - Only accessible in CONFIG_MODE

6. **OTA Upload Handler** (`/ota` - POST)
   - Receives and flashes firmware binary
   - Disables watchdog timer to prevent interruption
   - Shows progress on device display
   - Success: device reboots with new firmware
   - Error: re-enables watchdog and displays error message

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
- `GET /ota` - OTA firmware update page (CONFIG_MODE only)
- `POST /ota` - OTA firmware upload handler (CONFIG_MODE only)
- `404` - Not found (redirects to home)

### Access Point Configuration

- **SSID Format**: `inkplate-dashb-` + last 6 hex chars of MAC
- **Security**: Open (no password)
- **IP Address**: `192.168.4.1` (ESP32 default)
- **Timeout**: 5 minutes (AP_TIMEOUT_MS)

## Validation Rules

The portal validates:
1. **WiFi SSID** must not be empty
2. **Image URL** must not be empty
3. **Refresh rate** must be at least 1 minute (defaults to 5 if invalid)
4. **Screen rotation** must be 0-3 (defaults to 0 if invalid)

## After Configuration

Once configured:
1. Settings are saved to ESP32 NVS (non-volatile storage)
2. Device marked as "configured"
3. Device restarts
4. On next boot, device skips AP mode
5. Device enters normal operation mode (connects to WiFi, downloads image)

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

