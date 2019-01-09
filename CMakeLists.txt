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
  src/binary/code.cc
  src/binary/constant_expression.cc
  src/binary/custom_section.cc
  src/binary/data_segment.cc
  src/binary/element_segment.cc
  src/binary/export.cc
  src/binary/expression.cc
  src/binary/formatters.cc
  src/binary/function.cc
  src/binary/function_type.cc
  src/binary/global.cc
  src/binary/global_type.cc
  src/binary/import.cc
  src/binary/instruction.cc
  src/binary/known_section.cc
  src/binary/limits.cc
  src/binary/locals.cc
  src/binary/memory.cc
  src/binary/memory_type.cc
  src/binary/name_section.cc
  src/binary/section.cc
  src/binary/start.cc
  src/binary/table.cc
  src/binary/table_type.cc
  src/binary/type_entry.cc

  third_party/fmt/src/format.cc
)

add_executable(wasp
  src/tools/wasp.cc
  src/tools/dump.cc
)

target_link_libraries(wasp
  wasplib
)

add_executable(wasp_unittests
  src/base/formatters_test.cc
  src/binary/formatters_test.cc
  src/binary/lazy_expression_test.cc
  src/binary/lazy_module_test.cc
  src/binary/lazy_name_section_test.cc
  src/binary/lazy_section_test.cc
  src/binary/lazy_sequence_test.cc
  src/binary/read_block_type_test.cc
  src/binary/read_br_table_immediate_test.cc
  src/binary/read_bytes_test.cc
  src/binary/read_call_indirect_immediate_test.cc
  src/binary/read_code_test.cc
  src/binary/read_constant_expression_test.cc
  src/binary/read_count_test.cc
  src/binary/read_data_segment_test.cc
  src/binary/read_element_segment_test.cc
  src/binary/read_element_type_test.cc
  src/binary/read_export_test.cc
  src/binary/read_external_kind_test.cc
  src/binary/read_f32_test.cc
  src/binary/read_f64_test.cc
  src/binary/read_function_test.cc
  src/binary/read_function_type_test.cc
  src/binary/read_global_test.cc
  src/binary/read_global_type_test.cc
  src/binary/read_import_test.cc
  src/binary/read_indirect_name_assoc_test.cc
  src/binary/read_init_immediate_test.cc
  src/binary/read_instruction_test.cc
  src/binary/read_limits_test.cc
  src/binary/read_locals_test.cc
  src/binary/read_mem_arg_immediate_test.cc
  src/binary/read_memory_test.cc
  src/binary/read_memory_type_test.cc
  src/binary/read_mutability_test.cc
  src/binary/read_name_assoc_test.cc
  src/binary/read_name_subsection_id_test.cc
  src/binary/read_name_subsection_test.cc
  src/binary/read_opcode_test.cc
  src/binary/read_s32_test.cc
  src/binary/read_s64_test.cc
  src/binary/read_section_id_test.cc
  src/binary/read_section_test.cc
  src/binary/read_start_test.cc
  src/binary/read_string_test.cc
  src/binary/read_table_test.cc
  src/binary/read_table_type_test.cc
  src/binary/read_type_entry_test.cc
  src/binary/read_u32_test.cc
  src/binary/read_u8_test.cc
  src/binary/read_value_type_test.cc
  src/binary/read_vector_test.cc
  src/binary/test_utils.cc
)

target_link_libraries(wasp_unittests
  wasplib
  gtest_main
  gtest
)
