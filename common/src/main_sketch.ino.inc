// This file contains the shared setup() and loop() implementation
// It should be included from board-specific .ino files after board validation
// Assumes board_config.h has already been included by the .ino file

#include "Inkplate.h"

// Note: Font files are now included in display_manager.h
// No need to include them here anymore

#include <src/config.h>
#include <src/display_manager.h>
#include <src/utils.h>
#include <src/config_manager.h>
#include <src/wifi_manager.h>
#include <src/config_portal.h>
#include <src/image_manager.h>
#include <src/overlay_manager.h>
#include <src/power_manager.h>
#include <src/mqtt_manager.h>
#include <src/logger.h>
#include <src/ui/ui_messages.h>
#include <src/ui/ui_error.h>
#include <src/ui/ui_status.h>
#include <src/modes/ap_mode_controller.h>
#include <src/modes/config_mode_controller.h>
#include <src/modes/normal_mode_controller.h>
#include <src/frontlight_manager.h>


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
OverlayManager overlayManager(&display, &displayManager);
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

#if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true
FrontlightManager frontlightManager(&display);
#endif

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
    
    Logger::begin("Starting " + String(BOARD_NAME));
    Logger::end();
    
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
    
    // Set overlay manager for image manager (for status overlay)
    imageManager.setOverlayManager(&overlayManager);
    
    // Initialize display with rotation
    displayManager.init(shouldShowSplash, screenRotation);
    
    if (shouldShowSplash) {
        uiMessages.showSplashScreen(BOARD_NAME, SCREEN_WIDTH, SCREEN_HEIGHT);
        #ifndef DISPLAY_MODE_INKPLATE2
        // Read and log the panel VCOM for diagnostic purposes (not available on Inkplate 2)
        double panelVcom = displayManager.readPanelVCOM();
        if (!isnan(panelVcom)) {
            Logger::begin("VCOM");
            Logger::linef("VCOM: %.2f V", panelVcom);
            Logger::end();
        } else {
            Logger::begin("VCOM");
            Logger::line("VCOM read failed or not supported");
            Logger::end();
        }
        #endif
    }

    if (!configInitialized) {
        Logger::begin("Configuration");
        Logger::line("Failed to initialize ConfigManager!");
        Logger::end();
        uiMessages.showConfigInitError();
        return;
    }
    
    // Determine mode based on wake reason and configuration status
    bool hasWiFi = configManager.hasWiFiConfig();
    bool isFullyConfigured = configManager.isFullyConfigured();
    
    // RESET BUTTON PRESSED -> Enter config mode (only for button-less boards)
    #if !defined(HAS_BUTTON) || HAS_BUTTON == false
    if (wakeReason == WAKEUP_RESET_BUTTON) {
        Logger::begin("Mode Selection");
        Logger::line("Hardware reset button detected (button-less board) - entering config mode");
        Logger::end();
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
        Logger::begin("Mode Selection");
        Logger::line("Hardware reset button detected - ignoring (wake button available)");
        Logger::end();
        // For boards with wake button, ignore reset button and continue to normal operation
        // Users should use long press on wake button for config mode
    }
    #endif
    
    // FIRST BOOT (no WiFi configuration) -> AP Mode (Boot Mode)
    if (!hasWiFi && wakeReason == WAKEUP_FIRST_BOOT) {
        Logger::begin("Mode Selection");
        Logger::line("Device NOT configured - entering AP mode (boot mode)");
        Logger::end();
        apModeController.begin();
        apModeActive = true;
        return;
    }
    
    // PARTIAL CONFIG (has WiFi but no Image URL) -> Auto-enter Config Mode
    if (hasWiFi && !isFullyConfigured) {
        Logger::begin("Mode Selection");
        Logger::line("WiFi configured but missing Image URL - auto-entering config mode");
        Logger::end();
        configModeController.begin();
        configModeActive = true;
        return;
    }
    
    // Check for button press during any wake (button wake, timer wake, or first boot with config)
    // This allows users to enter config mode by holding button during power-on/reset
    if (isFullyConfigured) {
        ButtonPressType pressType = powerManager.detectButtonPressType();
        
        Logger::begin("Button Detection");
        Logger::linef("pressType=%d, wakeReason=%d (WAKEUP_BUTTON=%d)", 
                     pressType, wakeReason, WAKEUP_BUTTON);
        Logger::end();
        
        if (pressType == BUTTON_PRESS_LONG) {
            Logger::begin("Mode Selection");
            Logger::line("Long button press detected - entering config mode");
            Logger::end();
            configModeController.begin();
            configModeActive = true;
            return;
        } else if (pressType == BUTTON_PRESS_SHORT && wakeReason == WAKEUP_BUTTON) {
            // Only trigger immediate update for short press if it was a button wake
            // (not during timer wake or normal boot)
            Logger::begin("Mode Selection");
            Logger::line("Short button press detected - performing normal update");
            Logger::end();
            
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
        Logger::begin("Mode Selection");
        Logger::line("Device is fully configured - entering normal operation mode");
        Logger::end();
        normalModeController.execute();
        return;
    }
    
    // Fallback - shouldn't reach here
    Logger::begin("Mode Selection");
    Logger::line("WARNING: Unexpected state - entering AP mode");
    Logger::end();
    apModeController.begin();
    apModeActive = true;
}

void loop() {
    if (apModeActive) {
        // Handle AP mode (first boot configuration)
        apModeController.handleClient();
        
        // Check if configuration was received
        if (apModeController.isConfigReceived()) {
            Logger::begin("WiFi Configured");
            Logger::line("WiFi credentials received (boot mode)");
            Logger::line("Restarting to configure dashboard");
            Logger::end();
            
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
            Logger::begin("Configuration Updated");
            Logger::line("Restarting in 3 seconds");
            Logger::end();
            
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

