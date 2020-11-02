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

add_library(libwasp_text
  ../../include/wasp/text/desugar.h
  ../../include/wasp/text/formatters.h
  ../../include/wasp/text/numeric-inl.h
  ../../include/wasp/text/numeric.h
  ../../include/wasp/text/read.h
  ../../include/wasp/text/resolve.h
  ../../include/wasp/text/resolve_context.h
  ../../include/wasp/text/types.h
  ../../include/wasp/text/write.h
  ../../include/wasp/text/token_type.inc
  ../../include/wasp/text/read/context.h
  ../../include/wasp/text/read/lex.h
  ../../include/wasp/text/read/location_guard.h
  ../../include/wasp/text/read/macros.h
  ../../include/wasp/text/read/name_map.h
  ../../include/wasp/text/read/token-inl.h
  ../../include/wasp/text/read/token.h
  ../../include/wasp/text/read/tokenizer-inl.h
  ../../include/wasp/text/read/tokenizer.h

  context.cc
  desugar.cc
  formatters.cc
  lex.cc
  name_map.cc
  numeric.cc
  read.cc
  read_script.cc
  resolve.cc
  resolve_context.cc
  token.cc
  types.cc
)

target_compile_options(libwasp_text
  PRIVATE
  ${warning_flags}
)

target_include_directories(libwasp_text
  PRIVATE
  ${wasp_SOURCE_DIR}  # for keywords-inl.h
)

target_link_libraries(libwasp_text libwasp_base libgdtoa)
