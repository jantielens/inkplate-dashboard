#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Wake up reasons
enum WakeupReason {
    WAKEUP_TIMER,
    WAKEUP_BUTTON,
    WAKEUP_FIRST_BOOT,
    WAKEUP_RESET_BUTTON,    // Hardware reset button pressed
    WAKEUP_UNKNOWN
};

// Button press types (for button wake)
enum ButtonPressType {
    BUTTON_PRESS_NONE,      // No button press (or not a button wake)
    BUTTON_PRESS_SHORT,     // Short press (< 2 seconds) - trigger normal update
    BUTTON_PRESS_LONG       // Long press (>= 2 seconds) - enter config mode
};

class PowerManager {
public:
    PowerManager();
    
    // Initialize power management (configure wake sources)
    void begin(uint8_t buttonPin);
    
    // Get the reason for waking up
    WakeupReason getWakeupReason();
    
    // Detect button press type (short vs long) after button wake
    // Should be called immediately after begin() if wake reason is WAKEUP_BUTTON
    // Returns BUTTON_PRESS_SHORT, BUTTON_PRESS_LONG, or BUTTON_PRESS_NONE
    ButtonPressType detectButtonPressType();
    
    // Enter deep sleep with timer wake source
    // refreshRateMinutes: how long to sleep (in minutes)
    // loopTimeMs: optional full loop time in ms (displayed if > 0)
    void enterDeepSleep(uint16_t refreshRateMinutes, unsigned long loopTimeMs = 0);
    
    // Enter deep sleep with timer wake source (fractional minutes)
    // refreshRateMinutes: how long to sleep (in minutes, supports fractions)
    // loopTimeMs: optional full loop time in ms (displayed if > 0)
    void enterDeepSleep(float refreshRateMinutes, unsigned long loopTimeMs = 0);
    
    // Prepare for sleep (shutdown WiFi, display, etc.)
    void prepareForSleep();
    
    // Get sleep duration in microseconds
    uint64_t getSleepDuration(uint16_t refreshRateMinutes);
    
    // Get sleep duration in microseconds (fractional minutes)
    uint64_t getSleepDuration(float refreshRateMinutes);
    
    // Print wakeup reason to serial
    void printWakeupReason();
    
    // Read battery voltage in volts
    // Returns battery voltage (0.0 if reading fails)
    // inkplate: pointer to Inkplate display object (uses Inkplate's readBattery method)
    float readBatteryVoltage(void* inkplate = nullptr);
    
    // Calculate battery percentage from voltage
    // Uses lithium-ion discharge curve typical for ESP32 devices
    // voltage: battery voltage in volts
    // Returns percentage (0-100), rounded to 5% increments
    static int calculateBatteryPercentage(float voltage);
    
    // Mark that device is now running (for reset button detection)
    // This sets a flag in NVS that persists across resets
    // Should be called once when device enters normal operation
    void markDeviceRunning();
    
    // Enable watchdog timer (protects against lockups during normal operation)
    // Should be called at the start of normal update cycle
    // timeoutSeconds: how long to wait before forcing deep sleep
    //                 Uses WATCHDOG_TIMEOUT_SECONDS from board_config.h if not specified
    void enableWatchdog(uint32_t timeoutSeconds = 0);
    
    // Disable watchdog timer (should be called before entering deep sleep)
    void disableWatchdog();
    
private:
    uint8_t _buttonPin;
    WakeupReason _wakeupReason;
    
    // Detect wakeup reason from ESP32
    WakeupReason detectWakeupReason();
    
    // Check if button is currently pressed (LOW state)
    bool isButtonPressed();
};

#endif // POWER_MANAGER_H
