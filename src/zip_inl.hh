// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "zip.hh"
#include "io_inl.hh"

#include "utils_inl.hh"

using namespace zipprof;
using namespace impl;

template <typename R, typename W>
Deflater<R, W>::Deflater(R &in, W &out)
    : in_(in)
    , fixed_len_root_(0, 0)
    , has_fixed_len_root_(false)
    , fixed_dist_root_(0, 0)
    , has_fixed_dist_root_(false)
    , out_(NULL) {
  out_ = new OutputTracker<W>(32 * 1024, out);
}

template <typename R, typename W>
Deflater<R, W>::~Deflater() {
  delete out_;
}

template <typename R, typename W>
HuffNode Deflater<R, W>::fixed_len_root() {
  if (!has_fixed_len_root_) {
    stack_array<uint32_t, 288> table;
    table.slice(0, 144).fill(8);
    table.slice(144, 256).fill(9);
    table.slice(256, 280).fill(7);
    table.slice(280, 288).fill(8);
    fixed_len_root_ = fixed_code()->new_code_tree(table);
    has_fixed_len_root_ = true;
  }
  return fixed_len_root_;
}

template <typename R, typename W>
HuffNode Deflater<R, W>::fixed_dist_root() {
  if (!has_fixed_dist_root_) {
    stack_array<uint32_t, 32> table;
    table.fill(5);
    fixed_dist_root_ = fixed_code()->new_code_tree(table);
    has_fixed_dist_root_ = true;
  }
  return fixed_dist_root_;
}

template <typename R, typename W>
void Deflater<R, W>::deflate() {
  bool keep_going = true;
  while (keep_going) {
    Account block_account;
    uint8_t last_block_bit = in().next_bit(block_account);
    keep_going = (last_block_bit != 1);
    encoding_method method = encoding_method(in().template next_word<2>(block_account));
    out().out().open_block(static_cast<uint8_t>(method));
    switch (method) {
    case encoding_method::RAW:
      decompress_raw(block_account);
      break;
    case encoding_method::HUFFMAN_STATIC:
      decompress_huffman(block_account, fixed_code(), fixed_len_root(), fixed_dist_root());
      break;
    case encoding_method::HUFFMAN: {
      HuffCode container;
      HuffNode len_root(0, 0);
      HuffNode dist_root(0, 0);
      decode_huffman_codes(block_account, &container, &len_root, &dist_root);
      decompress_huffman(block_account, &container, len_root, dist_root);
      break;
    }
    case encoding_method::RESERVED:
      throw DeflateError();
    }
    out().out().close_block(block_account.close());
  }
}

template <typename R, typename W>
void Deflater<R, W>::decompress_huffman(Account &block_account, HuffCode *code,
    HuffNode len_root, HuffNode dist_root) {
  while (true) {
    Account account;
    uint32_t symbol = decode_symbol(account, code, len_root);
    if (symbol < 256) {
      uint8_t value = static_cast<uint8_t>(symbol);
      out().add(value, account.close());
    } else if (symbol == 256) {
      block_account.inc(account.close());
      break;
    } else {
      uint32_t run = decode_run_length(account, symbol);
      uint32_t dist_sym = decode_symbol(account, code, dist_root);
      uint32_t dist = decode_distance(account, dist_sym);
      out().copy(dist, run, account.close());
    }
  }
}

template <typename R, typename W>
uint32_t Deflater<R, W>::decode_symbol(Account &account, HuffCode *code,
    HuffNode root) {
  HuffNode current = root;
  while (true) {
    ASSERT(current.is_branch());
    HuffNode next = (in().next_bit(account) == 0)
        ? current.branch_left(code)
        : current.branch_right(code);
    if (next.is_leaf())
      return next.leaf_value();
    else
      current = next;
  }
}

template <typename R, typename W>
uint32_t Deflater<R, W>::decode_run_length(Account &account, uint32_t symbol) {
  if (symbol <= 264) {
    return symbol - 254;
  } else if (symbol <= 284) {
    uint32_t extra_bits = (symbol - 261) >> 2;
    return ((((symbol - 265) & 0x3) + 4) << extra_bits) + 3 + in().next_word(extra_bits, account);
  } else if (symbol == 285) {
    return 258;
  } else {
    throw DeflateError();
  }
}

template <typename R, typename W>
uint32_t Deflater<R, W>::decode_distance(Account &account, uint32_t symbol) {
  if (symbol <= 3) {
    return symbol + 1;
  } else if (symbol <= 29) {
    uint32_t extra_bits = (symbol >> 1) - 1;
    return (((symbol & 1) + 2) << extra_bits) + 1 + in().next_word(extra_bits, account);
  } else {
    throw DeflateError();
  }
}

