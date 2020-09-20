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

add_executable(wasp_base_unittests
  enumerate_test.cc
  formatters_test.cc
  hash_test.cc
  str_to_u32_test.cc
  utf8_test.cc
  v128_test.cc

  # TODO: Move to its own executable?
  ../tools/argparser_test.cc
)

target_compile_options(wasp_base_unittests
  PRIVATE
  ${warning_flags}
)

target_link_libraries(wasp_base_unittests
  libwasp_base
  libwasp_test
  gtest_main
)

add_test(
  NAME test_base_unittests
  COMMAND $<TARGET_FILE:wasp_base_unittests>)
