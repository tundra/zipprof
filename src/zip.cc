#include "zip_inl.hh"

#include "miniz.c"

using namespace zipprof;

Archive::Impl::Impl() { }

Array<std::string> Archive::Impl::entries() {
  if (entries_.empty())
    add_paths(&entries_);
  return Array<std::string>(entries_.data(), entries_.size());
}

class MzImpl : public Archive::Impl {
public:
  ~MzImpl();
  MzImpl(array<const uint8_t> bytes);
  virtual void add_paths(std::vector<std::string> *paths_out) override;
  virtual array<const uint8_t> stream(std::string path);

private:
  mz_zip_archive arc_;
  array<const uint8_t> bytes_;
};

Archive::Impl *Archive::Impl::open(array<const uint8_t> bytes) {
  return new MzImpl(bytes);
}

MzImpl::MzImpl(array<const uint8_t> bytes)
    : bytes_(bytes) {
  memset(&arc_, 0, sizeof(arc_));
  mz_zip_reader_init_mem(&arc_, bytes.begin(), bytes.size(), 0);
}

void MzImpl::add_paths(std::vector<std::string> *paths_out) {
  uint32_t count = mz_zip_reader_get_num_files(&arc_);
  for (uint32_t i = 0; i < count; i++) {
    char buf[4096];
    memset(buf, 0, 4096);
    uint32_t size = mz_zip_reader_get_filename(&arc_, i, buf, 4096);
    paths_out->push_back(std::string(buf, size));
  }
}

array<const uint8_t> MzImpl::stream(std::string path) {
  int loc = mz_zip_reader_locate_file(&arc_, path.c_str(), NULL, 0);
  if (loc == -1)
    return array<const uint8_t>();
  mz_zip_archive_file_stat stat;
  memset(&stat, 0, sizeof(stat));
  if (!mz_zip_reader_file_stat(&arc_, loc, &stat))
    return array<const uint8_t>();
  uint32_t header_ofs = stat.m_local_header_ofs;
  const uint8_t *header = bytes_.begin() + header_ofs;
  uint8_t buf[65536];
  uint16_t filename_size = *reinterpret_cast<const uint16_t*>(header + MZ_ZIP_LDH_FILENAME_LEN_OFS);
  uint16_t extra_size = *reinterpret_cast<const uint16_t*>(header + MZ_ZIP_LDH_EXTRA_LEN_OFS);
  uint32_t payload_ofs = header_ofs + MZ_ZIP_LOCAL_DIR_HEADER_SIZE + filename_size + extra_size;
  const uint8_t *payload = bytes_.begin() + payload_ofs;
  return array<const uint8_t>(payload, stat.m_comp_size);
}

MzImpl::~MzImpl() {
  mz_zip_reader_end(&arc_);
}
