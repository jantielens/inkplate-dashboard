# Screen Rotation Performance Optimization

## Overview

This document describes the performance optimization implemented for screen rotation functionality, which reduces the normal update cycle time by approximately 1 second.

## Problem

The initial rotation implementation applied screen rotation globally during `displayManager.init()`. This caused ALL subsequent text rendering operations to undergo coordinate transformation for every character drawn, adding significant overhead (~1 second) to the update cycle.

**Timing Impact:**
- Without rotation: ~6 seconds for manual refresh
- With global rotation: ~7 seconds for manual refresh
- With selective rotation: ~6 seconds for manual refresh ✅

## Solution: Selective Rotation

The optimization implements **selective rotation** where rotation is only enabled for screens that users **must** be able to read, while skipping rotation for transient status messages that are quickly replaced by the dashboard image.

### Implementation

#### 1. DisplayManager State Management

Added rotation state tracking to `DisplayManager`:

```cpp
class DisplayManager {
private:
    uint8_t _configuredRotation = 0;  // User's configured rotation
    uint8_t _currentRotation = 0;     // Currently active rotation
    
public:
    void enableRotation();   // Apply user's configured rotation
    void disableRotation();  // Set to 0 for performance
};
```

**Key Behavior:**
- `init()` stores the configured rotation but starts with rotation disabled (0°)
- `enableRotation()` applies the user's configured rotation
- `disableRotation()` sets rotation to 0 for performance
- State caching avoids redundant `setRotation()` calls

#### 2. Essential Screens (Rotation Enabled)

These screens **require rotation** because users need to read them:

**Error Screens (UIError):**
- `showWiFiError()` - WiFi connection failures
- `showImageError()` - Image download/display failures
- `showAPStartError()` - Access Point startup failures
- `showPortalError()` - Configuration portal failures
- `showConfigLoadError()` - Configuration loading failures
- `showConfigModeFailure()` - Config mode entry failures
- `showConfigInitError()` - Configuration initialization failures

**Setup Screens (UIStatus):**
- `showAPModeSetup()` - Initial setup instructions with AP credentials
- `showConfigModeSetup()` - Configuration portal URL display
- `showConfigModePartialSetup()` - Step 2 setup screen
- `showConfigModeConnecting()` - WiFi connection during setup
- `showConfigModeWiFiFailed()` - WiFi failure during config mode
- `showConfigModeAPFallback()` - Fallback to AP mode
- `showConfigModeTimeout()` - Config mode timeout notification

**Status/Confirmation Screens (UIStatus, UIMessages):**
- `showWiFiConfigured()` - WiFi configuration success
- `showSettingsUpdated()` - Settings update confirmation
- `showManualRefresh()` - Manual button press acknowledgment
- `showAPModeStarting()` - First boot AP mode starting message
- `showSplashScreen()` - Boot splash with board name and dimensions

**Rationale:** Users physically interact with the device during setup/errors and MUST read these instructions. These screens persist until user action completes. For manual refresh and first boot, the user is actively looking at the device and expects readable feedback. Splash screen appears during first boot or debug mode when user is observing the device startup.

**Transient Screens (Rotation Disabled)

These screens **skip rotation** for performance:

**Debug/Status Messages:**
- `showDebugStatus()` - Brief debug info overlay (debug mode only)
- `showDownloading()` - Download progress message

**Rationale:** These messages appear very briefly and are immediately replaced by the dashboard image. The 1-second rotation overhead would be noticeable for such transient content that users don't need to read in detail.

### Code Pattern

**Essential Screen (with rotation):**
```cpp
void UIError::showWiFiError(...) {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    // ... render content ...
    displayManager->refresh();
}
```

**Transient Screen (without rotation):**
```cpp
void UIStatus::showDebugStatus(...) {
    // Transient debug message - skip rotation for performance
    // This screen appears briefly before image download in debug mode
    
    // NO enableRotation() call - stays at 0°
    displayManager->showMessage(...);
    displayManager->refresh();
}
```

**User-Initiated Screen (with rotation):**
```cpp
void UIStatus::showManualRefresh() {
    // Enable rotation for manual refresh acknowledgment
    // User intentionally triggered this, so they're likely looking at the screen
    displayManager->enableRotation();
    
    displayManager->clear();
    // ... render content ...
    displayManager->refresh();
}
```

## Performance Impact

### Before Optimization
- All text rendering used configured rotation
- Per-character coordinate transformations
- Normal update: ~7 seconds

