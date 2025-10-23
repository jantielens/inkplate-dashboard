# Inkplate Dashboard - User Guide

A complete guide for using your Inkplate Dashboard to display images on your e-ink display.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Initial Setup & Configuration](#initial-setup--configuration)
3. [Configuration Options](#configuration-options)
4. [Normal Operation](#normal-operation)
5. [Manual Updates](#manual-updates)
6. [Firmware Updates (OTA)](#firmware-updates-ota)
7. [Entering Different Modes](#entering-different-modes)
8. [Troubleshooting](#troubleshooting)

---

## Getting Started

### What You Need

- **Inkplate Device** (Inkplate 2, Inkplate 5 V2, Inkplate 10, or Inkplate 6 Flick)
- **USB Cable** for initial programming
- **WiFi Network** (2.4GHz - 5GHz is not supported)
- **Computer** with USB port for flashing firmware
- **Image URL** - A web URL hosting your PNG image (can be on your local network or public internet)

### Flashing the Firmware

#### Option A: Web-based Flasher (Recommended)

The easiest way to flash firmware to your Inkplate device is using the web-based flasher:

1. **Connect your Inkplate** to your computer via USB cable

2. **Visit the web flasher**: [https://jantielens.github.io/inkplate-dashboard/](https://jantielens.github.io/inkplate-dashboard/)

3. **Select your device model** from the dropdown menu

4. **Click "Connect"** and select your device's serial port when prompted

5. **Click "Install"** - the latest firmware will be downloaded and flashed automatically

6. **Wait for completion** - you'll see progress indicators and a success message

7. **Press the reset button** on your Inkplate or unplug/replug the USB cable

**Advantages of the web flasher:**
- No software installation required
- Always uses the latest firmware
- Automatic device detection
- Built-in progress tracking
- Works on Windows, macOS, and Linux with Chrome/Edge browsers

#### Option B: Manual Flashing

If you prefer to download firmware files manually or the web flasher doesn't work in your environment:

1. **Download firmware**: Visit [GitHub Releases](https://github.com/jantielens/inkplate-dashboard/releases) and download the `.bin` file for your device

   - Use [ESPHome Flasher](https://github.com/esphome/esphome-flasher) (graphical tool)
   - Use [esptool.py](https://github.com/espressif/esptool) (command-line utility)
   - Or any compatible serial flashing utility

3. **Follow the tool's instructions** for uploading `.bin` files to ESP32-based boards

**Note**: Manual flashing requires more technical knowledge and additional software installation. The web flasher is recommended for most users.

---

## Initial Setup & Configuration

The setup process is divided into two steps to make it easier to configure your device.

### Step 1: WiFi Setup (Boot Mode)

When you first power on your device (or after a factory reset), it will create its own WiFi network.

**What You'll See on the Display:**

```
Setup - Step 1
Connect WiFi

1. Connect to WiFi:
   inkplate-dashb-XXXXXX
2. Open browser to:
   http://192.168.4.1
3. Enter WiFi settings
```

**How to Configure:**

1. **Look at your device's screen** - it will show the WiFi network name (something like `inkplate-dashb-A1B2C3`)

2. **Connect your phone or computer** to this WiFi network:
   - Network name: As shown on the screen
   - Password: None (open network)

3. **Open a web browser** and go to: `http://192.168.4.1`
   - **Note**: Use `http://` not `https://`
   - On phones, you may need to disable mobile data

4. **Enter your WiFi credentials**:
   - **WiFi SSID**: Your home/office WiFi network name
   - **WiFi Password**: Your WiFi password (leave blank if no password)
   
5. **Click "Save WiFi Settings"**

6. **Wait for the device to restart** - it will now connect to your WiFi network.

### Step 2: Dashboard Configuration (Auto Config Mode)

After the device connects to your WiFi, it will automatically enter configuration mode because you haven't set up the image URL yet.

**What You'll See on the Display:**

```
Setup - Step 2
Configure Dashboard

Open browser to:
  http://192.168.1.XXX
```

**How to Configure:**

1. **Connect to your regular WiFi network** (the same one you configured in Step 1)

2. **Look at the device's screen** - it will show an IP address (e.g., `http://192.168.1.123`)

3. **Open a web browser** and go to the IP address shown on the screen

4. **Complete your dashboard settings**:
   - **Image URL**: Full URL to your PNG image (e.g., `http://example.com/dashboard.png`)
   - **Refresh Rate**: How often to update (in minutes, default is 5)
   - **MQTT Settings** (optional): For Home Assistant integration
   - **Debug Mode** (optional): Shows status messages on the display

5. **Click "Save Configuration"**

6. **Device will restart** and begin normal operation - downloading and displaying your image!

---

## Configuration Options

### Required Settings

#### WiFi SSID
- **What it is**: Your WiFi network name
- **Required**: Yes (in Step 1)
- **Case sensitive**: Yes
- **Example**: `MyHomeWiFi`

#### WiFi Password
- **What it is**: Your WiFi password
- **Required**: Only if your network requires a password
- **Case sensitive**: Yes
- **Example**: `MySecurePassword123`

#### Image URL
- **What it is**: Full web address to your PNG image
- **Required**: Yes (in Step 2)
- **Format**: Must start with `http://` or `https://`
- **Can be local**: Works with local web servers (e.g., `http://192.168.1.50/dashboard.png`) as long as it's on the same network
- **Can be public**: Works with public URLs (e.g., `https://example.com/images/dashboard.png`)
- **Example**: `http://192.168.1.100:8123/local/dashboard.png` (Home Assistant) or `https://example.com/dashboard.png`

**Image Requirements:**
- **Format**: PNG only (JPG/GIF not supported)
- **Resolution**: Must match your screen exactly (in the orientation you've configured):
  - Inkplate 2: 212×104 pixels (landscape) or 104×212 pixels (portrait)
  - Inkplate 5 V2: 1280×720 pixels (landscape) or 720×1280 pixels (portrait)
  - Inkplate 10: 1200×825 pixels (landscape) or 825×1200 pixels (portrait)
  - Inkplate 6 Flick: 1024×758 pixels (landscape) or 758×1024 pixels (portrait)
- **Rotation**: Your image must be pre-rotated to match the Screen Rotation setting (see below)
- **Accessibility**: Must be reachable from the device's network - can be a local server on your network or a public URL on the internet

#### Refresh Rate
- **What it is**: How often the device updates the image (in minutes)
- **Required**: Yes (in Step 2)
- **Default**: 5 minutes
- **Minimum**: 1 minute
- **Example**: 15 (updates every 15 minutes)

#### Update Hours
- **What it is**: Select which hours (0-23) the device should perform scheduled updates
- **Required**: No (defaults to all hours enabled)
- **Default**: All 24 hours enabled (updates any time)
- **How it works**: Check the boxes for hours when you want the device to update. Unchecked hours will be skipped to save battery
- **Timezone**: Hours are adjusted to your configured timezone offset
- **Manual override**: Manual button press updates always work regardless of hour setting
- **Example**: Uncheck hours 0-7 (midnight to 8am) to prevent updates during night hours
- **Battery impact**: Disabling hours can extend battery life - fewer update cycles per day
- **Smart scheduling**: Device wakes on timer but checks the schedule; only performs update if current hour is enabled

### Optional Settings

#### Debug Mode
- **What it is**: Shows status messages on the display during updates
- **Default**: Disabled
- **When to enable**: If you want to see WiFi connection status, download progress, etc.
- **When to disable**: For a cleaner display that only shows your image

#### CRC32-Based Change Detection
- **What it is**: Checks if the image has changed before downloading it
- **Default**: Disabled
- **Battery impact**: Can extend battery life by 2.5× when images change infrequently
- **Requirements**: Your web server must provide `.crc32` files alongside images (e.g., `image.png.crc32`)
- **How it works**: Downloads a small checksum file first; if unchanged, skips the full image download
- **Fallback**: If `.crc32` file is missing or invalid, automatically downloads the full image
- **When to enable**: 
  - If you're running on battery power
  - If your image doesn't change on every refresh (e.g., once per day)
  - If you're using [@jantielens/ha-screenshotter](https://github.com/jantielens/ha-screenshotter) which automatically generates CRC32 files
- **When to disable**: If your server doesn't provide `.crc32` files, or if your image changes frequently

#### Timezone Offset
- **What it is**: Your timezone offset from UTC for adjusting hourly schedule times
- **Required**: No (defaults to 0 = UTC/GMT)
- **Range**: -12 to +14 hours
- **Format**: Enter as a number: `0` for UTC, `-5` for EST, `+1` for CET, `+9` for JST, etc.
- **Important note**: Daylight Saving Time changes require manual offset update when DST begins/ends in your region
- **Used by**: The hourly update schedule to convert UTC time to your local time
- **Example**: If you're in Eastern Time (EST = -5), enter `-5`. When DST begins (EDT = -4), remember to update it
- **Tip**: If you're unsure of your offset, search "my timezone offset UTC" or check a time zone website

#### Screen Rotation
- **What it is**: The orientation of your display
- **Required**: No (defaults to 0° = Landscape)
- **Options**: 
  - **0° (Landscape)**: Default horizontal orientation
  - **90° (Portrait)**: Rotated 90 degrees clockwise
  - **180° (Inverted Landscape)**: Upside down
  - **270° (Portrait Inverted)**: Rotated 270 degrees clockwise (or 90° counter-clockwise)
- **Important**: Your images must be oriented to match this setting
  - If you select 90° (Portrait), provide a portrait-oriented image (e.g., 720×1280 for Inkplate 5 V2)
  - If you select 0° (Landscape), provide a landscape-oriented image (e.g., 1280×720 for Inkplate 5 V2)
- **Use case**: For mounting your display in portrait orientation or upside-down
- **Example**: Set to 90° if your Inkplate is mounted vertically

### Battery Life Estimator

The configuration portal includes a real-time battery life estimator that helps you understand how your settings impact battery life. This interactive tool updates instantly as you adjust your configuration, providing immediate feedback on your power consumption choices.

#### What It Shows

The estimator displays:
- **Estimated battery life** in days and months
- **Color-coded status indicator**:
  - 🟢 **Excellent** (180+ days / 6+ months) - Great configuration for battery life
  - 🔵 **Good** (90-179 days / 3-6 months) - Balanced approach
  - 🟠 **Moderate** (45-89 days / 1.5-3 months) - Reasonable for frequent updates
  - 🔴 **Poor** (<45 days / <1.5 months) - Very aggressive settings or large battery needed
- **Detailed metrics**:
  - Daily power consumption (mAh/day)
  - Wake-ups per day
  - Active time per day (device awake with WiFi on)
  - Sleep time per day (deep sleep mode)
- **Visual progress bar** with color gradient matching the status
- **Context-aware tips** suggesting how to improve battery life

#### Battery Capacity Selection

You can select your battery capacity from the dropdown:
- **600 mAh** (Small) - Compact battery
- **1200 mAh** (Standard) - Default, typical for most Inkplate devices
- **1800 mAh** (Medium) - Mid-range extended battery
- **3000 mAh** (Large) - Extended battery pack
- **6000 mAh** (Extra Large) - Maximum battery capacity

#### Expected Image Changes Per Day

This setting helps the estimator provide more accurate predictions by accounting for your specific use case:

- **Default**: 5 changes/day (suitable for most dashboards)
- **Weather dashboards**: 24+ changes/day (updates every hour)
- **Calendar/events**: 5-10 changes/day (typical for agenda displays)
- **Static info**: 1-2 changes/day (rarely changing content)

When CRC32 change detection is enabled, this value represents how many times per day your image server actually generates a *different* image. Wake-ups that find no change only take 1 second (CRC32 check) instead of 7 seconds (full download), significantly improving battery life.

**Why it matters**: A weather dashboard might wake up every 5 minutes but only get new weather data once per hour (24 actual changes), while a meeting room display might check every 10 minutes but only update 3-4 times per day.

#### How Settings Impact Battery Life

The estimator considers all battery-impacting settings:
- **Refresh Rate**: Lower intervals = more wake-ups = shorter battery life
- **CRC32 Change Detection**: When enabled, dramatically extends battery life (5-8× improvement) by skipping unnecessary downloads
- **Update Hours**: Disabling night hours or off-peak times reduces daily wake-ups
- **Expected Image Changes Per Day**: Higher values = more full downloads = shorter battery life (only relevant with CRC32 enabled)
- **Battery Capacity**: Larger batteries last longer proportionally

#### Understanding the Calculations

The estimator uses these assumptions (clearly disclosed in the portal):
- **Power consumption**: WiFi active = 100mA, Display refresh = 50mA, Deep sleep = 20µA
- **Timing**: Full image update = 7 seconds, CRC32 check = 1 second
- **Signal strength**: Good WiFi signal (poor signal increases power consumption)
- **Image changes**: User-configurable (default 5/day) - represents actual content changes when using CRC32

**Important Note**: Real-world battery life may vary by ±20% based on:
- WiFi signal strength variations
- Image file size differences
- Battery quality and age
- Temperature effects
- Network latency

#### Example Scenarios

**Optimal Setup** (5-min refresh, CRC32 on, 17h active, 1200mAh):
- **Result**: ~174 days (5.8 months) - GOOD status
- **Daily power**: 6.9 mAh
- **Wake-ups**: 204 per day
- **Active time**: 3.4 minutes/day

**Aggressive Setup** (1-min refresh, CRC32 on, 24h active, 1200mAh):
- **Result**: ~40 days (1.3 months) - POOR status
- **Daily power**: 29.8 mAh
- **Wake-ups**: 1,020 per day
- **Tip shown**: "Consider increasing refresh rate to 5-10 minutes"

**Conservative Setup** (10-min refresh, CRC32 on, 17h active, 1200mAh):
- **Result**: ~300 days (10.0 months) - EXCELLENT status
- **Daily power**: 4.0 mAh
- **Wake-ups**: 102 per day

**Without CRC32** (5-min refresh, CRC32 off, 17h active, 1200mAh):
- **Result**: ~35 days (1.2 months) - POOR status
- **Daily power**: 34.1 mAh
- **Tip shown**: "Enable CRC32 change detection to extend battery life by 5-8×!"

#### Using the Estimator

1. **Access it** in the configuration portal after the "Update Hours" section
2. **Adjust your settings** and watch the estimator update in real-time
3. **Pay attention to tips** - they provide actionable advice for your configuration
4. **Balance your needs** between update frequency and battery life
5. **Consider CRC32** - it's the single biggest battery life improvement you can make

The estimator helps you make informed decisions without trial and error, setting realistic expectations for your battery-powered deployment.

#### MQTT Broker (Home Assistant Integration)
- **What it is**: Address of your MQTT broker for Home Assistant
- **Format**: `mqtt://hostname:port` or `mqtt://IP:port`
- **Example**: `mqtt://192.168.1.50:1883`
- **What it does**: Enables battery voltage and percentage reporting to Home Assistant with auto-discovery

#### MQTT Username
- **What it is**: Username for MQTT authentication
- **Required**: Only if your MQTT broker requires authentication
- **Example**: `homeassistant`

#### MQTT Password
- **What it is**: Password for MQTT authentication
- **Required**: Only if your MQTT broker requires authentication
- **Example**: `MyMQTTPassword`

---

## Normal Operation

Once your device is fully configured, it operates automatically:

### Normal Cycle Behavior

Your device follows a simple cycle that repeats automatically. When the device wakes up from deep sleep based on your configured refresh rate, you may notice the status LED briefly flash if your board has one. The first thing the device does is connect to your WiFi network, which usually takes between 3 and 5 seconds depending on signal strength.

Once connected, the device downloads the PNG image from your configured URL. The download time varies based on your image size and network speed, but typically takes between 2 and 10 seconds for a dashboard image. After the download completes, the device clears the e-ink screen and renders the downloaded image. The e-ink refresh process takes a few seconds - this is normal behavior for e-ink displays as they physically move particles to create the image.

If you have MQTT enabled for Home Assistant integration, the device will publish status information including battery voltage, battery percentage (calculated from voltage using Li-ion discharge curve), WiFi signal strength, and loop time (how long the entire update took). Finally, the device enters deep sleep mode to conserve power. During deep sleep, the device consumes almost no power - just 20 microamps - which is why battery life can extend for months. The device will wake automatically after the configured refresh interval and repeat the entire cycle.

### What You'll See

When debug mode is disabled (which is the default setting), your display shows only your image with a clean, professional appearance and no status messages cluttering the screen. This is ideal for a polished dashboard that looks like a finished product.

With debug mode enabled, you'll see brief status messages appear during the update process. First, the display shows information about your configuration including the network name, refresh rate, and connection status. Then as the download begins, you'll see the image URL and MQTT connection status if you have that configured. These messages help you understand what's happening during each update cycle, which can be useful for troubleshooting or just satisfying your curiosity about the process. After the status messages, your actual image appears on the display.

### Power Consumption

Your device uses different amounts of power depending on what it's doing. During active mode when WiFi is on and the device is downloading, it typically draws between 80 and 120 milliamps. When refreshing the display, power consumption drops to around 40 to 60 milliamps. The real magic happens during deep sleep, where the device uses only 0.02 milliamps (20 microamps) - barely any power at all.

To give you a practical example, if you're using a 3000mAh battery with a 15-minute refresh interval, your device wakes up 96 times per day. Each wake cycle takes about 15 seconds, so the device is only active for about 24 minutes per day. The rest of the time it's in deep sleep mode. With this usage pattern, you can expect your battery to last between 2 and 4 months, though the actual duration depends on factors like WiFi signal strength and image download time.

**With CRC32 Change Detection Enabled:** If your images only change occasionally (e.g., once per day), enabling CRC32-based change detection can dramatically extend battery life. When the image hasn't changed, the device only downloads a tiny checksum file (~10 bytes) instead of the full image, reducing wake time from 11 seconds to 4.5 seconds. This reduces daily power consumption by 60.7%, extending battery life by 2.5×. For example, with a 1200mAh battery and 5-minute refresh intervals, battery life increases from ~13 days to ~33 days.

---

## Manual Updates

You can manually trigger an immediate update without waiting for the automatic refresh cycle.

### Devices with a Wake Button (Inkplate 5 V2, 10, 6 Flick)

If your device has a wake button, triggering a manual update is simple. Just press and quickly release the wake button with a short press - don't hold it down. The device wakes immediately and shows you visual feedback with a message saying "Manual Refresh" and "Button pressed - updating..." so you know your button press was registered. The update then proceeds just like a normal automatic cycle, connecting to WiFi, downloading the image, and displaying it. After showing the updated image, the device returns to sleep and resumes its normal schedule.

You can find the button in different locations depending on your device. On the Inkplate 5 V2, look for a small button labeled "WAKE" on the board itself. The Inkplate 10 has a large round button prominently placed on the front panel, making it easy to press. The Inkplate 6 Flick features a button on the side of the device for convenient access.

This manual refresh feature is particularly useful when you've updated the source image and want to see it immediately without waiting for the next automatic cycle. It's also great for testing your configuration or showing someone the latest version of your dashboard on demand.

### Devices without a Wake Button (Inkplate 2)

The Inkplate 2 doesn't have a physical wake button, which means the manual refresh feature isn't available on this device. Your display will only update according to the automatic schedule you've configured. If you need more frequent updates, you can reduce the refresh rate to as low as 1 minute in the configuration settings, though this isn't recommended if you're running on battery power as it will drain much faster. Alternatively, you can use the reset button to enter config mode, which will trigger an update when you save your settings.

---

## Firmware Updates (OTA)

You can update your device's firmware wirelessly using the built-in OTA (Over-The-Air) update feature, without needing to connect a USB cable or use programming tools.

### Accessing the OTA Update Page

1. **Enter Config Mode** (long press the button for 2.5+ seconds)
   - For Inkplate 2: use the reset button to trigger config mode
   
2. **Connect to your WiFi network** (the same network your Inkplate is connected to)

3. **Open the configuration page** in your browser using the IP address shown on the display

4. **Click "⬆️ Firmware Update"** at the bottom of the configuration page

### Uploading New Firmware

**Option A: Use the Web Flasher (Recommended)**

For the easiest update experience, use the [web flasher](https://jantielens.github.io/inkplate-dashboard/) while your device is in config mode. This automatically downloads and installs the latest firmware.

**Option B: Manual OTA Update**

1. **Download the latest firmware** from the [releases page](https://github.com/jantielens/inkplate-dashboard/releases)
   - Make sure to download the correct `.bin` file for your device model
   - Example: `inkplate5v2-v0.4.0.bin` for Inkplate 5 V2

2. **On the OTA Update page**, click "Choose File" and select the `.bin` firmware file

3. **Click "Upload Firmware"**
   - A progress bar will show the upload status
   - **On the device screen**, you'll see "Firmware Update" with installation progress
   - Do NOT power off or disconnect the device during upload

4. **Wait for completion**
   - The upload typically takes 1-2 minutes
   - The device will automatically restart with the new firmware
   - You'll see a success message before the reboot

### Important Notes

⚠️ **Safety Warnings:**
- Only upload firmware files built specifically for your Inkplate model
- Do not power off the device during the update process
- Make sure your device has sufficient power (USB or battery above 50%)
- The update process takes about 1-2 minutes

✅ **After Update:**
- Your configuration settings are preserved (WiFi, image URL, etc.)
- The device will restart automatically and resume normal operation
- Check the version number in the config portal footer to confirm the update

💡 **Troubleshooting:**
- If the upload fails, simply try again
- If the device doesn't restart, press the reset button manually
- If the new firmware doesn't work, you can always re-flash via USB cable

---

## Entering Different Modes

Your device can operate in three different modes. Here's how to access each one:

### Boot Mode (AP Mode)

**When It Happens Automatically:**
- First time you power on the device
- After a factory reset
- When WiFi credentials are erased

**What It's For:**
- Initial WiFi setup
- Configuring WiFi credentials only

**Display Shows:**
```
Setup - Step 1
Connect WiFi

1. Connect to WiFi:
   inkplate-dashb-XXXXXX
2. Open browser to:
   http://192.168.4.1
3. Enter WiFi settings
```

**How to Access Manually:**
- Perform a factory reset (see below)

---

### Config Mode

**When It Happens Automatically:**
- After WiFi is configured but image URL is missing (Step 2 of setup)
- Immediately after saving WiFi settings in Boot Mode

**What It's For:**
- Configuring image URL and refresh rate (Step 2)
- Changing any settings after initial setup
- Factory reset
- Viewing current configuration

**Display Shows:**
```
Config Mode
Active for 5 minutes

Open browser to:
  http://192.168.1.XXX
```

**How to Enter Manually:**

#### Inkplate 5 V2 / 10 / 6 Flick (Devices with Wake Button)

1. **Press and hold** the wake button
2. **Keep holding** for at least 2.5 seconds
3. **Release** the button
4. **Device enters config mode** and displays the URL

**Tips:**
- Make sure you hold long enough (count to 3)
- The button should feel like a deliberate action
- If it doesn't work, try holding slightly longer

#### Inkplate 2 (No Wake Button)

1. **Wait** for the device to complete at least one sleep cycle (after initial setup)
2. **Press the small reset button** on the board (usually near the USB port)
3. **Device restarts** and enters config mode automatically

**Important Notes:**
- Reset button only works after the device has slept at least once
- On first power-on, reset button will just restart the device
- This is the **only** way to enter config mode on Inkplate 2

**Config Mode Timeout:**
- Config mode automatically exits after **5 minutes** of inactivity
- Device will return to sleep or continue normal operation
- To re-enter, use the button/reset method again

---

### Normal Operation Mode

**When It Happens Automatically:**
- After completing both setup steps (WiFi + Image URL)
- After exiting config mode (timeout or restart)
- On every timer-based wake-up

**What It's For:**
- Regular, automatic image updates
- Normal day-to-day operation
- Power-efficient operation

**How to Enter:**
- Just wait! After configuration is complete, the device automatically operates in this mode
- Or wait for config mode to timeout (2 minutes)

---

## Troubleshooting

### Can't See the WiFi Network (Boot Mode)

**Problem**: The WiFi network `inkplate-dashb-XXXXXX` doesn't appear in your WiFi list.

**Solutions:**
1. **Wait 30-60 seconds** after powering on - it takes time to start the access point
2. **Check the display** - make sure it shows "Setup - Step 1" with WiFi instructions
3. **Move closer** - e-ink devices have limited WiFi range in AP mode
4. **Restart the device** - unplug and replug the USB cable or battery
5. **Check your phone/computer** - ensure WiFi is enabled and scanning for networks

### Can't Access Configuration Page (http://192.168.4.1)

**Problem**: Browser shows "Can't reach this page" or similar error.

**Solutions:**
1. **Verify connection** - make sure you're connected to the Inkplate's WiFi network
2. **Use http://** - do NOT use `https://` (it won't work)
3. **Disable mobile data** - on phones, mobile data can interfere
4. **Try a different browser** - some browsers have issues with local IPs
5. **Check the address** - make sure you typed `192.168.4.1` exactly (no www, no extra text)
6. **Wait a moment** - the web server may need a few seconds to start after WiFi connects

### Can't Connect to My WiFi

**Problem**: After entering WiFi credentials, the device can't connect to your network.

**Solutions:**
1. **Check the password** - WiFi passwords are case-sensitive
2. **Verify network name** - SSID is also case-sensitive
3. **Check WiFi band** - ESP32 only supports 2.4GHz WiFi (not 5GHz)
4. **Check router settings** - ensure 2.4GHz is enabled and not hidden
5. **Move closer to router** - weak signal can prevent connection
6. **Check MAC filtering** - if enabled on your router, add the device's MAC address
7. **Try guest network** - if your main network has special security settings

**Checking Connection:**
- Enable debug mode in config to see connection status
- Serial monitor (115200 baud) shows detailed connection attempts
- Look for error messages like "Wrong password" or "Network not found"

### Image Won't Download

**Problem**: Display shows "Image Error!" or blank screen after trying to download.

**Solutions:**
1. **Check image URL** - must be complete URL starting with `http://` or `https://`
2. **Test URL in browser** - open the URL on your computer to verify it works
3. **Check image format** - must be PNG (JPG/GIF not supported)
4. **Verify image size and orientation** - must match your screen exactly in the orientation you've configured (see Configuration Options)
5. **Check rotation setting** - if you changed Screen Rotation, make sure your image matches (landscape images for 0°/180°, portrait for 90°/270°)
6. **Check network access** - image must be accessible from your WiFi network
7. **Try HTTP instead of HTTPS** - some HTTPS certificates cause issues
8. **Check server** - ensure the web server hosting the image is online and responsive

**Common URL Mistakes:**
- ❌ `www.example.com/image.png` - missing `http://`
- ❌ `example.com/image.jpg` - wrong file format
- ❌ `http://localhost/image.png` - not accessible from device's network
- ✅ `http://example.com/images/dashboard.png` - correct format

### Manual Refresh Doesn't Work

**Problem**: Pressing the button doesn't trigger an immediate update.

**For Devices with Wake Button (Inkplate 5 V2, 10, 6 Flick):**

**Solutions:**
1. **Press quickly** - it should be a short tap, not a long hold
2. **Make sure device is sleeping** - can't refresh if already awake
3. **Check button connection** - ensure button is properly connected (GPIO 36)
4. **Wait for completion** - previous update must finish before new one starts
5. **Look for feedback** - display should show "Manual Refresh" message

**For Inkplate 2:**
- This device does NOT have a wake button
- Manual refresh is not available
- Use automatic refresh cycle or reduce refresh rate

### Can't Enter Config Mode

**Problem**: Long button press or reset button doesn't enter config mode.

**For Inkplate 5 V2 / 10 / 6 Flick:**

**Solutions:**
1. **Hold longer** - must hold for full 2.5 seconds (count to 3)
2. **Make sure device is configured** - config mode requires initial setup to be complete
3. **Check button** - ensure wake button is working properly
4. **Look at display** - should show "Config Mode Active" with URL
5. **Don't use reset button** - these devices only respond to wake button long press

**For Inkplate 2:**

**Solutions:**
1. **Complete initial setup first** - reset button only works after device has slept once
2. **Press the correct button** - small reset button, usually near USB port
3. **Wait for device to sleep** - must complete one full cycle before reset button works for config
4. **Check display** - should show config mode URL after reset

### Config Mode Times Out

**Problem**: Config mode exits before you finish configuring.

**Solutions:**
1. **Work faster** - you have 2 minutes from entering config mode
2. **Prepare settings** - have your image URL and settings ready before entering config mode
3. **Re-enter config mode** - just use the button/reset method again
4. **Stay on the page** - navigating away may trigger timeout
5. **No timeout on initial setup** - Step 2 (auto config mode) doesn't have a timeout

### Battery Life Seems Short

**Problem**: Device battery drains faster than expected.

**Solutions:**
1. **Increase refresh rate** - longer intervals = longer battery life
   - 5 minutes: ~1-2 months
   - 15 minutes: ~2-4 months
   - 60 minutes: ~6-12 months
2. **Check WiFi signal** - weak signal uses more power for connection
3. **Optimize image size** - smaller files download faster = less power
4. **Disable debug mode** - reduces display refreshes
5. **Check for errors** - repeated failed downloads use more power
6. **Use quality battery** - cheap batteries may have lower actual capacity

### Display Shows Old Image

**Problem**: Display isn't showing the latest version of your image.

**Solutions:**
1. **Check source** - verify the image file on your server is actually updated
2. **Browser cache** - your server might be caching the image
3. **Add cache-busting** - add `?v=timestamp` to URL to force fresh download
4. **Wait for next cycle** - device only updates on schedule
5. **Trigger manual refresh** - use short button press (if available)
6. **Check refresh rate** - verify it's set to desired interval

### Device Keeps Entering Config Mode

**Problem**: Device enters config mode when you don't expect it.

**Solutions:**
1. **Inkplate 2 - Power cycle issue**: 
   - Wait longer between unplugging and replugging (at least 10 seconds)
   - Capacitors may maintain power briefly, making device think reset was pressed
2. **Accidental button press**: Make sure nothing is pressing the button
3. **Button stuck**: Check if wake button is physically stuck
4. **Complete configuration**: Ensure image URL is saved properly
5. **Wait for timeout**: Config mode exits automatically after 2 minutes

### Factory Reset Needed

**Problem**: Device is misconfigured or you need to start over completely.

**How to Factory Reset:**

1. **Enter config mode**:
   - **Inkplate 5 V2/10/6 Flick**: Long press wake button (2.5+ seconds)
   - **Inkplate 2**: Press reset button (after device has slept once)

2. **Open the configuration page** in your web browser (URL shown on display)

3. **Scroll to the bottom** to find the "Danger Zone" section

4. **Click "Factory Reset"** button

5. **Confirm** the action (this cannot be undone!)

6. **Device erases all settings**:
   - WiFi credentials deleted
   - Image URL deleted
   - Refresh rate reset to default
   - MQTT settings deleted
   - All preferences cleared

7. **Device restarts** in Boot Mode (AP mode) - ready for fresh setup

**What Gets Erased:**
- ✅ All WiFi settings
- ✅ Image URL
- ✅ Refresh rate
- ✅ MQTT configuration
- ✅ Debug mode setting
- ✅ Device state tracking

**What Doesn't Change:**
- ❌ Firmware version (stays the same)
- ❌ VCOM setting (display calibration voltage - stored in TPS65186 PMIC)

### VCOM Management (Advanced)

**⚠️ WARNING: This is an advanced feature. Only use if you know your panel's correct VCOM value.**

**What is VCOM?**

VCOM (Common Voltage) is a critical voltage setting that controls e-ink display contrast and quality. Each e-ink panel has an optimal VCOM value typically printed on a label on the panel itself.

**When to Use:**
- You're replacing the e-ink panel with a different model
- The panel manufacturer specifies a different VCOM value
- Display quality issues persist after trying other troubleshooting

**How to Access:**

1. **Enter config mode** (long press wake button or press reset button)
2. **Open the configuration page** in your web browser
3. **Scroll to the "Danger Zone" section**
4. **Click "⚠️ VCOM Management"** button
5. **View current VCOM** (displayed on the page)
6. **To program new value**:
   - Enter the voltage (e.g., "-1.53")
   - Click "Program VCOM"
   - Read and confirm the warning
   - Wait for verification

**Important Notes:**
- Programming an incorrect VCOM can damage your display
- Only change if you have the correct value from the panel label or manufacturer
- Valid range: -5.0V to 0V (typically -1.0V to -3.0V)
- The value persists in the TPS65186 PMIC EEPROM (not affected by factory reset)
- See the [Configuration Portal Guide](config_portal_guide.md#vcom-management) for detailed instructions

- ❌ Board type
- ❌ MAC address

### Getting More Help

If you're still having issues:

1. **Check serial output**:
   - Connect USB cable
   - Use a serial monitor at 115200 baud
   - Look for error messages during operation

2. **Enable debug mode**:
   - Shows status messages on display
   - Helps identify where issues occur

3. **Check the documentation**:
   - [GitHub Repository](https://github.com/jantielens/inkplate-dashboard)
   - [MODES.md](MODES.md) - Technical details about device modes
   - [README.md](README.md) - Developer documentation

4. **Report an issue**:
   - [GitHub Issues](https://github.com/jantielens/inkplate-dashboard/issues)
   - Include firmware version, board type, and detailed description
   - Serial output logs are very helpful!

---

## Tips for Best Results

### Image Preparation

Getting the best results from your e-ink display starts with properly preparing your image. The most important requirement is using the exact resolution that matches your screen size **in the orientation you've configured** - images must be pixel-perfect or they won't display correctly. 

**Important:** Images must be pre-rotated to match your Screen Rotation setting:
- **0° or 180° (Landscape)**: Provide landscape-oriented images (e.g., 1280×720 for Inkplate 5 V2)
- **90° or 270° (Portrait)**: Provide portrait-oriented images (e.g., 720×1280 for Inkplate 5 V2)

The device does **not** rotate images on-device for performance reasons. If you change your rotation setting, you'll need to update your image source to provide images in the new orientation.

Before uploading your image, test the URL in a web browser to verify it's accessible and displays properly.

Your image can be hosted on a local web server on your home network, such as a Home Assistant installation, or on a public web server accessible from the internet. The device doesn't care where the image comes from as long as it can reach it over the network. Make sure to use high-contrast images, as this improves readability on e-ink displays which excel at showing clear, sharp text and graphics. While you can include fine details in your images, keep in mind that e-ink displays have physical resolution limitations, so extremely intricate elements might not render as crisply as on an LCD screen.

### Power Management

Finding the right balance between update frequency and battery life is important for untethered operation. A good starting point is a 15-minute refresh interval, which provides a nice balance between seeing reasonably current information and maximizing battery life. If you notice your battery draining faster than expected, try increasing the interval to 30 or 60 minutes. On the other hand, if battery life isn't a concern because you have a permanent power source, you can decrease the interval for more frequent updates - though going below 5 minutes isn't generally recommended.

If you've enabled MQTT integration with Home Assistant, you can use it to monitor your battery voltage over time and track the health of your battery. Using quality batteries makes a significant difference - cheap batteries often have lower actual capacity than advertised, which can lead to disappointing battery life even with conservative refresh settings.

### WiFi Optimization

Getting the best WiFi performance from your device helps ensure reliable updates and longer battery life. Placing your device within good range of your router means stronger signal strength, which translates to faster connections and less power consumption. Remember that the ESP32 chip in your Inkplate only supports 2.4GHz WiFi networks, not 5GHz, so make sure your router has 2.4GHz enabled.

If your router is in a congested area with many WiFi networks, choosing less crowded channels can improve performance. A stable WiFi connection is essential - intermittent connectivity causes update failures and can drain the battery as the device repeatedly attempts to reconnect. If your device needs to be located far from your router, consider using a WiFi extender to provide better coverage in that area.

### Home Assistant Integration

While the MQTT integration with Home Assistant is completely optional, many users find it useful for monitoring their device's health. The setup process is remarkably simple - you just enter your MQTT broker URL in the configuration page, and the device handles the rest through auto-discovery. This means you don't need to manually configure any YAML files in Home Assistant.

Once configured, your device will automatically appear in Home Assistant as "Inkplate Dashboard XXXXXX" with a battery voltage sensor. You can monitor battery voltage trends over time to track your battery's health and predict when it might need replacement. The integration also opens up automation possibilities, such as sending notifications when battery voltage drops below a certain threshold or tracking how long your updates take to complete.

---

## Quick Reference

### Button Functions (Inkplate 5 V2, 10, 6 Flick)

| Action | Result |
|--------|--------|
| **Short Press** (quick tap) | Manual refresh - immediate image update |
| **Long Press** (hold 2.5s) | Enter config mode - access web interface |

### Button Functions (Inkplate 2)

| Action | Result |
|--------|--------|
| **Reset Button** | Enter config mode (only after first sleep cycle) |
| _(No wake button)_ | Manual refresh not available |

### Default Settings

| Setting | Default Value |
|---------|---------------|
| Refresh Rate | 5 minutes |
| Debug Mode | Disabled |
| MQTT | Not configured |
| Config Mode Timeout | 5 minutes |

### Required Image Sizes

| Device | Resolution (Landscape) | Resolution (Portrait) | Color Mode |
|--------|----------------------|---------------------|------------|
| Inkplate 2 | 212×104 | 104×212 | 1-bit (black & white) |
| Inkplate 5 V2 | 1280×720 | 720×1280 | 3-bit (8 grayscale levels) |
| Inkplate 10 | 1200×825 | 825×1200 | 1-bit (black & white) |
| Inkplate 6 Flick | 1024×758 | 758×1024 | 3-bit (8 grayscale levels) |

**Note:** Image resolution must match your Screen Rotation setting (0°/180° = Landscape, 90°/270° = Portrait)

### Configuration URLs

| Mode | URL | When to Use |
|------|-----|-------------|
| Boot Mode (AP) | `http://192.168.4.1` | First setup, WiFi configuration |
| Config Mode (WiFi) | Shown on display | After WiFi setup, configuration changes |

---

## Frequently Asked Questions

**Q: Can I use JPG or GIF images?**  
A: No, only PNG format is supported.

**Q: Can I use images from Google Drive or Dropbox?**  
A: Only if you have a direct download link that ends in .png and doesn't require authentication. Regular shared links typically require a login page and won't work with the device.

**Q: How long does the battery last?**  
A: With a 3000mAh battery and 15-minute refresh interval, you can typically expect 2-4 months of battery life. Longer refresh intervals will extend battery life proportionally.

**Q: Can I use 5GHz WiFi?**  
A: No, the ESP32 chip only supports 2.4GHz WiFi networks. Make sure your router has 2.4GHz enabled.

**Q: Does the device need internet access?**  
A: The device needs network access to wherever your image is hosted. This could be a local web server on your home network (like Home Assistant) or a public server on the internet. As long as the device can reach the URL, it will work.

**Q: Can I customize what information is displayed?**  
A: The device displays whatever PNG image you provide via the URL. You'll need to generate your dashboard image externally using tools like Home Assistant, Grafana, a custom Python script, or any other method you prefer. The device simply downloads and displays whatever image it finds at the URL.

**Q: How do I change the image?**  
A: Just update the image file on your web server. The device automatically downloads the latest version during each refresh cycle - you don't need to reconfigure anything on the device itself.

**Q: Can I rotate the display for portrait mounting?**  
A: Yes! Use the Screen Rotation setting in the configuration portal to select 0° (landscape), 90° (portrait), 180° (inverted landscape), or 270° (portrait inverted). **Important:** Your images must be pre-rotated to match your chosen orientation. For example, if you select 90° portrait mode, you need to provide portrait-oriented images (e.g., 720×1280 instead of 1280×720 for Inkplate 5 V2). The device does not rotate images automatically.

**Q: What if my WiFi password changes?**  
A: Enter config mode (long button press or reset button) and update the WiFi password in the web interface.

**Q: Can I have multiple devices showing different images?**  
A: Yes! Each device is configured independently with its own image URL.

**Q: Is my WiFi password stored securely?**  
A: Yes, credentials are stored in ESP32's encrypted NVS (Non-Volatile Storage).

**Q: What happens during a power outage?**  
A: Configuration is saved in flash memory and survives power loss. Device will resume normal operation when power returns.

---

**Version**: 1.0.0  
**Last Updated**: October 16, 2025  
**Supported Firmware**: v0.1.0+
