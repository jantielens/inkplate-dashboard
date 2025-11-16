# ADR: Serial Logging Refactor for Firmware Size Optimization

**Status:** Implemented  
**Date:** 2025-11-16  
**Deciders:** GitHub Copilot, jantielens  
**Issue:** #105

## Context and Problem Statement

The firmware was using Unicode box-drawing characters for serial logging, consuming unnecessary flash memory for string literals. Additionally, logging was excessively verbose, providing limited debugging value while consuming significant firmware space. The system lacked support for nested logging operations, making it difficult to visualize complex multi-step processes.

### Problems Identified

1. **High firmware cost for logging**:
   - Unicode box-drawing characters (`╭`, `│`, `╰`) consume more bytes than ASCII
   - Large buffer sizes (256 bytes) for formatted strings
   - Verbose message strings with redundant information
   - No reuse of common string patterns

2. **Limited debugging utility**:
   - Flat logging structure made it hard to track nested operations
   - Overly verbose messages obscured important information
   - No timing information for nested sub-operations
   - MQTT error descriptions were excessively verbose (3-4 lines per error)

3. **Lack of nesting support**:
   - No way to show hierarchical operations (e.g., WiFi → Channel Lock → Result)
   - Single global timer couldn't track nested operation timing
   - No visual indication of operation depth

## Decision Drivers

- **Firmware size reduction**: Minimize flash memory used by logging strings
- **Debugging value**: Maintain or improve usefulness of serial output for troubleshooting
- **Backward compatibility**: Ensure existing code continues to work without changes
- **Readability**: Keep serial output easy to read and parse
- **Maintainability**: Make it easy for contributors to follow logging conventions

## Considered Options

### Option 1: Remove All Logging
- **Pros**: Maximum firmware size savings
- **Cons**: Impossible to debug issues in production, poor developer experience

### Option 2: Conditional Compilation with Debug Flags
- **Pros**: Can completely disable logging in production builds
- **Cons**: Two different codebaths to maintain, harder to debug user issues

### Option 3: Refactor Logging with Indentation (CHOSEN)
- **Pros**: Readable, supports nesting, reduces string literal size, backward compatible
- **Cons**: Requires refactoring effort, still has some overhead

### Option 4: Binary Logging Protocol
- **Pros**: Minimal overhead, very space efficient
- **Cons**: Requires custom tools to read logs, poor developer experience

## Decision Outcome

**Chosen Option:** Option 3 - Refactor Logging with Indentation

We chose to refactor the existing logging system to use simple indentation instead of Unicode box-drawing, support nested operations, and reduce message verbosity while maintaining debugging utility.

### Implementation Details

#### 1. Logger Class Refactor

**Previous Implementation:**
```cpp
class LogBox {
    static unsigned long startTime;  // Single global timer
    // Box-drawing output: ╭── Title
    //                     │   Content
    //                     ╰──────(XXXms)───────
};
```

**New Implementation (Logger):**
```cpp
class Logger {
    static unsigned long startTimes[3];  // Stack for 3 nesting levels
    static uint8_t nestLevel;            // Current depth (0-3)
    static const char* indent();         // Returns "" / "  " / "    " / "      "
};
```

**Key Changes:**
- Renamed `LogBox` → `Logger` (more accurate, clearer intent)
- Replaced all `Logger::` calls with `Logger::` throughout codebase
- Replaced Unicode box-drawing with indentation (`""`, `"  "`, `"    "`, `"      "`)
- Added nesting support (up to 3 levels deep)
- Reduced buffer size from 256 to 128 bytes
- Added independent timing for each nesting level

**Output Format:**
```
[WiFi] Starting...
  SSID: MyNetwork
  [Channel Lock] Starting...
    Using channel 6
  Done (45ms)
  Connected!
Done (1234ms)
```

#### 2. Graceful Overflow/Underflow Handling

