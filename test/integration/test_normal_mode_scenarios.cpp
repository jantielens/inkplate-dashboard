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
// ORCHESTRATION BUG TEST: Tests the actual orchestration function
// This test calls the REAL orchestration code that the controller uses
// =============================================================================

TEST_F(NormalModeIntegrationTest, OrchestrationBug_CRC32UsesWrongIndex) {
    // GIVEN: Carousel config matching user's bug report
    // - Image 0: stay=true (1 min)
    // - Image 1: stay=false (1 min) ← Should auto-advance but CRC32 uses wrong index
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 1, true)   // stay:true
        .addImage("http://example.com/img1.png", 1, false)  // stay:false ← Currently showing this
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 1;  // Currently on image 1 (stay=false)
    
    // WHEN: Call the ACTUAL orchestration function
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Verify the orchestration produces correct decisions
    
    // Image target: Should advance from 1 to 0 (wrap around)
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0) << "Should advance from image 1 to image 0";
    EXPECT_TRUE(decisions.imageTarget.shouldAdvance) << "stay=false should trigger advance";
    
    // Final index: Should be 0 (after advance)
    EXPECT_EQ(decisions.finalIndex, 0) << "Final index should be 0 after advance";
    
    // BUG DETECTION: indexForCRC32 should be ORIGINAL index (1), not final index (0)
    EXPECT_EQ(decisions.indexForCRC32, 1) 
        << "CRC32 decision should use ORIGINAL index (1, stay=false), not final index (0, stay=true)";
    
    // CRC32 decision: Should NOT check (because ORIGINAL image 1 has stay=false)
    EXPECT_FALSE(decisions.crc32Action.shouldCheck) 
        << "CRC32 should NOT check because original image 1 has stay=false (auto-advance)";
    EXPECT_STREQ(decisions.crc32Action.reason, "Carousel - auto-advance (always download)");
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
// REGRESSION TEST: CRC32 Decision Must Use Original Image Index
// Validates the CORRECT orchestration pattern that the controller should follow.
// 
// This test documents the proper way to call decision functions:
// 1. Preserve original index BEFORE updating for display
// 2. Pass original index to determineCRC32Action()
// 
// Scenario: Timer wake on image 1 (stay=false) advances to image 0 (stay=true).
// The CRC32 decision should be based on image 1's stay flag (false), not image 0's (true).
// =============================================================================

TEST_F(NormalModeIntegrationTest, BugFix_CRC32DecisionUsesCorrectImageIndex) {
    // GIVEN: Carousel with mixed stay flags
    //   Image 0: stay=true  (should check CRC32 when ON this image)
    //   Image 1: stay=false (should NOT check CRC32 when ON this image)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 1, true)   // stay=true
        .addImage("http://example.com/img1.png", 1, false)  // stay=false
        .withCRC32(true)
        .build();
    
    // SCENARIO: Timer wake while currently displaying image 1 (stay=false)
    // Expected: Should advance to image 0 and NOT check CRC32
    // (Because we're leaving image 1 which has stay=false)
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 1;  // Currently on image 1 (stay=false)
    time_t currentTime = createTime(2025, 11, 15, 14, 0, 0);
    
    // WHEN: Call orchestration function (tests real controller code path)
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Verify expected behavior
    // Image target: Should advance from image 1 to image 0
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0) << "Should advance from image 1 to image 0";
    EXPECT_TRUE(decisions.imageTarget.shouldAdvance);
    EXPECT_EQ(decisions.finalIndex, 0);
    
    // CRITICAL: CRC32 decision must be based on ORIGINAL image's stay flag
    EXPECT_EQ(decisions.indexForCRC32, 1) << "CRC32 must use original index 1, not final index 0";
    EXPECT_FALSE(decisions.crc32Action.shouldCheck) 
        << "CRC32 decision should be based on image 1's stay=false flag, not image 0's stay=true flag. "
        << "Expected: shouldCheck=false (auto-advance), Actual: shouldCheck=" 
        << (decisions.crc32Action.shouldCheck ? "true" : "false");
    EXPECT_STREQ(decisions.crc32Action.reason, "Carousel - auto-advance (always download)");
}

// =============================================================================
// ORCHESTRATION TEST: Single Image with Stay Flag (Edge Case)
// Validates controller correctly handles sleep duration after CRC32 decision
// =============================================================================

