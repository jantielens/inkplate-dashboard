#include "power_manager.h"
#include <esp_sleep.h>
#include <WiFi.h>
#include "Inkplate.h"

// Include board config for battery pins
#include "../../../boards/inkplate5v2/board_config.h"
#ifndef BATTERY_ADC_PIN
#include "../../../boards/inkplate10/board_config.h"
#endif

PowerManager::PowerManager() : _buttonPin(0), _wakeupReason(WAKEUP_FIRST_BOOT) {
}

void PowerManager::begin(uint8_t buttonPin) {
    _buttonPin = buttonPin;
    
    // Configure button pin as input with pull-up
    pinMode(_buttonPin, INPUT_PULLUP);
    
    // Detect why we woke up
    _wakeupReason = detectWakeupReason();
    
    // Configure button as wake source (ext0 - single GPIO)
    // Wake when button is pressed (LOW level, assuming active low button)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)_buttonPin, 0);  // 0 = LOW, 1 = HIGH
    
    Serial.println("PowerManager initialized");
    Serial.printf("Button pin configured: GPIO %d\n", _buttonPin);
    printWakeupReason();
}

WakeupReason PowerManager::getWakeupReason() {
    return _wakeupReason;
}

WakeupReason PowerManager::detectWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by button press (EXT0)");
            return WAKEUP_BUTTON;
            
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            return WAKEUP_TIMER;
            
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            Serial.println("First boot or reset");
            return WAKEUP_FIRST_BOOT;
            
        default:
            Serial.printf("Wakeup caused by unknown reason: %d\n", wakeup_reason);
            return WAKEUP_UNKNOWN;
    }
}

void PowerManager::printWakeupReason() {
    Serial.print("Current wakeup reason: ");
    switch (_wakeupReason) {
        case WAKEUP_TIMER:
            Serial.println("TIMER (normal refresh cycle)");
            break;
        case WAKEUP_BUTTON:
            Serial.println("BUTTON (config mode requested)");
            break;
        case WAKEUP_FIRST_BOOT:
            Serial.println("FIRST_BOOT (initial setup)");
            break;
        default:
            Serial.println("UNKNOWN");
            break;
    }
}

bool PowerManager::isButtonPressed() {
    // Button is active LOW (pressed = LOW, released = HIGH with pull-up)
    return digitalRead(_buttonPin) == LOW;
}

ButtonPressType PowerManager::detectButtonPressType() {
    Serial.println("\n=================================");
    Serial.println("Detecting button press type...");
    Serial.println("=================================");
    
    // Check if button is currently pressed (works for any wake reason)
    if (!isButtonPressed()) {
        // For button wake, this means it was released quickly (short press)
        // For other wake reasons, no button is pressed
        if (_wakeupReason == WAKEUP_BUTTON) {
            Serial.println("Button already released - SHORT PRESS detected");
            Serial.println("=================================\n");
            return BUTTON_PRESS_SHORT;
        } else {
            Serial.println("No button press detected");
            Serial.println("=================================\n");
            return BUTTON_PRESS_NONE;
        }
    }
    
    Serial.println("Button is currently pressed, waiting to determine hold duration...");
    
    // Button is held down, wait for hold duration (2.5 seconds)
    const unsigned long HOLD_THRESHOLD_MS = 2500;
    unsigned long startTime = millis();
    
    // Wait for either button release or timeout
    while (millis() - startTime < HOLD_THRESHOLD_MS) {
        if (!isButtonPressed()) {
            // Button was released before timeout
            unsigned long pressDuration = millis() - startTime;
            Serial.printf("Button released after %lu ms - SHORT PRESS detected\n", pressDuration);
            Serial.println("=================================\n");
            return BUTTON_PRESS_SHORT;
        }
        delay(50);  // Small delay to prevent excessive polling
    }
    
    // Button is still held after threshold - it's a long press
    Serial.printf("Button held for >= %lu ms - LONG PRESS detected\n", HOLD_THRESHOLD_MS);
    Serial.println("=================================\n");
    return BUTTON_PRESS_LONG;
}

