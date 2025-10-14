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

// Create Inkplate instance
Inkplate display(DISPLAY_MODE);
DisplayManager displayManager(&display);
ConfigManager configManager;
WiFiManager wifiManager(&configManager);
ConfigPortal configPortal(&configManager, &wifiManager);
ImageManager imageManager(&display, &displayManager);
PowerManager powerManager;
MQTTManager mqttManager(&configManager);

// Application state
bool apModeActive = false;
bool configModeActive = false;  // Config mode triggered by button
bool normalUpdateActive = false;  // Normal update triggered by short button press
unsigned long configModeStartTime = 0;
const unsigned long CONFIG_MODE_TIMEOUT_MS = 2 * 60 * 1000;  // 2 minutes

// RTC memory for retry mechanism (persists across deep sleep)
RTC_DATA_ATTR uint8_t imageRetryCount = 0;

// Forward declarations
void enterAPMode();
void enterConfigMode();
void performNormalUpdate();  // Renamed from performNormalOperation for clarity

void setup() {
    Serial.begin(115200);
    delay(100);  // Give serial time to initialize
    Serial.println("\n\n=================================");
    Serial.println("Starting " + String(BOARD_NAME) + "...");
    Serial.println("=================================\n");
    
    // Initialize power manager first to detect wake reason
    powerManager.begin(WAKE_BUTTON_PIN);
    WakeupReason wakeReason = powerManager.getWakeupReason();
    
    // Initialize display
    displayManager.init();
    displayManager.clear();
    
    // Show board information
    displayManager.showMessage("Dashboard", 10, 20, 8);
    displayManager.showMessage(BOARD_NAME, 10, 100, 4);
    
    // Show screen dimensions
    String dimensions = String(SCREEN_WIDTH) + "x" + String(SCREEN_HEIGHT);
    displayManager.showMessage(dimensions.c_str(), 10, 160, 3);
    
    // Initialize configuration manager
    if (!configManager.begin()) {
        Serial.println("Failed to initialize ConfigManager!");
        displayManager.showMessage("ERROR: Config Init Failed", 10, 240, 2);
        displayManager.refresh();
        return;
    }
    
    // Determine mode based on wake reason and configuration status
    bool hasWiFi = configManager.hasWiFiConfig();
    bool isFullyConfigured = configManager.isFullyConfigured();
    
    // FIRST BOOT (no WiFi configuration) -> AP Mode (Boot Mode)
    if (!hasWiFi && wakeReason == WAKEUP_FIRST_BOOT) {
        Serial.println("Device NOT configured - entering AP mode (boot mode)");
        enterAPMode();
        return;
    }
    
    // PARTIAL CONFIG (has WiFi but no Image URL) -> Auto-enter Config Mode
    if (hasWiFi && !isFullyConfigured) {
        Serial.println("WiFi configured but missing Image URL - auto-entering config mode");
        enterConfigMode();
        return;
    }
    
    // Check for button press during any wake (button wake, timer wake, or first boot with config)
    // This allows users to enter config mode by holding button during power-on/reset
    if (isFullyConfigured) {
        ButtonPressType pressType = powerManager.detectButtonPressType();
        
        if (pressType == BUTTON_PRESS_LONG) {
            Serial.println("Long button press detected - entering config mode");
            enterConfigMode();
            return;
        } else if (pressType == BUTTON_PRESS_SHORT && wakeReason == WAKEUP_BUTTON) {
            // Only trigger immediate update for short press if it was a button wake
            // (not during timer wake or normal boot)
            Serial.println("Short button press detected - performing normal update");
            performNormalUpdate();
            return;
        }
        // If no button press or short press during non-button wake, continue to normal operation
    }
    
    // TIMER WAKE or normal boot with full configuration -> Normal Operation
    if (isFullyConfigured) {
        Serial.println("Device is fully configured - entering normal operation mode");
        performNormalUpdate();
        return;
    }
    
    // Fallback - shouldn't reach here
    Serial.println("WARNING: Unexpected state - entering AP mode");
    enterAPMode();
}

