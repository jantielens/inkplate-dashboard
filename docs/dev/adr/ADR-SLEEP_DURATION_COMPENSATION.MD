# ADR: Sleep Duration Compensation for Accurate Refresh Intervals

**Status:** Accepted  
**Date:** 2025-11-09  
**Author:** System  
**Context:** Button-only wake mode feature development

## Problem Statement

The device's actual refresh interval is longer than the configured interval due to active loop time not being accounted for in sleep duration calculations.

### Current Behavior

When a user configures a 1-minute refresh interval:
- Wake up → Connect WiFi (1-2s) → Download image (2-3s) → Display image (2-4s) = ~7s active
- Sleep for exactly 60 seconds
- **Actual cycle time: 67 seconds** (not 60 seconds as expected)

This drift accumulates over time and affects:
- **User expectations:** "1 minute interval" should mean images refresh every 60 seconds
- **Hourly scheduling accuracy:** 7-second drift per cycle = significant timing errors over hours
- **Battery calculations:** Estimates assume configured intervals are accurate
- **User experience:** Slower updates than expected

### Technical Context

The code already measures `loopTimeMs` (time from wake to sleep) and passes it to `enterDeepSleep()`, but it's only used for logging:

```cpp
unsigned long loopStartTime = millis();
// ... WiFi, download, display ...
unsigned long loopTimeMs = millis() - loopStartTime;
powerManager->enterDeepSleep(refreshMinutes, loopTimeMs);  // loopTimeMs only logged
```

The `getSleepDuration()` function simply converts minutes to microseconds without adjustment:

```cpp
uint64_t PowerManager::getSleepDuration(uint16_t refreshRateMinutes) {
    uint64_t microseconds = (uint64_t)refreshRateMinutes * 60ULL * 1000000ULL;
    return microseconds;  // No compensation for active time
}
```

## Decision

**Adjust sleep duration to compensate for active loop time**, ensuring the total cycle time (active + sleep) matches the configured interval.

### Implementation

Modify `enterDeepSleep()` methods to subtract `loopTimeMs` from the calculated sleep duration:

```cpp
void PowerManager::enterDeepSleep(uint16_t refreshRateMinutes, unsigned long loopTimeMs) {
    bool buttonOnlyMode = (refreshRateMinutes == 0);
    
    if (!buttonOnlyMode) {
        uint64_t sleepDuration = getSleepDuration(refreshRateMinutes);
        
        // Compensate for active loop time to maintain accurate intervals
        if (loopTimeMs > 0) {
            uint64_t loopTimeMicros = loopTimeMs * 1000ULL;  // ms to µs
            uint64_t targetCycleMicros = refreshRateMinutes * 60ULL * 1000000ULL;
            
            if (loopTimeMicros < targetCycleMicros) {
                sleepDuration = targetCycleMicros - loopTimeMicros;
            }
            // Edge case: loop time >= interval, sleep full interval anyway
            // This is an edge case (very slow network/display), accept drift
        }
        
        esp_sleep_enable_timer_wakeup(sleepDuration);
    }
    // ... rest of function
}
```

### Edge Case Handling

**When loop time >= configured interval** (e.g., 90s active time with 1min interval):
- **Decision:** Sleep for the full configured interval anyway
- **Rationale:** This is an edge case indicating slow network/display performance
- **Outcome:** Drift is acceptable in this scenario - the configured interval acts as a minimum sleep time
- **Example:** 90s active + 60s sleep = 150s total cycle (50s drift)

This prevents:
- Immediate wake-up cycles (0s sleep) that waste power
- Complex retry/fallback logic for rare scenarios
- User confusion from unexpected behavior

### Benefits

1. **Accurate timing:** Configured "1 minute" interval = actual 60-second cycles
2. **Better scheduling:** Hourly wake windows work as expected
3. **Accurate battery estimates:** Power calculations match real-world usage
4. **Simple implementation:** Uses existing `loopTimeMs` measurement
5. **Backward compatible:** Only affects sleep duration calculation, not config format

### Logging Changes

Update logging to show both configured and compensated sleep durations:

```
Entering Deep Sleep
  Configured interval: 1 minutes
  Active loop time: 7234ms
  Adjusted sleep: 52.766 seconds
  Wake sources: TIMER + BUTTON
```

## Consequences

### Positive
- Users get the refresh intervals they configure
- Hourly scheduling is more accurate
- Battery estimates match reality
- No config format changes needed

### Negative
- Slightly more complex sleep calculation logic
- Edge case behavior (long loops) may surprise users initially
- Existing deployments will see timing changes (minor user impact)

### Neutral
- Applies to all boards consistently
- No impact on button-only mode (interval=0)
- Logging provides transparency for debugging

## Alternatives Considered

### 1. Do Nothing (Status Quo)
- **Rejected:** User expectations not met, timing drift accumulates

### 2. Minimum Sleep Time for Long Loops
- Set minimum sleep (e.g., 10s) when loop time >= interval
- **Rejected:** Adds complexity, edge case doesn't justify it

### 3. Wake Immediately if Loop Exceeds Interval
- Skip sleep entirely, show next image immediately
- **Rejected:** Could create power-draining rapid cycles in poor network conditions

### 4. Document Drift Instead of Fixing
- Update docs to say "interval is sleep time only, not cycle time"
- **Rejected:** Unintuitive user experience, doesn't match "refresh interval" terminology

## Implementation Notes

### Code Changes Required

1. **`common/src/power_manager.cpp`:**
   - Modify `enterDeepSleep(uint16_t, unsigned long)` - compensate sleep duration
   - Modify `enterDeepSleep(float, unsigned long)` - compensate sleep duration
   - Update logging to show compensation details

2. **Testing:**
   - Verify 1-minute interval results in ~60s cycles (not 67s)
   - Verify edge case (90s loop with 60s interval) sleeps 60s
   - Verify button-only mode (interval=0) unaffected
   - Verify carousel mode with mixed intervals

### Rollout Considerations

- **User impact:** Minimal - devices will refresh slightly faster than before
- **Migration:** None needed - change is transparent to users
- **Communication:** Mention in changelog as timing accuracy improvement

## Related Documents

- `docs/dev/ADR-HOURLY_SCHEDULING.MD` - Hourly wake windows require accurate timing
- `docs/dev/ADR-CRC32_GUIDE.MD` - Battery calculations assume accurate intervals
- `docs/user/USING.md` - Battery estimator documentation

## Future Considerations

- Could expose actual vs configured cycle time in MQTT telemetry
- Could warn users (via log) when loop time consistently exceeds interval
- Could add web portal metric showing average loop time over last N cycles
