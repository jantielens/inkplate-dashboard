#include <src/modes/normal_mode_controller.h>
#include <WiFi.h>

NormalModeController::NormalModeController(Inkplate* disp, ConfigManager* config, WiFiManager* wifi,
                                           ImageManager* image, PowerManager* power, MQTTManager* mqtt,
                                           UIStatus* uiStatus, UIError* uiError, uint8_t* retryCount)
    : display(disp), configManager(config), wifiManager(wifi),
      imageManager(image), powerManager(power), mqttManager(mqtt),
      uiStatus(uiStatus), uiError(uiError), imageRetryCount(retryCount) {
}

void NormalModeController::execute() {
    // START LOOP TIME MEASUREMENT
    unsigned long loopStartTime = millis();
    
    // ENABLE WATCHDOG TIMER
    // This protects the entire normal operation cycle (WiFi, MQTT, image download, display)
    // If any step hangs, the device will be forced to deep sleep for recovery
    powerManager->enableWatchdog();
    
    DashboardConfig config;
    if (!loadConfiguration(config)) {
        return;
    }
    
    const bool showDebug = config.debugMode;
    if (showDebug) {
        uiStatus->showDebugStatus(config.wifiSSID.c_str(), config.refreshRate);
    }
    
    // Generate device ID and name for MQTT (needed for both success and error paths)
    String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    String deviceName = "Inkplate Dashboard " + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    // Connect to WiFi
    if (!connectWiFi(config)) {
        handleWiFiFailure(config, loopStartTime);
        return;
    }
    
    // Measure WiFi signal strength
    int wifiRSSI = WiFi.RSSI();
    LogBox::begin("WiFi Connected");
    LogBox::line("WiFi connected successfully");
    LogBox::linef("Signal Strength: %d dBm", wifiRSSI);
    LogBox::end();
    
    // Read battery voltage (needed for MQTT telemetry)
    float batteryVoltage = powerManager->readBatteryVoltage(display);
    
    // Publish MQTT telemetry with single connection (optimized)
    bool mqttSuccess = publishMQTTTelemetry(deviceId, deviceName, wifiRSSI, batteryVoltage);
    
    // Check CRC32 if enabled (always fetch, but only skip download on timer wake)
    uint32_t newCRC32 = 0;
    bool crc32WasChecked = false;
    bool crc32Matched = false;
    
    if (!checkAndHandleCRC32(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime)) {
        // CRC32 matched and timer wake - already went to sleep in the method
        return;
    }
    
    // Download and display image
    LogBox::begin("Image Download");
    LogBox::line("Downloading image from: " + config.imageURL);
    LogBox::end();
    
    if (showDebug) {
        uiStatus->showDownloading(config.imageURL.c_str(), mqttSuccess);
    }
    
    if (downloadAndDisplayImage(config, showDebug, mqttSuccess)) {
        handleImageSuccess(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, deviceId, mqttSuccess);
    } else {
        handleImageFailure(config, loopStartTime, deviceId, mqttSuccess);
    }
}

bool NormalModeController::loadConfiguration(DashboardConfig& config) {
    if (!configManager->loadConfig(config)) {
        LogBox::begin("Config Error");
        LogBox::line("Failed to load config");
        LogBox::end();
        uiError->showConfigLoadError();
        delay(3000);
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        powerManager->enterDeepSleep((uint16_t)5);  // Default 5 minutes
        return false;
    }
    return true;
}

bool NormalModeController::connectWiFi(const DashboardConfig& config) {
    return wifiManager->connectToWiFi();
}