// Helper function: Enter AP mode (first boot, no configuration)
void enterAPMode() {
    displayManager.showMessage("Status: First Boot", 10, 240, 2);
    displayManager.showMessage("Starting AP Mode...", 10, 270, 2);
    displayManager.refresh();
    
    if (wifiManager.startAccessPoint()) {
        String apName = wifiManager.getAPName();
        String apIP = wifiManager.getAPIPAddress();
        
        displayManager.clear();
        displayManager.showMessage("Setup - Step 1", 10, 20, 6);
        displayManager.showMessage("Connect WiFi", 10, 80, 4);
        displayManager.showMessage("", 10, 120, 2);
        displayManager.showMessage("1. Connect to WiFi:", 10, 140, 3);
        displayManager.showMessage(apName.c_str(), 30, 180, 3);
        displayManager.showMessage("2. Open browser to:", 10, 240, 3);
        displayManager.showMessage(("http://" + apIP).c_str(), 30, 280, 3);
        displayManager.showMessage("3. Enter WiFi settings", 10, 340, 3);
        displayManager.refresh();
        
        // Start configuration portal in BOOT_MODE
        if (configPortal.begin(BOOT_MODE)) {
            apModeActive = true;
            Serial.println("\n=================================");
            Serial.println("Configuration Portal Active (Boot Mode)");
            Serial.println("=================================");
            Serial.println("1. Connect to WiFi: " + apName);
            Serial.println("2. Open: http://" + apIP);
            Serial.println("3. Enter WiFi credentials");
            Serial.println("=================================\n");
        } else {
            Serial.println("Failed to start configuration portal!");
            displayManager.showMessage("ERROR: Portal Failed", 10, 400, 2);
            displayManager.refresh();
        }
    } else {
        Serial.println("Failed to start Access Point!");
        displayManager.showMessage("ERROR: AP Start Failed", 10, 300, 2);
        displayManager.refresh();
    }
}

