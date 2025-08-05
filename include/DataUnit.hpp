#pragma once
#include <vector>

struct DataUnit {
  uint32_t length = 0;
  std::vector<char> data;
};