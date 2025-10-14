#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Wake up reasons
enum WakeupReason {
    WAKEUP_TIMER,
    WAKEUP_BUTTON,
    WAKEUP_FIRST_BOOT,
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
    void enterDeepSleep(uint16_t refreshRateMinutes);
    
    // Prepare for sleep (shutdown WiFi, display, etc.)
    void prepareForSleep();
    
    // Get sleep duration in microseconds
    uint64_t getSleepDuration(uint16_t refreshRateMinutes);
    
    // Print wakeup reason to serial
    void printWakeupReason();
    
    // Read battery voltage in volts
    // Returns battery voltage (0.0 if reading fails)
    // inkplate: pointer to Inkplate display object (uses Inkplate's readBattery method)
    float readBatteryVoltage(void* inkplate = nullptr);
    
private:
    uint8_t _buttonPin;
    WakeupReason _wakeupReason;
    
    // Detect wakeup reason from ESP32
    WakeupReason detectWakeupReason();
    
    // Check if button is currently pressed (LOW state)
    bool isButtonPressed();
};

#endif // POWER_MANAGER_H
