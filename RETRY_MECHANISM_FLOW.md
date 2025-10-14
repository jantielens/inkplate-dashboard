# Image Download Retry Mechanism Flow

This document illustrates the flow of the retry mechanism for image download failures.

## Flow Diagram

```
┌─────────────────────────────────────┐
│  Device Wakes Up (Timer/Button)    │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  Connect to WiFi & Download Image   │
└──────────────┬──────────────────────┘
               │
         ┌─────┴─────┐
         │           │
    SUCCESS?      FAILURE?
         │           │
         ▼           ▼
    ┌────────┐  ┌──────────────────┐
    │ RESET  │  │ Check Retry Count│
    │ Count  │  └────────┬─────────┘
    │ to 0   │           │
    └───┬────┘     ┌─────┴──────┐
        │          │            │
        │    Retry < 2?   Retry >= 2?
        │          │            │
        │          ▼            ▼
        │    ┌──────────┐  ┌──────────────┐
        │    │INCREMENT │  │  SHOW ERROR  │
        │    │  COUNT   │  │   MESSAGE    │
        │    └────┬─────┘  │  RESET COUNT │
        │         │        │    to 0      │
        │         ▼        └──────┬───────┘
        │    ┌──────────┐         │
        │    │  SLEEP   │         ▼
        │    │ 20 sec   │    ┌──────────┐
        │    └────┬─────┘    │  SLEEP   │
        │         │          │  FULL    │
        │         │          │  RATE    │
        │         └──────┬───┴──────────┘
        │                │
        ▼                ▼
    ┌────────────────────────┐
    │   Display Image        │
    │   Show Success Info    │
    │   SLEEP Full Rate      │
    └────────────────────────┘
```

## Retry Count State Machine

```
State 0 (imageRetryCount = 0)
  ↓ (failure)
  Increment → imageRetryCount = 1
  Sleep 20s
  
State 1 (imageRetryCount = 1)
  ↓ (failure)
  Increment → imageRetryCount = 2
  Sleep 20s
  
State 2 (imageRetryCount = 2)
  ↓ (failure)
  Show Error → imageRetryCount = 0
  Sleep Full Rate
  
Any State
  ↓ (success)
  Reset → imageRetryCount = 0
  Sleep Full Rate
```

## Example Timeline

### Scenario: Intermittent Network Issue (Resolves on 2nd Retry)

```
Time    Event                           Retry Count    Action
─────────────────────────────────────────────────────────────────
00:00   Wake up (timer)                 0              Try download
00:05   Download fails (network)        0 → 1          Sleep 20s (no error shown)
00:25   Wake up (timer, 20s)            1              Try download
00:30   Download fails (network)        1 → 2          Sleep 20s (no error shown)
00:50   Wake up (timer, 20s)            2              Try download
00:55   Download succeeds!              2 → 0          Display image, sleep full rate
05:55   Wake up (timer, 5 min)          0              Normal operation resumes
```

### Scenario: Persistent Error (Invalid URL)

```
Time    Event                           Retry Count    Action
─────────────────────────────────────────────────────────────────
00:00   Wake up (timer)                 0              Try download
00:05   Download fails (404)            0 → 1          Sleep 20s (no error shown)
00:25   Wake up (timer, 20s)            1              Try download
00:30   Download fails (404)            1 → 2          Sleep 20s (no error shown)
00:50   Wake up (timer, 20s)            2              Try download
00:55   Download fails (404)            2 → 0          Show error, sleep full rate
05:55   Wake up (timer, 5 min)          0              Try again (3 attempts)
...     [Cycle repeats]
```

## Key Features

1. **Silent Retries**: No error message or display update during first 2 retries
2. **Short Sleep**: Only 20 seconds between retries (vs. full refresh rate)
3. **RTC Persistence**: Retry count survives deep sleep cycles
4. **Automatic Reset**: Count resets after showing error or on success
5. **Universal**: Works for all error types (network, HTTP, format)

## Implementation Notes

- RTC memory variable: `RTC_DATA_ATTR uint8_t imageRetryCount = 0;`
- Sleep duration for retry: `20.0 / 60.0` minutes (0.33 minutes = 20 seconds)
- Maximum retries: 2 (total 3 attempts including initial)
- Error display: Only shown after 3rd failed attempt
