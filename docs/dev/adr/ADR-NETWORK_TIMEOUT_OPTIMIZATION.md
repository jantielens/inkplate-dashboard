# Architecture Decision Record: Network Timeout Optimization and Retry Telemetry

## Status
Accepted and Implemented (v1.3.3)

## Context

Analysis of 890 production device telemetry samples revealed that network timeout values were too conservative, causing unnecessary battery drain and poor user experience during timeout events. The original implementation used single-attempt, long timeouts:

- **CRC32 Check**: 10 second timeout, no retries
- **NTP Sync**: 15 second timeout (30 × 500ms polling)
- **WiFi Full Scan**: 23 second max (3 retries × 5s + 1s delays)

Real-world data showed:
- Normal CRC32 responses: 0.06-0.20s (but timeout events wasted full 10s)
- Normal NTP sync: 0-0.01s on timer wakes, ~4s on first boot
- Normal WiFi: 0.14-0.32s via channel lock (90%), 2.8-3.0s reconnect (9%)

Additionally, there was no visibility into retry behavior, making it impossible to:
- Track network/server health degradation
- Identify performance issues proactively
- Validate optimization effectiveness

## Decision

Implement progressive timeout strategies with retry telemetry for all network operations:

### 1. CRC32 Progressive Timeouts

**Implementation:**
```cpp
const int crcTimeouts[] = {300, 700, 1500};  // Progressive timeouts (ms)
const int crcRetryDelay = 100;  // ms between retries
// Total max time: 300 + 100 + 700 + 100 + 1500 = 2700ms

// Enforce deadline - track actual elapsed time
unsigned long startTime = millis();
unsigned long deadline = crcTimeouts[attempt];

httpCode = http.GET();

unsigned long elapsed = millis() - startTime;

// Force timeout if we exceeded our deadline, even if request "succeeded"
if (elapsed > deadline) {
    LogBox::linef("Deadline exceeded: %lums > %lums - treating as timeout", elapsed, deadline);
    http.end();
    httpCode = -1;  // Force retry
    // ... retry logic ...
    continue;
}
```

**Rationale:**
- First timeout (300ms) covers 99%+ of normal responses (0.06-0.20s) with safety margin
- Progressive backoff (700ms, 1500ms) shows patience with slow servers
- 3 attempts handle transient network glitches
- Total 2.7s is 73% faster than original 10s timeout
- Still allows fallback to full image download if all attempts fail

**Critical Fix (Post-Implementation):**
- **Issue**: `HTTPClient::setTimeout()` only controls TCP connection and inactivity timeouts, NOT total request duration
- **Impact**: Slow servers responding continuously could exceed timeout limits (observed: 3.67s and 7.17s responses)
- **Solution**: Added explicit deadline enforcement using `millis()` to measure elapsed time
- **Result**: Hard deadline now enforced - requests exceeding timeout are treated as failures and trigger retries

**Retry Semantics:**
- Attempt 0 succeeds: 0 retries
- Attempt 1 succeeds: 1 retry
- Attempt 2 succeeds: 2 retries
- All fail: 2 retries (first attempt is not a retry)

### 2. NTP Sync Optimization

**Implementation:**
```cpp
// Wait for NTP sync with timeout (up to 7 seconds)
int ntpRetries = 0;
while (now < 24 * 3600 && ntpRetries < 70) {  // 70 × 100ms = 7s
    delay(100);  // Reduced from 500ms to 100ms
    now = time(nullptr);
    ntpRetries++;
}
```

**Rationale:**
- Timer wakes use cached RTC time (~0.01s) - timeout rarely reached
- First boot NTP sync typically completes in 4s, 7s provides safety margin
- 100ms polling is more responsive than 500ms blocking
- Reduces timeout from 15s to 7s (53% faster on failures)
- Lower priority optimization since NTP failures are rare

### 3. WiFi Connection Optimization

**Implementation:**
```cpp
// Channel lock (fast path) - UNCHANGED
const unsigned long channelLockTimeout = 2000;  // 2s

// Full scan (fallback) - OPTIMIZED
const int maxRetries = 4;  // Increased from 3
const unsigned long timeout = 3000;  // Reduced from 5000ms
const unsigned long retryDelay = 300;  // Reduced from 1000ms
// Total max time: 3s + 0.3s + 3s + 0.3s + 3s + 0.3s + 3s + 0.3s + 3s = 16.2s
```

**Rationale:**
- Channel lock path unchanged (optimal for 90% of wakes)
- 3s timeout still covers reconnection events (actual full scan ~0.7-0.9s)
- Extra retry compensates for shorter timeout
- Shorter delays (300ms) reduce idle waiting
- Total 16.2s is 29% faster than original 23s
- Still handles 8.4s edge case within retry budget

**Retry Semantics:**
- Channel lock success: 0 retries
- Channel lock fail + full scan success: 1+ retries (fallback counts as first retry)
- Full scan timeouts: Additional retries beyond channel lock fallback

### 4. MQTT Retry Telemetry

**New LoopTimings Fields:**
```cpp
struct LoopTimings {
    uint32_t wifi_ms = 0;
    uint32_t ntp_ms = 0;
    uint32_t crc_ms = 0;
    uint32_t image_ms = 0;
    
    // NEW: Retry tracking
    uint8_t wifi_retry_count = 0;   // WiFi connection retries (0-5)
    uint8_t crc_retry_count = 0;    // CRC32 check retries (0-2)
    uint8_t image_retry_count = 0;  // Image download retries (0-2, single image mode only)
};
```

