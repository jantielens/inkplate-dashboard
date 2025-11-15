#ifndef CONFIG_LOGIC_H
#define CONFIG_LOGIC_H

#include <stdint.h>

/**
 * @brief Pure configuration validation and helper functions
 * 
 * These functions contain NO dependencies on Arduino/ESP32 APIs,
 * making them fully testable with standard C++ unit testing frameworks.
 */

/**
 * @brief Apply timezone offset to UTC hour
 * 
 * @param utcHour Hour in UTC (0-23)
 * @param offsetHours Timezone offset in hours (-12 to +14)
 * @return Local hour (0-23), wrapped around 24-hour clock
 */
int applyTimezoneOffset(int utcHour, int offsetHours);

/**
 * @brief Check if a specific hour is enabled in the 24-hour bitmask
 * 
 * @param hour Hour to check (0-23)
 * @param bitmask 3-byte bitmask representing 24 hours (bit 0 = hour 0, etc.)
 * @return true if hour is enabled, false otherwise
 */
bool isHourEnabledInBitmask(int hour, const uint8_t bitmask[3]);

/**
 * @brief Check if all hours are enabled in the 24-hour bitmask
 * 
 * All hours enabled = all bits set = {0xFF, 0xFF, 0xFF}
 * 
 * @param bitmask 3-byte bitmask representing 24 hours
 * @return true if all hours enabled, false otherwise
 */
bool areAllHoursEnabled(const uint8_t bitmask[3]);

#endif // CONFIG_LOGIC_H
