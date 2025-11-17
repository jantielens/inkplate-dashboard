#include <gtest/gtest.h>
#include <battery_logic.h>

// Test fixture for battery percentage calculations
class BatteryLogicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
};

// ============================================================================
// Out-of-Range Tests
// ============================================================================

TEST_F(BatteryLogicTest, VoltageAboveMax_Returns100) {
    EXPECT_EQ(calculateBatteryPercentage(4.25f), 100);
    EXPECT_EQ(calculateBatteryPercentage(5.00f), 100);
}

TEST_F(BatteryLogicTest, VoltageBelowMin_Returns0) {
    EXPECT_EQ(calculateBatteryPercentage(3.42f), 0);
    EXPECT_EQ(calculateBatteryPercentage(3.00f), 0);
    EXPECT_EQ(calculateBatteryPercentage(2.50f), 0);
    EXPECT_EQ(calculateBatteryPercentage(0.00f), 0);
}

// ============================================================================
// Exact Map Point Tests (should return exact percentages)
// Based on real-world 192-hour discharge test (GitHub issue #57)
// ============================================================================

TEST_F(BatteryLogicTest, ExactMapPoints_ReturnCorrectPercentages) {
    EXPECT_EQ(calculateBatteryPercentage(4.13f), 100);
    EXPECT_EQ(calculateBatteryPercentage(4.12f), 95);
    EXPECT_EQ(calculateBatteryPercentage(4.08f), 90);
    EXPECT_EQ(calculateBatteryPercentage(4.04f), 85);
    EXPECT_EQ(calculateBatteryPercentage(4.02f), 80);
    EXPECT_EQ(calculateBatteryPercentage(4.00f), 75);
    EXPECT_EQ(calculateBatteryPercentage(3.98f), 70);
    EXPECT_EQ(calculateBatteryPercentage(3.95f), 65);
    EXPECT_EQ(calculateBatteryPercentage(3.92f), 60);
    EXPECT_EQ(calculateBatteryPercentage(3.88f), 55);
    EXPECT_EQ(calculateBatteryPercentage(3.85f), 50);
    EXPECT_EQ(calculateBatteryPercentage(3.82f), 45);
    EXPECT_EQ(calculateBatteryPercentage(3.79f), 40);
    EXPECT_EQ(calculateBatteryPercentage(3.75f), 35);
    EXPECT_EQ(calculateBatteryPercentage(3.71f), 30);
    EXPECT_EQ(calculateBatteryPercentage(3.66f), 25);
    EXPECT_EQ(calculateBatteryPercentage(3.64f), 20);
    EXPECT_EQ(calculateBatteryPercentage(3.59f), 15);
    EXPECT_EQ(calculateBatteryPercentage(3.53f), 10);
    EXPECT_EQ(calculateBatteryPercentage(3.47f), 5);
    EXPECT_EQ(calculateBatteryPercentage(3.43f), 0);
}

// ============================================================================
// Interpolation Tests (between map points)
// ============================================================================

TEST_F(BatteryLogicTest, Interpolation_BetweenHighPoints) {
    // Between 4.13V (100%) and 4.12V (95%)
    // Midpoint 4.125V should be ~97.5%, rounded to 95% or 100%
    int result = calculateBatteryPercentage(4.125f);
    EXPECT_GE(result, 95);
    EXPECT_LE(result, 100);
}

TEST_F(BatteryLogicTest, Interpolation_BetweenMidPoints) {
    // Between 3.85V (50%) and 3.82V (45%)
    // Midpoint 3.835V should be ~47.5%, rounded to 45% or 50%
    int result = calculateBatteryPercentage(3.835f);
    EXPECT_GE(result, 45);
    EXPECT_LE(result, 50);
}

TEST_F(BatteryLogicTest, Interpolation_BetweenLowPoints) {
    // Between 3.64V (20%) and 3.59V (15%)
    // Midpoint 3.615V should be ~17.5%, rounded to 15% or 20%
    int result = calculateBatteryPercentage(3.615f);
    EXPECT_GE(result, 15);
    EXPECT_LE(result, 20);
}