// Helper function: Enter config mode (button pressed, connect to WiFi and serve portal)
void enterConfigMode() {
    DashboardConfig config;
    bool hasPartialConfig = configManager.hasWiFiConfig() && !configManager.isFullyConfigured();
    
    // For partial config, we may not have a full config to load
    if (!hasPartialConfig && !configManager.loadConfig(config)) {
        Serial.println("Failed to load config in config mode!");
        displayManager.showMessage("ERROR: Config Load Failed", 10, 240, 2);
        displayManager.refresh();
        delay(3000);
        powerManager.prepareForSleep();
        powerManager.enterDeepSleep((uint16_t)5);  // Default 5 minutes
        return;
    }
    
    // For partial config, get WiFi credentials directly
    if (hasPartialConfig) {
        config.wifiSSID = configManager.getWiFiSSID();
        config.wifiPassword = configManager.getWiFiPassword();
        config.refreshRate = DEFAULT_REFRESH_RATE;
    }
    
    // Show config mode message
    displayManager.clear();
    if (hasPartialConfig) {
        displayManager.showMessage("Setup - Step 2", 10, 20, 6);
        displayManager.showMessage("Configure Dashboard", 10, 80, 4);
    } else {
        displayManager.showMessage("Config Mode", 10, 20, 6);
        displayManager.showMessage("Active for 2 minutes", 10, 120, 3);
    }
    displayManager.showMessage("", 10, 160, 2);
    displayManager.showMessage("Connecting to WiFi...", 10, 200, 3);
    displayManager.showMessage(config.wifiSSID.c_str(), 30, 240, 2);
    displayManager.refresh();
    
    // Connect to configured WiFi
    if (wifiManager.connectToWiFi()) {
        String localIP = wifiManager.getLocalIP();
        
        // Publish log message for config mode entry (if MQTT is configured)
        if (mqttManager.begin() && mqttManager.isConfigured()) {
            String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            if (mqttManager.connect()) {
                mqttManager.publishLastLog(deviceId, "Config mode entered", "info");
                mqttManager.disconnect();
            }
        }
        
        displayManager.clear();
        if (hasPartialConfig) {
            displayManager.showMessage("Setup - Step 2", 10, 20, 6);
            displayManager.showMessage("", 10, 100, 2);
        } else {
            displayManager.showMessage("Config Mode Active", 10, 20, 6);
            displayManager.showMessage("", 10, 100, 2);
        }
        displayManager.showMessage("Open browser to:", 10, 140, 3);
        displayManager.showMessage(("http://" + localIP).c_str(), 30, 180, 4);
        displayManager.showMessage("", 10, 240, 2);
        if (!hasPartialConfig) {
            displayManager.showMessage("Timeout: 2 minutes", 10, 280, 2);
        }
        displayManager.refresh();
        
        // Start configuration portal in CONFIG_MODE
        if (configPortal.begin(CONFIG_MODE)) {
            configModeActive = true;
            configModeStartTime = millis();
            
            Serial.println("\n=================================");
            Serial.println("Config Mode Active");
            Serial.println("=================================");
            Serial.println("Access at: http://" + localIP);
            if (!hasPartialConfig) {
                Serial.println("Timeout: 2 minutes");
            }
            Serial.println("=================================\n");
        } else {
            Serial.println("Failed to start configuration portal!");
            displayManager.showMessage("ERROR: Portal Failed", 10, 320, 2);
            displayManager.refresh();
            delay(3000);
            powerManager.prepareForSleep();
            powerManager.enterDeepSleep(config.refreshRate);
        }
    } else {
        // WiFi connection failed - fall back to AP mode
        Serial.println("WiFi connection failed in config mode - falling back to AP mode!");
        
        displayManager.clear();
        displayManager.showMessage("WiFi Failed", 10, 20, 5);
        displayManager.showMessage("Cannot connect to:", 10, 100, 3);
        displayManager.showMessage(config.wifiSSID.c_str(), 10, 140, 2);
        displayManager.showMessage("", 10, 180, 2);
        displayManager.showMessage("Starting AP mode...", 10, 220, 3);
        displayManager.refresh();
        delay(2000);
        
        // Start AP mode as fallback
        if (wifiManager.startAccessPoint()) {
            String apName = wifiManager.getAPName();
            String apIP = wifiManager.getAPIPAddress();
            
            displayManager.clear();
            displayManager.showMessage("Config Mode (AP)", 10, 20, 6);
            displayManager.showMessage("WiFi connection failed", 10, 100, 3);
            displayManager.showMessage("", 10, 140, 2);
            displayManager.showMessage("Connect to WiFi:", 10, 180, 3);
            displayManager.showMessage(apName.c_str(), 30, 220, 3);
            displayManager.showMessage("Open browser to:", 10, 280, 3);
            displayManager.showMessage(("http://" + apIP).c_str(), 30, 320, 3);
            displayManager.refresh();
            
            // Start configuration portal in CONFIG_MODE (with AP)
            if (configPortal.begin(CONFIG_MODE)) {
                configModeActive = true;
                configModeStartTime = millis();
                
                Serial.println("\n=================================");
                Serial.println("Config Mode Active (AP Fallback)");
                Serial.println("=================================");
                Serial.println("1. Connect to WiFi: " + apName);
                Serial.println("2. Open: http://" + apIP);
                Serial.println("3. Update your configuration");
                if (!hasPartialConfig) {
                    Serial.println("Timeout: 2 minutes");
                }
                Serial.println("=================================\n");
            } else {
                Serial.println("Failed to start AP mode fallback!");
                displayManager.showMessage("ERROR: AP Failed", 10, 380, 2);
                displayManager.refresh();
                delay(3000);
                powerManager.prepareForSleep();
                powerManager.enterDeepSleep(config.refreshRate);
            }
        } else {
            Serial.println("Failed to start AP mode fallback!");
            displayManager.clear();
            displayManager.showMessage("Config Mode Failed", 10, 20, 5);
            displayManager.showMessage("Cannot start AP", 10, 100, 3);
            displayManager.showMessage("", 10, 140, 2);
            displayManager.showMessage("Going to sleep...", 10, 180, 2);
            displayManager.refresh();
            delay(3000);
            powerManager.prepareForSleep();
            powerManager.enterDeepSleep(config.refreshRate);
        }
    }
}

