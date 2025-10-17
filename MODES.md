# Device Modes and Configuration

## Overview

The Inkplate Dashboard firmware operates in different modes depending on the device state and user input. This document explains the various modes, how to enter them, and the technical implementation details.

---

## Operating Modes

### 1. Normal Operation Mode

**Purpose**: Regular operation where the device refreshes the dashboard image at configured intervals.

**Behavior**:
- Wakes up from deep sleep based on refresh rate timer
- Connects to configured WiFi network
- Downloads image from configured URL
- Displays image on e-ink screen
- Optionally publishes battery status via MQTT
- Returns to deep sleep until next refresh

**When entered**:
- Device is fully configured (WiFi and Image URL set)
- Wake reason is timer-based (scheduled refresh)
- Wake reason is short button press (immediate update)

---

### 2. AP Mode (Boot Mode)

**Purpose**: Initial setup when device has no WiFi configuration.

**Behavior**:
- Creates a WiFi Access Point (AP) with name like "Inkplate-XXXXXX"
- Serves configuration web portal at `http://192.168.4.1`
- Allows user to configure WiFi credentials
- Displays setup instructions on e-ink screen
- Runs indefinitely until WiFi is configured

**When entered**:
- First boot with no WiFi configuration stored
- Wake reason is `WAKEUP_FIRST_BOOT` and no WiFi credentials exist

**Display shows**:
```
Setup - Step 1
Connect WiFi

1. Connect to WiFi:
   Inkplate-XXXXXX
2. Open browser to:
   http://192.168.4.1
3. Enter WiFi settings
```

---

### 3. Config Mode

**Purpose**: Modify device settings after initial setup.

**Behavior**:
- Connects to configured WiFi network
- Serves configuration web portal at device's IP address
- Allows updating Image URL, refresh rate, MQTT settings, and firmware (OTA)
- Times out after 5 minutes of inactivity
- Displays portal URL and timeout info on screen

**When entered**:
- Device has WiFi but missing Image URL (partial configuration)
- Long button press detected (≥2.5 seconds)
- Reset button pressed after device has been running
- User manually requests config mode

**Display shows**:
```
Config Mode
Active for 5 minutes

Open browser to:
http://192.168.1.XXX
```

---

## Entering Configuration Mode

### Device-Independent Methods

There are no truly device-independent methods, as each board type has different hardware capabilities. See device-specific instructions below.

---

## Device-Specific Instructions

### Inkplate 2

**Hardware**:
- ❌ No wake button (GPIO wake not available)
- ✅ Has hardware reset button
- Screen: 212×104 pixels, 1-bit mode

**Entering Config Mode**:

**Method: Reset Button (Only Option)**
1. Wait for device to complete at least one sleep cycle
2. Press the small reset button on the board
3. Device will enter config mode automatically

**Why only reset button?**
The Inkplate 2 does not have a physical wake button connected to GPIO. The reset button is the only hardware button available for entering config mode.

**Note**: On first power-on, reset button will not enter config mode (no previous operation recorded). Device must complete one full cycle first.

---

### Inkplate 5 V2

**Hardware**:
- ✅ Has wake button on GPIO36
- ✅ Has hardware reset button (not used for config)
- Screen: 1280×720 pixels, 3-bit mode

**Entering Config Mode**:

**Method: Wake Button Long Press (Only Method)**
1. Press and hold the wake button (labeled on board)
2. Hold for at least 2.5 seconds
3. Release
4. Device enters config mode

**Why not use reset button?**
While the Inkplate 5 V2 has a hardware reset button, it is **intentionally disabled** for config mode entry. Users should use the wake button long press instead, as it provides a more intentional and controlled way to enter configuration.

**Reset button behavior**: Pressing the reset button will simply restart the device and continue normal operation.

---

### Inkplate 10

**Hardware**:
- ✅ Has wake button on GPIO36
- ✅ Has hardware reset button (not used for config)
- Screen: 1200×825 pixels, 1-bit mode

**Entering Config Mode**:

**Method: Wake Button Long Press (Only Method)**
1. Press and hold the wake button (large button on front)
2. Hold for at least 2.5 seconds
3. Release
4. Device enters config mode

**Why not use reset button?**
While the Inkplate 10 has a hardware reset button, it is **intentionally disabled** for config mode entry. Users should use the wake button long press instead, as it provides a more intentional and controlled way to enter configuration.

**Reset button behavior**: Pressing the reset button will simply restart the device and continue normal operation.

---

## Technical Implementation

### Wake Reason Detection

