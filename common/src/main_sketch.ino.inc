// This file contains the shared setup() and loop() implementation
// It should be included from board-specific .ino files after board validation
// Assumes board_config.h has already been included by the .ino file

#include "Inkplate.h"
#include <src/config.h>
#include <src/display_manager.h>
#include <src/utils.h>
#include <src/config_manager.h>
#include <src/wifi_manager.h>
#include <src/config_portal.h>
#include <src/image_manager.h>
#include <src/power_manager.h>
#include <src/mqtt_manager.h>
#include <src/logger.h>
#include <src/ui/ui_messages.h>
#include <src/ui/ui_error.h>
#include <src/ui/ui_status.h>
#include <src/modes/ap_mode_controller.h>
#include <src/modes/config_mode_controller.h>
#include <src/modes/normal_mode_controller.h>


// Create Inkplate instance
// Inkplate 2 uses a different constructor (no display mode parameter)
#ifdef DISPLAY_MODE_INKPLATE2
Inkplate display;
#else
Inkplate display(DISPLAY_MODE);
#endif
DisplayManager displayManager(&display);
ConfigManager configManager;
WiFiManager wifiManager(&configManager);
ConfigPortal configPortal(&configManager, &wifiManager, &displayManager);
ImageManager imageManager(&display, &displayManager);
PowerManager powerManager;
MQTTManager mqttManager(&configManager);

// Application state
bool apModeActive = false;
bool configModeActive = false;  // Config mode triggered by button
bool normalUpdateActive = false;  // Normal update triggered by short button press
unsigned long configModeStartTime = 0;

// RTC memory for carousel position and retry state (persists across deep sleep)
// In carousel mode: tracks which image to display next (0-9)
// In single image mode: tracks retry attempts (0-2)
RTC_DATA_ATTR uint8_t imageStateIndex = 0;

// UI Components
UIMessages uiMessages(&displayManager);
UIError uiError(&displayManager);
UIStatus uiStatus(&displayManager);

// Mode Controllers
APModeController apModeController(&wifiManager, &configPortal, &uiStatus, &uiError);
ConfigModeController configModeController(&configManager, &wifiManager, &configPortal, &mqttManager, &powerManager, &uiStatus, &uiError);
NormalModeController normalModeController(&display, &configManager, &wifiManager, &imageManager, &powerManager, &mqttManager, &uiStatus, &uiError, &imageStateIndex);

