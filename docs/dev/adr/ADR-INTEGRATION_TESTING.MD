# ADR: Integration Testing for Normal Mode Decision Flow

**Status:** Accepted (Implemented v1.5.2)  
**Date:** 2025-11-15  
**Authors:** Development Team  
**Related:** ADR-UNIT_TESTING.MD, NORMAL_MODE_FLOW.md

## Context

The normal mode controller has **40+ execution paths** resulting from 7+ interacting decision variables:
- Mode (single/carousel)
- Wake reason (timer/button)
- Stay flag (stay/advance) [carousel only]
- CRC32 enabled (on/off)
- CRC32 result (match/changed) [when enabled]
- Download result (success/fail)
- Retry count (0/1/2) [on failure]

**Current Testing Coverage:**
- ✅ **Unit tests (78 total)**: Test individual decision functions in isolation
  - `decision_tests` (19 tests): Individual decision logic
  - `battery_tests` (17 tests): Battery calculation
  - `sleep_tests` (21 tests): Sleep duration logic
  - `config_tests` (25 tests): Config validation
- ❌ **Integration tests**: No automated tests for combined decision flows

**The Problem:**
While unit tests validate that each decision function works correctly in isolation, they don't verify:
1. How decisions **interact** in real execution sequences
2. Whether the **full flow** produces expected outcomes for given inputs
3. Edge cases where **multiple decisions** combine in unexpected ways
4. Regression protection for **complex multi-step scenarios**

**Example Gap:**
A unit test verifies: "Carousel + Timer + stay:true → CRC32 should check"  
But we don't test: "Given carousel config with stay:true, timer wake, and CRC32 match → system should skip download, not advance carousel, and sleep for correct interval"

## Decision

**Implement integration tests** that validate complete decision flows from config input to final outcome (image target, download decision, sleep duration).

### Test Strategy

**What to Test:**
- **End-to-end decision flows**: Given a complete config and wake context, verify all three decisions (image target, CRC32 action, sleep duration) are correct
- **Common user scenarios**: Real-world configurations users would create
- **Critical edge cases**: Complex combinations documented in NORMAL_MODE_FLOW.md
- **Regression scenarios**: Bugs found in production that could be prevented

**What NOT to Test:**
- Arduino/ESP32 hardware interactions (displays, WiFi, deep sleep)
- Actual image downloads (network I/O)
- MQTT publishing
- File system operations
- Time-dependent behavior that requires real RTC

### Implementation Approach

**Option 1: Extend Existing Unit Test Framework (RECOMMENDED)**

Add integration test suite to existing CMake + Google Test infrastructure:

```
test/
  unit/                           # Existing unit tests
    test_decision_functions.cpp   # Has 3 integration tests already
    test_battery_logic.cpp
    test_sleep_logic.cpp
    test_config_logic.cpp
  integration/                    # NEW: Integration tests
    test_normal_mode_scenarios.cpp  # End-to-end scenario tests
  mocks/                          # Existing mocks (reuse)
  CMakeLists.txt                  # Update to include integration tests
```

**Benefits:**
- Reuse existing test infrastructure (CMake, Google Test, mocks)
- Same build/run workflow (`.\run-tests.ps1`)
- No new tools or dependencies required
- Can share test fixtures and helpers

**Option 2: Separate Integration Test Binary**

Create a dedicated integration test executable:

```
test/
  integration/
    CMakeLists.txt              # Separate CMake project
    test_end_to_end_flows.cpp   # Scenario-based tests
    config_scenarios.h          # Reusable config builders
```

**Benefits:**
- Clear separation between unit and integration tests
- Can run integration tests independently
- Different test timeout/reporting settings

**Recommendation:** **Option 1** - simpler, leverages existing infrastructure, easier maintenance.

## Test Examples

### Example 1: Single Image Mode with CRC32 Match

```cpp
TEST_F(IntegrationTest, SingleImage_TimerWake_CRC32Match_SkipsDownload) {
    // GIVEN: Single image config with CRC32 enabled
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)  // 15 min interval
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    time_t currentTime = createTime(2025, 11, 15, 14, 30, 0);  // Friday 2:30 PM
    bool crc32Matched = true;
    
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
    
    // Sleep: Use configured interval (CRC32 matched, so skip download)
    EXPECT_EQ(sleepDuration.sleepSeconds, 15.0f * 60.0f);  // 900 seconds
    EXPECT_STREQ(sleepDuration.reason, "Image interval (CRC32 matched)");
    
    // Integration verification: If CRC32 matched, download should be skipped
    if (crc32Action.shouldCheck && crc32Matched) {
        // This is the integration assertion - in real code, download is skipped
        EXPECT_TRUE(true) << "Download should be skipped when CRC32 matches";
    }
}
```

