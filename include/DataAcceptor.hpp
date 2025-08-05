#pragma once

#include <vector>

class IDataFile;
class ITimestampWriter;
class IDataUnitConverter;

class IDataAcceptor {
public:
  virtual ~IDataAcceptor() = default;
  virtual void processRawData(const std::vector<char> &rawData) = 0;
  virtual size_t getDataUnitsReceived() const = 0;
  virtual size_t getTotalBytesReceived() const = 0;
};

class DataAcceptor : public IDataAcceptor {
public:
  DataAcceptor(std::unique_ptr<IDataFile> videoDataWriter,
               std::unique_ptr<ITimestampWriter> timestampWriter);

  void processRawData(const std::vector<char> &rawData) override;
  size_t getDataUnitsReceived() const override;
  size_t getTotalBytesReceived() const override;

private:
  std::unique_ptr<IDataFile> videoDataWriter_;
  std::unique_ptr<ITimestampWriter> timestampWriter_;
  std::unique_ptr<IDataUnitConverter> converter_;
  size_t dataUnitsReceived_ = 0;
  size_t totalBytesReceived_ = 0;
};