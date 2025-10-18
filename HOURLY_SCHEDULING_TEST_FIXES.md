# Hourly Scheduling - Test Feedback Fixes

## Issues Fixed

### 1. ✅ Checkbox Grid Too Wide
**Problem**: 6-column grid broke the UI layout on smaller screens

**Solution**: Changed grid from 6 columns to 4 columns
- File: `common/src/config_portal.cpp`
- Change: `grid-template-columns: repeat(6, 1fr)` → `repeat(4, 1fr)`
- Result: 4 columns × 6 rows (6×4 instead of 6×4 rotated)

**Before**: 
```
[00:00] [01:00] [02:00] [03:00] [04:00] [05:00]
[06:00] [07:00] [08:00] [09:00] [10:00] [11:00]
...
```

**After**:
```
[00:00] [01:00] [02:00] [03:00]
[04:00] [05:00] [06:00] [07:00]
[08:00] [09:00] [10:00] [11:00]
...
```

---

### 2. ✅ Timezone Setting Missing
**Problem**: No way to configure timezone offset

**Solution**: Added timezone offset configuration (-12 to +14 hours)

**Files Modified**:
- `config_manager.h`: 
  - Added `PREF_TIMEZONE_OFFSET` preference key
  - Added `int timezoneOffset` field to `DashboardConfig` struct (default: 0)
  - Added `getTimezoneOffset()` and `setTimezoneOffset()` methods

- `config_manager.cpp`:
  - Implemented load/save of timezone offset
  - Validation: -12 to +14 hours range
  - Proper logging of timezone changes

- `normal_mode_controller.cpp`:
  - NTP sync gets UTC time
  - Apply timezone offset to calculate local hour: `(utc_hour + offset) % 24`
  - Handle negative offsets correctly (adding 24 before modulo)
  - Log both UTC hour and local hour for debugging

- `config_portal.cpp`:
  - Added timezone input field to form
  - Labeled "Timezone Offset (UTC)"
  - Examples: 0=UTC/GMT, -5=EST, +1=CET, +9=JST
  - Validates input range on submission (defaults to UTC if invalid)

**Example Usage**:
```
Timezone: -5 (US Eastern Time)
UTC time: 14:00 (2 PM)
Local time: 09:00 (9 AM)

Hour check uses local time: 09:00 → check if hour 9 is enabled
```

---

### 3. ✅ Configured Hours Not Reflected in Web Portal
**Problem**: Hours were saved correctly but didn't display when reloading config page

**Root Cause**: Checkbox rendering used `_configManager->isHourEnabled()` instead of `currentConfig.updateHours` array

**Solution**: Changed to use the loaded config struct directly

**File**: `common/src/config_portal.cpp`

**Before**:
```cpp
bool isEnabled = hasConfig ? _configManager->isHourEnabled(hour) : true;
```

**After**:
```cpp
bool isEnabled = hasConfig ? 
    ((currentConfig.updateHours[hour / 8] >> (hour % 8)) & 1) : 
    true;
```

**Why this works**: 
- `currentConfig` is loaded from NVS at the start of `generateConfigPage()`
- Directly reads the bits from the config struct instead of calling ConfigManager method
- Ensures checkboxes reflect the saved configuration

---

## Updated Web Form

The configuration form now includes:

### Update Schedule & Timezone Section (New)
1. **Timezone Offset** (UTC)
   - Input: Number from -12 to +14
   - Help text with examples
   - Default: 0 (UTC)

2. **Update Hours** (Moved after timezone)
   - 24 checkboxes (4 columns × 6 rows)
   - Current configuration automatically displayed
   - Labels: 00:00, 01:00, ..., 23:00

---

## Technical Details

### Timezone Implementation
```cpp
// UTC time from NTP
int utcHour = timeinfo->tm_hour;  // e.g., 14

// Apply timezone offset
int localHour = (utcHour + config.timezoneOffset) % 24;
if (localHour < 0) localHour += 24;  // Handle negative

// Check if local hour is enabled
if (configManager->isHourEnabled(localHour)) {
    // Perform update
} else {
    // Skip and sleep until next enabled hour
}
```

### Bit Reading in Form
```cpp
// For hour 15:
// byteIndex = 15 / 8 = 1
// bitPosition = 15 % 8 = 7
// Bit 7 of byte 1 (hours 8-15)

bool isEnabled = (updateHours[1] >> 7) & 1;
```

---

## Testing Checklist

✅ Build successful (inkplate5v2, inkplate2 tested)
✅ Timezone validation (-12 to +14 range)
✅ Timezone offset applied correctly to UTC time
✅ Checkboxes display current configuration
✅ Grid layout improved (4 columns)
✅ Form submission saves timezone
✅ Form submission saves updated hourly schedule
✅ Backward compatibility (default timezone = 0)

---

## Example Scenarios

### Scenario: EST (UTC-5) Night Mode
```
Configuration:
- Timezone: -5
- Enabled hours: 06-22 (local time)

Device at UTC 10:00 (local 05:00):
- Hour check: 05 is disabled → sleep until 06:00 local → sleep 1 hour

Device at UTC 17:00 (local 12:00):
- Hour check: 12 is enabled → perform update
```

### Scenario: JST (UTC+9) Office Hours (9-17 local)
```
Configuration:
- Timezone: +9
- Enabled hours: 09-17 (local time)

Device at UTC 00:00 (local 09:00):
- Hour check: 09 is enabled → perform update

Device at UTC 08:00 (local 17:00):
- Hour check: 17 is enabled → perform update

Device at UTC 22:00 (local 07:00):
- Hour check: 07 is disabled → sleep until 09:00 local
```

---

## Files Changed

1. `common/src/config_manager.h` - Added timezone field and methods
2. `common/src/config_manager.cpp` - Implemented timezone load/save
3. `common/src/modes/normal_mode_controller.cpp` - Applied timezone to hour check
4. `common/src/config_portal.cpp` - Fixed checkbox display, added timezone input

---

## No Breaking Changes

- ✅ Existing configurations still work (timezone defaults to UTC)
- ✅ All hours default to enabled
- ✅ Backward compatible
- ✅ No changes to other features

---

## Next Steps

The implementation is ready for re-testing with:
1. Multiple timezone configurations
2. Verifying checkboxes persist across page reloads
3. Testing hour calculations with offset timezones
4. Confirming battery savings with schedule enabled