// ============================================================================
// Rounding to 5% Increments Tests
// ============================================================================

TEST_F(BatteryLogicTest, Rounding_To5PercentIncrements) {
    // All results should be multiples of 5
    for (float voltage = 3.0f; voltage <= 4.2f; voltage += 0.01f) {
        int percentage = calculateBatteryPercentage(voltage);
        EXPECT_EQ(percentage % 5, 0) << "Voltage " << voltage << " returned " << percentage;
    }
}

TEST_F(BatteryLogicTest, Rounding_NeverExceeds100) {
    for (float voltage = 3.0f; voltage <= 5.0f; voltage += 0.05f) {
        int percentage = calculateBatteryPercentage(voltage);
        EXPECT_LE(percentage, 100) << "Voltage " << voltage << " exceeded 100%";
    }
}

TEST_F(BatteryLogicTest, Rounding_NeverBelowZero) {
    for (float voltage = 0.0f; voltage <= 4.5f; voltage += 0.05f) {
        int percentage = calculateBatteryPercentage(voltage);
        EXPECT_GE(percentage, 0) << "Voltage " << voltage << " went below 0%";
    }
}

// ============================================================================
// Monotonic Behavior Tests (higher voltage = higher/equal percentage)
// ============================================================================

TEST_F(BatteryLogicTest, MonotonicBehavior_IncreasingVoltageNeverDecreases) {
    int previousPercentage = 0;
    for (float voltage = 3.0f; voltage <= 4.2f; voltage += 0.01f) {
        int percentage = calculateBatteryPercentage(voltage);
        EXPECT_GE(percentage, previousPercentage) 
            << "Voltage " << voltage << " returned " << percentage 
            << " which is less than previous " << previousPercentage;
        previousPercentage = percentage;
    }
}

// ============================================================================
// Realistic Scenario Tests
// ============================================================================

TEST_F(BatteryLogicTest, RealisticScenario_FullyCharged) {
    // Fully charged based on real-world data: 4.13V max
    EXPECT_EQ(calculateBatteryPercentage(4.13f), 100);
    EXPECT_EQ(calculateBatteryPercentage(4.14f), 100);
}

TEST_F(BatteryLogicTest, RealisticScenario_HalfCharged) {
    // Half-charged based on real-world data: 3.85V
    EXPECT_EQ(calculateBatteryPercentage(3.85f), 50);
}

TEST_F(BatteryLogicTest, RealisticScenario_LowBattery) {
    // Low battery warning threshold: ~3.70V (should be around 25-35%)
    int percentage = calculateBatteryPercentage(3.70f);
    EXPECT_GE(percentage, 25);
    EXPECT_LE(percentage, 35);
}

TEST_F(BatteryLogicTest, RealisticScenario_CriticallyLow) {
    // Critically low (near device cutoff): ~3.45V
    int percentage = calculateBatteryPercentage(3.45f);
    EXPECT_GE(percentage, 0);
    EXPECT_LE(percentage, 5);
}

TEST_F(BatteryLogicTest, RealisticScenario_NominalVoltage) {
    // Nominal lithium voltage: 3.70V (around 25-35% based on real data)
    int percentage = calculateBatteryPercentage(3.70f);
    EXPECT_GE(percentage, 25);
    EXPECT_LE(percentage, 35);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(BatteryLogicTest, EdgeCase_VeryCloseToMapPoint) {
    // Just slightly above/below map points should still round correctly
    EXPECT_EQ(calculateBatteryPercentage(4.1299f), 100);
    EXPECT_EQ(calculateBatteryPercentage(4.1301f), 100);
    EXPECT_EQ(calculateBatteryPercentage(3.4301f), 0);
}

TEST_F(BatteryLogicTest, EdgeCase_NegativeVoltage) {
    // Should handle negative voltages (faulty sensor)
    EXPECT_EQ(calculateBatteryPercentage(-1.0f), 0);
}
