// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include "utils.hh"
#include "zip.hh"
#include "zipprof.h"

namespace zipprof {

class DeflateProfile::Impl {
public:
  Impl(uint32_t deflated_size, uint32_t inflated_size, uint32_t literal_count,
      impl::array<impl::ByteStat> byte_stats, impl::array<impl::BlockStat> block_stats);
  ~Impl();

  impl::array<uint32_t> origins();
  impl::array<uint32_t> literal_weights();

  uint32_t deflated_size_;
  uint32_t inflated_size_;
  uint32_t literal_count_;
  impl::array<impl::ByteStat> byte_stats_;
  impl::array<impl::BlockStat> block_stats_;
  impl::array<uint32_t> origins_;
  impl::array<uint32_t> literal_weights_;
};

} // namespace zipprof
