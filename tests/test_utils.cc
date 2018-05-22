// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "gtest/gtest.h"
#include "utils_inl.hh"

using namespace zipprof;
using namespace zipprof::impl;

TEST(utils, array) {
  uint32_t elms[] = {1, 2, 3, 4};
  array<uint32_t> arr(elms, 4);
  EXPECT_EQ(4, arr.size());
  EXPECT_EQ(1, arr[0]);
}
