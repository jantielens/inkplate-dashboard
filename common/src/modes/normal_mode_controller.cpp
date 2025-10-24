#include <src/modes/normal_mode_controller.h>
#include <WiFi.h>

NormalModeController::NormalModeController(Inkplate* disp, ConfigManager* config, WiFiManager* wifi,
                                           ImageManager* image, PowerManager* power, MQTTManager* mqtt,
                                           UIStatus* uiStatus, UIError* uiError, uint8_t* stateIndex)
    : display(disp), configManager(config), wifiManager(wifi),
      imageManager(image), powerManager(power), mqttManager(mqtt),
      uiStatus(uiStatus), uiError(uiError), imageStateIndex(stateIndex) {
}

void NormalModeController::execute() {
    unsigned long loopStartTime = millis();
    powerManager->enableWatchdog();
    
    DashboardConfig config;
    if (!loadConfiguration(config)) {
        return;
    }
    
    if (config.debugMode) {
        int debugInterval = config.getAverageInterval();
        uiStatus->showDebugStatus(config.wifiSSID.c_str(), debugInterval);
    }
    
    // Collect telemetry data early
    String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    String deviceName = "Inkplate Dashboard " + String((uint32_t)ESP.getEfuseMac(), HEX);
    float batteryVoltage = powerManager->readBatteryVoltage(display);
    int batteryPercentage = PowerManager::calculateBatteryPercentage(batteryVoltage);
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
    int currentHour = ConfigManager::applyTimezoneOffset(timeinfo->tm_hour, config.timezoneOffset);
    
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
        bool hourEnabled = ConfigManager::isHourEnabledInBitmask(currentHour, config.updateHours);
        
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
                // Fallback to average interval
                powerManager->enterDeepSleep((float)config.getAverageInterval(), loopTimeMs);
            }
            return;
        }
        LogBox::line("Updates enabled for this hour");
    } else {
        LogBox::line("Skipping hourly schedule enforcement (manual trigger or non-timer wakeup)");
    }
    LogBox::end();
    

    // Get current image index and URL from carousel
    uint8_t currentIndex = *imageStateIndex % config.imageCount;
    String currentImageUrl = config.imageUrls[currentIndex];
    int currentInterval = config.imageIntervals[currentIndex];
    
    // Validate interval (sanity check)
    if (currentInterval < MIN_INTERVAL_MINUTES) {
        LogBox::begin("Config Error");
        LogBox::linef("Invalid interval for image %d, using default", currentIndex + 1);
        LogBox::end();
        currentInterval = DEFAULT_INTERVAL_MINUTES;
    }
    
    // Log mode information
    if (config.isCarouselMode()) {
        LogBox::begin("Carousel Mode");
        LogBox::linef("Displaying image %d of %d", currentIndex + 1, config.imageCount);
        LogBox::line("URL: " + currentImageUrl);
        LogBox::linef("Display for: %d minutes", currentInterval);
        LogBox::end();
    } else {
        LogBox::begin("Single Image Mode");
        LogBox::line("URL: " + currentImageUrl);
        LogBox::linef("Refresh in: %d minutes", currentInterval);
        LogBox::end();
    }
    
    // Check CRC32 and potentially skip download (only for single image mode)
    uint32_t newCRC32 = 0;
    bool crc32WasChecked = false;
    bool crc32Matched = false;
    bool shouldCheckCRC32 = config.useCRC32Check && !config.isCarouselMode();
    
    if (shouldCheckCRC32) {
        if (!checkAndHandleCRC32(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI)) {
            return;  // CRC32 matched and timer wake - already went to sleep
        }
    } else if (config.isCarouselMode()) {
        LogBox::begin("CRC32 Check");
        LogBox::line("CRC32 disabled in carousel mode");
        LogBox::end();
    }
    
    // Download and display image
    if (config.debugMode) {
        uiStatus->showDownloading(currentImageUrl.c_str(), false);
    }
    
    bool success = imageManager->downloadAndDisplay(currentImageUrl.c_str());
    
    // Handle carousel mode vs single image mode
    if (config.isCarouselMode()) {
        if (success) {
            // Success - advance to next image
            *imageStateIndex = (currentIndex + 1) % config.imageCount;
            
            // Save CRC32 if enabled (though disabled in carousel, keep for consistency)
            if (config.useCRC32Check && newCRC32 != 0) {
                imageManager->saveCRC32(newCRC32);
            }
            
            float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
            publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                               0, "Carousel image displayed successfully", "info");
            
            // Sleep with current image's interval
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            unsigned long loopTimeMs = millis() - loopStartTime;
            powerManager->enterDeepSleep((float)currentInterval, loopTimeMs);
        } else {
            // Failed - check if it's the first image
            if (currentIndex == 0) {
                // First image - use retry logic (same as single image mode)
                if (*imageStateIndex < 2) {
                    (*imageStateIndex)++;
                    LogBox::begin("Carousel Error");
                    LogBox::linef("First image failed, retry attempt %d of 2", *imageStateIndex);
                    LogBox::end();
                    
                    powerManager->disableWatchdog();
                    powerManager->prepareForSleep();
                    unsigned long loopTimeMs = millis() - loopStartTime;
                    powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds
                } else {
                    // Exhausted retries on first image - show error and move to next
                    LogBox::begin("Carousel Error");
                    LogBox::line("First image failed after retries, moving to next");
                    LogBox::end();
                    
                    *imageStateIndex = 1;  // Move to second image
                    uiError->showImageError(currentImageUrl.c_str(), imageManager->getLastError(), currentInterval);
                    
                    float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
                    String errorMessage = "First carousel image failed: " + String(imageManager->getLastError());
                    publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                                       0, errorMessage.c_str(), "error");
                    
                    delay(3000);
                    powerManager->disableWatchdog();
                    powerManager->prepareForSleep();
                    unsigned long loopTimeMs = millis() - loopStartTime;
                    powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds to next image
                }
            } else {
                // Non-first image - skip to next immediately
                LogBox::begin("Carousel Error");
                LogBox::linef("Image %d failed, skipping to next", currentIndex + 1);
                LogBox::end();
                
                *imageStateIndex = (currentIndex + 1) % config.imageCount;
                
                float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
                String errorMessage = "Carousel image " + String(currentIndex + 1) + " failed, skipped: " + String(imageManager->getLastError());
                publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                                   0, errorMessage.c_str(), "warning");
                
                powerManager->disableWatchdog();
                powerManager->prepareForSleep();
                unsigned long loopTimeMs = millis() - loopStartTime;
                powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds to next image
            }
        }
    } else {
        // Single image mode - use existing handlers
        if (success) {
            handleImageSuccess(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI);
        } else {
            handleImageFailure(config, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI);
        }
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
    int currentHour = ConfigManager::applyTimezoneOffset(timeinfo->tm_hour, timezoneOffset);
    int currentMinute = timeinfo->tm_min;
    int currentSecond = timeinfo->tm_sec;
    
    // First check if the current hour is enabled
    // If it is, we don't need to sleep - return invalid to signal "don't sleep"
    bool currentHourEnabled = ConfigManager::isHourEnabledInBitmask(currentHour, updateHours);
    
    if (currentHourEnabled) {
        // Current hour is enabled - don't sleep
        return -1;
    }
    
    // Current hour is disabled - find next enabled hour
    for (int i = 1; i <= 24; i++) {
        uint8_t checkHour = (currentHour + i) % 24;
        
        bool isEnabled = ConfigManager::isHourEnabledInBitmask(checkHour, updateHours);
        
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
                                               float batteryVoltage, int batteryPercentage, int wifiRSSI) {
    if (!config.useCRC32Check) {
        return true;  // Continue execution
    }
    
    crc32WasChecked = true;
    // Use first image URL for CRC32 check (only called in single image mode)
    String imageUrl = (config.imageCount > 0) ? config.imageUrls[0] : "";
    bool shouldDownload = imageManager->checkCRC32Changed(imageUrl.c_str(), &newCRC32);
    crc32Matched = !shouldDownload;
    
    // If CRC32 matched during timer wake, skip download and sleep
    if (wakeReason == WAKEUP_TIMER && !shouldDownload) {
        float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
        unsigned long loopTimeMs = millis() - loopStartTime;
        
        publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                           configManager->getLastCRC32(), "Image unchanged (CRC32 match)", "info");
        
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        
        // Calculate precise sleep time
        float sleepMinutes = calculateSleepMinutesToNextEnabledHour(currentTime, config.timezoneOffset, config.updateHours);
        if (sleepMinutes <= 0) {
            // Use first image's interval (single image mode)
            sleepMinutes = (config.imageCount > 0) ? (float)config.imageIntervals[0] : (float)DEFAULT_INTERVAL_MINUTES;
        }
        
        powerManager->enterDeepSleep(sleepMinutes, loopTimeMs);
        return false;  // Stop execution
    }
    
    return true;  // Continue execution
}

