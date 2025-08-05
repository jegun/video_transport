#!/bin/bash

# Video Transport - Full Test Workflow
# This script runs the complete test workflow:
# 1. Build and run unit tests
# 2. Start receiver
# 3. Start sender
# 4. Check data integrity
# 5. Analyze timestamp differences

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# Get the script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Configuration
BUILD_TYPE=${1:-Debug}  # Default to Debug if not specified
INPUT_FILE=${2:-"resources/front_0.bin"}  # Default to front_0.bin if not specified
RECEIVER_PORT=8080
OUTPUT_FILE="output/received_data.bin"
RECEIVER_LOG="output/receiver.log"
SENDER_LOG="output/sender.log"
RECEIVER_PID=""
SENDER_PID=""

# Print usage if help is requested
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [BUILD_TYPE] [INPUT_FILE]"
    echo ""
    echo "Parameters:"
    echo "  BUILD_TYPE: Debug or Release (default: Debug)"
    echo "  INPUT_FILE: Path to binary file to send (default: resources/front_0.bin)"
    echo ""
    echo "Examples:"
    echo "  $0 Release resources/front_0.bin"
    echo "  $0 Debug resources/front_0.bin"
    echo ""
    echo "Available test files:"
    echo "  resources/test.bin      - Normal test file with edge cases"
    echo "  resources/bad_test.bin  - File with data larger than MaxPacketSize - HeaderSizeBytes"
    echo "  resources/front_0.bin   - Original video data"
    exit 0
fi

