#pragma once

#include <boost/asio.hpp>

class DataProvider;

class AsioSender {
public:
  AsioSender(const std::string &destinationIp, uint16_t destinationPort,
             std::unique_ptr<DataProvider> dataProvider);

  void startTransport(
      std::chrono::milliseconds delay = std::chrono::milliseconds(10));

private:
  std::unique_ptr<DataProvider> dataProvider_;
  boost::asio::io_context ioContext_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::steady_timer timer_;
  std::chrono::milliseconds delay_;
};