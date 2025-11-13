# Factory Reset Feature

## Overview

The factory reset feature allows users to completely erase all stored configuration on the Inkplate device and return it to its initial, unconfigured state.

## How to Use

1. **Access the Configuration Portal**
   - If the device is already configured, connect to your WiFi network and access the device's IP address in your browser
   - If the device is not configured, it will create an AP (Access Point) - connect to it and navigate to `http://inkplate-xxxxxx.local` or `http://192.168.4.1`

2. **Locate the Factory Reset Button**
   - On the configuration page, scroll to the bottom
   - The "Danger Zone" section will appear (only visible on configured devices)
   - Click the **"🗑️ Factory Reset"** button

3. **Confirm the Reset**
   - A confirmation modal will appear asking you to confirm
   - Click **"Yes, Reset"** to proceed, or **"Cancel"** to abort

4. **Device Reboot**
   - After confirming, all configuration will be erased immediately
   - The device will automatically reboot in 2 seconds
   - After reboot, the device will start in setup mode with a fresh access point

## What Gets Erased

The factory reset clears **all** stored configuration, including:
- WiFi network name (SSID)
- WiFi password
- Image URL
- Refresh rate settings
- MQTT broker configuration (broker URL, username, password)
- Configuration status flag

## Technical Implementation

### Web Interface
- New route: `POST /factory-reset`
- Confirmation modal to prevent accidental resets
- Styled "Danger Zone" section with clear warnings

### Backend
- Uses `ConfigManager::clearConfig()` to erase all preferences
- Calls `ESP.restart()` to reboot the device after clearing config
- Only shows factory reset option when device is already configured

### Files Modified
- `common/src/config_portal.h` - Added handler declaration
- `common/src/config_portal.cpp` - Implemented factory reset UI and handler

## Safety Features

1. **Confirmation Modal**: Prevents accidental clicks by requiring explicit confirmation
2. **Visual Warnings**: Red "Danger Zone" section with warning icons
3. **Only on Configured Devices**: Factory reset option only appears if device has configuration to reset
4. **Automatic Reboot**: Device automatically restarts to apply the reset

## After Factory Reset

After a factory reset:
1. The device reboots automatically
2. It starts in unconfigured mode
3. An access point is created (e.g., "Inkplate-XXXXXX")
4. Users can connect and reconfigure from scratch
5. All previous settings are permanently lost

## Use Cases

- **Selling/Transferring Device**: Erase personal WiFi credentials and settings
- **Troubleshooting**: Start fresh if configuration becomes corrupted
- **Network Changes**: Quickly reset when moving to a new WiFi network
- **Testing**: Reset to factory state for development/testing purposes