bool NormalModeController::publishMQTTTelemetry(const String& deviceId, const String& deviceName, 
                                                 int wifiRSSI, float batteryVoltage) {
    bool mqttSuccess = false;
    
    if (!mqttManager->begin()) {
        LogBox::begin("MQTT");
        LogBox::line("Failed to initialize MQTT manager");
        LogBox::end();
        return false;
    }
    
    if (!mqttManager->isConfigured()) {
        LogBox::begin("MQTT");
        LogBox::line("MQTT not configured - skipping MQTT publishing");
        LogBox::end();
        return false;
    }
    
    // Determine if we should publish discovery based on wake reason
    WakeupReason wakeReason = powerManager->getWakeupReason();
    bool shouldPublishDiscovery = (wakeReason == WAKEUP_FIRST_BOOT || wakeReason == WAKEUP_RESET_BUTTON);
    
    LogBox::begin("MQTT Telemetry");
    LogBox::line("MQTT is configured - publishing telemetry");
    LogBox::linef("Wake reason: %d", wakeReason);
    LogBox::linef("Publishing discovery: %s", shouldPublishDiscovery ? "YES" : "NO");
    LogBox::end();
    
    // Connect to MQTT and publish discovery + battery + WiFi in batch
    // Loop time will be published later after image handling
    if (mqttManager->connect()) {
        LogBox::begin("MQTT Publishing");
        
        // Publish discovery + battery + WiFi in single batch
        // Loop time is 0.0 as placeholder - will be updated after image handling
        mqttManager->publishTelemetryBatch(deviceId, deviceName, BOARD_NAME, 
                                          batteryVoltage, wifiRSSI, 0.0, 
                                          shouldPublishDiscovery);
        
        LogBox::line("Initial telemetry published (discovery + battery + WiFi)");
        LogBox::end();
        
        mqttSuccess = true;
        // Keep connection open - loop time will be published later
    } else {
        LogBox::begin("MQTT Failed");
        LogBox::line("MQTT connection failed - skipping telemetry");
        LogBox::line("Error: " + mqttManager->getLastError());
        LogBox::end();
    }
    
    return mqttSuccess;
}

bool NormalModeController::checkAndHandleCRC32(const DashboardConfig& config, uint32_t& newCRC32, 
                                               bool& crc32WasChecked, bool& crc32Matched,
                                               unsigned long loopStartTime) {
    bool shouldDownload = true;
    WakeupReason wakeReason = powerManager->getWakeupReason();
    
    if (config.useCRC32Check) {
        LogBox::begin("CRC32 Check");
        LogBox::line("CRC32 check is ENABLED");
        LogBox::end();
        
        crc32WasChecked = true;
        // Always check CRC32 and capture new value (but only skip download on timer wake)
        shouldDownload = imageManager->checkCRC32Changed(config.imageURL.c_str(), &newCRC32);
        crc32Matched = !shouldDownload;  // If shouldDownload is false, CRC32 matched
        
        if (wakeReason == WAKEUP_TIMER && !shouldDownload) {
            // Image hasn't changed during normal timer wake - skip download and go to sleep
            LogBox::begin("Image Status");
            LogBox::line("Image unchanged - skipping download and going to sleep");
            
            // Calculate loop time
            unsigned long loopDuration = millis() - loopStartTime;
            float loopTimeSeconds = loopDuration / 1000.0;
            LogBox::linef("Loop time (CRC32 only): %.2f seconds", loopTimeSeconds);
            LogBox::end();
            
            // Publish loop time to MQTT (if configured)
            if (mqttManager->isConfigured()) {
                String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
                if (mqttManager->connect()) {
                    mqttManager->publishLoopTime(deviceId, loopTimeSeconds);
                    mqttManager->publishLastLog(deviceId, "Image unchanged (CRC32 match)", "info");
                    mqttManager->disconnect();
                }
            }
            
            // Mark device as running
            powerManager->markDeviceRunning();
            
            // Go to sleep without updating display
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            powerManager->enterDeepSleep((uint16_t)config.refreshRate);
            return false;  // Signal to stop execution
        } else if (wakeReason != WAKEUP_TIMER && !shouldDownload) {
            // CRC32 matched but not a timer wake (button press, config mode, etc.)
            // Still download to refresh display
            LogBox::begin("Image Download Reason");
            LogBox::line("CRC32 matched but not a normal timer wake");
            LogBox::line("Downloading anyway to refresh display");
            LogBox::end();
            shouldDownload = true;
        }
    } else {
        LogBox::begin("CRC32 Check");
        LogBox::line("CRC32 check is DISABLED");
        LogBox::end();
    }
    
    return true;  // Continue execution
}

bool NormalModeController::downloadAndDisplayImage(const DashboardConfig& config, bool showDebug, bool mqttSuccess) {
    return imageManager->downloadAndDisplay(config.imageURL.c_str());
}

