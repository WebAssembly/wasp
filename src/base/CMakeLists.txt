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

add_library(libwasp_base
  ../../include/wasp/base/absl_hash_value_macros.h
  ../../include/wasp/base/at.h
  ../../include/wasp/base/bitcast.h
  ../../include/wasp/base/buffer.h
  ../../include/wasp/base/concat.h
  ../../include/wasp/base/enumerate.h
  ../../include/wasp/base/enumerate-inl.h
  ../../include/wasp/base/error.h
  ../../include/wasp/base/errors_context_guard.h
  ../../include/wasp/base/errors.h
  ../../include/wasp/base/errors-inl.h
  ../../include/wasp/base/errors_nop.h
  ../../include/wasp/base/features.h
  ../../include/wasp/base/features.inc
  ../../include/wasp/base/file.h
  ../../include/wasp/base/formatter_macros.h
  ../../include/wasp/base/formatters.h
  ../../include/wasp/base/hash.h
  ../../include/wasp/base/hashmap.h
  ../../include/wasp/base/inc/external_kind.inc
  ../../include/wasp/base/inc/heap_kind.inc
  ../../include/wasp/base/inc/mutability.inc
  ../../include/wasp/base/inc/numeric_type.inc
  ../../include/wasp/base/inc/opcode.inc
  ../../include/wasp/base/inc/packed_type.inc
  ../../include/wasp/base/inc/reference_kind.inc
  ../../include/wasp/base/inc/tag_attribute.inc
  ../../include/wasp/base/macros.h
  ../../include/wasp/base/operator_eq_ne_macros.h
  ../../include/wasp/base/optional.h
  ../../include/wasp/base/span.h
  ../../include/wasp/base/string_view.h
  ../../include/wasp/base/str_to_u32.h
  ../../include/wasp/base/types.h
  ../../include/wasp/base/utf8.h
  ../../include/wasp/base/v128.h
  ../../include/wasp/base/v128-inl.h
  ../../include/wasp/base/variant.h
  ../../include/wasp/base/wasm_types.h

  at.cc
  features.cc
  file.cc
  formatters.cc
  span.cc
  str_to_u32.cc
  utf8.cc
  v128.cc
  wasm_types.cc
)

target_compile_options(libwasp_base
  PRIVATE
  ${warning_flags}
)

target_include_directories(libwasp_base
  PUBLIC
  ${wasp_SOURCE_DIR}/include
)

target_link_libraries(libwasp_base
  absl::base
  absl::container
  absl::hash
)
