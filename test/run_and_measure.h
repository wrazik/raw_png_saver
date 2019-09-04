#pragma once
#include <functional>
#include <chrono>

template <typename F, typename... Args>
auto run_and_measure(F&& func, Args&&... args) {
  auto start = std::chrono::steady_clock::now();
  std::invoke(std::forward<decltype(func)>(func), std::forward<Args>(args)...);
  return std::chrono::steady_clock::now() - start;
}
