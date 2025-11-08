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
    
    // Initialize timing structure
    LoopTimings timings;
    uint32_t timerStart;
    
    DashboardConfig config;
    if (!loadConfiguration(config)) {
        return;
    }
    
    // Capture image retry count from RTC memory (only for single image mode)
    // In single image mode: imageStateIndex tracks retry attempts (0 = first attempt, 1-2 = retries)
    // In carousel mode: imageStateIndex tracks carousel position, so we report 0 retries
    if (config.imageCount == 1) {
        timings.image_retry_count = *imageStateIndex;
    } else {
        // Carousel mode - retry count not applicable (report 0)
        timings.image_retry_count = 0;
    }
    
    if (config.debugMode) {
        int debugInterval = config.getAverageInterval();
        uiStatus->showDebugStatus(config.wifiSSID.c_str(), debugInterval);
    }
    
    // Collect telemetry data early
    String deviceId = wifiManager->getDeviceIdentifier();
    
    // For device name: use the friendly name directly if set, otherwise use "Inkplate Dashboard <MAC>"
    String deviceName;
    if (config.friendlyName.length() > 0) {
        // Use the original friendly name for display (keeps spaces, capitalization)
        deviceName = config.friendlyName;
    } else {
        // Fallback: use MAC-based name with "Inkplate Dashboard" prefix
        String deviceNameSuffix = deviceId;
        if (deviceNameSuffix.startsWith("inkplate-")) {
            deviceNameSuffix = deviceNameSuffix.substring(9);  // Remove "inkplate-" prefix
        }
        deviceName = "Inkplate Dashboard " + deviceNameSuffix;
    }
    float batteryVoltage = powerManager->readBatteryVoltage(display);
    int batteryPercentage = PowerManager::calculateBatteryPercentage(batteryVoltage);
    WakeupReason wakeReason = powerManager->getWakeupReason();
    
    // Connect to WiFi to sync time via NTP (measure timing)
    timerStart = millis();
    if (!wifiManager->connectToWiFi(&timings.wifi_retry_count)) {
        handleWiFiFailure(config, loopStartTime);
        return;
    }
    timings.wifi_ms = millis() - timerStart;
    
    int wifiRSSI = WiFi.RSSI();
    String wifiBSSID = WiFi.BSSIDstr();
    
    // Check if NTP sync is needed
    // Skip NTP sync if all 24 hours are enabled (no hourly scheduling needed)
    // Also skip if all hours are disabled (dormant mode - no scheduling needed)
    bool allHoursEnabled = ConfigManager::areAllHoursEnabled(config.updateHours);
    bool allHoursDisabled = ConfigManager::areAllHoursDisabled(config.updateHours);
    time_t now;
    
    if (allHoursEnabled || allHoursDisabled) {
        // Skip NTP sync for scheduling optimization
        LogBox::begin("NTP Time Sync");
        if (allHoursEnabled) {
            LogBox::line("Skipped - all 24 hours enabled");
        } else {
            LogBox::line("Skipped - all 24 hours disabled (dormant mode)");
        }
        LogBox::end();
        now = time(nullptr);  // Use last known time
        timings.ntp_ms = 0;
    } else {
        // Sync time via NTP to check if current hour is enabled for updates
        // Configure timezone to UTC first (we'll apply offset manually)
        timerStart = millis();
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        
        // Wait for NTP sync with timeout (up to 7 seconds)
        // Reduced from 15s (30 × 500ms) to 7s (70 × 100ms)
        // Real-world data shows normal syncs: 0-0.01s, first boot: ~4s, rare spike: 3.5s
        LogBox::begin("NTP Time Sync");
        now = time(nullptr);
        int ntpRetries = 0;
        while (now < 24 * 3600 && ntpRetries < 70) {  // 24*3600 = 1970-01-01 00:40:00 (minimal valid time)
            delay(100);  // Reduced from 500ms to 100ms for more responsive polling
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
        timings.ntp_ms = millis() - timerStart;
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
    
    // Check for dormant mode (all hours disabled)
    if (allHoursDisabled && wakeReason == WAKEUP_TIMER) {
        LogBox::line("Dormant mode: All 24 hours disabled");
        LogBox::line("Sleeping indefinitely (button wake only)");
        LogBox::end();
        
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        unsigned long loopTimeMs = millis() - loopStartTime;
        
        // Sleep indefinitely - only wake on button press
        powerManager->enterDeepSleepIndefinitely(loopTimeMs);
        return;
    }
    
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
        LogBox::messagef("Config Error", "Invalid interval for image %d, using default", currentIndex + 1);
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
        if (!checkAndHandleCRC32(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, wifiBSSID, timings)) {
            return;  // CRC32 matched and timer wake - already went to sleep (timing already captured)
        }
        // Timing already captured inside checkAndHandleCRC32
    } else if (config.isCarouselMode()) {
        LogBox::message("CRC32 Check", "CRC32 disabled in carousel mode");
    }
    
    // Download and display image (measure timing)
    if (config.debugMode) {
        uiStatus->showDownloading(currentImageUrl.c_str(), false);
    }
    
    timerStart = millis();
    bool success = imageManager->downloadAndDisplay(currentImageUrl.c_str());
    timings.image_ms = millis() - timerStart;
    
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
                               0, wifiBSSID, timings, "Carousel image displayed successfully", "info");
            
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
                    LogBox::messagef("Carousel Error", "First image failed, retry attempt %d of 2", *imageStateIndex);
                    
                    powerManager->disableWatchdog();
                    powerManager->prepareForSleep();
                    unsigned long loopTimeMs = millis() - loopStartTime;
                    powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds
                } else {
                    // Exhausted retries on first image - show error and move to next
                    LogBox::message("Carousel Error", "First image failed after retries, moving to next");
                    
                    *imageStateIndex = 1;  // Move to second image
                    uiError->showImageError(currentImageUrl.c_str(), imageManager->getLastError(), currentInterval);
                    
                    float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
                    String errorMessage = "First carousel image failed: " + String(imageManager->getLastError());
                    publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                                       0, wifiBSSID, timings, errorMessage.c_str(), "error");
                    
                    delay(3000);
                    powerManager->disableWatchdog();
                    powerManager->prepareForSleep();
                    unsigned long loopTimeMs = millis() - loopStartTime;
                    powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds to next image
                }
            } else {
                // Non-first image - skip to next immediately
                LogBox::messagef("Carousel Error", "Image %d failed, skipping to next", currentIndex + 1);
                
                *imageStateIndex = (currentIndex + 1) % config.imageCount;
                
                float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
                String errorMessage = "Carousel image " + String(currentIndex + 1) + " failed, skipped: " + String(imageManager->getLastError());
                publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                                   0, wifiBSSID, timings, errorMessage.c_str(), "warning");
                
                powerManager->disableWatchdog();
                powerManager->prepareForSleep();
                unsigned long loopTimeMs = millis() - loopStartTime;
                powerManager->enterDeepSleep((float)(20.0 / 60.0), loopTimeMs);  // 20 seconds to next image
            }
        }
    } else {
        // Single image mode - use existing handlers
        if (success) {
            handleImageSuccess(config, newCRC32, crc32WasChecked, crc32Matched, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, wifiBSSID, timings);
        } else {
            handleImageFailure(config, loopStartTime, now, deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, wifiBSSID, timings);
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
                                               float batteryVoltage, int batteryPercentage, int wifiRSSI,
                                               const String& wifiBSSID, LoopTimings& timings) {
    if (!config.useCRC32Check) {
        return true;  // Continue execution
    }
    
    // Measure CRC32 check timing
    uint32_t crcStart = millis();
    
    crc32WasChecked = true;
    // Use first image URL for CRC32 check (only called in single image mode)
    String imageUrl = (config.imageCount > 0) ? config.imageUrls[0] : "";
    bool shouldDownload = imageManager->checkCRC32Changed(imageUrl.c_str(), &newCRC32, &timings.crc_retry_count);
    crc32Matched = !shouldDownload;
    
    // Capture CRC timing (whether we continue or return early)
    timings.crc_ms = millis() - crcStart;
    
    // If CRC32 matched during timer wake, skip download and sleep
    if (wakeReason == WAKEUP_TIMER && !shouldDownload) {
        float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
        unsigned long loopTimeMs = millis() - loopStartTime;
        
        publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                           configManager->getLastCRC32(), wifiBSSID, timings, "Image unchanged (CRC32 match)", "info");
        
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
                                                uint32_t imageCRC32, const String& wifiBSSID, 
                                                const LoopTimings& timings, const char* message, const char* severity) {
    if (mqttManager->begin() && mqttManager->isConfigured()) {
        mqttManager->publishAllTelemetry(deviceId, deviceName, BOARD_NAME, wakeReason,
                                        batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds, imageCRC32, 
                                        message, severity, wifiBSSID,
                                        timings.wifiSeconds(), timings.ntpSeconds(), 
                                        timings.crcSeconds(), timings.imageSeconds(),
                                        timings.wifi_retry_count, timings.crc_retry_count, timings.image_retry_count);
    }
}

void NormalModeController::handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32,
                                              bool crc32WasChecked, bool crc32Matched,
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int batteryPercentage, int wifiRSSI,
                                              const String& wifiBSSID, const LoopTimings& timings) {
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
                       configManager->getLastCRC32(), wifiBSSID, timings, logMessage, "info");
    
    enterSleep(config, currentTime, loopStartTime);
}

void NormalModeController::handleImageFailure(const DashboardConfig& config,
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int batteryPercentage, int wifiRSSI,
                                              const String& wifiBSSID, const LoopTimings& timings) {
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
                           configManager->getLastCRC32(), wifiBSSID, timings, errorMessage.c_str(), "error");
        
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

