#include <gtest/gtest.h>
#include "DataUnit.hpp"

class DataUnitTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DataUnitTest, DefaultConstruction) {
  DataUnit unit;

  EXPECT_EQ(unit.length, 0);
  EXPECT_TRUE(unit.data.empty());
}
