# ADR: Multi-Image Carousel Implementation

**Date:** 2025-10-24 (v1.0.0), Updated: 2025-11-13 (v1.5.0)  
**Status:** Implemented  
**Issues:** #54 (v1.0.0), #94 (v1.5.0 - Stay Flag)

## Context

Users requested the ability to display multiple images in a rotating carousel, with each image having its own display duration. The previous system only supported a single image URL with a global refresh rate, which was too limited for use cases like:
- Rotating between multiple dashboards (weather, calendar, family photos)
- Different content with different update frequencies (static logo for 60min, dynamic dashboard for 5min)
- More engaging displays that cycle through content

## Decision

We implemented a **multi-image carousel with per-image intervals** as a breaking change in v1.0.0. Key design decisions:

### 1. Configuration Storage
- **Fixed NVS keys** instead of JSON for simplicity and robustness
- Store up to **10 image URLs** using keys: `img_url_0` through `img_url_9`
- Store **per-image intervals** using keys: `img_int_0` through `img_int_9`
- Store **per-image stay flags** using keys: `img_stay_0` through `img_stay_9` (v1.5.0+)
- Store **image count** using key: `img_count` (uint8_t: 0-10)
- **Removed global `refreshRate`** - replaced with per-image intervals
- **Config version tracking** added (`cfg_ver`) for future upgrade detection

**Rationale:** Fixed keys are simpler than JSON parsing, have no size limits per URL, allow per-URL validation, and enable graceful degradation.

### 2. Auto-Detection (KISS Principle)
- **No explicit "carousel mode" flag** - behavior is implicit
- If `imageCount == 1` → Single image mode (existing behavior with retry logic)
- If `imageCount > 1` → Carousel mode (automatic rotation)
- If `imageCount == 0` → Not configured (error state)

**Rationale:** Removes unnecessary UI toggle and flag. Mode is self-evident from configuration.

### 3. Breaking Change Strategy
- **No upgrade/migration code** - clean slate approach
- Users must reconfigure after upgrading to v1.0.0
- Config version flag initialized on new installs but not used for migration
- Kept for future upgrade detection (V2 → V3)

**Rationale:** Simplicity. Migration code adds complexity and potential bugs. Since this is v1.0.0 from v0.x, a breaking change is acceptable. Users were informed in issue discussion.

### 4. RTC Memory Variable
- **Renamed** `imageRetryCount` → `imageStateIndex` for clarity
- **Dual purpose:**
  - In carousel mode: Tracks which image to display next (0-9)
  - In single image mode: Tracks retry attempts (0-2)
- **Updated across:** `main_sketch.ino.inc`, `normal_mode_controller.h`, `normal_mode_controller.cpp`

**Rationale:** The variable serves two purposes depending on mode. "StateIndex" is neutral and accurately describes both use cases.

### 5. Error Handling Strategy
**First Image Special Treatment:**
- In carousel mode, the **first image (index 0) uses retry logic** (3 attempts, 20-second intervals)
- Rationale: If the device can't display anything, it should try harder before giving up

**Non-First Images:**
- In carousel mode, images 1-9 **skip to next immediately** on failure (20-second sleep)
- Rationale: Keep carousel moving. One broken URL shouldn't halt rotation.

**Single Image Mode:**
- Keeps existing retry logic (3 attempts, 20 seconds apart, then show error screen)

### 6. CRC32 Behavior
- **Disabled in carousel mode** (`imageCount > 1`)
- Only enabled for single image mode
- Skip per-image CRC32 tracking complexity

**Rationale:** CRC32 optimization is for "check if same image changed" - doesn't make sense for carousel which expects rotation. Keeping implementation simple.

### 7. Configuration Portal UI
**Progressive Disclosure:**
- Show **first 2 image slots** by default (required + optional)
- "Add Another Image" button reveals slots 3-10 on demand
- Button hides when all 10 slots visible

