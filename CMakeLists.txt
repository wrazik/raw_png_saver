cmake_minimum_required(VERSION 3.15)
project(raw_png_saver)

set(CMAKE_CXX_STANDARD 17)

add_executable(raw_png_saver main.cpp raw_png.hpp)
add_subdirectory(test)