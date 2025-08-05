#include "TimestampWriter.hpp"

#include "Constants.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include "DataUnit.hpp"

TimestampWriter::TimestampWriter(const std::string &filename)
    : filename_(filename) {
  open(filename);
}

TimestampWriter::~TimestampWriter() { close(); }

void TimestampWriter::open(const std::string &filename) {
  file_.open(filename, std::ios::out | std::ios::trunc);
  if (!file_.is_open()) {
    throw std::runtime_error("Could not open file: " + filename);
  }
}

void TimestampWriter::write(const DataUnit &dataUnit) {
  if (file_.is_open()) {
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    ss << " - Video Unit: " << (Constants::HeaderSizeBytes + dataUnit.length)
       << " bytes (length: " << dataUnit.length << ")" << std::endl;

    file_ << ss.str();
    file_.flush();
  }
}

void TimestampWriter::close() {
  if (file_.is_open()) {
    file_.close();
  }
}