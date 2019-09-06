#pragma once
#include <array>
#include <cstring>
#include <fstream>
#include <vector>

#include <iostream>
#include "errno.h"
#include "test/svpng.inc"

void svpng_int(std::fstream& file, unsigned w, unsigned h, unsigned char* img, int alpha);

namespace raw_png {
void save(std::string const& filename, unsigned w, unsigned h, unsigned char* img,
          int alpha) {
  std::fstream file(filename.c_str(), std::ios_base::binary | std::ios_base::out);
  svpng_int(file, w, h, img, alpha);
}
}  // namespace raw_png

namespace detail {

template <typename Bits>
Bits last_four_bits(Bits number) {
  static_assert(sizeof(Bits) >= 4);
  return number & 0b1111;
}

constexpr const char header[] = "\x89PNG\r\n\32\n";
}  // namespace detail

template <typename T>
unsigned char last_byte(T val) {
  return val & 0xFF;
}

std::array<unsigned char, 4> convert_to_4byte_arr(uint32_t val) {
  return {last_byte(val >> 24), last_byte(val >> 16), last_byte(val >> 8), last_byte(val)};
}

std::array<unsigned char, 2> convert_to_2byte_arr(uint16_t val) {
  return {last_byte(val), last_byte(val >> 8)};
}

void append_4_bytes(std::vector<unsigned char>& vec, uint32_t val) {
  auto const to_insert = convert_to_4byte_arr(val);
  vec.insert(vec.end(), to_insert.begin(), to_insert.end());
}

void append_2_bytes(std::vector<unsigned char>& vec, uint16_t val) {
  auto const to_insert = convert_to_2byte_arr(val);
  vec.insert(vec.end(), to_insert.begin(), to_insert.end());
}

template <typename Iterable>
void append(std::vector<unsigned char>& vec, Iterable const& it) {
  for (auto val : it) {
    vec.push_back(val);
  }
}

template <typename ForwardIt>
uint32_t calculate_crc(ForwardIt first, ForwardIt last) {
  constexpr std::array<uint32_t, 16> kCrc{0x0,        0x1db71064, 0x3b6e20c8, 0x26d930ac,
                                          0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                                          0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                          0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  uint32_t crc = ~0U;
  while (first != last) {
    crc ^= (*first++);
    crc = (crc >> 4) ^ kCrc[crc & 15];
    crc = (crc >> 4) ^ kCrc[crc & 15];
  }
  return ~crc;
}

uint32_t calculate_crc(std::vector<unsigned char> const& values) {
  const auto begin_after_size = values.begin() + 4;
  return calculate_crc(begin_after_size, values.end());
}


uint32_t calculate_adler(std::vector<unsigned char> const& values) {
  uint32_t a = 1;
  uint32_t b = 0;
  for (auto const c: values) {
    a = (a + (c)) % 65521;
    b = (b + a) % 65521;
  }
  return (b << 16) | a;
}

std::vector<unsigned char> create_addler(unsigned char * first, uint32_t size) {
  unsigned char const kNoFilter = 0 ;
  std::vector<unsigned char> addler;
  append_4_bytes(addler, kNoFilter);
  for (int i = 0; i < size; ++i, first++) {
    addler.push_back(*first);
  }
  append_4_bytes(addler, calculate_adler(addler));
  return addler;
}

std::vector<unsigned char> create_data_chunk(uint32_t w, uint32_t h, int alpha,
                                             unsigned char * data) {
  uint32_t const kPixelWidth = alpha ? 4 : 3;
  uint32_t const kRowWidth = w * kPixelWidth + 1;

  std::vector<unsigned char> idat_chunk;
  uint32_t kChunkSize = 2 + h * (5 + kRowWidth) + 4;

  append_4_bytes(idat_chunk, kChunkSize);

  idat_chunk.push_back('I');
  idat_chunk.push_back('D');
  idat_chunk.push_back('A');
  idat_chunk.push_back('T');
  idat_chunk.push_back('\x78');
  idat_chunk.push_back('\1'); //deflate block
  uint32_t a = 1;
  uint32_t b = 0;
  std::vector<unsigned char> for_addler;
  for (int i = 0; i < h; ++i) {
    if (i != h - 1)
      idat_chunk.push_back(0);
    else
      idat_chunk.push_back(1);
    append_2_bytes(idat_chunk, kRowWidth);
    append_2_bytes(idat_chunk, ~kRowWidth);
    unsigned char const kNoFilter = 0 ;
    for_addler.push_back(kNoFilter);
    idat_chunk.push_back(kNoFilter);
    for (int i = 0; i < kRowWidth - 1; ++i, data++) {
      for_addler.push_back(*data);
      idat_chunk.push_back(*data);
    }
  }
  append_4_bytes(idat_chunk, calculate_adler(for_addler));
  append_4_bytes(idat_chunk, calculate_crc(idat_chunk));
  return idat_chunk;
}

std::vector<unsigned char> create_ihdr_chunk(uint32_t w, uint32_t h, int alpha) {
  std::vector<unsigned char> hdr_chunk;
  uint32_t c = ~0U;
  unsigned char kDpeth = 8;
  unsigned char kChunkSize = 13;
  hdr_chunk.reserve(kChunkSize * 2);
  append_4_bytes(hdr_chunk, kChunkSize);

  c = ~0U;
  hdr_chunk.push_back('I');
  hdr_chunk.push_back('H');
  hdr_chunk.push_back('D');
  hdr_chunk.push_back('R');
  append_4_bytes(hdr_chunk, w);
  append_4_bytes(hdr_chunk, h);
  hdr_chunk.push_back(kDpeth);
  if (alpha == 1)
    hdr_chunk.push_back(6);
  else
    hdr_chunk.push_back(2);
  hdr_chunk.push_back('\0');
  hdr_chunk.push_back('\0');
  hdr_chunk.push_back('\0');

  uint32_t const crc = calculate_crc(hdr_chunk);
  append_4_bytes(hdr_chunk, crc);
  return hdr_chunk;
}

void svpng_int(std::fstream& fp, unsigned w, unsigned h, unsigned char* img, int alpha) {
  std::vector<unsigned char> png_data;
  png_data.reserve(4.5 * w * h);
  fp.write("\x89PNG\r\n\32\n", 8); /* Magic */

  std::vector<unsigned char> hdr_chunk = create_ihdr_chunk(w, h, alpha);
  std::copy(hdr_chunk.begin(), hdr_chunk.end(), std::back_inserter(png_data));

  std::vector<unsigned char> idat_chunk = create_data_chunk(w, h, alpha, img);
  std::copy(idat_chunk.begin(), idat_chunk.end(), std::back_inserter(png_data));
  std::vector<unsigned char> end;
  append_4_bytes(end, 0);
  end.push_back('I');
  end.push_back('E');
  end.push_back('N');
  end.push_back('D');
  uint32_t const crc = calculate_crc(end);
  append_4_bytes(end, crc);
  std::copy(end.begin(), end.end(), std::back_inserter(png_data));

  fp.write(reinterpret_cast<char*>(png_data.data()), png_data.size());
}

