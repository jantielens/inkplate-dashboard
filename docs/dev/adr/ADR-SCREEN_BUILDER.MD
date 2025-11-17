# Architecture Decision Record: Screen Builder Pattern

## Status
Implemented (v1.7.1)

## Context

Prior to v1.7.1, UI screens required manual Y-position tracking and repetitive code patterns:
- Manual `currentY` variable management in every screen function
- Inconsistent spacing between elements
- Code duplication across 21 UI screens
- No standardized logo/battery icon rendering
- Verbose API requiring multiple method calls per screen

Example of old pattern:
```cpp
void UIStatus::showConfigModeSetup(..., float batteryVoltage) {
    displayManager->clear();
    displayManager->enableRotation();
    
    int currentY = MARGIN;
    
    // Draw logo
    int logoX = (SCREEN_WIDTH - LOGO_WIDTH) / 2;
    displayManager->drawBitmap(logo_bitmap, logoX, currentY, LOGO_WIDTH, LOGO_HEIGHT);
    currentY += LOGO_HEIGHT + MARGIN;
    
    // Draw heading
    displayManager->showMessage("Config Mode Active", MARGIN, currentY, FONT_HEADING1);
    currentY += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING;
    
    // Draw text
    displayManager->showMessage("Open browser to:", MARGIN, currentY, FONT_NORMAL);
    currentY += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    displayManager->showMessage(localIP, MARGIN, currentY, FONT_NORMAL);
    currentY += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    // Draw battery icon
    if (batteryVoltage > 0.0) {
        drawBatteryIconBottomLeft(batteryVoltage);
    }
    
    displayManager->refresh();
}
```

## Decision

Implement a declarative Screen builder class that:
1. Eliminates manual Y-position tracking via automatic layout
2. Provides fluent API for building screens
3. Enables logo and battery icons by default
4. Automatically adds LINE_SPACING after content elements
5. Reduces screen setup code by ~60%

## Design

### Screen Builder Class

**Location:** `common/src/ui/screen.h/cpp`

**Inheritance:** Extends `UIBase` to inherit `overlayManager` and `drawBatteryIconBottomLeft()` functionality

**Constructor Signature:**
```cpp
Screen(DisplayManager* displayManager, 
       OverlayManager* overlayMgr = nullptr, 
       float batteryVoltage = 0.0f)
```

**Key Design Principles:**

1. **Default-On Logo and Battery:**
   - Logo enabled by default (`_showLogo = true`)
   - Battery enabled by default if `overlayMgr != nullptr && batteryVoltage > 0.0f`
   - Logo drawn immediately in constructor (updates `_currentY`)
   - Battery drawn in `display()` method (before refresh)

2. **Automatic Y-Position Tracking:**
   - Constructor initializes `_currentY = MARGIN`
   - Logo drawing updates `_currentY` to position content below
   - Content methods automatically increment `_currentY` by font height + LINE_SPACING
   - No manual position management required by caller

3. **Automatic Spacing:**
   - All content methods (`addHeading1`, `addHeading2`, `addText`, etc.) add `LINE_SPACING` automatically
   - Manual `addSpacing()` only needed for intentional extra gaps (e.g., `LINE_SPACING * 2` for visual hierarchy)
   - Eliminates 21 redundant spacing calls across codebase

4. **Fluent API:**
   - All methods return `*this` for chaining
   - Constructor-based configuration (battery voltage as 3rd parameter)
   - Opt-out methods: `withoutLogo()`, `withoutBattery()`

### API Design Evolution

**Version 1 (Old - Verbose):**
```cpp
// 6-7 lines per screen
Screen screen(displayManager, overlayManager);
screen.withLogo().withBattery(batteryVoltage);
screen.addHeading1("Config Mode");
screen.addSpacing(LINE_SPACING);  // Redundant!
screen.addText("Open browser to:");
screen.addSpacing(LINE_SPACING);  // Redundant!
screen.display();
```

**Version 2 (New - Concise):**
```cpp
// 2-3 lines per screen
Screen(displayManager, overlayManager, batteryVoltage)
    .addHeading1("Config Mode")
    .addText("Open browser to:")
    .display();
```