void setup() {
    Serial.begin(115200);
    delay(100);  // Give serial time to initialize
    
    LogBox::begin("Starting " + String(BOARD_NAME));
    LogBox::end();
    
    // Initialize power manager first to detect wake reason
    powerManager.begin(WAKE_BUTTON_PIN);
    WakeupReason wakeReason = powerManager.getWakeupReason();
    
    // Wire up power manager to WiFi manager for smart connection strategy
    wifiManager.setPowerManager(&powerManager);

    // Initialize configuration early to determine debug behavior and rotation
    bool configInitialized = configManager.begin();
    bool debugModeEnabled = false;
    uint8_t screenRotation = 0;
    if (configInitialized && configManager.isConfigured()) {
        debugModeEnabled = configManager.getDebugMode();
        screenRotation = configManager.getScreenRotation();
    }
    bool shouldShowSplash = ((wakeReason == WAKEUP_FIRST_BOOT) || debugModeEnabled);
#if !DISPLAY_FAST_REFRESH
    // Skip splash screen on slow displays to reduce boot time
    shouldShowSplash = false;
#endif
    
    // Set config manager for image manager (for CRC32 storage)
    imageManager.setConfigManager(&configManager);
    
    // Initialize display with rotation
    displayManager.init(shouldShowSplash, screenRotation);
    
    if (shouldShowSplash) {
        uiMessages.showSplashScreen(BOARD_NAME, SCREEN_WIDTH, SCREEN_HEIGHT);
        #ifndef DISPLAY_MODE_INKPLATE2
        // Read and log the panel VCOM for diagnostic purposes (not available on Inkplate 2)
        double panelVcom = displayManager.readPanelVCOM();
        if (!isnan(panelVcom)) {
            LogBox::begin("VCOM");
            LogBox::linef("VCOM: %.2f V", panelVcom);
            LogBox::end();
        } else {
            LogBox::begin("VCOM");
            LogBox::line("VCOM read failed or not supported");
            LogBox::end();
        }
        #endif
    }

    if (!configInitialized) {
        LogBox::begin("Configuration");
        LogBox::line("Failed to initialize ConfigManager!");
        LogBox::end();
        uiMessages.showConfigInitError();
        return;
    }
    
    // Determine mode based on wake reason and configuration status
    bool hasWiFi = configManager.hasWiFiConfig();
    bool isFullyConfigured = configManager.isFullyConfigured();
    
    // RESET BUTTON PRESSED -> Enter config mode (only for button-less boards)
    #if !defined(HAS_BUTTON) || HAS_BUTTON == false
    if (wakeReason == WAKEUP_RESET_BUTTON) {
        LogBox::begin("Mode Selection");
        LogBox::line("Hardware reset button detected (button-less board) - entering config mode");
        LogBox::end();
        if (isFullyConfigured) {
            configModeController.begin();
            configModeActive = true;
        } else {
            apModeController.begin();
            apModeActive = true;
        }
        return;
    }
    #else
    if (wakeReason == WAKEUP_RESET_BUTTON) {
        LogBox::begin("Mode Selection");
        LogBox::line("Hardware reset button detected - ignoring (wake button available)");
        LogBox::end();
        // For boards with wake button, ignore reset button and continue to normal operation
        // Users should use long press on wake button for config mode
    }
    #endif
    
    // FIRST BOOT (no WiFi configuration) -> AP Mode (Boot Mode)
    if (!hasWiFi && wakeReason == WAKEUP_FIRST_BOOT) {
        LogBox::begin("Mode Selection");
        LogBox::line("Device NOT configured - entering AP mode (boot mode)");
        LogBox::end();
        apModeController.begin();
        apModeActive = true;
        return;
    }
    
    // PARTIAL CONFIG (has WiFi but no Image URL) -> Auto-enter Config Mode
    if (hasWiFi && !isFullyConfigured) {
        LogBox::begin("Mode Selection");
        LogBox::line("WiFi configured but missing Image URL - auto-entering config mode");
        LogBox::end();
        configModeController.begin();
        configModeActive = true;
        return;
    }
    
    // Check for button press during any wake (button wake, timer wake, or first boot with config)
    // This allows users to enter config mode by holding button during power-on/reset
    if (isFullyConfigured) {
        ButtonPressType pressType = powerManager.detectButtonPressType();
        
        LogBox::begin("Button Detection");
        LogBox::linef("pressType=%d, wakeReason=%d (WAKEUP_BUTTON=%d)", 
                     pressType, wakeReason, WAKEUP_BUTTON);
        LogBox::end();
        
        if (pressType == BUTTON_PRESS_LONG) {
            LogBox::begin("Mode Selection");
            LogBox::line("Long button press detected - entering config mode");
            LogBox::end();
            configModeController.begin();
            configModeActive = true;
            return;
        } else if (pressType == BUTTON_PRESS_SHORT && wakeReason == WAKEUP_BUTTON) {
            // Only trigger immediate update for short press if it was a button wake
            // (not during timer wake or normal boot)
            LogBox::begin("Mode Selection");
            LogBox::line("Short button press detected - performing normal update");
            LogBox::end();
            
            #if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true
            // Enable frontlight for manual refresh (Inkplate 6 Flick)
            display.frontlight(true);
            display.setFrontlight(63);  // Max brightness
            LogBox::begin("Frontlight");
            LogBox::line("Frontlight enabled at max brightness for manual refresh");
            LogBox::end();
            #endif
            
            // Show immediate visual feedback for manual refresh
            uiStatus.showManualRefresh();
            delay(1500); // Show message for 1.5 seconds
            
            normalModeController.execute();
            return;
        }
        // If no button press or short press during non-button wake, continue to normal operation
    }
    
    // TIMER WAKE or normal boot with full configuration -> Normal Operation
    if (isFullyConfigured) {
        LogBox::begin("Mode Selection");
        LogBox::line("Device is fully configured - entering normal operation mode");
        LogBox::end();
        normalModeController.execute();
        return;
    }
    
    // Fallback - shouldn't reach here
    LogBox::begin("Mode Selection");
    LogBox::line("WARNING: Unexpected state - entering AP mode");
    LogBox::end();
    apModeController.begin();
    apModeActive = true;
}

void loop() {
    if (apModeActive) {
        // Handle AP mode (first boot configuration)
        apModeController.handleClient();
        
        // Check if configuration was received
        if (apModeController.isConfigReceived()) {
            LogBox::begin("WiFi Configured");
            LogBox::line("WiFi credentials received (boot mode)");
            LogBox::line("Restarting to configure dashboard");
            LogBox::end();
            
#if DISPLAY_FAST_REFRESH
            uiStatus.showWiFiConfigured();
            delay(3000);
#else
            delay(1000);  // Brief delay on slow displays
#endif
            ESP.restart();
        }
    } else if (configModeActive) {
        // Handle config mode (button-triggered or auto-entered)
        configModeController.handleClient();
        
        // Check if configuration was received
        if (configModeController.isConfigReceived()) {
            LogBox::begin("Configuration Updated");
            LogBox::line("Configuration updated");
            LogBox::line("Restarting in 3 seconds");
            LogBox::end();
            
            // Publish log message for settings updated (if MQTT is configured)
            if (mqttManager.begin() && mqttManager.isConfigured()) {
                String deviceId = wifiManager.getDeviceIdentifier();
                if (mqttManager.connect()) {
                    mqttManager.publishLastLog(deviceId, "Settings updated", "info");
                    mqttManager.disconnect();
                }
            }
            
#if DISPLAY_FAST_REFRESH
            uiStatus.showSettingsUpdated();
            delay(3000);
#else
            delay(1000);  // Brief delay on slow displays
#endif
            ESP.restart();
        }
        
        // Check for timeout
        if (configModeController.isTimedOut(configModeStartTime)) {
            // Load config to get average interval
            DashboardConfig config;
            uint16_t sleepMinutes = DEFAULT_INTERVAL_MINUTES;  // Default
            if (configManager.loadConfig(config)) {
                sleepMinutes = config.getAverageInterval();
            }
            
            configModeController.handleTimeout(sleepMinutes);
        }
    }
    // Note: Normal operation doesn't reach loop() - it goes directly to deep sleep from setup()
}