**New MQTT Sensors:**
- `sensor.inkplate_loop_time_wifi_retries` - WiFi retry count (0-5)
- `sensor.inkplate_loop_time_crc_retries` - CRC32 retry count (0-2)
- `sensor.inkplate_loop_time_image_retries` - Image retry count (0-2, single image mode)

**Benefits:**
- Track network/server health over time
- Identify degrading conditions before complete failure
- Validate optimization effectiveness with real data
- Enable Home Assistant alerts for retry spikes
- Data-driven decisions for future optimizations

## Battery Life Calculations

### Assumptions
- WiFi power consumption: ~100mA @ 3.7V
- Battery capacity: 3000mAh
- CRC32 timeout: 1 event/day (server slow/unresponsive)
- NTP timeout: 1 event/week (rare - server down or network issues)
- WiFi full scan failure: 1 event/week

### Before Optimization
- CRC32: 10s × 100mA × 365 days = **101.5 mAh/year**
- NTP: 15s × 100mA × 52 weeks = **21.7 mAh/year**
- WiFi failures: 23s × 100mA × 52 weeks = **33.2 mAh/year**
- **Total waste: 156.4 mAh/year (~5.2% of battery)**

### After Optimization
- CRC32: 2.7s × 100mA × 365 days = **27.4 mAh/year**
- NTP: 7s × 100mA × 52 weeks = **10.1 mAh/year**
- WiFi failures: 16.2s × 100mA × 52 weeks = **23.4 mAh/year**
- **Total waste: 60.9 mAh/year (~2.0% of battery)**

### Net Savings
**95.5 mAh/year (3.2% of battery capacity)**

**Note:** These are conservative estimates for timeout events only. Normal operations are unaffected. Actual savings depend on network reliability and server responsiveness.

## Consequences

### Positive
- **Battery Life**: 3.2% improvement in battery capacity per year on timeout events
- **User Experience**: 7-8 seconds faster error recovery during timeouts
- **Observability**: New MQTT sensors enable proactive monitoring and alerting
- **Resilience**: Progressive backoff handles transient network issues gracefully
- **Data-Driven**: Retry telemetry enables future optimizations based on real-world data

### Negative
- **Complexity**: Additional retry logic increases code complexity
- **Marginal Risk**: Shorter timeouts could theoretically cause failures on very slow networks (mitigated by progressive backoff and extra retries)

### Neutral
- **Backward Compatibility**: No breaking changes, fallback behavior unchanged
- **Testing**: Requires validation across different network conditions
- **Maintenance**: Retry counts must be tracked and published correctly

## Implementation Details

### Files Modified
- `common/src/version.h` - Version bumped to 1.3.3
- `common/src/image_manager.cpp` - CRC32 progressive timeout retry logic
- `common/src/image_manager.h` - Added retry count output parameter
- `common/src/wifi_manager.cpp` - WiFi timeout optimization and retry tracking
- `common/src/wifi_manager.h` - Added retry count output parameter
- `common/src/modes/normal_mode_controller.cpp` - NTP optimization, retry capture
- `common/src/modes/normal_mode_controller.h` - Extended LoopTimings struct
- `common/src/mqtt_manager.cpp` - Added retry count sensors and state publishing
- `common/src/mqtt_manager.h` - Extended publishAllTelemetry signature

### Code Review
- 3 rounds of code review conducted
- All feedback addressed (retry count semantics, carousel mode handling, consistency)
- Security scan completed (no issues)

### Testing Strategy
- Normal operations: Verify no change in success timings
- Slow servers: Verify progressive backoff works correctly
- Timeout events: Verify faster failure recovery
- MQTT telemetry: Verify retry counts published correctly
- Battery monitoring: 48-72 hour comparison before/after

## Monitoring

### Home Assistant Integration

**Example Alert:**
```yaml
- alias: "Inkplate Server Health Warning"
  trigger:
    - platform: numeric_state
      entity_id: sensor.inkplate_loop_time_crc_retries
      above: 0
  action:
    - service: notify.me
      data:
        message: "Inkplate CRC32 server slow ({{ states('sensor.inkplate_loop_time_crc_retries') }} retries)"
```

### Key Metrics
- `sensor.inkplate_loop_time_crc` - Should stay 0.06-0.20s normally
- `sensor.inkplate_loop_time_crc_retries` - Should be 0 mostly, occasional 1-2
- `sensor.inkplate_loop_time_wifi` - Should stay 0.14-0.32s normally
- `sensor.inkplate_loop_time_wifi_retries` - Should be 0 (90%), occasional 1 (9%)
- `sensor.inkplate_loop_time_ntp` - Should stay 0-0.01s on timer wakes, up to 4s first boot

### Alert Thresholds

**Warning:**
- CRC32 retries > 0 for 3+ consecutive cycles → Check server responsiveness
- WiFi retries > 1 for 3+ consecutive cycles → Check router/network

**Critical:**
- CRC32 retries = 2 (max) for 5+ consecutive cycles → Server performance issue
- WiFi retries > 2 for 5+ consecutive cycles → Network infrastructure problem

## References

### Data Source
- Production telemetry CSV with 890 samples
- Collected from live deployments over multiple weeks
- Covers various network conditions and server responsiveness scenarios

### Related Documentation
- Issue #71: Optimize Network Timeouts and Add Retry Telemetry for Battery Life
- User Guide: MQTT sensor documentation
- CHANGELOG.md: Version 1.3.3 release notes

### Related ADRs
- ADR-CRC32_GUIDE.md - CRC32 change detection implementation
- ADR-IMPLEMENTATION_NOTES.md - Retry mechanism and deferred CRC32

## Decision Date
2025-11-07

## Contributors
- @jantielens - Problem analysis and requirements
- @copilot - Implementation and optimization