The firmware uses ESP32's built-in wake source detection to determine why the device started:

#### Wake Sources

| Wake Source | ESP32 Constant | Firmware Enum | Description |
|-------------|----------------|---------------|-------------|
| Timer | `ESP_SLEEP_WAKEUP_TIMER` | `WAKEUP_TIMER` | Scheduled refresh interval expired |
| Button (GPIO) | `ESP_SLEEP_WAKEUP_EXT0` | `WAKEUP_BUTTON` | Wake button pressed (GPIO36) |
| Reset/First Boot | `ESP_SLEEP_WAKEUP_UNDEFINED` | `WAKEUP_FIRST_BOOT` or `WAKEUP_RESET_BUTTON` | Power-on, reset button, or first boot |
| Unknown | Other | `WAKEUP_UNKNOWN` | Unexpected wake source |

#### Code Location
```cpp
// File: common/src/power_manager.cpp
WakeupReason PowerManager::detectWakeupReason()
```

---

### Button Press Type Detection

For boards with a wake button (GPIO36), the firmware distinguishes between short and long presses:

#### Detection Algorithm

1. **Check if button is currently pressed** (GPIO LOW)
2. **If pressed**: Start timer, poll GPIO every 50ms
3. **Wait for release or timeout** (2.5 second threshold)
4. **Classification**:
   - Released < 2.5s → `BUTTON_PRESS_SHORT` → Normal update
   - Held ≥ 2.5s → `BUTTON_PRESS_LONG` → Config mode

#### Code Location
```cpp
// File: common/src/power_manager.cpp
ButtonPressType PowerManager::detectButtonPressType()
```

#### Board-Specific Handling

For boards without a wake button (`HAS_BUTTON = false`):
- `isButtonPressed()` always returns `false`
- `detectButtonPressType()` always returns `BUTTON_PRESS_NONE`
- No GPIO configuration or wake source setup for button

```cpp
#if defined(HAS_BUTTON) && HAS_BUTTON == true
    pinMode(_buttonPin, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)_buttonPin, 0);
#endif
```

---

### Reset Button vs Power-On Detection

**Challenge**: Both reset button and power-on report as `ESP_RST_POWERON` on ESP32.

**Solution**: Use NVS (Non-Volatile Storage) to track device state.

**Note**: This mechanism is **only used on Inkplate 2** (button-less board). On boards with wake buttons (Inkplate 5 V2, Inkplate 10), the reset button detection is disabled in favor of wake button long press.

#### Implementation

1. **Before entering deep sleep** (all boards):
   ```cpp
   // Mark device as running (one-time write, skipped if already set)
   powerManager.markDeviceRunning();
   ```

2. **On boot with ESP_RST_POWERON** (Inkplate 2 only):
   ```cpp
   #if !defined(HAS_BUTTON) || HAS_BUTTON == false
   // Only check reset button on button-less boards
   prefs.begin("power_mgr", true);
   bool was_running = prefs.getBool("was_running", false);
   prefs.end();
   
   if (was_running) {
       // Device was running → Reset button
       prefs.putBool("was_running", false);  // Clear flag
       return WAKEUP_RESET_BUTTON;  // Enter config mode
   } else {
       // First boot or full power cycle
       return WAKEUP_FIRST_BOOT;
   }
   #else
   // Boards with wake button ignore reset for config mode
   // Continue normal operation instead
   #endif
   ```

#### Why NVS?

| Storage Type | Survives Reset Button | Survives Power Cycle | Used For |
|--------------|----------------------|---------------------|----------|
| RAM | ❌ | ❌ | Runtime variables |
| RTC Memory | ❌ | ❌ | Wake source, boot count |
| NVS Flash | ✅ | ✅ | Reset detection (Inkplate 2 only), config |

**Key insight**: Inkplate 2's reset button performs a full power cycle (clears RTC memory), but NVS persists across all resets.

**Board-specific behavior**:
- **Inkplate 2**: Uses NVS flag to detect reset button and enter config mode
- **Inkplate 5 V2 / 10**: NVS flag is set but not used for config mode entry (wake button used instead)

#### NVS Write Optimization

To maximize flash memory lifetime, the firmware uses a **one-time write strategy**:

**Instead of writing every sleep cycle:**
```cpp
// OLD approach (writes every 15 min = 96 writes/day)
prefs.putULong("last_sleep", millis());  // ❌ High wear
```

**The firmware writes once on first operation:**
```cpp
// NEW approach (writes once per power cycle)
if (!prefs.getBool("was_running", false)) {
    prefs.putBool("was_running", true);  // ✅ One-time write
}
```

