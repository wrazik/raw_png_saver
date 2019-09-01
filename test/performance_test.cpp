/* Tests are designed only to work on linux/unix. It's because tests are checking, if testing
 * directory is mounted with RAM filesystem." */

#include "fs_utils.h"

#include <filesystem>
#include <stdexcept>
#include <string>

int main() {
  using namespace std::string_literals;
  std::string const test_path = "/tmp/data";
  fs_utils::DirHandler const test_dir{std::filesystem::path(test_path)};
  if (!test_dir.IsMountedOnRam()) {
    throw std::runtime_error("Test directory: "s + test_path);
  }
}