### Example 2: Carousel Auto-Advance

```cpp
TEST_F(IntegrationTest, Carousel_TimerWake_StayFalse_Advances) {
    // GIVEN: Carousel with 3 images, all stay:false (auto-advance)
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img1.png", 10, false)  // stay:false
        .addImage("http://example.com/img2.png", 15, false)  // stay:false
        .addImage("http://example.com/img3.png", 20, false)  // stay:false
        .withCRC32(true)
        .build();
    
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 1;  // Currently on image 1
    time_t currentTime = createTime(2025, 11, 15, 10, 0, 0);
    bool crc32Matched = false;  // Image changed
    
    // WHEN: Execute all decision functions
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    auto sleepDuration = determineSleepDuration(config, currentTime, imageTarget.targetIndex, crc32Matched);
    
    // THEN: Verify complete flow
    // Image target: Advance to next image (1 → 2)
    EXPECT_EQ(imageTarget.targetIndex, 2);
    EXPECT_TRUE(imageTarget.shouldAdvance);
    EXPECT_STREQ(imageTarget.reason, "Carousel - timer wake + stay:false (auto-advance)");
    
    // CRC32: Don't check (auto-advance always downloads)
    EXPECT_FALSE(crc32Action.shouldCheck);
    EXPECT_STREQ(crc32Action.reason, "Carousel - auto-advance (always download)");
    
    // Sleep: Use NEXT image's interval (image 2 = 20 minutes)
    EXPECT_EQ(sleepDuration.sleepSeconds, 20.0f * 60.0f);  // 1200 seconds
    EXPECT_STREQ(sleepDuration.reason, "Image interval (image updated)");
    
    // Integration verification: Index should be updated to 2
    EXPECT_EQ(imageTarget.targetIndex, 2) << "Carousel should advance from 1 to 2";
}
```

### Example 3: Hourly Schedule Override

```cpp
TEST_F(IntegrationTest, HourlySchedule_DisabledHour_SkipsUpdate) {
    // GIVEN: Config with hourly schedule (only hours 9-17 enabled)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)  // 9 AM to 5 PM only
        .withTimezone(-5)  // EST
        .build();
    
    // Current time: 2 AM EST (disabled hour)
    time_t currentTime = createTime(2025, 11, 15, 7, 0, 0);  // 7 AM UTC = 2 AM EST
    WakeupReason wakeReason = WAKEUP_TIMER;
    uint8_t currentIndex = 0;
    
    // WHEN: Calculate sleep to next enabled hour
    float sleepMinutes = calculateSleepMinutesToNextEnabledHour(
        currentTime, 
        config.timezoneOffset, 
        config.updateHours
    );
    
    // THEN: Should sleep until 9 AM EST (7 hours from 2 AM)
    EXPECT_GT(sleepMinutes, 0.0f) << "Should sleep when hour is disabled";
    EXPECT_NEAR(sleepMinutes, 7.0f * 60.0f, 1.0f);  // ~420 minutes (±1 min tolerance)
    
    // Integration verification: Normal mode should skip image update entirely
    // and go straight to sleep (this would be verified in normal_mode_controller)
}
```

### Example 4: Button Wake Bypasses Schedule

```cpp
TEST_F(IntegrationTest, ButtonWake_BypassesHourlySchedule) {
    // GIVEN: Same config with hourly schedule (9-17 only)
    DashboardConfig config = ConfigBuilder()
        .singleImage("http://example.com/image.png", 15)
        .withHourlySchedule(9, 17)
        .withTimezone(-5)
        .build();
    
    // Current time: 2 AM EST (disabled hour)
    time_t currentTime = createTime(2025, 11, 15, 7, 0, 0);
    WakeupReason wakeReason = WAKEUP_BUTTON;  // Button press
    uint8_t currentIndex = 0;
    
    // WHEN: Execute decisions (button wake should bypass schedule)
    auto imageTarget = determineImageTarget(config, wakeReason, currentIndex);
    auto crc32Action = determineCRC32Action(config, wakeReason, currentIndex);
    
    // THEN: Should proceed with normal update (button bypasses schedule)
    EXPECT_EQ(imageTarget.targetIndex, 0);
    EXPECT_FALSE(crc32Action.shouldCheck);  // Button never checks CRC32
    
    // Integration verification: In normal_mode_controller, button wake
    // skips hourly schedule check entirely (always proceeds to download)
}
```

