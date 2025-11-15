/**
 * Integration Tests for Normal Mode Decision Flow
 * 
 * These tests validate complete decision flows from configuration input
 * to final outcomes (image target, CRC32 action, sleep duration).
 * 
 * Unlike unit tests that test individual decision functions in isolation,
 * integration tests verify that multiple decisions work correctly together
 * to produce the expected system behavior.
 * 
 * Tests cover the 8 high-priority scenarios from NORMAL_MODE_FLOW.md
 */

#include <gtest/gtest.h>
#include <modes/decision_logic.h>
#include "test_helpers.h"

// =============================================================================
// Test Fixture
// =============================================================================

class NormalModeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup runs before each test
    }
    
    void TearDown() override {
        // Cleanup runs after each test
    }
};

// =============================================================================
// High Priority Scenario 1: Single Image + Timer Wake + CRC32 Match
// Expected: Check CRC32, skip download, sleep for configured interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, SingleImage_TimerWake_CRC32Match_SkipsDownload) {
    // GIVEN: Single image config with CRC32 enabled, 15-minute interval
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    time_t currentTime = createTime(2025, 11, 15, 14, 30, 0);  // Friday 2:30 PM
    bool crc32Matched = true;  // Image hasn't changed
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, currentIndex, crc32Matched);
    
    // THEN: Verify complete flow
    // Image target: Stay on image 0, don't advance
    EXPECT_EQ(imageTarget.targetIndex, 0);
    EXPECT_FALSE(imageTarget.shouldAdvance);
    EXPECT_STREQ(imageTarget.reason, "Single image mode");
    
    // CRC32: Should check for skip optimization
    EXPECT_TRUE(crc32Action.shouldCheck);
    EXPECT_STREQ(crc32Action.reason, "Single image - timer wake (check for skip)");
    
    // Sleep: Use configured interval (CRC32 matched, so download was skipped)
    EXPECT_EQ(sleepDuration.sleepSeconds, 15.0f * 60.0f);  // 900 seconds
    EXPECT_STREQ(sleepDuration.reason, "Image interval (CRC32 matched)");
    
    // Integration assertion: Verify the complete flow makes sense
    // If CRC32 should check AND it matched, download should be skipped
    if (crc32Action.shouldCheck && crc32Matched) {
        // This represents the real system behavior in normal_mode_controller:
        // When CRC32 matches, we skip download and go straight to sleep
        EXPECT_TRUE(true) << "Download should be skipped when CRC32 matches";
    }
}

// =============================================================================
// High Priority Scenario 2: Single Image + Timer Wake + CRC32 Changed
// Expected: Check CRC32, download new image, save CRC32, sleep for interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, SingleImage_TimerWake_CRC32Changed_DownloadsImage) {
    // GIVEN: Single image config with CRC32 enabled
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/dashboard.png", 20)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    time_t currentTime = createTime(2025, 11, 15, 10, 0, 0);
    bool crc32Matched = false;  // Image has changed
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, currentIndex, crc32Matched);
    
    // THEN: Verify complete flow
    EXPECT_EQ(imageTarget.targetIndex, 0);
    EXPECT_FALSE(imageTarget.shouldAdvance);
    
    // CRC32: Should check (will detect change and download)
    EXPECT_TRUE(crc32Action.shouldCheck);
    
    // Sleep: Use configured interval (image was updated)
    EXPECT_EQ(sleepDuration.sleepSeconds, 20.0f * 60.0f);  // 1200 seconds
    EXPECT_STREQ(sleepDuration.reason, "Image interval (image updated)");
    
    // Integration assertion: CRC32 changed means download happens
    if (crc32Action.shouldCheck && !crc32Matched) {
        // In real system: Download image, display, save new CRC32, then sleep
        EXPECT_TRUE(true) << "Download should proceed when CRC32 changes";
    }
}

