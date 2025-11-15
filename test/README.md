# Unit Tests

This directory contains unit tests for the inkplate-dashboard firmware decision functions using CMake and Google Test.

## Overview

The test suite validates **standalone pure C++ functions** extracted from the firmware for true unit testing:

### Decision Logic (19 tests)
Core decision functions from `normal_mode_controller.cpp`:
- `determineImageTarget()` - Which image to display and whether to advance
- `determineCRC32Action()` - Whether to check CRC32 for optimization
- `determineSleepDuration()` - How long to sleep until next wake

Validates **40+ execution paths** documented in [NORMAL_MODE_FLOW.md](../docs/dev/NORMAL_MODE_FLOW.md).

### Battery Logic (17 tests)
Battery percentage calculation from `power_manager.cpp`:
- `calculateBatteryPercentage()` - Li-ion voltage to percentage conversion
- 22-point discharge curve with linear interpolation
- 5% rounding for stability

### Sleep Logic (21 tests)
Sleep duration compensation from `power_manager.cpp`:
- `calculateAdjustedSleepDuration()` - Active loop time compensation
- Button-only mode handling
- Microsecond precision timing

### Config Logic (25 tests)
Configuration validation helpers from `config_manager.cpp`:
- `applyTimezoneOffset()` - 24-hour wrapping with timezone offsets
- `isHourEnabledInBitmask()` - Hour-based scheduling validation
- `areAllHoursEnabled()` - 24/7 schedule detection

**Total: 78 tests across 4 modules**

## Test Architecture

### Standalone Function Architecture

Functions are extracted into **standalone pure C++ modules** in `common/src/`. Tests compile and validate REAL production code without Arduino/ESP32 dependencies using minimal mocks.

```
test/
├── unit/
│   ├── test_decision_functions.cpp     # Decision logic tests (19 tests)
│   ├── test_battery_logic.cpp          # Battery calculation tests (17 tests)
│   ├── test_sleep_logic.cpp            # Sleep duration tests (21 tests)
│   └── test_config_logic.cpp           # Config validation tests (25 tests)
├── mocks/
│   ├── Arduino.h                       # Mock Arduino types (String, millis, etc.)
│   ├── Inkplate.h                      # Mock Inkplate display class
│   ├── config.h                        # Mock DashboardConfig struct and types
│   ├── config_manager.cpp              # Mock ConfigManager (delegates to config_logic)
│   └── config_manager.h                # Prevent Arduino Preferences.h include
├── CMakeLists.txt                      # CMake build configuration (4 test executables)
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

### Expected Output

```
=== Inkplate Dashboard Unit Test Runner ===

CMake version:
  cmake version 4.1.2

Building tests...
[... build output ...]

Running tests...
100% tests passed, 0 tests failed out of 78

Total Test time (real) = 0.79 sec

✅ All tests passed!
```

## Test Coverage

### Decision Logic Tests (19 tests)

**Image Target Tests (5 tests):**
- Single mode always returns index 0
- Carousel button wake always advances
- Carousel timer wake with `stay:false` advances
- Carousel timer wake with `stay:true` stays on current
- Mixed stay flags respected per-image

**CRC32 Action Tests (7 tests):**
- Disabled in config never checks
- Single mode timer wake checks
- Single mode button wake does not check
- Carousel button wake does not check
- Carousel timer wake with `stay:false` does not check
- Carousel timer wake with `stay:true` checks
- Mixed stay flags only check on `stay:true` images

**Sleep Duration Tests (4 tests):**
- Button-only mode returns 0 (indefinite sleep)
- Normal interval uses config value
- CRC32 matched uses interval with matched reason
- Carousel uses per-image interval

**Integration Tests (3 tests):**
- Single image timer wake with CRC32 match
- Carousel button wake (advance + no check + next interval)
- Carousel timer stay:true with CRC32 match

### Battery Logic Tests (17 tests)

**Out-of-Range Tests:**
- Voltage below 3.0V returns 0%
- Voltage above 4.2V returns 100%

**Exact Map Point Tests:**
- All 22 voltage points return exact percentages (4.20V→100%, 4.15V→95%, ..., 3.00V→0%)

**Interpolation Tests:**
- Mid-point between map values uses linear interpolation
- Multiple interpolation points validated

**Rounding Tests:**
- All results rounded to nearest 5%
- Never exceeds 100% or drops below 0%

**Behavior Tests:**
- Monotonic behavior (increasing voltage never decreases percentage)
- Realistic scenarios (fully charged, half charged, low battery, critically low)

### Sleep Logic Tests (21 tests)

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

### Config Logic Tests (25 tests)

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

## Build Artifacts

Build outputs are in `test/build/` (gitignored):
- `Release/decision_tests.exe` - Decision logic test executable (19 tests)
- `Release/battery_tests.exe` - Battery logic test executable (17 tests)
- `Release/sleep_tests.exe` - Sleep logic test executable (21 tests)
- `Release/config_tests.exe` - Config logic test executable (25 tests)
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

1. Add test case to appropriate file:
   - `test/unit/test_decision_functions.cpp` - Decision logic
   - `test/unit/test_battery_logic.cpp` - Battery calculations
   - `test/unit/test_sleep_logic.cpp` - Sleep duration
   - `test/unit/test_config_logic.cpp` - Config validation
2. Use Google Test macros: `TEST_F(TestSuiteName, TestName)` or `TEST(TestSuiteName, TestName)`
3. Call standalone functions directly: `calculateBatteryPercentage(voltage)`
4. Assert results: `EXPECT_EQ()`, `EXPECT_TRUE()`, `EXPECT_FLOAT_EQ()`
5. Run tests to validate: `.\run-tests.ps1`

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
