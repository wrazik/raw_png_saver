#pragma once
#include <array>
#include <cstring>
#include <fstream>
#include <type_traits>
#include <vector>

#include <iostream>
#include "errno.h"
#include "test/svpng.inc"

namespace raw_png {
void save(std::string const& filename, unsigned w, unsigned h, unsigned char* img, int alpha);
}

namespace detail {

class DataChunk {
public:
  explicit DataChunk(size_t start_size) { bytes.reserve(start_size); }

  void add(std::string_view str) {
    for (auto c : str) {
      bytes.push_back(c);
    }
  }

  template <typename... Chars>
  void add_chars(Chars&&... chars) {
    (bytes.push_back(chars), ...);
  }

  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  void add(T val) = delete;

  void add(std::byte val) { bytes.push_back(static_cast<unsigned char>(val)); }
  void add(uint16_t val) { add_chars(val, val >> 8); }
  void add(uint32_t val) { add_chars(val >> 24, val >> 16, val >> 8, val); }
  void add_crc() {
    auto const kIgnoredBytesCount = 4;
    add(calculate_crc(bytes.begin() + kIgnoredBytesCount, bytes.end()));
  }

  void add_as_4_bytes(std::byte byte) {
    add_chars(0x00, 0x00, 0x00);
    bytes.push_back(static_cast<unsigned char>(byte));
  }

  void add_addler(uint32_t width, uint32_t height, int alpha) {
    auto const kIgnoredAtTheBegin = 10;
    auto const kIgnoredEachRow = 5;
    auto const kPixelSize = alpha ? 4 : 3;
    auto const kRowWidth = (width * kPixelSize);
    auto const kFilterAndWidth = kRowWidth + 1;

    auto it = bytes.begin() + kIgnoredAtTheBegin;
    uint32_t a = 1;
    uint32_t b = 0;
    for(int i = 0; i < height; ++i) {
      it += kIgnoredEachRow;
      for (int j = 0; j < kFilterAndWidth; ++j) {
        a = (a + (*it++)) % 65521;
        b = (b + a) % 65521;
      }
    }
    uint32_t addler = (b << 16) | a;
    add(addler);
  }

  std::vector<unsigned char>& data() { return bytes; }

private:
  std::vector<unsigned char> bytes;

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
};

constexpr const char header[] = "\x89PNG\r\n\32\n";

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
  for (auto const c : values) {
    a = (a + (c)) % 65521;
    b = (b + a) % 65521;
  }
  return (b << 16) | a;
}

std::vector<unsigned char> create_ihdr_chunk(uint32_t w, uint32_t h, int alpha) {
  std::byte const kDpeth{8};
  uint32_t const kChunkSize{13};
  DataChunk hdr_chunk(26);
  hdr_chunk.add(kChunkSize);

  hdr_chunk.add("IHDR");
  hdr_chunk.add(w);
  hdr_chunk.add(h);
  hdr_chunk.add(kDpeth);
  if (alpha == 1)
    hdr_chunk.add(std::byte(6));
  else
    hdr_chunk.add(std::byte(2));
  hdr_chunk.add_chars('\0', '\0', '\0');
  hdr_chunk.add_crc();

  return hdr_chunk.data();
}

std::vector<unsigned char> create_data_chunk(uint32_t w, uint32_t h, int alpha,
                                             unsigned char* data) {
  uint32_t const kPixelWidth = alpha ? 4 : 3;
  uint16_t const kRowWidth = w * kPixelWidth + 1;

  uint32_t kChunkSize = 2 + h * (5 + kRowWidth) + 4;

  DataChunk idat_chunk(kChunkSize * 2);
  idat_chunk.add(kChunkSize);    // 4
  idat_chunk.add("IDAT\x78\1");  // 6 / 10

  uint32_t a = 1;
  uint32_t b = 0;
  for (int i = 0; i < h; ++i) {
    if (i != h - 1)
      idat_chunk.add(std::byte(0));  // 1 / 11
    else
      idat_chunk.add(std::byte(1));
    idat_chunk.add(kRowWidth);                          // 2 / 13
    idat_chunk.add(static_cast<uint16_t>(~kRowWidth));  // 2 / 15

    unsigned char const kNoFilter = 0;
    idat_chunk.add(std::byte(kNoFilter));
    for (int i = 0; i < kRowWidth - 1; ++i, data++) {
      idat_chunk.add(std::byte(*data));
    }
  }

  idat_chunk.add_addler(w, h, alpha);
  idat_chunk.add_crc();
  return idat_chunk.data();
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

template <typename Bits>
Bits last_four_bits(Bits number) {
  static_assert(sizeof(Bits) >= 4);
  return number & 0b1111;
}

}  // namespace detail

namespace raw_png {
void save(std::string const& filename, unsigned w, unsigned h, unsigned char* img, int alpha) {
  std::fstream file(filename.c_str(), std::ios_base::binary | std::ios_base::out);
  detail::svpng_int(file, w, h, img, alpha);
}
}  // namespace raw_png
