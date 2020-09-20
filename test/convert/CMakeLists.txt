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

add_executable(wasp_convert_unittests
  ../binary/constants.cc
  ../text/constants.cc
  to_binary_test.cc
)

target_compile_options(wasp_convert_unittests
  PRIVATE
  ${warning_flags}
)

target_link_libraries(wasp_convert_unittests
  libwasp_convert
  libwasp_test
  gtest
  gtest_main
)

add_test(
  NAME test_convert_unittests
  COMMAND $<TARGET_FILE:wasp_convert_unittests>)
