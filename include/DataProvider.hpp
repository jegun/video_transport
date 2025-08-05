#pragma once
#include <optional>
#include <memory>
#include <vector>

class IDataFile;
class DataUnitConverter;

class DataProvider {
public:
  explicit DataProvider(std::unique_ptr<IDataFile> dataFile);

  std::optional<std::vector<char>> getNextData();

private:
  std::unique_ptr<IDataFile> dataFile_;
  std::unique_ptr<DataUnitConverter> converter_;
};