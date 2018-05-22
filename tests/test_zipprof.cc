// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "zip.hh"

#include "zipprof.h"
#include "gtest/gtest.h"

#include <zlib.h>
#include <fstream>
#include <sstream>
#include <chrono>

#include "testutils_inl.hh"

using namespace zipprof;

TEST(zipprof, no_compression) {
  DeflateProfile profile = Profiler::profile_string("FooBarBaz",
      Compressor::zlib_no_compression());
  EXPECT_EQ(10, profile.literal_count());
  EXPECT_EQ(10, profile.inflated_size());
}

DeflateProfile check_fixture(std::string name) {
  std::string root_path = "../tests/data/";
  std::string defl_str = read_file(root_path + name + ".z");
  std::string infl_str = read_file(root_path + name);

  Array<const uint8_t> defl = string_to_data(defl_str);
  DeflateProfile profile = Profiler::profile_zlib(defl);
  EXPECT_EQ(defl.size(), profile.deflated_size());
  EXPECT_EQ(infl_str.size(), profile.inflated_size());

  double total_contribution = 0;
  for (uint32_t i = 0; i < profile.inflated_size(); i++)
    total_contribution += profile.literal_contribution(i);
  EXPECT_FLOAT_EQ(profile.literal_count(), total_contribution);

  return profile;
}

TEST(zipprof, lipsum) {
  DeflateProfile profile = check_fixture("lipsum.txt");
  EXPECT_EQ(552, profile.literal_count());
  EXPECT_EQ(1, profile.block_count());
}

TEST(zipprof, lipsum_big) {
  DeflateProfile profile = check_fixture("lipsum-big.txt");
  EXPECT_EQ(1842, profile.literal_count());
  EXPECT_EQ(1, profile.block_count());
}

TEST(zipprof, shakespeare) {
  DeflateProfile profile = check_fixture("shakespeare.txt");
  EXPECT_EQ(371081, profile.literal_count());
  EXPECT_EQ(68, profile.block_count());
}

TEST(zipprof, lipsums) {
  std::string str = read_file("../tests/data/lipsums.zip");
  Archive archive(string_to_data(str));
  EXPECT_EQ(4, archive.entries().size());

  DeflateProfile l1p = archive.profile("lipsums/1.txt");
  std::string l1 = data_to_string(l1p.contents());
  EXPECT_EQ(3041, l1.size());
  EXPECT_EQ(552, l1p.literal_count());
  EXPECT_STREQ("Fusce convallis tincidunt", l1.substr(0, 25).c_str());

  DeflateProfile l2p = archive.profile("lipsums/2.txt");
  std::string l2 = data_to_string(l2p.contents());
  EXPECT_EQ(2613, l2.size());
  EXPECT_EQ(571, l2p.literal_count());
  EXPECT_STREQ("Nullam non sem sit amet", l2.substr(0, 23).c_str());

  DeflateProfile l3p = archive.profile("lipsums/3.txt");
  std::string l3 = data_to_string(l3p.contents());
  EXPECT_EQ(2561, l3.size());
  EXPECT_EQ(562, l3p.literal_count());
  EXPECT_STREQ("Integer mattis lectus tellus", l3.substr(0, 28).c_str());

  DeflateProfile l4p = archive.profile("lipsums/4.txt");
  std::string l4 = data_to_string(l4p.contents());
  EXPECT_EQ(2813, l4.size());
  EXPECT_EQ(548, l4p.literal_count());
  EXPECT_STREQ("Ut non elit vitae lorem feugiat", l4.substr(0, 31).c_str());
}
