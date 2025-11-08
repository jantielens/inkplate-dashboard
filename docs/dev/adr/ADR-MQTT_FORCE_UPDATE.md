# Architecture Decision Record: MQTT Sensor force_update Logic Refactoring

## Status
Accepted and Implemented (v1.3.4)

## Context

Home Assistant uses MQTT sensors to monitor device health and telemetry. By default, Home Assistant only updates the "last seen" timestamp and entity state when a **new value** arrives. If a sensor consistently reports the same value (e.g., stable battery level, constant WiFi signal strength, retry count of zero), Home Assistant may appear to 'lose' connection to the sensor between state changes, even though the device is actively reporting.

### Original Implementation (v1.3.3 and earlier)

The `publishSensorDiscovery()` function in `mqtt_manager.cpp` used a **whitelist approach** for the `force_update` property:

```cpp
// Force update for timing sensors so Home Assistant updates "last seen" even with same value
if (sensorType == "loop_time" || 
    sensorType == "loop_time_wifi" || 
    sensorType == "loop_time_ntp" || 
    sensorType == "loop_time_crc" || 
    sensorType == "loop_time_image") {
    payload += "\"force_update\":true,";
}
```

**Only 5 sensors** had `force_update: true`:
- `loop_time` - Total loop duration
- `loop_time_wifi` - WiFi connection time
- `loop_time_ntp` - NTP sync time
- `loop_time_crc` - CRC check time
- `loop_time_image` - Image download/display time

**8 sensors lacked `force_update`:**
- `battery_voltage` - Battery voltage in volts
- `battery_percentage` - Battery percentage (0-100)
- `wifi_signal` - WiFi RSSI in dBm
- `wifi_bssid` - WiFi access point BSSID/MAC
- `image_crc32` - CRC32 of displayed image
- `loop_time_wifi_retries` - WiFi connection retries (0-4)
- `loop_time_crc_retries` - CRC32 check retries (0-2)
- `loop_time_image_retries` - Image download retries (0-2)

### Problems with the Whitelist Approach

1. **Hidden Device Health Issues**
   - A device with stable battery voltage reporting every hour would appear inactive in Home Assistant
   - Users could not distinguish between "device stopped reporting" vs "value hasn't changed"
   - "Last seen" timestamp only updated when voltage changed, which could be days/weeks

2. **Invisible Success States**
   - Retry counts are often **zero** (successful first attempts)
   - Without `force_update`, a retry count of 0 would never update Home Assistant
   - Users couldn't see that network operations were succeeding reliably

3. **Confusion and Debugging Difficulty**
   - WiFi signal strength is typically stable within a location
   - Same BSSID for weeks/months is normal
   - These stable values made sensors appear "stuck" or "dead"
   - Debugging required checking device logs instead of Home Assistant dashboard

4. **Maintenance Burden**
   - Every new diagnostic sensor required updating the whitelist
   - Easy to forget `force_update` when adding new telemetry
   - Default behavior was incorrect for most sensors

### Home Assistant `force_update` Property

From Home Assistant documentation:

> `force_update` (boolean, optional, default: false)  
> Sends update events even if the value hasn't changed. Useful if you want to have meaningful value graphs in history or want to create an automation that triggers on **every update** rather than only on state change.

**Use cases:**
- Device health monitoring (updates indicate device is alive)
- Success state visibility (zero retries is success)
- Continuous telemetry (even stable values are important)

**When NOT to use:**
- Event-like sensors (discrete, non-repeating events)
- Sensors where repetition is meaningless

## Decision

Refactor `publishSensorDiscovery()` to use a **blacklist approach**: apply `force_update: true` to **all sensors** except those representing **discrete events** (currently only `last_log`).

### Implementation (v1.3.4)

```cpp
// Force update for all sensors EXCEPT event-like sensors (last_log)
// This ensures Home Assistant updates "last seen" timestamp even when values don't change
// Benefits:
// - Reliable device health monitoring (stable battery/WiFi values still show device is alive)
// - Retry fields (often zero) are visible as successful states
// - Reduces confusion when stable sensors appear inactive but are actively reporting
// Event sensors like last_log are excluded because they represent discrete events,
// not continuous telemetry, and repeating the same event message is not useful
if (sensorType != "last_log") {
    payload += "\"force_update\":true,";
}
```