**Form Fields per Image:**
- Image URL (required for slot 0, optional for 1-9)
- Display interval in minutes (required when URL provided, min 0 minutes)
- Stay checkbox (optional, v1.5.0+) - "Stay on this image (refresh interval still works, advance on button press)"
- Default interval: 5 minutes (auto-filled via JavaScript)
- Default stay: unchecked (false)

**JavaScript Features:**
- `addImageSlot()` function for progressive disclosure
- Auto-fill interval when URL entered
- Battery calculator uses average of all intervals
- Dynamic event listeners for recalculation

**Rationale:** Less overwhelming for users. Most will use 2-3 images. Advanced users can add more.

### 8. Battery Life Calculator
- **Calculates average interval** from all configured images
- Formula: `avgInterval = sum(all intervals) / imageCount`
- Used for battery life estimates (wakeups per day)
- CRC32 checkbox still works (applies only to single image mode)

**Rationale:** Simple and accurate. Carousel cycles through all images, so average interval represents typical refresh behavior.

### 9. Data Structure Changes

```cpp
// Old (v0.x)
struct DashboardConfig {
    String imageURL;        // Single URL
    int refreshRate;        // Global interval
    // ...
};

// New (v1.0.0, updated v1.5.0)
struct DashboardConfig {
    uint8_t imageCount;              // 0-10
    String imageUrls[10];            // Array of URLs
    int imageIntervals[10];          // Per-image intervals in minutes
    bool imageStay[10];              // Per-image stay flags (v1.5.0+)
    // ...
    
    bool isCarouselMode() const {
        return imageCount > 1;
    }
    
    int getAverageInterval() const {
        if (imageCount == 0) return DEFAULT_INTERVAL_MINUTES;
        int total = 0;
        for (uint8_t i = 0; i < imageCount; i++) {
            total += imageIntervals[i];
        }
        return total / imageCount;
    }
};
```

### 10. Validation Strategy
- **Validate strictly on form submission only**
- Trust stored data is valid (no re-validation on load)
- Checks:
  - At least 1 URL provided
  - Max 10 URLs
  - URL starts with `http://` or `https://`
  - URL length ≤ 250 characters
  - **Interval required and > 0 for each URL**
- Return clear error messages on validation failure

**Rationale:** Validate once at entry point. Faster boot, less redundant code.

## Implementation Details

### Files Modified (9 total)
1. **config_manager.h** - Updated struct, added constants, helper methods
2. **config_manager.cpp** - Updated save/load logic, removed old getters/setters
3. **main_sketch.ino.inc** - Renamed RTC variable
4. **normal_mode_controller.h** - Updated parameter name
5. **normal_mode_controller.cpp** - Core carousel logic, error handling, CRC32 skip
6. **config_mode_controller.cpp** - Replaced refreshRate with getAverageInterval()
7. **config_portal.cpp** - New form parsing, carousel validation
8. **config_portal_css.h** - CSS for image slots, JavaScript functions
9. **version.h** - Bumped to 1.0.0

### Key Code Sections

**Carousel Execution (normal_mode_controller.cpp):**
```cpp
// Get current image from carousel
uint8_t currentIndex = *imageStateIndex % config.imageCount;
String currentImageUrl = config.imageUrls[currentIndex];
int currentInterval = config.imageIntervals[currentIndex];

if (config.isCarouselMode()) {
    // Carousel mode
    if (success) {
        // v1.5.0: Check stay flag and wakeup reason
        bool shouldAdvance = false;
        if (wakeReason == WAKEUP_BUTTON) {
            shouldAdvance = true;  // Button always advances
        } else if (!config.imageStay[currentIndex]) {
            shouldAdvance = true;  // Timer + stay:false = auto-advance
        }
        // Only advance if shouldAdvance is true
        if (shouldAdvance) {
            *imageStateIndex = (currentIndex + 1) % config.imageCount;
        }
        // Sleep with current image's interval
    } else {
        if (currentIndex == 0) {
            // First image - retry logic
            if (*imageStateIndex < 2) {
                (*imageStateIndex)++;
                // 20-second retry
            } else {
                *imageStateIndex = 1; // Move to next
            }
        } else {
            // Skip to next
            *imageStateIndex = (currentIndex + 1) % config.imageCount;
            // 20-second sleep
        }
    }
} else {
    // Single image mode - existing retry logic
}
```

