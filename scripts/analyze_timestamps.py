#!/usr/bin/env python3
import re
from datetime import datetime

def parse_timestamp(line):
    # Extract timestamp from line like: "2025-08-02 17:21:27.306355 - Video Unit: 6 bytes (length: 2)"
    match = re.match(r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+)', line)
    if match:
        return datetime.strptime(match.group(1), '%Y-%m-%d %H:%M:%S.%f')
    return None

def analyze_recent_timestamps(filename):
    timestamps = []
    
    with open(filename, 'r') as f:
        for line in f:
            ts = parse_timestamp(line.strip())
            if ts:
                timestamps.append(ts)
    
    # Find the most recent group of timestamps (likely the last test run)
    if len(timestamps) < 2:
        print("Need at least 2 timestamps to calculate differences")
        return
    
    # Look for a large gap to separate different runs
    recent_start = 0
    for i in range(1, len(timestamps)):
        diff = timestamps[i] - timestamps[i-1]
        if diff.total_seconds() > 10:  # More than 10 seconds gap indicates different run
            recent_start = i
            break
    
    recent_timestamps = timestamps[recent_start:]
    
    print(f"Analyzing most recent run with {len(recent_timestamps)} timestamps")
    print(f"Recent run started at: {recent_timestamps[0]}")
    print()
    
    if len(recent_timestamps) < 2:
        print("Need at least 2 timestamps in recent run")
        return
    
    print("Timestamp differences (in microseconds):")
    print("-" * 50)
    
    for i in range(1, len(recent_timestamps)):
        diff = recent_timestamps[i] - recent_timestamps[i-1]
        diff_microseconds = diff.total_seconds() * 1_000_000
        print(f"Between {i} and {i+1}: {diff_microseconds:.0f} μs ({diff_microseconds/1000:.1f} ms)")
    
    print()
    print("Summary:")
    print(f"First timestamp: {recent_timestamps[0]}")
    print(f"Last timestamp: {recent_timestamps[-1]}")
    total_time = recent_timestamps[-1] - recent_timestamps[0]
    print(f"Total time: {total_time.total_seconds() * 1000:.1f} ms")
    if len(recent_timestamps) > 1:
        avg_interval = total_time.total_seconds() * 1_000_000 / (len(recent_timestamps)-1)
        print(f"Average interval: {avg_interval:.0f} μs ({avg_interval/1000:.1f} ms)")

if __name__ == "__main__":
    import sys
    filename = sys.argv[1] if len(sys.argv) > 1 else "received_timestamps.txt"
    analyze_recent_timestamps(filename) 