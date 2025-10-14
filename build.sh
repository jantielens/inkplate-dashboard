#!/bin/bash
# Arduino CLI Build Script for Multiple Inkplate Boards
# This script compiles the Arduino sketch using Arduino CLI

set -e

BOARD="${1:-inkplate5v2}"

# Get absolute path to workspace
WORKSPACE_PATH="$(pwd)"
COMMON_PATH="$WORKSPACE_PATH/common"

# Board configurations
declare -A BOARDS=(
    [inkplate5v2]="Inkplate 5 V2|Inkplate_Boards:esp32:Inkplate5V2|boards/inkplate5v2"
    [inkplate10]="Inkplate 10|Inkplate_Boards:esp32:Inkplate10|boards/inkplate10"
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
    
    arduino-cli compile \
        --fqbn "$BOARD_FQBN" \
        --build-path "$BUILD_DIR" \
        --library "$COMMON_PATH" \
        --build-property "compiler.cpp.extra_flags=-I\"$COMMON_PATH\" -I\"$COMMON_PATH/src\" -I\"$WORKSPACE_PATH/$SKETCH_PATH\"" \
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
        echo "✅ Build successful!"
        echo "Build artifacts: $BUILD_DIR"
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
