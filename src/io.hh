// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include "utils.hh"

#include <vector>

namespace zipprof {
namespace impl {

// Bit reader that takes its data from a flat array of bytes.
class ArrayBitReader {
public:
  ArrayBitReader(array<const uint8_t> data);

  // Returns the current bit offset, from 0 to 7.
  uint8_t bit_cursor();

  // Returns the next bit.
  inline uint8_t next_bit();

  // Returns the next block_size-size integer value.
  inline uint32_t next_word(uint32_t word_size);

  // Like next_word(uint32_t) but where the word size is fixed statically.
  template <uint32_t W> inline uint32_t next_word();

  inline uint16_t next_short();
  inline uint8_t next_byte();

  uint8_t ensure_aligned();

private:
  // Advances to the next byte boundary.
  void flush_current_byte();

  void fetch_next_block();

  uint32_t next_word_fallback(uint32_t word_size);

  array<const uint8_t> data_;
  // Index of the beginning of the current buffer.
  uint32_t data_cursor_;
  // Index of the next bit in the buffer.
  uint32_t buffer_cursor_;
  uint64_t buffer_;
};

// Utility for writing output as blocks of bytes.
class ByteWriter {
public:
  // Write the given byte of data to the output which has been copied from the
  // given source location as part of the copy operation with the given serial
  // number.
  void copy(uint8_t data, uint32_t source, int32_t copy, uint32_t bit_size);

  // Append the given literal byte to the output.
  void append(uint8_t data, uint32_t bit_size);

  // Called whenever a new block is encountered.
  void open_block(uint8_t type) { }

  void close_block(uint32_t bit_count) { }
};

class VectorByteWriter : public ByteWriter {
public:
  VectorByteWriter(std::vector<uint8_t> &buf);
  void copy(uint8_t data, uint32_t source, int32_t copy, uint32_t size_bits);
  void append(uint8_t data, uint32_t size_bits);

private:
  std::vector<uint8_t> &buf_;
};

// Information about an individual byte in the output.
struct ByteStat {
  // Index of the byte this one was copied from.
  uint32_t source;

  // Unique id of the copy operation that caused this byte to appear, or 0 if
  // it was a literal.
  uint32_t copy;

  // How many bits did it take to encode this byte?
  uint32_t bit_size;
};

struct BlockStat {
  uint32_t type;
};

class ProfilingByteWriter : public ByteWriter {
public:
  ProfilingByteWriter();
  ~ProfilingByteWriter();
  inline void copy(uint8_t data, uint32_t source, uint32_t copy, uint32_t bit_size);
  inline void append(uint8_t data, uint32_t bit_size);
  void open_block(uint8_t type);
  void close_block(uint32_t bit_count);
  DeflateProfile::Impl *flush(uint32_t zsize);
private:
  inline void add_stat(const ByteStat &stat);
  void grow_buffer();

  array<ByteStat> bytes_;
  std::vector<BlockStat> blocks_;
  uint32_t cursor_;
  uint32_t literal_count_;
};

} // namespace impl
} // namespace zipprof

