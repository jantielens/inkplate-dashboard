#!/usr/bin/env bash
# Prepare release artifacts and manifests
# Usage: prepare_release.sh <version>
set -euo pipefail

VERSION=${1:-}

if [ -z "$VERSION" ]; then
  echo "Usage: $0 <version>"
  echo "Example: $0 v0.13.0"
  exit 2
fi

# Remove 'v' prefix if present, then add it back for consistency
VERSION="${VERSION#v}"
TAG="v${VERSION}"

echo "=== Preparing Release $TAG ==="
echo ""

# Step 1: Build all boards
echo "📦 Step 1: Building all boards..."
./build.sh all
if [ $? -ne 0 ]; then
  echo "❌ Build failed!"
  exit 1
fi
echo "✅ All boards built successfully"
echo ""

# Step 2: Prepare artifacts directory
echo "📁 Step 2: Preparing artifacts directory..."
ARTIFACTS_DIR="artifacts"
rm -rf "$ARTIFACTS_DIR"
mkdir -p "$ARTIFACTS_DIR"

# Copy all binaries (firmware, bootloader, partitions) to artifacts
for board_dir in build/*/; do
  board=$(basename "$board_dir")
  echo "  Copying artifacts for $board..."
  
  # Copy versioned files
  cp "$board_dir"/${board}-v${VERSION}.bin "$ARTIFACTS_DIR/" 2>/dev/null || true
  cp "$board_dir"/${board}-v${VERSION}.bootloader.bin "$ARTIFACTS_DIR/" 2>/dev/null || true
  cp "$board_dir"/${board}-v${VERSION}.partitions.bin "$ARTIFACTS_DIR/" 2>/dev/null || true
done

# List artifacts
echo ""
echo "Artifacts prepared:"
ls -lh "$ARTIFACTS_DIR"
echo ""

# Step 3: Generate manifests
echo "📝 Step 3: Generating ESP Web Tools manifests..."
./scripts/generate_manifests.sh "$TAG" "$ARTIFACTS_DIR" flasher
echo ""

# Step 4: Generate latest.json
echo "📄 Step 4: Generating latest.json..."
./scripts/generate_latest_json.sh "$TAG" "$ARTIFACTS_DIR" flasher/latest.json
echo ""

# Summary
echo "=========================================="
echo "✅ Release preparation complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "1. Review artifacts in: $ARTIFACTS_DIR/"
echo "2. Test flasher locally:"
echo "   cd flasher && python3 -m http.server 8000"
echo "3. Create GitHub release with tag: $TAG"
echo "4. Upload all files from $ARTIFACTS_DIR/ to the release"
echo "5. Commit updated manifests:"
echo "   git add flasher/"
echo "   git commit -m 'Update flasher for $TAG'"
echo "   git push"
echo ""
echo "Binary sizes:"
for bin in "$ARTIFACTS_DIR"/*-v${VERSION}.bin; do
  [ -f "$bin" ] && du -h "$bin"
done
