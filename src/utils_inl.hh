// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include "utils.hh"

#include <cassert>

namespace zipprof {
namespace impl {

template <typename T>
T &array<T>::operator[](uint32_t i) {
  ASSERT(i < size_);
  return elms_[i];
}

template <typename T>
void array<T>::fill(T value) {
  for (uint32_t i = 0; i < size_; i++)
    elms_[i] = value;
}

template <typename T>
array<T> array<T>::slice(uint32_t start) {
  ASSERT(start <= size_);
  return array<T>(elms_ + start, size_ - start);
}

template <typename T>
array<T> array<T>::slice(uint32_t start, uint32_t end) {
  ASSERT(start <= end);
  ASSERT(end <= size_);
  return array<T>(elms_ + start, end - start);
}

template <typename T, uint32_t S>
stack_array<T, S>::stack_array()
    : array<T>(stack_elms_, S) { array<T>::fill(0); }


} // namespace impl
} // namespace zipprof
