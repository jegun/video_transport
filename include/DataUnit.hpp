#pragma once
#include <vector>
#include <cstdint>

struct DataUnit {
  uint32_t length = 0;
  std::vector<char> data;
};