// Helper function: Perform normal operation (download image and display)
void performNormalUpdate() {
    // START LOOP TIME MEASUREMENT
    unsigned long loopStartTime = millis();
    unsigned long loopDuration = 0;
    float loopTimeSeconds = 0.0;
    
    DashboardConfig config;
    if (!configManager.loadConfig(config)) {
        Serial.println("Failed to load config!");
        displayManager.showMessage("Status: Config Load Failed", 10, 240, 2);
        displayManager.refresh();
        delay(3000);
        powerManager.prepareForSleep();
        powerManager.enterDeepSleep((uint16_t)5);  // Default 5 minutes
        return;
    }
    
    displayManager.showMessage("Status: Configured", 10, 240, 2);
    displayManager.showMessage(("SSID: " + config.wifiSSID).c_str(), 10, 270, 2);
    displayManager.showMessage(("Refresh: " + String(config.refreshRate) + " min").c_str(), 10, 300, 2);
    displayManager.showMessage("Connecting to WiFi...", 10, 330, 2);
    displayManager.refresh();
    
    // Generate device ID and name for MQTT (needed for both success and error paths)
    String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    String deviceName = "Inkplate Dashboard " + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    // Connect to WiFi
    if (wifiManager.connectToWiFi()) {
        Serial.println("WiFi connected successfully!");
        
        // Measure WiFi signal strength
        int wifiRSSI = WiFi.RSSI();
        Serial.println("WiFi Signal Strength: " + String(wifiRSSI) + " dBm");
        
        // Initialize and connect to MQTT (if configured)
        bool mqttSuccess = false;
        if (mqttManager.begin()) {
            if (mqttManager.isConfigured()) {
                Serial.println("\nMQTT is configured - attempting connection...");
                
                if (mqttManager.connect()) {
                    // Publish Home Assistant discovery
                    mqttManager.publishDiscovery(deviceId, deviceName, BOARD_NAME);
                    
                    // Read and publish battery voltage using Inkplate's readBattery method
                    float batteryVoltage = powerManager.readBatteryVoltage(&display);
                    if (batteryVoltage > 0.0) {
                        mqttManager.publishBatteryVoltage(deviceId, batteryVoltage);
                        mqttSuccess = true;
                    }
                    
                    // Publish WiFi signal strength
                    mqttManager.publishWiFiSignal(deviceId, wifiRSSI);
                    
                    // Disconnect from MQTT (will reconnect later to publish loop time)
                    mqttManager.disconnect();
                } else {
                    Serial.println("MQTT connection failed - continuing without MQTT");
                    Serial.println("Error: " + mqttManager.getLastError());
                    // Note: Can't publish MQTT failure event since we can't connect to MQTT
                }
            } else {
                Serial.println("MQTT not configured - skipping MQTT publishing");
            }
        }
        
        // Download and display image
        Serial.println("\nDownloading image from: " + config.imageURL);
        
        displayManager.clear();
        displayManager.showMessage("Downloading...", 10, 20, 4);
        displayManager.showMessage(config.imageURL.c_str(), 10, 80, 2);
        if (mqttSuccess) {
            displayManager.showMessage("MQTT: Connected", 10, 120, 2);
        }
        displayManager.refresh();
        
        if (imageManager.downloadAndDisplay(config.imageURL.c_str())) {
            Serial.println("Image displayed successfully!");
            displayManager.refresh();
            
            // Reset retry count on success
            imageRetryCount = 0;
            
            // Calculate loop time and publish to MQTT
            loopDuration = millis() - loopStartTime;
            loopTimeSeconds = loopDuration / 1000.0;
            Serial.println("Loop time: " + String(loopTimeSeconds, 2) + " seconds");
            
            // Publish loop time to MQTT (if configured)
            if (mqttManager.isConfigured()) {
                if (mqttManager.connect()) {
                    mqttManager.publishLoopTime(deviceId, loopTimeSeconds);
                    mqttManager.disconnect();
                }
            }
            
            // Success - go to deep sleep
            Serial.println("\nImage display successful - preparing for sleep");
            delay(1000);  // Give user time to see the image
            powerManager.prepareForSleep();
            powerManager.enterDeepSleep(config.refreshRate);
        } else {
            Serial.println("Failed to download/display image!");
            Serial.println("Retry count: " + String(imageRetryCount));
            
            // Check if we should retry silently
            if (imageRetryCount < 2) {
                // Increment retry count and sleep for 20 seconds
                imageRetryCount++;
                Serial.println("Will retry in 20 seconds (attempt " + String(imageRetryCount + 1) + " of 3)");
                
                powerManager.prepareForSleep();
                powerManager.enterDeepSleep(20.0 / 60.0);  // 20 seconds as fraction of minutes
            } else {
                // Max retries reached - show error and reset count
                Serial.println("Max retries reached - showing error message");
                imageRetryCount = 0;
                
                displayManager.clear();
                displayManager.showMessage("Image Error!", 10, 20, 5);
                displayManager.showMessage("Failed to download", 10, 100, 3);
                displayManager.showMessage(config.imageURL.c_str(), 10, 140, 2);
                displayManager.showMessage("", 10, 180, 2);
                displayManager.showMessage(("Error: " + String(imageManager.getLastError())).c_str(), 10, 210, 2);
                displayManager.showMessage("", 10, 250, 2);
                displayManager.showMessage("Will retry on next wake", 10, 280, 2);
                displayManager.showMessage(("(" + String(config.refreshRate) + " minutes)").c_str(), 10, 310, 2);
                displayManager.showMessage("", 10, 350, 2);
                displayManager.showMessage("If you want to update the", 10, 380, 2);
                displayManager.showMessage("Image URL, hold the button", 10, 410, 2);
                displayManager.showMessage("to enter config mode.", 10, 440, 2);
                displayManager.refresh();
                
                // Calculate loop time and publish to MQTT
                loopDuration = millis() - loopStartTime;
                loopTimeSeconds = loopDuration / 1000.0;
                Serial.println("Loop time: " + String(loopTimeSeconds, 2) + " seconds");
                
                // Publish loop time to MQTT (if configured)
                if (mqttManager.isConfigured()) {
                    if (mqttManager.connect()) {
                        mqttManager.publishLoopTime(deviceId, loopTimeSeconds);
                        mqttManager.publishLastLog(deviceId, "Image download failed: " + String(imageManager.getLastError()), "error");
                        mqttManager.disconnect();
                    }
                }
                
                // Error - still go to sleep and retry later
                delay(3000);
                powerManager.prepareForSleep();
                powerManager.enterDeepSleep(config.refreshRate);
            }
        }
    } else {
        Serial.println("WiFi connection failed!");
        
        // Calculate loop time (even though we failed, measure what we did)
        unsigned long loopDuration = millis() - loopStartTime;
        float loopTimeSeconds = loopDuration / 1000.0;
        Serial.println("Loop time: " + String(loopTimeSeconds, 2) + " seconds");
        
        // Note: Can't publish WiFi failure event to MQTT since WiFi failed
        
        displayManager.clear();
        displayManager.showMessage("WiFi Error!", 10, 20, 5);
        displayManager.showMessage("Failed to connect", 10, 100, 3);
        displayManager.showMessage(("SSID: " + config.wifiSSID).c_str(), 10, 140, 2);
        displayManager.showMessage(("Status: " + wifiManager.getStatusString()).c_str(), 10, 170, 2);
        displayManager.showMessage("", 10, 210, 2);
        displayManager.showMessage("Will retry on next wake", 10, 240, 2);
        displayManager.showMessage(("(" + String(config.refreshRate) + " minutes)").c_str(), 10, 270, 2);
        displayManager.refresh();
        
        // Calculate loop time (even though we failed, measure what we did)
        loopDuration = millis() - loopStartTime;
        loopTimeSeconds = loopDuration / 1000.0;
        Serial.println("Loop time: " + String(loopTimeSeconds, 2) + " seconds");
        
        // Note: Can't publish to MQTT since WiFi failed
        
        // WiFi error - go to sleep and retry later
        delay(3000);
        powerManager.prepareForSleep();
        powerManager.enterDeepSleep(config.refreshRate);
    }
}


