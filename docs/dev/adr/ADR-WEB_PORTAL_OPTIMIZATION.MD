# Architecture Decision Record: Web Portal Resource Separation

## Status
Accepted

## Context

The configuration web portal serves HTML pages with embedded CSS and JavaScript. The ESP32 has severe memory constraints:
- String class buffer overflow occurs around 50-60KB during concatenation
- Original HTML page size was ~60KB (HTML + inline CSS + inline JavaScript)
- ESP32 WebServer concatenates strings in RAM, causing memory corruption
- Browser cannot cache embedded resources, requiring full download on every page load

### Initial Problem

When generating HTML pages, the code concatenated large CSS (16KB) and JavaScript blocks into a single String object:
```cpp
html += CONFIG_PORTAL_CSS;  // 16KB CSS block
html += CONFIG_PORTAL_*_SCRIPT;  // Multiple JS blocks
```

This caused:
1. **Memory corruption** - ESP32 String buffer overflow
2. **CSS rendering failure** - Truncated CSS due to overflow
3. **Poor performance** - No browser caching, ~60KB download per page
4. **Maintenance issues** - Hard to debug layout problems

## Decision

Split the web portal into separate endpoints serving HTML, CSS, and JavaScript as independent resources with proper MIME types and browser caching.

### Implementation

**1. CSS Separation (`/styles.css` endpoint)**
- Created `handleCSS()` method serving `CONFIG_PORTAL_CSS` constant
- MIME type: `text/css`
- Removed `<style>` wrapper tags from CSS constant (pure CSS only)
- All 8 HTML pages now use: `<link rel='stylesheet' href='/styles.css'>`

**2. JavaScript Separation (3 endpoints)**
- **`/scripts/main.js`** - Combines 4 scripts for main config page:
  - Modal dialog (factory reset confirmation)
  - Friendly name sanitization
  - Battery life calculator
  - Floating battery badge
- **`/scripts/ota.js`** - OTA update page script
- **`/scripts/ota-status.js`** - OTA progress monitoring script
- MIME type: `application/javascript`
- Removed `<script>` wrapper tags from all JavaScript constants (pure JS only)
- Pages use: `<script src='/scripts/main.js'></script>`

**3. Memory Optimization**
- Reduced `generateConfigPage()` memory reservation from 51200 to 36000 bytes
- Pre-allocated 20KB for combined `main.js` script concatenation
- Individual scripts served directly without concatenation

### Route Registration
```cpp
_server->on("/styles.css", [this]() { this->handleCSS(); });
_server->on("/scripts/main.js", [this]() { this->handleMainJS(); });
_server->on("/scripts/ota.js", [this]() { this->handleOTAJS(); });
_server->on("/scripts/ota-status.js", [this]() { this->handleOTAStatusJS(); });
```

### File Structure
```
common/src/
├── config_portal.h          # Handler method declarations
├── config_portal.cpp        # Route handlers and HTML generators
├── config_portal_css.h      # Pure CSS constant (no <style> tags)
├── config_portal_js.h       # Pure JavaScript constants (no <script> tags)
└── config_portal_html.h     # HTML fragments
```

## Consequences

### Positive

**Memory Safety:**
- HTML page size reduced from ~60KB to ~35-40KB
- CSS served separately: 15.3KB
- JavaScript served separately: varies by page
- No more ESP32 String buffer overflow
- Reduced memory reservation requirements

**Browser Performance:**
- CSS cached after first load (~15KB saved per page navigation)
- JavaScript cached after first load (varies by page)
- Faster page loads on subsequent visits
- Reduced WiFi bandwidth usage on ESP32

**Maintainability:**
- Clear separation of concerns (HTML/CSS/JS)
- Easier to debug CSS and JavaScript issues
- Standard web development practices
- Proper MIME types for each resource type

**Code Quality:**
- Removed duplicate `getCSS()` function
- Cleaner HTML generation code
- No HTML wrapper tags in CSS/JS constants
- Professional web architecture pattern

### Negative

**Complexity:**
- More route handlers to maintain (4 additional endpoints)
- CSS/JS must be loaded from constants (can't be modified without recompile)
- Main.js concatenates 4 scripts (slight overhead)

**Testing:**
- Must verify external resources load correctly
- Must test browser caching behavior
- Must ensure MIME types are correct

### Technical Details

**CSS Constant (config_portal_css.h):**
```cpp
const char* CONFIG_PORTAL_CSS = R"(
/* Pure CSS - no <style> tags */
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, ...;
}
)";
```

**JavaScript Constants (config_portal_js.h):**
```cpp
const char* CONFIG_PORTAL_MODAL_SCRIPT = R"(
// Pure JavaScript - no <script> tags
function showResetModal() {
  document.getElementById('resetModal').style.display = 'block';
}
)";
```

**HTML Usage:**
```cpp
html += "<link rel='stylesheet' href='/styles.css'>";
html += "<script src='/scripts/main.js'></script>";
```

### Size Comparison

**Before (Inline):**
- Main config page: ~60KB (HTML + CSS + JS concatenated)
- No caching possible
- Memory corruption risk

**After (Separated):**
- Main config page HTML: ~35KB
- CSS (cached): 15.3KB
- JavaScript (cached): varies by page
- Total first load: ~50-55KB
- Total subsequent loads: ~35KB (cached resources)

### Browser Caching

External CSS and JavaScript enable standard HTTP caching:
- Browser caches `/styles.css` after first request
- Browser caches `/scripts/*.js` after first request
- Subsequent page loads only download HTML (~35KB)
- 30-40% bandwidth reduction on navigation

## Alternatives Considered

### 1. SPIFFS File System
Store CSS/JS as files in SPIFFS partition.

**Rejected because:**
- Requires SPIFFS partition (reduces OTA space)
- Filesystem overhead and complexity
- Slower than serving from constants
- More failure modes (file read errors)

### 2. Gzip Compression
Compress CSS/JS before serving.

**Rejected because:**
- ESP32 overhead for decompression
- Memory usage for decompression buffer
- Not needed - separation already solved memory issue
- Browser caching more effective than compression

### 3. Single Combined Resource
Serve all CSS+JS as one `/assets.js` file.

**Rejected because:**
- Different pages need different JavaScript
- Would load unnecessary code on every page
- No selective caching by page type
- Larger initial download

## References

- ESP32 WebServer documentation
- HTTP MIME types specification
- Browser caching best practices
- Issue #94: Per-image stay flag (context for this optimization)

## Migration Notes

**For developers:**
1. Never add `<style>` or `<script>` tags to CSS/JS constants
2. Use external links in HTML: `<link>` for CSS, `<script src>` for JS
3. Test all pages after modifying CSS or JavaScript
4. Verify MIME types are correct in route handlers

**For users:**
- No impact - web portal works identically
- Faster page loads after first visit
- Lower memory usage improves stability

## Future Improvements

**Possible enhancements:**
1. Add HTTP cache headers (max-age) to CSS/JS endpoints
2. Version CSS/JS URLs for cache busting on updates (e.g., `/styles.css?v=1.5.0`)
3. Minify CSS/JavaScript to reduce size further
4. Consider splitting main.js into smaller modules if needed
5. Add ETag support for conditional requests

**Not recommended:**
- SPIFFS storage (reduces OTA space)
- Runtime CSS/JS modification (adds complexity)
- Server-side preprocessing (ESP32 overhead)