**Form Generation (config_portal.cpp):**
```cpp
// Always show first 2 slots
for (uint8_t i = 0; i < 2; i++) {
    html += "<div class='image-slot' id='slot_" + String(i) + "'>";
    html += "<input type='text' name='img_url_" + String(i) + "' " + (i==0 ? "required" : "") + ">";
    html += "<input type='number' name='img_int_" + String(i) + "' min='1'>";
    html += "</div>";
}

// Slots 3-10 hidden by default, revealed on demand
```

### Testing Considerations
- Single image mode with custom interval (verify retry logic)
- 2-image carousel with same intervals
- 5-image carousel with different intervals
- 10-image carousel (max limit)
- First image failure in carousel (verify retry)
- Non-first image failure (verify skip)
- Carousel position preserved across sleep cycles
- Battery calculator accuracy with mixed intervals
- CRC32 disabled in carousel mode
- Form validation for missing intervals
- Progressive disclosure UI behavior

## Consequences

### Positive
- ✅ **More flexible**: Users can now display multiple images with different durations
- ✅ **Advanced carousel control** (v1.5.0): Per-image stay flags enable hybrid workflows with multiple pause points
- ✅ **Simpler mental model**: Each image has its own "display time"
- ✅ **Better UX**: Progressive disclosure makes form less overwhelming
- ✅ **Clean implementation**: No migration code, no dual-format support
- ✅ **Future-proof**: Config version tracking enables future migrations
- ✅ **Backward compatible stay flags** (v1.5.0): Default false preserves classic auto-advance behavior

### Negative
- ⚠️ **Breaking change**: All users must reconfigure after v1.0.0 upgrade
- ⚠️ **No CRC32 in carousel**: Users with carousel can't benefit from power savings
- ⚠️ **More complex error handling**: First image vs non-first image logic

### Neutral
- 🔄 **Battery life estimate**: Uses average interval (close enough for mixed durations)
- 🔄 **Hourly schedule**: Works with per-image intervals (respects schedule boundaries)

## Alternatives Considered

### 1. JSON-based Configuration
**Rejected:** More complex, size limits, harder to validate, slower parsing.

### 2. Global Refresh Rate + Per-Image Overrides
**Rejected:** Confusing mental model. "Base rate + override" vs "per-image duration" is less intuitive.

### 3. Migration Code (V1 → V2 Upgrade)
**Rejected:** Adds complexity for a major version bump. Clean break is acceptable for v1.0.0.

### 4. CRC32 Per-Image in Carousel
**Rejected:** Complex to implement. Carousel implies rotation (users expect all images to show).

### 5. Single Retry Logic for All Carousel Images
**Rejected:** If first image fails repeatedly, display shows nothing. Better to try harder on first image.

## References

- Issue: #54 - Feature: Multi-Image Carousel Support with Per-Image Intervals (v1.0.0)
- Issue: #94 - Add per-image "stay" flag for advanced carousel control (v1.5.0)
- Related: ADR-HOURLY_SCHEDULING.MD - Hourly schedule compatibility
- Related: ADR-CRC32_GUIDE.MD - CRC32 implementation details

## Notes

- Config version flag (`cfg_ver`) initialized but not used for migration in this release
- Future versions (V2, V3) can use `cfg_ver` to detect old formats and migrate gracefully
- Battery life calculator remains accurate for mixed intervals (uses average)
- Hourly schedule still works: if interval exceeds remaining active time, truncates to schedule boundary
