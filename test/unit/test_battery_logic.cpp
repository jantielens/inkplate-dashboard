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
    EXPECT_EQ(calculateBatteryPercentage(2.99f), 0);
    EXPECT_EQ(calculateBatteryPercentage(2.50f), 0);
    EXPECT_EQ(calculateBatteryPercentage(0.00f), 0);
}

// ============================================================================
// Exact Map Point Tests (should return exact percentages)
// ============================================================================

TEST_F(BatteryLogicTest, ExactMapPoints_ReturnCorrectPercentages) {
    EXPECT_EQ(calculateBatteryPercentage(4.20f), 100);
    EXPECT_EQ(calculateBatteryPercentage(4.15f), 95);
    EXPECT_EQ(calculateBatteryPercentage(4.11f), 90);
    EXPECT_EQ(calculateBatteryPercentage(4.08f), 85);
    EXPECT_EQ(calculateBatteryPercentage(4.02f), 80);
    EXPECT_EQ(calculateBatteryPercentage(3.98f), 75);
    EXPECT_EQ(calculateBatteryPercentage(3.95f), 70);
    EXPECT_EQ(calculateBatteryPercentage(3.91f), 65);
    EXPECT_EQ(calculateBatteryPercentage(3.87f), 60);
    EXPECT_EQ(calculateBatteryPercentage(3.85f), 55);
    EXPECT_EQ(calculateBatteryPercentage(3.84f), 50);
    EXPECT_EQ(calculateBatteryPercentage(3.82f), 45);
    EXPECT_EQ(calculateBatteryPercentage(3.80f), 40);
    EXPECT_EQ(calculateBatteryPercentage(3.79f), 35);
    EXPECT_EQ(calculateBatteryPercentage(3.77f), 30);
    EXPECT_EQ(calculateBatteryPercentage(3.75f), 25);
    EXPECT_EQ(calculateBatteryPercentage(3.73f), 20);
    EXPECT_EQ(calculateBatteryPercentage(3.71f), 15);
    EXPECT_EQ(calculateBatteryPercentage(3.69f), 10);
    EXPECT_EQ(calculateBatteryPercentage(3.61f), 5);
    EXPECT_EQ(calculateBatteryPercentage(3.00f), 0);
}

// ============================================================================
// Interpolation Tests (between map points)
// ============================================================================

TEST_F(BatteryLogicTest, Interpolation_BetweenHighPoints) {
    // Between 4.20V (100%) and 4.15V (95%)
    // Midpoint 4.175V should be ~97.5%, rounded to 100%
    int result = calculateBatteryPercentage(4.175f);
    EXPECT_GE(result, 95);
    EXPECT_LE(result, 100);
}

TEST_F(BatteryLogicTest, Interpolation_BetweenMidPoints) {
    // Between 3.84V (50%) and 3.82V (45%)
    // Midpoint 3.83V should be ~47.5%, rounded to 45% or 50%
    int result = calculateBatteryPercentage(3.83f);
    EXPECT_GE(result, 45);
    EXPECT_LE(result, 50);
}

TEST_F(BatteryLogicTest, Interpolation_BetweenLowPoints) {
    // Between 3.69V (10%) and 3.61V (5%)
    // Midpoint 3.65V should be ~7.5%, rounded to 5% or 10%
    int result = calculateBatteryPercentage(3.65f);
    EXPECT_GE(result, 5);
    EXPECT_LE(result, 10);
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
    // Typical fully charged lithium battery: 4.18-4.20V
    EXPECT_EQ(calculateBatteryPercentage(4.18f), 100);
    EXPECT_EQ(calculateBatteryPercentage(4.19f), 100);
}

TEST_F(BatteryLogicTest, RealisticScenario_HalfCharged) {
    // Typical half-charged battery: ~3.84V
    EXPECT_EQ(calculateBatteryPercentage(3.84f), 50);
}

TEST_F(BatteryLogicTest, RealisticScenario_LowBattery) {
    // Low battery warning threshold: ~3.7V (should be around 15-20%)
    int percentage = calculateBatteryPercentage(3.70f);
    EXPECT_GE(percentage, 10);
    EXPECT_LE(percentage, 20);
}

TEST_F(BatteryLogicTest, RealisticScenario_CriticallyLow) {
    // Critically low (should warn user): ~3.3V
    int percentage = calculateBatteryPercentage(3.30f);
    EXPECT_GE(percentage, 0);
    EXPECT_LE(percentage, 5);
}

TEST_F(BatteryLogicTest, RealisticScenario_NominalVoltage) {
    // Nominal lithium voltage: 3.7V (around 20-30%)
    int percentage = calculateBatteryPercentage(3.70f);
    EXPECT_GE(percentage, 15);
    EXPECT_LE(percentage, 25);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(BatteryLogicTest, EdgeCase_VeryCloseToMapPoint) {
    // Just slightly above/below map points should still round correctly
    EXPECT_EQ(calculateBatteryPercentage(4.1999f), 100);
    EXPECT_EQ(calculateBatteryPercentage(4.2001f), 100);
    EXPECT_EQ(calculateBatteryPercentage(3.0001f), 0);
}

TEST_F(BatteryLogicTest, EdgeCase_NegativeVoltage) {
    // Should handle negative voltages (faulty sensor)
    EXPECT_EQ(calculateBatteryPercentage(-1.0f), 0);
}
