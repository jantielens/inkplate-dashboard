# Timezone Configuration Guide

## Overview

Starting with firmware version 1.4.0, the Inkplate Dashboard supports **named timezones with automatic Daylight Saving Time (DST) handling**. You no longer need to manually update your timezone offset when DST changes in your region.

## What's New

### Before (v1.3.x and earlier)

- Manual timezone offset entry (-12 to +14)
- Example: "Enter +1 for Central European Time"
- **Problem**: Users had to remember to update offset when DST changed (e.g., +1 → +2 in summer)
- Missed updates caused schedule errors

### After (v1.4.0+)

- Named timezone selection from dropdown
- Example: "Select Europe/Berlin from dropdown"
- **Benefit**: DST is applied automatically based on your region's rules
- No manual updates needed - ever!

## Supported Timezones

The firmware includes 60+ commonly used timezones:

### Europe
- UTC (Coordinated Universal Time)
- Europe/London (GMT/BST)
- Europe/Paris, Europe/Berlin, Europe/Amsterdam (CET/CEST)
- Europe/Athens, Europe/Helsinki (EET/EEST)
- Europe/Moscow (MSK - no DST since 2014)
- And 15+ more European cities

### Americas
- America/New_York (EST/EDT)
- America/Chicago (CST/CDT)
- America/Denver (MST/MDT)
- America/Los_Angeles (PST/PDT)
- America/Phoenix (MST - no DST)
- America/Toronto, America/Vancouver
- South America: São Paulo, Buenos Aires, Mexico City

### Asia & Pacific
- Asia/Tokyo, Asia/Seoul (no DST)
- Asia/Shanghai, Asia/Hong_Kong, Asia/Singapore
- Asia/Dubai, Asia/Kolkata (India)
- Australia/Sydney, Australia/Melbourne, Australia/Brisbane
- Pacific/Auckland (New Zealand)
- Pacific/Honolulu (Hawaii)

### Africa
- Africa/Cairo, Africa/Johannesburg, Africa/Nairobi, Africa/Lagos

## How to Configure

### Initial Setup (New Device)

1. Power on your Inkplate device
2. Connect to WiFi AP: `inkplate-XXXXXX`
3. Open browser to `http://192.168.4.1`
4. In the configuration form:
   - Find the **Timezone** dropdown
   - Select your timezone (e.g., "Europe/Berlin", "America/New_York")
   - Complete other settings (WiFi, image URL, etc.)
5. Click "Save Configuration"

### Updating Existing Device

1. Press and hold the button for 2+ seconds to enter config mode
2. Connect to `inkplate-XXXXXX` AP
3. Open `http://192.168.4.1`
4. Change **Timezone** dropdown to your preferred timezone
5. Click "Save Configuration"

## How It Works

### Automatic DST Calculation

The device uses **POSIX TZ strings** to automatically calculate:
- Standard time offset (e.g., UTC+1 for Central European Time)
- DST offset (e.g., UTC+2 for Central European Summer Time)
- DST transition dates (e.g., last Sunday in March → spring forward)

**Example: Europe/Berlin**
- Winter (November - March): UTC+1 (CET)
- Summer (March - October): UTC+2 (CEST)
- Transition: Automatic at 2:00 AM on last Sunday of March/October

### Hourly Schedule Compatibility

If you use hourly scheduling (e.g., only update 09:00-17:00):
- Times are based on **local time** in your selected timezone
- DST transitions are handled automatically
- No gaps or double-updates during spring/fall transitions

**Example:**
- You set schedule for 09:00-17:00 local time
- Before DST: Device wakes 08:00-16:00 UTC (09:00-17:00 CET)
- After DST: Device wakes 07:00-15:00 UTC (09:00-17:00 CEST)
- You don't change anything - it just works!

## Troubleshooting

### My timezone isn't in the list

If your timezone isn't listed:
1. Select the **closest timezone** with the same UTC offset and DST rules
2. Example: If "Europe/Stockholm" isn't listed, use "Europe/Berlin" (same rules)
3. Open a GitHub issue to request your timezone be added