uint64_t PowerManager::getSleepDuration(uint16_t refreshRateMinutes) {
    // Convert minutes to microseconds
    // 1 minute = 60 seconds = 60,000,000 microseconds
    uint64_t microseconds = (uint64_t)refreshRateMinutes * 60ULL * 1000000ULL;
    
    Serial.printf("Sleep duration: %u minutes = %llu microseconds\n", 
                  refreshRateMinutes, microseconds);
    
    return microseconds;
}

uint64_t PowerManager::getSleepDuration(float refreshRateMinutes) {
    // Convert minutes to microseconds
    // 1 minute = 60 seconds = 60,000,000 microseconds
    uint64_t microseconds = (uint64_t)(refreshRateMinutes * 60.0 * 1000000.0);
    
    Serial.printf("Sleep duration: %.2f minutes = %llu microseconds\n", 
                  refreshRateMinutes, microseconds);
    
    return microseconds;
}

void PowerManager::prepareForSleep() {
    Serial.println("\n=================================");
    Serial.println("Preparing for deep sleep...");
    Serial.println("=================================");
    
    // Disconnect WiFi
    Serial.println("Disconnecting WiFi...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // Small delay to ensure WiFi is fully shut down
    delay(100);
    
    Serial.println("Ready for deep sleep");
}

void PowerManager::enterDeepSleep(uint16_t refreshRateMinutes) {
    // Calculate sleep duration
    uint64_t sleepDuration = getSleepDuration(refreshRateMinutes);
    
    // Configure timer wake source
    esp_sleep_enable_timer_wakeup(sleepDuration);
    
    Serial.printf("\nEntering deep sleep for %u minutes...\n", refreshRateMinutes);
    Serial.println("Wake sources: TIMER + BUTTON");
    Serial.println("=================================\n");
    
    // Flush serial before sleeping
    Serial.flush();
    
    // Enter deep sleep
    // The device will wake up either from:
    // 1. Timer (after refreshRateMinutes)
    // 2. Button press (ext0 wake source configured in begin())
    esp_deep_sleep_start();
}

void PowerManager::enterDeepSleep(float refreshRateMinutes) {
    // Calculate sleep duration
    uint64_t sleepDuration = getSleepDuration(refreshRateMinutes);
    
    // Configure timer wake source
    esp_sleep_enable_timer_wakeup(sleepDuration);
    
    Serial.printf("\nEntering deep sleep for %.2f minutes...\n", refreshRateMinutes);
    Serial.println("Wake sources: TIMER + BUTTON");
    Serial.println("=================================\n");
    
    // Flush serial before sleeping
    Serial.flush();
    
    // Enter deep sleep
    // The device will wake up either from:
    // 1. Timer (after refreshRateMinutes)
    // 2. Button press (ext0 wake source configured in begin())
    esp_deep_sleep_start();
}

float PowerManager::readBatteryVoltage(void* inkplate) {
    Serial.println("\n=================================");
    Serial.println("Reading battery voltage...");
    Serial.println("=================================");
    
    // If Inkplate object provided, use its built-in readBattery() method
    if (inkplate != nullptr) {
        // Cast to Inkplate* and call readBattery()
        // The Inkplate library has a readBattery() method that returns voltage
        float voltage = ((Inkplate*)inkplate)->readBattery();
        
        Serial.printf("Battery Voltage: %.3f V (from Inkplate library)\n", voltage);
        Serial.println("=================================\n");
        
        return voltage;
    }
    
    #ifdef BATTERY_ADC_PIN
    // Fallback: manual ADC reading if no Inkplate object provided
    Serial.println("Using manual ADC reading (no Inkplate object)");
    
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
    
    Serial.printf("ADC Value: %d (raw)\n", adcValue);
    Serial.printf("ADC Voltage: %.3f V\n", adcVoltage);
    Serial.printf("Battery Voltage: %.3f V (with divider)\n", batteryVoltage);
    Serial.println("=================================\n");
    
    return batteryVoltage;
    #else
    Serial.println("Battery ADC pin not defined for this board");
    Serial.println("=================================\n");
    return 0.0;
    #endif
}