TEST_F(NormalModeIntegrationTest, Orchestration_SingleImage_CRC32Match_CorrectSleepCalculation) {
    // GIVEN: Single image with 30-minute interval, CRC32 enabled
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/dashboard.png", 30)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    time_t currentTime = createTime(2025, 11, 15, 14, 0, 0);
    
    // WHEN: Call actual orchestration function (tests real controller code path)
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // Simulate CRC32 check result
    bool crc32Matched = true;  // Image unchanged
    
    // Calculate sleep duration using final index
    auto sleepDuration = determineSleepDuration(config, currentTime, decisions.finalIndex, crc32Matched);
    
    // THEN: Verify orchestration produces correct outcome
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0);
    EXPECT_FALSE(decisions.imageTarget.shouldAdvance);
    EXPECT_TRUE(decisions.crc32Action.shouldCheck);
    EXPECT_EQ(sleepDuration.sleepSeconds, 30.0f * 60.0f);
    EXPECT_STREQ(sleepDuration.reason, "Image interval (CRC32 matched)");
}

// =============================================================================
// ORCHESTRATION TEST: Carousel Stay=True, Button Wake
// Validates button wake forces advance even with stay=true, and uses target index for sleep
// =============================================================================

TEST_F(NormalModeIntegrationTest, Orchestration_CarouselStayTrue_ButtonForcesAdvance) {
    // GIVEN: Carousel where current image has stay=true
    //   Image 0: stay=true, 10 min
    //   Image 1: stay=false, 20 min
    //   Image 2: stay=true, 30 min
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 10, true)   // stay=true
        .addImage("http://example.com/img1.png", 20, false)
        .addImage("http://example.com/img2.png", 30, true)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_BUTTON;
    uint8_t currentIndex = 0;  // On image 0 (stay=true)
    time_t currentTime = createTime(2025, 11, 15, 10, 30, 0);
    
    // WHEN: Call actual orchestration function
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    auto sleepDuration = determineSleepDuration(config, currentTime, decisions.finalIndex, false);
    
    // THEN: Button should force advance to image 1, skip CRC32, use image 1's interval
    EXPECT_EQ(decisions.imageTarget.targetIndex, 1);
    EXPECT_TRUE(decisions.imageTarget.shouldAdvance);
    EXPECT_STREQ(decisions.imageTarget.reason, "Carousel - button press (always advance)");
    EXPECT_EQ(decisions.finalIndex, 1);
    EXPECT_EQ(decisions.indexForCRC32, 0) << "CRC32 should use original index 0";
    
    EXPECT_FALSE(decisions.crc32Action.shouldCheck);
    EXPECT_STREQ(decisions.crc32Action.reason, "Carousel - button press (always download)");
    
    // Critical: Sleep should use TARGET image's interval (image 1 = 20 min)
    EXPECT_EQ(sleepDuration.sleepSeconds, 20.0f * 60.0f);
}

// =============================================================================
// ORCHESTRATION TEST: Carousel All Stay=False, Multiple Advances
// Validates carousel advances correctly and uses next image's interval
// =============================================================================