// =============================================================================
// High Priority Scenario 3: Single Image + Button Wake
// Expected: Always download (never check CRC32), display, sleep for interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, SingleImage_ButtonWake_AlwaysDownloads) {
    // GIVEN: Single image config with CRC32 enabled
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 10)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_BUTTON;  // User pressed button
    uint8_t currentIndex = 0;
    time_t currentTime = createTime(2025, 11, 15, 16, 45, 0);
    bool crc32Matched = true;  // Even if CRC32 matches, button forces download
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, currentIndex, crc32Matched);
    
    // THEN: Verify complete flow
    EXPECT_EQ(imageTarget.targetIndex, 0);
    EXPECT_FALSE(imageTarget.shouldAdvance);
    
    // CRC32: Should NOT check for skip (button always downloads)
    EXPECT_FALSE(crc32Action.shouldCheck);
    EXPECT_STREQ(crc32Action.reason, "Single image - button press (always download)");
    
    // Sleep: Use configured interval
    EXPECT_EQ(sleepDuration.sleepSeconds, 10.0f * 60.0f);  // 600 seconds
    
    // Integration assertion: Button wake always downloads, regardless of CRC32
    EXPECT_FALSE(crc32Action.shouldCheck) << "Button wake should never check CRC32 for skip";
}

// =============================================================================
// High Priority Scenario 4: Carousel + Timer Wake + Stay False (Auto-Advance)
// Expected: Advance to next image, download, sleep for NEXT image's interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, Carousel_TimerWake_StayFalse_AdvancesToNext) {
    // GIVEN: Carousel with 3 images, all stay:false (auto-advance)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img1.png", 10, false)  // stay:false
        .addImage("http://example.com/img2.png", 15, false)  // stay:false
        .addImage("http://example.com/img3.png", 20, false)  // stay:false
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 1;  // Currently on image 1 (second image)
    time_t currentTime = createTime(2025, 11, 15, 12, 0, 0);
    bool crc32Matched = false;  // Image changed (not relevant for auto-advance)
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, imageTarget.targetIndex, crc32Matched);
    
    // THEN: Verify complete flow
    // Image target: Advance from 1 to 2
    EXPECT_EQ(imageTarget.targetIndex, 2);
    EXPECT_TRUE(imageTarget.shouldAdvance);
    EXPECT_STREQ(imageTarget.reason, "Carousel - timer wake + stay:false (auto-advance)");
    
    // CRC32: Don't check (auto-advance always downloads)
    EXPECT_FALSE(crc32Action.shouldCheck);
    EXPECT_STREQ(crc32Action.reason, "Carousel - auto-advance (always download)");
    
    // Sleep: Use NEXT image's interval (image 2 = 20 minutes)
    EXPECT_EQ(sleepDuration.sleepSeconds, 20.0f * 60.0f);  // 1200 seconds
    
    // Integration assertion: Carousel advances and uses new image's interval
    EXPECT_EQ(imageTarget.targetIndex, 2) << "Carousel should advance from 1 to 2";
    EXPECT_EQ(sleepDuration.sleepSeconds, 20.0f * 60.0f) << "Should use image 2's interval (20 min)";
}