template <typename R, typename W>
void Deflater<R, W>::decode_huffman_codes(Account &account, HuffCode *code,
    HuffNode *len_root_out, HuffNode *dist_root_out) {
  uint32_t num_lit_len_codes = in().template next_word<5>(account) + 257;
  uint32_t num_dist_codes = in().template next_word<5>(account) + 1;
  uint32_t num_code_len_codes = in().template next_word<4>(account) + 4;
  stack_array<uint32_t, 19> code_len_code_len;
  code_len_code_len.fill(0);
  code_len_code_len[16] = in().template next_word<3>(account);
  code_len_code_len[17] = in().template next_word<3>(account);
  code_len_code_len[18] = in().template next_word<3>(account);
  code_len_code_len[0] = in().template next_word<3>(account);
  for (uint32_t i = 0; i < num_code_len_codes - 4; i++) {
    uint32_t index = ((i & 1) == 0) ? (8 + i / 2) : (7 - i / 2);
    code_len_code_len[index] = in().template next_word<3>(account);
  }
  HuffNode code_len_code = code->new_code_tree(code_len_code_len);
  uint32_t code_lens_len = num_lit_len_codes + num_dist_codes;
  uint32_t *code_lens_buf = new uint32_t[code_lens_len];
  array<uint32_t> code_lens(code_lens_buf, code_lens_len);
  code_lens.fill(0);
  int32_t run_val = -1;
  int32_t run_len = 0;
  for (uint32_t i = 0; i < code_lens_len;) {
    if (run_len > 0) {
      code_lens[i] = run_val;
      run_len--;
      i++;
    } else {
      uint32_t symbol = decode_symbol(account, code, code_len_code);
      if (0 <= symbol && symbol <= 15) {
        code_lens[i] = symbol;
        run_val = symbol;
        i++;
      } else if (symbol == 16) {
        run_len = in().template next_word<2>(account) + 3;
      } else if (symbol == 17) {
        run_val = 0;
        run_len = in().template next_word<3>(account) + 3;
      } else if (symbol == 18) {
        run_val = 0;
        run_len = in().template next_word<7>(account) + 11;
      }
    }
  }
  *len_root_out = code->new_code_tree(code_lens.slice(0, num_lit_len_codes));
  *dist_root_out = code->new_code_tree(code_lens.slice(num_lit_len_codes));
  delete[] code_lens_buf;
}

template <typename R, typename W>
void Deflater<R, W>::decompress_raw(Account &block_account) {
  in().ensure_aligned(block_account);
  uint32_t len = in().next_short(block_account);
  uint32_t nlen = in().next_short(block_account);
  if ((nlen ^ 0xFFFF) != len)
    throw DeflateError();
  for (uint32_t i = 0; i < len; i++) {
    Account next_account;
    uint8_t next = in().next_byte(next_account);
    out().add(next, next_account.close());
  }
}

void Account::inc(uint32_t delta) {
  bit_count_ += delta;
}

uint32_t Account::close() {
  uint32_t result = bit_count_;
  bit_count_ = 0;
  return result;
}

Account::~Account() {
  ASSERT(bit_count_ == 0);
}

template <typename R>
uint32_t InputTracker<R>::next_bit(Account &account) {
  account.inc(1);
  return reader().next_bit();
}

template <typename R>
template <uint32_t W>
uint32_t InputTracker<R>::next_word(Account &account) {
  account.inc(W);
  return reader().next_word<W>();
}

template <typename R>
uint32_t InputTracker<R>::next_word(uint32_t width, Account &account) {
  account.inc(width);
  return reader().next_word(width);
}

template <typename R>
uint8_t InputTracker<R>::next_byte(Account &account) {
  account.inc(8);
  return reader().next_byte();
}

template <typename R>
uint16_t InputTracker<R>::next_short(Account &account) {
  account.inc(16);
  return reader().next_short();
}

template <typename R>
uint8_t InputTracker<R>::ensure_aligned(Account &account) {
  uint8_t result = reader().ensure_aligned();
  account.inc(result);
  return result;
}

template <typename W>
OutputTracker<W>::OutputTracker(uint32_t size, W &out)
    : buf_(new uint8_t[size])
    , size_(size)
    , mask_(size - 1)
    , out_cur_(0)
    , copy_cur_(0)
    , out_(out) { }

template <typename W>
OutputTracker<W>::~OutputTracker() {
  delete[] buf_;
}

template <typename W>
void OutputTracker<W>::add(uint8_t value, uint32_t bit_size) {
  out().append(value, bit_size);
  buf_[out_cur_++ & mask_] = value;
}

template <typename W>
void OutputTracker<W>::copy(uint32_t dist, uint32_t len, uint32_t bit_size) {
  uint32_t start = out_cur_ - dist;
  for (uint32_t i = 0; i < len; i++) {
    uint32_t pos = start + i;
    uint8_t value = buf_[pos & mask_];
    out().copy(value, pos, copy_cur_, bit_size);
    buf_[out_cur_++ & mask_] = value;
  }
  copy_cur_++;
}

