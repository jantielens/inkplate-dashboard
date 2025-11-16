/**
 * Unit tests for Normal Mode Controller decision functions
 * 
 * These tests validate the decision logic extracted in v1.5.2 refactoring.
 * Tests cover 40+ execution paths documented in NORMAL_MODE_FLOW.md
 */

#include <gtest/gtest.h>
#include <modes/decision_logic.h>  // Real production code!
#include <ctime>

// =============================================================================
// Test Fixture
// =============================================================================

class DecisionFunctionsTest : public ::testing::Test {
protected:
    DashboardConfig config;
    
    void SetUp() override {
        // Setup default config
        config = DashboardConfig();
    }
    
    // Helper to create single image config
    DashboardConfig createSingleImageConfig() {
        DashboardConfig cfg;
        cfg.imageCount = 1;
        cfg.imageUrls[0] = "http://example.com/image.png";
        cfg.imageIntervals[0] = 15;  // 15 minutes
        return cfg;
    }
    
    // Helper to create carousel config
    DashboardConfig createCarouselConfig(uint8_t count, bool defaultStay = false) {
        DashboardConfig cfg;
        cfg.imageCount = count;
        for (uint8_t i = 0; i < count; i++) {
            String url = String("http://example.com/image") + String(i) + String(".png");
            cfg.imageUrls[i] = url;
            cfg.imageIntervals[i] = 10 + i;  // 10, 11, 12, etc.
            cfg.imageStay[i] = defaultStay;
        }
        return cfg;
    }
};

// =============================================================================
// Tests for determineImageTarget()
// =============================================================================

TEST_F(DecisionFunctionsTest, ImageTarget_SingleMode_AlwaysReturnsIndexZero) {
    config = createSingleImageConfig();
    
    // Timer wake with any current index should return 0
    auto result = determineImageTarget(config, WAKEUP_TIMER, 5);
    EXPECT_EQ(result.targetIndex, 0);
    EXPECT_FALSE(result.shouldAdvance);
    EXPECT_STREQ(result.reason, "Single image mode");
    
    // Button wake should also return 0
    result = determineImageTarget(config, WAKEUP_BUTTON, 3);
    EXPECT_EQ(result.targetIndex, 0);
    EXPECT_FALSE(result.shouldAdvance);
}

TEST_F(DecisionFunctionsTest, ImageTarget_CarouselButtonWake_AlwaysAdvances) {
    config = createCarouselConfig(5);
    
    // Button wake should advance to next image
    auto result = determineImageTarget(config, WAKEUP_BUTTON, 2);
    EXPECT_EQ(result.targetIndex, 3);
    EXPECT_TRUE(result.shouldAdvance);
    EXPECT_STREQ(result.reason, "Carousel - button press (always advance)");
    
    // Test wrap-around: last image (4) should go to 0
    result = determineImageTarget(config, WAKEUP_BUTTON, 4);
    EXPECT_EQ(result.targetIndex, 0);
    EXPECT_TRUE(result.shouldAdvance);
}

TEST_F(DecisionFunctionsTest, ImageTarget_CarouselTimerStayFalse_Advances) {
    config = createCarouselConfig(5, false);  // All stay:false
    
    auto result = determineImageTarget(config, WAKEUP_TIMER, 2);
    EXPECT_EQ(result.targetIndex, 3);
    EXPECT_TRUE(result.shouldAdvance);
    EXPECT_STREQ(result.reason, "Carousel - timer wake + stay:false (auto-advance)");
}

TEST_F(DecisionFunctionsTest, ImageTarget_CarouselTimerStayTrue_DoesNotAdvance) {
    config = createCarouselConfig(5, true);  // All stay:true
    
    auto result = determineImageTarget(config, WAKEUP_TIMER, 2);
    EXPECT_EQ(result.targetIndex, 2);  // Same index
    EXPECT_FALSE(result.shouldAdvance);
    EXPECT_STREQ(result.reason, "Carousel - stay flag set (stay:true)");
}

TEST_F(DecisionFunctionsTest, ImageTarget_CarouselMixedStay_RespectsPerImageFlag) {
    config = createCarouselConfig(3);
    config.imageStay[0] = false;  // Auto-advance
    config.imageStay[1] = true;   // Stay
    config.imageStay[2] = false;  // Auto-advance
    
    // Image 0 (stay:false) should advance
    auto result = determineImageTarget(config, WAKEUP_TIMER, 0);
    EXPECT_EQ(result.targetIndex, 1);
    EXPECT_TRUE(result.shouldAdvance);
    
    // Image 1 (stay:true) should not advance
    result = determineImageTarget(config, WAKEUP_TIMER, 1);
    EXPECT_EQ(result.targetIndex, 1);
    EXPECT_FALSE(result.shouldAdvance);
    
    // Image 2 (stay:false) should advance (wraps to 0)
    result = determineImageTarget(config, WAKEUP_TIMER, 2);
    EXPECT_EQ(result.targetIndex, 0);
    EXPECT_TRUE(result.shouldAdvance);
}

