#pragma once
#include <array>
#include <cstring>
#include <fstream>
#include <vector>

#include <iostream>
#include "errno.h"
#include "test/svpng.inc"

void svpng_int(std::fstream& file, unsigned w, unsigned h, const unsigned char* img, int alpha);

namespace raw_png {
void save(std::string const& filename, unsigned w, unsigned h, const unsigned char* img,
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

void append_4_bytes(std::vector<unsigned char>& vec, uint32_t val) {
  auto const to_insert = convert_to_4byte_arr(val);
  vec.insert(vec.end(), to_insert.begin(), to_insert.end());
}

template <typename Iterable>
void append(std::vector<unsigned char>& vec, Iterable const& it) {
  for (auto val : it) {
    vec.push_back(val);
  }
}

uint32_t calculate_crc(std::vector<unsigned char> const& values) {
  constexpr std::array<uint32_t, 16> kCrc{0x0,        0x1db71064, 0x3b6e20c8, 0x26d930ac,
                                          0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                                          0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                          0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  uint32_t crc = ~0U;
  for (unsigned char const c : values) {
    crc ^= (c);
    crc = (crc >> 4) ^ kCrc[crc & 15];
    crc = (crc >> 4) ^ kCrc[crc & 15];
  }
  return ~crc;
}

std::vector<unsigned char> create_ihdr_chunk(uint32_t w, uint32_t h, int alpha) {
  std::vector<unsigned char> hdr_chunk;
  uint32_t c = ~0U;
  unsigned char kDpeth = 8;

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

void svpng_int(std::fstream& fp, unsigned w, unsigned h, const unsigned char* img, int alpha) {
  std::vector<unsigned char> png_data;
  png_data.reserve(4.5 * w * h);
  constexpr std::array<uint32_t, 16> kCrc{0x0,        0x1db71064, 0x3b6e20c8, 0x26d930ac,
                                          0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                                          0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                          0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  unsigned a = 1;
  unsigned b = 0;
  unsigned c;
  unsigned p = w * (alpha ? 4 : 3) + 1;
  unsigned x, y;
  unsigned i;

#define SVPNG_U32(u)             \
  do {                           \
    append_4_bytes(png_data, u); \
  } while (0)

#define SVPNG_U8C(u)             \
  do {                           \
    png_data.push_back(u);       \
    c ^= (u);                    \
    c = (c >> 4) ^ kCrc[c & 15]; \
    c = (c >> 4) ^ kCrc[c & 15]; \
  } while (0)
#define SVPNG_U8AC(ua, l) \
  for (i = 0; i < l; i++) SVPNG_U8C((ua)[i])
#define SVPNG_U16LC(u)           \
  do {                           \
    SVPNG_U8C((u)&255);          \
    SVPNG_U8C(((u) >> 8) & 255); \
  } while (0)

#define SVPNG_U32C(u)             \
  do {                            \
    SVPNG_U8C((u) >> 24);         \
    SVPNG_U8C(((u) >> 16) & 255); \
    SVPNG_U8C(((u) >> 8) & 255);  \
    SVPNG_U8C((u)&255);           \
  } while (0)

#define SVPNG_U8ADLER(u)   \
  do {                     \
    SVPNG_U8C(u);          \
    a = (a + (u)) % 65521; \
    b = (b + a) % 65521;   \
  } while (0)
#define SVPNG_BEGIN(s, l)        \
  do {                           \
    append_4_bytes(png_data, l); \
    c = ~0U;                     \
    SVPNG_U8AC(s, 4);            \
  } while (0)

#define SVPNG_END() append_4_bytes(png_data, ~c)
  fp.write("\x89PNG\r\n\32\n", 8); /* Magic */

  append_4_bytes(png_data, 13);
  append(png_data, create_ihdr_chunk(w, h, alpha));

  SVPNG_BEGIN("IDAT", 2 + h * (5 + p) + 4); /* IDAT chunk { */
  SVPNG_U8AC("\x78\1", 2);                  /*   Deflate block begin (2 bytes) */
  for (y = 0; y < h; y++) { /*   Each horizontal line makes a block for simplicity */
    SVPNG_U8C(y == h - 1);  /*   1 for the last block, 0 for others (1 byte) */
    SVPNG_U16LC(p);
    SVPNG_U16LC(~p);  /*   Size of block in little endian and its 1's complement (4 bytes) */
    SVPNG_U8ADLER(0); /*   No filter prefix (1 byte) */
    for (x = 0; x < p - 1; x++, img++) SVPNG_U8ADLER(*img); /*   Image pixel data */
  }
  SVPNG_U32C((b << 16) | a); /*   Deflate block end with adler (4 bytes) */
  SVPNG_END();               /* } */
  SVPNG_BEGIN("IEND", 0);
  SVPNG_END(); /* IEND chunk {} */

  fp.write(reinterpret_cast<char*>(png_data.data()), png_data.size());
}

#undef SVPNG_OUTPUT
#undef SVPNG_U8A
#undef SVPNG_U32
#undef SVPNG_U8C
#undef SVPNG_U8AC
#undef SVPNG_U16LC
#undef SVPNG_U32C
#undef SVPNG_U8ADLER
#undef SVPNG_BEGIN
#undef SVPNG_END
