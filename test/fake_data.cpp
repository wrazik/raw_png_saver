#include "fake_data.h"

#include <algorithm>
#include <random>

namespace fake_data {

std::byte RandomByte() {
  return std::byte(255);
}

std::vector<std::byte> RawPng(uint32_t width, uint32_t height) {
  uint64_t const size = width * height * 4;
  std::vector<std::byte> pixels;
  pixels.reserve(size);
  std::generate_n(std::back_inserter(pixels), size, RandomByte);
  return pixels;
}

}  // namespace fake_data
