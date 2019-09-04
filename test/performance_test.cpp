/* Tests are designed only to work on linux/unix. It's because tests are checking, if testing
 * directory is mounted with RAM filesystem." */

#include "../raw_png.hpp"
#include "fake_data.h"
#include "fs_utils.h"
#include "run_and_measure.h"
#include "svpng.inc"
#include <bitset>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using std::string_literals::operator""s;
std::string const test_path = "/tmp/raw_png_saver_test_data";

void run_reference_lib(std::string const& filename, unsigned w, unsigned h,
                       const unsigned char* img, int alpha) {
  FILE* output = fopen(filename.c_str(), "wb");
  svpng(output, w, h, img, alpha);

  fclose(output);
}

int main() {
  fs_utils::DirHandler const test_dir{std::filesystem::path(test_path)};
  if (!test_dir.IsMountedOnRam()) {
    throw std::runtime_error("Test directory: "s + test_path);
  }

  constexpr uint32_t width = 800;
  constexpr uint32_t height = 600;

  std::vector<std::byte> const fake_png = fake_data::RawPng(width, height);
  auto const data_ptr = reinterpret_cast<unsigned char const* const>(fake_png.data());

  auto test = 0b1001'0101'1001'0100'1001'0101'0010'0111;
  std::cout << "test " << std::bitset<32>(test) << "\n";
  std::cout << "test >> 24" << std::bitset<32>(test >> 24) << "\n";
  std::cout << "test >> 16 & ff" << std::bitset<32>((test >> 16) & 255) << "\n";
  std::cout << "test >> 8 & ff" << std::bitset<32>((test >> 8) & 255) << "\n";
  std::cout << "test & ff" << std::bitset<32>(test & 255) << "\n";

  auto duration =
      run_and_measure(raw_png::save, test_path + "/test.png", width, height, data_ptr, 0);
  std::cout << "my: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
            << "ms\n";

  auto duration2 =
      run_and_measure(run_reference_lib, test_path + "/test2.png", width, height, data_ptr, 0);
  std::cout << "svnlib: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(duration2).count() << "ms\n";
}