#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <string_view>

void svpng(std::ofstream& file, unsigned w, unsigned h, const unsigned char* img, int alpha);

namespace raw_png {
void save(std::string_view filename, unsigned w, unsigned h, const unsigned char* img, int alpha) {
  std::ofstream file(std::filesystem::path(filename), std::ios_base::binary | std::ios_base::out);
  svpng(file, w, h, img, alpha);
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

void svpng(std::ofstream& file, unsigned w, unsigned h, const unsigned char* img, int alpha) {
  constexpr std::array<uint32_t, 16> crc_table{{0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190,
                                                0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320,
                                                0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0,
                                                0x86d3d2d4, 0xa00ae278, 0xbdbdf21c}};

  unsigned a = 1, b = 0, c, p = w * (alpha ? 4 : 3) + 1, x, y, i; /* ADLER-a, ADLER-b, CRC, pitch */
#define SVPNG_U32(u)             \
  do {                           \
    file.put((u) >> 24);         \
    file.put(((u) >> 16) & 255); \
    file.put(((u) >> 8) & 255);  \
    file.put((u)&255);           \
  } while (0)
#define SVPNG_U8C(u)                  \
  do {                                \
    file.put(u);                      \
    c ^= (u);                         \
    c = (c >> 4) ^ crc_table[c & 15]; \
    c = (c >> 4) ^ crc_table[c & 15]; \
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
#define SVPNG_BEGIN(s, l) \
  do {                    \
    SVPNG_U32(l);         \
    c = ~0U;              \
    SVPNG_U8AC(s, 4);     \
  } while (0)
#define SVPNG_END() SVPNG_U32(~c)

  file << detail::header;

  SVPNG_BEGIN("IHDR", 13);       /* IHDR chunk { */
  SVPNG_U32C(w);
  SVPNG_U32C(h); /*   Width & Height (8 bytes) */
  SVPNG_U8C(8);
  SVPNG_U8C(alpha ? 6 : 2); /*   Depth=8, Color=True color with/without alpha (2 bytes) */
  SVPNG_U8AC("\0\0\0", 3);  /*   Compression=Deflate, Filter=No, Interlace=No (3 bytes) */
  SVPNG_END();              /* } */
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
}
