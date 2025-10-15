# Testing Image Download Retry Mechanism

This document describes how to test the retry mechanism for image download failures.

## Test Setup

1. Build and flash the firmware to your Inkplate device
2. Configure the device with WiFi credentials and an image URL
3. Use the Serial Monitor to observe retry behavior

## Test Cases

### Test 1: Invalid URL (Network Error)

**Setup:**
- Configure an invalid image URL (e.g., `https://invalid.example.com/image.png`)
- This will trigger a network error

**Expected Behavior:**
1. First attempt: Download fails, device sleeps for 20 seconds (no error display)
2. Second attempt (after 20s): Download fails again, device sleeps for 20 seconds (no error display)
3. Third attempt (after 20s): Download fails again, device shows error message and sleeps for full refresh rate

**Serial Output to Look For:**
```
Failed to download/display image!
Retry count: 0
Will retry in 20 seconds (attempt 2 of 3)
Entering deep sleep for 0.33 minutes...
```

Then after 20 seconds:
```
Failed to download/display image!
Retry count: 1
Will retry in 20 seconds (attempt 3 of 3)
Entering deep sleep for 0.33 minutes...
```

Then after 20 seconds:
```
Failed to download/display image!
Retry count: 2
Max retries reached - showing error message
```

### Test 2: Intermittent Network (Success on Retry)

**Setup:**
- Start with WiFi disconnected or unstable
- Use a valid image URL
- After first or second retry, restore WiFi connection

**Expected Behavior:**
1. First attempt: Download fails, device sleeps for 20 seconds
2. Second attempt: Download succeeds (if WiFi restored), shows image, sleeps for full refresh rate
3. Retry count is reset to 0

**Serial Output to Look For:**
```
Failed to download/display image!
Retry count: 0
Will retry in 20 seconds (attempt 2 of 3)
```

Then after WiFi is restored and 20 seconds pass:
```
Image displayed successfully!
[Reset retry count to 0]
```

### Test 3: Valid URL (Immediate Success)

**Setup:**
- Configure a valid image URL
- Ensure WiFi is working

**Expected Behavior:**
1. Download succeeds on first attempt
2. Retry count is reset to 0
3. Device sleeps for full refresh rate

**Serial Output to Look For:**
```
Image displayed successfully!
[Retry count reset to 0]
Entering deep sleep for <refresh_rate> minutes...
```

### Test 4: Format Error (Invalid Image)

**Setup:**
- Configure a URL that returns a non-PNG file or invalid PNG

**Expected Behavior:**
- Same as Test 1 (3 attempts with 20 second delays, then error message)

## Verification Points

- [ ] Retry count persists across deep sleep cycles (check Serial output shows incrementing count)
- [ ] Device sleeps for exactly 20 seconds between retries (not the full refresh rate)
- [ ] No error message is displayed during first 2 retry attempts
- [ ] Error message is displayed after 3rd failed attempt
- [ ] Retry count is reset to 0 on success
- [ ] After max retries, device sleeps for full refresh rate

## Serial Monitor Configuration

- Baud rate: 115200
- Monitor immediately after flash to see boot messages
- Look for retry count messages to verify behavior

## Notes

- RTC memory retains values across deep sleep but is cleared on power cycle or reset
- To test from scratch, do a full power cycle or press reset button
- The retry mechanism treats all error types the same (network, HTTP, format errors)
