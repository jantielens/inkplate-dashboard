#ifndef SLEEP_LOGIC_H
#define SLEEP_LOGIC_H

#include <stdint.h>

/**
 * @brief Pure sleep calculation functions
 * 
 * These functions contain NO dependencies on Arduino/ESP32 APIs,
 * making them fully testable with standard C++ unit testing frameworks.
 */

/**
 * @brief Calculate adjusted sleep duration with loop time compensation
 * 
 * Compensates for active loop time to maintain accurate refresh intervals.
 * If loop time >= target interval, returns 0 (no sleep needed, accept drift).
 * 
 * @param targetIntervalSeconds Desired refresh interval in seconds
 * @param loopTimeSeconds Time spent in active loop (0 = no compensation)
 * @return Adjusted sleep duration in microseconds (0 for button-only mode)
 */
uint64_t calculateAdjustedSleepDuration(float targetIntervalSeconds, float loopTimeSeconds);

#endif // SLEEP_LOGIC_H
