# Architecture Decision Record: On-Image Overlay System

## Status
Implemented (v1.7.0)

## Context

Users need at-a-glance visibility of device status (battery level, last update time, performance metrics) without requiring separate screens or relying on Home Assistant integration. Battery-powered operation makes status visibility critical for monitoring device health.

## Decision

Implement a configurable on-image overlay system that renders status information on top of dashboard images after download but before display refresh.

## Design

### Components

#### OverlayManager (`overlay_manager.h/cpp`)
Central overlay rendering engine responsible for compositing status information onto the display buffer.

**Key Features:**
- Battery icon with proportional fill level
- Battery percentage text
- Update time (24-hour format, timezone-aware)
- Cycle time (performance debugging)
- Configurable position, size, and color
- Rotation-aware positioning

#### Integration Points

1. **ImageManager**: After successful image display, calls `overlayManager->renderOverlay()` with current configuration and telemetry data
2. **UIBase**: Provides `drawBatteryIconBottomLeft()` for logo screens (splash, setup, manual refresh)
3. **Configuration**: 8 NVS preferences store overlay settings

### Configuration Options

#### Position (4 options)
- `OVERLAY_POS_TOP_LEFT` (0)
- `OVERLAY_POS_TOP_RIGHT` (1)
- `OVERLAY_POS_BOTTOM_LEFT` (2)
- `OVERLAY_POS_BOTTOM_RIGHT` (3)

**Rationale:** Four corners provide maximum flexibility while keeping implementation simple. Overlay respects configured screen rotation.

#### Size (3 options)
- `OVERLAY_SIZE_SMALL` (0) - FreeSans7pt7b (~8px icon height)
- `OVERLAY_SIZE_MEDIUM` (1) - Roboto_Regular12pt7b (~12px icon height)
- `OVERLAY_SIZE_LARGE` (2) - Roboto_Bold20pt7b (~20px icon height)

**Rationale:** 
- Small: Compact, minimal intrusion (recommended for Inkplate 2)
- Medium: Default, balanced visibility/size
- Large: High visibility (recommended for Inkplate 10)

#### Color (4 options)
- `OVERLAY_COLOR_BLACK` (0) - Grayscale value 0 (darkest)
- `OVERLAY_COLOR_DARK_GRAY` (1) - Grayscale value 2 (~30%)
- `OVERLAY_COLOR_LIGHT_GRAY` (2) - Grayscale value 5 (~70%)
- `OVERLAY_COLOR_WHITE` (3) - Grayscale value 7 (lightest)

**Rationale:** Uses 3-bit grayscale values (0-7) supported by Inkplate displays. Intermediate gray values provide flexibility for different image backgrounds. Direct mapping eliminates conversion overhead.

**Implementation Detail:** Color constants store config values (0-3), but rendering uses grayscale values (0, 2, 5, 7). This separation allows future expansion without breaking stored configurations.

#### Toggleable Elements (4 flags)
- `overlayShowBatteryIcon` (bool) - Battery icon with fill level
- `overlayShowBatteryPercentage` (bool) - Numeric percentage (e.g., "85%")
- `overlayShowUpdateTime` (bool) - Last update time (e.g., "14:23")
- `overlayShowCycleTime` (bool) - Cycle duration (e.g., "8.5s")

**Rationale:** Independent toggles allow users to show only desired information, keeping overlay compact.

### Rendering Architecture

#### Text Positioning
Uses **GFXfont baseline positioning** for accurate text alignment:
- Y coordinate is the baseline (not top-left)
- `getTextBounds()` provides bounding box with negative Y offset for ascenders
- Top positioning: `baselineY = overlayY - y1`
- Bottom positioning: `baselineY = overlayY + totalHeight - (textHeight - (-y1))`

**Challenge:** Bottom positioning requires careful baseline calculation to account for descenders while ensuring overlay doesn't extend beyond margin.

#### Battery Icon Design
Proportional battery icon with visual fill level:
- **Aspect ratio**: 3:5 (width:height) maintains recognizable shape at all sizes
- **Terminal width**: 
  - Small icons (height < 12px): 2px terminal
  - Medium/large icons: 4px terminal
- **Terminal height**: height / 2 (scales proportionally)
- **Rounded corners**: 4px radius for medium/large icons (more pronounced appearance)
- **Fill level**: Percentage-based fill with 2px padding

**Icon-Text Alignment:** Icon vertically centered with text cap height (not baseline) for balanced appearance:
```cpp
int textVisibleHeight = -y1;  // Cap height area
int iconY = baselineY - textVisibleHeight - ((iconHeight - textVisibleHeight) / 2);
```

#### Layout Calculation
1. Measure text bounds with `getTextBounds()`
2. Calculate icon dimensions from font height
3. Compute total overlay width: `iconWidth + iconPadding + textWidth`
4. Compute total overlay height: `textHeight` (actual bounding box height)
5. Calculate position based on configuration and margins
6. Render icon first, then text

### Battery Icon on Logo Screens

**Independent Feature:** All screens with the Inkplate logo (splash, setup screens, manual refresh) display a medium-sized black battery icon at bottom-left, **regardless of user overlay configuration**.