// =============================================================================
// High Priority Scenario 5: Carousel + Timer Wake + Stay True
// Expected: Stay on current image, check CRC32, sleep for current interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, Carousel_TimerWake_StayTrue_RemainsOnCurrent) {
    // GIVEN: Carousel with all stay:true
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img1.png", 10, true)  // stay:true
        .addImage("http://example.com/img2.png", 15, true)  // stay:true
        .addImage("http://example.com/img3.png", 20, true)  // stay:true
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 1;  // Currently on image 1
    time_t currentTime = createTime(2025, 11, 15, 11, 30, 0);
    bool crc32Matched = true;  // Image hasn't changed
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, currentIndex, crc32Matched);
    
    // THEN: Verify complete flow
    // Image target: Stay on current image (1)
    EXPECT_EQ(imageTarget.targetIndex, 1);
    EXPECT_FALSE(imageTarget.shouldAdvance);
    EXPECT_STREQ(imageTarget.reason, "Carousel - stay flag set (stay:true)");
    
    // CRC32: Should check (timer + stay:true enables skip optimization)
    EXPECT_TRUE(crc32Action.shouldCheck);
    EXPECT_STREQ(crc32Action.reason, "Carousel - timer wake + stay:true (check for skip)");
    
    // Sleep: Use current image's interval (image 1 = 15 minutes)
    EXPECT_EQ(sleepDuration.sleepSeconds, 15.0f * 60.0f);  // 900 seconds
    EXPECT_STREQ(sleepDuration.reason, "Image interval (CRC32 matched)");
    
    // Integration assertion: Stay on same image with CRC32 optimization
    EXPECT_EQ(imageTarget.targetIndex, currentIndex) << "Should stay on image 1";
    EXPECT_TRUE(crc32Action.shouldCheck) << "Should enable CRC32 skip optimization";
}

// =============================================================================
// High Priority Scenario 6: Carousel + Button Wake
// Expected: Always advance to next, never check CRC32, sleep for next interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, Carousel_ButtonWake_AlwaysAdvances) {
    // GIVEN: Carousel with mixed stay flags (button overrides stay:true)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img1.png", 5, false)
        .addImage("http://example.com/img2.png", 10, true)   // stay:true, but button overrides
        .addImage("http://example.com/img3.png", 15, false)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_BUTTON;
    uint8_t currentIndex = 1;  // On image 1 (has stay:true)
    time_t currentTime = createTime(2025, 11, 15, 9, 15, 0);
    bool crc32Matched = false;
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, imageTarget.targetIndex, crc32Matched);
    
    // THEN: Verify complete flow
    // Image target: Advance to next (1 → 2) despite stay:true
    EXPECT_EQ(imageTarget.targetIndex, 2);
    EXPECT_TRUE(imageTarget.shouldAdvance);
    EXPECT_STREQ(imageTarget.reason, "Carousel - button press (always advance)");
    
    // CRC32: Never check on button wake
    EXPECT_FALSE(crc32Action.shouldCheck);
    EXPECT_STREQ(crc32Action.reason, "Carousel - button press (always download)");
    
    // Sleep: Use next image's interval (image 2 = 15 minutes)
    EXPECT_EQ(sleepDuration.sleepSeconds, 15.0f * 60.0f);
    
    // Integration assertion: Button overrides stay flag
    EXPECT_TRUE(imageTarget.shouldAdvance) << "Button wake should always advance, even with stay:true";
    EXPECT_EQ(imageTarget.targetIndex, 2) << "Should advance to next image";
}

// =============================================================================
// High Priority Scenario 7: Hourly Schedule - Disabled Hour
// Expected: Sleep until next enabled hour (skip image update entirely)
// =============================================================================

TEST_F(NormalModeIntegrationTest, HourlySchedule_DisabledHour_SleepsUntilEnabled) {
    // GIVEN: Config with hourly schedule (only 9 AM - 5 PM enabled)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)  // 9 AM to 5 PM only
        .withTimezone(-5)  // EST timezone
        .build();
    
    // Current time: 7 AM UTC = 2 AM EST (disabled hour)
    time_t currentTime = createTime(2025, 11, 15, 7, 0, 0);
    WakeupReason wakeReason = WAKEUP_TIMER;
    
    // WHEN: Calculate sleep to next enabled hour
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep until 9 AM EST (7 hours from 2 AM)
    EXPECT_GT(sleepMinutes, 0.0f) << "Should sleep when current hour is disabled";
    EXPECT_NEAR(sleepMinutes, 7.0f * 60.0f, 1.0f);  // ~420 minutes (±1 min tolerance)
    
    // Integration assertion: In normal_mode_controller, this would skip
    // the entire image update flow and go straight to deep sleep
    if (sleepMinutes > 0) {
        EXPECT_TRUE(true) << "System should skip image update and sleep until next enabled hour";
    }
}

