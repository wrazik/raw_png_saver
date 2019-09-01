#pragma once
#include <filesystem>
#include <string>

namespace fs_utils {

/* RAII class for creating directory & clean up. If folder doesn't exist, it will create one and
 * remove on destruction. Otherwise, it will do nothing. */
class DirHandler {
public:
  explicit DirHandler(const std::filesystem::path&);
  ~DirHandler();
  bool IsMountedOnRam() const;

private:
  const std::filesystem::path& path_;
  const bool is_new_dir_;
};

/* Class for running system calls with popen and returning the output */
class SystemCall {
public:
  static std::string Run(const char* command);
};

}  // namespace fs_utils
