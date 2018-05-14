// Copyright (c) 2018 Tundra. All right reserved.
// Use of this code is governed by the terms defined in LICENSE.

#include "zipprof.h"

#include <argp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


using namespace zipprof;

class Arguments {
public:
  void parse(Array<char*> cmdline);

  std::vector<std::string> &args() { return args_; }

private:
  static error_t dispatch_parse_option(int key, char *arg, argp_state *state);
  error_t parse_option(int key, char *arg, argp_state *state);

  static const argp_option kOptions[2];
  static const argp kParser;

  std::vector<std::string> args_;
};


const argp_option Arguments::kOptions[2] = {
    {"histogram", 'h', 0, 0, "b"},
    {NULL}
};

error_t Arguments::dispatch_parse_option(int key, char *arg, struct argp_state *state) {
  return static_cast<Arguments*>(state->input)->parse_option(key, arg, state);
}

error_t Arguments::parse_option(int key, char *arg, struct argp_state *state) {
  switch (key) {
  case 'h':
    break;
  case ARGP_KEY_ARG:
    args_.push_back(arg);
    break;
  case ARGP_KEY_END:
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

void Arguments::parse(Array<char*> cmdline) {
  argp_parse(&kParser, cmdline.size(), cmdline.begin(), 0, 0, this);
}

const argp Arguments::kParser = { kOptions, dispatch_parse_option, "", NULL };

class ZProf {
public:
  int main(Array<char*> cmdline);

private:
  static std::string read_file(std::string path);
  void profile_file(std::string path);

  Arguments args_;
};

std::string ZProf::read_file(std::string path) {
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

void ZProf::profile_file(std::string path) {
  std::string str = read_file(path);
  Array<const uint8_t> data(reinterpret_cast<const uint8_t*>(str.c_str()), str.size());
  DeflateProfile profile = Profiler::profile_zlib(data);
  std::cout << profile.block_count() << std::endl;
}

int ZProf::main(Array<char*> cmdline) {
  args_.parse(cmdline);
  for (auto it = args_.args().begin(); it != args_.args().end(); it++)
    profile_file(*it);
  return 0;
}

int main(int argc, char *argv[]) {
  ZProf zprof;
  return zprof.main(Array<char*>(argv, argc));
}