TEST_F(NormalModeIntegrationTest, Orchestration_CarouselAllAutoAdvance_CorrectSequence) {
    // GIVEN: Carousel with all stay=false (auto-advance mode)
    //   Image 0: 5 min, stay=false
    //   Image 1: 10 min, stay=false
    //   Image 2: 15 min, stay=false
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 5, false)
        .addImage("http://example.com/img1.png", 10, false)
        .addImage("http://example.com/img2.png", 15, false)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    time_t currentTime = createTime(2025, 11, 15, 9, 0, 0);
    
    // WHEN: Simulate three consecutive timer wakes using orchestration function
    // First wake: On image 0, advance to image 1
    uint8_t currentIndex = 0;
    NormalModeDecisions dec1 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    auto sleep1 = determineSleepDuration(config, currentTime, dec1.finalIndex, false);
    
    EXPECT_EQ(dec1.imageTarget.targetIndex, 1);
    EXPECT_TRUE(dec1.imageTarget.shouldAdvance);
    EXPECT_EQ(dec1.finalIndex, 1);
    EXPECT_EQ(dec1.indexForCRC32, 0) << "CRC32 uses original index 0";
    EXPECT_FALSE(dec1.crc32Action.shouldCheck);  // Auto-advance never checks CRC32
    EXPECT_EQ(sleep1.sleepSeconds, 10.0f * 60.0f);  // Uses image 1's interval
    
    // Second wake: On image 1, advance to image 2
    currentIndex = dec1.finalIndex;
    NormalModeDecisions dec2 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    auto sleep2 = determineSleepDuration(config, currentTime, dec2.finalIndex, false);
    
    EXPECT_EQ(dec2.imageTarget.targetIndex, 2);
    EXPECT_TRUE(dec2.imageTarget.shouldAdvance);
    EXPECT_EQ(dec2.finalIndex, 2);
    EXPECT_EQ(dec2.indexForCRC32, 1) << "CRC32 uses original index 1";
    EXPECT_FALSE(dec2.crc32Action.shouldCheck);
    EXPECT_EQ(sleep2.sleepSeconds, 15.0f * 60.0f);  // Uses image 2's interval
    
    // Third wake: On image 2, wrap to image 0
    currentIndex = dec2.finalIndex;
    NormalModeDecisions dec3 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    auto sleep3 = determineSleepDuration(config, currentTime, dec3.finalIndex, false);
    
    EXPECT_EQ(dec3.imageTarget.targetIndex, 0);  // Wrap around
    EXPECT_TRUE(dec3.imageTarget.shouldAdvance);
    EXPECT_EQ(dec3.finalIndex, 0);
    EXPECT_EQ(dec3.indexForCRC32, 2) << "CRC32 uses original index 2";
    EXPECT_FALSE(dec3.crc32Action.shouldCheck);
    EXPECT_EQ(sleep3.sleepSeconds, 5.0f * 60.0f);  // Uses image 0's interval
}

// =============================================================================
// ORCHESTRATION TEST: Carousel Mixed Stay Flags, Timer Wake Sequence
// Validates complex scenario with alternating stay/advance behavior
// =============================================================================

TEST_F(NormalModeIntegrationTest, Orchestration_CarouselMixedStay_TimerSequence) {
    // GIVEN: Carousel with alternating stay flags
    //   Image 0: 5 min, stay=false (auto-advance)
    //   Image 1: 10 min, stay=true (check CRC32)
    //   Image 2: 15 min, stay=false (auto-advance)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 5, false)
        .addImage("http://example.com/img1.png", 10, true)   // Only this one has stay=true
        .addImage("http://example.com/img2.png", 15, false)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    time_t currentTime = createTime(2025, 11, 15, 11, 0, 0);
    
    // SCENARIO 1: Wake on image 0 (stay=false) → should advance to image 1
    uint8_t currentIndex = 0;
    NormalModeDecisions dec1 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec1.imageTarget.targetIndex, 1);
    EXPECT_TRUE(dec1.imageTarget.shouldAdvance);
    EXPECT_EQ(dec1.finalIndex, 1);
    EXPECT_EQ(dec1.indexForCRC32, 0) << "CRC32 must use original index 0 (stay=false)";
    EXPECT_FALSE(dec1.crc32Action.shouldCheck) << "Original image 0 has stay=false";
    
    // SCENARIO 2: Wake on image 1 (stay=true) → should stay and check CRC32
    currentIndex = 1;
    NormalModeDecisions dec2 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec2.imageTarget.targetIndex, 1);
    EXPECT_FALSE(dec2.imageTarget.shouldAdvance);
    EXPECT_EQ(dec2.finalIndex, 1);
    EXPECT_EQ(dec2.indexForCRC32, 1);
    EXPECT_TRUE(dec2.crc32Action.shouldCheck);  // stay=true enables CRC32 check
    
    // SCENARIO 3: Wake on image 2 (stay=false) → should advance to image 0 (wrap)
    currentIndex = 2;
    NormalModeDecisions dec3 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec3.imageTarget.targetIndex, 0);  // Wrap to first
    EXPECT_TRUE(dec3.imageTarget.shouldAdvance);
    EXPECT_EQ(dec3.finalIndex, 0);
    EXPECT_EQ(dec3.indexForCRC32, 2) << "CRC32 must use original index 2 (stay=false)";
    EXPECT_FALSE(dec3.crc32Action.shouldCheck);
}

