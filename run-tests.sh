#!/bin/bash
# Test Runner Script for Inkplate Dashboard Unit Tests
# Builds and runs unit tests using CMake + Google Test

set -e

CLEAN=0
VERBOSE=0

# Parse arguments
for arg in "$@"; do
  case $arg in
    --clean)
      CLEAN=1
      shift
      ;;
    --verbose)
      VERBOSE=1
      shift
      ;;
    *)
      ;;
  esac
done

echo "=== Inkplate Dashboard Unit Test Runner ==="
echo ""

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install CMake:"
    echo "  Ubuntu/Debian: sudo apt-get install cmake"
    echo "  macOS:         brew install cmake"
    echo "  Fedora:        sudo dnf install cmake"
    exit 1
fi

echo "CMake version:"
cmake --version | head -1
echo ""

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test"
BUILD_DIR="$TEST_DIR/build"

if [ $CLEAN -eq 1 ] && [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# Configure
echo "Configuring CMake..."
if [ $VERBOSE -eq 1 ]; then
    cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
else
    cmake ..
fi
echo ""

# Build
echo "Building tests..."
cmake --build . --config Release
echo ""

# Run tests
echo "Running tests..."
echo ""
ctest -C Release --output-on-failure --verbose

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ All tests passed!"
else
    echo ""
    echo "❌ Some tests failed"
    exit 1
fi

echo ""
echo "Test run complete"
