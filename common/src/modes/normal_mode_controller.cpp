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
    
    // Connect to WiFi to sync time via NTP
    if (!wifiManager->connectToWiFi()) {
        handleWiFiFailure(config, loopStartTime);
        return;
    }
    
    int wifiRSSI = WiFi.RSSI();
    
    // Sync time via NTP and check if current hour is enabled for updates
    // Configure timezone to UTC first (we'll apply offset manually)
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    // Wait for NTP sync with timeout (up to 15 seconds)
    LogBox::begin("NTP Time Sync");
    time_t now = time(nullptr);
    int ntpRetries = 0;
    while (now < 24 * 3600 && ntpRetries < 30) {  // 24*3600 = 1970-01-01 00:40:00 (minimal valid time)
        delay(500);
        now = time(nullptr);
        ntpRetries++;
    }
    
    if (now < 24 * 3600) {
        LogBox::line("WARNING: NTP sync timeout, using last known time");
        LogBox::end();
    } else {
        LogBox::line("Time synced via NTP");
        LogBox::end();
    }
    
    struct tm* timeinfo = localtime(&now);
    
    // Apply timezone offset
    int currentHour = (timeinfo->tm_hour + config.timezoneOffset) % 24;
    if (currentHour < 0) {
        currentHour += 24;  // Handle negative offsets
    }
    
    // Log current time
    LogBox::begin("Hourly Schedule Check");
    LogBox::linef("Current time (UTC): %04d-%02d-%02d %02d:%02d:%02d",
                  timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    LogBox::linef("Timezone offset: %+d, Local hour: %d", config.timezoneOffset, currentHour);
    
    // Only enforce hourly schedule for timer-based wakeups from deep sleep
    // Skip enforcement for button presses, config mode, first boot, and other manual triggers
    bool shouldEnforceHourlySchedule = (wakeReason == WAKEUP_TIMER);
    
    if (shouldEnforceHourlySchedule) {
        // Check if current hour is enabled using the loaded config
        uint8_t byteIndex = currentHour / 8;
        uint8_t bitPosition = currentHour % 8;
        bool hourEnabled = (config.updateHours[byteIndex] >> bitPosition) & 1;
        
        if (!hourEnabled) {
            // Updates disabled for this hour - calculate precise sleep until next enabled hour
            LogBox::line("Updates disabled for this hour");
            
            float sleepMinutes = calculateSleepMinutesToNextEnabledHour(now, config.timezoneOffset, config.updateHours);
            LogBox::linef("Sleeping %.1f minutes until next enabled hour", sleepMinutes);
            LogBox::end();
            
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            unsigned long loopTimeMs = millis() - loopStartTime;
            
            if (sleepMinutes > 0) {
                powerManager->enterDeepSleep(sleepMinutes, loopTimeMs);
            } else {
                powerManager->enterDeepSleep((float)config.refreshRate, loopTimeMs);  // Fallback to refresh rate
            }
            return;
        }
        LogBox::line("Updates enabled for this hour");
    } else {
        LogBox::line("Skipping hourly schedule enforcement (manual trigger or non-timer wakeup)");
    }
    LogBox::end();
    

    // Check CRC32 and potentially skip download
    uint32_t newCRC32 = 0;
    bool crc32WasChecked = false;
    bool crc32Matched = false;
    
    if (!checkAndHandleCRC32(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI)) {
        return;  // CRC32 matched and timer wake - already went to sleep
    }
    
    // Download and display image
    if (config.debugMode) {
        uiStatus->showDownloading(config.imageURL.c_str(), false);
    }
    
    if (imageManager->downloadAndDisplay(config.imageURL.c_str())) {
        handleImageSuccess(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI);
    } else {
        handleImageFailure(config, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, wifiRSSI);
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

int NormalModeController::calculateSleepUntilNextEnabledHour(uint8_t currentHour, const uint8_t updateHours[3]) {
    // Find next enabled hour starting from currentHour+1, wrapping around at 24
    for (int i = 1; i <= 24; i++) {
        uint8_t checkHour = (currentHour + i) % 24;
        
        // Check if hour is enabled using the passed updateHours array
        uint8_t byteIndex = checkHour / 8;
        uint8_t bitPosition = checkHour % 8;
        bool isEnabled = (updateHours[byteIndex] >> bitPosition) & 1;
        
        if (isEnabled) {
            // Found next enabled hour
            if (checkHour <= currentHour) {
                // Wrapped around to next day
                return (24 - currentHour) + checkHour;  // Hours until midnight + hours to enabled hour
            } else {
                return checkHour - currentHour;
            }
        }
    }
    
    // Fallback: all hours disabled (shouldn't happen with defaults), sleep full refresh rate
    return -1;
}

float NormalModeController::calculateSleepMinutesToNextEnabledHour(time_t currentTime, int timezoneOffset, const uint8_t updateHours[3]) {
    // Get current time info
    struct tm* timeinfo = localtime(&currentTime);
    
    // Calculate current local hour and minutes
    int currentHour = (timeinfo->tm_hour + timezoneOffset) % 24;
    if (currentHour < 0) {
        currentHour += 24;
    }
    int currentMinute = timeinfo->tm_min;
    int currentSecond = timeinfo->tm_sec;
    
    // First check if the current hour is enabled
    // If it is, we don't need to sleep - return invalid to signal "don't sleep"
    uint8_t byteIndex = currentHour / 8;
    uint8_t bitPosition = currentHour % 8;
    bool currentHourEnabled = (updateHours[byteIndex] >> bitPosition) & 1;
    
    if (currentHourEnabled) {
        // Current hour is enabled - don't sleep
        return -1;
    }
    
    // Current hour is disabled - find next enabled hour
    for (int i = 1; i <= 24; i++) {
        uint8_t checkHour = (currentHour + i) % 24;
        
        uint8_t checkByteIndex = checkHour / 8;
        uint8_t checkBitPosition = checkHour % 8;
        bool isEnabled = (updateHours[checkByteIndex] >> checkBitPosition) & 1;
        
        if (isEnabled) {
            // Found next enabled hour - calculate precise sleep time
            float hoursToSleep;
            
            if (checkHour <= currentHour) {
                // Wrapped to next day
                hoursToSleep = (24 - currentHour) + checkHour;
            } else {
                // Same day
                hoursToSleep = checkHour - currentHour;
            }
            
            // Subtract current minutes and seconds to get precise sleep time
            // (sleep until the start of the next enabled hour)
            float minutesAndSecondsToSubtract = currentMinute + (currentSecond / 60.0);
            float sleepMinutes = (hoursToSleep * 60.0) - minutesAndSecondsToSubtract;
            
            return sleepMinutes;
        }
    }
    
    // Fallback: all hours disabled (shouldn't happen with defaults)
    return -1;
}

bool NormalModeController::checkAndHandleCRC32(const DashboardConfig& config, uint32_t& newCRC32, 
                                               bool& crc32WasChecked, bool& crc32Matched,
                                               unsigned long loopStartTime, time_t currentTime, const String& deviceId, 
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
        
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        
        // Calculate precise sleep time
        float sleepMinutes = calculateSleepMinutesToNextEnabledHour(currentTime, config.timezoneOffset, config.updateHours);
        if (sleepMinutes <= 0) {
            sleepMinutes = config.refreshRate;
        }
        
        powerManager->enterDeepSleep(sleepMinutes, loopTimeMs);
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
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
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
    
    enterSleep(config, currentTime, loopStartTime);
}

void NormalModeController::handleImageFailure(const DashboardConfig& config,
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int wifiRSSI) {
    if (*imageRetryCount < 2) {
        (*imageRetryCount)++;
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
        enterSleep(config, currentTime, loopStartTime);
    }
}

void NormalModeController::handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime) {
    uiError->showWiFiError(config.wifiSSID.c_str(), wifiManager->getStatusString().c_str(), config.refreshRate);
    delay(3000);
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    unsigned long loopTimeMs = millis() - loopStartTime;
    powerManager->enterDeepSleep((uint16_t)config.refreshRate, loopTimeMs);
}


void NormalModeController::enterSleep(const DashboardConfig& config, time_t currentTime, unsigned long loopStartTime) {
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    unsigned long loopTimeMs = millis() - loopStartTime;
    
    // Calculate precise sleep time based on hourly schedule
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(currentTime, config.timezoneOffset, config.updateHours);
    
    // If no enabled hours found or calculation failed, use refresh rate
    if (sleepMinutes <= 0) {
        sleepMinutes = config.refreshRate;
    }
    
    powerManager->enterDeepSleep(sleepMinutes, loopTimeMs);
}