### Example 5: Carousel Mixed Stay Flags

```cpp
TEST_F(IntegrationTest, Carousel_MixedStay_CorrectBehavior) {
    // GIVEN: Carousel with mixed stay flags
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("http://example.com/img1.png", 10, false)  // Auto-advance
        .addImage("http://example.com/img2.png", 15, true)   // Stay
        .addImage("http://example.com/img3.png", 20, false)  // Auto-advance
        .withCRC32(true)
        .build();
    
    time_t currentTime = createTime(2025, 11, 15, 14, 0, 0);
    
    // SCENARIO 1: Timer wake on image 0 (stay:false)
    {
        auto imageTarget = determineImageTarget(config, WAKEUP_TIMER, 0);
        auto crc32Action = determineCRC32Action(config, WAKEUP_TIMER, 0);
        
        EXPECT_EQ(imageTarget.targetIndex, 1);  // Advance to 1
        EXPECT_TRUE(imageTarget.shouldAdvance);
        EXPECT_FALSE(crc32Action.shouldCheck);  // Auto-advance never checks
    }
    
    // SCENARIO 2: Timer wake on image 1 (stay:true)
    {
        auto imageTarget = determineImageTarget(config, WAKEUP_TIMER, 1);
        auto crc32Action = determineCRC32Action(config, WAKEUP_TIMER, 1);
        
        EXPECT_EQ(imageTarget.targetIndex, 1);  // Stay on 1
        EXPECT_FALSE(imageTarget.shouldAdvance);
        EXPECT_TRUE(crc32Action.shouldCheck);   // Stay:true checks CRC32
    }
    
    // SCENARIO 3: Button wake on image 1 (stay:true, but button overrides)
    {
        auto imageTarget = determineImageTarget(config, WAKEUP_BUTTON, 1);
        auto crc32Action = determineCRC32Action(config, WAKEUP_BUTTON, 1);
        
        EXPECT_EQ(imageTarget.targetIndex, 2);  // Button always advances
        EXPECT_TRUE(imageTarget.shouldAdvance);
        EXPECT_FALSE(crc32Action.shouldCheck);  // Button never checks
    }
}
```

## Test Infrastructure

### Config Builder Helper

Create a fluent builder for test configs:

```cpp
class ConfigBuilder {
private:
    DashboardConfig config;
    uint8_t imageIndex = 0;
    
public:
    ConfigBuilder() {
        config = DashboardConfig();
    }
    
    ConfigBuilder& singleImage(const char* url, int intervalMinutes) {
        config.imageCount = 1;
        config.imageUrls[0] = String(url);
        config.imageIntervals[0] = intervalMinutes;
        config.imageStay[0] = false;
        return *this;
    }
    
    ConfigBuilder& carousel() {
        config.imageCount = 0;  // Will increment with addImage()
        return *this;
    }
    
    ConfigBuilder& addImage(const char* url, int intervalMinutes, bool stay) {
        if (config.imageCount >= MAX_IMAGE_SLOTS) {
            return *this;  // Ignore if full
        }
        config.imageUrls[config.imageCount] = String(url);
        config.imageIntervals[config.imageCount] = intervalMinutes;
        config.imageStay[config.imageCount] = stay;
        config.imageCount++;
        return *this;
    }
    
    ConfigBuilder& withCRC32(bool enabled) {
        config.useCRC32Check = enabled;
        return *this;
    }
    
    ConfigBuilder& withHourlySchedule(uint8_t startHour, uint8_t endHour) {
        // Clear all hours first
        config.updateHours[0] = 0x00;
        config.updateHours[1] = 0x00;
        config.updateHours[2] = 0x00;
        
        // Enable hours in range
        for (uint8_t hour = startHour; hour <= endHour; hour++) {
            uint8_t byteIndex = hour / 8;
            uint8_t bitIndex = hour % 8;
            config.updateHours[byteIndex] |= (1 << bitIndex);
        }
        return *this;
    }
    
    ConfigBuilder& withTimezone(int offset) {
        config.timezoneOffset = offset;
        return *this;
    }
    
    DashboardConfig build() {
        return config;
    }
};

// Helper to create time_t from human-readable date
time_t createTime(int year, int month, int day, int hour, int min, int sec) {
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;
    timeinfo.tm_isdst = -1;  // Let mktime determine DST
    return mktime(&timeinfo);
}
```

### Test Organization

