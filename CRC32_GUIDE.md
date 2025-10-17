# CRC32 Change Detection Guide

This guide explains how to set up CRC32-based change detection to dramatically extend battery life.

## Overview

CRC32 change detection allows your Inkplate device to skip downloading images when they haven't changed. Instead of downloading the full PNG image (typically 100-500KB), the device downloads only a tiny checksum file (~10 bytes), compares it with the stored value, and only downloads the full image if it has changed.

**Battery Life Impact:** With images changing once per day, battery life increases from ~13 days to ~33 days (2.5× improvement).

## Server Requirements

Your web server must provide a `.crc32` file alongside each PNG image:

```
image.png           # Your PNG image
image.png.crc32     # CRC32 checksum file
```

## CRC32 File Format

The `.crc32` file should contain the CRC32 checksum in hexadecimal format. Both of these formats are supported:

**With 0x prefix:**
```
0x1a2b3c4d
```

**Without prefix:**
```
1a2b3c4d
```

The checksum should be calculated on the PNG image file contents.

## Generating CRC32 Files

### Using Python

```python
import zlib

def calculate_crc32(filename):
    with open(filename, 'rb') as f:
        data = f.read()
        crc32 = zlib.crc32(data) & 0xffffffff
        return f"0x{crc32:08x}"

# Generate CRC32 for an image
image_path = "dashboard.png"
crc32_value = calculate_crc32(image_path)

# Write to .crc32 file
with open(f"{image_path}.crc32", 'w') as f:
    f.write(crc32_value)

print(f"CRC32 for {image_path}: {crc32_value}")
```

### Using Bash/Linux

```bash
#!/bin/bash
# Generate CRC32 file for a PNG image

IMAGE_FILE="dashboard.png"
CRC32_FILE="${IMAGE_FILE}.crc32"

# Calculate CRC32 and save to file
crc32 "$IMAGE_FILE" | awk '{print "0x" $1}' > "$CRC32_FILE"

echo "Generated $CRC32_FILE"
```

### Using Node.js

```javascript
const fs = require('fs');
const { crc32 } = require('crc');

const imagePath = 'dashboard.png';
const data = fs.readFileSync(imagePath);
const checksum = crc32(data).toString(16).padStart(8, '0');

fs.writeFileSync(`${imagePath}.crc32`, `0x${checksum}`);
console.log(`CRC32 for ${imagePath}: 0x${checksum}`);
```

## Automation Examples

### Python Script with Web Server Integration

```python
import os
import zlib
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class ImageWatcher(FileSystemEventHandler):
    def on_modified(self, event):
        if event.src_path.endswith('.png'):
            self.generate_crc32(event.src_path)
    
    def generate_crc32(self, image_path):
        with open(image_path, 'rb') as f:
            crc32 = zlib.crc32(f.read()) & 0xffffffff
            
        crc32_path = f"{image_path}.crc32"
        with open(crc32_path, 'w') as f:
            f.write(f"0x{crc32:08x}")
        
        print(f"Updated CRC32 for {image_path}: 0x{crc32:08x}")

# Watch a directory for PNG changes
if __name__ == "__main__":
    path = "/var/www/html/dashboards"
    event_handler = ImageWatcher()
    observer = Observer()
    observer.schedule(event_handler, path, recursive=False)
    observer.start()
    print(f"Watching {path} for PNG changes...")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
```

### Bash Script for Batch Processing

```bash
#!/bin/bash
# Generate CRC32 files for all PNG images in a directory

DIRECTORY="/var/www/html/dashboards"

for png_file in "$DIRECTORY"/*.png; do
    if [ -f "$png_file" ]; then
        crc32_file="${png_file}.crc32"
        crc32 "$png_file" | awk '{print "0x" $1}' > "$crc32_file"
        echo "Generated CRC32 for $(basename "$png_file")"
    fi
done

echo "Done processing all PNG files"
```

## Home Assistant Integration