void loop() {
    if (apModeActive) {
        // Handle AP mode (first boot configuration)
        configPortal.handleClient();
        
        // Check if configuration was received
        if (configPortal.isConfigReceived()) {
            Serial.println("\n=================================");
            Serial.println("WiFi credentials received (boot mode)");
            Serial.println("Restarting to configure dashboard...");
            Serial.println("=================================\n");
            
            displayManager.clear();
            displayManager.showMessage("WiFi Configured!", 10, 100, 5);
            displayManager.showMessage("Restarting...", 10, 200, 3);
            displayManager.refresh();
            
            delay(3000);
            ESP.restart();
        }
    } else if (configModeActive) {
        // Handle config mode (button-triggered or auto-entered)
        configPortal.handleClient();
        
        // Check if configuration was received
        if (configPortal.isConfigReceived()) {
            Serial.println("\n=================================");
            Serial.println("Configuration updated!");
            Serial.println("Restarting in 3 seconds...");
            Serial.println("=================================\n");
            
            // Publish log message for settings updated (if MQTT is configured)
            if (mqttManager.begin() && mqttManager.isConfigured()) {
                String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
                if (mqttManager.connect()) {
                    mqttManager.publishLastLog(deviceId, "Settings updated", "info");
                    mqttManager.disconnect();
                }
            }
            
            displayManager.clear();
            displayManager.showMessage("Settings Updated!", 10, 100, 5);
            displayManager.showMessage("Restarting...", 10, 200, 3);
            displayManager.refresh();
            
            delay(3000);
            ESP.restart();
        }
        
        // Check for timeout (only if not in partial config auto-enter mode)
        bool hasPartialConfig = configManager.hasWiFiConfig() && !configManager.isFullyConfigured();
        if (!hasPartialConfig && millis() - configModeStartTime > CONFIG_MODE_TIMEOUT_MS) {
            Serial.println("\n=================================");
            Serial.println("Config mode timeout!");
            Serial.println("Returning to sleep...");
            Serial.println("=================================\n");
            
            // Publish log message for config mode timeout (if MQTT is configured)
            if (mqttManager.begin() && mqttManager.isConfigured()) {
                String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
                if (mqttManager.connect()) {
                    mqttManager.publishLastLog(deviceId, "Config mode timeout", "info");
                    mqttManager.disconnect();
                }
            }
            
            displayManager.clear();
            displayManager.showMessage("Config Mode Timeout", 10, 100, 4);
            displayManager.showMessage("Going to sleep...", 10, 160, 3);
            displayManager.refresh();
            
            delay(2000);
            
            // Load config to get refresh rate
            DashboardConfig config;
            uint16_t sleepMinutes = 5;  // Default
            if (configManager.loadConfig(config)) {
                sleepMinutes = config.refreshRate;
            }
            
            powerManager.prepareForSleep();
            powerManager.enterDeepSleep(sleepMinutes);
        }
    }
    // Note: Normal operation doesn't reach loop() - it goes directly to deep sleep from setup()
}
