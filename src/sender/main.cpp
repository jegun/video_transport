#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <functional>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

using boost::asio::ip::tcp;

std::vector<char> loadBinaryFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  auto fileSize = file.tellg();
  if (fileSize <= 0) {
    throw std::runtime_error("File is empty or invalid: " + filename);
  }

  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(fileSize);
  file.read(buffer.data(), fileSize);

  if (file.gcount() != fileSize) {
    throw std::runtime_error("Failed to read entire file: " + filename);
  }

  return buffer;
}

std::vector<std::vector<char>> parseDataUnits(const std::vector<char> &buffer) {
  std::vector<std::vector<char>> dataUnits;
  size_t offset = 0;

  std::cout << "Parsing buffer of size: " << buffer.size() << std::endl;

  while (offset + 4 <= buffer.size()) {
    uint32_t length = 0;
    for (int i = 0; i < 4; ++i) {
      length |=
          static_cast<uint32_t>(static_cast<unsigned char>(buffer[offset + i]))
          << (8 * (3 - i));
    }

    std::cout << "At offset " << offset << ": length = " << length << std::endl;

    if (length == 0) {
      std::cerr << "Warning: Zero length at offset " << offset << std::endl;
      offset++;
      continue;
    }

    if (length > 10000000) {
      std::cerr << "Warning: Length too large " << length << " at offset "
                << offset << std::endl;
      offset++;
      continue;
    }

    if (offset + 4 + length > buffer.size()) {
      std::cerr << "Warning: Incomplete data unit at offset " << offset
                << " (need " << (4 + length) << " bytes, have "
                << (buffer.size() - offset) << ")" << std::endl;
      break;
    }

    std::vector<char> dataUnit(buffer.begin() + offset,
                               buffer.begin() + offset + 4 + length);
    dataUnits.push_back(std::move(dataUnit));

    std::cout << "Extracted data unit " << dataUnits.size() << " with "
              << (4 + length) << " bytes (length: " << length << ")"
              << std::endl;

    offset += 4 + length;
  }

  return dataUnits;
}

void sendDataUnits(const std::vector<std::vector<char>> &dataUnits,
                   const std::string &destinationIp, uint16_t destinationPort) {
  boost::asio::io_context ioContext;
  tcp::socket socket(ioContext);

  tcp::resolver resolver(ioContext);
  auto endpoints =
      resolver.resolve(destinationIp, std::to_string(destinationPort));

  boost::asio::connect(socket, endpoints);

  socket.set_option(boost::asio::ip::tcp::no_delay(true));

  std::cout << "Connected to " << destinationIp << ":" << destinationPort
            << std::endl;

  std::cout << "Sending " << dataUnits.size() << " data units to "
            << destinationIp << ":" << destinationPort << std::endl;

  boost::asio::steady_timer timer(ioContext);
  auto startTime = std::chrono::high_resolution_clock::now();
  size_t currentUnit = 0;
  size_t currentOffset = 0;
  std::array<char, 1024> sendBuffer;

  std::function<void(const boost::system::error_code &)> sendNextChunk;
  sendNextChunk = [&](const boost::system::error_code &ec) {
    if (ec) {
      std::cerr << "Timer error: " << ec.message() << std::endl;
      return;
    }

    if (currentUnit >= dataUnits.size()) {
      std::cout << "All data units sent successfully!" << std::endl;
      return;
    }

    const auto &dataUnit = dataUnits[currentUnit];
    size_t remainingBytes = dataUnit.size() - currentOffset;
    size_t bytesToSend = std::min(remainingBytes, sendBuffer.size());

    std::copy(dataUnit.begin() + currentOffset,
              dataUnit.begin() + currentOffset + bytesToSend,
              sendBuffer.begin());

    boost::asio::write(socket,
                       boost::asio::buffer(sendBuffer.data(), bytesToSend));

    currentOffset += bytesToSend;

    if (currentOffset >= dataUnit.size()) {
      auto sendTime = std::chrono::high_resolution_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
          sendTime - startTime);

      std::cout << "Sent data unit " << (currentUnit + 1) << "/"
                << dataUnits.size() << " (" << dataUnit.size() << " bytes) at "
                << std::fixed << std::setprecision(2)
                << elapsed.count() / 1000.0 << "ms" << std::endl;

      currentUnit++;
      currentOffset = 0;

      if (currentUnit < dataUnits.size()) {
        timer.expires_after(std::chrono::milliseconds(10));
        timer.async_wait(sendNextChunk);
      }
    } else {
      sendNextChunk(boost::system::error_code{});
    }
  };

  sendNextChunk(boost::system::error_code{});

  ioContext.run();
}

int main(int argc, char *argv[]) {
  std::cout << "Video Transport Sender" << std::endl;

  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <input_file> <destination_ip> <port>"
              << std::endl;
    std::cerr << "Example: " << argv[0]
              << " resources/front_0.bin 127.0.0.1 8080" << std::endl;
    return 1;
  }

  std::string filename = argv[1];
  std::string destinationIp = argv[2];
  uint16_t destinationPort = static_cast<uint16_t>(std::stoi(argv[3]));

  try {
    std::cout << "Loading file: " << filename << std::endl;
    auto fileData = loadBinaryFile(filename);
    std::cout << "Successfully loaded " << fileData.size() << " bytes"
              << std::endl;

    std::cout << "Parsing video data units..." << std::endl;
    auto dataUnits = parseDataUnits(fileData);
    std::cout << "Found " << dataUnits.size() << " video data units"
              << std::endl;

    if (dataUnits.empty()) {
      std::cerr << "Error: No video data units found in file" << std::endl;
      return 1;
    }

    sendDataUnits(dataUnits, destinationIp, destinationPort);

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}