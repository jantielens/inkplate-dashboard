#!/bin/bash
# Arduino CLI Build Script for Multiple Inkplate Boards
# This script compiles the Arduino sketch using Arduino CLI

set -e

BOARD="${1:-inkplate5v2}"

# Get absolute path to workspace
WORKSPACE_PATH="$(pwd)"
COMMON_PATH="$WORKSPACE_PATH/common"

# Extract firmware version from version.h
VERSION_FILE="$COMMON_PATH/src/version.h"
if [ -f "$VERSION_FILE" ]; then
    FIRMWARE_VERSION=$(grep '#define FIRMWARE_VERSION "' "$VERSION_FILE" | sed 's/.*"\(.*\)".*/\1/')
    echo "Firmware version: $FIRMWARE_VERSION"
else
    echo "⚠️  Warning: version.h not found, using 'unknown' as version"
    FIRMWARE_VERSION="unknown"
fi

# Board configurations
declare -A BOARDS=(
    [inkplate2]="Inkplate 2|Inkplate_Boards:esp32:Inkplate2|boards/inkplate2"
    [inkplate5v2]="Inkplate 5 V2|Inkplate_Boards:esp32:Inkplate5V2|boards/inkplate5v2"
    [inkplate10]="Inkplate 10|Inkplate_Boards:esp32:Inkplate10|boards/inkplate10"
    [inkplate6flick]="Inkplate 6 Flick|Inkplate_Boards:esp32:Inkplate6Flick|boards/inkplate6flick"
)

# Function to build a board
build_board() {
    local BOARD_KEY="$1"
    
    if [[ ! -v BOARDS[$BOARD_KEY] ]]; then
        echo "❌ Error: Unknown board '$BOARD_KEY'"
        echo "Available boards: ${!BOARDS[@]}"
        exit 1
    fi
    
    IFS='|' read -r BOARD_NAME BOARD_FQBN SKETCH_PATH <<< "${BOARDS[$BOARD_KEY]}"
    BUILD_DIR="build/$BOARD_KEY"
    
    echo ""
    echo "========================================"
    echo "Building: $BOARD_NAME"
    echo "========================================"
    
    # Create build directory if it doesn't exist
    mkdir -p "$BUILD_DIR"
    
    # Copy common source files directly to sketch directory for Arduino to compile (only .cpp files)
    for file in "$COMMON_PATH/src"/*.cpp; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            cp "$file" "$SKETCH_PATH/$filename"
            echo "Copied $filename to sketch directory"
        fi
    done
    
    # Compile the sketch with custom library path and build properties
    echo "Compiling $SKETCH_PATH..."
    echo "Including common libraries from: $COMMON_PATH"
    echo "Using partition scheme: Minimal SPIFFS (with OTA support)"
    
    arduino-cli compile \
        --fqbn "$BOARD_FQBN" \
        --board-options "PartitionScheme=min_spiffs" \
        --build-path "$BUILD_DIR" \
        --library "$COMMON_PATH" \
        --build-property "compiler.cpp.extra_flags=-I$COMMON_PATH -I$COMMON_PATH/src" \
        "$SKETCH_PATH"
    
    # Capture the build result before cleanup
    BUILD_RESULT=$?
    
    # Clean up copied files after build (only .cpp files)
    for file in "$COMMON_PATH/src"/*.cpp; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            rm -f "$SKETCH_PATH/$filename"
        fi
    done
    
    if [ $BUILD_RESULT -eq 0 ]; then
        # Rename binary to include version
        ORIGINAL_BIN="$BUILD_DIR/${BOARD_KEY}.ino.bin"
        VERSIONED_BIN="$BUILD_DIR/${BOARD_KEY}-v${FIRMWARE_VERSION}.bin"
        
        if [ -f "$ORIGINAL_BIN" ]; then
            cp "$ORIGINAL_BIN" "$VERSIONED_BIN"
            echo "✅ Build successful!"
            echo "Build artifacts: $BUILD_DIR"
            echo "Firmware binary: ${BOARD_KEY}-v${FIRMWARE_VERSION}.bin"
        else
            echo "✅ Build successful (binary not found for renaming)!"
            echo "Build artifacts: $BUILD_DIR"
        fi
        return 0
    else
        echo "❌ Build failed!"
        return 1
    fi
}

# Main execution
echo "=== Multi-Board Build System ==="

SUCCESS=true

if [ "$BOARD" = "all" ]; then
    echo "Building all boards..."
    for board_key in "${!BOARDS[@]}"; do
        if ! build_board "$board_key"; then
            SUCCESS=false
        fi
    done
else
    if ! build_board "$BOARD"; then
        SUCCESS=false
    fi
fi

echo ""
echo "========================================"
if [ "$SUCCESS" = true ]; then
    echo "✅ All builds completed successfully!"
else
    echo "❌ Some builds failed!"
    exit 1
fi
echo "========================================"
