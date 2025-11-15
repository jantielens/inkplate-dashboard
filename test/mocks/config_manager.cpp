// Mock ConfigManager implementation that delegates to standalone functions
#include "config_manager.h"
#include "config_logic.h"

bool ConfigManager::areAllHoursEnabled(const uint8_t updateHours[3]) {
    return ::areAllHoursEnabled(updateHours);
}

bool ConfigManager::isHourEnabledInBitmask(uint8_t hour, const uint8_t updateHours[3]) {
    return ::isHourEnabledInBitmask(hour, updateHours);
}

int ConfigManager::applyTimezoneOffset(int hour, int offset) {
    return ::applyTimezoneOffset(hour, offset);
}