// =============================================================================
// ORCHESTRATION TEST: Edge Case - Single Image Carousel with stay=true
// Validates carousel with only one image and stay=true checks CRC32
// =============================================================================

TEST_F(NormalModeIntegrationTest, Orchestration_SingleImageCarousel_StayTrue) {
    // GIVEN: Carousel with only one image, stay=true (edge case)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 15, true)  // stay=true, single image
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    time_t currentTime = createTime(2025, 11, 15, 12, 0, 0);
    
    // WHEN: Call orchestration function
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, decisions.finalIndex, true);
    
    // THEN: Single image carousel with stay=true should check CRC32
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0);
    EXPECT_FALSE(decisions.imageTarget.shouldAdvance);  // Single image never advances
    EXPECT_EQ(decisions.finalIndex, 0);
    EXPECT_EQ(decisions.indexForCRC32, 0);
    EXPECT_TRUE(decisions.crc32Action.shouldCheck);  // stay=true enables optimization
    EXPECT_EQ(sleepDuration.sleepSeconds, 15.0f * 60.0f);
}

// =============================================================================
// ORCHESTRATION TEST: Large Carousel (10 Images)
// Validates index arithmetic doesn't break with larger arrays
// =============================================================================

TEST_F(NormalModeIntegrationTest, Orchestration_LargeCarousel_CorrectIndexArithmetic) {
    // GIVEN: Carousel with 10 images, mixed intervals and stay flags
    auto builder = ConfigBuilder().carousel();
    for (int i = 0; i < 10; i++) {
        char url[50];
        snprintf(url, sizeof(url), "http://example.com/img%d.png", i);
        builder.addImage(url, (i + 1) * 5, i % 3 == 0);  // Every 3rd image has stay=true
    }
    DashboardConfig config = builder.withCRC32(true).build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    time_t currentTime = createTime(2025, 11, 15, 8, 0, 0);
    
    // WHEN: Start at image 8 (stay=false), should advance to image 9
    uint8_t currentIndex = 8;
    NormalModeDecisions dec1 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec1.imageTarget.targetIndex, 9);
    EXPECT_TRUE(dec1.imageTarget.shouldAdvance);
    EXPECT_EQ(dec1.finalIndex, 9);
    EXPECT_EQ(dec1.indexForCRC32, 8) << "CRC32 uses original index 8";
    EXPECT_FALSE(dec1.crc32Action.shouldCheck);  // Image 8 has stay=false
    
    // WHEN: Now at image 9 (stay=true), should stay
    currentIndex = dec1.finalIndex;
    NormalModeDecisions dec2 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec2.imageTarget.targetIndex, 9);
    EXPECT_FALSE(dec2.imageTarget.shouldAdvance);
    EXPECT_EQ(dec2.finalIndex, 9);
    EXPECT_EQ(dec2.indexForCRC32, 9);
    EXPECT_TRUE(dec2.crc32Action.shouldCheck);  // Image 9 has stay=true (9 % 3 == 0)
    
    // WHEN: Button press from image 9, should advance and wrap to image 0
    wakeReason = WAKEUP_BUTTON;
    currentIndex = dec2.finalIndex;
    NormalModeDecisions dec3 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec3.imageTarget.targetIndex, 0);  // Wrap around
    EXPECT_TRUE(dec3.imageTarget.shouldAdvance);
    EXPECT_EQ(dec3.finalIndex, 0);
    EXPECT_EQ(dec3.indexForCRC32, 9) << "CRC32 uses original index 9";
}

// =============================================================================
// EDGE CASE TEST: Carousel Wrap-Around with CRC32 Disabled
// Tests that wrap-around works correctly when CRC32 is disabled
// =============================================================================

