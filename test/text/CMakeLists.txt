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

add_executable(wasp_text_unittests
  constants.cc
  desugar_test.cc
  formatters_test.cc
  lex_test.cc
  name_map_test.cc
  numeric_test.cc
  read_test.cc
  read_script_test.cc
  resolve_test.cc
  token_test.cc
  types_test.cc
  write_test.cc
)

target_compile_options(wasp_text_unittests
  PRIVATE
  ${warning_flags}
)

target_link_libraries(wasp_text_unittests
  libwasp_text
  libwasp_test
  gtest_main
)

add_test(
  NAME test_text_unittests
  COMMAND $<TARGET_FILE:wasp_text_unittests>)
