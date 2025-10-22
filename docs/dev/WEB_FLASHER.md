# Web Flasher Implementation

## Overview

The Inkplate Dashboard includes a web-based firmware flasher that allows users to flash their devices directly from a browser without installing any software. It uses [ESP Web Tools](https://esphome.github.io/esp-web-tools/) and the Web Serial API.

## Quick Links

- **Live Flasher:** https://jantielens.github.io/inkplate-dashboard/flasher/
- **Local Testing:** `./scripts/setup_local_testing.sh` then `cd flasher && python3 -m http.server 8080`
- **ESP Web Tools Docs:** https://esphome.github.io/esp-web-tools/

## Architecture

### High-Level Flow

```
┌─────────────┐
│   Browser   │ (Chrome/Edge - Web Serial API required)
└──────┬──────┘
       │ 1. User selects board
       ▼
┌─────────────────┐
│  index.html     │ Loads manifest_inkplate*.json
│  ESP Web Tools  │
└────────┬────────┘
         │ 2. Fetches manifest
         ▼
┌──────────────────────┐
│  manifest_*.json     │ Points to GitHub Pages URLs
└──────────┬───────────┘
           │ 3. Downloads binaries (same origin - no CORS)
           ▼
┌─────────────────────────────────────┐
│  GitHub Pages (main-flasher)         │
│  flasher/firmware/v0.14.0/           │
│  - bootloader.bin (offset 0x1000)    │
│  - partitions.bin (offset 0x8000)    │
│  - firmware.bin   (offset 0x10000)   │
└─────────────────┬───────────────────┘
                  │ 4. Flash via Web Serial
                  ▼
           ┌──────────────┐
           │  ESP32       │
           │  Inkplate    │
           └──────────────┘
```

## Project Structure

```
flasher/                          # On main-flasher branch
├── index.html                    # Web flasher UI
├── manifest_inkplate2.json       # Production manifest (auto-generated)
├── manifest_inkplate5v2.json     # Production manifest (auto-generated)
├── manifest_inkplate6flick.json  # Production manifest (auto-generated)
├── manifest_inkplate10.json      # Production manifest (auto-generated)
├── latest.json                   # Metadata (auto-generated)
├── firmware/                     # Binary storage (main-flasher only)
│   └── v0.14.0/                  # Versioned directory
│       ├── inkplate2-v0.14.0.bin
│       ├── inkplate2-v0.14.0.bootloader.bin
│       ├── inkplate2-v0.14.0.partitions.bin
│       └── ... (all 4 boards × 3 files)
├── manifest_*_local.json         # Local testing manifests (gitignored)
├── builds/                       # Local binary copies (gitignored)
└── README.md                     # User documentation

scripts/                          # On main branch
├── generate_manifests.sh         # Generates ESP Web Tools manifests
├── generate_latest_json.sh       # Generates metadata
├── prepare_release.sh            # Manual release preparation
└── setup_local_testing.sh        # Sets up local testing environment
```

## How It Works

### 1. Binary Files Required

For each board, three binary files are needed for complete flashing:

| File | Purpose | Memory Offset | Size |
|------|---------|---------------|------|
| `bootloader.bin` | ESP32 bootloader | 0x1000 (4096) | ~19KB |
| `partitions.bin` | Partition table | 0x8000 (32768) | ~3KB |
| `firmware.bin` | Main application | 0x10000 (65536) | ~1.1MB |

**Why three files?**
- This matches the standard ESP32 flashing process
- Same approach used by `arduino-cli upload` and `esptool.py`
- Allows OTA updates while preserving bootloader

### 2. Build Process

The build system (`build.sh`) automatically generates all three binaries:

```bash
./build.sh inkplate6flick

# Generates in build/inkplate6flick/:
# - inkplate6flick-v0.13.0.bin            (firmware)
# - inkplate6flick-v0.13.0.bootloader.bin
# - inkplate6flick-v0.13.0.partitions.bin
```

**Modified in `build.sh`:**
```bash
# Copy and rename all binaries to include version
VERSIONED_BIN="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.bin"
VERSIONED_BOOTLOADER="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.bootloader.bin"
VERSIONED_PARTITIONS="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.partitions.bin"

cp "$ORIGINAL_BIN" "$VERSIONED_BIN"
cp "$ORIGINAL_BOOTLOADER" "$VERSIONED_BOOTLOADER"
cp "$ORIGINAL_PARTITIONS" "$VERSIONED_PARTITIONS"
```

### 3. Manifest Files

ESP Web Tools uses JSON manifest files to describe what to flash and where.

**Example: `manifest_inkplate6flick.json`**
```json
{
  "name": "Inkplate Dashboard for Inkplate 6 Flick",
  "version": "0.14.0",
  "home_assistant_domain": "inkplate_dashboard",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        {
          "path": "https://jantielens.github.io/inkplate-dashboard/firmware/v0.14.0/inkplate6flick-v0.14.0.bootloader.bin",
          "offset": 4096
        },
        {
          "path": "https://jantielens.github.io/inkplate-dashboard/firmware/v0.14.0/inkplate6flick-v0.14.0.partitions.bin",
          "offset": 32768
        },
        {
          "path": "https://jantielens.github.io/inkplate-dashboard/firmware/v0.14.0/inkplate6flick-v0.14.0.bin",
          "offset": 65536
        }
      ]
    }
  ]
}
```

**Note:** URLs point to GitHub Pages (same origin as flasher) to avoid CORS restrictions.

**Manifest Fields:**
- `name` - Display name shown to users
- `version` - Firmware version
- `home_assistant_domain` - Optional HA integration
- `new_install_prompt_erase` - Ask user to erase flash
- `builds[].chipFamily` - Target chip (ESP32)
- `builds[].parts[]` - Array of binaries to flash
  - `path` - URL to binary file
  - `offset` - Memory address to flash to (decimal)

### 4. Web Interface

**`flasher/index.html`** provides:
- Board selection UI (4 boards with visual cards)
- ESP Web Tools integration
- Automatic local/production mode detection
- Comprehensive instructions and troubleshooting
- Dark/light mode support
- Mobile-responsive design

**Key JavaScript:**
```javascript
// Detect environment
const isLocal = window.location.hostname === 'localhost' || 
               window.location.hostname === '127.0.0.1' ||
               window.location.hostname.includes('github.dev');

// Load appropriate manifest
boardRadios.forEach(radio => {
  radio.addEventListener('change', (e) => {
    const boardId = e.target.value;
    const manifestSuffix = isLocal ? '_local' : '';
    installButton.manifest = `./manifest_${boardId}${manifestSuffix}.json`;
    installButton.classList.remove('hidden');
  });
});
```

## Release Workflow

### Automated (GitHub Actions)

When you push a tag (e.g., `v0.14.0`), the workflow automatically:

**`.github/workflows/release.yml`** does:

1. **Build all boards** (matrix strategy):
   ```yaml
   strategy:
     matrix:
       board: [inkplate2, inkplate5v2, inkplate10, inkplate6flick]
   ```

2. **Upload all binaries as artifacts**:
   ```yaml
   - name: Upload firmware artifacts (all binaries)
     uses: actions/upload-artifact@v4
     with:
       path: |
         build/${{ matrix.board }}/*-v*.bin
         build/${{ matrix.board }}/*-v*.bootloader.bin
         build/${{ matrix.board }}/*-v*.partitions.bin
   ```

3. **Create GitHub Release** with all 12 binaries (4 boards × 3 files)

4. **Generate manifests**:
   ```yaml
   - name: Generate web flasher manifests
     run: |
       chmod +x scripts/generate_manifests.sh
       scripts/generate_manifests.sh "${TAG}" artifacts flasher
   ```

5. **Commit manifests** to the flasher repo/directory:
   ```yaml
   - name: Build and publish latest.json and manifests
     run: |
       cd flasher
       git add latest.json manifest_*.json
       git commit -m "Update flasher for ${TAG} [skip ci]"
       git push
   ```

**Result:** Flasher is automatically updated with new firmware!

### Manual (For Testing)

```bash
# 1. Prepare everything locally
./scripts/prepare_release.sh v0.14.0

# This generates:
# - artifacts/*.bin (all binaries)
# - flasher/manifest_*.json (production manifests)
# - flasher/latest.json (metadata)

# 2. Manually create GitHub release and upload artifacts/

# 3. Commit manifests
git add flasher/manifest_*.json
git commit -m "Update flasher for v0.14.0"
git push
```

## Local Testing

### Setup

```bash
# 1. Build a board
./build.sh inkplate6flick

# 2. Setup local testing environment
./scripts/setup_local_testing.sh

# This creates:
# - flasher/manifest_*_local.json (points to local files)
# - flasher/builds/ (copies of binaries)

# 3. Start local server
cd flasher
python3 -m http.server 8080

# 4. Open http://localhost:8080 in Chrome/Edge
```

### How Local Testing Works

**Local manifests** point to relative paths:
```json
{
  "parts": [
    { "path": "builds/inkplate6flick/inkplate6flick-v0.13.0.bootloader.bin", ... },
    { "path": "builds/inkplate6flick/inkplate6flick-v0.13.0.partitions.bin", ... },
    { "path": "builds/inkplate6flick/inkplate6flick-v0.13.0.bin", ... }
  ]
}
```

**Automatic detection in `index.html`:**
```javascript
// Detects localhost or github.dev codespaces
const isLocal = window.location.hostname === 'localhost' || 
               window.location.hostname === '127.0.0.1' ||
               window.location.hostname.includes('github.dev');

// Shows warning banner
if (isLocal) {
  document.getElementById('local-test-warning').style.display = 'block';
}
```

**Files ignored by git** (`.gitignore`):
```gitignore
# Flasher - local testing files (keep production manifests)
flasher/*_local.json
flasher/builds/
flasher/latest.json
```

## Scripts Reference

### `generate_manifests.sh`

**Purpose:** Generate ESP Web Tools manifest files

**Usage:**
```bash
./scripts/generate_manifests.sh <tag> <artifacts_dir> <output_dir>
./scripts/generate_manifests.sh v0.14.0 artifacts flasher
```

**What it does:**
1. Scans `artifacts_dir` for versioned binaries
2. For each board, creates a manifest JSON
3. URLs point to GitHub Release downloads
4. Outputs to `output_dir/manifest_<board>.json`

**Manifest generation logic:**
```bash
bootloader_url="${base_url}/${board}-v${VERSION}.bootloader.bin"
partitions_url="${base_url}/${board}-v${VERSION}.partitions.bin"
firmware_url="${base_url}/${board}-v${VERSION}.bin"

jq -n \
  --arg name "Inkplate Dashboard for ${display_name}" \
  --arg version "$VERSION" \
  --arg bootloader_url "$bootloader_url" \
  --argjson bootloader_offset "4096" \
  --arg partitions_url "$partitions_url" \
  --argjson partitions_offset "32768" \
  --arg firmware_url "$firmware_url" \
  --argjson firmware_offset "65536" \
  '{...manifest structure...}'
```

### `generate_latest_json.sh`

**Purpose:** Generate metadata for latest release

**Usage:**
```bash
./scripts/generate_latest_json.sh <tag> <artifacts_dir> <output_file>
./scripts/generate_latest_json.sh v0.14.0 artifacts flasher/latest.json
```

**Output format:**
```json
{
  "tag_name": "v0.14.0",
  "published_at": "2025-10-22T12:00:00Z",
  "assets": [
    {
      "board": "inkplate2",
      "filename": "inkplate2-v0.14.0.bin",
      "url": "https://github.com/.../inkplate2-v0.14.0.bin",
      "bootloader_url": "https://github.com/.../inkplate2-v0.14.0.bootloader.bin",
      "partitions_url": "https://github.com/.../inkplate2-v0.14.0.partitions.bin",
      "display_name": "Inkplate 2"
    }
    // ... more boards
  ]
}
```

### `prepare_release.sh`

**Purpose:** Manual release preparation

**Usage:**
```bash
./scripts/prepare_release.sh v0.14.0
```

**Steps:**
1. Builds all boards: `./build.sh all`
2. Creates `artifacts/` directory
3. Copies all binaries to `artifacts/`
4. Generates manifests: `generate_manifests.sh`
5. Generates metadata: `generate_latest_json.sh`
6. Displays summary and next steps

**Use cases:**
- Testing before creating actual release
- Manual release process
- Debugging build/manifest generation

### `setup_local_testing.sh`

**Purpose:** Setup local testing environment

**Usage:**
```bash
./scripts/setup_local_testing.sh
```

**Steps:**
1. Creates `flasher/builds/` directory structure
2. Copies binaries from `build/*/` to `flasher/builds/*/`
3. Generates local manifests pointing to `builds/` directory
4. Shows instructions for starting local server

**Generated structure:**
```
flasher/
├── builds/
│   ├── inkplate2/
│   │   ├── inkplate2-v0.13.0.bin
│   │   ├── inkplate2-v0.13.0.bootloader.bin
│   │   └── inkplate2-v0.13.0.partitions.bin
│   └── ... (other boards)
├── manifest_inkplate2_local.json
└── ... (other local manifests)
```

## Browser Compatibility

### Supported Browsers

| Browser | Version | Status |
|---------|---------|--------|
| Chrome | 89+ | ✅ Fully Supported |
| Edge | 89+ | ✅ Fully Supported |
| Opera | 75+ | ✅ Fully Supported |
| Firefox | Any | ❌ Not Supported (no Web Serial) |
| Safari | Any | ❌ Not Supported (no Web Serial) |

**Why Chrome/Edge only?**
- Requires [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
- Firefox and Safari have not implemented it
- Security concerns prevent it from working on insecure (HTTP) connections

### Requirements

1. **HTTPS or localhost** - Web Serial API requires secure context
2. **USB drivers** - CP210x drivers for ESP32 serial communication
3. **Permissions** - User must grant serial port access

## Security & CORS

### How Binary Downloads Work

**Production flow:**
1. Browser loads `index.html` from GitHub Pages (HTTPS)
2. Manifest loaded from same origin (GitHub Pages)
3. Binaries fetched from GitHub Pages `/firmware/<version>/` (same origin)
4. ESP Web Tools downloads and flashes

**Why GitHub Pages instead of GitHub Releases:**

GitHub Releases don't set proper CORS headers for cross-origin requests. When the flasher
(hosted at `jantielens.github.io`) tries to fetch binaries from `github.com/releases/download/`,
the browser blocks the request:

```
Access to fetch at 'https://github.com/jantielens/inkplate-dashboard/releases/download/...'
from origin 'https://jantielens.github.io' has been blocked by CORS policy
```

**Solution:** Host binaries on GitHub Pages in the `main-flasher` branch:
- Same origin as flasher (`jantielens.github.io`)
- No CORS restrictions
- Reliable, fast delivery via GitHub's CDN
- Binaries stored in `flasher/firmware/<version>/`

**Local testing CORS:**
- All files served from same Python HTTP server
- No CORS restrictions (same origin)
- Works seamlessly for development

## Troubleshooting

### "No 'Access-Control-Allow-Origin' header"

**Cause:** Trying to flash from production site with local binaries

**Solution:** Use local manifests when testing locally

### "404 Not Found" on binary files

**Cause:** Release doesn't have all required binaries

**Solution:** 
- Ensure release includes bootloader and partition files
- Check manifest URLs are correct
- Verify tag matches release version

### Serial port not showing

**Causes:**
1. USB cable is charge-only (no data)
2. Missing CP210x drivers
3. Port in use by another program
4. Browser doesn't support Web Serial

**Solutions:**
1. Try different USB cable
2. Install [CP210x drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
3. Close Arduino IDE, PlatformIO, serial monitors
4. Use Chrome or Edge browser

### Flash fails partway through

**Causes:**
1. Device not in bootloader mode
2. Interrupted connection
3. Corrupt binary file

**Solutions:**
1. Press reset button before flashing
2. Use shorter USB cable
3. Rebuild firmware

## Best Practices

### Adding New Boards

1. **Update build system:**
   ```bash
   # Add to boards/ directory
   # Update build.sh BOARDS array
   ```

2. **Update manifest generator:**
   ```bash
   # scripts/generate_manifests.sh automatically handles new boards
   # Just ensure board friendly name in NAMES array
   ```

3. **Update web interface:**
   ```html
   <!-- Add board option in index.html -->
   <div class="board-option">
     <input type="radio" name="board" id="newboard" value="newboard" />
     <label for="newboard">
       <span class="board-icon">📱</span>
       New Board
     </label>
   </div>
   ```

### Version Management

- Version is defined in `common/src/version.h`
- Must match Git tag (enforced by CI)
- Embedded in firmware and manifest files
- Displayed in web interface

### Release Checklist

- [ ] Update version in `common/src/version.h`
- [ ] Update `CHANGELOG.md`
- [ ] Test builds locally: `./build.sh all`
- [ ] Commit and push changes
- [ ] Create and push tag: `git tag v0.x.x && git push origin v0.x.x`
- [ ] GitHub Actions will handle the rest
- [ ] Verify release on GitHub
- [ ] Test flasher with new version

## Future Enhancements

### Potential Improvements

1. **Version Selection**
   - Allow users to choose from multiple versions
   - Fetch version list from GitHub API
   - Add release notes in UI

2. **OTA Update Mode**
   - Flash only firmware (skip bootloader/partitions)
   - Faster updates for devices already running dashboard
   - Preserve configuration

3. **WiFi Pre-Configuration**
   - Allow users to set WiFi credentials before flashing
   - Modify firmware image before upload
   - Skip initial setup portal

4. **Advanced Options**
   - Erase NVS option
   - Custom partition tables
   - Debug builds

5. **Better Analytics**
   - Track flash success/failure rates
   - Board usage statistics
   - Error reporting

## References

- [ESP Web Tools Documentation](https://esphome.github.io/esp-web-tools/)
- [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
- [ESP32 Flash Layout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)
- [esptool.py](https://github.com/espressif/esptool)
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32)

## Summary

The web flasher provides a user-friendly way to install Inkplate Dashboard firmware without requiring technical knowledge or software installation. It uses industry-standard tools (ESP Web Tools, Web Serial API) and follows ESP32 best practices for flashing.

**Key Benefits:**
- ✅ No software installation required
- ✅ Works directly in browser
- ✅ Automated through GitHub Actions
- ✅ Easy local testing for development
- ✅ Matches arduino-cli upload behavior
- ✅ Professional, polished UI