void NormalModeController::publishMQTTTelemetry(const String& deviceId, const String& deviceName, 
                                                WakeupReason wakeReason, float batteryVoltage, 
                                                int batteryPercentage, int wifiRSSI, float loopTimeSeconds,
                                                uint32_t imageCRC32, const char* message, const char* severity) {
    if (mqttManager->begin() && mqttManager->isConfigured()) {
        mqttManager->publishAllTelemetry(deviceId, deviceName, BOARD_NAME, wakeReason,
                                        batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds, imageCRC32, message, severity);
    }
}

void NormalModeController::handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32,
                                              bool crc32WasChecked, bool crc32Matched,
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int batteryPercentage, int wifiRSSI) {
    // Save CRC32 only if it changed
    if (config.useCRC32Check && crc32WasChecked && !crc32Matched && newCRC32 != 0) {
        imageManager->saveCRC32(newCRC32);
    }
    
    // Reset state index (single image mode: reset retry counter, carousel: handled separately)
    *imageStateIndex = 0;
    float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
    
    // Determine appropriate log message based on CRC32 check results
    const char* logMessage;
    if (config.useCRC32Check && crc32WasChecked && !crc32Matched) {
        logMessage = "Image updated successfully";
    } else {
        logMessage = "Image displayed successfully";
    }
    
    publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                       configManager->getLastCRC32(), logMessage, "info");
    
    enterSleep(config, currentTime, loopStartTime);
}

