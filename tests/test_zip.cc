// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "zip_inl.hh"

#include <zlib.h>

#include "testutils_inl.hh"

#include "gtest/gtest.h"
#include "testutils_inl.hh"

using namespace zipprof;
using namespace zipprof::impl;

TEST(zip, array_bit_reader) {
  uint8_t elms[] = {213, 154, 0, 255, 0, 255, 0, 255};
  {
    ArrayBitReader reader(array<uint8_t>(elms, 8));
    constexpr uint32_t kBitCount = 64;
    uint8_t bits[kBitCount] = {
        1, 0, 1, 0, 1, 0, 1, 1,
        0, 1, 0, 1, 1, 0, 0, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1
    };
    for (uint32_t i = 0; i < kBitCount; i++) {
      EXPECT_EQ(i % 8, reader.bit_cursor());
      EXPECT_EQ(bits[i], reader.next_bit());
    }
  }

  {
    ArrayBitReader reader(array<uint8_t>(elms, 4));
    constexpr uint32_t kBitCount = 64;
    uint8_t bits[kBitCount] = {
        1, 0, 1, 0, 1, 0, 1, 1,
        0, 1, 0, 1, 1, 0, 0, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    for (uint32_t i = 0; i < kBitCount; i++) {
      EXPECT_EQ(i % 8, reader.bit_cursor());
      EXPECT_EQ(bits[i], reader.next_bit());
    }
  }

  {
    ArrayBitReader reader(array<uint8_t>(elms, 8));
    constexpr uint32_t kBitCount = 24;
    uint8_t bits[kBitCount] = {
        1, 1, 1, 3,
        2, 2, 1, 2,
        0, 0, 0, 0,
        3, 3, 3, 3,
        0, 0, 0, 0,
        3, 3, 3, 3,
    };
    for (uint32_t i = 0; i < kBitCount; i++) {
      EXPECT_EQ((i * 2) % 8, reader.bit_cursor());
      EXPECT_EQ(bits[i], reader.next_word(2));
    }
  }

}

class ZLib {
public:
  static array<uint8_t> deflate(array<const char> input, array<uint8_t> output,
      uint32_t compression=Z_DEFAULT_COMPRESSION);
  static array<char> inflate(array<uint8_t> data, array<char> output);
};

array<uint8_t> ZLib::deflate(array<const char> input, array<uint8_t> output,
    uint32_t compression) {
  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = input.size() + 1;
  stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.begin()));
  stream.avail_out = output.size();
  stream.next_out = output.begin();
  ::deflateInit(&stream, compression);
  if (::deflate(&stream, Z_FINISH) != Z_STREAM_END)
    return array<uint8_t>(NULL, 0);
  ::deflateEnd(&stream);
  return array<uint8_t>(output.begin(), stream.total_out);
}

array<char> ZLib::inflate(array<uint8_t> input, array<char> output) {
  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = input.size();
  stream.next_in = reinterpret_cast<Bytef*>(input.begin());
  stream.avail_out = output.size();
  stream.next_out = reinterpret_cast<Bytef*>(output.begin());
  ::inflateInit(&stream);
  if (::inflate(&stream, Z_FINISH) != Z_STREAM_END)
    return array<char>(NULL, 0);
  ::inflateEnd(&stream);
  return array<char>(output.begin(), stream.total_out);
}

static void test_transflate(const char *str, uint32_t compression=Z_DEFAULT_COMPRESSION) {
  // First compress and decompress with zlib.
  array<const char> input(c_str_to_array(str));
  stack_array<uint8_t, 65536> def_buf;
  array<uint8_t> comped = ZLib::deflate(input, def_buf, compression);
  EXPECT_EQ(0x78, comped[0]);
  stack_array<char, 65536> inf_buf;
  array<char> decomped = ZLib::inflate(comped, inf_buf);
  EXPECT_STREQ(decomped.begin(), str);

  // Then decompress with the deflater.
  std::vector<uint8_t> vector;
  ArrayBitReader reader(comped.slice(2));
  VectorByteWriter writer(vector);
  Deflater<ArrayBitReader, VectorByteWriter> deflater(reader, writer);
  deflater.deflate();
  EXPECT_STREQ(reinterpret_cast<char*>(vector.data()), str);
}

static const char *kLipsum =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "In eu sem lacus. Nullam non dapibus dolor. Aenean nec dui aliquet, "
        "blandit justo eu, bibendum dolor. Etiam ornare mauris purus, id "
        "lobortis sapien auctor sagittis. Pellentesque porttitor massa justo, "
        "eu eleifend lorem vehicula sit amet. Curabitur eu luctus metus. "
        "Phasellus lacinia, purus non convallis vestibulum, felis arcu mollis "
        "libero, nec feugiat mi lectus at lacus. Vivamus imperdiet orci turpis, "
        "sed consequat augue rutrum vel. Nunc libero tellus, lobortis eu nunc "
        "et, mollis suscipit augue. Nunc malesuada ornare urna vel commodo. "
        "Vestibulum lobortis odio vel ante mattis molestie. Vestibulum tempor "
        "sit amet sapien eget tempus. Interdum et malesuada fames ac ante "
        "ipsum primis in faucibus. Vivamus imperdiet, quam ut dignissim "
        "euismod, mauris sapien malesuada ex, vel eleifend nunc risus quis "
        "ligula. Donec vulputate felis sem, in volutpat ligula facilisis eget.";

TEST(zip, simple) {
  test_transflate("FoFooFooo", Z_NO_COMPRESSION);
  test_transflate("FoFooFooo", Z_BEST_COMPRESSION);
  test_transflate(kLipsum);
}

TEST(zip, archive) {
  std::string zip_str = read_file("../tests/data/lipsums.zip");
  Archive::Impl *arc = Archive::Impl::open(array<const uint8_t>(reinterpret_cast<const uint8_t*>(zip_str.c_str()), zip_str.size()));
  std::vector<std::string> paths;
  arc->add_paths(&paths);
  EXPECT_EQ(4, paths.size());
  EXPECT_STREQ("lipsums/1.txt", paths[0].c_str());
  EXPECT_STREQ("lipsums/2.txt", paths[1].c_str());
  EXPECT_STREQ("lipsums/3.txt", paths[2].c_str());
  EXPECT_STREQ("lipsums/4.txt", paths[3].c_str());
  EXPECT_EQ(1221, arc->stream("lipsums/1.txt").size());
  EXPECT_EQ(1128, arc->stream("lipsums/2.txt").size());
  EXPECT_EQ(1093, arc->stream("lipsums/3.txt").size());
  EXPECT_EQ(1174, arc->stream("lipsums/4.txt").size());
}
