#pragma once
#include <cstdint>
#include <vector>

namespace fake_data {

/* Generate random image with specific size. It will create 4 values per each pixel*/
std::vector<std::byte> RawPng(uint32_t width, uint32_t height);

}