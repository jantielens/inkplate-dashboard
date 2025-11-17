# Unit & Integration Tests

This directory contains unit and integration tests for the inkplate-dashboard firmware using CMake and Google Test.

## Overview

The test suite validates **standalone pure C++ functions** extracted from the firmware:

### Unit Tests

#### Decision Logic
Core decision functions from `normal_mode_controller.cpp`:
- `determineImageTarget()` - Which image to display and whether to advance
- `determineCRC32Action()` - Whether to check CRC32 for optimization
- `determineSleepDuration()` - How long to sleep until next wake

Validates individual decisions in isolation.

#### Battery Logic
Battery percentage calculation from `power_manager.cpp`:
- `calculateBatteryPercentage()` - Li-ion voltage to percentage conversion
- 22-point discharge curve with linear interpolation
- 5% rounding for stability

### Sleep Logic
Sleep duration compensation from `power_manager.cpp`:
- `calculateAdjustedSleepDuration()` - Active loop time compensation
- Button-only mode handling
- Microsecond precision timing

### Config Logic
Configuration validation helpers from `config_manager.cpp`:
- `applyTimezoneOffset()` - 24-hour wrapping with timezone offsets
- `isHourEnabledInBitmask()` - Hour-based scheduling validation
- `areAllHoursEnabled()` - 24/7 schedule detection

### Integration Tests

End-to-end scenario tests that validate **complete decision flows** for real-world configurations:
- Single image mode with CRC32 optimization
- Carousel auto-advance and stay behavior
- Button wake bypassing hourly schedules
- Carousel wrap-around (last to first image)
- Button-only mode (interval = 0)
- Carousel edge cases (CRC32 disabled, single-image carousel, boundary conditions)
- Hourly schedule integration (calculateSleepMinutesToNextEnabledHour validation)
- End-to-end orchestration scenarios (orchestrateNormalModeDecisions validation)

Integration tests verify that multiple decision functions work correctly together to produce expected system behavior, covering **40+ execution paths** documented in [NORMAL_MODE_FLOW.md](../docs/dev/NORMAL_MODE_FLOW.md).

## Test Architecture

### Standalone Function Architecture

Functions are extracted into **standalone pure C++ modules** in `common/src/`. Tests compile and validate REAL production code without Arduino/ESP32 dependencies using minimal mocks.

```
test/
├── unit/
│   ├── test_decision_functions.cpp     # Decision logic tests
│   ├── test_battery_logic.cpp          # Battery calculation tests
│   ├── test_sleep_logic.cpp            # Sleep duration tests
│   └── test_config_logic.cpp           # Config validation tests
├── integration/
│   ├── test_normal_mode_scenarios.cpp  # End-to-end scenario tests
│   └── test_helpers.h                  # ConfigBuilder and test utilities
├── mocks/
│   ├── Arduino.h                       # Mock Arduino types (String, millis, etc.)
│   ├── Inkplate.h                      # Mock Inkplate display class
│   ├── config.h                        # Mock DashboardConfig struct and types
│   ├── config_manager.cpp              # Mock ConfigManager (delegates to config_logic)
│   └── config_manager.h                # Prevent Arduino Preferences.h include
├── CMakeLists.txt                      # CMake build configuration (5 test executables)
└── run-tests.ps1                       # PowerShell test runner

common/src/
├── battery_logic.h/cpp                 # Battery percentage calculation
├── sleep_logic.h/cpp                   # Sleep duration compensation
├── config_logic.h/cpp                  # Config validation helpers
└── modes/
    ├── decision_logic.h/cpp            # Normal mode decision functions
    └── ...                             # Other mode controllers
```

**Key Principle:** Tests compile and run the ACTUAL production code from `decision_logic.cpp`. No manual synchronization needed - when you change logic, tests automatically validate the new behavior.

## Prerequisites

- **CMake 4.1.2+** (installed via `winget install --id Kitware.CMake`)
- **Visual Studio 2022 Build Tools** (or full VS 2022)
- **Google Test 1.12.1** (fetched automatically by CMake)

## Running Tests

### Windows (PowerShell)

**Run all tests:**
```powershell
.\run-tests.ps1
```

**Clean build and run:**
```powershell
.\run-tests.ps1 -Clean
```

**Verbose output:**
```powershell
.\run-tests.ps1 -Verbose
```

