# Inkplate Dashboard Web Flasher

This directory contains the web-based firmware flasher for Inkplate Dashboard, built using [ESP Web Tools](https://esphome.github.io/esp-web-tools/).

## 🌐 Live Flasher

Visit the web flasher at: **[https://jantielens.github.io/inkplate-dashboard/flasher/](https://jantielens.github.io/inkplate-dashboard/flasher/)**

## 📋 How It Works

The flasher uses the Web Serial API to flash ESP32 devices directly from your browser. No installation required!

### Files

- `index.html` - Main flasher interface with board selection
- `manifest_*.json` - ESP Web Tools manifest files for each board (generated during release)

## 🔧 For Developers

### Generating Manifests

Manifests are generated automatically during the release process. To manually generate manifests:

```bash
# Generate manifests from build artifacts
./scripts/generate_manifests.sh v0.13.0 build/inkplate2 flasher

# Or for all boards from artifacts directory
./scripts/generate_manifests.sh v0.13.0 artifacts flasher
```

### Testing Locally

1. Build the firmware for a board:
   ```bash
   ./build.sh inkplate2
   ```

2. Generate the manifest:
   ```bash
   ./scripts/generate_manifests.sh v0.13.0 build/inkplate2 flasher
   ```

3. Start a local web server:
   ```bash
   cd flasher
   python3 -m http.server 8000
   ```

4. Open `http://localhost:8000` in Chrome/Edge

5. For local testing, temporarily edit the manifest to point to local files:
   ```json
   {
     "parts": [
       { "path": "/build/inkplate2/inkplate2-v0.13.0.bootloader.bin", "offset": 4096 },
       ...
     ]
   }
   ```

### Manifest Format

Each manifest file follows the ESP Web Tools specification:

```json
{
  "name": "Inkplate Dashboard for Inkplate 2",
  "version": "0.13.0",
  "home_assistant_domain": "inkplate_dashboard",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        {
          "path": "https://github.com/.../bootloader.bin",
          "offset": 4096
        },
        {
          "path": "https://github.com/.../partitions.bin",
          "offset": 32768
        },
        {
          "path": "https://github.com/.../firmware.bin",
          "offset": 65536
        }
      ]
    }
  ]
}
```

## 🚀 Release Process

When creating a new release:

1. **Build all boards:**
   ```bash
   ./build.sh all
   ```

2. **Copy artifacts to the artifacts directory:**
   ```bash
   mkdir -p artifacts
   cp build/inkplate2/inkplate2-v*.bin artifacts/
   cp build/inkplate5v2/inkplate5v2-v*.bin artifacts/
   cp build/inkplate6flick/inkplate6flick-v*.bin artifacts/
   cp build/inkplate10/inkplate10-v*.bin artifacts/
   ```

3. **Generate manifests:**
   ```bash
   ./scripts/generate_manifests.sh v0.13.0 artifacts flasher
   ```

4. **Create GitHub release and upload binaries:**
   - Upload all `*.bin`, `*.bootloader.bin`, and `*.partitions.bin` files
   - The manifests reference these files via GitHub release URLs

5. **Commit the manifests:**
   ```bash
   git add flasher/manifest_*.json
   git commit -m "Update flasher manifests for v0.13.0"
   git push
   ```

## 🔐 CORS and Security

The flasher fetches binaries directly from GitHub Releases. GitHub serves files with appropriate CORS headers, allowing the Web Serial API to download and flash them.

## 📱 Browser Support

The Web Serial API is supported in:
- ✅ Chrome 89+
- ✅ Edge 89+
- ✅ Opera 75+
- ❌ Firefox (not supported)
- ❌ Safari (not supported)

## 🐛 Troubleshooting

### Manifest not loading
- Check that the manifest file exists in the flasher directory
- Verify the GitHub release URLs are correct
- Check browser console for CORS errors

### Flashing fails
- Ensure device is in bootloader mode (usually automatic)
- Try pressing reset button before flashing
- Check that USB cable supports data transfer
- Verify serial port drivers are installed

### Testing with local binaries
Due to browser security restrictions, you cannot directly flash from `file://` URLs. Use a local web server and ensure files are served with correct MIME types.

## 📚 References

- [ESP Web Tools Documentation](https://esphome.github.io/esp-web-tools/)
- [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
- [ESP32 Flash Layout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)
