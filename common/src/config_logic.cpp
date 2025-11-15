#include <config_logic.h>

int applyTimezoneOffset(int utcHour, int offsetHours) {
    int localHour = utcHour + offsetHours;
    
    // Wrap around 24-hour clock
    while (localHour < 0) {
        localHour += 24;
    }
    while (localHour >= 24) {
        localHour -= 24;
    }
    
    return localHour;
}

bool isHourEnabledInBitmask(int hour, const uint8_t bitmask[3]) {
    // Validate hour range
    if (hour < 0 || hour >= 24) {
        return false;
    }
    
    // Calculate byte and bit position
    // Hour 0-7 → byte 0, hours 8-15 → byte 1, hours 16-23 → byte 2
    int byteIndex = hour / 8;
    int bitIndex = hour % 8;
    
    // Check if bit is set
    return (bitmask[byteIndex] & (1 << bitIndex)) != 0;
}

bool areAllHoursEnabled(const uint8_t bitmask[3]) {
    return (bitmask[0] == 0xFF && bitmask[1] == 0xFF && bitmask[2] == 0xFF);
}
