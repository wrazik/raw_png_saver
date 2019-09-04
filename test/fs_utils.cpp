#include "fs_utils.h"

#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace fs_utils {
using std::string_literals::operator""s;

DirHandler::DirHandler(const std::filesystem::path& path)
    : path_(path), is_new_dir_(!std::filesystem::exists(path_)) {
  if (is_new_dir_) {
    std::filesystem::create_directories(path_);
  }
}
DirHandler::~DirHandler() {
  if (is_new_dir_) {
    std::filesystem::remove_all(path_);
  }
}

bool DirHandler::IsMountedOnRam() const {
  const auto parition_type_cmd = "df -P "s + path_.string() + "| tail -1 | cut -d' ' -f 1"s;
  const std::string partition_type = SystemCall::Run(parition_type_cmd.c_str());
  return partition_type == "ramfs\n" || partition_type == "tmpfs\n";
}

std::string SystemCall::Run(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed! command: "s + cmd);
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

}  // namespace fs_utils
