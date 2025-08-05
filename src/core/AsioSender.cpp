#include "AsioSender.hpp"
#include "DataProvider.hpp"
#include "DataFile.hpp"
#include "DataUnitConverter.hpp"
#include <iostream>
#include <chrono>

using boost::asio::ip::tcp;

AsioSender::AsioSender(const std::string &destinationIp,
                       uint16_t destinationPort,
                       std::unique_ptr<DataProvider> dataProvider)
    : dataProvider_(std::move(dataProvider)), socket_(ioContext_),
      timer_(ioContext_) {
  boost::asio::ip::tcp::resolver resolver(ioContext_);
  auto endpoints =
      resolver.resolve(destinationIp, std::to_string(destinationPort));
  boost::asio::connect(socket_, endpoints);
  socket_.set_option(boost::asio::ip::tcp::no_delay(true));
}

void AsioSender::startTransport(std::chrono::milliseconds delay) {
  delay_ = delay;
  std::function<void()> processNextData = [this, &processNextData]() {
    try {
      auto data = dataProvider_->getNextData();
      if (data.has_value()) {
        boost::asio::async_write(
            socket_, boost::asio::buffer(data.value()),
            [this, &processNextData](const boost::system::error_code &error,
                                     std::size_t bytesTransferred) {
              try {
                if (!error) {
                  timer_.expires_after(delay_);
                  timer_.async_wait(
                      [this, &processNextData](
                          const boost::system::error_code &timerError) {
                        try {
                          if (!timerError) {
                            processNextData();
                          } else {
                            std::cerr << "Timer error: " << timerError.message()
                                      << std::endl;
                            return;
                          }
                        } catch (const std::exception &ex) {
                          std::cerr
                              << "Exception in timer handler: " << ex.what()
                              << std::endl;
                          return;
                        }
                      });
                } else {
                  std::cerr << "Error sending data: " + error.message()
                            << std::endl;
                  return;
                }
              } catch (const std::exception &ex) {
                std::cerr << "Exception in async_write handler: " << ex.what()
                          << std::endl;
                return;
              }
            });
      } else {
        std::cout << "Transport completed - no more data available"
                  << std::endl;
        return;
      }
    } catch (const std::exception &ex) {
      std::cerr << "Exception in processNextData: " << ex.what() << std::endl;
      return;
    }
  };
  processNextData();
  ioContext_.run();
}