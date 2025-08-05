#include <iostream>
#include "AsioReceiver.hpp"
#include "DataFile.hpp"
#include "TimestampWriter.hpp"
#include "DataAcceptor.hpp"
#include "DataUnitConverter.hpp"

int main(int argc, char *argv[]) {
  std::cout << "Video Transport Receiver" << std::endl;

  if (argc != 3) {
    std::cerr << "Usage: " + std::string(argv[0]) +
                     " <output_file> <listening_port>"
              << std::endl;
    std::cerr << "Example: " + std::string(argv[0]) +
                     " received_video_data.bin 8080"
              << std::endl;
    return 1;
  }

  std::string outputFile = argv[1];
  uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

  try {
    auto videoDataWriter =
        std::make_unique<DataFile>(outputFile, DataFile::Mode::Write);
    auto timestampWriter =
        std::make_unique<TimestampWriter>(outputFile + "_timestamps.txt");

    auto dataAcceptor = std::make_unique<DataAcceptor>(
        std::move(videoDataWriter), std::move(timestampWriter));

    auto receiver =
        std::make_unique<AsioReceiver>(port, std::move(dataAcceptor));

    std::cout << "Receiver started. Waiting for connections..." << std::endl;

    receiver->start();

    std::stringstream stats;
    stats << "Total data units received: " << receiver->getDataUnitsReceived()
          << std::endl;
    stats << "Total bytes received: " << receiver->getTotalBytesReceived();
    std::cout << "\n=== RECEIVER STATISTICS ===" << std::endl;
    std::cout << stats.str() << std::endl;
    std::cout << "===========================" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " + std::string(e.what()) << std::endl;
    return 1;
  }

  return 0;
}