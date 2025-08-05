#include <iostream>
#include "AsioSender.hpp"
#include "DataProvider.hpp"
#include "DataFile.hpp"
#include "DataUnitConverter.hpp"

#include <vector>
#include <stdexcept>
#include <chrono>
#include <functional>

int main(int argc, char *argv[]) {
  std::cout << "Video Transport Sender" << std::endl;

  if (argc != 4) {
    std::cerr << "Usage: " + std::string(argv[0]) +
                     " <input_file> <destination_ip> <port>"
              << std::endl;
    std::cerr << "Example: " + std::string(argv[0]) +
                     " resources/front_0.bin 127.0.0.1 8080"
              << std::endl;
    return 1;
  }

  std::string filename = argv[1];
  std::string destinationIp = argv[2];
  uint16_t destinationPort = static_cast<uint16_t>(std::stoi(argv[3]));

  try {
    auto dataFile = std::make_unique<DataFile>(filename);
    auto dataProvider = std::make_unique<DataProvider>(std::move(dataFile));
    auto socket = std::make_unique<AsioSender>(destinationIp, destinationPort,
                                               std::move(dataProvider));

    socket->startTransport(std::chrono::milliseconds(10));
  } catch (const std::exception &e) {
    std::cerr << "Error: " + std::string(e.what()) << std::endl;
    return 1;
  }

  return 0;
}
