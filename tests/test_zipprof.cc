// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "zip.hh"

#include "zipprof.h"
#include "gtest/gtest.h"

#include <zlib.h>
#include <fstream>
#include <sstream>
#include <chrono>

using namespace zipprof;

TEST(zipprof, no_compression) {
  DeflateProfile profile = Profiler::profile_string("FooBarBaz",
      Compressor::zlib_no_compression());
  EXPECT_EQ(10, profile.literal_count());
  EXPECT_EQ(10, profile.inflated_size());
}

static std::string read_file(std::string path) {
  std::stringstream bytes;
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::cerr << "Couldn't open file " << path << std::endl;
    return std::string();
  }
  bytes << file.rdbuf();
  file.close();
  return bytes.str();
}

DeflateProfile check_fixture(std::string name) {
  std::string root_path = "../tests/data/";
  std::string defl_str = read_file(root_path + name + ".z");
  std::string infl_str = read_file(root_path + name);

  Array<const uint8_t> defl(reinterpret_cast<const uint8_t*>(defl_str.c_str()),
      defl_str.size());
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
