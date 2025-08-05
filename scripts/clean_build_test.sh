#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_usage() {
    echo "Usage: $0 [Debug|Release]"
    echo "  Debug   - Build with debug symbols and optimizations disabled"
    echo "  Release - Build with optimizations enabled"
    echo "  Default - Debug (if no parameter provided)"
}

# Get the script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Parse build type parameter
BUILD_TYPE="Debug"  # Default

if [ $# -eq 1 ]; then
    case "$1" in
        "Debug"|"debug")
            BUILD_TYPE="Debug"
            ;;
        "Release"|"release")
            BUILD_TYPE="Release"
            ;;
        *)
            print_error "Invalid build type: $1"
            print_usage
            exit 1
            ;;
    esac
elif [ $# -gt 1 ]; then
    print_error "Too many arguments"
    print_usage
    exit 1
fi

print_status "Video Transport Clean Build and Test Runner"
print_status "Script directory: $SCRIPT_DIR"
print_status "Project root: $PROJECT_ROOT"
print_status "Build type: $BUILD_TYPE"
echo

# Change to project root
cd "$PROJECT_ROOT"

BUILD_DIR="build"

# Clean build directory
print_status "Cleaning build directory..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
    print_status "Removed existing build directory"
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

print_status "Configuring project with CMake (Build Type: $BUILD_TYPE)..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

print_status "Building project..."
make -j$(sysctl -n hw.ncpu)

print_success "Build completed successfully!"
echo

print_status "Running unit tests with ctest..."
echo "========================================"

# Run ctest with verbose output
if ctest --verbose; then
    print_success "All tests passed! 🎉"
    exit 0
else
    print_error "Some tests failed"
    exit 1
fi