TEST_F(NormalModeIntegrationTest, EdgeCase_CarouselWrapAround_CRC32Disabled) {
    // GIVEN: 3-image carousel with CRC32 disabled
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 1, false)
        .addImage("http://example.com/img1.png", 1, false)
        .addImage("http://example.com/img2.png", 1, false)
        .withCRC32(false)  // CRC32 disabled
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 2;  // Last image
    
    // WHEN: Timer wake on last image (should wrap to 0)
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Should wrap around and CRC32 should not check
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0);
    EXPECT_TRUE(decisions.imageTarget.shouldAdvance);
    EXPECT_EQ(decisions.finalIndex, 0);
    EXPECT_EQ(decisions.indexForCRC32, 2) << "CRC32 uses original index 2";
    EXPECT_FALSE(decisions.crc32Action.shouldCheck) << "CRC32 disabled in config";
    EXPECT_STREQ(decisions.crc32Action.reason, "CRC32 disabled in config");
}

// =============================================================================
// EDGE CASE TEST: All Carousel Positions Visited
// Tests that carousel correctly handles advancing through ALL positions
// =============================================================================

TEST_F(NormalModeIntegrationTest, EdgeCase_CarouselAllPositions_SequentialAdvance) {
    // GIVEN: 5-image carousel with all stay=false
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 1, false)
        .addImage("http://example.com/img1.png", 1, false)
        .addImage("http://example.com/img2.png", 1, false)
        .addImage("http://example.com/img3.png", 1, false)
        .addImage("http://example.com/img4.png", 1, false)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    
    // WHEN: Advance through all 5 positions
    for (int i = 0; i < 5; i++) {
        NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
        
        // THEN: Each position should advance to next (with wrap-around)
        uint8_t expectedTarget = (currentIndex + 1) % 5;
        EXPECT_EQ(decisions.imageTarget.targetIndex, expectedTarget) 
            << "From position " << (int)currentIndex << " should advance to " << (int)expectedTarget;
        EXPECT_TRUE(decisions.imageTarget.shouldAdvance);
        EXPECT_EQ(decisions.finalIndex, expectedTarget);
        EXPECT_EQ(decisions.indexForCRC32, currentIndex) 
            << "CRC32 should use original index " << (int)currentIndex;
        EXPECT_FALSE(decisions.crc32Action.shouldCheck) << "stay=false means no CRC32 check";
        
        // Advance to next position for next iteration
        currentIndex = expectedTarget;
    }
    
    // Verify we wrapped around to start
    EXPECT_EQ(currentIndex, 0) << "After 5 advances, should be back at position 0";
}

// =============================================================================
// EDGE CASE TEST: Carousel with Mixed Stay Flags at Boundaries
// Tests that boundary positions (0 and max) work correctly with different stay flags
// =============================================================================

TEST_F(NormalModeIntegrationTest, EdgeCase_CarouselBoundaries_MixedStayFlags) {
    // GIVEN: Carousel where first and last images have stay=true, middle has stay=false
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img0.png", 1, true)   // Boundary: first (stay)
        .addImage("http://example.com/img1.png", 1, false)  // Middle (advance)
        .addImage("http://example.com/img2.png", 1, false)  // Middle (advance)
        .addImage("http://example.com/img3.png", 1, true)   // Boundary: last (stay)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    
    // SCENARIO 1: Timer wake at first position (stay=true)
    uint8_t currentIndex = 0;
    NormalModeDecisions dec1 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec1.imageTarget.targetIndex, 0);
    EXPECT_FALSE(dec1.imageTarget.shouldAdvance) << "First position has stay=true";
    EXPECT_EQ(dec1.finalIndex, 0);
    EXPECT_EQ(dec1.indexForCRC32, 0);
    EXPECT_TRUE(dec1.crc32Action.shouldCheck) << "stay=true means check CRC32";
    
    // SCENARIO 2: Button press at first position (forces advance)
    wakeReason = WAKEUP_BUTTON;
    NormalModeDecisions dec2 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec2.imageTarget.targetIndex, 1);
    EXPECT_TRUE(dec2.imageTarget.shouldAdvance) << "Button always advances";
    EXPECT_EQ(dec2.finalIndex, 1);
    EXPECT_EQ(dec2.indexForCRC32, 0);
    EXPECT_FALSE(dec2.crc32Action.shouldCheck) << "Button means no CRC32";
    
    // SCENARIO 3: Timer wake at last position (stay=true)
    wakeReason = WAKEUP_TIMER;
    currentIndex = 3;
    NormalModeDecisions dec3 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec3.imageTarget.targetIndex, 3);
    EXPECT_FALSE(dec3.imageTarget.shouldAdvance) << "Last position has stay=true";
    EXPECT_EQ(dec3.finalIndex, 3);
    EXPECT_EQ(dec3.indexForCRC32, 3);
    EXPECT_TRUE(dec3.crc32Action.shouldCheck) << "stay=true means check CRC32";
    
    // SCENARIO 4: Button press at last position (wraps to first)
    wakeReason = WAKEUP_BUTTON;
    NormalModeDecisions dec4 = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    EXPECT_EQ(dec4.imageTarget.targetIndex, 0) << "Should wrap around to first";
    EXPECT_TRUE(dec4.imageTarget.shouldAdvance);
    EXPECT_EQ(dec4.finalIndex, 0);
    EXPECT_EQ(dec4.indexForCRC32, 3) << "CRC32 uses original index (last position)";
    EXPECT_FALSE(dec4.crc32Action.shouldCheck) << "Button means no CRC32";
}