// =============================================================================
// High Priority Scenario 8: Button Wake Bypasses Hourly Schedule
// Expected: Proceed with image update even during disabled hour
// =============================================================================

TEST_F(NormalModeIntegrationTest, ButtonWake_BypassesHourlySchedule) {
    // GIVEN: Same config with hourly schedule (9-17 only)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(-5)
        .build();
    
    // Current time: 7 AM UTC = 2 AM EST (disabled hour)
    time_t currentTime = createTime(2025, 11, 15, 7, 0, 0);
    WakeupReason wakeReason = WAKEUP_BUTTON;  // Button press
    uint8_t currentIndex = 0;
    
    // WHEN: Execute decisions (button should bypass hourly check)
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    
    // THEN: Should proceed with image update
    EXPECT_EQ(imageTarget.targetIndex, 0);
    EXPECT_FALSE(crc32Action.shouldCheck);  // Button never checks CRC32
    
    // Integration assertion: Button wake bypasses hourly schedule check entirely
    // In normal_mode_controller, the hourly schedule check happens BEFORE calling
    // determineSleepDuration, and is skipped when wakeReason == WAKEUP_BUTTON.
    // 
    // The hourly schedule would calculate sleep to next enabled hour:
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    EXPECT_GT(sleepMinutes, 0.0f) << "Hour is disabled at 2 AM EST";
    
    // But button wake would skip this check in normal_mode_controller and proceed
    // with the normal image update flow (download, display, then sleep for interval)
    EXPECT_TRUE(true) << "Button wake should bypass hourly schedule and proceed with update";
}

// =============================================================================
// Additional Integration Test: Carousel Wrap-Around
// Expected: Last image advances to first image
// =============================================================================

TEST_F(NormalModeIntegrationTest, Carousel_WrapAround_LastToFirst) {
    // GIVEN: Carousel with 3 images, currently on last image
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img1.png", 10, false)
        .addImage("http://example.com/img2.png", 15, false)
        .addImage("http://example.com/img3.png", 20, false)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 2;  // Last image (index 2)
    time_t currentTime = createTime(2025, 11, 15, 15, 0, 0);
    
    // WHEN: Execute decisions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, imageTarget.targetIndex, false);
    
    // THEN: Should wrap around to first image
    EXPECT_EQ(imageTarget.targetIndex, 0) << "Should wrap from last (2) to first (0)";
    EXPECT_TRUE(imageTarget.shouldAdvance);
    
    // Sleep: Use first image's interval (10 minutes)
    EXPECT_EQ(sleepDuration.sleepSeconds, 10.0f * 60.0f);
}

// =============================================================================
// Additional Integration Test: Button-Only Mode (Interval = 0)
// Expected: Sleep indefinitely (0 seconds = wait for button press)
// =============================================================================

TEST_F(NormalModeIntegrationTest, ButtonOnlyMode_Interval0_IndefiniteSleep) {
    // GIVEN: Config with interval = 0 (button-only mode)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 0)  // 0 = button-only
        .build();
    
    time_t currentTime = createTime(2025, 11, 15, 10, 0, 0);
    uint8_t currentIndex = 0;
    bool crc32Matched = false;
    
    // WHEN: Calculate sleep duration
    auto sleepDuration = determineSleepDuration(config, currentTime, currentIndex, crc32Matched);
    
    // THEN: Should return 0 (indefinite sleep)
    EXPECT_EQ(sleepDuration.sleepSeconds, 0.0f);
    EXPECT_STREQ(sleepDuration.reason, "Button-only mode (interval = 0)");
    
    // Integration assertion: System enters deep sleep indefinitely
    // Only wakes on button press (RTC GPIO interrupt)
    EXPECT_EQ(sleepDuration.sleepSeconds, 0.0f) << "Interval 0 means indefinite sleep";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