**Benefits:**
- **~99% reduction in NVS writes** (from 96/day to ~1-2/month)
- **Extended device lifetime**: Decades instead of 1-3 years
- **Same functionality**: Reset detection still works perfectly

**How it works:**
1. Flag is set when device enters normal operation (first sleep)
2. Flag persists across all resets and power cycles
3. Reset button detection clears the flag
4. Next operation cycle sets it again (one write)

#### Code Location
```cpp
// File: common/src/power_manager.cpp
// - detectWakeupReason() - Checks NVS for was_running flag
// - markDeviceRunning() - Sets flag before first sleep (one-time write)

// File: common/src/main_sketch.ino.inc
// - Conditional compilation: Only Inkplate 2 uses WAKEUP_RESET_BUTTON for config mode
// - Inkplate 5 V2 / 10 ignore reset button, use wake button long press instead
```

**Note**: The `markDeviceRunning()` method is called before every deep sleep on all boards but only writes to NVS once. Subsequent calls detect the flag is already set and skip the write operation, eliminating unnecessary flash wear.

---

### Reset Reason Codes

ESP32 provides detailed reset reasons via `esp_reset_reason()`:

| Code | ESP32 Constant | Meaning |
|------|----------------|---------|
| 1 | `ESP_RST_POWERON` | Power-on reset (initial power or reset button) |
| 12 | `ESP_RST_SW` | Software reset (via `ESP.restart()`) |
| 5 | `ESP_RST_DEEPSLEEP` | Wake from deep sleep |
| 3 | `ESP_RST_EXT` | External reset pin (rare) |

#### Code Example
```cpp
esp_reset_reason_t reset_reason = esp_reset_reason();
Serial.printf("Reset reason code: %d\n", reset_reason);
```

---

### Mode Selection Logic

The firmware uses a decision tree to determine the appropriate mode:

```
setup()
  ├─> WAKEUP_RESET_BUTTON + No Wake Button (Inkplate 2)?
  │   ├─> Yes → Enter config mode
  │   └─> No → Continue (Inkplate 5 V2/10 ignore reset button)
  │
  ├─> Has WiFi config?
  │   ├─> No + WAKEUP_FIRST_BOOT → Enter AP Mode
  │   └─> Yes → Continue
  │
  ├─> Has Image URL?
  │   ├─> No → Enter config mode (partial config)
  │   └─> Yes → Continue
  │
  ├─> Detect button press type (if wake button available)
  │   ├─> BUTTON_PRESS_LONG → Enter config mode
  │   ├─> BUTTON_PRESS_SHORT + WAKEUP_BUTTON → Normal update
  │   └─> BUTTON_PRESS_NONE → Continue
  │
  └─> Fully configured?
      ├─> Yes → Normal operation mode
      └─> No → Enter AP Mode (fallback)
```

#### Code Location
```cpp
// File: common/src/main_sketch.ino.inc
void setup() {
    // Mode selection logic
}
```

---

## Configuration Persistence

Configuration is stored in ESP32's NVS (Non-Volatile Storage) using the `Preferences` library:

### Stored Settings

| Preference Key | Type | Purpose |
|----------------|------|---------|
| `wifi_ssid` | String | WiFi network name |
| `wifi_pass` | String | WiFi password |
| `image_url` | String | Dashboard image URL |
| `refresh_rate` | UInt | Minutes between updates |
| `mqtt_broker` | String | MQTT broker address |
| `mqtt_user` | String | MQTT username |
| `mqtt_pass` | String | MQTT password |
| `last_sleep` | ULong | Timestamp for reset detection |

### Namespace: `dashboard`

All settings except `last_sleep` use the `dashboard` namespace. The `last_sleep` timestamp uses the `power_mgr` namespace for separation.

### Factory Reset

Clears all stored preferences:
```cpp
ConfigManager::clearConfig() {
    Preferences prefs;
    prefs.begin("dashboard", false);
    prefs.clear();
    prefs.end();
}
```

---

## Serial Output Examples

### First Boot (No Config)
```
=================================
Starting Inkplate 2...
=================================

Reset reason code: 1
RTC boot count: 0
RTC was running: false
Last sleep timestamp: 0
Current millis: 234
No previous sleep recorded - initial power-on
PowerManager initialized (no button)
Current wakeup reason: FIRST_BOOT (initial setup)

Device NOT configured - entering AP mode (boot mode)
```

