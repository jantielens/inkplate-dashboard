#!/usr/bin/env python3
"""
Simple script to generate CRC32 checksum file for PNG images.

This script calculates the CRC32 checksum of a PNG file and creates
a companion .crc32 file that the Inkplate Dashboard can use for
change detection.

Usage:
    python3 generate_crc32.py image.png

This will create image.png.crc32 with the checksum in hexadecimal format.

For more information, see CRC32_GUIDE.md in the project root.
"""

import sys
import zlib
import os

def calculate_crc32(filename):
    """Calculate CRC32 checksum for a file."""
    with open(filename, 'rb') as f:
        data = f.read()
        crc32 = zlib.crc32(data) & 0xffffffff
        return f"0x{crc32:08x}"

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 generate_crc32.py <image.png>")
        print("\nGenerates a .crc32 checksum file for use with Inkplate Dashboard")
        print("change detection feature.")
        sys.exit(1)
    
    image_path = sys.argv[1]
    
    if not os.path.exists(image_path):
        print(f"Error: File '{image_path}' not found")
        sys.exit(1)
    
    if not image_path.lower().endswith('.png'):
        print(f"Warning: File '{image_path}' doesn't have .png extension")
        response = input("Continue anyway? (y/n): ")
        if response.lower() != 'y':
            sys.exit(0)
    
    try:
        print(f"Calculating CRC32 for {image_path}...")
        crc32_value = calculate_crc32(image_path)
        crc32_file = f"{image_path}.crc32"
        
        with open(crc32_file, 'w') as f:
            f.write(crc32_value)
        
        print(f"✓ Generated {crc32_file}")
        print(f"  CRC32: {crc32_value}")
        print(f"\nUpload both files to your web server:")
        print(f"  - {image_path}")
        print(f"  - {crc32_file}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
