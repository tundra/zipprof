#pragma once

#include "zipprof.h"

#include <sstream>
#include <fstream>
#include <iostream>

static std::string read_file(std::string path) {
  std::stringstream bytes;
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::cerr << "Couldn't open file " << path << std::endl;
    return std::string();
  }
  bytes << file.rdbuf();
  file.close();
  return bytes.str();
}

static zipprof::Array<const uint8_t> string_to_data(const std::string &str) {
  return zipprof::Array<const uint8_t>(
      reinterpret_cast<const uint8_t*>(str.c_str()),
      str.size());
}

static std::string data_to_string(zipprof::Array<const uint8_t> data) {
  return std::string(reinterpret_cast<const char*>(data.begin()), data.size());
}
