#include <gtest/gtest.h>
#include "DataUnitConverter.hpp"
#include "DataUnit.hpp"

class DataUnitConverterTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DataUnitConverterTest, DecodeHeaderValid) {
  DataUnit testUnit{12345, {'H', 'e', 'l', 'l', 'o'}};
  DataUnitConverter converter;

  std::vector<char> binaryData = converter.encodeDataUnit(testUnit);

  auto length = converter.decodeHeader(binaryData);

  EXPECT_TRUE(length.has_value());
  EXPECT_EQ(length.value(), 12345);
}

TEST_F(DataUnitConverterTest, DecodeHeaderInsufficientData) {
  std::vector<char> insufficientData{'A', 'B', 'C'};
  DataUnitConverter converter;

  auto length = converter.decodeHeader(insufficientData);

  EXPECT_FALSE(length.has_value());
}

TEST_F(DataUnitConverterTest, TwoBuffers) {
  DataUnitConverter converter;

  std::vector<char> firstBuffer = {0x00, 0x00, 0x00, 0x05, 'A', 'B'};

  std::vector<char> secondBuffer = {'C', 'D', 'E'};

  auto firstUnit = converter.decodeDataUnit(firstBuffer);
  EXPECT_FALSE(firstUnit.has_value());

  auto secondUnit = converter.decodeDataUnit(secondBuffer);
  EXPECT_TRUE(secondUnit.has_value());
  EXPECT_EQ(secondUnit->length, 5);
  EXPECT_EQ(secondUnit->data.size(), 5);
  EXPECT_EQ(secondUnit->data[0], 'A');
  EXPECT_EQ(secondUnit->data[1], 'B');
  EXPECT_EQ(secondUnit->data[2], 'C');
  EXPECT_EQ(secondUnit->data[3], 'D');
  EXPECT_EQ(secondUnit->data[4], 'E');
}