### Device shows wrong time after DST transition

This should never happen with named timezones. If it does:
1. Check device logs (serial console) for timezone mode
2. Should show: `Timezone: Europe/Berlin (automatic DST)`
3. If shows `Legacy numeric offset`, reconfigure with named timezone
4. Report issue on GitHub with timezone name and date

### Upgrading from legacy offset

Devices configured with old firmware (v1.3.x) using numeric offsets:
- Will continue to work after upgrade
- Will show "Legacy numeric offset" in logs
- **Recommended**: Reconfigure with named timezone for automatic DST

To upgrade:
1. Enter config mode
2. Select named timezone from dropdown
3. Save configuration
4. Device will now use automatic DST

### Config portal doesn't show timezone dropdown

Check firmware version:
- Named timezones require firmware v1.4.0 or later
- Older versions only have numeric offset field
- Upgrade firmware via OTA or web flasher

## Technical Details

### Storage

Timezone is stored in NVS (Non-Volatile Storage) as:
- **Key**: `tz_name`
- **Value**: String (e.g., "Europe/Berlin", "America/New_York")
- **Default**: "UTC" (if not configured)

### POSIX TZ Format

Behind the scenes, each timezone maps to a POSIX TZ string:
- **Format**: `std offset [dst [offset][,start[/time],end[/time]]]`
- **Example**: `CET-1CEST,M3.5.0,M10.5.0/3`
  - `CET-1`: Central European Time is UTC+1 (note: sign inverted in POSIX)
  - `CEST`: Central European Summer Time
  - `M3.5.0`: DST starts last Sunday (5th occurrence) of March (month 3)
  - `M10.5.0/3`: DST ends last Sunday of October at 3:00 AM

### Backward Compatibility

Legacy numeric offset mode is maintained for:
- Devices configured before v1.4.0
- Testing and debugging purposes
- Regions not in the timezone list (temporary workaround)

When both `tz_name` and `timezoneOffset` exist:
- Named timezone takes precedence
- Numeric offset is ignored

## Best Practices

1. **Use Named Timezones**: Always select from dropdown for automatic DST
2. **Test After DST**: Verify device wakes at correct local time after DST transition
3. **Update Firmware**: Keep firmware current for DST rule updates (rare)
4. **Report Issues**: If timezone is wrong, report with:
   - Firmware version
   - Selected timezone
   - Expected vs actual local time
   - Date/time of issue

## FAQs

**Q: Do I need to update firmware every year for DST?**
A: No. DST rules are embedded in POSIX TZ strings and rarely change. Only update if your government changes DST rules permanently.

**Q: What happens during the "spring forward" hour skip?**
A: The device automatically skips the missing hour (e.g., 2:00 AM → 3:00 AM). No duplicate wakes.

**Q: What happens during the "fall back" repeated hour?**
A: The device handles the repeated hour correctly. If hourly schedule is enabled, it may wake twice (once per occurrence of the hour).

**Q: Can I still use numeric offset?**
A: Yes, for backward compatibility. But named timezones are strongly recommended for automatic DST.

**Q: How do I know which timezone to choose?**
A: Select the major city in your timezone. Example:
- Germany → Europe/Berlin
- Eastern US → America/New_York
- California → America/Los_Angeles
- Japan → Asia/Tokyo

**Q: My country stopped observing DST. Is that supported?**
A: Yes! Examples:
- Russia: Europe/Moscow (no DST since 2014)
- Turkey: Europe/Istanbul (no DST since 2016)
- Arizona (US): America/Phoenix (no DST)

## Related Documentation

- [Configuration Portal Guide](config_portal_guide.md)
- [Hourly Scheduling Guide](hourly_scheduling.md)
- [Firmware Update Guide](../user/USING.md#ota-updates)

## Support

For issues or questions:
- GitHub Issues: https://github.com/jantielens/inkplate-dashboard/issues
- Reference Issue #33 for timezone feature discussion
