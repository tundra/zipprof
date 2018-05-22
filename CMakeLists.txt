# Copyright (c) 2018 Tundra. All right reserved.
# Use of this code is governed by the terms defined in LICENSE.

cmake_minimum_required (VERSION 2.8.12)
project(zipprof)
enable_testing()

add_subdirectory(deps/googletest googletest EXCLUDE_FROM_ALL)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-Wall -Werror -Wno-unused-function -Wno-unused-variable -o3 -g -std=c++11")
else()
  set(CMAKE_CXX_FLAGS "-Wall -Werror -Wno-unused-function -Wno-unused-variable -o0 -g -std=c++11")
endif()


file(GLOB src_files "src/zip.cc" "src/io.cc" "src/zipprof.cc" "src/huff.cc")
add_library(zipprof SHARED ${src_files})
include_directories(zipprof "include" "lib")
target_link_libraries(zipprof "z")

add_executable(zprof "src/main.cc")
target_link_libraries(zprof zipprof)

file(GLOB test_files "tests/*.hh" "tests/*.cc")
add_executable(zipprof_test_main ${test_files} ${src_files})
target_link_libraries(zipprof_test_main gtest_main "z")
include_directories(zipprof_test_main
  "src"
  "include"
  "deps/googletest/googletest/include")
add_test(NAME zipprof_test COMMAND zipprof_test_main)
