// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "huff.hh"

#include "utils_inl.hh"


using namespace zipprof;
using namespace zipprof::impl;

HuffNode::HuffNode()
    : is_leaf_(true)
    , index_(0)
    , leaf_value_(0) { }

HuffNode::HuffNode(uint32_t index, uint32_t value)
    : is_leaf_(true)
    , index_(index)
    , leaf_value_(value) { }

HuffNode::HuffNode(uint32_t index, HuffNode left, HuffNode right)
    : is_leaf_(false)
    , index_(index)
    , branch_left_(left.index_)
    , branch_right_(right.index_) { }

template <typename... As>
HuffNode HuffCode::new_huff_node(As... as) {
  uint32_t index = node_pool_.size();
  HuffNode result(index, as...);
  node_pool_.push_back(result);
  return node(index);
}

HuffNode HuffCode::new_code_tree(array<uint32_t> table) {
  std::vector<HuffNode> *nodes = new std::vector<HuffNode>();
  for (int32_t sil = 15; sil >= 0; sil--) {
    uint32_t il = static_cast<uint32_t>(sil);
    std::vector<HuffNode> *new_nodes = new std::vector<HuffNode>();
    if (il > 0) {
      for (uint32_t ie = 0; ie < table.size(); ie++) {
        if (table[ie] == il)
          new_nodes->push_back(new_huff_node(ie));
      }
    }
    for (uint32_t in = 0; in < nodes->size(); in += 2)
      new_nodes->push_back(new_huff_node(nodes->at(in), nodes->at(in + 1)));
    delete nodes;
    nodes = new_nodes;
  }
  HuffNode root = nodes->at(0);
  delete nodes;
  return node(root.index());
}

HuffCode::HuffCode() { }

HuffCode::~HuffCode() { }
