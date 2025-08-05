#include "DataAcceptor.hpp"
#include "DataFile.hpp"
#include "TimestampWriter.hpp"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include "DataUnitConverter.hpp"

DataAcceptor::DataAcceptor(std::unique_ptr<IDataFile> videoDataWriter,
                           std::unique_ptr<ITimestampWriter> timestampWriter)
    : videoDataWriter_(std::move(videoDataWriter)),
      timestampWriter_(std::move(timestampWriter)),
      converter_(std::make_unique<DataUnitConverter>()) {}

void DataAcceptor::processRawData(const std::vector<char> &rawData) {
  totalBytesReceived_ += rawData.size();

  auto dataUnit = converter_->decodeDataUnit(rawData);
  if (!dataUnit.has_value()) {
    return;
  }

  std::vector<char> binaryData = converter_->encodeDataUnit(dataUnit.value());
  videoDataWriter_->writeBinaryData(binaryData);

  timestampWriter_->write(dataUnit.value());
  dataUnitsReceived_++;
}

size_t DataAcceptor::getDataUnitsReceived() const { return dataUnitsReceived_; }

size_t DataAcceptor::getTotalBytesReceived() const {
  return totalBytesReceived_;
}
