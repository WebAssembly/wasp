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

option(BUILD_TESTS "Build tests" ON)
option(BUILD_TOOLS "Build tools" ON)

# TODO: Start using target-specific definitions for all of these.
set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
add_definitions(-Wall -Wextra -Wno-unused-parameter)
include_directories(
  ${wasp_SOURCE_DIR}
  ${wasp_SOURCE_DIR}/include
  ${wasp_SOURCE_DIR}/include/gdtoa
  ${wasp_SOURCE_DIR}/third_party/fmt/include
  ${wasp_SOURCE_DIR}/third_party/span-lite/include
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

add_library(libwasp_base
  src/base/at.cc
  src/base/features.cc
  src/base/file.cc
  src/base/print_to.cc
  src/base/span.cc
  src/base/str_to_u32.cc
  src/base/utf8.cc
  src/base/v128.cc
  src/base/wasm_types.cc

  third_party/fmt/src/format.cc
)

add_library(libwasp_binary
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
)
target_link_libraries(libwasp_binary libwasp_base)

add_library(libwasp_valid
  src/valid/context.cc
  src/valid/disjoint_set.cc
  src/valid/formatters.cc
  src/valid/local_map.cc
  src/valid/match.cc
  src/valid/print_to.cc
  src/valid/types.cc
  src/valid/validate.cc
  src/valid/validate_instruction.cc
  src/valid/validate_visitor.cc
)
target_link_libraries(libwasp_valid libwasp_binary)

add_library(libwasp_text
  src/text/context.cc
  src/text/desugar.cc
  src/text/lex.cc
  src/text/name_map.cc
  src/text/numeric.cc
  src/text/print_to.cc
  src/text/read.cc
  src/text/read_script.cc
  src/text/resolve.cc
  src/text/resolve_context.cc
  src/text/token.cc
  src/text/types.cc
  ${gdtoa_files}
)
target_link_libraries(libwasp_text libwasp_base)

add_library(libwasp_convert
  src/convert/to_binary.cc
)
target_link_libraries(libwasp_convert libwasp_binary libwasp_text)

add_library(wasplib INTERFACE)
target_link_libraries(wasplib
  INTERFACE
  libwasp_base
  libwasp_binary
  libwasp_valid
  libwasp_text
  libwasp_convert
)

set_source_files_properties(
  ${gdtoa_files}
  PROPERTIES
  COMPILE_FLAGS "-Wno-parentheses -Wno-implicit-fallthrough -Wno-unused-label -Wno-sign-compare"
)

if (BUILD_TESTS)
  add_subdirectory(third_party/gtest)

  add_library(libwasp_test
    src/tools/argparser.cc
    test/test_utils.cc
  )

  add_executable(wasp_base_unittests
    test/base/enumerate_test.cc
    test/base/formatters_test.cc
    test/base/hash_test.cc
    test/base/str_to_u32_test.cc
    test/base/utf8_test.cc
    test/base/v128_test.cc
    # TODO: Move to its own executable?
    test/tools/argparser_test.cc
  )
  target_link_libraries(wasp_base_unittests
    libwasp_base
    libwasp_test
    gmock
    gmock_main
  )

  add_executable(wasp_binary_unittests
    test/binary/constants.cc
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
  )
  target_link_libraries(wasp_binary_unittests
    libwasp_binary
    libwasp_test
    gmock
    gmock_main
  )

  add_executable(wasp_text_unittests
    test/text/constants.cc
    test/text/desugar_test.cc
    test/text/formatters_test.cc
    test/text/lex_test.cc
    test/text/name_map_test.cc
    test/text/numeric_test.cc
    test/text/read_test.cc
    test/text/read_script_test.cc
    test/text/resolve_test.cc
    test/text/token_test.cc
    test/text/types_test.cc
    test/text/write_test.cc
  )
  target_link_libraries(wasp_text_unittests
    libwasp_text
    libwasp_test
    gmock
    gmock_main
  )

  add_executable(wasp_valid_unittests
    test/binary/constants.cc
    test/valid/disjoint_set_test.cc
    test/valid/test_utils.cc
    test/valid/local_map_test.cc
    test/valid/match_test.cc
    test/valid/validate_test.cc
    test/valid/validate_code_test.cc
    test/valid/validate_instruction_test.cc
  )
  target_link_libraries(wasp_valid_unittests
    libwasp_valid
    libwasp_test
    gmock
    gmock_main
  )

  add_executable(wasp_convert_unittests
    test/binary/constants.cc
    test/text/constants.cc
    test/convert/to_binary_test.cc
  )
  target_link_libraries(wasp_convert_unittests
    libwasp_convert
    libwasp_test
    gmock
    gmock_main
  )

  add_custom_target(run_tests
    COMMAND $<TARGET_FILE:wasp_base_unittests>
    COMMAND $<TARGET_FILE:wasp_binary_unittests>
    COMMAND $<TARGET_FILE:wasp_valid_unittests>
    COMMAND $<TARGET_FILE:wasp_text_unittests>
    COMMAND $<TARGET_FILE:wasp_convert_unittests>
  )
endif()

if (BUILD_TOOLS)
# TODO: Link stdc++fs conditionally, only when needed.
  set(filesystem_lib "stdc++fs")

  add_executable(wasp
    src/tools/argparser.cc
    src/tools/binary_errors.cc
    src/tools/callgraph.cc
    src/tools/cfg.cc
    src/tools/dfg.cc
    src/tools/dump.cc
    src/tools/pattern.cc
    src/tools/text_errors.cc
    src/tools/validate.cc
    src/tools/wasp.cc
    src/tools/wat2wasm.cc
  )

  target_link_libraries(wasp
    wasplib
    ${filesystem_lib}
  )
  add_executable(run_spec_tests
    src/tools/argparser.cc
    src/tools/binary_errors.cc
    src/tools/text_errors.cc
    test/run_spec_tests.cc
  )

  target_link_libraries(run_spec_tests
    wasplib
    ${filesystem_lib}
  )
endif ()
