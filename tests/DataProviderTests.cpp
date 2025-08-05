#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DataProvider.hpp"
#include "DataFile.hpp"
#include "DataUnitConverter.hpp"
#include "Constants.hpp"
#include <chrono>
#include <boost/system/error_code.hpp>

class MockDataFile : public IDataFile {
public:
  MOCK_METHOD(void, writeBinaryData, (const std::vector<char> &data),
              (override));
  MOCK_METHOD(std::optional<std::vector<char>>, readNextDataUnit, (),
              (override));
};

class DataProviderTest : public ::testing::Test {
protected:
  void SetUp() override {
    mockDataFile_ = std::make_unique<MockDataFile>();
    converter_ = std::make_unique<DataUnitConverter>();
  }

  void TearDown() override { dataProvider_.reset(); }

  std::unique_ptr<MockDataFile> mockDataFile_;
  std::unique_ptr<DataProvider> dataProvider_;
  std::unique_ptr<DataUnitConverter> converter_;
};

TEST_F(DataProviderTest, SmallDataUnit) {
  DataUnit smallUnit{2, {'H', 'i'}};
  std::vector<char> smallBinaryData = converter_->encodeDataUnit(smallUnit);

  EXPECT_CALL(*mockDataFile_, readNextDataUnit())
      .WillOnce(testing::Return(smallBinaryData))
      .WillOnce(testing::Return(std::nullopt));

  dataProvider_ = std::make_unique<DataProvider>(std::move(mockDataFile_));

  auto data = dataProvider_->getNextData();
  EXPECT_TRUE(data.has_value());
  EXPECT_EQ(data->size(), 6);
  EXPECT_EQ(data->at(4), 'H');
  EXPECT_EQ(data->at(5), 'i');

  auto endData = dataProvider_->getNextData();
  EXPECT_FALSE(endData.has_value());
}

TEST_F(DataProviderTest, NoDataLeft) {
  DataUnit smallUnit{2, {'H', 'i'}};
  std::vector<char> smallBinaryData = converter_->encodeDataUnit(smallUnit);

  EXPECT_CALL(*mockDataFile_, readNextDataUnit())
      .WillOnce(testing::Return(smallBinaryData))
      .WillOnce(testing::Return(std::nullopt));

  dataProvider_ = std::make_unique<DataProvider>(std::move(mockDataFile_));

  auto data = dataProvider_->getNextData();

  auto endData = dataProvider_->getNextData();
  EXPECT_FALSE(endData.has_value());
}

TEST_F(DataProviderTest, LargeDataUnit) {
  std::vector<char> largeData(
      Constants::MaxPacketSize - Constants::HeaderSizeBytes, 'X');
  DataUnit largeUnit{Constants::MaxPacketSize - Constants::HeaderSizeBytes,
                     largeData};
  std::vector<char> largeBinaryData = converter_->encodeDataUnit(largeUnit);

  EXPECT_CALL(*mockDataFile_, readNextDataUnit())
      .WillOnce(testing::Return(largeBinaryData));

  dataProvider_ = std::make_unique<DataProvider>(std::move(mockDataFile_));

  auto data = dataProvider_->getNextData();
  EXPECT_TRUE(data.has_value());
  EXPECT_EQ(data->size(), Constants::MaxPacketSize);
}

TEST_F(DataProviderTest, OversizedDataUnit) {
  std::vector<char> oversizedData(
      Constants::MaxPacketSize - Constants::HeaderSizeBytes + 1000, 'Y');
  DataUnit oversizedUnit{Constants::MaxPacketSize - Constants::HeaderSizeBytes +
                             1000,
                         oversizedData};
  std::vector<char> oversizedBinaryData =
      converter_->encodeDataUnit(oversizedUnit);

  EXPECT_CALL(*mockDataFile_, readNextDataUnit())
      .WillOnce(testing::Return(oversizedBinaryData));

  dataProvider_ = std::make_unique<DataProvider>(std::move(mockDataFile_));

  EXPECT_THROW(dataProvider_->getNextData(), std::runtime_error);
}

TEST_F(DataProviderTest, WrongLengthDataUnit) {
  std::vector<char> actualData{'H', 'i'};
  DataUnit wrongLengthUnit{5, actualData};
  std::vector<char> wrongLengthBinaryData =
      converter_->encodeDataUnit(wrongLengthUnit);

  EXPECT_CALL(*mockDataFile_, readNextDataUnit())
      .WillOnce(testing::Return(wrongLengthBinaryData));

  dataProvider_ = std::make_unique<DataProvider>(std::move(mockDataFile_));

  EXPECT_THROW(dataProvider_->getNextData(), std::runtime_error);
}