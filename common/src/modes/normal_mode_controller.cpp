#include <src/modes/normal_mode_controller.h>
#include <WiFi.h>
#include <src/frontlight_manager.h>

// Error retry interval: how long to wait before retrying after image download failure
// This prevents indefinite sleep when configured interval is 0 (button-only mode)
const float ERROR_RETRY_INTERVAL_MINUTES = 1.0;

NormalModeController::NormalModeController(Inkplate* disp, ConfigManager* config, WiFiManager* wifi,
                                           ImageManager* image, PowerManager* power, MQTTManager* mqtt,
                                           UIStatus* uiStatus, UIError* uiError, uint8_t* stateIndex)
    : display(disp), configManager(config), wifiManager(wifi),
      imageManager(image), powerManager(power), mqttManager(mqtt),
      uiStatus(uiStatus), uiError(uiError), imageStateIndex(stateIndex) {
}

ImageTargetDecision NormalModeController::determineImageTarget(const DashboardConfig& config, 
                                                                WakeupReason wakeReason, 
                                                                uint8_t currentIndex) {
    ImageTargetDecision decision;
    
    // Single image mode: always display image 0, never advance
    if (!config.isCarouselMode()) {
        decision.targetIndex = 0;
        decision.shouldAdvance = false;
        decision.reason = "Single image mode";
        return decision;
    }
    
    // Carousel mode: decision based on wake reason and stay flag
    bool currentStay = config.imageStay[currentIndex];
    
    if (wakeReason == WAKEUP_BUTTON) {
        // Button press: advance to next image
        uint8_t nextIndex = (currentIndex + 1) % config.imageCount;
        decision.targetIndex = nextIndex;
        decision.shouldAdvance = true;
        decision.reason = "Carousel - button press (always advance)";
        return decision;
    }
    
    // Timer wake: check stay flag
    if (!currentStay) {
        // stay=false: auto-advance to next image
        uint8_t nextIndex = (currentIndex + 1) % config.imageCount;
        decision.targetIndex = nextIndex;
        decision.shouldAdvance = true;
        decision.reason = "Carousel - auto-advance (stay:false)";
        return decision;
    }
    
    // stay=true: display current image, don't advance
    decision.targetIndex = currentIndex;
    decision.shouldAdvance = false;
    decision.reason = "Carousel - stay flag set (stay:true)";
    return decision;
}

CRC32Decision NormalModeController::determineCRC32Action(const DashboardConfig& config, 
                                                         WakeupReason wakeReason, 
                                                         uint8_t currentIndex) {
    CRC32Decision decision;
    decision.shouldCheck = false;
    decision.reason = "CRC32 disabled in config";
    
    if (!config.useCRC32Check) {
        return decision;
    }
    
    // CRC32 enabled - decide if we should check for skip optimization
    // (we always fetch the CRC32 value when enabled, for saving)
    
    // Single image mode: check on timer wake only
    if (!config.isCarouselMode()) {
        if (wakeReason == WAKEUP_TIMER) {
            decision.shouldCheck = true;
            decision.reason = "Single image - timer wake (check for skip)";
        } else {
            decision.shouldCheck = false;
            decision.reason = "Single image - button press (always download)";
        }
        return decision;
    }
    
    // Carousel mode: only check on timer wake + stay:true
    bool currentStay = config.imageStay[currentIndex];
    
    if (wakeReason == WAKEUP_BUTTON) {
        decision.shouldCheck = false;
        decision.reason = "Carousel - button press (always download)";
        return decision;
    }
    
    if (!currentStay) {
        decision.shouldCheck = false;
        decision.reason = "Carousel - auto-advance (always download)";
        return decision;
    }
    
    // Timer wake + stay:true
    decision.shouldCheck = true;
    decision.reason = "Carousel - timer wake + stay:true (check for skip)";
    return decision;
}

