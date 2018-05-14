// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include "huff.hh"
#include "io.hh"
#include "utils.hh"

namespace zipprof {
namespace impl {

class DeflateError : public std::exception { };

class Account {
public:
  inline Account() : bit_count_(0) { }
  inline ~Account();
  inline void inc(uint32_t delta);
  inline uint32_t close();

private:
  uint32_t bit_count_;
};

// A utility wrapped around a bit reader that takes care of accounting the
// number of bits read to some account.
template <typename Reader>
class InputTracker {
public:
  InputTracker(Reader &reader)
      : reader_(reader) { }
  inline uint32_t next_bit(Account &account);
  template <uint32_t W> uint32_t next_word(Account &account);
  inline uint32_t next_word(uint32_t width, Account &account);
  inline uint8_t next_byte(Account &account);
  inline uint16_t next_short(Account &account);
  inline uint8_t ensure_aligned(Account &account);

private:
  Reader &reader() { return reader_; }
  Reader &reader_;
};

// A utility wrapped around a byte writer that keeps track of positions and
// buffers data so it can be copied.
template <typename Writer>
class OutputTracker {
public:
  OutputTracker(uint32_t size, Writer &out);
  ~OutputTracker();

  // Adds a single byte to the output.
  void add(uint8_t c, uint32_t bit_size);

  // Copies a previously seen range.
  void copy(uint32_t dist, uint32_t len, uint32_t bit_size);

  Writer &out() { return out_; }

private:
  // Adds a single byte along with the position the byte originated from,
  // possibly the current position, and an index of the copy operation this
  // add is part of, if it is part of one otherwise -1.
  void add(uint8_t c, uint32_t pos, int32_t copy);

  uint8_t *buf_;
  uint32_t size_;
  uint32_t mask_;
  uint32_t out_cur_;
  uint32_t copy_cur_;
  Writer &out_;
};

template <typename Reader, typename Writer>
class Deflater {
public:
  Deflater(Reader &in, Writer &out);
  ~Deflater();

  // Deflates input from the reader, writing it to the writer.
  void deflate();

private:
  enum class encoding_method {
    RAW = 0,
    HUFFMAN_STATIC = 1,
    HUFFMAN = 2,
    RESERVED = 3
  };

  void decompress_raw(Account &block_account);
  void decompress_huffman(Account &block_account, HuffCode *code, HuffNode len_root, HuffNode dist_root);
  uint32_t decode_symbol(Account &account, HuffCode *code, HuffNode root);
  uint32_t decode_run_length(Account &account, uint32_t symbol);
  uint32_t decode_distance(Account &account, uint32_t symbol);
  void decode_huffman_codes(Account &account, HuffCode *code,
      HuffNode *len_root_out, HuffNode *dist_root_out);

  InputTracker<Reader> in_;
  InputTracker<Reader> &in() { return in_; }

  // Returns the fixed length code tree, creating it on first call.
  HuffNode fixed_len_root();
  HuffNode fixed_len_root_;
  bool has_fixed_len_root_;

  // Returns the fixed distance code tree, creating it on first call.
  HuffNode fixed_dist_root();
  HuffNode fixed_dist_root_;
  bool has_fixed_dist_root_;

  HuffCode *fixed_code() { return &fixed_code_; }
  HuffCode fixed_code_;
  OutputTracker<Writer> &out() { return *out_; }
  OutputTracker<Writer> *out_;
};

} // namespace impl
} // namespace zipprof