### Affected Sensors

**Sensors now having `force_update: true` (8 new + 5 existing = 13 total):**

| Sensor Type | Category | Typical Behavior | Why `force_update` Helps |
|-------------|----------|------------------|-------------------------|
| `battery_voltage` | Power | Stable for hours/days | Health monitoring - device is alive |
| `battery_percentage` | Power | Stable for hours/days | Health monitoring - device is alive |
| `wifi_signal` | Network | Stable within location | Health monitoring - WiFi is working |
| `wifi_bssid` | Network | Stable for weeks/months | Shows which AP is in use |
| `image_crc32` | Display | Stable between image changes | Shows display is updating on schedule |
| `loop_time_wifi_retries` | Diagnostics | Often 0 (success) | Shows successful first attempts |
| `loop_time_crc_retries` | Diagnostics | Often 0 (success) | Shows successful first attempts |
| `loop_time_image_retries` | Diagnostics | Often 0 (success) | Shows successful first attempts |
| `loop_time` | Timing | Varies, but can be stable | Already had `force_update` |
| `loop_time_wifi` | Timing | Often stable | Already had `force_update` |
| `loop_time_ntp` | Timing | Often 0-0.01s | Already had `force_update` |
| `loop_time_crc` | Timing | Can be stable | Already had `force_update` |
| `loop_time_image` | Timing | Can be stable | Already had `force_update` |

**Sensor excluded from `force_update` (1 sensor):**

| Sensor Type | Category | Why Excluded |
|-------------|----------|--------------|
| `last_log` | Event | Discrete events - repeating same error message is not useful |

## Rationale

### Why Blacklist is Better

1. **Correct Default Behavior**
   - Most telemetry sensors benefit from `force_update`
   - New sensors automatically get correct behavior
   - No maintenance burden when adding sensors

2. **Explicit Exception Handling**
   - Event-like sensors are the exception, not the rule
   - `last_log` clearly represents discrete events
   - Easy to add more exceptions if needed (unlikely)

3. **Simpler Code**
   - One condition vs. five conditions
   - Easier to understand intent
   - Less error-prone

### Why `last_log` is Excluded

The `last_log` sensor represents **discrete events**, not continuous telemetry:

- **Event**: `[ERROR] Failed to download image` (happened once at 3:15 PM)
- **Not telemetry**: Not a measurement that should update every cycle

Repeating the same log message every refresh cycle (e.g., every hour) would:
- Create false impression that errors are recurring
- Clutter Home Assistant history with duplicate events
- Not provide additional diagnostic value

If a device is reporting the same error every cycle, that's a separate issue that should be fixed, not masked by `force_update`.

### Alternative Approaches Considered

#### Alternative 1: Add `force_update` to specific sensors
**Rejected**: Requires updating whitelist for each new sensor, defeats the purpose of the refactoring

#### Alternative 2: Add `force_update` to all sensors (including `last_log`)
**Rejected**: Would cause confusing duplicate event messages in Home Assistant

#### Alternative 3: Make `force_update` configurable per sensor
**Rejected**: Adds complexity without clear benefit; users shouldn't need to understand this implementation detail

## Consequences

### Positive

1. **Reliable Device Health Monitoring**
   - Home Assistant "last seen" timestamp updates every cycle
   - Users can immediately see if a device stops reporting
   - Stable values (battery, WiFi) still show device is alive

2. **Success State Visibility**
   - Retry counts of zero are visible as successful states
   - Users can see network operations are working reliably
   - Proactive monitoring of network health

3. **Reduced User Confusion**
   - Stable sensors no longer appear "stuck" or "dead"
   - Clear distinction between "value unchanged" vs "device offline"
   - Better diagnostic experience

4. **Simplified Maintenance**
   - Future diagnostic sensors automatically work correctly
   - No need to update whitelist when adding sensors
   - Less error-prone implementation

5. **Backward Compatible**
   - No breaking changes to Home Assistant entities
   - Existing entities simply start updating more reliably
   - No user action required

### Negative

