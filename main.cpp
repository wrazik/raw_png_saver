#include <iostream>
#include <fstream>
#include <vector>

#include "raw_png.hpp"

int main() {
  std::cout << "Hello, World!" << std::endl;
  std::ifstream file("../data.txt", std::ios_base::binary | std::ios_base::in);
  uint32_t width = 1280;
  uint32_t height = 720;
  std::vector<unsigned char> content;
  content.reserve(width * height* 4);
  for (uint64_t i = 0; i < width*height*4 - 1; ++i) {
    int c;
    file >> c;
    content.push_back(static_cast<unsigned char>(c));
  }

  raw_png::save("../dupa.png", width, height, content.data(), 0);

  return 0;
}