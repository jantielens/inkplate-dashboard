#!/bin/bash
# Temporary test wrapper to run build.sh with arduino-cli in PATH

# Add arduino-cli to PATH for Windows
export PATH="$PATH:/mnt/c/Program Files/Arduino CLI"

# Run the build script
./build.sh "$@"
