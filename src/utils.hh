// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include <cstring>
#include <stdint.h>
#include <string>

#include "zipprof.h"

#ifndef NDEBUG
#  define ASSERT(EXPR) assert(EXPR)
#else
#  define ASSERT(EXPR)
#endif

namespace zipprof {
namespace impl {

// A simple wrapper around an array and a length.
template <typename T>
class array {
public:
  array() : elms_(NULL), size_(0) { }
  array(T *elms, uint32_t size) : elms_(elms), size_(size) { }
  array(Array<T> arr) : elms_(arr.begin()), size_(arr.size()) { }
  array(array<typename _StripConst<T>::Type> that) : elms_(that.begin()), size_(that.size()) { }
  uint32_t size() const { return size_; }
  array<T> slice(uint32_t start);
  array<T> slice(uint32_t start, uint32_t end);
  void fill(T value);
  T &operator[](uint32_t i);
  T *begin() { return elms_; }
private:
  T *elms_;
  uint32_t size_;
};

// A stack-allocated array.
template <typename T, uint32_t S>
class stack_array : public array<T> {
public:
  stack_array();
private:
  T stack_elms_[S];
};

static array<const char> c_str_to_array(const char *str) {
  return array<const char>(str, ::strlen(str) + 1);
}

} // namespace impl
} // namespace zipprof
