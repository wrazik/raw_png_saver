#include "fake_data.h"

#include <algorithm>
#include <random>

namespace fake_data {

std::byte RandomByte() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, 3);
  std::array rgb = {std::byte(0), std::byte(100), std::byte(255)};

  return rgb[dist(rng)];
}

std::vector<std::byte> RawPng(uint32_t width, uint32_t height) {
  uint64_t const size = width * height * 4;
  std::vector<std::byte> pixels;
  pixels.reserve(size);
  std::generate_n(std::back_inserter(pixels), size, RandomByte);
  return pixels;
}

}  // namespace fake_data
