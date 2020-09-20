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

add_executable(wasp_valid_unittests
  ../binary/constants.cc
  disjoint_set_test.cc
  test_utils.cc
  local_map_test.cc
  match_test.cc
  validate_test.cc
  validate_code_test.cc
  validate_instruction_test.cc
)

target_compile_options(wasp_valid_unittests
  PRIVATE
  ${warning_flags}
)

target_link_libraries(wasp_valid_unittests
  libwasp_valid
  libwasp_test
  gtest
  gtest_main
)

add_test(
  NAME test_valid_unittests
  COMMAND $<TARGET_FILE:wasp_valid_unittests>)