### Reset Button Press (After Sleep)
```
=================================
Starting Inkplate 2...
=================================

Reset reason code: 1
RTC boot count: 0
RTC was running: false
Last sleep timestamp: 15234
Current millis: 187
Device had previous sleep - likely reset button press
PowerManager initialized (no button)
Current wakeup reason: RESET_BUTTON (hardware reset pressed)

Hardware reset button detected - entering config mode
```

### Timer Wake (Normal Operation)
```
=================================
Starting Inkplate 5 V2...
=================================

Wakeup caused by timer
PowerManager initialized
Button pin configured: GPIO 36
Current wakeup reason: TIMER (normal refresh cycle)

Device is fully configured - entering normal operation mode
```

### Long Button Press
```
=================================
Starting Inkplate 10...
=================================

Wakeup caused by button press (EXT0)
PowerManager initialized
Button pin configured: GPIO 36
Current wakeup reason: BUTTON (config mode requested)

=================================
Detecting button press type...
=================================
Button is currently pressed, waiting to determine hold duration...
Button still held after 2500 ms - LONG PRESS detected
=================================

Long button press detected - entering config mode
```

---

## Troubleshooting

### Issue: Reset button doesn't enter config mode (Inkplate 5 V2 / 10)

**This is expected behavior!**

**Cause**: On boards with wake buttons (Inkplate 5 V2, Inkplate 10), the reset button is intentionally disabled for config mode entry.

**Solution**: Use the wake button long press (hold for 2.5+ seconds) to enter config mode. This is the only supported method for these boards.

**Why?**: The wake button provides a more intentional and controlled way to enter config mode. The reset button simply restarts the device and continues normal operation.

---

### Issue: Reset button always enters AP mode instead of config mode (Inkplate 2)

**Cause**: Device hasn't completed a sleep cycle yet (no `was_running` flag).

**Solution**: Let the device complete initial setup, download an image, and enter sleep at least once. Then reset button will work for config mode.

**Note**: This only applies to Inkplate 2, as other boards don't use the reset button for config entry.

---

### Issue: Device unexpectedly enters config mode on power-up

**Symptoms**: After unplugging and re-plugging power, device enters config mode instead of normal operation.

**Cause**: Device was unplugged only briefly. Internal capacitors maintained power to the ESP32, preserving the `last_sleep` timestamp in NVS. The device interprets this as a reset button press rather than a fresh power cycle.

**Solutions**:
1. **Wait longer**: Leave device unplugged for 10-30 seconds to ensure capacitors fully discharge
2. **Wait it out**: Config mode times out after 2 minutes and device will return to normal operation
3. **Don't unplug briefly**: If you need to power cycle, wait long enough for complete power loss

**Prevention**: To ensure clean power cycles, wait at least 10 seconds between unplugging and re-plugging power.

---

### Issue: Wake button doesn't respond

**Possible causes**:
1. Board doesn't have a wake button (Inkplate 2)
2. Wrong GPIO pin configured
3. Button not held long enough (need ≥2.5 seconds)

**Check**:
```cpp
// In board_config.h
#define HAS_BUTTON true
#define WAKE_BUTTON_PIN 36
```

**Applicable boards**: Inkplate 5 V2, Inkplate 10 only. Inkplate 2 does not have a wake button.

---

### Issue: Can't exit config mode

**Cause**: Config mode times out after 2 minutes of inactivity.

**Solution**: 
- Complete configuration within timeout period
- If timeout occurs, device returns to sleep
- Use wake button long press (Inkplate 5 V2/10) or reset button (Inkplate 2) to re-enter config mode

---

## Summary

| Feature | Inkplate 2 | Inkplate 5 V2 | Inkplate 10 |
|---------|-----------|--------------|-------------|
| **Wake Button** | ❌ | ✅ GPIO36 | ✅ GPIO36 |
| **Reset Button** | ✅ | ✅ (restart only) | ✅ (restart only) |
| **Config via Long Press** | ❌ | ✅ (Only method) | ✅ (Only method) |
| **Config via Reset** | ✅ (Only method) | ❌ Disabled | ❌ Disabled |
| **Short Press Update** | ❌ | ✅ | ✅ |
| **Reset Detection** | NVS-based | Not used | Not used |

**Recommended config method**:
- **Inkplate 2**: Use reset button (after first sleep cycle) - **only option**
- **Inkplate 5 V2**: Use wake button long press - **only option**
- **Inkplate 10**: Use wake button long press - **only option**

**Key Design Decision**: Boards with wake buttons (Inkplate 5 V2, Inkplate 10) intentionally ignore the reset button for config mode entry. This prevents accidental config mode entry and provides a more intentional user experience through the wake button long press.

