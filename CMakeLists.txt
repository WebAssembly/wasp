#
# Copyright 2018 WebAssembly Community Group participants
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.1)
project(wasp)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

add_definitions(-Wall)

add_subdirectory(third_party/gtest)

include_directories(
  ${wasp_SOURCE_DIR}
  ${wasp_SOURCE_DIR}/third_party/fmt/include
  ${wasp_SOURCE_DIR}/third_party/optional-lite/include
  ${wasp_SOURCE_DIR}/third_party/span-lite/include
  ${wasp_SOURCE_DIR}/third_party/string-view-lite/include
  ${wasp_SOURCE_DIR}/third_party/variant-lite/include
)

add_library(wasp
  src/base/file.cc
  src/binary/formatters.cc
  src/binary/reader.cc
  src/binary/types.cc

  third_party/fmt/src/format.cc
)

add_executable(wasp_objdump
  src/example/objdump.cc
)

target_link_libraries(wasp_objdump
  wasp
)

add_executable(wasp_unittests
  src/base/formatters_test.cc
  src/binary/formatters_test.cc
  src/binary/lazy_expression_test.cc
  src/binary/lazy_module_test.cc
  src/binary/lazy_name_section_test.cc
  src/binary/lazy_section_test.cc
  src/binary/lazy_sequence_test.cc
  src/binary/reader_test.cc
  src/binary/test_utils.cc
)

target_link_libraries(wasp_unittests
  wasp
  gtest_main
  gtest
)