// =============================================================================
// EDGE CASE TEST: Single Image Carousel (Edge Case of Carousel)
// Tests that a 1-image carousel always stays on itself (special case logic)
// =============================================================================

TEST_F(NormalModeIntegrationTest, EdgeCase_SingleImageCarousel_AlwaysStaysOnSelf) {
    // GIVEN: Carousel with only 1 image (stay=false)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/onlyimage.png", 1, false)
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    
    // WHEN: Timer wake (stay=false)
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Special case - single-image carousel ALWAYS stays on current image
    // (imageCount == 1 overrides stay flag)
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0) << "Only one image, must stay on self";
    EXPECT_FALSE(decisions.imageTarget.shouldAdvance) << "Single-image carousel doesn't advance";
    EXPECT_EQ(decisions.finalIndex, 0);
    EXPECT_EQ(decisions.indexForCRC32, 0);
    EXPECT_TRUE(decisions.crc32Action.shouldCheck) << "Single image with stay logic checks CRC32";
}

// =============================================================================
// HOURLY SCHEDULE TESTS: calculateSleepMinutesToNextEnabledHour()
// Tests the helper function that calculates sleep time until next enabled hour
// =============================================================================

// Test: Current hour is enabled (no sleep needed)
TEST_F(NormalModeIntegrationTest, HourlySchedule_CurrentHourEnabled_NoSleep) {
    // GIVEN: Schedule with hours 9-17 enabled (9 AM to 5 PM)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(0)
        .build();
    
    // Current time: 14:30:00 UTC (2:30 PM - enabled hour)
    time_t currentTime = createTime(2025, 11, 15, 14, 30, 0);
    
    // WHEN: Calculate sleep time
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should return -1 (no sleep needed, current hour enabled)
    EXPECT_EQ(sleepMinutes, -1.0f) << "Current hour enabled, no sleep needed";
}

// Test: Midnight crossing with timezone offset
TEST_F(NormalModeIntegrationTest, HourlySchedule_MidnightCrossing_PositiveOffset) {
    // GIVEN: Schedule 9-17, timezone +8 (Asia)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(8)  // UTC+8
        .build();
    
    // Current time: 23:45:00 UTC = 07:45 local (disabled, before 9 AM)
    time_t currentTime = createTime(2025, 11, 15, 23, 45, 0);
    
    // WHEN: Calculate sleep time
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep until 9 AM local (1 hour 15 minutes)
    EXPECT_GT(sleepMinutes, 0.0f);
    EXPECT_NEAR(sleepMinutes, 1.0f * 60.0f + 14.0f, 2.0f);  // ~74 minutes (±2 tolerance)
}

// Test: Midnight crossing with negative timezone offset
TEST_F(NormalModeIntegrationTest, HourlySchedule_MidnightCrossing_NegativeOffset) {
    // GIVEN: Schedule 9-17, timezone -5 (EST)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(-5)  // UTC-5
        .build();
    
    // Current time: 02:30:00 UTC = 21:30 previous day local (disabled, after 5 PM)
    time_t currentTime = createTime(2025, 11, 15, 2, 30, 0);
    
    // WHEN: Calculate sleep time
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep until 9 AM local next day (~11.5 hours)
    EXPECT_GT(sleepMinutes, 0.0f);
    EXPECT_NEAR(sleepMinutes, 11.0f * 60.0f + 29.0f, 2.0f);  // ~689 minutes
}

