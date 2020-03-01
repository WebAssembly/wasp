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

add_definitions(-Wall -Wextra -Wno-unused-parameter)

add_subdirectory(third_party/gtest)

include_directories(
  ${wasp_SOURCE_DIR}
  ${wasp_SOURCE_DIR}/include
  ${wasp_SOURCE_DIR}/third_party/fmt/include
  ${wasp_SOURCE_DIR}/third_party/optional-lite/include
  ${wasp_SOURCE_DIR}/third_party/span-lite/include
  ${wasp_SOURCE_DIR}/third_party/string-view-lite/include
  ${wasp_SOURCE_DIR}/third_party/variant-lite/include
  ${wasp_SOURCE_DIR}/third_party/parallel-hashmap
)

add_library(wasplib
  src/base/at.cc
  src/base/features.cc
  src/base/file.cc
  src/base/span.cc
  src/base/str_to_u32.cc
  src/base/utf8.cc
  src/base/v128.cc
  src/binary/context.cc
  src/binary/formatters.cc
  src/binary/lazy_expression.cc
  src/binary/lazy_module.cc
  src/binary/lazy_sequence.cc
  src/binary/linking_section/read.cc
  src/binary/linking_section/sections.cc
  src/binary/linking_section/types.cc
  src/binary/name_section/read.cc
  src/binary/name_section/sections.cc
  src/binary/name_section/types.cc
  src/binary/read.cc
  src/binary/sections.cc
  src/binary/types.cc
  src/valid/context.cc
  src/valid/formatters.cc
  src/valid/validate.cc
  src/valid/validate_instruction.cc

  third_party/fmt/src/format.cc
)

add_executable(wasp
  src/tools/argparser.cc
  src/tools/callgraph.cc
  src/tools/cfg.cc
  src/tools/dfg.cc
  src/tools/dump.cc
  src/tools/validate.cc
  src/tools/pattern.cc
  src/tools/wasp.cc
)

target_link_libraries(wasp
  wasplib
)

add_executable(wasp_unittests
  src/tools/argparser.cc
  test/base/enumerate_test.cc
  test/base/formatters_test.cc
  test/base/hash_test.cc
  test/base/str_to_u32_test.cc
  test/base/utf8_test.cc
  test/base/v128_test.cc
  test/binary/formatters_test.cc
  test/binary/lazy_expression_test.cc
  test/binary/lazy_linking_section_test.cc
  test/binary/lazy_module_test.cc
  test/binary/lazy_module_utils_test.cc
  test/binary/lazy_name_section_test.cc
  test/binary/lazy_relocation_section_test.cc
  test/binary/lazy_section_test.cc
  test/binary/lazy_sequence_test.cc
  test/binary/read_test.cc
  test/binary/read_linking_test.cc
  test/binary/test_utils.cc
  test/binary/visitor_test.cc
  test/binary/write_test.cc
  test/tools/argparser_test.cc
  test/valid/test_utils.cc
  test/valid/validate_test.cc
  test/valid/validate_code_test.cc
  test/valid/validate_instruction_test.cc
)

target_link_libraries(wasp_unittests
  wasplib
  gmock
  gmock_main
)

add_executable(wasp_sqlite
  src/tools/argparser.cc
  src/tools/wasp_sqlite.cc
  third_party/sqlite/sqlite3.c
)

target_link_libraries(wasp_sqlite
  wasplib
  dl
  pthread
)
