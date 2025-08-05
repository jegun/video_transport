#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DataAcceptor.hpp"
#include "DataFile.hpp"
#include "DataUnitConverter.hpp"
#include "TimestampWriter.hpp"
#include <memory>

class MockDataFile : public IDataFile {
public:
  MOCK_METHOD(void, writeBinaryData, (const std::vector<char> &data),
              (override));
  MOCK_METHOD(std::optional<std::vector<char>>, readNextDataUnit, (),
              (override));
};

class MockTextFileWriter : public ITimestampWriter {
public:
  MOCK_METHOD(void, write, (const DataUnit &dataUnit), (override));
  MOCK_METHOD(void, open, (const std::string &filename), (override));
  MOCK_METHOD(void, close, (), (override));
};

class DataAcceptorTest : public ::testing::Test {
protected:
  void SetUp() override {
    mockDataFile_ = std::make_unique<MockDataFile>();
    mockTimestampWriter_ = std::make_unique<MockTextFileWriter>();
    converter_ = std::make_unique<DataUnitConverter>();
  }
  void TearDown() override { dataAcceptor_.reset(); }
  std::unique_ptr<MockDataFile> mockDataFile_;
  std::unique_ptr<MockTextFileWriter> mockTimestampWriter_;
  std::unique_ptr<DataAcceptor> dataAcceptor_;
  std::unique_ptr<DataUnitConverter> converter_;
};

TEST_F(DataAcceptorTest, ProcessCompleteDataUnit) {
  std::vector<char> rawData = {0x00, 0x00, 0x00, 0x02, 'H', 'i'};

  auto dataUnit = converter_->decodeDataUnit(rawData);
  EXPECT_TRUE(dataUnit.has_value());

  EXPECT_CALL(*mockDataFile_, writeBinaryData(testing::_))
      .WillOnce(testing::Invoke(
          [this, &dataUnit](const std::vector<char> &binaryData) {
            EXPECT_EQ(binaryData.size(), 6);
            EXPECT_EQ(binaryData[4], 'H');
            EXPECT_EQ(binaryData[5], 'i');
          }));
  EXPECT_CALL(*mockTimestampWriter_, write(testing::_))
      .WillOnce(testing::Invoke(
          [](const DataUnit &dataUnit) { EXPECT_EQ(dataUnit.length, 2); }));

  dataAcceptor_ = std::make_unique<DataAcceptor>(
      std::move(mockDataFile_), std::move(mockTimestampWriter_));

  dataAcceptor_->processRawData(rawData);

  EXPECT_EQ(dataAcceptor_->getDataUnitsReceived(), 1);
  EXPECT_EQ(dataAcceptor_->getTotalBytesReceived(), 6);
}
