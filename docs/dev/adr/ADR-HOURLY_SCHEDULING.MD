
# Architecture Decision Record: Hourly Scheduling with 24-bit Bitmask

## Status
Accepted

## Context
To optimize battery life, the Inkplate Dashboard needs a way to control which hours of the day the device performs updates, allowing users to skip unnecessary refreshes during inactive periods.

## Decision
Implement per-hour update scheduling using a 24-bit bitmask, where each bit represents an hour (0-23). Users can enable or disable updates for each hour via the configuration portal. The device sleeps until the next enabled hour if the current hour is disabled.

## Consequences
- Users have granular control over update times, improving battery life.
- The implementation is efficient (3 bytes of storage) and backward compatible.
- Adds complexity to configuration and update logic, but provides significant power savings.

# Hourly Scheduling Feature

## Overview

The Inkplate Dashboard supports **per-hour update scheduling** using a 24-bit bitmask. This allows users to control which hours of the day the device should perform updates, enabling significant battery savings by skipping unnecessary refreshes during inactive periods.

## Key Features

### ✅ What It Does

- **Granular Control**: Enable or disable updates for each hour (0-23)
- **Battery Optimization**: Device skips entire disabled periods by sleeping until the next enabled hour
- **NTP Time Sync**: Device automatically syncs time via NTP before checking the schedule
- **Smart Sleep**: If disabled at 3 AM and next enabled hour is 5 AM, device sleeps 2 hours instead of waking at the normal refresh interval
- **Default: All Enabled**: Backward compatible - all hours are enabled by default

### ⏰ Example Scenarios

**Scenario 1: Night Mode (Sleep 11 PM - 6 AM)**
```
Enabled hours:  6, 7, 8, ..., 22 (6 AM - 10 PM)
Disabled hours: 23, 0, 1, 2, 3, 4, 5 (11 PM - 5 AM)

Device behavior:
- 2 PM wake: Checks hour, hour is enabled → perform update → sleep 5 minutes
- 11:55 PM wake: Checks hour, hour disabled → calculate sleep until 6 AM → sleep 6 hours 5 minutes
- 5:55 AM wake: Checks hour, hour disabled → calculate sleep until 6 AM → sleep 5 minutes
- 6:00 AM wake: Checks hour, hour enabled → perform update → sleep 5 minutes
```

**Scenario 2: Weekday Work Hours (Updates during office hours only)**
```
Note: Currently the device doesn't distinguish weekdays/weekends, 
but you can manually configure favorite hours or use MQTT to periodically update the schedule.

Enabled hours: 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 (8 AM - 5 PM)
All other hours disabled.
```

## Configuration

### Via Web Portal

1. **Access the Configuration Portal**
   - Connect to the device's WiFi (or navigate to config mode via button press)
   - Open the configuration page in your browser

2. **Hourly Schedule Section**
   - You'll see 24 checkboxes labeled with hours in 24-hour format (00:00, 01:00, ..., 23:00)
   - Checkboxes are organized in a 6×4 grid for easy viewing
   - **Checked** = Updates enabled for that hour
   - **Unchecked** = Updates disabled for that hour

3. **Default Behavior**
   - All hours are **checked** by default (backward compatible)
   - This maintains current behavior for users who don't configure scheduling

4. **Save Configuration**
   - Click "Update Configuration" to save your hourly schedule
   - Device will reboot and apply the new settings

### Data Storage

The hourly schedule is stored as a **24-bit bitmask** in three NVS (Preferences) storage slots:
- `upd_hours_0`: Byte 0 = Hours 0-7 (midnight - 8 AM)
- `upd_hours_1`: Byte 1 = Hours 8-15 (8 AM - 4 PM)
- `upd_hours_2`: Byte 2 = Hours 16-23 (4 PM - midnight)

Each bit represents one hour (1 = enabled, 0 = disabled).

## How It Works

### Update Cycle Flow

