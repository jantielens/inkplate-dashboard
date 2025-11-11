#include "power_manager.h"
#include <esp_sleep.h>
#include <esp_system.h>  // For esp_reset_reason()
#include <WiFi.h>
#include <Preferences.h>
#include "Inkplate.h"
#include "logger.h"
#include "config_manager.h"  // For DashboardConfig and ConfigManager

// RTC memory to track if device was previously running
RTC_DATA_ATTR uint32_t rtc_boot_count = 0;
RTC_DATA_ATTR bool rtc_was_running = false;

// Preferences for persistent storage across full resets
static Preferences prefs;

// Include board config for battery pins and special flags
#include "../../../boards/inkplate2/board_config.h"
#ifndef BATTERY_ADC_PIN
#include "../../../boards/inkplate5v2/board_config.h"
#endif
#ifndef BATTERY_ADC_PIN
#include "../../../boards/inkplate10/board_config.h"
#endif

PowerManager::PowerManager() : _buttonPin(0), _wakeupReason(WAKEUP_FIRST_BOOT) {
}

void PowerManager::begin(uint8_t buttonPin) {
    _buttonPin = buttonPin;
    
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    // Only configure button for boards that have a physical button
    // Configure button pin as input with pull-up
    pinMode(_buttonPin, INPUT_PULLUP);
    #endif
    
    // Detect why we woke up
    _wakeupReason = detectWakeupReason();
    
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    // Configure button as wake source (ext0 - single GPIO)
    // Wake when button is pressed (LOW level, assuming active low button)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)_buttonPin, 0);  // 0 = LOW, 1 = HIGH
    
    LogBox::messagef("PowerManager initialized", "Button pin configured: GPIO %d", _buttonPin);
    #else
    LogBox::message("PowerManager initialized", "No button on this board");
    #endif
    
    printWakeupReason();
}

WakeupReason PowerManager::getWakeupReason() {
    return _wakeupReason;
}

WakeupReason PowerManager::detectWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            LogBox::message("Wakeup Detection", "Wakeup caused by button press (EXT0)");
            return WAKEUP_BUTTON;
            
        case ESP_SLEEP_WAKEUP_TIMER:
            LogBox::message("Wakeup Detection", "Wakeup caused by timer");
            return WAKEUP_TIMER;
            
        case ESP_SLEEP_WAKEUP_UNDEFINED: {
            // Check if this was a hardware reset button press
            esp_reset_reason_t reset_reason = esp_reset_reason();
            
            LogBox::begin("Wakeup Detection");
            LogBox::linef("Reset reason code: %d", reset_reason);
            LogBox::linef("RTC boot count: %d", rtc_boot_count);
            LogBox::linef("RTC was running: %s", rtc_was_running ? "true" : "false");
            
            // Increment boot counter
            rtc_boot_count++;
            
            if (reset_reason == ESP_RST_POWERON) {
                // Power-on reset - check if this might be reset button vs full power cycle
                // Open preferences to check if device was running
                prefs.begin("power_mgr", true);  // Read-only
                bool was_running = prefs.getBool("was_running", false);
                prefs.end();
                
                LogBox::linef("Device was running flag: %s", was_running ? "true" : "false");
                
                if (was_running) {
                    // Device was previously running - this is a reset button press
                    LogBox::line("Device was running - reset button press detected");
                    LogBox::end();
                    
                    // Clear the flag for next time
                    prefs.begin("power_mgr", false);  // Read-write
                    prefs.putBool("was_running", false);
                    prefs.end();
                    
                    return WAKEUP_RESET_BUTTON;
                } else {
                    // Device was not running - genuine first power-on or long power cycle
                    LogBox::line("Device was not running - initial power-on or long power cycle");
                    LogBox::line("Setting running flag for reset detection on next boot");
                    LogBox::end();
                    
                    // Set flag so reset button can be detected on next boot
                    prefs.begin("power_mgr", false);  // Read-write
                    prefs.putBool("was_running", true);
                    prefs.end();
                    
                    return WAKEUP_FIRST_BOOT;
                }
            } else if (reset_reason == ESP_RST_SW || reset_reason == ESP_RST_DEEPSLEEP) {
                // Software reset or wake from deep sleep
                LogBox::line("Software reset or deep sleep wake");
                LogBox::end();
                return WAKEUP_FIRST_BOOT;
            } else if (reset_reason == ESP_RST_EXT) {
                // External reset (reset button on some boards)
                LogBox::line("External reset button pressed");
                LogBox::end();
                return WAKEUP_RESET_BUTTON;
            } else {
                LogBox::linef("Other reset reason: %d", reset_reason);
                LogBox::end();
                return WAKEUP_FIRST_BOOT;
            }
        }
            
        default:
            LogBox::messagef("Wakeup Detection", "Wakeup caused by unknown reason: %d", wakeup_reason);
            return WAKEUP_UNKNOWN;
    }
}

