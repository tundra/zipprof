// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "io_inl.hh"

#include "utils_inl.hh"
#include "zipprof_impl.hh"

using namespace zipprof;
using namespace zipprof::impl;

ArrayBitReader::ArrayBitReader(array<const uint8_t> data)
  : data_(data)
  , data_cursor_(static_cast<uint32_t>(-8))
  , buffer_cursor_(64)
  , buffer_(0) {
}

uint8_t ArrayBitReader::bit_cursor() {
  return buffer_cursor_ & 0x7;
}

void ArrayBitReader::flush_current_byte() {
  while (bit_cursor() != 0)
    next_bit();
}

void ArrayBitReader::fetch_next_block() {
  uint64_t new_buffer;
  data_cursor_ += 8;
  uint32_t cursor = data_cursor_;
  if ((cursor + 8) <= data_.size()) {
    new_buffer = reinterpret_cast<const uint64_t*>(data_.begin() + cursor)[0];
  } else {
    new_buffer = 0;
    for (uint32_t i = 0; i < 8; i++, cursor++) {
      uint64_t byte = (cursor < data_.size()) ? data_[cursor] : 0;
      new_buffer |= byte << (i << 3);
    }
  }
  buffer_ = new_buffer;
  buffer_cursor_ = 0;
}

uint32_t ArrayBitReader::next_word_fallback(uint32_t word_size) {
  uint32_t result = 0;
  for (uint32_t i = 0; i < word_size; i++)
    result = result | (next_bit() << i);
  return result;
}

uint8_t ArrayBitReader::ensure_aligned() {
  if (bit_cursor() != 0) {
    uint8_t result = 7 - bit_cursor();
    flush_current_byte();
    return result;
  } else {
    return 0;
  }
}

VectorByteWriter::VectorByteWriter(std::vector<uint8_t> &buf)
    : buf_(buf) { }

void VectorByteWriter::copy(uint8_t data, uint32_t source, int32_t copy, uint32_t size_bits) {
  buf_.push_back(data);
}

void VectorByteWriter::append(uint8_t data, uint32_t size_bits) {
  buf_.push_back(data);
}

ProfilingByteWriter::ProfilingByteWriter()
    : bytes_(new ByteStat[1024 * 1024], 1024 * 1024)
    , cursor_(0)
    , literal_count_(0) {
}

ProfilingByteWriter::~ProfilingByteWriter() {
}

void ProfilingByteWriter::grow_buffer() {
  array<ByteStat> old_stats = bytes_;
  uint32_t old_size = old_stats.size();
  uint32_t new_size = old_size * 2;
  array<ByteStat> new_stats(new ByteStat[new_size], new_size);
  memcpy(new_stats.begin(), old_stats.begin(), old_size * sizeof(ByteStat));
  bytes_ = new_stats;
}

void ProfilingByteWriter::open_block(uint8_t type) {
  BlockStat stat = {type};
  blocks_.push_back(stat);
}

void ProfilingByteWriter::close_block(uint32_t bit_count) {
}

DeflateProfile::Impl *ProfilingByteWriter::flush(uint32_t deflated_size) {
  uint32_t inflated_size = cursor_;
  array<ByteStat> bytes(new ByteStat[inflated_size], inflated_size);
  memcpy(bytes.begin(), bytes_.begin(), inflated_size * sizeof(ByteStat));
  uint32_t block_count = blocks_.size();
  array<BlockStat> blocks(new BlockStat[block_count], block_count);
  memcpy(blocks.begin(), blocks_.data(), block_count * sizeof(BlockStat));
  return new DeflateProfile::Impl(deflated_size, inflated_size, literal_count_,
      bytes, blocks);
}
