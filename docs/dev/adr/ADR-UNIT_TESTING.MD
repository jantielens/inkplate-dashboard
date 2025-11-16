# Unit Testing Pattern for Embedded Firmware

## Status
Accepted (November 2025)

## Context

The inkplate-dashboard firmware is embedded C++ targeting ESP32 with tight Arduino framework dependencies. Traditional unit testing approaches for embedded systems face challenges:

1. **Arduino Framework Dependencies**: Code heavily uses Arduino types (`String`), functions (`millis()`), and hardware abstractions (Inkplate display classes)
2. **Cross-Platform Build Complexity**: Arduino CLI uses a flat file structure with copied dependencies, while standard C++ testing uses hierarchical includes
3. **Hardware Coupling**: Business logic mixed with hardware interactions makes testing difficult
4. **No Test Infrastructure**: No existing unit tests - manual testing on hardware only

**Problem**: How do we implement true unit testing for embedded firmware without requiring hardware, while maintaining compatibility with Arduino's build system?

## Decision

Implement **standalone pure function extraction** with **conditional compilation** for cross-platform testing:

### 1. Extract Pure Functions

Move business logic from manager classes into standalone pure C++ functions:

```cpp
// common/src/battery_logic.h
float calculateBatteryPercentage(float voltage);

// common/src/battery_logic.cpp
float calculateBatteryPercentage(float voltage) {
  // Pure C++ - no Arduino dependencies
  static const float voltageMap[][2] = {
    {4.20f, 100.0f}, {4.15f, 95.0f}, /* ... */
  };
  // Linear interpolation + rounding logic
}
```

**Manager classes delegate to standalone functions:**

```cpp
// common/src/power_manager.cpp
float PowerManager::calculateBatteryPercentage() {
  float voltage = readBatteryVoltage();
  return ::calculateBatteryPercentage(voltage);  // Call standalone function
}
```

### 2. Conditional Compilation for Include Paths

Use preprocessor directives to include different headers for Arduino vs. tests:

```cpp
// common/src/modes/decision_logic.h
#ifndef UNIT_TEST
  // Arduino build - use relative paths
  #include "../config_manager.h"
  #include "../power_manager.h"
#else
  // Test build - use mocks from test/mocks/
  #include "config.h"
#endif
```

CMake defines `UNIT_TEST` for all test builds:

```cmake
add_compile_definitions(UNIT_TEST)
```

### 3. Minimal Mocking Strategy

Create minimal mocks for Arduino/hardware dependencies in `test/mocks/`:

```cpp
// test/mocks/Arduino.h
class String {
public:
  String(const char* s = "") : _data(s ? s : "") {}
  const char* c_str() const { return _data.c_str(); }
  bool operator==(const char* s) const { return _data == s; }
private:
  std::string _data;
};

unsigned long millis() { return 0; }
```

**Mock delegation pattern** - mocks call real standalone functions:

```cpp
// test/mocks/config_manager.cpp
int ConfigManager::applyTimezoneOffset(int hour, int offset) {
  return ::applyTimezoneOffset(hour, offset);  // Delegate to standalone function
}
```

### 4. CMake Build System for Tests

Separate CMake project in `test/` directory:

```cmake
# test/CMakeLists.txt
include_directories(
  ${CMAKE_SOURCE_DIR}/mocks           # Mocks prioritized first
  ${CMAKE_SOURCE_DIR}/../common/src   # Real production code
)

add_executable(battery_tests
  unit/test_battery_logic.cpp
  ../common/src/battery_logic.cpp     # Compile REAL production code
)

target_link_libraries(battery_tests gtest_main)
gtest_discover_tests(battery_tests)
```

### 5. Test Execution via PowerShell Wrapper

```powershell
# run-tests.ps1
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Architecture

### Module Structure

```
common/src/
├── battery_logic.h/cpp        # Pure function: calculateBatteryPercentage(voltage)
├── sleep_logic.h/cpp          # Pure function: calculateAdjustedSleepDuration(target, loop)
├── config_logic.h/cpp         # Pure functions: applyTimezoneOffset, isHourEnabledInBitmask, etc.
└── modes/
    └── decision_logic.h/cpp   # Pure functions: determineImageTarget, determineCRC32Action, etc.