SleepDecision NormalModeController::determineSleepDuration(const DashboardConfig& config, 
                                                           time_t currentTime, 
                                                           uint8_t currentIndex, 
                                                           bool crc32Matched) {
    SleepDecision decision;
    
    // Calculate sleep considering hourly schedule
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(currentTime, config.timezoneOffset, config.updateHours);
    
    if (sleepMinutes > 0) {
        decision.sleepSeconds = sleepMinutes * 60.0f;
        decision.reason = "Hourly schedule - sleep until next enabled hour";
        return decision;
    }
    
    // No hourly schedule constraints - use image interval
    int currentInterval = config.imageIntervals[currentIndex];
    
    if (currentInterval == 0) {
        decision.sleepSeconds = 0.0f;
        decision.reason = "Button-only mode (interval = 0)";
        return decision;
    }
    
    decision.sleepSeconds = (float)currentInterval * 60.0f;
    decision.reason = crc32Matched ? "Image interval (CRC32 matched)" : "Image interval (image updated)";
    return decision;
}

void NormalModeController::execute() {
    /*
     * TRUTH TABLE: Normal Mode Execution Paths
     * ==========================================
     * 
     * HIGH-LEVEL FLOW:
     * 1. Load config → 2. WiFi connect → 3. NTP sync → 4. Hourly check → 
     * 5. Image target → 6. CRC32 check → 7. Download → 8. Handle result → 9. Sleep
     * 
     * KEY DECISION POINTS:
     * - Mode: Single or Carousel
     * - Wake: Timer, Button, Other
     * - Stay: stay, advance [carousel only]
     * - CRC32: crc32on, crc32off
     * - CRC32 Result: crc32match, crc32changed [when enabled]
     * - Download: downloadok, downloadfail
     * - Retry: retry0, retry1, retry2 [on failure]
     * 
     * EXECUTION PATHS (40+ combinations):
     * 
     * Single Image Mode:
     *   Single + Timer + crc32off + downloadok 
     *     ==> Display image, sleep interval
     *   Single + Timer + crc32off + downloadfail 
     *     ==> Retry (retry0→retry1→retry2), then error screen
     *   Single + Timer + crc32on + crc32match 
     *     ==> Skip download, sleep interval
     *   Single + Timer + crc32on + crc32changed + downloadok 
     *     ==> Display image, save CRC32, sleep interval
     *   Single + Timer + crc32on + crc32changed + downloadfail 
     *     ==> Retry (retry0→retry1→retry2), then error screen
     *   Single + Button + (any) + downloadok 
     *     ==> Display image, sleep interval (always fetch CRC32)
     *   Single + Button + (any) + downloadfail 
     *     ==> Retry (retry0→retry1→retry2), then error screen
     * 
     * Carousel Mode:
     *   Carousel + Timer + stay + crc32off + downloadok 
     *     ==> Display current, sleep interval (stay on current)
     *   Carousel + Timer + stay + crc32off + downloadfail 
     *     ==> Retry if idx=0, else skip to next
     *   Carousel + Timer + stay + crc32on + crc32match 
     *     ==> Skip download, sleep interval (stay on current)
     *   Carousel + Timer + stay + crc32on + crc32changed + downloadok 
     *     ==> Display current, save CRC32, sleep (stay on current)
     *   Carousel + Timer + stay + crc32on + crc32changed + downloadfail 
     *     ==> Retry if idx=0, else skip to next
     *   Carousel + Timer + advance + (any) + downloadok 
     *     ==> Advance to next, display, sleep interval
     *   Carousel + Timer + advance + (any) + downloadfail 
     *     ==> Retry if idx=0, else skip to next
     *   Carousel + Button + (any) + downloadok 
     *     ==> Advance to next, display, sleep interval
     *   Carousel + Button + (any) + downloadfail 
     *     ==> Retry if idx=0, else skip to next
     * 
     * HOURLY SCHEDULE:
     * - If current hour disabled + timer wake → Skip all, sleep until next enabled hour
     * - Button wake always bypasses hourly schedule
     * 
     * ERROR RETRY LOGIC:
     * - Single image: 3 attempts (retry0→retry1→retry2), 20s between retries
     * - Carousel first image (idx=0): same as single image
     * - Carousel other images: skip to next immediately (20s sleep)
     * 
     * SLEEP CALCULATION:
     * - Hourly schedule active → sleep until next enabled hour
     * - Interval = 0 → button-only mode (indefinite sleep)
     * - Otherwise → sleep for image interval
     */
    
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
    if (config.imageCount == 1) {
        timings.image_retry_count = *imageStateIndex;
    } else {
        timings.image_retry_count = 0;  // Carousel mode - retry count not applicable
    }
    
    if (config.debugMode) {
        int debugInterval = config.getAverageInterval();
        uiStatus->showDebugStatus(config.wifiSSID.c_str(), debugInterval);
    }
    
    // Collect telemetry data early
    String deviceId = wifiManager->getDeviceIdentifier();
    String deviceName;
    if (config.friendlyName.length() > 0) {
        deviceName = config.friendlyName;
    } else {
        String deviceNameSuffix = deviceId;
        if (deviceNameSuffix.startsWith("inkplate-")) {
            deviceNameSuffix = deviceNameSuffix.substring(9);
        }
        deviceName = "Inkplate Dashboard " + deviceNameSuffix;
    }
    float batteryVoltage = powerManager->readBatteryVoltage(display);
    int batteryPercentage = PowerManager::calculateBatteryPercentage(batteryVoltage);
    WakeupReason wakeReason = powerManager->getWakeupReason();
    
    // Connect to WiFi (measure timing)
    timerStart = millis();
    if (!wifiManager->connectToWiFi(&timings.wifi_retry_count)) {
        handleWiFiFailure(config, loopStartTime);
        return;
    }
    timings.wifi_ms = millis() - timerStart;
    
    int wifiRSSI = WiFi.RSSI();
    String wifiBSSID = WiFi.BSSIDstr();
    
    // NTP sync (skip if all hours enabled for optimization)
    bool allHoursEnabled = ConfigManager::areAllHoursEnabled(config.updateHours);
    time_t now;
    
    if (allHoursEnabled) {
        LogBox::begin("NTP Time Sync");
        LogBox::line("Skipped - all 24 hours enabled");
        LogBox::end();
        now = time(nullptr);
        timings.ntp_ms = 0;
    } else {
        timerStart = millis();
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        
        LogBox::begin("NTP Time Sync");
        now = time(nullptr);
        int ntpRetries = 0;
        while (now < 24 * 3600 && ntpRetries < 70) {
            delay(100);
            now = time(nullptr);
            ntpRetries++;
        }
        
        if (now < 24 * 3600) {
            LogBox::line("WARNING: NTP sync timeout, using last known time");
        } else {
            LogBox::line("Time synced via NTP");
        }
        LogBox::end();
        timings.ntp_ms = millis() - timerStart;
    }
    
    // Hourly schedule check (only enforce on timer wake)
    struct tm* timeinfo = localtime(&now);
    int currentHour = ConfigManager::applyTimezoneOffset(timeinfo->tm_hour, config.timezoneOffset);
    
    LogBox::begin("Hourly Schedule Check");
    LogBox::linef("Current time (UTC): %04d-%02d-%02d %02d:%02d:%02d",
                  timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    LogBox::linef("Timezone offset: %+d, Local hour: %d", config.timezoneOffset, currentHour);
    
    if (wakeReason == WAKEUP_TIMER && !allHoursEnabled) {
        bool hourEnabled = ConfigManager::isHourEnabledInBitmask(currentHour, config.updateHours);
        
        if (!hourEnabled) {
            LogBox::line("Updates disabled for this hour");
            float sleepMinutes = calculateSleepMinutesToNextEnabledHour(now, config.timezoneOffset, config.updateHours);
            LogBox::linef("Sleeping %.1f minutes until next enabled hour", sleepMinutes);
            LogBox::end();
            
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            unsigned long loopTimeMs = millis() - loopStartTime;
            
            if (sleepMinutes > 0) {
                powerManager->enterDeepSleep(sleepMinutes * 60.0f, loopTimeMs / 1000.0f);
            } else {
                float avgInterval = (float)config.getAverageInterval();
                float fallbackInterval = (avgInterval > 0) ? avgInterval : 5.0;
                powerManager->enterDeepSleep(fallbackInterval * 60.0f, loopTimeMs / 1000.0f);
            }
            return;
        }
        LogBox::line("Updates enabled for this hour");
    } else {
        LogBox::line("Skipping hourly schedule enforcement (manual trigger or non-timer wakeup)");
    }
    LogBox::end();
    
    // DECISION POINT 1: Determine which image to display and whether to advance
    uint8_t currentIndex = *imageStateIndex % config.imageCount;
    ImageTargetDecision targetDecision = determineImageTarget(config, wakeReason, currentIndex);
    
    LogBox::begin(config.isCarouselMode() ? "Carousel Mode" : "Single Image Mode");
    LogBox::linef("Decision: %s", targetDecision.reason);
    if (config.isCarouselMode()) {
        LogBox::linef("Target image: %d of %d", targetDecision.targetIndex + 1, config.imageCount);
    }
    
    // Update index if we should advance before display
    if (targetDecision.shouldAdvance) {
        *imageStateIndex = targetDecision.targetIndex;
        currentIndex = targetDecision.targetIndex;
    }
    
    String currentImageUrl = config.imageUrls[currentIndex];
    int currentInterval = config.imageIntervals[currentIndex];
    
    if (currentInterval < 0) {
        LogBox::messagef("Config Error", "Invalid interval for image %d, using default", currentIndex + 1);
        currentInterval = DEFAULT_INTERVAL_MINUTES;
    }
    
    LogBox::line("URL: " + currentImageUrl);
    if (currentInterval == 0) {
        LogBox::line("Button-only wake mode (interval = 0)");
    } else {
        LogBox::linef("Interval: %d minutes", currentInterval);
    }
    LogBox::end();
    
    // DECISION POINT 2: Determine CRC32 check behavior
    CRC32Decision crc32Decision = determineCRC32Action(config, wakeReason, currentIndex);
    
    LogBox::begin("CRC32 Check Decision");
    LogBox::linef("Decision: %s", crc32Decision.reason);
    LogBox::end();
    
    uint32_t newCRC32 = 0;
    bool crc32Matched = false;
    
    if (config.useCRC32Check) {
        // Always fetch CRC32 when enabled (for saving to storage)
        timerStart = millis();
        bool shouldDownload = imageManager->checkCRC32Changed(currentImageUrl.c_str(), &newCRC32, &timings.crc_retry_count);
        timings.crc_ms = millis() - timerStart;
        crc32Matched = !shouldDownload;
        
        // Only skip download if we're checking AND it matched
        if (crc32Decision.shouldCheck && wakeReason == WAKEUP_TIMER && crc32Matched) {
            // CRC32 matched on timer wake - skip download and sleep
            float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
            unsigned long loopTimeMs = millis() - loopStartTime;
            
            publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                               configManager->getLastCRC32(), wifiBSSID, timings, "Image unchanged (CRC32 match)", "info");
            
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            
            SleepDecision sleepDecision = determineSleepDuration(config, now, currentIndex, true);
            powerManager->enterDeepSleep(sleepDecision.sleepSeconds, loopTimeMs / 1000.0f);
            return;
        }
        // Otherwise continue to download (CRC32 will be saved after successful download)
    }
    
    // Download and display image
    if (config.debugMode) {
        uiStatus->showDownloading(currentImageUrl.c_str(), false);
    }
    
    timerStart = millis();
    bool success = imageManager->downloadAndDisplay(currentImageUrl.c_str());
    timings.image_ms = millis() - timerStart;
    
    // DECISION POINT 3: Handle result (success or failure)
    if (success) {
        handleImageSuccess(config, newCRC32, crc32Decision.shouldCheck, crc32Matched, loopStartTime, now, 
                          deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, wifiBSSID, timings);
    } else {
        handleImageFailure(config, loopStartTime, now, deviceId, deviceName, wakeReason, 
                          batteryVoltage, batteryPercentage, wifiRSSI, wifiBSSID, timings);
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
    powerManager->enterDeepSleep(300.0f);  // 5 minutes = 300 seconds, no loop timing for early failure
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
    // Save CRC32 if it changed
    if (config.useCRC32Check && crc32WasChecked && !crc32Matched && newCRC32 != 0) {
        imageManager->saveCRC32(newCRC32);
    }
    
    // Handle carousel vs single image mode
    if (config.isCarouselMode()) {
        // Carousel mode: index already updated before display in execute()
        // Just sleep with current image's interval
        uint8_t currentIndex = *imageStateIndex % config.imageCount;
        
        // Enable frontlight after successful image display (only for button wake)
        #if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true
        if (wakeReason == WAKEUP_BUTTON && config.frontlightDuration > 0) {
            extern FrontlightManager frontlightManager;
            unsigned long durationMs = config.frontlightDuration * 1000UL;
            frontlightManager.turnOn(config.frontlightBrightness, durationMs);
        }
        #endif
        
        float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
        publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                           configManager->getLastCRC32(), wifiBSSID, timings, "Carousel image displayed successfully", "info");
        
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        unsigned long loopTimeMs = millis() - loopStartTime;
        
        SleepDecision sleepDecision = determineSleepDuration(config, currentTime, currentIndex, crc32Matched);
        powerManager->enterDeepSleep(sleepDecision.sleepSeconds, loopTimeMs / 1000.0f);
    } else {
        // Single image mode: reset retry counter
        *imageStateIndex = 0;
        
        // Enable frontlight after successful image display (only for button wake)
        #if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true
        if (wakeReason == WAKEUP_BUTTON && config.frontlightDuration > 0) {
            extern FrontlightManager frontlightManager;
            unsigned long durationMs = config.frontlightDuration * 1000UL;
            frontlightManager.turnOn(config.frontlightBrightness, durationMs);
        }
        #endif
        
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
        
        powerManager->disableWatchdog();
        powerManager->prepareForSleep();
        unsigned long loopTimeMs = millis() - loopStartTime;
        
        SleepDecision sleepDecision = determineSleepDuration(config, currentTime, 0, crc32Matched);
        powerManager->enterDeepSleep(sleepDecision.sleepSeconds, loopTimeMs / 1000.0f);
    }
}