### Configuration Methods

#### `withoutLogo()`
Disables logo rendering. Useful for OTA screens or minimal displays.

**Note:** Logo is already drawn in constructor, so this method mainly documents intent.

#### `withoutBattery()`
Disables battery icon rendering at bottom-left.

#### `withRotation()` / `withoutRotation()`
Control screen rotation. Default: enabled.

### Content Methods

All content methods automatically add `LINE_SPACING` after rendering:

#### `addHeading1(const String& text)`
Large heading using `FONT_HEADING1`.

**Layout:** `_currentY += fontHeight + LINE_SPACING`

#### `addHeading2(const String& text)`
Medium heading using `FONT_HEADING2`.

**Layout:** `_currentY += fontHeight + LINE_SPACING`

#### `addText(const String& text)`
Body text using `FONT_NORMAL`.

**Layout:** `_currentY += fontHeight + LINE_SPACING`

#### `addSpacing(int pixels = LINE_SPACING)`
Manual spacing for intentional gaps.

**Use case:** Visual hierarchy (e.g., `addSpacing(LINE_SPACING * 2)` to separate sections)

#### `addKeyValue(const String& key, const String& value)`
Renders "Key: Value" format.

**Layout:** `_currentY += fontHeight + LINE_SPACING`

#### `addNumberedItem(uint8_t number, const String& text)`
Renders "1. Text" format.

**Layout:** `_currentY += fontHeight + LINE_SPACING`

### Render Method

#### `display()`
Finalizes screen rendering:
1. Draws battery icon if `_showBattery` is true
2. Calls `displayManager->refresh()`

**Battery Rendering:** Uses inherited `drawBatteryIconBottomLeft()` from UIBase.

### Private Methods

#### `drawLogo()`
Centers logo horizontally, positions at top margin, updates `_currentY`.

**Algorithm:**
```cpp
int logoX = MARGIN + (maxLogoX - minLogoX) / 2;  // Center horizontally
int logoY = MARGIN;  // Top margin
displayManager->drawBitmap(logo_bitmap, logoX, logoY, LOGO_WIDTH, LOGO_HEIGHT);
_currentY = logoY + LOGO_HEIGHT + MARGIN;  // Position content below
```

#### `drawBattery()`
Delegates to `UIBase::drawBatteryIconBottomLeft()` with stored `_batteryVoltage`.

#### `ensureLineHeight(const GFXfont* font)`
Ensures display has minimum vertical space for font. Used internally for layout calculations.

## Implementation

### Files Modified

**Core Screen Builder:**
- `common/src/ui/screen.h` - Class declaration with updated API
- `common/src/ui/screen.cpp` - Implementation with default-on logic

**21 Screen Functions Updated:**
- `common/src/ui/ui_status.cpp` - 10 status screens
- `common/src/ui/ui_error.cpp` - 6 error screens
- `common/src/ui/ui_messages.cpp` - 2 message screens
- `common/src/config_portal.cpp` - 3 OTA screens (logo only, no battery)

### Code Metrics

**Before (v1.7.0):**
- Average screen: 6-7 lines of code
- Manual Y-position tracking in every function
- 21 redundant `addSpacing(LINE_SPACING)` calls
- 42 method calls: `.withLogo()` + `.withBattery()` on each screen

**After (v1.7.1):**
- Average screen: 2-3 lines of code
- Automatic Y-position tracking
- 0 redundant spacing calls (kept 6 intentional `LINE_SPACING * 2` for hierarchy)
- 0 method calls for logo/battery (constructor-based)

**Net Reduction:**
- ~300 lines of code eliminated
- ~60% reduction in screen setup complexity
- Eliminated 21 redundant spacing calls
- Eliminated 42 method calls

## Consequences

### Benefits

