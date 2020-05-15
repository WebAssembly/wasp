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

set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")

add_definitions(-Wall -Wextra -Wno-unused-parameter)

add_subdirectory(third_party/gtest)

include_directories(
  ${wasp_SOURCE_DIR}
  ${wasp_SOURCE_DIR}/include
  ${wasp_SOURCE_DIR}/include/gdtoa
  ${wasp_SOURCE_DIR}/third_party/fmt/include
  ${wasp_SOURCE_DIR}/third_party/optional-lite/include
  ${wasp_SOURCE_DIR}/third_party/span-lite/include
  ${wasp_SOURCE_DIR}/third_party/string-view-lite/include
  ${wasp_SOURCE_DIR}/third_party/variant-lite/include
  ${wasp_SOURCE_DIR}/third_party/parallel-hashmap
)

set(gdtoa_files
  third_party/gdtoa/strtorf.c
  third_party/gdtoa/strtord.c
  third_party/gdtoa/strtodg.c
  third_party/gdtoa/dmisc.c
  third_party/gdtoa/misc.c
  third_party/gdtoa/gmisc.c
  third_party/gdtoa/smisc.c
  third_party/gdtoa/gethex.c
  third_party/gdtoa/sum.c
  third_party/gdtoa/hd_init.c
  third_party/gdtoa/g_ffmt.c
  third_party/gdtoa/g_dfmt.c
  third_party/gdtoa/g__fmt.c
  third_party/gdtoa/gdtoa.c
)

add_library(wasplib
  src/base/at.cc
  src/base/features.cc
  src/base/file.cc
  src/base/print_to.cc
  src/base/span.cc
  src/base/str_to_u32.cc
  src/base/utf8.cc
  src/base/v128.cc
  src/base/wasm_types.cc
  src/binary/context.cc
  src/binary/encoding.cc
  src/binary/formatters.cc
  src/binary/lazy_expression.cc
  src/binary/lazy_module.cc
  src/binary/lazy_sequence.cc
  src/binary/linking_section/encoding.cc
  src/binary/linking_section/print_to.cc
  src/binary/linking_section/read.cc
  src/binary/linking_section/sections.cc
  src/binary/linking_section/types.cc
  src/binary/name_section/encoding.cc
  src/binary/name_section/print_to.cc
  src/binary/name_section/read.cc
  src/binary/name_section/sections.cc
  src/binary/name_section/types.cc
  src/binary/print_to.cc
  src/binary/read.cc
  src/binary/sections.cc
  src/binary/types.cc
  src/convert/to_binary.cc
  src/valid/context.cc
  src/valid/formatters.cc
  src/valid/validate.cc
  src/valid/validate_instruction.cc
  src/text/context.cc
  src/text/desugar.cc
  src/text/lex.cc
  src/text/numeric.cc
  src/text/print_to.cc
  src/text/read.cc
  src/text/read_script.cc
  src/text/resolve.cc
  src/text/token.cc
  src/text/types.cc

  third_party/fmt/src/format.cc
  ${gdtoa_files}
)

set_source_files_properties(
  ${gdtoa_files}
  PROPERTIES
  COMPILE_FLAGS "-Wno-parentheses -Wno-implicit-fallthrough -Wno-unused-label -Wno-sign-compare"
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
  test/test_utils.cc
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
  test/binary/visitor_test.cc
  test/binary/write_test.cc
  test/convert/to_binary_test.cc
  test/text/desugar_test.cc
  test/text/formatters_test.cc
  test/text/lex_test.cc
  test/text/numeric_test.cc
  test/text/read_test.cc
  test/text/resolve_test.cc
  test/text/token_test.cc
  test/text/types_test.cc
  test/text/write_test.cc
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

add_executable(run_spec_tests
  src/tools/argparser.cc
  test/run_spec_tests.cc
)

target_link_libraries(run_spec_tests
  wasplib
)

# TODO: Link stdc++fs conditionally, only when needed.
target_link_libraries(run_spec_tests
  stdc++fs
)

add_executable(wasp_sqlite
  src/tools/argparser.cc
  src/tools/wasp_sqlite.cc
  third_party/sqlite/sqlite3.c
)

set(sqlite_flags "-Wno-implicit-fallthrough")
if (CMAKE_C_COMPILER_ID MATCHES "GNU")
  set(sqlite_flags "${sqlite_flags} -Wno-cast-function-type")
endif ()

set_source_files_properties(
  third_party/sqlite/sqlite3.c
  PROPERTIES
  COMPILE_FLAGS "${sqlite_flags}"
)

target_link_libraries(wasp_sqlite
  wasplib
  dl
  pthread
)