void NormalModeController::handleImageFailure(const DashboardConfig& config,
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int batteryPercentage, int wifiRSSI) {
    // Retry logic for single image mode (state index tracks retry attempts)
    if (*imageStateIndex < 2) {
        (*imageStateIndex)++;
        powerManager->prepareForSleep();
        unsigned long loopTimeMs = millis() - loopStartTime;
        powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds
    } else {
        *imageStateIndex = 0;
        // For single image mode, use first image's interval; for carousel use average
        int displayInterval = (config.imageCount > 0) ? config.imageIntervals[0] : DEFAULT_INTERVAL_MINUTES;
        String firstUrl = (config.imageCount > 0) ? config.imageUrls[0] : "";
        uiError->showImageError(firstUrl.c_str(), imageManager->getLastError(), displayInterval);
        
        float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
        String errorMessage = "Image download failed: " + String(imageManager->getLastError());
        publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                           configManager->getLastCRC32(), errorMessage.c_str(), "error");
        
        delay(3000);
        enterSleep(config, currentTime, loopStartTime);
    }
}

void NormalModeController::handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime) {
    int fallbackInterval = config.getAverageInterval();
    uiError->showWiFiError(config.wifiSSID.c_str(), wifiManager->getStatusString().c_str(), fallbackInterval);
    delay(3000);
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    unsigned long loopTimeMs = millis() - loopStartTime;
    powerManager->enterDeepSleep((uint16_t)fallbackInterval, loopTimeMs);
}


void NormalModeController::enterSleep(const DashboardConfig& config, time_t currentTime, unsigned long loopStartTime) {
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    unsigned long loopTimeMs = millis() - loopStartTime;
    
    // Calculate precise sleep time based on hourly schedule
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(currentTime, config.timezoneOffset, config.updateHours);
    
    // If no enabled hours found or calculation failed, use current image's interval
    if (sleepMinutes <= 0) {
        uint8_t currentIndex = *imageStateIndex % config.imageCount;
        sleepMinutes = (float)config.imageIntervals[currentIndex];
    }
    
    powerManager->enterDeepSleep(sleepMinutes, loopTimeMs);
}

