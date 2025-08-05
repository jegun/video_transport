#pragma once

#include <boost/asio.hpp>

class IDataAcceptor;

class AsioReceiver {
public:
  AsioReceiver(uint16_t port, std::unique_ptr<IDataAcceptor> dataAcceptor);

  void start();
  size_t getDataUnitsReceived() const;
  size_t getTotalBytesReceived() const;

private:
  boost::asio::io_context ioContext_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;
  std::unique_ptr<IDataAcceptor> dataAcceptor_;
};