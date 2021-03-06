#
# Copyright 2020 WebAssembly Community Group participants
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

add_library(libwasp_test
  ../src/tools/argparser.cc
  test_utils.cc
)

target_compile_options(libwasp_test
  PRIVATE
  ${warning_flags}
)

target_include_directories(libwasp_test
  PUBLIC
  ${wasp_SOURCE_DIR}
)

target_link_libraries(libwasp_test
  libwasp_base
  absl::raw_hash_set
  absl::str_format
  gtest
)

add_subdirectory(base)
add_subdirectory(binary)
add_subdirectory(text)
add_subdirectory(valid)
add_subdirectory(convert)

if (BUILD_TOOLS)
  add_executable(run_spec_tests
    run_spec_tests.cc
  )

  target_compile_options(run_spec_tests
    PRIVATE
    ${warning_flags}
  )

  target_link_libraries(run_spec_tests wasp_tool)

  add_test(
    NAME test_run_spec_tests
    COMMAND $<TARGET_FILE:run_spec_tests> ${wasp_SOURCE_DIR}/third_party/testsuite)
endif ()
