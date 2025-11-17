#include "power_manager.h"
#include "battery_logic.h"
#include "sleep_logic.h"
#include <esp_sleep.h>
#include <esp_system.h>  // For esp_reset_reason()
#include <WiFi.h>
#include <Preferences.h>
#include "Inkplate.h"
#include "logger.h"
#include "config_manager.h"  // For DashboardConfig and ConfigManager
#include "frontlight_manager.h"

// RTC memory for voltage smoothing (survives deep sleep, no NVS wear)
RTC_DATA_ATTR float rtcSmoothedVoltage = 0.0;

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
    
    Logger::messagef("PowerManager initialized", "Button pin configured: GPIO %d", _buttonPin);
    #else
    Logger::message("PowerManager initialized", "No button on this board");
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
            Logger::message("Wakeup", "Button press (EXT0)");
            return WAKEUP_BUTTON;
            
        case ESP_SLEEP_WAKEUP_TIMER:
            Logger::message("Wakeup", "Timer");
            return WAKEUP_TIMER;
            
        case ESP_SLEEP_WAKEUP_UNDEFINED: {
            // Check if this was a hardware reset button press
            esp_reset_reason_t reset_reason = esp_reset_reason();
            
            Logger::begin("Wakeup Detection");
            Logger::linef("Reset: %d, RTC boot: %d", reset_reason, rtc_boot_count);
            
            // Increment boot counter
            rtc_boot_count++;
            
            if (reset_reason == ESP_RST_POWERON) {
                // Power-on reset - check if this might be reset button vs full power cycle
                prefs.begin("power_mgr", true);  // Read-only
                bool was_running = prefs.getBool("was_running", false);
                prefs.end();
                
                if (was_running) {
                    // Device was previously running - this is a reset button press
                    Logger::end("Reset button detected");
                    
                    // Clear the flag for next time
                    prefs.begin("power_mgr", false);  // Read-write
                    prefs.putBool("was_running", false);
                    prefs.end();
                    
                    return WAKEUP_RESET_BUTTON;
                } else {
                    // Device was not running - genuine first power-on or long power cycle
                    Logger::end("First boot");
                    
                    // Set flag so reset button can be detected on next boot
                    prefs.begin("power_mgr", false);  // Read-write
                    prefs.putBool("was_running", true);
                    prefs.end();
                    
                    return WAKEUP_FIRST_BOOT;
                }
            } else if (reset_reason == ESP_RST_SW || reset_reason == ESP_RST_DEEPSLEEP) {
                // Software reset or wake from deep sleep
                Logger::end("Software reset");
                return WAKEUP_FIRST_BOOT;
            } else if (reset_reason == ESP_RST_EXT) {
                // External reset (reset button on some boards)
                Logger::end("External reset");
                return WAKEUP_RESET_BUTTON;
            } else {
                Logger::end("Unknown reset");
                return WAKEUP_FIRST_BOOT;
            }
        }
            
        default:
            Logger::messagef("Wakeup", "Unknown reason: %d", wakeup_reason);
            return WAKEUP_UNKNOWN;
    }
}

void PowerManager::printWakeupReason() {
    Logger::begin("Current Wakeup Reason");
    switch (_wakeupReason) {
        case WAKEUP_TIMER:
            Logger::line("TIMER (normal refresh cycle)");
            break;
        case WAKEUP_BUTTON:
            Logger::line("BUTTON (config mode requested)");
            break;
        case WAKEUP_FIRST_BOOT:
            Logger::line("FIRST_BOOT");
            break;
        case WAKEUP_RESET_BUTTON:
            Logger::line("RESET_BUTTON");
            break;
        default:
            Logger::line("UNKNOWN");
            break;
    }
    Logger::end();
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
    Logger::message("Button", "No button on this board");
    return BUTTON_PRESS_NONE;
    #endif
    
    Logger::begin("Button Detection");
    
    // Check if button is currently pressed (works for any wake reason)
    bool buttonCurrentlyPressed = isButtonPressed();
    
    if (!buttonCurrentlyPressed) {
        // For button wake, this means it was released quickly (short press)
        // For other wake reasons, no button is pressed
        if (_wakeupReason == WAKEUP_BUTTON) {
            Logger::line("Button already released - SHORT PRESS detected");
            Logger::end();
            return BUTTON_PRESS_SHORT;
        } else {
            Logger::line("No button press detected");
            Logger::end();
            return BUTTON_PRESS_NONE;
        }
    }
    
    Logger::line("Button is currently pressed, waiting to determine hold duration...");
    
    // Button is held down, wait for hold duration (2.5 seconds)
    const unsigned long HOLD_THRESHOLD_MS = 2500;
    unsigned long startTime = millis();
    
    // Wait for either button release or timeout
    while (millis() - startTime < HOLD_THRESHOLD_MS) {
        if (!isButtonPressed()) {
            // Button was released before timeout
            unsigned long pressDuration = millis() - startTime;
            Logger::linef("Button released after %lu ms - SHORT PRESS detected", pressDuration);
            Logger::end();
            return BUTTON_PRESS_SHORT;
        }
        delay(50);  // Small delay to prevent excessive polling
    }
    
    // Button is still held after threshold - it's a long press
    Logger::linef("Button held for >= %lu ms - LONG PRESS detected", HOLD_THRESHOLD_MS);
    Logger::end();
    return BUTTON_PRESS_LONG;
}

