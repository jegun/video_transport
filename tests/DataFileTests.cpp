#include <gtest/gtest.h>
#include "DataFile.hpp"
#include "DataUnitConverter.hpp"

#include <filesystem>
#include <fstream>

class DataFileTest : public ::testing::Test {
protected:
  void SetUp() override {
    testFileName_ = "test_data_file.bin";
    createTestFile();
    converter_ = std::make_unique<DataUnitConverter>();
  }

  void TearDown() override {
    if (std::filesystem::exists(testFileName_)) {
      std::filesystem::remove(testFileName_);
    }
  }

  void createTestFile() {
    std::ofstream file(testFileName_, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    uint32_t length = 5;
    for (int i = 3; i >= 0; --i) {
      char byte = static_cast<char>((length >> (8 * i)) & 0xFF);
      file.put(byte);
    }

    file.write("Hello", 5);
    file.close();
  }

  std::string testFileName_;
  std::unique_ptr<DataUnitConverter> converter_;
};

TEST_F(DataFileTest, WriteAndReadDataUnit) {
  std::string writeFileName = "write_test.bin";

  DataUnit unit;
  unit.length = 4;
  unit.data = {'T', 'e', 's', 't'};
  std::vector<char> binaryData = converter_->encodeDataUnit(unit);

  {
    DataFile writeFile(writeFileName, DataFile::Mode::Write);
    writeFile.writeBinaryData(binaryData);
  }

  DataFile readFile(writeFileName, DataFile::Mode::Read);
  auto readBinaryData = readFile.readNextDataUnit();

  EXPECT_TRUE(readBinaryData.has_value());
  EXPECT_EQ(readBinaryData->size(), binaryData.size());

  auto readDataUnit = converter_->decodeDataUnit(readBinaryData.value());

  EXPECT_TRUE(readDataUnit.has_value());
  EXPECT_EQ(readDataUnit->length, 4);
  EXPECT_EQ(readDataUnit->data.size(), 4);
  EXPECT_EQ(std::string(readDataUnit->data.begin(), readDataUnit->data.end()),
            "Test");

  if (std::filesystem::exists(writeFileName)) {
    std::filesystem::remove(writeFileName);
  }
}

TEST_F(DataFileTest, ReadAllDataUnitsFromTestBin) {
  std::string testBinPath = "../../resources/front_0.bin";

  ASSERT_TRUE(std::filesystem::exists(testBinPath))
      << "front_0.bin file not found at " << testBinPath;

  DataFile dataFile(testBinPath, DataFile::Mode::Read);

  std::vector<DataUnit> dataUnits;
  size_t totalBytesRead = 0;

  while (auto binaryData = dataFile.readNextDataUnit()) {
    auto dataUnit = converter_->decodeDataUnit(binaryData.value());

    if (dataUnit.has_value()) {
      dataUnits.push_back(*dataUnit);
      totalBytesRead += dataUnit->length;

      std::cout << "Read data unit " + std::to_string(dataUnits.size()) +
                       " with length " + std::to_string(dataUnit->length) +
                       " bytes"
                << std::endl;
    }
  }

  EXPECT_EQ(dataUnits.size(), 10)
      << "Expected 10 data units, but found " << dataUnits.size();

  for (size_t i = 0; i < dataUnits.size(); ++i) {
    EXPECT_EQ(dataUnits[i].length, dataUnits[i].data.size())
        << "Data unit " << i
        << " length mismatch: declared=" << dataUnits[i].length
        << ", actual=" << dataUnits[i].data.size();

    EXPECT_GE(dataUnits[i].length, 0)
        << "Data unit " << i << " has negative length";
  }

  std::cout << "Successfully read " + std::to_string(dataUnits.size()) +
                   " data units with total " + std::to_string(totalBytesRead) +
                   " bytes of data"
            << std::endl;
}
