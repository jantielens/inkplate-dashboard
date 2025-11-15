#include <gtest/gtest.h>
#include <sleep_logic.h>

// Test fixture for sleep duration calculations
class SleepLogicTest : public ::testing::Test {
protected:
    // Microseconds per second for assertions
    static constexpr uint64_t MICROS_PER_SECOND = 1000000ULL;
};

// ============================================================================
// Button-Only Mode Tests (interval = 0)
// ============================================================================

TEST_F(SleepLogicTest, ButtonOnlyMode_ReturnsZero) {
    EXPECT_EQ(calculateAdjustedSleepDuration(0.0f, 0.0f), 0);
    EXPECT_EQ(calculateAdjustedSleepDuration(0.0f, 5.0f), 0);
}

// ============================================================================
// No Compensation Tests (loopTime = 0 or negative)
// ============================================================================

TEST_F(SleepLogicTest, NoCompensation_ReturnsFullInterval) {
    // 60 seconds = 60,000,000 microseconds
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, 0.0f), 60 * MICROS_PER_SECOND);
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, -1.0f), 60 * MICROS_PER_SECOND);
}

TEST_F(SleepLogicTest, NoCompensation_VariousIntervals) {
    EXPECT_EQ(calculateAdjustedSleepDuration(300.0f, 0.0f), 300 * MICROS_PER_SECOND);  // 5 minutes
    EXPECT_EQ(calculateAdjustedSleepDuration(1800.0f, 0.0f), 1800 * MICROS_PER_SECOND); // 30 minutes
    EXPECT_EQ(calculateAdjustedSleepDuration(3600.0f, 0.0f), 3600 * MICROS_PER_SECOND); // 1 hour
}

// ============================================================================
// Normal Compensation Tests (loopTime < interval)
// ============================================================================

TEST_F(SleepLogicTest, NormalCompensation_SubtractsLoopTime) {
    // 60s interval - 7s loop = 53s sleep
    uint64_t expected = (60 - 7) * MICROS_PER_SECOND;
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, 7.0f), expected);
}

TEST_F(SleepLogicTest, NormalCompensation_SmallLoopTime) {
    // 300s interval - 2.5s loop = 297.5s sleep
    uint64_t expected = (uint64_t)((300.0f - 2.5f) * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(300.0f, 2.5f), expected);
}

TEST_F(SleepLogicTest, NormalCompensation_LargeInterval) {
    // 3600s interval - 120s loop = 3480s sleep
    uint64_t expected = (3600 - 120) * MICROS_PER_SECOND;
    EXPECT_EQ(calculateAdjustedSleepDuration(3600.0f, 120.0f), expected);
}

TEST_F(SleepLogicTest, NormalCompensation_FractionalSeconds) {
    // 60.5s interval - 7.3s loop = 53.2s sleep
    uint64_t expected = (uint64_t)((60.5f - 7.3f) * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(60.5f, 7.3f), expected);
}

// ============================================================================
// Edge Case: Loop Time >= Interval (prevent 0-second sleep)
// ============================================================================

TEST_F(SleepLogicTest, LoopTimeEqualsInterval_ReturnsFullInterval) {
    // If loop takes exactly as long as interval, still sleep full interval
    // (accept drift rather than 0-second sleep)
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, 60.0f), 60 * MICROS_PER_SECOND);
}

TEST_F(SleepLogicTest, LoopTimeExceedsInterval_ReturnsFullInterval) {
    // If loop takes longer than interval (poor network), still sleep full interval
    // This prevents 0-second sleep cycles
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, 70.0f), 60 * MICROS_PER_SECOND);
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, 120.0f), 60 * MICROS_PER_SECOND);
}

// ============================================================================
// Realistic Scenario Tests
// ============================================================================

TEST_F(SleepLogicTest, RealisticScenario_FastRefreshGoodNetwork) {
    // 5 minute refresh, 6 second active time
    float interval = 5.0f * 60.0f;  // 300s
    float loopTime = 6.0f;
    uint64_t expected = (uint64_t)((300.0f - 6.0f) * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(interval, loopTime), expected);
}

TEST_F(SleepLogicTest, RealisticScenario_HourlyRefreshSlowNetwork) {
    // 1 hour refresh, 45 second active time
    float interval = 3600.0f;
    float loopTime = 45.0f;
    uint64_t expected = (3600 - 45) * MICROS_PER_SECOND;
    EXPECT_EQ(calculateAdjustedSleepDuration(interval, loopTime), expected);
}

TEST_F(SleepLogicTest, RealisticScenario_ShortIntervalFastDevice) {
    // 1 minute refresh, 3.5 second active time
    float interval = 60.0f;
    float loopTime = 3.5f;
    uint64_t expected = (uint64_t)((60.0f - 3.5f) * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(interval, loopTime), expected);
}

TEST_F(SleepLogicTest, RealisticScenario_VerySlowNetwork) {
    // 5 minute refresh, but network took 6 minutes (timeout case)
    // Should return full interval to prevent 0-second sleep
    float interval = 300.0f;
    float loopTime = 360.0f;
    EXPECT_EQ(calculateAdjustedSleepDuration(interval, loopTime), 300 * MICROS_PER_SECOND);
}

// ============================================================================
// Microsecond Precision Tests
// ============================================================================

TEST_F(SleepLogicTest, MicrosecondPrecision_VerySmallInterval) {
    // 0.1 second interval (100ms) - 0.05s loop = 0.05s sleep
    uint64_t expected = (uint64_t)(0.05f * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(0.1f, 0.05f), expected);
}

TEST_F(SleepLogicTest, MicrosecondPrecision_VerySmallLoopTime) {
    // 60s interval - 0.001s loop (1ms)
    uint64_t expected = (uint64_t)((60.0f - 0.001f) * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(60.0f, 0.001f), expected);
}

// ============================================================================
// Boundary Tests
// ============================================================================

TEST_F(SleepLogicTest, Boundary_VeryLargeInterval) {
    // 24 hours (86400 seconds)
    float interval = 86400.0f;
    float loopTime = 30.0f;
    uint64_t expected = (uint64_t)((86400.0f - 30.0f) * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(interval, loopTime), expected);
}

TEST_F(SleepLogicTest, Boundary_AlmostZeroInterval) {
    // Very small but non-zero interval
    float interval = 0.001f;  // 1ms
    uint64_t expected = (uint64_t)(0.001f * 1000000.0);
    EXPECT_EQ(calculateAdjustedSleepDuration(interval, 0.0f), expected);
}

TEST_F(SleepLogicTest, Boundary_AlmostFullCompensation) {
    // Loop time almost equals interval (59.999s loop, 60s interval)
    float interval = 60.0f;
    float loopTime = 59.999f;
    uint64_t expected = (uint64_t)((60.0f - 59.999f) * 1000000.0);
    
    uint64_t result = calculateAdjustedSleepDuration(interval, loopTime);
    
    // Should be very small but non-zero
    EXPECT_GT(result, 0);
    EXPECT_LT(result, 10000);  // Less than 10ms
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F(SleepLogicTest, Consistency_MultipleCallsSameResult) {
    // Same inputs should always produce same outputs
    uint64_t result1 = calculateAdjustedSleepDuration(300.0f, 12.5f);
    uint64_t result2 = calculateAdjustedSleepDuration(300.0f, 12.5f);
    uint64_t result3 = calculateAdjustedSleepDuration(300.0f, 12.5f);
    
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result2, result3);
}
