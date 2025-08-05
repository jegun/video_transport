#pragma once

#include "DataUnit.hpp"
#include <vector>
#include <optional>
#include <cstdint>

class IDataUnitConverter {
public:
  virtual ~IDataUnitConverter() = default;
  virtual std::vector<char> encodeDataUnit(const DataUnit &unit) = 0;
  virtual std::optional<DataUnit>
  decodeDataUnit(const std::vector<char> &data) = 0;
};

class DataUnitConverter : public IDataUnitConverter {
public:
  std::vector<char> encodeDataUnit(const DataUnit &unit) override;

  std::optional<DataUnit>
  decodeDataUnit(const std::vector<char> &data) override;

  std::optional<uint32_t> decodeHeader(const std::vector<char> &data);

private:
  std::vector<char> buffer_;
};