// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include "io.hh"

#include "utils_inl.hh"

namespace zipprof {
namespace impl {

uint8_t ArrayBitReader::next_bit() {
  if (buffer_cursor_ == 64)
    fetch_next_block();
  return (buffer_ >> (buffer_cursor_++)) & 0x1;
}

template <uint32_t W>
uint32_t ArrayBitReader::next_word() {
  if (buffer_cursor_ + W > 64)
    return next_word_fallback(W);
  uint32_t result = (buffer_ >> buffer_cursor_) & ((1 << W) - 1);
  buffer_cursor_ += W;
  return result;
}

uint32_t ArrayBitReader::next_word(uint32_t word_size) {
  if (buffer_cursor_ + word_size > 64)
    return next_word_fallback(word_size);
  uint32_t result = (buffer_ >> buffer_cursor_) & ((1 << word_size) - 1);
  buffer_cursor_ += word_size;
  return result;
}

uint16_t ArrayBitReader::next_short() {
  ASSERT(bit_cursor() == 0);
  if (buffer_cursor_ > (64 - 16))
    return next_byte() | (next_byte() << 8);
  uint16_t result = reinterpret_cast<const uint16_t*>(data_.begin() + data_cursor_ + (buffer_cursor_ >> 3))[0];
  buffer_cursor_ += 16;
  return result;

}

uint8_t ArrayBitReader::next_byte() {
  ASSERT(bit_cursor() == 0);
  if (buffer_cursor_ > (64 - 8))
    fetch_next_block();
  uint8_t result = data_[data_cursor_ + (buffer_cursor_ >> 3)];
  buffer_cursor_ += 8;
  return result;
}

void ProfilingByteWriter::copy(uint8_t value, uint32_t source, uint32_t copy, uint32_t bit_size) {
  ASSERT(source <= cursor_);
  ByteStat stat;
  stat.source = source;
  stat.copy = (copy + 1);
  stat.bit_size = bit_size;
  stat.value = value;
  add_stat(stat);
}

void ProfilingByteWriter::append(uint8_t value, uint32_t bit_size) {
  ByteStat stat;
  stat.source = cursor_;
  stat.copy = 0;
  stat.bit_size = bit_size;
  stat.value = value;
  literal_count_++;
  add_stat(stat);
}

void ProfilingByteWriter::add_stat(const ByteStat &stat) {
  if (cursor_ >= bytes_.size())
    grow_buffer();
  bytes_[cursor_++] = stat;
}

} // namespace impl
} // namespace zipprof
