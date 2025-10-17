#!/bin/bash
# Generate CRC32 checksum file for PNG image
#
# This script calculates the CRC32 checksum of a PNG file and creates
# a companion .crc32 file that the Inkplate Dashboard can use for
# change detection.
#
# Usage:
#   ./generate_crc32.sh image.png
#
# This will create image.png.crc32 with the checksum in hexadecimal format.
#
# Requirements:
#   - crc32 command (install with: apt-get install libarchive-zip-perl)
#
# For more information, see CRC32_GUIDE.md in the project root.

set -e

# Check if image file is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <image.png>"
    echo ""
    echo "Generates a .crc32 checksum file for use with Inkplate Dashboard"
    echo "change detection feature."
    exit 1
fi

IMAGE_FILE="$1"

# Check if file exists
if [ ! -f "$IMAGE_FILE" ]; then
    echo "Error: File '$IMAGE_FILE' not found"
    exit 1
fi

# Check if crc32 command is available
if ! command -v crc32 &> /dev/null; then
    echo "Error: 'crc32' command not found"
    echo ""
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install libarchive-zip-perl"
    echo "  macOS: brew install crc32"
    exit 1
fi

CRC32_FILE="${IMAGE_FILE}.crc32"

# Calculate CRC32 and save to file
echo "Calculating CRC32 for $IMAGE_FILE..."
crc32 "$IMAGE_FILE" | awk '{print "0x" $1}' > "$CRC32_FILE"

# Read the value for display
CRC32_VALUE=$(cat "$CRC32_FILE")

echo "✓ Generated $CRC32_FILE"
echo "  CRC32: $CRC32_VALUE"
echo ""
echo "Upload both files to your web server:"
echo "  - $IMAGE_FILE"
echo "  - $CRC32_FILE"
