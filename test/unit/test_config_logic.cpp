#include <gtest/gtest.h>
#include <config_logic.h>

// Test fixture for configuration logic
class ConfigLogicTest : public ::testing::Test {
protected:
    // Helper to create bitmask with specific hours enabled
    void setBitmask(uint8_t bitmask[3], const std::vector<int>& enabledHours) {
        // Start with all disabled
        bitmask[0] = bitmask[1] = bitmask[2] = 0;
        
        // Enable specified hours
        for (int hour : enabledHours) {
            if (hour >= 0 && hour < 24) {
                int byteIndex = hour / 8;
                int bitIndex = hour % 8;
                bitmask[byteIndex] |= (1 << bitIndex);
            }
        }
    }
};

// ============================================================================
// Timezone Offset Tests
// ============================================================================

TEST_F(ConfigLogicTest, TimezoneOffset_NoOffset_ReturnsOriginal) {
    EXPECT_EQ(applyTimezoneOffset(12, 0), 12);
    EXPECT_EQ(applyTimezoneOffset(0, 0), 0);
    EXPECT_EQ(applyTimezoneOffset(23, 0), 23);
}

TEST_F(ConfigLogicTest, TimezoneOffset_PositiveOffset_NoWrap) {
    EXPECT_EQ(applyTimezoneOffset(10, 2), 12);  // 10 UTC + 2h = 12 local
    EXPECT_EQ(applyTimezoneOffset(5, 3), 8);    // 5 UTC + 3h = 8 local
}

TEST_F(ConfigLogicTest, TimezoneOffset_PositiveOffset_WithWrap) {
    EXPECT_EQ(applyTimezoneOffset(23, 1), 0);   // 23 UTC + 1h = 0 next day
    EXPECT_EQ(applyTimezoneOffset(22, 5), 3);   // 22 UTC + 5h = 3 next day
    EXPECT_EQ(applyTimezoneOffset(20, 8), 4);   // 20 UTC + 8h = 4 next day
}

TEST_F(ConfigLogicTest, TimezoneOffset_NegativeOffset_NoWrap) {
    EXPECT_EQ(applyTimezoneOffset(12, -2), 10); // 12 UTC - 2h = 10 local
    EXPECT_EQ(applyTimezoneOffset(8, -3), 5);   // 8 UTC - 3h = 5 local
}

TEST_F(ConfigLogicTest, TimezoneOffset_NegativeOffset_WithWrap) {
    EXPECT_EQ(applyTimezoneOffset(0, -1), 23);  // 0 UTC - 1h = 23 prev day
    EXPECT_EQ(applyTimezoneOffset(3, -5), 22);  // 3 UTC - 5h = 22 prev day
    EXPECT_EQ(applyTimezoneOffset(2, -8), 18);  // 2 UTC - 8h = 18 prev day
}

TEST_F(ConfigLogicTest, TimezoneOffset_Extremes) {
    // UTC+14 (Kiribati)
    EXPECT_EQ(applyTimezoneOffset(12, 14), 2);  // Wraps to next day
    
    // UTC-12 (Baker Island)
    EXPECT_EQ(applyTimezoneOffset(10, -12), 22); // Wraps to prev day
    
    // Multiple wraps
    EXPECT_EQ(applyTimezoneOffset(23, 25), 0);  // 23 + 25 = 48 → 0
    EXPECT_EQ(applyTimezoneOffset(1, -26), 23); // 1 - 26 = -25 → 23
}

TEST_F(ConfigLogicTest, TimezoneOffset_AllHours_PositiveOffset) {
    // Verify all hours wrap correctly with +5 offset
    for (int hour = 0; hour < 24; hour++) {
        int result = applyTimezoneOffset(hour, 5);
        EXPECT_GE(result, 0);
        EXPECT_LT(result, 24);
        EXPECT_EQ(result, (hour + 5) % 24);
    }
}

TEST_F(ConfigLogicTest, TimezoneOffset_AllHours_NegativeOffset) {
    // Verify all hours wrap correctly with -5 offset
    for (int hour = 0; hour < 24; hour++) {
        int result = applyTimezoneOffset(hour, -5);
        EXPECT_GE(result, 0);
        EXPECT_LT(result, 24);
        int expected = (hour - 5 + 24) % 24;
        EXPECT_EQ(result, expected);
    }
}

