#!/usr/bin/env bash
# Generate latest.json manifest for flasher repo
# Usage: generate_latest_json.sh <tag> <artifacts_dir> <output_path>
set -euo pipefail

TAG=${1:-}
ARTIFACTS_DIR=${2:-artifacts}
OUT_FILE=${3:-latest.json}

if [ -z "$TAG" ]; then
  echo "Usage: $0 <tag> [artifacts_dir] [output_path]"
  exit 2
fi

PUBLISHED_AT=$(date -u +%Y-%m-%dT%H:%M:%SZ)

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

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

jq -n --arg tag "$TAG" --arg published_at "$PUBLISHED_AT" '{tag_name: $tag, published_at: $published_at, assets: []}' > "$TMP"

for f in "$ARTIFACTS_DIR"/*-v*.bin; do
  [ -f "$f" ] || continue
  filename=$(basename "$f")
  
  # Skip bootloader and partition files for the main assets list
  if [[ "$filename" == *".bootloader.bin" ]] || [[ "$filename" == *".partitions.bin" ]]; then
    continue
  fi
  
  # sanitize inputs (remove CR and LF)
  filename=$(printf "%s" "$filename" | tr -d '\r\n')
  TAG=$(printf "%s" "$TAG" | tr -d '\r\n')
  board=$(echo "$filename" | sed -E 's/-v.*//')
  
  # Construct URLs for all files (use GitHub Pages to avoid CORS issues)
  base_url="https://jantielens.github.io/inkplate-dashboard/firmware/${TAG}"
  firmware_url="${base_url}/${filename}"
  bootloader_file="${board}-v${TAG#v}.bootloader.bin"
  bootloader_url="${base_url}/${bootloader_file}"
  partitions_file="${board}-v${TAG#v}.partitions.bin"
  partitions_url="${base_url}/${partitions_file}"
  
  # remove any stray CR/LF that may exist in variables
  firmware_url=$(printf "%s" "$firmware_url" | tr -d '\r\n')
  bootloader_url=$(printf "%s" "$bootloader_url" | tr -d '\r\n')
  partitions_url=$(printf "%s" "$partitions_url" | tr -d '\r\n')
  
  display_name="${NAMES[$board]:-$board}"
  
  # append asset with all binary URLs
  jq --arg board "$board" \
     --arg filename "$filename" \
     --arg url "$firmware_url" \
     --arg bootloader_url "$bootloader_url" \
     --arg partitions_url "$partitions_url" \
     --arg display_name "$display_name" \
     '.assets += [{
       board: $board,
       filename: $filename,
       url: $url,
       bootloader_url: $bootloader_url,
       partitions_url: $partitions_url,
       display_name: $display_name
     }]' "$TMP" > "$TMP.tmp" && mv "$TMP.tmp" "$TMP"
done

# sort assets by board for determinism
jq '.assets |= sort_by(.board)' "$TMP" > "$TMP.tmp" && mv "$TMP.tmp" "$TMP"

mv "$TMP" "$OUT_FILE"

echo "Generated $OUT_FILE"
