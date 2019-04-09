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
)

add_library(wasplib
  src/base/features.cc
  src/base/file.cc
  src/base/str_to_u32.cc
  src/base/v128.cc
  src/binary/br_on_exn_immediate.cc
  src/binary/br_table_immediate.cc
  src/binary/call_indirect_immediate.cc
  src/binary/code.cc
  src/binary/comdat.cc
  src/binary/comdat_symbol.cc
  src/binary/constant_expression.cc
  src/binary/copy_immediate.cc
  src/binary/custom_section.cc
  src/binary/data_count.cc
  src/binary/data_segment.cc
  src/binary/element_expression.cc
  src/binary/element_segment.cc
  src/binary/empty_immediate.cc
  src/binary/export.cc
  src/binary/expression.cc
  src/binary/formatters.cc
  src/binary/function.cc
  src/binary/function_type.cc
  src/binary/global.cc
  src/binary/global_type.cc
  src/binary/import.cc
  src/binary/indirect_name_assoc.cc
  src/binary/init_immediate.cc
  src/binary/instruction.cc
  src/binary/known_section.cc
  src/binary/lazy_expression.cc
  src/binary/limits.cc
  src/binary/linking_subsection.cc
  src/binary/locals.cc
  src/binary/mem_arg_immediate.cc
  src/binary/memory.cc
  src/binary/memory_type.cc
  src/binary/name_assoc.cc
  src/binary/name_subsection.cc
  src/binary/read.cc
  src/binary/read_module.cc
  src/binary/read_name_subsection.cc
  src/binary/read_section.cc
  src/binary/relocation_entry.cc
  src/binary/section.cc
  src/binary/segment_info.cc
  src/binary/start.cc
  src/binary/symbol_info.cc
  src/binary/table.cc
  src/binary/table_type.cc
  src/binary/type_entry.cc
  src/valid/context.cc
  src/valid/validate.cc
  src/valid/validate_instruction.cc

  third_party/fmt/src/format.cc
)

add_executable(wasp
  src/tools/wasp.cc
  src/tools/callgraph.cc
  src/tools/cfg.cc
  src/tools/dfg.cc
  src/tools/dump.cc
)

target_link_libraries(wasp
  wasplib
)

add_executable(wasp_unittests
  test/base/formatters_test.cc
  test/base/str_to_u32_test.cc
  test/base/v128_test.cc
  test/binary/formatters_test.cc
  test/binary/lazy_expression_test.cc
  test/binary/lazy_module_test.cc
  test/binary/lazy_module_utils_test.cc
  test/binary/lazy_name_section_test.cc
  test/binary/lazy_section_test.cc
  test/binary/lazy_sequence_test.cc
  test/binary/read_test.cc
  test/binary/read_linking_test.cc
  test/binary/test_utils.cc
  test/binary/write_test.cc
  test/valid/validate_test.cc
  test/valid/validate_code_test.cc
  test/valid/validate_instruction_test.cc
)

target_link_libraries(wasp_unittests
  wasplib
  gtest_main
  gtest
)
