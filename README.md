# Video Transport

A C++ application for sending video data units over socket with precise timing control.

## Requirements

- **CMake**: 3.16 or higher
- **Compiler**: Clang 15.0.0 or compatible C++17 compiler
- **Dependencies**:
  - Boost (system component)
  - Google Test (for unit testing)

## Description

Video Transport sends data units over TCP with a 10ms interval between sequential calls. Each data unit consists of:

- **4-byte header**: Contains the length of the video data (big-endian format)
- **Video data**: Raw video data following the header

The maximum size of a data unit is currently set to 16387 bytes.

Due to TCP's packet nature, data sent from the sender may be split into several packets. The receiver includes an accumulating buffer that collects data until a complete data unit is received.

## Architecture

### Core Components

- **DataUnit**: Represents a video data unit with length and raw data
- **DataUnitConverter**: Handles encoding/decoding of data units to/from binary format
- **DataProvider**: Reads data units from files and provides them to the sender
- **DataAcceptor**: Processes received raw data and extracts complete data units
- **AsioSender**: Sends data units over TCP using Boost.Asio
- **AsioReceiver**: Receives data units over TCP using Boost.Asio
- **TimestampWriter**: Records timestamps for received data units and writes them into a file

### Network Protocol

- **Transport**: TCP
- **Data Format**: 4-byte length header + raw video data
- **Timing**: 10ms intervals between data unit transmissions
- **Buffering**: Receiver accumulates partial data until complete units are available

## Limitations

- **No Network Recovery**: Network failures are not handled automatically
- **No Session Management**: Multiple client support is not implemented

## Potential Improvements

- **Memory Management**: Currently uses dynamically allocated vectors which can cause latency. Pre-allocated arrays would be more efficient
- **Session Support**: Implement multiple client handling with session management
- **Error Recovery**: Add network failure handling and recovery mechanisms
- **Bigger data unit size support**: Consider increasing the maximum DataUnit size. Note: This will require updating buffer allocation and protocol logic in both sender and receiver to handle the new size correctly.

## Building

```bash
./scripts/clean_build_test.sh
```

## Testing

```bash
# Run all tests
ctest

# Run specific test
./bin/DataUnitTests
./bin/DataFileTests
./bin/DataProviderTests
./bin/DataAcceptorTests
./bin/DataUnitConverterTests
```

## Usage

### Sender
```bash
./bin/sender <input_file> <host> <port>
```

### Receiver
```bash
./bin/receiver <port>
```

## Full Workflow Test

Run the complete end-to-end test:
```bash
./scripts/full_test_workflow.sh
```