// =============================================================================
// Tests for determineCRC32Action()
// =============================================================================

TEST_F(DecisionFunctionsTest, CRC32Action_DisabledInConfig_NeverChecks) {
    config = createSingleImageConfig();
    config.useCRC32Check = false;
    
    auto result = determineCRC32Action(config, WAKEUP_TIMER, 0);
    EXPECT_FALSE(result.shouldCheck);
    EXPECT_STREQ(result.reason, "CRC32 disabled in config");
    
    // Should be same for button wake
    result = determineCRC32Action(config, WAKEUP_BUTTON, 0);
    EXPECT_FALSE(result.shouldCheck);
}

TEST_F(DecisionFunctionsTest, CRC32Action_SingleModeTimerWake_Checks) {
    config = createSingleImageConfig();
    config.useCRC32Check = true;
    
    auto result = determineCRC32Action(config, WAKEUP_TIMER, 0);
    EXPECT_TRUE(result.shouldCheck);
    EXPECT_STREQ(result.reason, "Single image - timer wake (check for skip)");
}

TEST_F(DecisionFunctionsTest, CRC32Action_SingleModeButtonWake_NoCheck) {
    config = createSingleImageConfig();
    config.useCRC32Check = true;
    
    auto result = determineCRC32Action(config, WAKEUP_BUTTON, 0);
    EXPECT_FALSE(result.shouldCheck);
    EXPECT_STREQ(result.reason, "Single image - button press (always download)");
}

TEST_F(DecisionFunctionsTest, CRC32Action_CarouselButtonWake_NoCheck) {
    config = createCarouselConfig(3, true);
    config.useCRC32Check = true;
    
    auto result = determineCRC32Action(config, WAKEUP_BUTTON, 0);
    EXPECT_FALSE(result.shouldCheck);
    EXPECT_STREQ(result.reason, "Carousel - button press (always download)");
}

TEST_F(DecisionFunctionsTest, CRC32Action_CarouselTimerStayFalse_NoCheck) {
    config = createCarouselConfig(3, false);  // stay:false
    config.useCRC32Check = true;
    
    auto result = determineCRC32Action(config, WAKEUP_TIMER, 1);
    EXPECT_FALSE(result.shouldCheck);
    EXPECT_STREQ(result.reason, "Carousel - auto-advance (always download)");
}

TEST_F(DecisionFunctionsTest, CRC32Action_CarouselTimerStayTrue_Checks) {
    config = createCarouselConfig(3, true);  // stay:true
    config.useCRC32Check = true;
    
    auto result = determineCRC32Action(config, WAKEUP_TIMER, 1);
    EXPECT_TRUE(result.shouldCheck);
    EXPECT_STREQ(result.reason, "Carousel - timer wake + stay:true (check for skip)");
}

TEST_F(DecisionFunctionsTest, CRC32Action_CarouselMixedStay_ChecksOnlyStayTrue) {
    config = createCarouselConfig(3);
    config.useCRC32Check = true;
    config.imageStay[0] = false;
    config.imageStay[1] = true;
    config.imageStay[2] = false;
    
    // Image 0 (stay:false) should not check
    auto result = determineCRC32Action(config, WAKEUP_TIMER, 0);
    EXPECT_FALSE(result.shouldCheck);
    
    // Image 1 (stay:true) should check
    result = determineCRC32Action(config, WAKEUP_TIMER, 1);
    EXPECT_TRUE(result.shouldCheck);
    
    // Image 2 (stay:false) should not check
    result = determineCRC32Action(config, WAKEUP_TIMER, 2);
    EXPECT_FALSE(result.shouldCheck);
}

// =============================================================================
// Tests for determineSleepDuration()
// =============================================================================

TEST_F(DecisionFunctionsTest, SleepDuration_ButtonOnlyMode_ReturnsZero) {
    config = createSingleImageConfig();
    config.imageIntervals[0] = 0;  // Button-only mode
    
    time_t now = time(nullptr);
    auto result = determineSleepDuration(config, now, 0, false);
    
    EXPECT_EQ(result.sleepSeconds, 0.0f);
    EXPECT_STREQ(result.reason, "Button-only mode (interval = 0)");
}

TEST_F(DecisionFunctionsTest, SleepDuration_NormalInterval_ReturnsIntervalSeconds) {
    config = createSingleImageConfig();
    config.imageIntervals[0] = 15;  // 15 minutes
    
    time_t now = time(nullptr);
    auto result = determineSleepDuration(config, now, 0, false);
    
    EXPECT_EQ(result.sleepSeconds, 15.0f * 60.0f);  // 900 seconds
    EXPECT_STREQ(result.reason, "Image interval (image updated)");
}