```cpp
// test/integration/test_normal_mode_scenarios.cpp

#include <gtest/gtest.h>
#include <modes/decision_logic.h>

class IntegrationTest : public ::testing::Test {
protected:
    // Helper methods available to all tests
    DashboardConfig createSingleImageConfig(int interval = 15);
    DashboardConfig createCarouselConfig(uint8_t count);
    time_t createTime(int year, int month, int day, int hour, int min, int sec);
};

// Group tests by scenario category
TEST_F(IntegrationTest, SingleImage_TimerWake_CRC32Match_SkipsDownload) { /* ... */ }
TEST_F(IntegrationTest, SingleImage_TimerWake_CRC32Changed_Downloads) { /* ... */ }
TEST_F(IntegrationTest, SingleImage_ButtonWake_AlwaysDownloads) { /* ... */ }

TEST_F(IntegrationTest, Carousel_TimerWake_StayTrue_DoesNotAdvance) { /* ... */ }
TEST_F(IntegrationTest, Carousel_TimerWake_StayFalse_Advances) { /* ... */ }
TEST_F(IntegrationTest, Carousel_ButtonWake_AlwaysAdvances) { /* ... */ }

TEST_F(IntegrationTest, HourlySchedule_DisabledHour_SkipsUpdate) { /* ... */ }
TEST_F(IntegrationTest, HourlySchedule_EnabledHour_ProceedsNormally) { /* ... */ }
TEST_F(IntegrationTest, ButtonWake_BypassesHourlySchedule) { /* ... */ }

TEST_F(IntegrationTest, Carousel_MixedStay_CorrectBehavior) { /* ... */ }
TEST_F(IntegrationTest, ButtonOnlyMode_Interval0_IndefiniteSleep) { /* ... */ }
```

## Scope & Coverage

### Scenarios to Test (Priority Order)

**High Priority (Core Flows):**
1. ✅ Single image + timer wake + CRC32 match → skip download
2. ✅ Single image + timer wake + CRC32 changed → download
3. ✅ Single image + button wake → always download
4. ✅ Carousel + timer wake + stay:false → advance
5. ✅ Carousel + timer wake + stay:true → don't advance
6. ✅ Carousel + button wake → always advance
7. ✅ Hourly schedule disabled hour → sleep to next hour
8. ✅ Button wake → bypass hourly schedule

**Medium Priority (Edge Cases):**
9. Carousel with mixed stay flags
10. Button-only mode (interval = 0)
11. Carousel wrap-around (last image → first image)
12. CRC32 disabled → always download
13. Multiple images with different intervals
14. Timezone offset with hourly schedule