```
Device wakes up
    ↓
Connect to WiFi
    ↓
Sync time via NTP (pool.ntp.org, time.nist.gov)
    ↓
Get current hour (0-23)
    ↓
Check if current hour is enabled in bitmask
    ├─ YES → Perform normal update cycle
    │         ├─ Check CRC32 (if enabled)
    │         ├─ Download image
    │         ├─ Display image
    │         ├─ Publish MQTT telemetry
    │         └─ Sleep for refresh interval (e.g., 5 minutes)
    │
    └─ NO → Calculate sleep until next enabled hour
             ├─ Find next enabled hour (wrapping at 24)
             ├─ Calculate sleep duration in minutes
             ├─ Log: "Updates disabled for this hour"
             ├─ Log: "Sleeping X hour(s) until next enabled hour"
             └─ Enter deep sleep for calculated duration
```

### Hour Calculation Algorithm

When updates are disabled for the current hour:

1. Start from next hour: `(currentHour + 1) % 24`
2. Check each hour forward, wrapping at midnight
3. Find the first enabled hour
4. Calculate sleep duration:
   - If no wraparound: `enabled_hour - current_hour` hours
   - If wraparound (next day): `(24 - current_hour) + enabled_hour` hours

**Example**:
- Current hour: 22 (10 PM)
- Enabled hours: 6-18 (6 AM - 6 PM)
- Next enabled: 6 (6 AM)
- Sleep duration: (24 - 22) + 6 = 8 hours

### Edge Cases

**1. All Hours Disabled**
- Fallback: Sleep for full refresh interval to prevent battery drain loop
- Log warning message
- *Note*: Not recommended, as device will never update

**2. Manual Refresh During Disabled Hour**
- Short button press still forces image download (ignores schedule)
- Normal refresh interval applies after download

**3. Long Button Press During Disabled Hour**
- Enters config mode (ignores schedule)

**4. Power Cycle During Disabled Period**
- Device still respects hourly schedule on boot
- If hour is disabled, device will sleep until next enabled hour

## MQTT Integration

Currently, MQTT telemetry is only published during **enabled** update hours. If you want visibility into "skipped hours," you can:

1. **Future Enhancement**: Add optional MQTT message for skipped hours (not yet implemented)
2. **Manual Schedule Updates**: Use MQTT to periodically update the hourly schedule via external scripts

## Battery Impact

### Estimated Savings

With default **5-minute refresh rate**, typical scenario:
- **Without scheduling**: ~288 updates per day (24h × 60m ÷ 5m)
- **With 8-hour sleep window** (11 PM - 6 AM): ~240 updates per day
- **Reduction**: ~48 updates skipped (~17% battery saving in update operations)

**Note**: Actual battery savings depend on:
- WiFi connection time (typically 2-5 seconds)
- Image download time (depends on server/network)
- Display refresh power draw
- Deep sleep power consumption (varies by board)

For maximum battery life, combine hourly scheduling with CRC32 change detection.

## Configuration Examples

### Example 1: Disable Nighttime (11 PM - 6 AM)

**Enabled hours**: 06-22 (6 AM through 10 PM)

Use when dashboard is in bedroom or you want to minimize nighttime activity.

### Example 2: Disable Very Early Morning (Midnight - 7 AM)

**Enabled hours**: 07-23 (7 AM through 11 PM)

Good for dashboards that aren't viewed before morning.

### Example 3: Daytime Only (9 AM - 5 PM)

**Enabled hours**: 09-17 (9 AM through 5 PM)

Useful for office environments where dashboard is only monitored during work hours.

### Example 4: No Restrictions (Default)

**Enabled hours**: 00-23 (all hours)

Default behavior - updates happen regardless of time. Maintains backward compatibility.

## Troubleshooting

### Device Not Updating at Expected Times

**Possible causes**:
1. Hour is disabled in schedule - check configuration
2. Device hasn't synced time yet - check WiFi connection
3. Previous disabled hour calculation sleeping past expected wake time

**How to verify**:
- Enable debug mode in config portal
- Watch serial output for "Hourly Schedule Check" messages
- Check which hour is reported and whether it's enabled

### Device Waking Too Frequently

**Cause**: Scheduled hour may overlap with a very short disabled period