# Validate input file exists
if [ ! -f "$INPUT_FILE" ]; then
    print_error "Input file not found: $INPUT_FILE"
    echo ""
    echo "Available files in resources/:"
    ls -la resources/*.bin 2>/dev/null || echo "No .bin files found in resources/"
    exit 1
fi

print_status "Video Transport Full Test Workflow"
print_status "Script directory: $SCRIPT_DIR"
print_status "Project root: $PROJECT_ROOT"
print_status "Build type: $BUILD_TYPE"
print_status "Input file: $INPUT_FILE"
echo

# Function to cleanup at start
cleanup_at_start() {
    print_status "Cleaning up at start..."
    
    # Kill any existing receiver processes
    pkill -f "receiver.*$RECEIVER_PORT" 2>/dev/null || true
    
    # Kill any existing sender processes
    pkill -f "sender.*localhost.*$RECEIVER_PORT" 2>/dev/null || true
    
    # Clean up test files and create output directory
    rm -f "$OUTPUT_FILE" "$OUTPUT_FILE"_timestamps.txt "$RECEIVER_LOG" "$SENDER_LOG" 2>/dev/null || true
    mkdir -p "$(dirname "$OUTPUT_FILE")" 2>/dev/null || true
    
    print_success "Cleanup completed"
}

# Change to project root
cd "$PROJECT_ROOT"

# Perform cleanup at start
cleanup_at_start
echo

# Step 1: Build and run unit tests
print_step "Step 1: Building project and running unit tests"
echo "=================================================="
if ./scripts/clean_build_test.sh "$BUILD_TYPE"; then
    print_success "Build and unit tests completed successfully"
else
    print_error "Build or unit tests failed"
    exit 1
fi
echo

# Step 2: Verify clean state
print_step "Step 2: Verifying clean state"
echo "=================================="
if [ -f "$OUTPUT_FILE" ] || [ -f "$OUTPUT_FILE"_timestamps.txt ] || [ -f "$RECEIVER_LOG" ] || [ -f "$SENDER_LOG" ]; then
    print_warning "Some test files still exist, cleaning up..."
    rm -f "$OUTPUT_FILE" "$OUTPUT_FILE"_timestamps.txt "$RECEIVER_LOG" "$SENDER_LOG" 2>/dev/null || true
else
    print_success "Clean state verified"
fi

# Ensure output directory exists
mkdir -p "$(dirname "$OUTPUT_FILE")" 2>/dev/null || true
print_status "Output directory: $(dirname "$OUTPUT_FILE")"
echo

# Step 3: Start receiver in background
print_step "Step 3: Starting receiver"
echo "============================="
print_status "Starting receiver on port $RECEIVER_PORT..."
./build/bin/receiver "$OUTPUT_FILE" "$RECEIVER_PORT" > "$RECEIVER_LOG" 2>&1 &
RECEIVER_PID=$!

# Wait for receiver to start
sleep 3

# Check if receiver is still running
if ! kill -0 $RECEIVER_PID 2>/dev/null; then
    print_error "Receiver failed to start"
    cat "$RECEIVER_LOG"
    exit 1
fi

print_success "Receiver started successfully (PID: $RECEIVER_PID)"
echo

# Step 4: Start sender
print_step "Step 4: Starting sender"
echo "=========================="
print_status "Starting sender with input file: $INPUT_FILE"
if ./build/bin/sender "$INPUT_FILE" localhost "$RECEIVER_PORT" > "$SENDER_LOG" 2>&1; then
    print_success "Sender completed successfully"
else
    print_error "Sender failed"
    cat "$SENDER_LOG"
    exit 1
fi
echo

# Step 5: Wait for receiver to finish
print_step "Step 5: Waiting for receiver to finish"
echo "=========================================="
print_status "Waiting for receiver to finish processing..."
wait $RECEIVER_PID || true
RECEIVER_PID=""
print_success "Receiver finished"
echo

# Step 6: Check data integrity
print_step "Step 6: Checking data integrity"
echo "==================================="
if [ ! -f "$OUTPUT_FILE" ]; then
    print_error "Output file not found: $OUTPUT_FILE"
    exit 1
fi

if [ ! -f "$INPUT_FILE" ]; then
    print_error "Input file not found: $INPUT_FILE"
    exit 1
fi

# Compare file sizes
INPUT_SIZE=$(wc -c < "$INPUT_FILE")
OUTPUT_SIZE=$(wc -c < "$OUTPUT_FILE")

print_status "Input file size: $INPUT_SIZE bytes"
print_status "Output file size: $OUTPUT_SIZE bytes"

if [ "$INPUT_SIZE" -eq "$OUTPUT_SIZE" ]; then
    print_success "File sizes match"
else
    print_error "File sizes don't match!"
    exit 1
fi

# Binary comparison
if diff "$INPUT_FILE" "$OUTPUT_FILE" >/dev/null; then
    print_success "Data integrity verified - files are identical!"
else
    print_error "Data integrity check failed - files differ!"
    exit 1
fi
echo

# Step 7: Analyze timestamp differences
print_step "Step 7: Analyzing timestamp differences"
echo "==========================================="
TIMESTAMP_FILE="$OUTPUT_FILE"_timestamps.txt

if [ ! -f "$TIMESTAMP_FILE" ]; then
    print_error "Timestamp file not found: $TIMESTAMP_FILE"
    exit 1
fi

# Count data units
DATA_UNITS=$(wc -l < "$TIMESTAMP_FILE")
print_status "Total data units received: $DATA_UNITS"

# Run timestamp analysis
if command -v python3 >/dev/null 2>&1; then
    if [ -f "scripts/analyze_timestamps.py" ]; then
        print_status "Running timestamp analysis..."
        echo "----------------------------------------"
        python3 scripts/analyze_timestamps.py "$TIMESTAMP_FILE"
        echo "----------------------------------------"
    else
        print_warning "scripts/analyze_timestamps.py not found, skipping detailed analysis"
    fi
else
    print_warning "Python3 not available, skipping timestamp analysis"
fi

# Also print a simple summary of timestamp differences
print_status "Timestamp differences summary:"
if [ -f "$TIMESTAMP_FILE" ]; then
    # Extract timestamps and calculate differences
    echo "----------------------------------------"
    echo "Timestamp differences (in milliseconds):"
    echo "----------------------------------------"
    
    # Read timestamps and calculate differences
    prev_timestamp=""
    line_num=0
    
    while IFS= read -r line; do
        # Extract timestamp from line like: "2025-08-04 22:21:30.783 - Video Unit: 6 bytes (length: 2)"
        timestamp=$(echo "$line" | grep -o '^[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} [0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\.[0-9]\{3\}' | head -1)
        
        if [ ! -z "$timestamp" ]; then
            line_num=$((line_num + 1))
            
            if [ ! -z "$prev_timestamp" ]; then
                # Calculate difference using date command
                diff_ms=$(python3 -c "
import datetime
t1 = datetime.datetime.strptime('$prev_timestamp', '%Y-%m-%d %H:%M:%S.%f')
t2 = datetime.datetime.strptime('$timestamp', '%Y-%m-%d %H:%M:%S.%f')
diff = (t2 - t1).total_seconds() * 1000
print(f'{diff:.1f}')
" 2>/dev/null || echo "N/A")
                
                echo "Between $((line_num-1)) and $line_num: ${diff_ms} ms"
            fi
            prev_timestamp="$timestamp"
        fi
    done < "$TIMESTAMP_FILE"
    
    echo "----------------------------------------"
else
    print_warning "Timestamp file not found for manual analysis"
fi
echo

# Step 8: Display receiver statistics
print_step "Step 8: Receiver statistics"
echo "==============================="
if [ -f "$RECEIVER_LOG" ]; then
    print_status "Receiver log output:"
    echo "----------------------"
    tail -30 "$RECEIVER_LOG"
    echo "----------------------"
fi

if [ -f "$SENDER_LOG" ]; then
    print_status "Sender log output:"
    echo "----------------------"
    tail -30 "$SENDER_LOG"
    echo "----------------------"
fi
echo

# Final summary
print_step "Test Workflow Summary"
echo "========================"
print_success "✅ Build and unit tests: PASSED"
print_success "✅ Receiver startup: PASSED"
print_success "✅ Sender execution: PASSED"
print_success "✅ Data integrity: PASSED"
print_success "✅ Timestamp analysis: COMPLETED"
print_success "✅ Full workflow: COMPLETED SUCCESSFULLY"

echo
print_success "🎉 All tests passed! Video transport system is working correctly." 