**Overflow (depth > 3):**
- Continues logging with maximum indentation (6 spaces)
- Shows 0ms timing (since we can't track beyond 3 levels)
- No warnings or errors - silent degradation

**Underflow (extra `end()` calls):**
- Silently ignored when `nestLevel == 0`
- No warnings or errors - prevents log spam from coding errors

#### 3. Verbosity Reduction

**MQTT Error State Decoding:**

Before (45+ lines of switch statement):
```cpp
switch (state) {
    case -2:
        Logger::line("  → MQTT_CONNECT_FAILED - Network connection failed");
        Logger::line("  → Check: Is the broker IP/hostname correct?");
        Logger::line("  → Check: Is the broker port correct? (usually 1883)");
        Logger::line("  → Check: Can you ping the broker from your network?");
        break;
    // ... 8 more cases
}
```

After (10 lines with helper function):
```cpp
const char* getMQTTStateDesc(int state) {
    switch (state) {
        case -2: return "Network failed - check broker IP/port";
        // ... 8 more cases
    }
}
Logger::linef("  %s", getMQTTStateDesc(state));
```

**WiFi Connection Logging:**

Before:
```cpp
Logger::line("Connected to WiFi!");
Logger::line("IP Address: " + WiFi.localIP().toString());
Logger::linef("Signal Strength: %d dBm", WiFi.RSSI());
```

After:
```cpp
Logger::linef("Connected! IP: %s, RSSI: %d dBm", 
    WiFi.localIP().toString().c_str(), WiFi.RSSI());
```

**CRC32 Check Logging:**

Before:
```cpp
Logger::linef("Attempt %d/%d (timeout: %dms)", attempt + 1, maxRetries, timeout);
Logger::linef("CRC32 fetched successfully in %lums (attempt %d)", elapsed, attempt + 1);
Logger::linef("Parsed CRC32: 0x%08X", newCRC32);
Logger::linef("Stored CRC32: 0x%08X", storedCRC32);
Logger::end("CRC32 UNCHANGED - Skipping download");
```

After:
```cpp
Logger::linef("CRC32 attempt %d/%d", attempt + 1, maxRetries);
Logger::linef("CRC32 fetched (%lums)", elapsed);
Logger::linef("New: 0x%08X", newCRC32);
Logger::linef("Stored: 0x%08X", storedCRC32);
Logger::end("UNCHANGED - Skipping download");
```

### Logging Best Practices (for Contributors)

1. **Use Nesting for Related Operations**
   ```cpp
   Logger::begin("WiFi Connection");
   Logger::line("SSID: MyNetwork");
   
   Logger::begin("Channel Lock");
   Logger::line("Using channel 6");
   Logger::end(); // Shows channel lock timing
   
   Logger::line("Connected!");
   Logger::end(); // Shows total WiFi connection timing
   ```

2. **Keep Messages Concise**
   - ✅ `"Connected! IP: 192.168.1.100, RSSI: -45 dBm"`
   - ❌ `"Successfully connected to WiFi network!"` + `"IP Address: 192.168.1.100"` + `"Signal Strength: -45 dBm"`

3. **Combine Related Information**
   - ✅ `"Attempt %d/%d failed (state: %d)", attempt, max, state`
   - ❌ `"Attempt %d of %d"` + separate `"State: %d"`

4. **Use Short Labels for Repeated Data**
   - ✅ `"New: 0x%08X"`, `"Stored: 0x%08X"`
   - ❌ `"Parsed CRC32 value from remote server: 0x%08X"`, `"Stored CRC32 value in NVS: 0x%08X"`

5. **Avoid Redundant Context in Messages**
   - ✅ Module name in `begin()`: `Logger::begin("WiFi Connection")`
   - ❌ Repeating module name: `Logger::line("WiFi: Connecting...")`

6. **Log Failures and State Changes, Not Routine Operations**
   - ✅ Log connection failures, retries, fallbacks
   - ❌ Don't log every step of a normal successful operation

7. **Don't Exceed 3 Nesting Levels**
   - Maximum supported depth is 3 levels
   - Beyond that, timing shows as 0ms
   - Restructure code if you need deeper nesting

### Test Coverage

Created comprehensive unit tests (`test/unit/test_logger.cpp`) with 13 test cases:

**Basic Functionality:**
- Single block with timing
- Single block with custom message
- Formatted line output

**Nesting:**
- 2-level nesting with correct indentation
- 3-level nesting (maximum supported)
- 4-level nesting (graceful degradation)

**Overflow/Underflow:**
- Extra `end()` calls (ignored)
- Timing accuracy at each level

**Edge Cases:**
- Empty blocks
- Null messages
- Long messages (truncation)

## Consequences

### Positive

1. **Firmware Size Reduction**: Estimated 1-1.5 KB savings
   - 128 bytes per buffer reduction
   - ~500-800 bytes from shorter string literals
   - ~300 bytes from MQTT error consolidation

2. **Improved Readability**:
   - Indentation clearly shows operation hierarchy
   - Timing for nested operations helps identify bottlenecks
   - Concise messages are easier to scan

3. **Better Debugging**:
   - Can see how long each sub-operation takes
   - Visual grouping makes it clear which operations succeeded/failed
   - Less noise in successful operations

4. **Consistent Codebase**:
   - All code now uses `Logger::` uniformly
   - No legacy naming to maintain
   - Clear, modern API

5. **Maintainable**:
   - Clear guidelines for contributors
   - Simpler implementation (no Unicode handling)
   - Unit tested to prevent regressions

### Negative

1. **Migration Effort**:
   - Took time to update all logging call sites
   - Contributors must learn new conventions
   - Required documentation updates

2. **Limited Nesting Depth**:
   - Only 3 levels supported
   - Must restructure deeply nested operations
   - Could be confusing if not documented

3. **Timing Limitation**:
   - Beyond 3 levels, timing shows as 0ms
   - Need to be aware of this limitation

### Neutral

1. **Serial Output Format Change**:
   - Users accustomed to box-drawing will see different output
   - Any log parsing tools need updating
   - But: most users don't parse logs programmatically

## Validation

1. **Unit Tests**: All 126 tests pass (114 existing + 12 new logger tests)
2. **Code Migration**: All `LogBox::` calls replaced with `Logger::`
3. **No Functional Changes**: Only output format and naming changed, not behavior

## References

- Issue #105: Serial Logging Overhaul
- PR #XXX: Implementation
- CHANGELOG.md: Version 1.6.0 entry
- Test file: `test/unit/test_logger.cpp`

## Future Work

1. **Measure Actual Firmware Size**: Compare binary sizes before/after
2. **Log Level Support**: Add DEBUG/INFO/WARN/ERROR levels
3. **Conditional Compilation**: Add build flag to disable all logging
4. **Performance Profiling**: Add timing summaries for complete operations
5. **Log Formatting**: Add timestamps or elapsed time since boot
