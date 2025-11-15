#include <sleep_logic.h>

uint64_t calculateAdjustedSleepDuration(float targetIntervalSeconds, float loopTimeSeconds) {
    // Handle button-only mode (interval = 0)
    if (targetIntervalSeconds == 0.0f) {
        return 0;  // No timer wake source
    }
    
    // Convert to microseconds
    uint64_t targetIntervalMicros = (uint64_t)(targetIntervalSeconds * 1000000.0);
    
    // No compensation needed if loop time not provided
    if (loopTimeSeconds <= 0.0f) {
        return targetIntervalMicros;
    }
    
    uint64_t loopTimeMicros = (uint64_t)(loopTimeSeconds * 1000000.0);
    
    // If loop time >= target interval, sleep full interval anyway
    // This prevents 0-second sleep cycles and accepts drift for this edge case
    if (loopTimeMicros >= targetIntervalMicros) {
        return targetIntervalMicros;
    }
    
    // Normal case: subtract loop time from target interval
    return targetIntervalMicros - loopTimeMicros;
}