void NormalModeController::handleImageFailure(const DashboardConfig& config,
                                              unsigned long loopStartTime, time_t currentTime, const String& deviceId,
                                              const String& deviceName, WakeupReason wakeReason,
                                              float batteryVoltage, int batteryPercentage, int wifiRSSI,
                                              const String& wifiBSSID, const LoopTimings& timings) {
    uint8_t currentIndex = *imageStateIndex % config.imageCount;
    
    // Carousel mode: check if first image (special retry logic) or other image (skip to next)
    if (config.isCarouselMode()) {
        if (currentIndex == 0) {
            // First image - use retry logic (same as single image mode)
            if (*imageStateIndex < 2) {
                (*imageStateIndex)++;
                LogBox::messagef("Carousel Error", "First image failed, retry attempt %d of 2", *imageStateIndex);
                
                // Clear stored CRC32 to force download on next retry
                configManager->setLastCRC32(0);
                
                powerManager->disableWatchdog();
                powerManager->prepareForSleep();
                unsigned long loopTimeMs = millis() - loopStartTime;
                powerManager->enterDeepSleep(20.0f, loopTimeMs / 1000.0f);
            } else {
                // Exhausted retries on first image - show error and move to next
                LogBox::message("Carousel Error", "First image failed after retries, moving to next");
                
                *imageStateIndex = 1;  // Move to second image
                String currentImageUrl = config.imageUrls[currentIndex];
                uiError->showImageError(currentImageUrl.c_str(), imageManager->getLastError());
                
                float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
                String errorMessage = "First carousel image failed: " + String(imageManager->getLastError());
                publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                                   configManager->getLastCRC32(), wifiBSSID, timings, errorMessage.c_str(), "error");
                
                // Clear stored CRC32
                configManager->setLastCRC32(0);
                
                delay(3000);
                powerManager->disableWatchdog();
                powerManager->prepareForSleep();
                unsigned long loopTimeMs = millis() - loopStartTime;
                powerManager->enterDeepSleep(20.0f, loopTimeMs / 1000.0f);
            }
        } else {
            // Non-first image - skip to next immediately
            LogBox::messagef("Carousel Error", "Image %d failed, skipping to next", currentIndex + 1);
            
            *imageStateIndex = (currentIndex + 1) % config.imageCount;
            
            float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
            String errorMessage = "Carousel image " + String(currentIndex + 1) + " failed, skipped: " + String(imageManager->getLastError());
            publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                               configManager->getLastCRC32(), wifiBSSID, timings, errorMessage.c_str(), "warning");
            
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            unsigned long loopTimeMs = millis() - loopStartTime;
            powerManager->enterDeepSleep(20.0f, loopTimeMs / 1000.0f);
        }
    } else {
        // Single image mode: retry logic
        if (*imageStateIndex < 2) {
            (*imageStateIndex)++;
            
            // Clear stored CRC32 to force download on next retry
            configManager->setLastCRC32(0);
            
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            unsigned long loopTimeMs = millis() - loopStartTime;
            powerManager->enterDeepSleep(20.0f, loopTimeMs / 1000.0f);
        } else {
            *imageStateIndex = 0;
            String firstUrl = (config.imageCount > 0) ? config.imageUrls[0] : "";
            uiError->showImageError(firstUrl.c_str(), imageManager->getLastError());
            
            float loopTimeSeconds = (millis() - loopStartTime) / 1000.0;
            String errorMessage = "Image download failed: " + String(imageManager->getLastError());
            publishMQTTTelemetry(deviceId, deviceName, wakeReason, batteryVoltage, batteryPercentage, wifiRSSI, loopTimeSeconds,
                               configManager->getLastCRC32(), wifiBSSID, timings, errorMessage.c_str(), "error");
            
            // Clear stored CRC32 to force download on next retry
            configManager->setLastCRC32(0);
            
            delay(3000);
            
            // For error retry, always use ERROR_RETRY_INTERVAL_MINUTES regardless of configured interval
            powerManager->disableWatchdog();
            powerManager->prepareForSleep();
            unsigned long loopTimeMs = millis() - loopStartTime;
            powerManager->enterDeepSleep(ERROR_RETRY_INTERVAL_MINUTES * 60.0f, loopTimeMs / 1000.0f);
        }
    }
}

void NormalModeController::handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime) {
    uiError->showWiFiError(config.wifiSSID.c_str(), wifiManager->getStatusString().c_str());
    delay(3000);
    powerManager->disableWatchdog();
    powerManager->prepareForSleep();
    unsigned long loopTimeMs = millis() - loopStartTime;
    powerManager->enterDeepSleep(ERROR_RETRY_INTERVAL_MINUTES * 60.0f, loopTimeMs / 1000.0f);  // Convert minutes to seconds, ms to s
}