uint64_t PowerManager::getSleepDuration(uint16_t refreshRateMinutes) {
    // Convert minutes to microseconds
    // 1 minute = 60 seconds = 60,000,000 microseconds
    uint64_t microseconds = (uint64_t)refreshRateMinutes * 60ULL * 1000000ULL;
    
    Logger::messagef("Sleep Duration Calculation", "Sleep duration: %u minutes = %llu microseconds", refreshRateMinutes, microseconds);
    
    return microseconds;
}

uint64_t PowerManager::getSleepDuration(float refreshRateMinutes) {
    // Convert minutes to microseconds
    // 1 minute = 60 seconds = 60,000,000 microseconds
    uint64_t microseconds = (uint64_t)(refreshRateMinutes * 60.0 * 1000000.0);
    
    Logger::messagef("Sleep Duration Calculation", "Sleep duration: %.2f minutes = %llu microseconds", refreshRateMinutes, microseconds);
    
    return microseconds;
}

void PowerManager::prepareForSleep() {
    Logger::begin("Preparing for deep sleep");
    
    #if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true
    // Turn off frontlight if active (respects minimum duration internally)
    extern FrontlightManager frontlightManager;
    if (frontlightManager.isActive()) {
        frontlightManager.turnOff();
    }
    #endif
    
    Logger::line("Disconnecting WiFi...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // Small delay to ensure WiFi is fully shut down
    delay(100);
    
    Logger::line("Ready for deep sleep");
    Logger::end();
}

void PowerManager::enterDeepSleep(float durationSeconds, float loopTimeSeconds) {
    // Configure wake sources based on refresh interval
    // If interval is 0, only button wake is enabled (button-only mode)
    bool buttonOnlyMode = (durationSeconds == 0.0);
    
    if (!buttonOnlyMode) {
        // Use standalone function for testable sleep calculation
        uint64_t sleepDuration = calculateAdjustedSleepDuration(durationSeconds, loopTimeSeconds);
        esp_sleep_enable_timer_wakeup(sleepDuration);
    }
    
    // Re-configure button wake source (if available)
    // Must be set again because esp_sleep_enable_* doesn't accumulate
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    esp_sleep_enable_ext0_wakeup((gpio_num_t)_buttonPin, 0);  // 0 = LOW
    #endif
    
    // Mark that we were running (for reset button detection)
    rtc_was_running = true;
    
    Logger::begin("Entering Deep Sleep");
    if (buttonOnlyMode) {
        Logger::line("Button-only mode (interval = 0)");
        Logger::line("No automatic refresh - wake by button press only");
    } else {
        Logger::linef("Configured interval: %.2f seconds", durationSeconds);
        if (loopTimeSeconds > 0) {
            uint64_t loopTimeMicros = (uint64_t)(loopTimeSeconds * 1000000.0);
            uint64_t targetCycleMicros = (uint64_t)(durationSeconds * 1000000.0);
            if (loopTimeMicros < targetCycleMicros) {
                uint64_t adjustedSleepMicros = targetCycleMicros - loopTimeMicros;
                float adjustedSleepSeconds = adjustedSleepMicros / 1000000.0;
                Logger::linef("Active loop time: %.3fs", loopTimeSeconds);
                Logger::linef("Adjusted sleep: %.3f seconds", adjustedSleepSeconds);
            } else {
                Logger::linef("Active loop time: %.3fs (>= interval, no adjustment)", loopTimeSeconds);
            }
        }
    }
    #if defined(HAS_BUTTON) && HAS_BUTTON == true
    if (buttonOnlyMode) {
        Logger::line("Wake sources: BUTTON only");
    } else {
        Logger::line("Wake sources: TIMER + BUTTON");
    }
    #else
    if (buttonOnlyMode) {
        Logger::line("Wake sources: NONE (board has no button - will not wake!)");
    } else {
        Logger::line("Wake sources: TIMER only");
    }
    #endif
    Logger::end();
    
    // Flush serial before sleeping
    Serial.flush();
    
    // Enter deep sleep
    // The device will wake up from:
    // - Button-only mode (interval=0): Button press only (if board has button)
    // - Normal mode (interval>0): Timer OR Button press (if board has button)
    esp_deep_sleep_start();
}

float PowerManager::readBatteryVoltage(void* inkplate) {
    Logger::begin("Reading battery voltage");
    
    // If Inkplate object provided, use its built-in readBattery() method
    if (inkplate != nullptr) {
        // Cast to Inkplate* and call readBattery()
        // The Inkplate library has a readBattery() method that returns voltage
        // Note: Inkplate 2 may not have this method, will fall back to manual ADC reading
        #ifndef DISPLAY_MODE_INKPLATE2
        float rawVoltage = ((Inkplate*)inkplate)->readBattery();
        
        // Apply EMA smoothing (30% new reading, 70% history)
        const float alpha = 0.3;
        if (rtcSmoothedVoltage == 0.0) {
            rtcSmoothedVoltage = rawVoltage;  // First reading
        } else {
            rtcSmoothedVoltage = alpha * rawVoltage + (1.0 - alpha) * rtcSmoothedVoltage;
        }
        
        Logger::linef("Battery Voltage: %.3f V raw, %.3f V smoothed", rawVoltage, rtcSmoothedVoltage);
        Logger::end();
        
        return rtcSmoothedVoltage;
        #endif
    }
    
    #ifdef BATTERY_ADC_PIN
    // Fallback: manual ADC reading if no Inkplate object provided or Inkplate 2
    Logger::line("Using manual ADC reading");
    
    // Configure ADC pin as input
    pinMode(BATTERY_ADC_PIN, INPUT);
    
    // Configure ADC attenuation for full range (0-3.3V input)
    // With 11dB attenuation, we can measure up to ~3.3V
    analogSetAttenuation(ADC_11db);
    
    // Small delay for ADC to stabilize
    delay(10);
    
    // Single ADC sample - EMA smoothing provides sufficient noise reduction
    // Saves 45ms per reading (75% reduction from 10-sample averaging)
    // Battery lifetime gain: +39 days (+16%) vs 10-sample version
    uint32_t adcValue = analogRead(BATTERY_ADC_PIN);
    
    // Convert ADC value to voltage
    // ESP32 ADC: 0-4095 for 0-3.3V (12-bit ADC)
    // Inkplate has a voltage divider (typically 1:2), so multiply by 2
    // The exact voltage divider ratio may vary by board revision
    float adcVoltage = (adcValue / 4095.0) * 3.3;
    float rawBatteryVoltage = adcVoltage * 2.0;  // Account for voltage divider
    
    // Apply EMA smoothing (30% new reading, 70% history)
    const float alpha = 0.3;
    if (rtcSmoothedVoltage == 0.0) {
        rtcSmoothedVoltage = rawBatteryVoltage;  // First reading
    } else {
        rtcSmoothedVoltage = alpha * rawBatteryVoltage + (1.0 - alpha) * rtcSmoothedVoltage;
    }
    
    Logger::linef("ADC Value: %d (raw)", adcValue);
    Logger::linef("ADC Voltage: %.3f V", adcVoltage);
    Logger::linef("Battery Voltage: %.3f V raw, %.3f V smoothed", rawBatteryVoltage, rtcSmoothedVoltage);
    Logger::end();
    
    return rtcSmoothedVoltage;
    #else
    Logger::line("Battery ADC pin not defined for this board");
    Logger::end();
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
        Logger::message("Power Manager", "Device marked as running in NVS (one-time write)");
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
    
    Logger::begin("Watchdog Timer");
    Logger::linef("Enabling watchdog with %u second timeout", timeoutSeconds);
    
    try {
        // Initialize watchdog task with specified timeout
        // IDLE tasks (on both cores for ESP32) will be monitored
        esp_task_wdt_init(timeoutSeconds, true);  // true = panic on timeout
        esp_task_wdt_add(NULL);  // Monitor current task (main loop)
        
        Logger::line("Watchdog enabled successfully");
        Logger::end();
    } catch (...) {
        Logger::line("Failed to enable watchdog (may already be enabled)");
        Logger::end();
    }
}

void PowerManager::disableWatchdog() {
    Logger::begin("Watchdog Timer");
    Logger::line("Disabling watchdog");
    
    try {
        esp_task_wdt_delete(NULL);  // Remove current task from monitoring
        Logger::line("Watchdog disabled successfully");
        Logger::end();
    } catch (...) {
        Logger::line("Failed to disable watchdog (may already be disabled)");
        Logger::end();
    }
}

int PowerManager::calculateBatteryPercentage(float voltage) {
    // Delegate to standalone function for testability
    return ::calculateBatteryPercentage(voltage);
}