**Rationale:**
- Provides battery status visibility during device configuration
- Users need to know battery level when setting up device
- Logo screens are transient (user interaction contexts)
- Medium size balances visibility with available space
- Black color ensures visibility on light logo background
- Bottom-left position avoids overlap with logo (centered) and text content

**Implementation:**
- `UIBase::drawBatteryIconBottomLeft()` encapsulates rendering logic
- UIStatus and UIMessages inherit from UIBase
- Battery voltage passed as optional parameter to screen rendering functions
- OverlayManager's public `drawBatteryIcon()` method reused for consistency

### NVS Storage (8 keys)

```cpp
overlayEnabled           // bool (1 byte)
overlayPosition          // uint8_t (0-3)
overlaySize              // uint8_t (0-2)
overlayTextColor         // uint8_t (0-3)
overlayShowBattIcon      // bool (1 byte)
overlayShowBattPct       // bool (1 byte)
overlayShowUpdateTime    // bool (1 byte)
overlayShowCycleTime     // bool (1 byte)
```

**Total storage**: 8 bytes (minimal NVS footprint)

## Consequences

### Benefits

✅ **At-a-glance status visibility** - No need to check Home Assistant or serial logs  
✅ **Battery awareness** - Users can monitor battery level directly on display  
✅ **Performance monitoring** - Cycle time helps identify performance issues  
✅ **Flexible positioning** - Works with various image layouts  
✅ **Minimal intrusion** - Compact overlay doesn't obscure important image content  
✅ **Logo screen battery icon** - Configuration-time battery visibility  
✅ **DRY architecture** - UIBase eliminates code duplication  

### Trade-offs

⚠️ **Image obscuring** - Overlay covers a small portion of the image (user can position to minimize impact)  
⚠️ **Memory overhead** - Adds ~2KB code size for overlay logic  
⚠️ **Rendering complexity** - Baseline positioning and alignment logic adds complexity  
⚠️ **NVS usage** - Uses 8 additional NVS keys (out of namespace limit)  

### Alternatives Considered

#### 1. Separate Status Screen
**Rejected:** Requires extra screen refresh (reduces battery life), adds complexity to mode switching.

#### 2. Status Bar (Full Width)
**Rejected:** More intrusive, obscures more image content, doesn't work well with rotation.

#### 3. Blinking Icon
**Rejected:** E-ink refresh rate makes blinking impractical, increases power consumption.

## Implementation Notes

### Coordinate System
- Origin (0,0) is top-left corner
- X increases rightward
- Y increases downward
- Rotation applied by DisplayManager before overlay rendering

### Font Resources
Overlay uses shared font resources from `display_manager.h`:
- `FreeSans7pt7b` - Small size
- `Roboto_Regular12pt7b` - Medium size
- `Roboto_Bold20pt7b` - Large size

### Battery Percentage Calculation
Uses `calculateBatteryPercentage()` from `battery_logic.h`:
- LiPo discharge curve (4.2V → 3.2V)
- Voltage-to-percentage mapping
- 0-100% clamped range

### Timezone Support
Update time respects `timezoneOffset` configuration:
```cpp
time_t currentTime = time(nullptr);
currentTime += (config.timezoneOffset * 3600);
strftime(timeStr, sizeof(timeStr), "%H:%M", localtime(&currentTime));
```

## Testing

### Unit Tests
- Battery icon scaling (test/unit/test_overlay_logic.cpp)
- Position calculations (various screen sizes and rotations)
- Color mapping (config value → grayscale value)
- Text alignment edge cases

### Integration Tests
- Overlay rendering on all 4 board variants
- All position combinations
- All size combinations
- All color combinations
- Rotation combinations (0°, 90°, 180°, 270°)

### Manual Testing Checklist
- [ ] Battery icon visible at all sizes
- [ ] Text readable at all sizes
- [ ] Overlay positioned correctly at all corners
- [ ] Colors visible against light/dark images
- [ ] Rotation respected
- [ ] Logo screen battery icon appears correctly
- [ ] Timezone offset applied correctly
- [ ] Battery percentage accurate
- [ ] Cycle time reflects actual duration

## Future Enhancements

### Potential Improvements
- **Custom positioning** (X/Y coordinates instead of corners)
- **Transparency/opacity** options (requires grayscale blending)
- **Shadow/outline** for better visibility on varied backgrounds
- **Additional metrics** (WiFi signal strength, free memory, etc.)
- **Icon library** (more battery icon styles)
- **Custom fonts** (user-provided font files)

### Extensibility
The OverlayManager architecture supports easy addition of new overlay elements:
1. Add configuration field to DashboardConfig
2. Add NVS key for persistence
3. Add rendering logic in `renderOverlay()`
4. Add web UI control in ConfigPortal

## Related

- **PR #112** - Initial overlay system implementation
- **Issue #114** - UI screen rendering refactoring (DRY improvements)
- **ADR-ARCHITECTURE.MD** - Component architecture documentation
- **ADR-UNIT_TESTING.MD** - Unit testing pattern for overlay logic

## References

- GFXfont documentation: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
- Inkplate Library: https://github.com/SolderedElectronics/Inkplate-Arduino-library
- ESP32 NVS documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
