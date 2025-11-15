#ifndef DECISION_LOGIC_H
#define DECISION_LOGIC_H

#include <stdint.h>
#include <time.h>

// Include config types - path resolution depends on build context:
// - Arduino: Uses "../config_manager.h" (real headers) via angle brackets from --build-property
// - CMake tests: Uses "config.h" (mocks) via test/mocks include path
#ifndef UNIT_TEST
    #include "../config_manager.h"
    #include "../power_manager.h"
#else
    #include "config.h"  // Mock version from test/mocks/
#endif

/**
 * @brief Decision structure for image target determination
 */
struct ImageTargetDecision {
    uint8_t targetIndex;        // Which image to display (0-9)
    bool shouldAdvance;         // Should advance to next image after display
    const char* reason;         // Human-readable reason for this decision
};

/**
 * @brief Decision structure for CRC32 check behavior
 */
struct CRC32Decision {
    bool shouldCheck;           // Check CRC32 and potentially skip download (always fetch when enabled)
    const char* reason;         // Human-readable reason for this decision
};

/**
 * @brief Decision structure for sleep duration calculation
 */
struct SleepDecision {
    float sleepSeconds;         // How long to sleep (0 = indefinite/button-only)
    const char* reason;         // Human-readable reason for this duration
};

/**
 * @brief Pure decision functions for normal mode controller
 * 
 * These functions contain NO dependencies on Arduino/ESP32 APIs,
 * making them fully testable with standard C++ unit testing frameworks.
 * 
 * All decision logic is based on configuration state and wake reason only.
 */

/**
 * @brief Determine which image to display and whether to advance carousel
 * 
 * @param config Dashboard configuration
 * @param wakeReason Why the device woke up (timer, button, etc.)
 * @param currentIndex Current carousel position (0-9)
 * @return ImageTargetDecision with target index and advance flag
 */
ImageTargetDecision determineImageTarget(const DashboardConfig& config, 
                                         WakeupReason wakeReason, 
                                         uint8_t currentIndex);

/**
 * @brief Determine whether to check CRC32 for download optimization
 * 
 * @param config Dashboard configuration
 * @param wakeReason Why the device woke up (timer, button, etc.)
 * @param currentIndex Current carousel position (0-9)
 * @return CRC32Decision with shouldCheck flag and reason
 */
CRC32Decision determineCRC32Action(const DashboardConfig& config, 
                                   WakeupReason wakeReason, 
                                   uint8_t currentIndex);

/**
 * @brief Determine how long to sleep until next wake
 * 
 * @param config Dashboard configuration
 * @param currentTime Current time (for hourly schedule calculation)
 * @param currentIndex Current carousel position (0-9)
 * @param crc32Matched Whether CRC32 check matched (affects reason string)
 * @return SleepDecision with sleep duration and reason
 */
SleepDecision determineSleepDuration(const DashboardConfig& config, 
                                     time_t currentTime, 
                                     uint8_t currentIndex, 
                                     bool crc32Matched);

/**
 * @brief Calculate sleep duration until next enabled hour
 * 
 * Helper function for hourly scheduling. Returns -1 if current hour is enabled
 * (no sleep needed) or sleep minutes to next enabled hour.
 * 
 * @param currentTime Current time (UTC)
 * @param timezoneOffset Timezone offset in hours
 * @param updateHours 24-hour bitmask (3 bytes)
 * @return Sleep minutes to next enabled hour, or -1 if current hour enabled
 */
float calculateSleepMinutesToNextEnabledHour(time_t currentTime, 
                                             int timezoneOffset, 
                                             const uint8_t updateHours[3]);

#endif // DECISION_LOGIC_H