void NormalModeController::handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32,
                                              bool crc32WasChecked, bool crc32Matched,
                                              unsigned long loopStartTime, const String& deviceId, bool mqttConnected) {
    LogBox::begin("Image Success");
    LogBox::line("Image displayed successfully");
    
    // Refresh display without clearing (image is already drawn)
    // Note: displayManager is not available here, so we skip this line
    // The imageManager.downloadAndDisplay already handles the refresh
    
    // Save CRC32 only AFTER successful display
    if (config.useCRC32Check) {
        LogBox::begin("CRC32 Save");
        LogBox::linef("Debug: crc32WasChecked=%d, crc32Matched=%d, newCRC32=0x%08X", 
                     crc32WasChecked, crc32Matched, newCRC32);
        if (crc32WasChecked && crc32Matched) {
            // CRC32 matched - no need to save the same value again
            LogBox::line("Skipped - CRC32 unchanged (same as stored value)");
        } else if (newCRC32 != 0) {
            // CRC32 changed or first time - save new value
            imageManager->saveCRC32(newCRC32);
        } else if (crc32WasChecked) {
            // CRC32 check was performed but returned 0 (no .crc32 file on server)
            LogBox::line("Skipped - no CRC32 value from server (file may not be available)");
        } else {
            // CRC32 checking is disabled
            LogBox::line("Skipped - CRC32 checking disabled");
        }
        LogBox::end();
    }
    
    // Reset retry count on success
    *imageRetryCount = 0;
    
    // Calculate loop time and publish to MQTT
    unsigned long loopDuration = millis() - loopStartTime;
    float loopTimeSeconds = loopDuration / 1000.0;
    LogBox::linef("Loop time: %.2f seconds", loopTimeSeconds);
    LogBox::end();
    
    // Publish loop time to MQTT (if configured and already connected)
    if (mqttManager->isConfigured() && mqttConnected) {
        // Connection already open - just publish loop time without reconnecting
        mqttManager->publishLoopTime(deviceId, loopTimeSeconds);
    }
    
    // Success - go to deep sleep
    LogBox::begin("Sleep Preparation");
    LogBox::line("Image display successful - preparing for sleep");
    LogBox::end();
    delay(1000);  // Give user time to see the image
    
    enterSleep(config.refreshRate);
}

void NormalModeController::handleImageFailure(const DashboardConfig& config,
                                              unsigned long loopStartTime, const String& deviceId, bool mqttConnected) {
    LogBox::begin("Image Error");
    LogBox::line("Failed to download/display image");
    LogBox::linef("Retry count: %d", *imageRetryCount);
    
    // Check if we should retry silently
    if (*imageRetryCount < 2) {
        // Increment retry count and sleep for 20 seconds
        (*imageRetryCount)++;
        LogBox::linef("Will retry in 20 seconds (attempt %d of 3)", *imageRetryCount + 1);
        LogBox::end();
        
        powerManager->markDeviceRunning();
        powerManager->prepareForSleep();
        powerManager->enterDeepSleep((float)(20.0 / 60.0));  // 20 seconds as fraction of minutes
    } else {
        // Max retries reached - show error and reset count
        LogBox::line("Max retries reached - showing error message");
        LogBox::end();
        *imageRetryCount = 0;
        
        uiError->showImageError(config.imageURL.c_str(), imageManager->getLastError(), config.refreshRate);
        
        // Calculate loop time and publish to MQTT
        unsigned long loopDuration = millis() - loopStartTime;
        float loopTimeSeconds = loopDuration / 1000.0;
        LogBox::begin("Loop Time");
        LogBox::linef("Loop time: %.2f seconds", loopTimeSeconds);
        LogBox::end();
        
        // Publish loop time to MQTT (if configured and already connected)
        if (mqttManager->isConfigured() && mqttConnected) {
            // Connection already open - just publish loop time without reconnecting
            mqttManager->publishLoopTime(deviceId, loopTimeSeconds);
            mqttManager->publishLastLog(deviceId, "Image download failed: " + String(imageManager->getLastError()), "error");
        }
        
        // Error - still go to sleep and retry later
        delay(3000);
        
        enterSleep(config.refreshRate);
    }
}

void NormalModeController::handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime) {
    LogBox::begin("WiFi Error");
    LogBox::line("WiFi connection failed");
    
    // Calculate loop time (even though we failed, measure what we did)
    unsigned long loopDuration = millis() - loopStartTime;
    float loopTimeSeconds = loopDuration / 1000.0;
    LogBox::linef("Loop time: %.2f seconds", loopTimeSeconds);
    LogBox::end();
    
    // Note: Can't publish WiFi failure event to MQTT since WiFi failed
    
    uiError->showWiFiError(config.wifiSSID.c_str(), wifiManager->getStatusString().c_str(), config.refreshRate);
    
    // WiFi error - go to sleep and retry later
    delay(3000);
    
    powerManager->disableWatchdog();
    enterSleep(config.refreshRate);
}

void NormalModeController::enterSleep(uint16_t minutes) {
    powerManager->markDeviceRunning();
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    powerManager->enterDeepSleep(minutes);
}
