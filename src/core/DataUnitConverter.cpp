#include "DataUnitConverter.hpp"
#include "Constants.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdint>

std::vector<char> DataUnitConverter::encodeDataUnit(const DataUnit &unit) {
  std::vector<char> encodedData;
  encodedData.reserve(Constants::HeaderSizeBytes + unit.length);

  uint32_t length = unit.length;
  for (int i = Constants::HeaderSizeBytes - 1; i >= 0; --i) {
    encodedData.push_back(static_cast<char>((length >> (8 * i)) & 0xFF));
  }

  encodedData.insert(encodedData.end(), unit.data.begin(), unit.data.end());

  return encodedData;
}

std::optional<DataUnit>
DataUnitConverter::decodeDataUnit(const std::vector<char> &data) {
  buffer_.insert(buffer_.end(), data.begin(), data.end());

  while (buffer_.size() >= Constants::HeaderSizeBytes) {
    auto length = decodeHeader(buffer_);
    if (!length.has_value()) {
      return std::nullopt;
    }

    size_t totalDataUnitSize = Constants::HeaderSizeBytes + length.value();
    if (buffer_.size() < totalDataUnitSize) {
      return std::nullopt;
    }

    DataUnit unit;
    unit.length = length.value();
    unit.data.assign(buffer_.begin() + Constants::HeaderSizeBytes,
                     buffer_.begin() + Constants::HeaderSizeBytes +
                         length.value());

    buffer_.erase(buffer_.begin(), buffer_.begin() + totalDataUnitSize);

    return unit;
  }

  return std::nullopt;
}

std::optional<uint32_t>
DataUnitConverter::decodeHeader(const std::vector<char> &data) {
  if (data.size() < Constants::HeaderSizeBytes) {
    return std::nullopt;
  }

  uint32_t length = 0;
  for (int i = 0; i < Constants::HeaderSizeBytes; ++i) {
    length |= static_cast<uint32_t>(static_cast<unsigned char>(data[i]))
              << (8 * (3 - i));
  }

  return length;
}