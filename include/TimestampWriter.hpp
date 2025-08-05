#pragma once

#include <fstream>
#include <string>

class DataUnit;

class ITimestampWriter {
public:
  virtual ~ITimestampWriter() = default;
  virtual void write(const DataUnit &dataUnit) = 0;
  virtual void open(const std::string &filename) = 0;
  virtual void close() = 0;
};

class TimestampWriter : public ITimestampWriter {
private:
  std::ofstream file_;
  std::string filename_;

public:
  TimestampWriter(const std::string &filename);
  ~TimestampWriter();

  void write(const DataUnit &dataUnit) override;
  void open(const std::string &filename) override;
  void close() override;
};