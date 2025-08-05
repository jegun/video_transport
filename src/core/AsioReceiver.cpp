#include "AsioReceiver.hpp"
#include "DataAcceptor.hpp"
#include "Constants.hpp"
#include <iostream>
#include <chrono>
#include <array>

using boost::asio::ip::tcp;

AsioReceiver::AsioReceiver(uint16_t port,
                           std::unique_ptr<IDataAcceptor> dataAcceptor)
    : acceptor_(ioContext_, tcp::endpoint(tcp::v4(), port)),
      socket_(ioContext_), dataAcceptor_(std::move(dataAcceptor)) {
  std::cout << "Receiver server listening on 0.0.0.0:" + std::to_string(port)
            << std::endl;
}

void AsioReceiver::start() {
  acceptor_.accept(socket_);
  socket_.set_option(boost::asio::ip::tcp::no_delay(true));
  std::function<void()> processNextData;
  processNextData = [this, &processNextData]() {
    try {
      auto tempBuffer =
          std::make_shared<std::array<char, Constants::MaxPacketSize>>();
      socket_.async_read_some(
          boost::asio::buffer(*tempBuffer),
          [this, &processNextData, tempBuffer](
              const boost::system::error_code &error, std::size_t bytesRead) {
            try {
              if (!error && bytesRead > 0) {
                std::vector<char> receivedData(tempBuffer->begin(),
                                               tempBuffer->begin() + bytesRead);
                dataAcceptor_->processRawData(receivedData);
                processNextData();
              } else {
                if (error == boost::asio::error::eof) {
                  std::cout << "Connection closed by client" << std::endl;
                  return;
                } else {
                  std::cerr << "Error reading data: " + error.message()
                            << std::endl;
                  return;
                }
              }
            } catch (const std::exception &ex) {
              std::cerr << "Exception in async handler: " << ex.what()
                        << std::endl;
              return;
            }
          });
    } catch (const std::exception &ex) {
      std::cerr << "Exception in processNextData: " << ex.what() << std::endl;
      return;
    }
  };
  processNextData();
  ioContext_.run();
}

size_t AsioReceiver::getDataUnitsReceived() const {
  return dataAcceptor_ ? dataAcceptor_->getDataUnitsReceived() : 0;
}

size_t AsioReceiver::getTotalBytesReceived() const {
  return dataAcceptor_ ? dataAcceptor_->getTotalBytesReceived() : 0;
}