### After Optimization
- Most screens: Rotation enabled for readability
- Only 2 transient screens skip rotation: `showDebugStatus()` and `showDownloading()`
- **Normal operation with debug mode OFF: ~6 seconds** (typical case - no debug screens shown)
- **Normal operation with debug mode ON: ~7 seconds** (debug status + downloading messages shown)

### Why This Works

**Normal operation flow (debug mode OFF):**
1. Wake from deep sleep
2. Download and display image (no rotation for image) - **fast**
3. Go to sleep
4. **Result: ~6 seconds** (no text screens shown at all)

**Normal operation flow (debug mode ON):**
1. Wake from deep sleep
2. Show transient debug status (no rotation) - **fast**
3. Show downloading message (no rotation) - **fast**
4. Download and display image (no rotation for image) - **fast**
5. Go to sleep
6. **Result: ~6 seconds** (transient screens skip rotation)

**Error flow:**
1. Wake from deep sleep
2. Attempt WiFi connection
3. **Fail** → Show WiFi error (rotation enabled) - ~7 seconds
4. User needs to read error → rotation is essential
5. Go to sleep

**Setup flow:**
1. First boot or config mode
2. Show splash screen (rotation enabled) - readable
3. Show setup instructions (rotation enabled) - readable
4. User follows instructions → rotation is essential
5. Wait for user configuration

**Manual refresh flow:**
1. User presses button
2. Show manual refresh message (rotation enabled) - readable
3. Download and display image
4. Go to sleep

**Key Insight:** Only the two most transient screens (`showDebugStatus()` and `showDownloading()`) skip rotation. All other screens are rotated for optimal user experience. Normal operation without debug mode shows NO text screens, making it fast (~6s). With debug mode, the brief transient screens skip rotation to maintain speed.

## Image Rendering

Images continue to use the existing performance optimization where rotation is temporarily disabled during `drawImage()`:

```cpp
// In image_manager.cpp
uint8_t currentRotation = displayManager->getRotation();
displayManager->disableRotation();  // Set to 0 for image
_display->drawImage(url, 0, 0, true, false);
displayManager->setRotation(currentRotation);  // Restore for UI
```

This ensures images are never rotated on-device (too slow), relying on user-provided pre-rotated images.

## Trade-offs

### Advantages
✅ Normal operation remains fast (~6s without debug mode)
✅ Nearly all screens are readable at correct orientation
✅ Minimal code complexity
✅ Only 2 extremely transient screens skip rotation

### Disadvantages
❌ Two debug screens (`showDebugStatus()`, `showDownloading()`) at 0° regardless of rotation setting
❌ Slight inconsistency in debug mode (most rotated, 2 not)

### Rationale for Trade-offs

The disadvantages are minimal and acceptable because:
1. **Only 2 screens skip rotation** - All user-facing, setup, error, and confirmation screens ARE rotated
2. **Non-rotated screens are extremely brief** - Only shown for 1-2 seconds in debug mode
3. **Normal operation (no debug) shows no text screens** - Goes straight to image display
4. **Debug mode is opt-in** - Users enabling debug mode accept transient status messages
5. **Image display is the primary function** - Dashboard image is always correctly oriented

## Testing Recommendations

1. **Normal operation** (expected: ~6s)
   - Configure device with image URL
   - Trigger manual refresh (short button press)
   - Verify fast update cycle

2. **Error screens** (expected: readable at configured orientation)
   - Disconnect WiFi → verify `showWiFiError()` is rotated
   - Invalid image URL → verify `showImageError()` is rotated

3. **Setup flow** (expected: readable at configured orientation)
   - Factory reset → verify AP mode instructions are rotated
   - Config mode → verify portal URL is rotated

4. **Debug mode** (expected: fast but unrotated status)
   - Enable debug mode
   - Verify `showDebugStatus()` appears briefly (may be 0° orientation)
   - Verify image displays correctly

## Future Enhancements

Possible improvements if needed:

1. **Profile actual timing** - Measure exact overhead per screen
2. **User preference** - Add "Rotate all screens" option for users who prefer consistency over speed
3. **Hardware rotation** - Investigate if Inkplate hardware supports native rotation (unlikely)
4. **Pre-rendered text bitmaps** - Cache common messages as rotated bitmaps

## Related Documentation

- `SCREEN_ROTATION_IMPLEMENTATION.md` - Original feature implementation
- `CHANGELOG.md` - Version 0.11.0 notes about performance optimization
- `USING.md` - User-facing documentation about screen rotation and pre-rotated images
