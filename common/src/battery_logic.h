#ifndef BATTERY_LOGIC_H
#define BATTERY_LOGIC_H

/**
 * @brief Pure battery calculation functions
 * 
 * These functions contain NO dependencies on Arduino/ESP32 APIs,
 * making them fully testable with standard C++ unit testing frameworks.
 */

/**
 * @brief Calculate battery percentage from voltage
 * 
 * Uses lithium-ion discharge curve typical for ESP32 devices.
 * Voltage range: 4.2V (100%) to 3.0V (0%)
 * 
 * @param voltage Battery voltage in volts
 * @return Battery percentage (0-100), rounded to 5% increments
 */
int calculateBatteryPercentage(float voltage);

#endif // BATTERY_LOGIC_H
