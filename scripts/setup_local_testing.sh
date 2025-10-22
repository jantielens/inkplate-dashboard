#!/usr/bin/env bash
# Setup local testing environment for web flasher
# This copies build artifacts to flasher/builds/ and creates local manifests

set -euo pipefail

echo "🧪 Setting up local testing environment..."
echo ""

FLASHER_DIR="flasher"
BUILD_DIR="build"

# Create builds directory structure
mkdir -p "$FLASHER_DIR/builds"

# Copy binaries for each board
for board in inkplate2 inkplate5v2 inkplate6flick inkplate10; do
  echo "📦 Setting up $board..."
  
  # Create board directory
  mkdir -p "$FLASHER_DIR/builds/$board"
  
  # Copy binaries if they exist
  if [ -d "$BUILD_DIR/$board" ]; then
    cp "$BUILD_DIR/$board/${board}-v"*.bin "$FLASHER_DIR/builds/$board/" 2>/dev/null || echo "  ⚠️  No binaries found for $board (run ./build.sh $board first)"
    
    # Create local manifest
    if [ -f "$FLASHER_DIR/builds/$board/${board}-v0.13.0.bin" ]; then
      cat > "$FLASHER_DIR/manifest_${board}_local.json" << EOF
{
  "name": "Inkplate Dashboard for ${board} (Local Test)",
  "version": "0.13.0",
  "home_assistant_domain": "inkplate_dashboard",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        {
          "path": "./builds/${board}/${board}-v0.13.0.bootloader.bin",
          "offset": 4096
        },
        {
          "path": "./builds/${board}/${board}-v0.13.0.partitions.bin",
          "offset": 32768
        },
        {
          "path": "./builds/${board}/${board}-v0.13.0.bin",
          "offset": 65536
        }
      ]
    }
  ]
}
EOF
      echo "  ✅ Created local manifest and copied binaries"
      ls -lh "$FLASHER_DIR/builds/$board/" | grep -E "\.bin$" | awk '{print "     - " $9 " (" $5 ")"}'
    fi
  else
    echo "  ⚠️  Build directory not found: $BUILD_DIR/$board"
  fi
  echo ""
done

echo "=========================================="
echo "✅ Local testing environment ready!"
echo "=========================================="
echo ""
echo "To test the flasher:"
echo "  cd flasher && python3 -m http.server 8080"
echo ""
echo "Then open: http://localhost:8080"
echo ""
echo "Note: Make sure to use a Chromium-based browser"
echo "      (Chrome, Edge, or Opera) for Web Serial API support"
