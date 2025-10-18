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
    unsigned long loopStartTime = millis();
    powerManager->enableWatchdog();
    
    DashboardConfig config;
    if (!loadConfiguration(config)) {
        return;
    }
    
    if (config.debugMode) {
        uiStatus->showDebugStatus(config.wifiSSID.c_str(), config.refreshRate);
    }
    
    // Collect telemetry data early
    String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    String deviceName = "Inkplate Dashboard " + String((uint32_t)ESP.getEfuseMac(), HEX);
    float batteryVoltage = powerManager->readBatteryVoltage(display);
    WakeupReason wakeReason = powerManager->getWakeupReason();
    
    // Connect to WiFi
    if (!wifiManager->connectToWiFi()) {
        handleWiFiFailure(config, loopStartTime);
        return;
    }
    
    int wifiRSSI = WiFi.RSSI();
    
    // Check CRC32 and potentially skip download
    uint32_t newCRC32 = 0;
    bool crc32WasChecked = false;
    bool crc32Matched = false;
    
    if (!checkAndHandleCRC32(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI)) {
        return;  // CRC32 matched and timer wake - already went to sleep
    }
    
    // Download and display image
    if (config.debugMode) {
        uiStatus->showDownloading(config.imageURL.c_str(), false);
    }
    
    if (imageManager->downloadAndDisplay(config.imageURL.c_str())) {
        handleImageSuccess(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI);
    } else {
        handleImageFailure(config, loopStartTime, deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI);
    }
}

bool NormalModeController::loadConfiguration(DashboardConfig& config) {
    if (configManager->loadConfig(config)) {
        return true;
    }
    
    uiError->showConfigLoadError();
    delay(3000);
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    powerManager->enterDeepSleep((uint16_t)5);  // No loop timing for early failure
    return false;
}

bool NormalModeController::checkAndHandleCRC32(const DashboardConfig& config, uint32_t& newCRC32, 
                                               bool& crc32WasChecked, bool& crc32Matched,
                                               unsigned long loopStartTime, const String& deviceId, 
                                               const String& deviceName, WakeupReason wakeReason,
                                               float batteryVoltage, int wifiRSSI) {
    if (!config.useCRC32Check) {
        return true;  // Continue execution
    }
    
    crc32WasChecked = true;
    bool shouldDownload = imageManager->checkCRC32Changed(config.imageURL.c_str(), &newCRC32);
    crc32Matched = !shouldDownload;
    
    // If CRC32 matched during timer wake, skip download and sleep
    if (wakeReason == WAKEUP_TIMER && !shouldDownload) {
        float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
        unsigned long loopTimeMs = millis() - loopStartTime;
        
        publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI, loopTimeSeconds,
                           "Image unchanged (CRC32 match)", "info");
        
        powerManager->markDeviceRunning();
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        powerManager->enterDeepSleep((uint16_t)config.refreshRate, loopTimeMs);
        return false;  // Stop execution
    }
    
    return true;  // Continue execution
}

void NormalModeController::publishMQTTTelemetry(const String& deviceId, const String& deviceName, 
                                                WakeupReason wakeReason, float batteryVoltage, 
                                                int wifiRSSI, float loopTimeSeconds,
                                                const char* message, const char* severity) {
    if (mqttManager->begin() && mqttManager->isConfigured()) {
        mqttManager->publishAllTelemetry(deviceId, deviceName, BOARD_NAME, wakeReason,
                                        batteryVoltage, wifiRSSI, loopTimeSeconds, message, severity);
    }
}

void NormalModeController::handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32,
                                              bool crc32WasChecked, bool crc32Matched,
                                              unsigned long loopStartTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int wifiRSSI) {
    // Save CRC32 only if it changed
    if (config.useCRC32Check && crc32WasChecked && !crc32Matched && newCRC32 != 0) {
        imageManager->saveCRC32(newCRC32);
    }
    
    *imageRetryCount = 0;
    float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
    
    // Determine appropriate log message based on CRC32 check results
    const char* logMessage;
    if (config.useCRC32Check && crc32WasChecked && !crc32Matched) {
        logMessage = "Image updated successfully";
    } else {
        logMessage = "Image displayed successfully";
    }
    
    publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI, loopTimeSeconds,
                       logMessage, "info");
    
    enterSleep(config.refreshRate, loopStartTime);
}

void NormalModeController::handleImageFailure(const DashboardConfig& config,
                                              unsigned long loopStartTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int wifiRSSI) {
    if (*imageRetryCount < 2) {
        (*imageRetryCount)++;
        powerManager->markDeviceRunning();
        powerManager->prepareForSleep();
        unsigned long loopTimeMs = millis() - loopStartTime;
        powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds
    } else {
        *imageRetryCount = 0;
        uiError->showImageError(config.imageURL.c_str(), imageManager->getLastError(), config.refreshRate);
        
        float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
        String errorMessage = "Image download failed: " + String(imageManager->getLastError());
        publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI, loopTimeSeconds,
                           errorMessage.c_str(), "error");
        
        delay(3000);
        enterSleep(config.refreshRate, loopStartTime);
    }
}

void NormalModeController::handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime) {
    uiError->showWiFiError(config.wifiSSID.c_str(), wifiManager->getStatusString().c_str(), config.refreshRate);
    delay(3000);
    powerManager->disableWatchdog();
    enterSleep(config.refreshRate, loopStartTime);
}

void NormalModeController::enterSleep(uint16_t minutes, unsigned long loopStartTime) {
    powerManager->markDeviceRunning();
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    unsigned long loopTimeMs = (loopStartTime > 0) ? (millis() - loopStartTime) : 0;
    powerManager->enterDeepSleep(minutes, loopTimeMs);
}