void PowerManager::printWakeupReason() {
    LogBox::begin("Current Wakeup Reason");
    switch (_wakeupReason) {
        case WAKEUP_TIMER:
            LogBox::line("TIMER (normal refresh cycle)");
            break;
        case WAKEUP_BUTTON:
            LogBox::line("BUTTON (config mode requested)");
            break;
        case WAKEUP_FIRST_BOOT:
            LogBox::line("FIRST_BOOT (initial setup)");
            break;
        case WAKEUP_RESET_BUTTON:
            LogBox::line("RESET_BUTTON (hardware reset pressed)");
            break;
        default:
            LogBox::line("UNKNOWN");
            break;
    }
    LogBox::end();
}

bool PowerManager::isButtonPressed() {
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    // Button is active LOW (pressed = LOW, released = HIGH with pull-up)
    return digitalRead(_buttonPin) == LOW;
    #else
    // No button on this board
    return false;
    #endif
}

ButtonPressType PowerManager::detectButtonPressType() {
    #if defined(HAS_BUTTON) && HAS_BUTTON == false
    // Board has no button - always return no press
    LogBox::begin("Button Detection");
    LogBox::line("Board has no physical button");
    LogBox::line("Skipping button detection");
    LogBox::end();
    return BUTTON_PRESS_NONE;
    #endif
    
    LogBox::begin("Detecting button press type");
    LogBox::linef("Wake reason: %d (WAKEUP_BUTTON=%d)", _wakeupReason, WAKEUP_BUTTON);
    
    // Check if button is currently pressed (works for any wake reason)
    bool buttonCurrentlyPressed = isButtonPressed();
    LogBox::linef("Button currently pressed: %s", buttonCurrentlyPressed ? "YES" : "NO");
    
    if (!buttonCurrentlyPressed) {
        // For button wake, this means it was released quickly (short press)
        // For other wake reasons, no button is pressed
        if (_wakeupReason == WAKEUP_BUTTON) {
            LogBox::line("Button already released - SHORT PRESS detected");
            LogBox::end();
            return BUTTON_PRESS_SHORT;
        } else {
            LogBox::line("No button press detected");
            LogBox::end();
            return BUTTON_PRESS_NONE;
        }
    }
    
    LogBox::line("Button is currently pressed, waiting to determine hold duration...");
    
    // Button is held down, wait for hold duration (2.5 seconds)
    const unsigned long HOLD_THRESHOLD_MS = 2500;
    unsigned long startTime = millis();
    
    // Wait for either button release or timeout
    while (millis() - startTime < HOLD_THRESHOLD_MS) {
        if (!isButtonPressed()) {
            // Button was released before timeout
            unsigned long pressDuration = millis() - startTime;
            LogBox::linef("Button released after %lu ms - SHORT PRESS detected", pressDuration);
            LogBox::end();
            return BUTTON_PRESS_SHORT;
        }
        delay(50);  // Small delay to prevent excessive polling
    }
    
    // Button is still held after threshold - it's a long press
    LogBox::linef("Button held for >= %lu ms - LONG PRESS detected", HOLD_THRESHOLD_MS);
    LogBox::end();
    return BUTTON_PRESS_LONG;
}

uint64_t PowerManager::getSleepDuration(uint16_t refreshRateMinutes) {
    // Convert minutes to microseconds
    // 1 minute = 60 seconds = 60,000,000 microseconds
    uint64_t microseconds = (uint64_t)refreshRateMinutes * 60ULL * 1000000ULL;
    
    LogBox::messagef("Sleep Duration Calculation", "Sleep duration: %u minutes = %llu microseconds", refreshRateMinutes, microseconds);
    
    return microseconds;
}

