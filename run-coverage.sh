#!/bin/bash
# Code Coverage Runner for Inkplate Dashboard Tests
# Uses gcov + lcov to generate HTML coverage reports

set -e

CLEAN=0
INSTALL=0

# Parse arguments
for arg in "$@"; do
  case $arg in
    --clean)
      CLEAN=1
      shift
      ;;
    --install)
      INSTALL=1
      shift
      ;;
    *)
      ;;
  esac
done

echo "=== Inkplate Dashboard Code Coverage ==="
echo ""

# Check if lcov is installed
if ! command -v lcov &> /dev/null; then
    echo "ERROR: lcov not found"
    echo ""
    echo "To install lcov:"
    echo "  Ubuntu/Debian: sudo apt-get install lcov"
    echo "  macOS:         brew install lcov"
    echo "  Fedora:        sudo dnf install lcov"
    echo ""
    
    if [ $INSTALL -eq 1 ]; then
        echo "Attempting to install lcov..."
        if command -v apt-get &> /dev/null; then
            sudo apt-get update && sudo apt-get install -y lcov
        elif command -v brew &> /dev/null; then
            brew install lcov
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y lcov
        else
            echo "Could not determine package manager. Please install lcov manually."
            exit 1
        fi
    else
        exit 1
    fi
fi

echo "lcov version: $(lcov --version | head -1)"
echo ""

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test"
BUILD_DIR="$TEST_DIR/build"
COVERAGE_DIR="$TEST_DIR/coverage"

if [ $CLEAN -eq 1 ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    if [ -d "$COVERAGE_DIR" ]; then
        echo "Cleaning coverage directory..."
        rm -rf "$COVERAGE_DIR"
    fi
fi

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$COVERAGE_DIR"

cd "$BUILD_DIR"

# Configure with coverage enabled
echo "Configuring CMake with coverage..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

# Build
echo ""
echo "Building tests..."
cmake --build .

# Run tests
echo ""
echo "Running tests..."
ctest --output-on-failure --verbose

# Capture coverage data
echo ""
echo "Capturing coverage data..."

# Initialize coverage
lcov --zerocounters --directory .
lcov --capture --initial --directory . --output-file coverage_base.info

# Run tests again to generate coverage
ctest --output-on-failure

# Capture test coverage
lcov --capture --directory . --output-file coverage_test.info

# Combine baseline and test coverage
lcov --add-tracefile coverage_base.info --add-tracefile coverage_test.info --output-file coverage_total.info

# Filter out system headers and test files
lcov --remove coverage_total.info '/usr/*' '*/test/*' '*/mocks/*' '*/googletest/*' --output-file coverage_filtered.info

# Generate HTML report
echo ""
echo "Generating HTML report..."
genhtml coverage_filtered.info --output-directory "$COVERAGE_DIR"

echo ""
echo "✅ Coverage analysis complete!"
echo ""
echo "Coverage reports:"
echo "  HTML: $COVERAGE_DIR/index.html"
echo "  LCOV: $BUILD_DIR/coverage_filtered.info"
echo ""

# Show coverage summary
echo "Coverage summary:"
lcov --summary coverage_filtered.info

# Open HTML report (macOS)
if command -v open &> /dev/null; then
    echo ""
    echo "Opening coverage report in browser..."
    open "$COVERAGE_DIR/index.html"
# Open HTML report (Linux with xdg-open)
elif command -v xdg-open &> /dev/null; then
    echo ""
    echo "Opening coverage report in browser..."
    xdg-open "$COVERAGE_DIR/index.html"
fi

echo ""
echo "Coverage run complete"
