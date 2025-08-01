#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

using boost::asio::ip::tcp;

class AsyncTimestampWriter {
public:
  AsyncTimestampWriter() : running_(true) {
    writerThread_ = std::thread([this] { writeTimestampsLoop(); });
  }

  ~AsyncTimestampWriter() {
    running_ = false;

    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      cv_.notify_one();
    }

    if (writerThread_.joinable()) {
      writerThread_.join();
    }
  }

  void addTimestamp(const std::string &timestamp) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    timestampQueue_.push(timestamp);
    cv_.notify_one();
  }

private:
  void writeTimestampsLoop() {
    std::ofstream timestampFile("received_timestamps.txt",
                                std::ios::out | std::ios::app);
    if (!timestampFile.is_open()) {
      std::cerr << "Error: Could not open timestamp file for writing"
                << std::endl;
      return;
    }

    std::cout << "Timestamp writer started. Writing to received_timestamps.txt"
              << std::endl;

    while (running_) {
      std::string timestamp;

      {
        std::unique_lock<std::mutex> lock(queueMutex_);
        cv_.wait(lock,
                 [this] { return !timestampQueue_.empty() || !running_; });

        if (!running_ && timestampQueue_.empty()) {
          break;
        }

        if (!timestampQueue_.empty()) {
          timestamp = timestampQueue_.front();
          timestampQueue_.pop();
        }
      }

      if (!timestamp.empty()) {
        timestampFile << timestamp << std::endl;
        timestampFile.flush();
      }
    }

    timestampFile.close();
    std::cout << "Timestamp writer stopped" << std::endl;
  }

  std::thread writerThread_;
  std::mutex queueMutex_;
  std::condition_variable cv_;
  std::queue<std::string> timestampQueue_;
  std::atomic<bool> running_;
};

class ReceiverSession : public std::enable_shared_from_this<ReceiverSession> {
public:
  ReceiverSession(tcp::socket socket, AsyncTimestampWriter &timestampWriter)
      : socket_(std::move(socket)), timestampWriter_(timestampWriter) {
    outputFile_.open("received_video_data.bin",
                     std::ios::binary | std::ios::out);
    if (!outputFile_.is_open()) {
      std::cerr << "Error: Could not open output file for writing" << std::endl;
    } else {
      std::cout << "Output file opened: received_video_data.bin" << std::endl;
    }
  }

  ~ReceiverSession() {
    if (outputFile_.is_open()) {
      outputFile_.close();
      std::cout << "Output file closed. Total bytes written: "
                << totalBytesReceived_ << std::endl;
    }
  }

  void start() {
    std::cout << "New client connected from "
              << socket_.remote_endpoint().address().to_string() << ":"
              << socket_.remote_endpoint().port() << std::endl;

    receiveData();
  }

private:
  void receiveData() {
    auto self(shared_from_this());

    socket_.async_read_some(
        boost::asio::buffer(receiveBuffer_),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            processReceivedData(length);

            receiveData();
          } else {
            std::cout << "Client disconnected: " << ec.message() << std::endl;
          }
        });
  }

  void processReceivedData(std::size_t length) {
    receivedBuffer_.insert(receivedBuffer_.end(), receiveBuffer_.begin(),
                           receiveBuffer_.begin() + length);

    if (outputFile_.is_open()) {
      outputFile_.write(receiveBuffer_.data(), length);
      if (outputFile_.good()) {
        totalBytesReceived_ += length;
      } else {
        std::cerr << "Error writing to file!" << std::endl;
      }
    } else {
      std::cerr << "Output file is not open!" << std::endl;
    }

    parseVideoUnits();
  }

  void parseVideoUnits() {
    size_t offset = 0;

    while (offset + 4 <= receivedBuffer_.size()) {
      uint32_t length = 0;
      for (int i = 0; i < 4; ++i) {
        length |= static_cast<uint32_t>(
                      static_cast<unsigned char>(receivedBuffer_[offset + i]))
                  << (8 * (3 - i));
      }

      if (length == 0 || length > 10000000) {
        offset++;
        continue;
      }

      if (offset + 4 + length > receivedBuffer_.size()) {
        break;
      }

      std::vector<char> dataUnit(receivedBuffer_.begin() + offset,
                                 receivedBuffer_.begin() + offset + 4 + length);

      auto now = std::chrono::high_resolution_clock::now();
      auto timeT = std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now());
      auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                    now.time_since_epoch()) %
                1000000;

      std::stringstream timestampSs;
      timestampSs << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
      timestampSs << "." << std::setfill('0') << std::setw(6) << us.count();
      timestampSs << " - Video Unit: " << (4 + length)
                  << " bytes (length: " << length << ")";

      timestampWriter_.addTimestamp(timestampSs.str());

      std::cout << "Parsed video unit: " << (4 + length)
                << " bytes (length: " << length << ")" << std::endl;

      receivedBuffer_.erase(receivedBuffer_.begin(),
                            receivedBuffer_.begin() + offset + 4 + length);
      offset = 0;
    }
  }

  tcp::socket socket_;
  std::ofstream outputFile_;
  std::array<char, 1024> receiveBuffer_;
  std::size_t totalBytesReceived_ = 0;

  AsyncTimestampWriter &timestampWriter_;

  std::vector<char> receivedBuffer_;
};

class ReceiverServer {
public:
  ReceiverServer(boost::asio::io_context &ioContext,
                 const tcp::endpoint &endpoint)
      : acceptor_(ioContext, endpoint), timestampWriter_() {
    std::cout << "Receiver server listening on "
              << endpoint.address().to_string() << ":" << endpoint.port()
              << std::endl;

    startAccept();
  }

private:
  void startAccept() {
    acceptor_.async_accept([this](boost::system::error_code ec,
                                  tcp::socket socket) {
      if (!ec) {
        std::make_shared<ReceiverSession>(std::move(socket), timestampWriter_)
            ->start();
      } else {
        std::cerr << "Accept failed: " << ec.message() << std::endl;
      }

      startAccept();
    });
  }

  tcp::acceptor acceptor_;
  AsyncTimestampWriter timestampWriter_;
};

int main(int argc, char *argv[]) {
  std::cout << "Video Transport Receiver" << std::endl;

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    std::cerr << "Example: " << argv[0] << " 8080" << std::endl;
    return 1;
  }

  uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));

  try {
    boost::asio::io_context ioContext;

    tcp::endpoint endpoint(tcp::v4(), port);

    ReceiverServer server(ioContext, endpoint);

    std::cout << "Receiver started. Waiting for connections..." << std::endl;

    ioContext.run();

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}