### Linux/macOS (Bash)

**Run all tests:**
```bash
chmod +x run-tests.sh  # First time only
./run-tests.sh
```

**Clean build and run:**
```bash
./run-tests.sh --clean
```

## Code Coverage

### Windows (PowerShell)

**Prerequisites:**
- OpenCppCoverage - Install via: `winget install OpenCppCoverage.OpenCppCoverage`
- Or download from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases

**Run coverage:**
```powershell
.\run-coverage.ps1
```

**Auto-install and run:**
```powershell
.\run-coverage.ps1 -Install
```

**Clean build:**
```powershell
.\run-coverage.ps1 -Clean
```

**Output:**
- HTML report: `test/coverage/index.html` (opens automatically)
- XML report: `test/coverage/coverage.xml` (Cobertura format)

### Linux/macOS (Bash)

**Prerequisites:**
- lcov - Install via:
  - Ubuntu/Debian: `sudo apt-get install lcov`
  - macOS: `brew install lcov`
  - Fedora: `sudo dnf install lcov`

**Run coverage:**
```bash
chmod +x run-coverage.sh  # First time only
./run-coverage.sh
```

**Auto-install and run:**
```bash
./run-coverage.sh --install
```

**Clean build:**
```bash
./run-coverage.sh --clean
```

**Output:**
- HTML report: `test/coverage/index.html` (opens automatically)
- LCOV report: `test/build/coverage_filtered.info`

### Coverage Reports

Coverage reports show:
- **Line coverage** - Which lines of code are executed by tests
- **Function coverage** - Which functions are called by tests
- **Branch coverage** - Which code branches (if/else) are tested

**What's covered:**
- `common/src/modes/decision_logic.cpp` - Normal mode decision functions
- `common/src/battery_logic.cpp` - Battery percentage calculation
- `common/src/sleep_logic.cpp` - Sleep duration compensation
- `common/src/config_logic.cpp` - Configuration validation helpers

**What's excluded:**
- Test files (`test/unit/*`, `test/integration/*`)
- Mock files (`test/mocks/*`)
- Google Test framework
- System headers

### Expected Output

```
=== Inkplate Dashboard Unit Test Runner ===

CMake version:
  cmake version 4.1.2

Building tests...
[... build output ...]

Running tests...
[==========] Running tests...
[==========] All tests passed

Total Test time (real) = 1.15 sec

✅ All tests passed!
```

## Test Coverage

### Unit Tests

#### Decision Logic Tests

**Image Target Tests:**
- Single mode always returns index 0
- Carousel button wake always advances
- Carousel timer wake with `stay:false` advances
- Carousel timer wake with `stay:true` stays on current
- Mixed stay flags respected per-image

**CRC32 Action Tests:**
- Disabled in config never checks
- Single mode timer wake checks
- Single mode button wake does not check
- Carousel button wake does not check
- Carousel timer wake with `stay:false` does not check
- Carousel timer wake with `stay:true` checks
- Mixed stay flags only check on `stay:true` images

**Sleep Duration Tests:**
- Button-only mode returns 0 (indefinite sleep)
- Normal interval uses config value
- CRC32 matched uses interval with matched reason
- Carousel uses per-image interval

**Integration Tests:**
- Single image timer wake with CRC32 match
- Carousel button wake (advance + no check + next interval)
- Carousel timer stay:true with CRC32 match

#### Battery Logic Tests

**Out-of-Range Tests:**
- Voltage below 3.43V returns 0%
- Voltage above 4.13V returns 100%