// ============================================================================
// Hour Enabled in Bitmask Tests
// ============================================================================

TEST_F(ConfigLogicTest, HourBitmask_AllDisabled_ReturnsFalse) {
    uint8_t bitmask[3] = {0x00, 0x00, 0x00};
    
    for (int hour = 0; hour < 24; hour++) {
        EXPECT_FALSE(isHourEnabledInBitmask(hour, bitmask)) << "Hour " << hour;
    }
}

TEST_F(ConfigLogicTest, HourBitmask_AllEnabled_ReturnsTrue) {
    uint8_t bitmask[3] = {0xFF, 0xFF, 0xFF};
    
    for (int hour = 0; hour < 24; hour++) {
        EXPECT_TRUE(isHourEnabledInBitmask(hour, bitmask)) << "Hour " << hour;
    }
}

TEST_F(ConfigLogicTest, HourBitmask_SingleHours) {
    uint8_t bitmask[3];
    
    // Test each hour individually
    for (int targetHour = 0; targetHour < 24; targetHour++) {
        setBitmask(bitmask, {targetHour});
        
        for (int checkHour = 0; checkHour < 24; checkHour++) {
            if (checkHour == targetHour) {
                EXPECT_TRUE(isHourEnabledInBitmask(checkHour, bitmask)) 
                    << "Hour " << checkHour << " should be enabled";
            } else {
                EXPECT_FALSE(isHourEnabledInBitmask(checkHour, bitmask))
                    << "Hour " << checkHour << " should be disabled";
            }
        }
    }
}

TEST_F(ConfigLogicTest, HourBitmask_BusinessHours) {
    uint8_t bitmask[3];
    setBitmask(bitmask, {9, 10, 11, 12, 13, 14, 15, 16, 17}); // 9 AM - 5 PM
    
    EXPECT_TRUE(isHourEnabledInBitmask(9, bitmask));
    EXPECT_TRUE(isHourEnabledInBitmask(12, bitmask));
    EXPECT_TRUE(isHourEnabledInBitmask(17, bitmask));
    
    EXPECT_FALSE(isHourEnabledInBitmask(8, bitmask));
    EXPECT_FALSE(isHourEnabledInBitmask(18, bitmask));
    EXPECT_FALSE(isHourEnabledInBitmask(0, bitmask));
}

TEST_F(ConfigLogicTest, HourBitmask_NightHours) {
    uint8_t bitmask[3];
    setBitmask(bitmask, {22, 23, 0, 1, 2, 3}); // 10 PM - 3 AM
    
    EXPECT_TRUE(isHourEnabledInBitmask(22, bitmask));
    EXPECT_TRUE(isHourEnabledInBitmask(0, bitmask));
    EXPECT_TRUE(isHourEnabledInBitmask(3, bitmask));
    
    EXPECT_FALSE(isHourEnabledInBitmask(12, bitmask));
    EXPECT_FALSE(isHourEnabledInBitmask(4, bitmask));
}

TEST_F(ConfigLogicTest, HourBitmask_EveryOtherHour) {
    uint8_t bitmask[3];
    setBitmask(bitmask, {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22});
    
    for (int hour = 0; hour < 24; hour++) {
        if (hour % 2 == 0) {
            EXPECT_TRUE(isHourEnabledInBitmask(hour, bitmask)) << "Hour " << hour;
        } else {
            EXPECT_FALSE(isHourEnabledInBitmask(hour, bitmask)) << "Hour " << hour;
        }
    }
}

TEST_F(ConfigLogicTest, HourBitmask_BoundaryHours) {
    uint8_t bitmask[3];
    setBitmask(bitmask, {0, 7, 8, 15, 16, 23}); // Byte boundaries
    
    EXPECT_TRUE(isHourEnabledInBitmask(0, bitmask));   // First bit of byte 0
    EXPECT_TRUE(isHourEnabledInBitmask(7, bitmask));   // Last bit of byte 0
    EXPECT_TRUE(isHourEnabledInBitmask(8, bitmask));   // First bit of byte 1
    EXPECT_TRUE(isHourEnabledInBitmask(15, bitmask));  // Last bit of byte 1
    EXPECT_TRUE(isHourEnabledInBitmask(16, bitmask));  // First bit of byte 2
    EXPECT_TRUE(isHourEnabledInBitmask(23, bitmask));  // Last bit of byte 2
}