TEST_F(DecisionFunctionsTest, SleepDuration_CRC32Matched_UsesMatchedReason) {
    config = createSingleImageConfig();
    config.imageIntervals[0] = 10;  // 10 minutes
    
    time_t now = time(nullptr);
    auto result = determineSleepDuration(config, now, 0, true);  // CRC32 matched
    
    EXPECT_EQ(result.sleepSeconds, 10.0f * 60.0f);  // 600 seconds
    EXPECT_STREQ(result.reason, "Image interval (CRC32 matched)");
}

TEST_F(DecisionFunctionsTest, SleepDuration_CarouselUsesCurrentImageInterval) {
    config = createCarouselConfig(3);
    config.imageIntervals[0] = 5;
    config.imageIntervals[1] = 10;
    config.imageIntervals[2] = 15;
    
    time_t now = time(nullptr);
    
    // Image 0: 5 minutes
    auto result = determineSleepDuration(config, now, 0, false);
    EXPECT_EQ(result.sleepSeconds, 5.0f * 60.0f);
    
    // Image 1: 10 minutes
    result = determineSleepDuration(config, now, 1, false);
    EXPECT_EQ(result.sleepSeconds, 10.0f * 60.0f);
    
    // Image 2: 15 minutes
    result = determineSleepDuration(config, now, 2, false);
    EXPECT_EQ(result.sleepSeconds, 15.0f * 60.0f);
}

// =============================================================================
// Integration Tests - Combined Decision Flow
// =============================================================================

TEST_F(DecisionFunctionsTest, IntegrationFlow_SingleImageTimerWakeCRC32Match) {
    // Scenario: Single image, timer wake, CRC32 enabled and matches
    // Expected: Check CRC32, skip download, sleep for interval
    config = createSingleImageConfig();
    config.useCRC32Check = true;
    config.imageIntervals[0] = 15;
    
    auto targetDecision = determineImageTarget(config, WAKEUP_TIMER, 0);
    EXPECT_EQ(targetDecision.targetIndex, 0);
    EXPECT_FALSE(targetDecision.shouldAdvance);
    
    auto crc32Decision = determineCRC32Action(config, WAKEUP_TIMER, 0);
    EXPECT_TRUE(crc32Decision.shouldCheck);  // Should check for skip
    
    time_t now = time(nullptr);
    auto sleepDecision = determineSleepDuration(config, now, 0, true);
    EXPECT_EQ(sleepDecision.sleepSeconds, 15.0f * 60.0f);
}

TEST_F(DecisionFunctionsTest, IntegrationFlow_CarouselButtonWake) {
    // Scenario: Carousel, button wake, CRC32 enabled
    // Expected: Advance to next, no CRC32 check, sleep for next image interval
    config = createCarouselConfig(3);
    config.useCRC32Check = true;
    config.imageIntervals[0] = 5;
    config.imageIntervals[1] = 10;
    config.imageIntervals[2] = 15;
    
    auto targetDecision = determineImageTarget(config, WAKEUP_BUTTON, 1);
    EXPECT_EQ(targetDecision.targetIndex, 2);
    EXPECT_TRUE(targetDecision.shouldAdvance);
    
    auto crc32Decision = determineCRC32Action(config, WAKEUP_BUTTON, 1);
    EXPECT_FALSE(crc32Decision.shouldCheck);  // Button wake never checks
    
    time_t now = time(nullptr);
    auto sleepDecision = determineSleepDuration(config, now, 2, false);
    EXPECT_EQ(sleepDecision.sleepSeconds, 15.0f * 60.0f);  // Image 2 interval
}

TEST_F(DecisionFunctionsTest, IntegrationFlow_CarouselTimerStayTrueCRC32Match) {
    // Scenario: Carousel, timer wake, stay:true, CRC32 match
    // Expected: Don't advance, check CRC32, skip download, sleep interval
    config = createCarouselConfig(3, true);  // All stay:true
    config.useCRC32Check = true;
    config.imageIntervals[1] = 20;
    
    auto targetDecision = determineImageTarget(config, WAKEUP_TIMER, 1);
    EXPECT_EQ(targetDecision.targetIndex, 1);  // Stay on current
    EXPECT_FALSE(targetDecision.shouldAdvance);
    
    auto crc32Decision = determineCRC32Action(config, WAKEUP_TIMER, 1);
    EXPECT_TRUE(crc32Decision.shouldCheck);  // Timer + stay:true checks
    
    time_t now = time(nullptr);
    auto sleepDecision = determineSleepDuration(config, now, 1, true);
    EXPECT_EQ(sleepDecision.sleepSeconds, 20.0f * 60.0f);
    EXPECT_STREQ(sleepDecision.reason, "Image interval (CRC32 matched)");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