test/
├── unit/
│   ├── test_battery_logic.cpp     # 17 tests
│   ├── test_sleep_logic.cpp       # 21 tests
│   ├── test_config_logic.cpp      # 25 tests
│   └── test_decision_functions.cpp # 19 tests
├── mocks/
│   ├── Arduino.h              # Mock Arduino types (String, millis, etc.)
│   ├── Inkplate.h             # Mock Inkplate display class
│   ├── config.h               # Mock DashboardConfig struct and WakeupReason enum
│   ├── config_manager.cpp     # Mock ConfigManager (delegates to config_logic)
│   └── config_manager.h       # Prevent Arduino Preferences.h include
├── CMakeLists.txt             # 4 test executables, Google Test integration
└── run-tests.ps1              # Test runner (CMake + CTest)
```

### Test Coverage (78 tests total)

1. **Decision Logic (19 tests)** - Normal mode execution flow
2. **Battery Logic (17 tests)** - Li-ion voltage to percentage conversion
3. **Sleep Logic (21 tests)** - Active loop time compensation
4. **Config Logic (25 tests)** - Timezone offset and hour bitmask validation

### Build System Compatibility

**Arduino Build (production):**
- Uses `build.ps1` script
- Copies `.cpp` files from `common/src/` to board directories
- Includes use relative paths (`../config_manager.h`)
- No `UNIT_TEST` define - uses real Arduino headers

**CMake Build (tests):**
- Uses `test/run-tests.ps1` script
- Compiles production code directly from `common/src/`
- Includes use mocks (`config.h` from `test/mocks/`)
- Defines `UNIT_TEST` - triggers conditional compilation

## Consequences

### Positive

✅ **True Unit Testing**: Tests compile REAL production code, not mocks/stubs  
✅ **Zero Manual Sync**: No copying logic between production and test code  
✅ **Fast Execution**: 78 tests run in <1 second (no hardware required)  
✅ **No Hardware Required**: Test on any machine with CMake + MSVC  
✅ **Backward Compatible**: No breaking changes to Arduino builds  
✅ **CI/CD Ready**: Can integrate into GitHub Actions easily  
✅ **Refactoring Safety**: Tests catch regressions immediately  
✅ **Documentation**: Tests serve as executable specifications  

### Negative

⚠️ **Build Complexity**: Two separate build systems (Arduino CLI + CMake)  
⚠️ **Mock Maintenance**: Mocks must stay synchronized with Arduino APIs  
⚠️ **Conditional Compilation**: Include path logic in preprocessor directives  
⚠️ **Initial Setup**: CMake/MSVC installation required for development  
⚠️ **Not Full Coverage**: Only business logic tested, not hardware integration  

### Neutral

🔄 **Learning Curve**: Developers need CMake knowledge  
🔄 **Platform Specific**: PowerShell scripts are Windows-specific (could add bash)  
🔄 **Manager Overhead**: Small delegation overhead in manager classes  

## Implementation Pattern

### Extracting a New Module for Testing

**Step 1: Create standalone pure functions**

```cpp
// common/src/new_module_logic.h
#pragma once
float pureCalculation(float input);

// common/src/new_module_logic.cpp
#include "new_module_logic.h"
float pureCalculation(float input) {
  // Pure C++ - no Arduino dependencies
  return input * 2.0f;
}
```

**Step 2: Update manager class to delegate**

```cpp
// common/src/manager.cpp
#include "new_module_logic.h"

float Manager::calculate() {
  float input = getInput();
  return ::pureCalculation(input);  // Delegate
}
```

**Step 3: Create test suite**

```cpp
// test/unit/test_new_module_logic.cpp
#include <gtest/gtest.h>
#include "new_module_logic.h"

TEST(NewModuleTest, BasicCalculation) {
  EXPECT_FLOAT_EQ(pureCalculation(5.0f), 10.0f);
}
```

**Step 4: Add to CMakeLists.txt**

```cmake
add_executable(new_module_tests
  unit/test_new_module_logic.cpp
  ../common/src/new_module_logic.cpp
)
target_link_libraries(new_module_tests gtest_main)
gtest_discover_tests(new_module_tests)
```

**Step 5: Run tests**

```powershell
.\run-tests.ps1
```

## Maintenance

### When to Extract Functions for Testing

**Good Candidates:**
- ✅ Complex calculations (battery percentage, sleep duration)
- ✅ Decision logic (state machines, conditional flows)
- ✅ Data transformations (timezone offset, bitmask validation)
- ✅ Algorithms with edge cases (interpolation, rounding)

**Poor Candidates:**
- ❌ Hardware I/O operations (display writes, network requests)
- ❌ Simple getters/setters (no logic to test)
- ❌ One-line delegations (overhead not worth it)

### Keeping Mocks Synchronized

When Arduino APIs change:
1. Update mocks in `test/mocks/` to match new signatures
2. Run tests to validate compatibility
3. Update conditional includes if needed

### Adding New Test Suites

1. Create `test/unit/test_{module}_logic.cpp`
2. Add executable to `test/CMakeLists.txt`
3. Write comprehensive tests (boundary, edge, realistic scenarios)
4. Run `.\\run-tests.ps1` to validate

## References

- [test/README.md](../../test/README.md) - Complete testing documentation
- [NORMAL_MODE_FLOW.md](../NORMAL_MODE_FLOW.md) - Decision logic specification
- [Google Test Primer](https://google.github.io/googletest/primer.html) - Testing framework
- [CMake Documentation](https://cmake.org/documentation/) - Build system

## Related Decisions

- **ADR-ARCHITECTURE.MD** - Multi-board design (shared code structure enables testing)
- **ADR-NORMAL_MODE_FLOW.MD** - Decision logic specification (validated by tests)
- **ADR-CRC32_GUIDE.MD** - CRC32 optimization (tested in decision logic suite)
- **ADR-HOURLY_SCHEDULING.MD** - Hourly scheduling (config logic tests validate bitmask)

## Examples

See implementation:
- `common/src/battery_logic.cpp` - 22-point Li-ion discharge curve
- `common/src/sleep_logic.cpp` - Loop time compensation algorithm
- `common/src/config_logic.cpp` - Timezone and bitmask helpers
- `common/src/modes/decision_logic.cpp` - Normal mode decision functions
- `test/unit/test_*.cpp` - 78 comprehensive tests across 4 modules
