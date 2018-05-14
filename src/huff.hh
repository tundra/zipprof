// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include "utils.hh"

#include <vector>

namespace zipprof {
namespace impl {

class HuffCode;

// A node in a huffman code tree.
class HuffNode {
public:
  // Creates a branch node.
  HuffNode(uint32_t index, HuffNode left, HuffNode right);

  // Creates a leaf node.
  HuffNode(uint32_t index, uint32_t value);

  // Creates an empty node.
  HuffNode();

  bool is_leaf() { return is_leaf_; }
  bool is_branch() { return !is_leaf_; }
  uint16_t index() { return index_; }
  inline HuffNode branch_left(HuffCode *code);
  inline HuffNode branch_right(HuffCode *code);
  uint16_t leaf_value() { return leaf_value_; }

private:
  bool is_leaf_;
  uint16_t index_;
  union {
    struct {
      uint16_t branch_left_;
      uint16_t branch_right_;
    };
    uint16_t leaf_value_;
  };
};

// A container of huffman tree nodes that takes care of disposing them when
// a block has been decoded.
class HuffCode {
public:
  HuffCode();
  ~HuffCode();

  // Returns a huff tree based on the given table of lengths.
  HuffNode new_code_tree(array<uint32_t> table);

  inline HuffNode node(uint32_t index) { return node_pool_[index]; }

  template <typename... As>
  HuffNode new_huff_node(As... as);

private:

  std::vector<HuffNode> node_pool_;
};

HuffNode HuffNode::branch_left(HuffCode *code) {
  return code->node(branch_left_);
}

HuffNode HuffNode::branch_right(HuffCode *code) {
  return code->node(branch_right_);
}

} // namespace impl
} // namespace zipprof
