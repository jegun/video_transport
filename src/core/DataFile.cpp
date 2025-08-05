#include "DataFile.hpp"
#include "DataUnitConverter.hpp"
#include "Constants.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>

DataFile::DataFile(const std::string &filename, Mode mode)
    : filename_(filename), mode_(mode) {
  if (mode == Mode::Read) {
    file_.open(filename, std::ios::binary | std::ios::in);
    if (!file_.is_open()) {
      throw std::runtime_error("Could not open file for reading: " + filename);
    }
  } else {
    file_.open(filename, std::ios::binary | std::ios::out);
    if (!file_.is_open()) {
      throw std::runtime_error("Could not open file for writing: " + filename);
    }
    std::cout << "Output file opened: " + filename << std::endl;
  }
}

DataFile::~DataFile() {
  if (file_.is_open()) {
    file_.close();
    if (mode_ == Mode::Write) {
      std::cout << "Output file closed. Total bytes written: " +
                       std::to_string(bytesWritten_)
                << std::endl;
    }
  }
}

void DataFile::writeBinaryData(const std::vector<char> &data) {
  if (!file_.is_open()) {
    throw std::runtime_error("File is not open");
  }

  file_.write(data.data(), data.size());
  bytesWritten_ += data.size();

  file_.flush();
}

std::optional<std::vector<char>> DataFile::readNextDataUnit() {
  if (!file_.is_open()) {
    return std::nullopt;
  }

  if (file_.eof()) {
    return std::nullopt;
  }

  std::vector<char> dataUnit;

  std::vector<char> header(Constants::HeaderSizeBytes);
  for (int i = 0; i < Constants::HeaderSizeBytes; ++i) {
    if (!file_.get(header[i])) {
      return std::nullopt;
    }
  }

  DataUnitConverter converter;
  auto length = converter.decodeHeader(header);
  if (!length.has_value()) {
    return std::nullopt;
  }

  dataUnit.insert(dataUnit.end(), header.begin(), header.end());

  if (length.value() > 0) {
    std::vector<char> data(length.value());
    if (!file_.read(data.data(), length.value())) {
      return std::nullopt;
    }
    dataUnit.insert(dataUnit.end(), data.begin(), data.end());
  }

  return dataUnit;
}