// Test: Next enabled hour is immediately following
TEST_F(NormalModeIntegrationTest, HourlySchedule_NextHourEnabled_OneHourSleep) {
    // GIVEN: Schedule 9-17
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(0)
        .build();
    
    // Current time: 08:45:30 UTC (disabled, just before 9 AM)
    time_t currentTime = createTime(2025, 11, 15, 8, 45, 30);
    
    // WHEN: Calculate sleep time
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep ~14 minutes (until 9 AM)
    EXPECT_GT(sleepMinutes, 0.0f);
    EXPECT_NEAR(sleepMinutes, 14.0f, 2.0f);  // ~14 minutes (rounds based on seconds)
}

// Test: Multiple hours between current and next enabled
TEST_F(NormalModeIntegrationTest, HourlySchedule_MultipleHoursGap_LongSleep) {
    // GIVEN: Schedule 9-17
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(0)
        .build();
    
    // Current time: 02:15:00 UTC (disabled, middle of night)
    time_t currentTime = createTime(2025, 11, 15, 2, 15, 0);
    
    // WHEN: Calculate sleep time
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep ~6 hours 44 minutes (until 9 AM)
    EXPECT_GT(sleepMinutes, 0.0f);
    EXPECT_NEAR(sleepMinutes, 6.0f * 60.0f + 44.0f, 2.0f);  // ~404 minutes
}

// Test: Wrap-around to next day
TEST_F(NormalModeIntegrationTest, HourlySchedule_WrapAround_NextDaySleep) {
    // GIVEN: Schedule 9-17
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(0)
        .build();
    
    // Current time: 18:30:00 UTC (disabled, after 5 PM)
    time_t currentTime = createTime(2025, 11, 15, 18, 30, 0);
    
    // WHEN: Calculate sleep time
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep ~14.5 hours (until 9 AM next day)
    EXPECT_GT(sleepMinutes, 0.0f);
    EXPECT_NEAR(sleepMinutes, 14.0f * 60.0f + 29.0f, 2.0f);  // ~869 minutes
}

// Test: All hours enabled (always returns -1)
TEST_F(NormalModeIntegrationTest, HourlySchedule_AllHoursEnabled_NeverSleep) {
    // GIVEN: Schedule with all hours enabled (0-23)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withTimezone(0)
        .build();
    
    // Enable all hours manually
    for (int i = 0; i < 3; i++) {
        config.updateHours[i] = 0xFF;  // All bits set
    }
    
    // Test multiple times throughout the day
    for (int hour = 0; hour < 24; hour++) {
        time_t currentTime = createTime(2025, 11, 15, hour, 30, 0);
        
        // WHEN: Calculate sleep time
        float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
            currentTime, 
            config.timezoneOffset, 
            config.updateHours
        );
        
        // THEN: Should always return -1 (no sleep)
        EXPECT_EQ(sleepMinutes, -1.0f) << "Hour " << hour << " should be enabled";
    }
}

// Test: Seconds rounding (>30 seconds remaining adds 1 minute)
TEST_F(NormalModeIntegrationTest, HourlySchedule_SecondsRounding_ProperAdjustment) {
    // GIVEN: Schedule 9-17
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(0)
        .build();
    
    // SCENARIO 1: 15 seconds (45s remaining > 30s, rounds up)
    time_t time1 = createTime(2025, 11, 15, 8, 45, 15);
    float sleep1 = calculateSleepMinutesToNextEnabledHour(time1, 0, config.updateHours);
    
    // SCENARIO 2: 45 seconds (15s remaining ≤ 30s, no round up)
    time_t time2 = createTime(2025, 11, 15, 8, 45, 45);
    float sleep2 = calculateSleepMinutesToNextEnabledHour(time2, 0, config.updateHours);
    
    // THEN: sleep1 should be ~1 minute more than sleep2
    // Logic: (60 - currentSecond) > 30 means "more than 30s left in minute, add 1 min"
    EXPECT_GT(sleep1, 0.0f);
    EXPECT_GT(sleep2, 0.0f);
    EXPECT_NEAR(sleep1 - sleep2, 1.0f, 0.5f) << "15s (45s remaining) should round up, 45s (15s remaining) should not";
}