**Example**:
- Refresh rate: 5 minutes
- Disabled: 2:00 AM - 2:15 AM
- Device waking at 2:10 AM will sleep until 2:15 AM instead of full interval

**Solution**: Disable full hour windows instead of partial hours

### Schedule Not Applied After Save

**Solution**:
1. Verify configuration was saved (should show success page)
2. Device should automatically restart after config save
3. Check that the new settings appear in config portal
4. If still not working, try factory reset and reconfigure

## Advanced Usage

### Updating Schedule via MQTT

Future enhancement opportunity: Send MQTT message to update hourly schedule dynamically without reconfiguration.

### Timezone Considerations

Currently, the device uses **UTC/GMT** time from NTP servers. If you want a specific timezone:

**Option 1** (Current): Configure your NTP server to already return local time
**Option 2** (Future): Add timezone offset configuration to device

### Combining with Other Features

**CRC32 Change Detection**:
- CRC32 skip happens *before* hourly check
- If CRC32 matches during disabled hour, still skips update
- Most battery-efficient combination

**Manual Refresh During Sleep Window**:
- Button press overrides hourly schedule
- Forces download even if hour disabled

## Implementation Details

### Code Changes

#### Configuration Structure (`common/src/config.h`)
```cpp
struct DashboardConfig {
    // ... other fields ...
    uint8_t updateHours[3];  // 24-bit bitmask (hours 0-23)
};
```

#### ConfigManager API (`common/src/config_manager.h`)
```cpp
bool isHourEnabled(uint8_t hour);              // hour: 0-23
void setHourEnabled(uint8_t hour, bool enabled);
void getUpdateHours(uint8_t hours[3]);
void setUpdateHours(const uint8_t hours[3]);
```

#### NormalModeController (`common/src/modes/normal_mode_controller.cpp`)
- Added NTP time sync after WiFi connection
- Added hourly schedule check before CRC32 check
- Added `calculateSleepUntilNextEnabledHour()` helper function
- Smart sleep duration calculation

#### ConfigPortal (`common/src/config_portal.cpp`)
- Added 24 hourly schedule checkboxes to configuration form
- Parses checkbox form data into 24-bit bitmask
- Displays current schedule when editing configuration

## Performance Notes

- **Time Sync**: ~1 second for NTP sync (included in normal update time)
- **Memory**: 3 bytes for bitmask (negligible)
- **Flash**: ~100 bytes additional code for schedule logic
- **No performance impact** on update operations (only affects wake-up decision)

## Future Enhancements

Possible improvements not currently implemented:

1. **Weekday-specific schedules**: Different hours for weekdays vs. weekends
2. **MQTT dynamic updates**: Change schedule without web portal
3. **Timezone support**: Automatic timezone offset configuration
4. **Partial hour scheduling**: Enable/disable specific half-hours
5. **Schedule templates**: Pre-defined profiles (Night Mode, Office Hours, etc.)
6. **MQTT telemetry for skipped hours**: Send message when hour is disabled

## FAQ

**Q: Does hourly scheduling work with button wake?**
A: No, pressing the button during a disabled hour will trigger an immediate update, bypassing the schedule.

**Q: What timezone does the device use?**
A: UTC/GMT from NTP servers. Current time is obtained via `localtime()` which uses UTC.

**Q: Can I have different schedules for different days?**
A: Not currently - the schedule is the same every day. Future enhancement could add weekday distinction.

**Q: Does hourly scheduling override CRC32 checking?**
A: No, they work together. CRC32 is checked *after* the hourly check, so if hour is disabled, CRC32 check is skipped entirely.

**Q: What happens if all 24 hours are disabled?**
A: Device will still sleep at normal refresh intervals. Not recommended, as device becomes non-functional.

**Q: Can I set different refresh rates for different hours?**
A: No, refresh rate is global. Hourly scheduling only controls whether to update or skip.

---

## See Also

- [Architecture Documentation](ARCHITECTURE.md) - System design and components
- [Modes](MODES.md) - Detailed documentation of operating modes
- [Configuration](IMPLEMENTATION_NOTES.md) - Implementation notes and design decisions