If you're using [@jantielens/ha-screenshotter](https://github.com/jantielens/ha-screenshotter), CRC32 files are automatically generated for you. No additional setup is required!

Just enable CRC32 change detection in your Inkplate configuration, and the add-on will handle everything.

## Enabling on Your Inkplate Device

1. **Enter Config Mode**
   - Long press the wake button (hold for 2.5+ seconds)
   - Device will connect to WiFi and show the configuration URL

2. **Access Configuration Page**
   - Open a web browser to the IP address shown on the device
   
3. **Enable CRC32 Check**
   - Scroll to the "Enable CRC32-based change detection" checkbox
   - Check the box
   - Click "Save Configuration"

4. **Verify Operation**
   - Check serial output for "CRC32 check is ENABLED" messages
   - When image is unchanged, you'll see "CRC32 UNCHANGED - Skipping download"
   - When image changes, you'll see "CRC32 CHANGED - Will download image"

## Troubleshooting

### CRC32 File Not Found

**Symptom:** Serial output shows "CRC32 file not found or error"

**Solution:** 
- Ensure the `.crc32` file exists at `{IMAGE_URL}.crc32`
- Check file permissions on your web server
- Test by accessing the URL directly in a browser

**Note:** The device automatically falls back to downloading the full image if the CRC32 file is missing.

### Image Not Updating

**Symptom:** Device skips download but image should have changed

**Solution:**
- Verify the CRC32 file was updated when the image changed
- Check that your automation script is working correctly
- Temporarily disable CRC32 check to force a download

### Battery Life Not Improving

**Symptom:** Expected battery life improvement not seen

**Causes:**
- Image is changing on every wake (CRC32 doesn't help if image always changes)
- CRC32 file server is slow or unreliable
- WiFi signal is poor, causing connection overhead

**Solution:**
- Review how often your image actually changes
- Monitor serial output to see CRC32 check success rate
- Consider optimizing WiFi signal strength

## Technical Details

### How It Works

1. **Wake from Deep Sleep**
   - Device wakes based on refresh interval
   - Connects to WiFi network

2. **CRC32 Check (if enabled)**
   - Downloads `{IMAGE_URL}.crc32` (~10 bytes)
   - Parses hexadecimal checksum value
   - Compares with stored value from last successful download
   
3. **Decision Point**
   - **If unchanged:** Skip download, go back to sleep immediately
   - **If changed:** Download full image and update stored CRC32

4. **Fallback Behavior**
   - If CRC32 file is missing, malformed, or download fails
   - Automatically falls back to downloading full image
   - No error displayed to user

### Performance Metrics

**Without CRC32 (5-minute refresh, image changes daily):**
- Wake duration: ~11 seconds
- Daily wakes: 288
- Daily active time: ~52.8 minutes
- Daily consumption: ~92.6 mAh
- Battery life (1200mAh): ~13 days

**With CRC32 (5-minute refresh, image changes daily):**
- Wake duration (no change): ~4.5 seconds
- Wake duration (with change): ~11 seconds
- Daily wakes: 288 (287 no-change, 1 with change)
- Daily active time: ~21.5 minutes
- Daily consumption: ~36.4 mAh
- Battery life (1200mAh): ~33 days

**Improvement:**
- 60.7% reduction in power consumption
- 2.5× longer battery life
- 59% less active time

## Security Considerations

CRC32 is **not** a cryptographic hash function and should not be used for security purposes:

- ✅ **Good for:** Change detection, data integrity verification
- ❌ **Not for:** Security, authentication, tamper detection

If you need to verify that your images haven't been tampered with, consider using HTTPS and proper server authentication rather than relying on CRC32 checksums.

## Additional Resources

- [Main README](README.md)
- [User Guide](USING.md)
- [@jantielens/ha-screenshotter](https://github.com/jantielens/ha-screenshotter)
- [Issue #56: CRC32 Support](https://github.com/jantielens/ha-screenshotter/issues/56)
