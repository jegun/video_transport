#include "DataProvider.hpp"
#include "DataFile.hpp"
#include "Constants.hpp"
#include "DataUnit.hpp"
#include "DataUnitConverter.hpp"

#include <iostream>

DataProvider::DataProvider(std::unique_ptr<IDataFile> dataFile)
    : dataFile_(std::move(dataFile)),
      converter_(std::make_unique<DataUnitConverter>()) {}

std::optional<std::vector<char>> DataProvider::getNextData() {
  auto binaryDataUnit = dataFile_->readNextDataUnit();
  if (!binaryDataUnit.has_value()) {
    return std::nullopt; // No more data units
  }

  auto dataUnit = converter_->decodeDataUnit(*binaryDataUnit);

  if (!dataUnit.has_value()) {
    throw std::runtime_error("Failed to decode data unit from binary data.");
  }

  if (dataUnit->length >
      Constants::MaxPacketSize - Constants::HeaderSizeBytes) {
    throw std::runtime_error(
        "Data unit length exceeds MaxPacketSize - HeaderSizeBytes: " +
        std::to_string(dataUnit->length) + " > " +
        std::to_string(Constants::MaxPacketSize - Constants::HeaderSizeBytes));
  }

  if (dataUnit->length != dataUnit->data.size()) {
    throw std::runtime_error("Data unit length does not match data size: " +
                             std::to_string(dataUnit->length) +
                             " != " + std::to_string(dataUnit->data.size()));
  }

  return binaryDataUnit;
}