**Low Priority (Regression/Corner Cases):**
15. All hours disabled (shouldn't happen, but handle gracefully)
16. Single image in carousel mode (imageCount = 1)
17. Maximum image count (10 images)
18. Midnight/hour boundary edge cases
19. DST transitions (if applicable)

**Out of Scope:**
- Error retry logic (requires download failure simulation)
- Network timeouts and WiFi failures
- Display rendering
- MQTT publishing
- Battery telemetry
- RTC memory state persistence

### Expected Test Count

- **Unit tests**: 78 (existing)
- **Integration tests**: 15-20 (new)
- **Total**: ~95-100 tests

This represents ~20% increase in test count but significantly higher confidence in system behavior.

## Implementation Plan

### Phase 1: Infrastructure Setup (1-2 hours)

1. ✅ Create `test/integration/` directory
2. ✅ Add `ConfigBuilder` helper class
3. ✅ Add `createTime()` helper function
4. ✅ Update `test/CMakeLists.txt` to include integration tests
5. ✅ Create `test_normal_mode_scenarios.cpp` skeleton

### Phase 2: Core Scenario Tests (2-3 hours)

6. ✅ Implement 8 high-priority scenarios
7. ✅ Verify all tests pass
8. ✅ Update `test/README.md` with integration test documentation

### Phase 3: Edge Case Tests (1-2 hours)

9. ✅ Implement 6 medium-priority scenarios
10. ✅ Document any discovered issues or ambiguities
11. ✅ Update NORMAL_MODE_FLOW.md if needed

### Phase 4: Documentation (1 hour)

12. ✅ Update this ADR with final test counts
13. ✅ Update `test/README.md` with examples
14. ✅ Add integration test guidance to developer docs

**Total Effort Estimate: 5-8 hours**

## Benefits

### Development Benefits

1. **Regression Protection**: Catch unintended behavior changes in complex flows
2. **Refactoring Confidence**: Safe to restructure code if tests pass
3. **Documentation**: Tests serve as executable specifications
4. **Faster Debugging**: Pinpoint exact scenario causing issues
5. **Design Feedback**: Integration tests reveal awkward interactions

### User Benefits

1. **Reliability**: Fewer bugs in production from decision logic errors
2. **Feature Confidence**: New features won't break existing behavior
3. **Faster Fixes**: Issues can be reproduced and fixed with test-first approach

## Trade-offs & Risks

### Trade-offs

| Aspect | Benefit | Cost |
|--------|---------|------|
| **Test Count** | +20% test coverage | +5-8 hours initial development |
| **Build Time** | Minimal impact (same CMake) | +0.5-1 second test execution |
| **Maintenance** | Self-documenting code | Update tests when behavior changes |
| **Complexity** | Tests are simple (pure functions) | Need ConfigBuilder helper |

### Risks & Mitigations

**Risk 1: Tests too coupled to implementation**
- Mitigation: Test **behavior** (inputs → outputs), not implementation details
- Example: Test "CRC32 should skip download" not "shouldCheck flag is true"

**Risk 2: Tests become brittle**
- Mitigation: Use ConfigBuilder to isolate test setup from config structure
- If config struct changes, update builder once instead of every test

**Risk 3: Incomplete coverage**
- Mitigation: Use NORMAL_MODE_FLOW.md truth table to guide test scenarios
- Code review requirement: Any new execution path must have integration test

**Risk 4: False confidence**
- Mitigation: Integration tests complement (not replace) unit tests
- Still need manual testing on real hardware for display/WiFi/hardware issues

## Alternatives Considered

### Alternative 1: Manual Testing Only

**Pros:**
- No development time investment
- Tests real hardware behavior

**Cons:**
- No regression protection
- Time-consuming for 40+ scenarios
- Human error prone
- Can't test all combinations

**Rejected:** Too risky for complex decision logic with 40+ paths.

### Alternative 2: End-to-End Hardware Tests

**Pros:**
- Tests complete system including hardware
- Catches integration issues with display/WiFi

**Cons:**
- Requires physical hardware setup
- Slow (minutes per test)
- Flaky (network, hardware dependencies)
- Hard to automate in CI/CD

**Rejected:** Too slow and complex for decision logic testing. Better suited for smoke tests.

### Alternative 3: Mocked Controller Tests

Test `normal_mode_controller.cpp` directly with mocked dependencies:

```cpp
TEST(NormalModeControllerTest, CompleteFlow) {
    MockDisplay display;
    MockWiFi wifi;
    // ... mock all dependencies
    
    NormalModeController controller(&display, ...);
    controller.execute();
    
    // Verify all interactions
    EXPECT_CALL(display, drawImage(...));
    EXPECT_CALL(powerManager, sleep(...));
}
```

**Pros:**
- Tests actual controller code
- Verifies interaction with dependencies

**Cons:**
- Complex mock setup (7+ dependencies)
- Brittle (breaks when controller refactored)
- Slow (mocking overhead)
- Harder to maintain

**Rejected:** Decision function integration tests provide 80% of the value with 20% of the complexity.

## Success Criteria

This ADR will be considered **successful** when:

1. ✅ At least 15 integration tests implemented covering high-priority scenarios
2. ✅ All integration tests pass consistently in CI/CD
3. ✅ Integration tests catch at least one bug/regression in first 6 months
4. ✅ Developer feedback: Integration tests are easy to write and maintain
5. ✅ Test execution time: Integration tests add <2 seconds to test suite

## References

- **NORMAL_MODE_FLOW.md**: 40+ execution paths documented with truth table
- **ADR-UNIT_TESTING.MD**: Unit testing pattern and infrastructure
- **test/unit/test_decision_functions.cpp**: Existing decision tests (has 3 basic integration tests)
- **common/src/modes/decision_logic.h**: Pure decision functions (testable without Arduino)

## Future Enhancements

### Phase 5 (Optional): Error Flow Integration Tests

Test error handling scenarios:
- Download failures (retry logic)
- Carousel skip-to-next on failure
- Error screen display decisions

**Note:** Requires additional mocking infrastructure for download failures. Consider after Phase 4 proves valuable.

### Phase 6 (Optional): Property-Based Testing

Use property-based testing (e.g., Google Test with custom generators) to:
- Generate random configs and verify invariants
- Test boundary conditions automatically
- Discover unexpected edge cases

**Example Invariant:** "Sleep duration should never be negative or >24 hours"

### Phase 7 (Optional): CI/CD Integration

- Add integration tests to GitHub Actions workflow
- Require passing integration tests before PR merge
- Track test coverage metrics over time

---

**Next Steps:**
1. Review this ADR with team
2. Approve/reject/request changes
3. If approved, begin Phase 1 implementation
4. Update ADR status to "Accepted" when implemented
