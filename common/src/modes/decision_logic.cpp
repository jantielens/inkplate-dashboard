#include <modes/decision_logic.h>
#include "config_logic.h"

ImageTargetDecision determineImageTarget(const DashboardConfig& config, 
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
        decision.reason = "Carousel - timer wake + stay:false (auto-advance)";
        return decision;
    }
    
    // stay=true: display current image, don't advance
    decision.targetIndex = currentIndex;
    decision.shouldAdvance = false;
    decision.reason = "Carousel - stay flag set (stay:true)";
    return decision;
}

CRC32Decision determineCRC32Action(const DashboardConfig& config, 
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

SleepDecision determineSleepDuration(const DashboardConfig& config, 
                                     time_t currentTime, 
                                     uint8_t currentIndex, 
                                     bool crc32Matched) {
    SleepDecision decision;
    
    // Calculate sleep considering hourly schedule
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(currentTime, config.timezoneOffset, config.updateHours);
    
    if (sleepMinutes > 0) {
        decision.sleepSeconds = sleepMinutes * 60.0f;
        decision.reason = "Sleep until next enabled hour";
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

float calculateSleepMinutesToNextEnabledHour(time_t currentTime, int timezoneOffset, const uint8_t updateHours[3]) {
    // Get current time info
    struct tm* timeinfo = localtime(&currentTime);
    
    // Calculate current local hour and minutes
    int currentHour = applyTimezoneOffset(timeinfo->tm_hour, timezoneOffset);
    int currentMinute = timeinfo->tm_min;
    int currentSecond = timeinfo->tm_sec;
    
    // First check if the current hour is enabled
    // If it is, we don't need to sleep - return invalid to signal "don't sleep"
    bool currentHourEnabled = isHourEnabledInBitmask(currentHour, updateHours);
    
    if (currentHourEnabled) {
        return -1;  // Current hour is enabled, no sleep needed
    }
    
    // Current hour is disabled - find next enabled hour
    for (int i = 1; i <= 24; i++) {
        int nextHour = (currentHour + i) % 24;
        bool nextHourEnabled = isHourEnabledInBitmask(nextHour, updateHours);
        
        if (nextHourEnabled) {
            // Calculate minutes until this hour starts
            // If we're at 14:37:42 and next hour is 16:00, we need:
            // - Minutes left in current hour: 60 - 37 - 1 = 22 (minus 1 because we're past :00)
            // - Full hours in between: (16 - 14 - 1) * 60 = 60 minutes
            // - Total: 22 + 60 = 82 minutes
            
            int hoursUntilNext = i - 1;  // -1 because we're counting from start of next hour
            int minutesLeftInCurrentHour = 60 - currentMinute - 1;  // -1 to round down
            int secondsAdjustment = (60 - currentSecond) > 30 ? 1 : 0;  // Round up if >30 seconds
            
            float sleepMinutes = (hoursUntilNext * 60) + minutesLeftInCurrentHour + secondsAdjustment;
            return sleepMinutes;
        }
    }
    
    // Fallback: all hours disabled (shouldn't happen with defaults)
    return -1;
}

/**
 * @brief Orchestrate all normal mode decisions in correct order
 * 
 * Ensures the three decision functions are called with correct parameters.
 * CRITICAL: CRC32 decision must use ORIGINAL index before carousel advance.
 */
NormalModeDecisions orchestrateNormalModeDecisions(const DashboardConfig& config,
                                                    WakeupReason wakeReason,
                                                    uint8_t currentIndex) {
    NormalModeDecisions result;
    
    // Preserve original index for CRC32 decision
    uint8_t originalIndex = currentIndex;
    
    // Step 1: Determine image target
    result.imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    
    // Step 2: Update index if advancing
    uint8_t finalIndex = currentIndex;
    if (result.imageTarget.shouldAdvance) {
        finalIndex = result.imageTarget.targetIndex;
    }
    result.finalIndex = finalIndex;
    
    // Step 3: Determine CRC32 action - MUST use originalIndex!
    result.crc32Action = determineCRC32Action(config, wakeReason, originalIndex);
    result.indexForCRC32 = originalIndex;
    
    return result;
}
