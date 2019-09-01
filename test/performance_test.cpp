/* Tests are designed only to work on linux/unix. It's because tests are checking, if testing
 * directory is mounted with RAM filesystem." */

#include "../raw_png.hpp"
#include "fake_data.h"
#include "fs_utils.h"

#include <filesystem>
#include <stdexcept>
#include <string>

using std::string_literals::operator""s;

int main() {
  std::string const test_path = "/tmp/raw_png_saver_test_data";
  fs_utils::DirHandler const test_dir{std::filesystem::path(test_path)};
  if (!test_dir.IsMountedOnRam()) {
    throw std::runtime_error("Test directory: "s + test_path);
  }

  constexpr uint32_t width = 800;
  constexpr uint32_t height = 600;

  std::vector<std::byte> fake_png = fake_data::RawPng(width, height);

  raw_png::save(test_path + "/test.png", width, height,
                reinterpret_cast<unsigned char*>(fake_png.data()), 0);
}