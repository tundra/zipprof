// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "zipprof_impl.hh"
#include "zip_inl.hh"
#include "utils_inl.hh"

#define ZLIB_CONST 1
#include <zlib.h>

using namespace zipprof;

class ZlibOutput : public Compressor::Output {
public:
  ZlibOutput() { }
  virtual ~ZlibOutput() { }
  virtual Array<const uint8_t> contents() override;
  void append(Array<uint8_t> block);
private:
  std::vector<uint8_t> bytes_;
};

void ZlibOutput::append(Array<uint8_t> block) {
  bytes_.insert(bytes_.end(), block.begin(), block.begin() + block.size());
}

static Array<const uint8_t> strip_zlib_header(Array<const uint8_t> data_arr) {
  array<const uint8_t> data(data_arr);
  uint8_t cmf = data[0];
  ASSERT(cmf == 0x78);
  uint8_t flg = data[1];
  ASSERT(((cmf * 256) + flg) % 31 == 0);
  uint8_t fdict = (flg >> 5) & 0x1;
  ASSERT(fdict == 0);
  uint8_t level = (flg >> 6) & 0x3;
  return Array<const uint8_t>(data.begin() + 2, data.size());
}

Array<const uint8_t> ZlibOutput::contents() {
  return strip_zlib_header(Array<uint8_t>(bytes_.data(), bytes_.size()));
}

class ZlibCompressor : public Compressor {
public:
  ZlibCompressor(uint32_t level)
      : level_(level) { }
  virtual ZlibOutput *compress(Array<const uint8_t> data) const override;

private:
  friend class Compressor;
  static const ZlibCompressor kBestSpeed;
  static const ZlibCompressor kBestCompression;
  static const ZlibCompressor kNoCompression;

  uint32_t level_;
};

ZlibOutput *ZlibCompressor::compress(Array<const uint8_t> input) const {
  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = input.size();
  stream.next_in = reinterpret_cast<const Bytef*>(input.begin());
  uint8_t buf[4096];
  stream.next_out = buf;
  stream.avail_out = 4096;
  if (deflateInit(&stream, level_) != Z_OK)
    return NULL;
  ZlibOutput *output = new ZlibOutput();
  uint32_t last_total_out = 0;
  bool keep_going = true;
  while (keep_going) {
    int res = deflate(&stream, Z_FINISH);
    if (res < 0) {
      delete output;
      return NULL;
    } else if (res == Z_STREAM_END) {
      keep_going = false;
    }
    output->append(Array<uint8_t>(buf, stream.total_out - last_total_out));
    last_total_out = stream.total_out;
  }
  ::deflateEnd(&stream);
  return output;
}

const ZlibCompressor ZlibCompressor::kBestSpeed(Z_BEST_SPEED);
const ZlibCompressor ZlibCompressor::kBestCompression(Z_BEST_COMPRESSION);
const ZlibCompressor ZlibCompressor::kNoCompression(Z_NO_COMPRESSION);

const Compressor &Compressor::zlib_best_speed() {
  return ZlibCompressor::kBestSpeed;
}

const Compressor &Compressor::zlib_best_compression() {
  return ZlibCompressor::kBestCompression;
}

const Compressor &Compressor::zlib_no_compression() {
  return ZlibCompressor::kNoCompression;
}

DeflateProfile Profiler::profile_deflated(Array<const uint8_t> data) {
  impl::ArrayBitReader reader(data);
  impl::ProfilingByteWriter writer;
  impl::Deflater<impl::ArrayBitReader, impl::ProfilingByteWriter> deflater(reader, writer);
  deflater.deflate();
  return writer.flush(data.size());
}

DeflateProfile Profiler::profile_zlib(Array<const uint8_t> data) {
  Array<const uint8_t> stripped = strip_zlib_header(data);
  return profile_deflated(stripped);
}

DeflateProfile Profiler::profile_string(std::string str, const Compressor &compressor) {
  Array<const uint8_t> data(reinterpret_cast<const uint8_t*>(str.c_str()), str.size() + 1);
  Compressor::Output *output = compressor.compress(data);
  DeflateProfile result = profile_deflated(output->contents());
  delete output;
  return result;
}

DeflateProfile::DeflateProfile() { }

DeflateProfile::DeflateProfile(Impl *impl)
    : impl_(impl) { }

DeflateProfile::~DeflateProfile() { }

uint32_t DeflateProfile::deflated_size() {
  return impl().deflated_size_;
}

uint32_t DeflateProfile::literal_count() {
  return impl().literal_count_;
}

uint32_t DeflateProfile::inflated_size() {
  return impl().inflated_size_;
}

uint32_t DeflateProfile::block_count() {
  return impl().block_stats_.size();
}

uint32_t DeflateProfile::literal_weight(uint32_t index) {
  return impl().literal_weights()[impl().origins()[index]];
}

double DeflateProfile::literal_contribution(uint32_t index) {
  return 1.0 / literal_weight(index);
}

Array<const uint8_t> DeflateProfile::contents() {
  array<const uint8_t> raw_contents = impl().contents();
  return Array<const uint8_t>(raw_contents.begin(), raw_contents.size());
}

DeflateProfile::Impl::Impl(uint32_t deflated_size, uint32_t inflated_size,
    uint32_t literal_count, array<ByteStat> byte_stats, array<BlockStat> block_stats)
    : deflated_size_(deflated_size)
    , inflated_size_(inflated_size)
    , literal_count_(literal_count)
    , byte_stats_(byte_stats)
    , block_stats_(block_stats) { }

array<uint32_t> DeflateProfile::Impl::origins() {
  if (origins_.begin() == NULL) {
    origins_ = array<uint32_t>(new uint32_t[inflated_size_], inflated_size_);
    memset(origins_.begin(), 0, origins_.size() * sizeof(uint32_t));
    for (uint32_t i = 0; i < inflated_size_; i++) {
      ByteStat stat = byte_stats_[i];
      origins_[i] = (stat.source == i)
          ? i
          : origins_[stat.source];
    }
  }
  return origins_;
}

array<uint32_t> DeflateProfile::Impl::literal_weights() {
  if (literal_weights_.begin() == NULL) {
    literal_weights_ = array<uint32_t>(new uint32_t[inflated_size_], inflated_size_);
    memset(literal_weights_.begin(), 0, literal_weights_.size() * sizeof(uint32_t));
    array<uint32_t> origins = this->origins();
    for (uint32_t i = 0; i < inflated_size_; i++)
      literal_weights_[origins[i]]++;
  }
  return literal_weights_;
}

array<uint8_t> DeflateProfile::Impl::contents() {
  if (contents_.begin() == NULL) {
    contents_ = array<uint8_t>(new uint8_t[inflated_size_], inflated_size_);
    for (uint32_t i = 0; i < inflated_size_; i++)
      contents_[i] = byte_stats_[i].value;
  }
  return contents_;
}

DeflateProfile::Impl::~Impl() {
  delete[] byte_stats_.begin();
  delete[] block_stats_.begin();
  delete[] origins_.begin();
  delete[] literal_weights_.begin();
  delete[] contents_.begin();
}

Archive::Archive(Array<const uint8_t> data)
    : impl_(Impl::open(data)) {

}

Archive::~Archive() { }

Array<std::string> Archive::entries() {
  return impl().entries();
}

DeflateProfile Archive::profile(std::string path) {
  array<const uint8_t> data = impl().stream(path);
  if (data.size() == 0)
    return DeflateProfile();
  return Profiler::profile_deflated(Array<const uint8_t>(data.begin(), data.size()));
}