TEST_F(ConfigLogicTest, HourBitmask_InvalidHours_ReturnFalse) {
    uint8_t bitmask[3] = {0xFF, 0xFF, 0xFF};
    
    EXPECT_FALSE(isHourEnabledInBitmask(-1, bitmask));
    EXPECT_FALSE(isHourEnabledInBitmask(24, bitmask));
    EXPECT_FALSE(isHourEnabledInBitmask(100, bitmask));
}

// ============================================================================
// All Hours Enabled Tests
// ============================================================================

TEST_F(ConfigLogicTest, AllHoursEnabled_AllSet_ReturnsTrue) {
    uint8_t bitmask[3] = {0xFF, 0xFF, 0xFF};
    EXPECT_TRUE(areAllHoursEnabled(bitmask));
}

TEST_F(ConfigLogicTest, AllHoursEnabled_AllClear_ReturnsFalse) {
    uint8_t bitmask[3] = {0x00, 0x00, 0x00};
    EXPECT_FALSE(areAllHoursEnabled(bitmask));
}

TEST_F(ConfigLogicTest, AllHoursEnabled_OneBitMissing_ReturnsFalse) {
    // All set except hour 12
    uint8_t bitmask[3];
    for (int i = 0; i < 3; i++) bitmask[i] = 0xFF;
    
    bitmask[12/8] &= ~(1 << (12%8)); // Clear bit for hour 12
    
    EXPECT_FALSE(areAllHoursEnabled(bitmask));
}

TEST_F(ConfigLogicTest, AllHoursEnabled_PartialBytes) {
    uint8_t bitmask1[3] = {0xFF, 0x00, 0xFF};
    EXPECT_FALSE(areAllHoursEnabled(bitmask1));
    
    uint8_t bitmask2[3] = {0x00, 0xFF, 0xFF};
    EXPECT_FALSE(areAllHoursEnabled(bitmask2));
    
    uint8_t bitmask3[3] = {0xFF, 0xFF, 0x00};
    EXPECT_FALSE(areAllHoursEnabled(bitmask3));
}

TEST_F(ConfigLogicTest, AllHoursEnabled_AlmostAll) {
    uint8_t bitmask[3] = {0xFF, 0xFF, 0xFE}; // All except hour 16
    EXPECT_FALSE(areAllHoursEnabled(bitmask));
}

// ============================================================================
// Integration Tests (combining functions)
// ============================================================================

TEST_F(ConfigLogicTest, Integration_TimezoneAwareBitmaskCheck) {
    // Business hours in UTC-5 (9 AM - 5 PM EST)
    uint8_t bitmask[3];
    setBitmask(bitmask, {14, 15, 16, 17, 18, 19, 20, 21, 22}); // 14-22 UTC
    
    // Check if 10 AM EST (15 UTC) is enabled
    int utcHour = 15;
    EXPECT_TRUE(isHourEnabledInBitmask(utcHour, bitmask));
    
    // Check local time display (EST = UTC-5)
    int estHour = applyTimezoneOffset(utcHour, -5);
    EXPECT_EQ(estHour, 10);
}

TEST_F(ConfigLogicTest, Integration_CrossMidnightSchedule) {
    // Night schedule: 10 PM - 2 AM local (UTC+2)
    // In UTC: 20-22 (current day) and 0-2 (next day)
    uint8_t bitmask[3];
    setBitmask(bitmask, {20, 21, 22, 23, 0, 1});
    
    // 11 PM local time in UTC+2
    int localHour = 23;
    int utcHour = applyTimezoneOffset(localHour, -2); // Convert to UTC
    EXPECT_EQ(utcHour, 21);
    EXPECT_TRUE(isHourEnabledInBitmask(utcHour, bitmask));
    
    // 1 AM local time in UTC+2
    localHour = 1;
    utcHour = applyTimezoneOffset(localHour, -2); // Convert to UTC
    EXPECT_EQ(utcHour, 23);
    EXPECT_TRUE(isHourEnabledInBitmask(utcHour, bitmask));
}