**Exact Map Point Tests:**
- All 21 voltage points return exact percentages (4.13V→100%, 4.12V→95%, ..., 3.43V→0%)
- Based on real-world 192-hour discharge test data (GitHub issue #57)

**Interpolation Tests:**
- Mid-point between map values uses linear interpolation
- Multiple interpolation points validated

**Rounding Tests:**
- All results rounded to nearest 5%
- Never exceeds 100% or drops below 0%

**Behavior Tests:**
- Monotonic behavior (increasing voltage never decreases percentage)
- Realistic scenarios (fully charged, half charged, low battery, critically low)

#### Sleep Logic Tests

**Button-Only Mode:**
- Returns 0 for indefinite sleep

**No Compensation:**
- Returns full interval when loop time is 0
- Various intervals (60s, 300s, 3600s) convert correctly

**Normal Compensation:**
- Subtracts loop time from target interval
- Small/large intervals handled correctly
- Fractional seconds with microsecond precision

**Edge Cases:**
- Loop time equals interval returns full interval
- Loop time exceeds interval returns full interval

**Realistic Scenarios:**
- Fast refresh with good network (5min - 8s = 292s)
- Hourly refresh with slow network (3600s - 45s = 3555s)
- Very slow network (5min - 2min = 3min)

**Boundaries:**
- Very large intervals (86400s)
- Almost-zero intervals (1s)
- Consistency across multiple calls

#### Config Logic Tests

**Timezone Offset Tests:**
- No offset returns original hour
- Positive offset with/without wrap (10+5=15, 22+5=3)
- Negative offset with/without wrap (15-8=7, 2-5=21)
- Extreme offsets (±23 hours)
- All 24 hours tested with positive/negative offsets

**Hour Bitmask Tests:**
- All disabled (0x00) returns false
- All enabled (0xFF) returns true
- Single hours (0, 12, 23) validated
- Business hours (9-17) validated
- Night hours (22-6) validated
- Every other hour pattern
- Boundary hours (0, 23)
- Invalid hours (24, 25, 255) return false

**All-Hours-Enabled Tests:**
- All bits set (0xFF 0xFF 0xFF) returns true
- All bits clear returns false
- One bit missing returns false
- Partial bytes validated

**Integration Tests:**
- Timezone-aware bitmask checking
- Cross-midnight schedule validation

### Integration Tests

Complete end-to-end scenario tests validating multiple decisions working together:

**Core Scenarios:**
1. Single image + timer wake + CRC32 match → Skip download, sleep interval
2. Single image + timer wake + CRC32 changed → Download, display, sleep interval
3. Single image + button wake → Always download (never check CRC32)
4. Carousel + timer wake + stay:false → Advance to next, download, use next interval
5. Carousel + timer wake + stay:true → Stay on current, check CRC32, use current interval
6. Carousel + button wake → Always advance (overrides stay:true)
7. Hourly schedule disabled hour → Sleep until next enabled hour
8. Button wake → Bypass hourly schedule (always proceed)

**Additional Scenarios (2 tests):**
9. Carousel wrap-around → Last image advances to first
10. Button-only mode (interval=0) → Indefinite sleep

**Refactored Orchestration Tests (7 tests):**
11-17. All core integration tests refactored to use `orchestrateNormalModeDecisions()` function

**Carousel Edge Cases (4 tests):**
18. Carousel wrap-around with orchestration → Validates carousel boundary
19. CRC32 disabled in carousel → Verifies orchestration with CRC32 off
20. Carousel boundary conditions → Tests first/last image transitions
21. Single-image carousel → Validates degenerate case

**Hourly Schedule Tests (9 tests):**
22-30. Comprehensive tests for `calculateSleepMinutesToNextEnabledHour()` including disabled hours, wrap-around, partial schedules, business hours, and seconds rounding

**End-to-End Orchestration Tests (5 tests):**
31-35. Complete orchestration validation including button wake bypass, index preservation for CRC32, hourly schedule integration, and all-hours-enabled scenarios

**Integration Test Benefits:**
- Validates **complete decision flows** (image target + CRC32 + sleep)
- Tests **realistic user configurations** (not just isolated functions)
- Covers **40+ execution paths** from NORMAL_MODE_FLOW.md
- Catches **interaction bugs** between decision functions
- Serves as **executable documentation** of system behavior

**Example Integration Test:**
```cpp
TEST_F(NormalModeIntegrationTest, Carousel_TimerWake_StayFalse_AdvancesToNext) {
    // GIVEN: Carousel config with stay:false
    DashboardConfig config = ConfigBuilder()
        .carousel()
        .addImage("img1.png", 10, false)
        .addImage("img2.png", 15, false)
        .addImage("img3.png", 20, false)
        .build();
    
    // WHEN: Timer wake on image 1
    auto imageTarget = determineImageTarget(config, WAKEUP_TIMER, 1);
    auto crc32Action = determineCRC32Action(config, WAKEUP_TIMER, 1);
    auto sleepDuration = determineSleepDuration(config, time, 2, false);
    
    // THEN: Verify complete flow
    EXPECT_EQ(imageTarget.targetIndex, 2);      // Advance 1→2
    EXPECT_FALSE(crc32Action.shouldCheck);      // No CRC32 check
    EXPECT_EQ(sleepDuration.sleepSeconds, 1200.0f);  // Image 2 interval (20 min)
}
```

## Build Artifacts

Build outputs are in `test/build/` (gitignored):
- `Release/decision_tests.exe` - Decision logic unit tests (19 tests)
- `Release/battery_tests.exe` - Battery logic unit tests (17 tests)
- `Release/sleep_tests.exe` - Sleep logic unit tests (21 tests)
- `Release/config_tests.exe` - Config logic unit tests (25 tests)
- `Release/integration_tests.exe` - Integration scenario tests (10 tests)
- `lib/Release/*.lib` - Google Test libraries
- `CTestTestfile.cmake` - CTest configuration
- `compile_commands.json` - Clang tooling support

## Maintenance

### Updating Tests After Logic Changes

When modifying extracted functions:

1. **Update functions** in `common/src/{module}_logic.cpp`
   - `modes/decision_logic.cpp` - Decision functions
   - `battery_logic.cpp` - Battery calculations
   - `sleep_logic.cpp` - Sleep duration
   - `config_logic.cpp` - Config helpers
2. **Run tests immediately** to verify: `.\run-tests.ps1`
3. **Update test expectations if needed** in `test/unit/test_{module}_logic.cpp`

**Benefits:** Tests compile the REAL production code - no manual copying/syncing required!

### Adding New Tests

**Unit Tests:**
1. Add test case to appropriate file in `test/unit/`:
   - `test_decision_functions.cpp` - Decision logic
   - `test_battery_logic.cpp` - Battery calculations
   - `test_sleep_logic.cpp` - Sleep duration
   - `test_config_logic.cpp` - Config validation
2. Use Google Test macros: `TEST_F(TestSuiteName, TestName)` or `TEST(TestSuiteName, TestName)`
3. Call standalone functions directly: `calculateBatteryPercentage(voltage)`
4. Assert results: `EXPECT_EQ()`, `EXPECT_TRUE()`, `EXPECT_FLOAT_EQ()`
5. Run tests to validate: `.\run-tests.ps1`

**Integration Tests:**
1. Add test case to `test/integration/test_normal_mode_scenarios.cpp`
2. Use `ConfigBuilder` for fluent config creation:
   ```cpp
   DashboardConfig config = ConfigBuilder()
       .carousel()
       .addImage("url1", 10, false)
       .addImage("url2", 15, true)
       .withCRC32(true)
       .build();
   ```
3. Execute all relevant decision functions (image target, CRC32, sleep)
4. Assert complete flow outcomes (not just individual decisions)
5. Add integration assertions that verify decision interactions
6. Run tests to validate: `.\run-tests.ps1`

## Troubleshooting

### Build Errors

**Error: "CMake not found"**
- Install CMake: `winget install --id Kitware.CMake`
- Restart PowerShell to refresh PATH

**Error: "Cannot open include file"**
- CMake include paths set in `CMakeLists.txt`
- Mocks are prioritized over Arduino includes (`test/mocks/` included first)
- Check that `test/mocks/` exists with `Arduino.h`, `Inkplate.h`, `config.h`, `config_manager.h`
- `config_manager.h` mock prevents Arduino `Preferences.h` from being included

**Error: "binary '+': no global operator found"**
- Mock `String` class missing operators
- Add concatenation operators to `test/mocks/Arduino.h`

### Test Failures

**Unexpected results:**
1. Verify mock config matches real `DashboardConfig` struct in `common/src/config.h`
2. Review [NORMAL_MODE_FLOW.md](../docs/dev/NORMAL_MODE_FLOW.md) for expected behavior
3. Check if production code behavior changed (tests validate REAL code)
4. Add logging to production functions if needed (in `decision_logic.cpp`)

**All tests fail:**
- Verify Google Test libraries built successfully
- Check test runner output for compilation errors
- Try clean build: `.\run-tests.ps1 -Clean`

## References

- [Normal Mode Flow Documentation](../docs/dev/NORMAL_MODE_FLOW.md) - Truth table and execution paths
- [CMake Documentation](https://cmake.org/documentation/) - Build system reference
- [Google Test Primer](https://google.github.io/googletest/primer.html) - Testing framework guide
