#!/usr/bin/env bash
# Generate ESP Web Tools manifest JSON files for each board
# Usage: generate_manifests.sh <tag> <artifacts_dir> <output_dir>
set -euo pipefail

TAG=${1:-}
ARTIFACTS_DIR=${2:-artifacts}
OUTPUT_DIR=${3:-flasher}

if [ -z "$TAG" ]; then
  echo "Usage: $0 <tag> [artifacts_dir] [output_dir]"
  exit 2
fi

# Remove 'v' prefix from tag if present
VERSION="${TAG#v}"

# ensure jq exists
if ! command -v jq >/dev/null 2>&1; then
  echo "jq is required but not installed. Please install jq."
  exit 1
fi

# Friendly display names for boards
declare -A NAMES
NAMES[inkplate2]="Inkplate 2"
NAMES[inkplate5v2]="Inkplate 5 V2"
NAMES[inkplate10]="Inkplate 10"
NAMES[inkplate6flick]="Inkplate 6 Flick"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# ESP32 memory offsets (in decimal)
BOOTLOADER_OFFSET=4096      # 0x1000
PARTITIONS_OFFSET=32768     # 0x8000
FIRMWARE_OFFSET=65536       # 0x10000

echo "Generating ESP Web Tools manifests for version ${VERSION}..."

# Find all main firmware binaries and generate manifests
for firmware_bin in "$ARTIFACTS_DIR"/*-v*.bin; do
  [ -f "$firmware_bin" ] || continue
  
  # Skip bootloader and partition files
  if [[ "$firmware_bin" == *".bootloader.bin" ]] || [[ "$firmware_bin" == *".partitions.bin" ]]; then
    continue
  fi
  
  filename=$(basename "$firmware_bin")
  # Extract board name (e.g., "inkplate2" from "inkplate2-v0.13.0.bin")
  board=$(echo "$filename" | sed -E 's/-v.*//')
  
  # Construct filenames for all parts
  bootloader_file="${board}-v${VERSION}.bootloader.bin"
  partitions_file="${board}-v${VERSION}.partitions.bin"
  firmware_file="${board}-v${VERSION}.bin"
  
  # Check if all required files exist
  if [ ! -f "$ARTIFACTS_DIR/$bootloader_file" ]; then
    echo "⚠️  Warning: Bootloader not found for $board: $bootloader_file"
    continue
  fi
  
  if [ ! -f "$ARTIFACTS_DIR/$partitions_file" ]; then
    echo "⚠️  Warning: Partitions file not found for $board: $partitions_file"
    continue
  fi
  
  # Generate URLs
  base_url="https://github.com/jantielens/inkplate-dashboard/releases/download/${TAG}"
  bootloader_url="${base_url}/${bootloader_file}"
  partitions_url="${base_url}/${partitions_file}"
  firmware_url="${base_url}/${firmware_file}"
  
  display_name="${NAMES[$board]:-$board}"
  manifest_file="$OUTPUT_DIR/manifest_${board}.json"
  
  # Create manifest JSON
  jq -n \
    --arg name "Inkplate Dashboard for ${display_name}" \
    --arg version "$VERSION" \
    --arg bootloader_url "$bootloader_url" \
    --argjson bootloader_offset "$BOOTLOADER_OFFSET" \
    --arg partitions_url "$partitions_url" \
    --argjson partitions_offset "$PARTITIONS_OFFSET" \
    --arg firmware_url "$firmware_url" \
    --argjson firmware_offset "$FIRMWARE_OFFSET" \
    '{
      name: $name,
      version: $version,
      home_assistant_domain: "inkplate_dashboard",
      new_install_prompt_erase: true,
      builds: [{
        chipFamily: "ESP32",
        parts: [
          { path: $bootloader_url, offset: $bootloader_offset },
          { path: $partitions_url, offset: $partitions_offset },
          { path: $firmware_url, offset: $firmware_offset }
        ]
      }]
    }' > "$manifest_file"
  
  echo "✅ Generated manifest: $manifest_file"
done

echo ""
echo "Manifest generation complete!"
echo "Manifests saved to: $OUTPUT_DIR/"