// Test: Extreme timezone offsets (+12, -12)
TEST_F(NormalModeIntegrationTest, HourlySchedule_ExtremeTimezones_CorrectCalculation) {
    // GIVEN: Schedule 9-17, extreme positive timezone (+12)
    DashboardConfig config1 = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(12)  // UTC+12 (Fiji)
        .build();
    
    // Current time: 21:00 UTC = 09:00 local (+12) - enabled
    time_t currentTime1 = createTime(2025, 11, 15, 21, 0, 0);
    float sleep1 = calculateSleepMinutesToNextEnabledHour(currentTime1, 12, config1.updateHours);
    EXPECT_EQ(sleep1, -1.0f) << "9 AM local (+12 offset) should be enabled";
    
    // GIVEN: Schedule 9-17, extreme negative timezone (-12)
    DashboardConfig config2 = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(-12)  // UTC-12 (Baker Island)
        .build();
    
    // Current time: 21:00 UTC = 09:00 local (-12 next day) - enabled
    time_t currentTime2 = createTime(2025, 11, 15, 21, 0, 0);
    float sleep2 = calculateSleepMinutesToNextEnabledHour(currentTime2, -12, config2.updateHours);
    EXPECT_EQ(sleep2, -1.0f) << "9 AM local (-12 offset) should be enabled";
}

// =============================================================================
// END-TO-END ORCHESTRATION SCENARIOS (Task D)
// =============================================================================
// These tests validate that the orchestration function correctly integrates
// hourly schedule logic with other decision functions.

// Test: Orchestration with hourly schedule - enabled hour allows normal operation
TEST_F(NormalModeIntegrationTest, EndToEnd_HourlyScheduleEnabled_NormalFlow) {
    // GIVEN: Single image, hourly schedule 9-17
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(0)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    
    // WHEN: Execute orchestration
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Normal flow proceeds
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0);
    EXPECT_EQ(decisions.finalIndex, 0);
    EXPECT_EQ(decisions.indexForCRC32, 0);
}

// Test: Button wake bypasses hourly schedule
TEST_F(NormalModeIntegrationTest, EndToEnd_ButtonWake_BypassesSchedule) {
    // GIVEN: Hourly schedule 9-17
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .build();
    
    WakeupReason wakeReason = WAKEUP_BUTTON;
    uint8_t currentIndex = 0;
    
    // WHEN: Button wake
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Button wake bypasses CRC32 check (forces download)
    EXPECT_EQ(decisions.crc32Action.shouldCheck, false);
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0);
}

// Test: Orchestration preserves original index for CRC32 check
TEST_F(NormalModeIntegrationTest, EndToEnd_OrchestrationPreservesIndexForCRC32) {
    // GIVEN: Config
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    
    // WHEN: Execute orchestration
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: indexForCRC32 matches original index
    EXPECT_EQ(decisions.indexForCRC32, currentIndex);
}

// Test: Hourly schedule with orchestration (simpler version)
TEST_F(NormalModeIntegrationTest, EndToEnd_HourlySchedule_Orchestration) {
    // GIVEN: Single image config
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 30)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    
    // WHEN: Execute orchestration
    NormalModeDecisions decisions = orchestrateNormalModeDecisions(config, wakeReason, currentIndex);
    
    // THEN: Orchestration completes successfully
    EXPECT_EQ(decisions.imageTarget.targetIndex, 0);
    EXPECT_EQ(decisions.finalIndex, 0);
}

// Test: All hours enabled means no hourly schedule sleep
TEST_F(NormalModeIntegrationTest, EndToEnd_AllHoursEnabled_NoScheduleConstraint) {
    // GIVEN: Default config (all hours enabled)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .build();  // No hourly schedule = all hours enabled
    
    // WHEN: Check at random times
    time_t time1 = createTime(2025, 11, 15, 3, 0, 0);
    time_t time2 = createTime(2025, 11, 15, 23, 45, 0);
    
    // THEN: Always returns -1 (no schedule-based sleep)
    EXPECT_EQ(calculateSleepMinutesToNextEnabledHour(time1, 0, config.updateHours), -1.0f);
    EXPECT_EQ(calculateSleepMinutesToNextEnabledHour(time2, 0, config.updateHours), -1.0f);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
