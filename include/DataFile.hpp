#pragma once

#include <string>
#include <fstream>
#include <optional>
#include <vector>

class IDataFile {
public:
  virtual ~IDataFile() = default;
  virtual void writeBinaryData(const std::vector<char> &data) = 0;
  virtual std::optional<std::vector<char>> readNextDataUnit() = 0;
};

class DataFile : public IDataFile {
public:
  enum class Mode { Read, Write };

  DataFile(const std::string &filename, Mode mode = Mode::Read);
  ~DataFile() override;

  void writeBinaryData(const std::vector<char> &data) override;
  std::optional<std::vector<char>> readNextDataUnit() override;

private:
  std::string filename_;
  std::fstream file_;
  Mode mode_;
  size_t bytesWritten_ = 0;
};