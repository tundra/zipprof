// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <memory>

namespace zipprof {

namespace impl {
template <typename S> struct _StripConst { struct Type; };
template <typename S> struct _StripConst<const S> { typedef S Type; };
} // namespace impl

template <typename T>
class Array {
public:
  Array(T *elms, size_t size) : elms_(elms), size_(size) { }
  Array(Array<typename impl::_StripConst<T>::Type> that) : elms_(that.begin()), size_(that.size()) { }
  size_t size() { return size_; }
  T *begin() { return elms_; }
private:
  T *elms_;
  size_t size_;
};

class DeflateProfile {
public:
  class Impl;
  DeflateProfile();
  ~DeflateProfile();

  // The size in bytes of the compressed data. This is the size of the full
  // deflate structure, including code tables etc.
  uint32_t deflated_size();

  // The number of literal bytes int the compressed archive, that is, the number
  // of bytes that appear directly and aren't copied from earlier offsets.
  uint32_t literal_count();

  // The size in bytes of the data that was compressed.
  uint32_t inflated_size();

  // The number of zip blocks the archive was divided into.
  uint32_t block_count();

  // Returns the weight of the byte at that index'th position, that is, the
  // number of times it has been copied. All copies of the same byte have the
  // same weight. This is the reciprocal value of the literal_contribution.
  uint32_t literal_weight(uint32_t index);

  // How much does the byte at the given index contribute towards the total
  // count of literals. A single literal byte contributes one, a byte that is
  // copied exactly once contributes 0.5 with the other copy contributing the
  // other 0.5, and so on. The sum of literal contributions yields the literal
  // count.
  double literal_contribution(uint32_t index);

  // Returns the deflated contents of the input.
  Array<const uint8_t> contents();

private:
  friend class Profiler;
  DeflateProfile(Impl *impl);

  Impl &impl() { return *impl_; }
  std::shared_ptr<Impl> impl_;
};

class Compressor {
public:
  class Output {
  public:
    Output() { }
    virtual ~Output() { }
    virtual Array<const uint8_t> contents() = 0;
  };

  virtual ~Compressor() { }
  virtual Output *compress(Array<const uint8_t> data) const = 0;

  static const Compressor &zlib_best_speed();
  static const Compressor &zlib_best_compression();
  static const Compressor &zlib_no_compression();
};

class Profiler {
public:
  // Returns a profile of a naked deflated block.
  static DeflateProfile profile_deflated(Array<const uint8_t> data);

  // Returns a profile of the output of compressing data with zlib.
  static DeflateProfile profile_zlib(Array<const uint8_t> data);

  // Returns the profile of compressing the given string with the given
  // compressor.
  static DeflateProfile profile_string(std::string str,
      const Compressor &compressor = Compressor::zlib_best_compression());
};

class Archive {
public:
  class Impl;
  Archive(Array<const uint8_t> data);
  ~Archive();

  // Returns a profile of the file within this archive with the given path.
  DeflateProfile profile(std::string path);

  // Returns an array of the entries within this archive. The result is owned
  // by the archive and valid until the instance is destroyed.
  Array<std::string> entries();

private:
  Impl &impl() { return *impl_; }
  std::shared_ptr<Impl> impl_;
};

} // namespace zipprof