✅ **Reduced Code Duplication** - Centralized Y-position tracking eliminates copy-paste errors  
✅ **Improved Readability** - Declarative API makes screen structure clear  
✅ **Automatic Spacing** - No need to manually add LINE_SPACING after every element  
✅ **Default-On Convenience** - Logo and battery icons enabled by default (matches common case)  
✅ **60% Code Reduction** - Simpler screen setup (from 6-7 lines to 2-3 lines)  
✅ **Type Safety** - Method chaining provides compile-time validation  
✅ **Flexibility** - Opt-out methods available for special cases  

### Trade-offs

⚠️ **Learning Curve** - New developers need to understand builder pattern  
⚠️ **Constructor Complexity** - 3 parameters (but with sensible defaults)  
⚠️ **Logo Drawn Eagerly** - Logo drawn in constructor (can't conditionally skip after construction)  
⚠️ **Memory Overhead** - ~1KB additional code size for Screen class  

### Alternatives Considered

#### 1. Separate withLogo() and withBattery() Methods (Rejected)
**Rejected:** Verbose API requiring 2 extra method calls per screen. 60% of screens need both, making default-on more ergonomic.

#### 2. Smart Auto-Detection (Rejected)
**Rejected:** Heuristics (e.g., "show battery if voltage > 3.0V") are brittle and hard to test. Explicit configuration clearer.

#### 3. Preset Configurations (Rejected)
**Rejected:** Multiple constructors (e.g., `ScreenWithLogo`, `ScreenMinimal`) create API proliferation. Single constructor with defaults more flexible.

## Usage Examples

### Standard Screen (Logo + Battery)
```cpp
void UIStatus::showConfigModeSetup(const char* localIP, float batteryVoltage) {
    Screen(displayManager, overlayManager, batteryVoltage)
        .addHeading1("Config Mode Active")
        .addText("Open browser to:")
        .addText(localIP)
        .display();
}
```

### OTA Screen (Logo Only, No Battery)
```cpp
void ConfigPortal::showOTASuccess() {
    Screen(_displayManager)  // No overlayManager, no battery
        .addHeading1("Update Complete!")
        .addText("Restarting...")
        .display();
}
```

### Screen Without Logo
```cpp
void UIStatus::showMinimalStatus(float batteryVoltage) {
    Screen(displayManager, overlayManager, batteryVoltage)
        .withoutLogo()
        .addText("Minimal status screen")
        .display();
}
```

### Screen With Intentional Extra Spacing
```cpp
void UIStatus::showWiFiConfigured(float batteryVoltage) {
    Screen(displayManager, overlayManager, batteryVoltage)
        .addHeading1("WiFi Configured!")
        .addSpacing(LINE_SPACING * 2)  // Visual hierarchy
        .addText("Restarting...")
        .display();
}
```

## Testing

### Build Validation
All 4 board types compiled successfully:
- Inkplate 2 (104x212, minimal UI)
- Inkplate 5V2 (720x540)
- Inkplate 6 Flick (800x600)
- Inkplate 10 (1024x758)

### Hardware Testing
Tested on Inkplate 5V2:
- Logo positioning correct (centered at top)
- Battery icon displayed at bottom-left
- Automatic spacing consistent across screens
- Intentional extra spacing (LINE_SPACING * 2) creates clear visual hierarchy

## Future Enhancements

1. **Centered Text:** Add `.addTextCentered()` for centered body text
2. **Horizontal Rules:** Add `.addDivider()` for visual separation
3. **Two-Column Layout:** Support side-by-side content (e.g., key-value pairs)
4. **Custom Margins:** Allow override of default MARGIN constant
5. **Progress Indicators:** Built-in progress bar rendering

## Related ADRs

- **ADR-OVERLAY_SYSTEM.MD** - Documents UIBase battery icon feature
- **ADR-ARCHITECTURE.MD** - UI components architecture overview
- **ADR-UNIT_TESTING.MD** - Testing pattern (Screen builder not unit-tested due to DisplayManager dependency)

## References

- Issue #114: Screen builder pattern implementation
- Commit 024cf3d: Add battery icons to remaining screens
- Commit d52c379: Simplify Screen API to default-on
- Commit f8bae5d: Remove redundant addSpacing calls
- Commit 1bbd909: Version bump to 1.7.1
