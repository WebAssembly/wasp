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

add_library(libwasp_valid
  ../../include/wasp/valid/disjoint_set.h
  ../../include/wasp/valid/formatters.h
  ../../include/wasp/valid/local_map.h
  ../../include/wasp/valid/match.h
  ../../include/wasp/valid/types.h
  ../../include/wasp/valid/valid_ctx.h
  ../../include/wasp/valid/validate.h
  ../../include/wasp/valid/validate_visitor.h
  ../../include/wasp/valid/stack_type.inc

  disjoint_set.cc
  formatters.cc
  local_map.cc
  match.cc
  types.cc
  valid_ctx.cc
  validate.cc
  validate_instruction.cc
  validate_visitor.cc
)

target_compile_options(libwasp_valid
  PRIVATE
  ${warning_flags}
)

target_link_libraries(libwasp_valid libwasp_binary)
