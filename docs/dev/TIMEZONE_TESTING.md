# Timezone Feature Testing Guide

## Overview

This document provides testing guidance for the named timezone feature with automatic DST calculation (v1.4.0).

## Feature Summary

- **What Changed**: Replaced manual numeric timezone offset (-12 to +14) with named timezone selection
- **User Benefit**: Automatic DST handling - users never need to manually update timezone settings
- **Implementation**: Uses POSIX TZ strings with `setenv("TZ")` and `tzset()` for DST-aware time
- **Backward Compatibility**: Legacy numeric offset mode still works (defaults to UTC)

## Testing Scenarios

### 1. Fresh Device Configuration

**Test**: Configure a new device with a named timezone

**Steps**:
1. Power on unconfigured device
2. Connect to AP mode (inkplate-XXXXXX)
3. Open config portal (http://192.168.4.1)
4. Select timezone from dropdown (e.g., "Europe/Berlin")
5. Configure WiFi and image URL
6. Submit configuration
7. Wait for device to wake and check logs

**Expected Result**:
- Config portal shows timezone dropdown with 60+ options
- Selected timezone is saved to NVS as `tz_name`
- Device logs show: `Timezone: Europe/Berlin (automatic DST)`
- Local hour is calculated correctly based on timezone

### 2. DST Transition Testing

**Test**: Verify correct hour calculation before/during/after DST transition

**Timezones to Test**:
- **Europe/Berlin**: DST starts last Sunday in March, ends last Sunday in October
- **America/New_York**: DST starts 2nd Sunday in March, ends 1st Sunday in November
- **Australia/Sydney**: DST starts 1st Sunday in October, ends 1st Sunday in April (southern hemisphere)
- **Asia/Tokyo**: No DST (control test)

**Steps**:
1. Set device timezone to test zone
2. Simulate or wait for DST transition date
3. Check logs for correct local hour before/after transition

**Expected Result**:
- Hour calculation automatically adjusts at DST boundary
- No manual intervention required
- Hourly schedule respects new local time

### 3. Backward Compatibility Testing

**Test**: Verify devices with legacy numeric offset still work

**Steps**:
1. Configure device using old firmware (v1.3.x or earlier) with numeric offset
2. Upgrade to v1.4.0
3. Device should continue working with legacy offset
4. Check logs

**Expected Result**:
- Device logs show: `Mode: Legacy numeric offset`
- Timezone offset applied manually as before
- No errors or crashes
- User can upgrade to named timezone via config portal

### 4. Timezone Validation

**Test**: Verify all 60+ timezones in dropdown work correctly

**Sample Test Cases**:

| Timezone | POSIX TZ String | UTC Offset | DST? | Test Date | Expected Hour (UTC 12:00) |
|----------|----------------|------------|------|-----------|---------------------------|
| UTC | UTC0 | +0 | No | Any | 12 |
| Europe/London | GMT0BST,M3.5.0/1,M10.5.0 | +0/+1 | Yes | 2025-07-01 | 13 (BST) |
| Europe/Paris | CET-1CEST,M3.5.0,M10.5.0/3 | +1/+2 | Yes | 2025-07-01 | 14 (CEST) |
| America/New_York | EST5EDT,M3.2.0,M11.1.0 | -5/-4 | Yes | 2025-07-01 | 08 (EDT) |
| Asia/Tokyo | JST-9 | +9 | No | Any | 21 |
| Australia/Sydney | AEST-10AEDT,M10.1.0,M4.1.0/3 | +10/+11 | Yes | 2025-01-01 | 23 (AEDT) |

### 5. Hourly Schedule with DST

**Test**: Verify hourly schedule respects DST transitions

**Scenario**: Device configured with hourly schedule (e.g., updates 09:00-17:00 local time)

**Steps**:
1. Configure device with Europe/Berlin timezone
2. Set hourly schedule: enable only hours 9-17
3. Simulate DST transition (e.g., March 31, 2025 - spring forward at 2:00 AM)
4. Verify device still wakes at 09:00-17:00 local time (adjusted for DST)

**Expected Result**:
- Before DST (March 30): Wakes at 08:00-16:00 UTC (09:00-17:00 CET)
- After DST (March 31): Wakes at 07:00-15:00 UTC (09:00-17:00 CEST)
- No gaps or double-wakeups during transition

### 6. Config Portal UI Testing

**Test**: Verify config portal timezone dropdown

**Steps**:
1. Open config portal
2. Check timezone dropdown
3. Select different timezones
4. Submit form
5. Reload portal

**Expected Result**:
- Dropdown shows 60+ timezone options in logical order (UTC first, then by region)
- Selected timezone is highlighted after form submission
- Help text says: "Select your timezone. Daylight Saving Time (DST) will be applied automatically."
- No more warning about manual DST updates

### 7. Migration Testing

**Test**: Upgrade from legacy numeric offset to named timezone

**Steps**:
1. Device configured with `timezoneOffset = +1` (legacy)
2. User opens config portal
3. User selects "Europe/Berlin" from dropdown
4. Submit configuration
5. Device reboots

**Expected Result**:
- `tz_name = "Europe/Berlin"` saved to NVS
- `timezoneOffset = 0` (reset or ignored)
- Device logs show: `Mode: Named timezone`
- Local hour calculated correctly with DST

## Validation Checklist

Before releasing v1.4.0, verify:

- [ ] All 4 boards compile successfully (inkplate2, inkplate5v2, inkplate10, inkplate6flick)
- [ ] Config portal dropdown renders correctly on all boards
- [ ] At least 5 different timezones tested (Europe, Americas, Asia, Pacific)
- [ ] DST transition tested for at least 2 timezones (spring forward and fall back)
- [ ] Legacy numeric offset mode still works (backward compatibility)
- [ ] Hourly schedule respects DST-adjusted local time
- [ ] Serial logs clearly indicate timezone mode (named vs legacy)
- [ ] No memory leaks or crashes during timezone application
- [ ] NVS storage works correctly for `tz_name` field

## Known Limitations

1. **Timezone Database Size**: Only 60+ common timezones included (not full IANA database)
   - **Mitigation**: Covers 95%+ of users; can expand list if needed
   
2. **Future DST Rule Changes**: POSIX TZ strings are static (not auto-updated)
   - **Mitigation**: Rare occurrence; firmware update would fix
   
3. **No Timezone Search**: Dropdown only, no autocomplete/search
   - **Mitigation**: Timezones sorted by region for easy browsing

## Debugging Tips

**Check Timezone Mode**:
```
LogBox: Timezone Configuration
  Mode: Named timezone
  Name: Europe/Berlin
  POSIX: CET-1CEST,M3.5.0,M10.5.0/3
```

**Check Local Hour Calculation**:
```
LogBox: Hourly Schedule Check
  Current time (UTC): 2025-11-08 20:00:00
  Timezone: Europe/Berlin (automatic DST)
  Local hour: 21
```

**Legacy Mode Example**:
```
LogBox: Timezone Configuration
  Mode: Legacy numeric offset
  Offset: UTC+1
```

## Test Results Summary

| Test Scenario | Status | Notes |
|---------------|--------|-------|
| POSIX TZ Logic Validation | ✅ PASS | C++ test confirms setenv/tzset works correctly |
| Fresh Config | ⏳ PENDING | Requires Arduino CLI build |
| DST Transition | ⏳ PENDING | Requires time simulation or wait for actual DST |
| Backward Compatibility | ⏳ PENDING | Requires device with legacy config |
| Timezone Dropdown UI | ⏳ PENDING | Requires web browser test |
| Hourly Schedule + DST | ⏳ PENDING | Requires DST simulation |

## References

- [POSIX TZ Environment Variable](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html)
- [IANA Time Zone Database](https://www.iana.org/time-zones)
- [ESP32 Time Functions](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html)