uint64_t PowerManager::getSleepDuration(float refreshRateMinutes) {
    // Convert minutes to microseconds
    // 1 minute = 60 seconds = 60,000,000 microseconds
    uint64_t microseconds = (uint64_t)(refreshRateMinutes * 60.0 * 1000000.0);
    
    LogBox::messagef("Sleep Duration Calculation", "Sleep duration: %.2f minutes = %llu microseconds", refreshRateMinutes, microseconds);
    
    return microseconds;
}

void PowerManager::prepareForSleep() {
    LogBox::begin("Preparing for deep sleep");
    
    #if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true
    // Check if frontlight was activated and ensure it stays on for configured duration
    extern bool frontlightActivated;
    extern unsigned long frontlightStartTime;
    extern ConfigManager configManager;
    
    if (frontlightActivated) {
        // Load config to get frontlight duration
        DashboardConfig config;
        uint8_t frontlightDuration = 15;  // Default fallback
        if (configManager.loadConfig(config)) {
            frontlightDuration = config.frontlightDuration;
        }
        
        if (frontlightDuration > 0) {
            unsigned long frontlightMinDurationMs = frontlightDuration * 1000UL;
            unsigned long elapsedTime = millis() - frontlightStartTime;
            
            if (elapsedTime < frontlightMinDurationMs) {
                unsigned long remainingTime = frontlightMinDurationMs - elapsedTime;
                LogBox::linef("Frontlight minimum duration: waiting %lu ms", remainingTime);
                delay(remainingTime);
            }
        }
        
        // Turn off frontlight before sleep
        LogBox::line("Turning off frontlight...");
        extern Inkplate display;  // Access global display object
        display.frontlight(false);
        
        // Reset frontlight tracking
        frontlightActivated = false;
        frontlightStartTime = 0;
    }
    #endif
    
    LogBox::line("Disconnecting WiFi...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // Small delay to ensure WiFi is fully shut down
    delay(100);
    
    LogBox::line("Ready for deep sleep");
    LogBox::end();
}

void PowerManager::enterDeepSleep(float durationSeconds, float loopTimeSeconds) {
    // Configure wake sources based on refresh interval
    // If interval is 0, only button wake is enabled (button-only mode)
    bool buttonOnlyMode = (durationSeconds == 0.0);
    
    if (!buttonOnlyMode) {
        // Calculate sleep duration and configure timer wake source
        // Convert seconds to minutes for getSleepDuration (which expects minutes)
        float durationMinutes = durationSeconds / 60.0;
        uint64_t sleepDuration = getSleepDuration(durationMinutes);
        
        // Compensate for active loop time to maintain accurate refresh intervals
        // Example: 60s interval with 7s active time → sleep 53s (not 60s)
        // This ensures total cycle time = configured interval
        if (loopTimeSeconds > 0) {
            uint64_t loopTimeMicros = (uint64_t)(loopTimeSeconds * 1000000.0);  // Convert s to µs
            uint64_t targetCycleMicros = (uint64_t)(durationSeconds * 1000000.0);
            
            if (loopTimeMicros < targetCycleMicros) {
                // Normal case: adjust sleep to compensate for loop time
                sleepDuration = targetCycleMicros - loopTimeMicros;
            }
            // Edge case: loop time >= interval
            // Sleep full interval anyway (accept drift for this rare case)
            // This prevents 0-second sleep cycles in poor network conditions
        }
        
        esp_sleep_enable_timer_wakeup(sleepDuration);
    }
    
    // Re-configure button wake source (if available)
    // Must be set again because esp_sleep_enable_* doesn't accumulate
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    esp_sleep_enable_ext0_wakeup((gpio_num_t)_buttonPin, 0);  // 0 = LOW
    #endif
    
    // Mark that we were running (for reset button detection)
    rtc_was_running = true;
    
    LogBox::begin("Entering Deep Sleep");
    if (buttonOnlyMode) {
        LogBox::line("Button-only mode (interval = 0)");
        LogBox::line("No automatic refresh - wake by button press only");
    } else {
        LogBox::linef("Configured interval: %.2f seconds", durationSeconds);
        if (loopTimeSeconds > 0) {
            uint64_t loopTimeMicros = (uint64_t)(loopTimeSeconds * 1000000.0);
            uint64_t targetCycleMicros = (uint64_t)(durationSeconds * 1000000.0);
            if (loopTimeMicros < targetCycleMicros) {
                uint64_t adjustedSleepMicros = targetCycleMicros - loopTimeMicros;
                float adjustedSleepSeconds = adjustedSleepMicros / 1000000.0;
                LogBox::linef("Active loop time: %.3fs", loopTimeSeconds);
                LogBox::linef("Adjusted sleep: %.3f seconds", adjustedSleepSeconds);
            } else {
                LogBox::linef("Active loop time: %.3fs (>= interval, no adjustment)", loopTimeSeconds);
            }
        }
    }
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    if (buttonOnlyMode) {
        LogBox::line("Wake sources: BUTTON only");
    } else {
        LogBox::line("Wake sources: TIMER + BUTTON");
    }
    #else
    if (buttonOnlyMode) {
        LogBox::line("Wake sources: NONE (board has no button - will not wake!)");
    } else {
        LogBox::line("Wake sources: TIMER only");
    }
    #endif
    LogBox::end();
    
    // Flush serial before sleeping
    Serial.flush();
    
    // Enter deep sleep
    // The device will wake up from:
    // - Button-only mode (interval=0): Button press only (if board has button)
    // - Normal mode (interval>0): Timer OR Button press (if board has button)
    esp_deep_sleep_start();
}

float PowerManager::readBatteryVoltage(void* inkplate) {
    LogBox::begin("Reading battery voltage");
    
    // If Inkplate object provided, use its built-in readBattery() method
    if (inkplate != nullptr) {
        // Cast to Inkplate* and call readBattery()
        // The Inkplate library has a readBattery() method that returns voltage
        // Note: Inkplate 2 may not have this method, will fall back to manual ADC reading
        #ifndef DISPLAY_MODE_INKPLATE2
        float voltage = ((Inkplate*)inkplate)->readBattery();
        
        LogBox::linef("Battery Voltage: %.3f V (from Inkplate library)", voltage);
        LogBox::end();
        
        return voltage;
        #endif
    }
    
    #ifdef BATTERY_ADC_PIN
    // Fallback: manual ADC reading if no Inkplate object provided or Inkplate 2
    LogBox::line("Using manual ADC reading");
    
    // Configure ADC pin as input
    pinMode(BATTERY_ADC_PIN, INPUT);
    
    // Configure ADC attenuation for full range (0-3.3V input)
    // With 11dB attenuation, we can measure up to ~3.3V
    analogSetAttenuation(ADC_11db);
    
    // Small delay for ADC to stabilize
    delay(10);
    
    // Read ADC value multiple times and average for accuracy
    const int numSamples = 10;
    uint32_t adcSum = 0;
    
    for (int i = 0; i < numSamples; i++) {
        adcSum += analogRead(BATTERY_ADC_PIN);
        delay(5);
    }
    
    uint32_t adcValue = adcSum / numSamples;
    
    // Convert ADC value to voltage
    // ESP32 ADC: 0-4095 for 0-3.3V (12-bit ADC)
    // Inkplate has a voltage divider (typically 1:2), so multiply by 2
    // The exact voltage divider ratio may vary by board revision
    float adcVoltage = (adcValue / 4095.0) * 3.3;
    float batteryVoltage = adcVoltage * 2.0;  // Account for voltage divider
    
    LogBox::linef("ADC Value: %d (raw)", adcValue);
    LogBox::linef("ADC Voltage: %.3f V", adcVoltage);
    LogBox::linef("Battery Voltage: %.3f V (with divider)", batteryVoltage);
    LogBox::end();
    
    return batteryVoltage;
    #else
    LogBox::line("Battery ADC pin not defined for this board");
    LogBox::end();
    return 0.0;
    #endif
}

void PowerManager::markDeviceRunning() {
    // Set flag in NVS to indicate device is running
    // This flag persists across resets and allows us to detect reset button presses
    prefs.begin("power_mgr", false);  // Read-write
    bool was_already_set = prefs.getBool("was_running", false);
    
    if (!was_already_set) {
        prefs.putBool("was_running", true);
        LogBox::message("Power Manager", "Device marked as running in NVS (one-time write)");
    }
    
    prefs.end();
}

// Watchdog timer implementation
#include <esp_task_wdt.h>

// Default watchdog timeout (can be overridden per board in board_config.h)
#ifndef WATCHDOG_TIMEOUT_SECONDS
#define WATCHDOG_TIMEOUT_SECONDS 30
#endif

void PowerManager::enableWatchdog(uint32_t timeoutSeconds) {
    // Use board-specific timeout if not provided
    if (timeoutSeconds == 0) {
        timeoutSeconds = WATCHDOG_TIMEOUT_SECONDS;
    }
    
    LogBox::begin("Watchdog Timer");
    LogBox::linef("Enabling watchdog with %u second timeout", timeoutSeconds);
    
    try {
        // Initialize watchdog task with specified timeout
        // IDLE tasks (on both cores for ESP32) will be monitored
        esp_task_wdt_init(timeoutSeconds, true);  // true = panic on timeout
        esp_task_wdt_add(NULL);  // Monitor current task (main loop)
        
        LogBox::line("Watchdog enabled successfully");
        LogBox::end();
    } catch (...) {
        LogBox::line("Failed to enable watchdog (may already be enabled)");
        LogBox::end();
    }
}

void PowerManager::disableWatchdog() {
    LogBox::begin("Watchdog Timer");
    LogBox::line("Disabling watchdog");
    
    try {
        esp_task_wdt_delete(NULL);  // Remove current task from monitoring
        LogBox::line("Watchdog disabled successfully");
        LogBox::end();
    } catch (...) {
        LogBox::line("Failed to disable watchdog (may already be disabled)");
        LogBox::end();
    }
}

int PowerManager::calculateBatteryPercentage(float voltage) {
    // Lithium-ion discharge curve for ESP32 devices
    // Based on typical Li-ion/LiPo battery discharge characteristics
    // Voltage range: 4.2V (100%) to 3.0V (0%)
    
    // Voltage to percentage mapping (non-linear discharge curve)
    // These values represent the typical discharge curve of a lithium-ion battery
    const float voltageMap[][2] = {
        {4.20, 100},  // Fully charged
        {4.15, 95},
        {4.11, 90},
        {4.08, 85},
        {4.02, 80},
        {3.98, 75},
        {3.95, 70},
        {3.91, 65},
        {3.87, 60},
        {3.85, 55},
        {3.84, 50},
        {3.82, 45},
        {3.80, 40},
        {3.79, 35},
        {3.77, 30},
        {3.75, 25},
        {3.73, 20},
        {3.71, 15},
        {3.69, 10},
        {3.61, 5},
        {3.00, 0}     // Empty/cutoff voltage
    };
    
    const int mapSize = sizeof(voltageMap) / sizeof(voltageMap[0]);
    
    // Handle out-of-range values
    if (voltage >= voltageMap[0][0]) {
        return 100;  // Above maximum voltage
    }
    if (voltage <= voltageMap[mapSize - 1][0]) {
        return 0;  // Below minimum voltage
    }
    
    // Find the two points to interpolate between
    for (int i = 0; i < mapSize - 1; i++) {
        if (voltage >= voltageMap[i + 1][0] && voltage <= voltageMap[i][0]) {
            // Linear interpolation between two points
            float v1 = voltageMap[i][0];
            float p1 = voltageMap[i][1];
            float v2 = voltageMap[i + 1][0];
            float p2 = voltageMap[i + 1][1];
            
            float percentage = p1 + (voltage - v1) * (p2 - p1) / (v2 - v1);
            
            // Round to nearest 5%
            int rounded = ((int)(percentage + 2.5) / 5) * 5;
            
            // Clamp to valid range
            if (rounded < 0) rounded = 0;
            if (rounded > 100) rounded = 100;
            
            return rounded;
        }
    }
    
    // Fallback (shouldn't reach here)
    return 0;
}