1. **Slightly Increased MQTT Traffic**
   - Home Assistant processes updates even when values don't change
   - **Impact**: Minimal - MQTT messages are small (~50-200 bytes)
   - **Mitigation**: Only publishes during active refresh cycles (not continuous streaming)

2. **Increased Home Assistant Database Size**
   - More state changes recorded in history
   - **Impact**: Small - only 13 sensors × ~1 update/hour = ~13 entries/hour
   - **Mitigation**: Home Assistant auto-purges old history (default 10 days)

### Neutral

1. **No Impact on Device Battery**
   - MQTT publish count unchanged (same sensors, same frequency)
   - Only changes Home Assistant behavior, not device behavior

2. **No Impact on Firmware Size**
   - Code is actually simpler (fewer conditions)
   - Negligible change in compiled binary size

## Implementation Notes

### Code Location
- **File**: `common/src/mqtt_manager.cpp`
- **Function**: `MQTTManager::publishSensorDiscovery()`
- **Lines**: 549-559 (v1.3.4)

### Discovery vs State Messages
This change only affects **discovery messages** (Home Assistant auto-discovery configuration). State messages (actual sensor values) are published regardless of `force_update`.

**Discovery message example:**
```json
{
  "name": "Battery Voltage",
  "unique_id": "inkplate_abc123_battery_voltage",
  "state_topic": "homeassistant/sensor/inkplate_abc123/battery_voltage/state",
  "device_class": "voltage",
  "unit_of_measurement": "V",
  "force_update": true,  // <-- This is what changed
  "value_template": "{{ value }}",
  "device": { ... }
}
```

### When Discovery is Published
Discovery messages are only published on:
- **First boot** (`WAKEUP_FIRST_BOOT`)
- **Reset button** (`WAKEUP_RESET_BUTTON`)

They are **NOT** published on:
- **Timer wake** (`WAKEUP_TIMER`) - most common
- **Button wake** (`WAKEUP_BUTTON`)

This minimizes MQTT traffic and retained message count (see `shouldPublishDiscovery()` in `mqtt_manager.cpp`).

### Testing Considerations

**Manual Testing:**
1. Configure MQTT broker in device
2. Check Home Assistant entity states every refresh cycle
3. Verify "last seen" timestamp updates even when values don't change
4. Verify retry counts of 0 are visible
5. Verify `last_log` doesn't duplicate events

**CI/CD Testing:**
- Not applicable - this is a runtime Home Assistant integration feature
- Compilation verified by standard build workflow
- Logic verified by code review

## Related Documentation

- **CHANGELOG**: v1.3.4 entry documents user-visible changes
- **ADR-NETWORK_TIMEOUT_OPTIMIZATION.md**: Documents retry telemetry sensors (v1.3.3)
- **User Guide** (`docs/user/USING.md`): MQTT broker configuration

## Version History

- **v1.3.4** (2025-11-08): Refactored to blacklist approach
- **v1.3.3** (2025-11-07): Added retry telemetry sensors (still whitelist)
- **v1.3.0 and earlier**: Original whitelist for timing sensors only

## Future Considerations

### Adding New Event-Like Sensors

If future sensors represent **discrete events** (not continuous telemetry), add them to the exclusion condition:

```cpp
if (sensorType != "last_log" && sensorType != "new_event_sensor") {
    payload += "\"force_update\":true,";
}
```

**Criteria for exclusion:**
- Represents a discrete event (happened at a specific time)
- Repeating the same message is not useful
- Not a measurement or telemetry value

**Examples that should be excluded:**
- Error events
- User actions/button presses
- One-time status changes

**Examples that should NOT be excluded:**
- Any numeric measurement (voltage, temperature, count, etc.)
- Any diagnostic value (retry counts, timing, etc.)
- Any status indicator that represents current state

### Configurable `force_update` per Sensor

If users request the ability to disable `force_update` for specific sensors, consider adding a configuration option. However, this adds complexity and is unlikely to be needed.

## References

- **Home Assistant MQTT Discovery**: https://www.home-assistant.io/integrations/mqtt/#discovery
- **Home Assistant Sensor force_update**: https://www.home-assistant.io/integrations/sensor.mqtt/#force_update
- **Issue**: Refactor MQTT sensor force_update logic for improved Home Assistant diagnostics
