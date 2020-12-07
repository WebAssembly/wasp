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

add_library(libwasp_binary
  ../../include/wasp/binary/encoding.h
  ../../include/wasp/binary/formatters.h
  ../../include/wasp/binary/inc/comdat_symbol_kind.inc
  ../../include/wasp/binary/inc/linking_subsection_id.inc
  ../../include/wasp/binary/inc/name_subsection_id.inc
  ../../include/wasp/binary/inc/relocation_type.inc
  ../../include/wasp/binary/inc/section_id.inc
  ../../include/wasp/binary/inc/symbol_info_kind.inc
  ../../include/wasp/binary/lazy_expression.h
  ../../include/wasp/binary/lazy_module.h
  ../../include/wasp/binary/lazy_module_utils.h
  ../../include/wasp/binary/lazy_module_utils-inl.h
  ../../include/wasp/binary/lazy_section.h
  ../../include/wasp/binary/lazy_sequence.h
  ../../include/wasp/binary/lazy_sequence-inl.h
  ../../include/wasp/binary/linking_section/encoding.h
  ../../include/wasp/binary/linking_section/formatters.h
  ../../include/wasp/binary/linking_section/read.h
  ../../include/wasp/binary/linking_section/sections.h
  ../../include/wasp/binary/linking_section/types.h
  ../../include/wasp/binary/linking_section/write.h
  ../../include/wasp/binary/name_section/encoding.h
  ../../include/wasp/binary/name_section/formatters.h
  ../../include/wasp/binary/name_section/read.h
  ../../include/wasp/binary/name_section/sections.h
  ../../include/wasp/binary/name_section/types.h
  ../../include/wasp/binary/name_section/write.h
  ../../include/wasp/binary/read.h
  ../../include/wasp/binary/read/location_guard.h
  ../../include/wasp/binary/read/macros.h
  ../../include/wasp/binary/read/read_ctx.h
  ../../include/wasp/binary/read/read_var_int.h
  ../../include/wasp/binary/read/read_vector.h
  ../../include/wasp/binary/sections.h
  ../../include/wasp/binary/types.h
  ../../include/wasp/binary/var_int.h
  ../../include/wasp/binary/visitor.h
  ../../include/wasp/binary/write.h

  encoding.cc
  formatters.cc
  lazy_expression.cc
  lazy_module.cc
  lazy_sequence.cc
  linking_section/encoding.cc
  linking_section/formatters.cc
  linking_section/read.cc
  linking_section/sections.cc
  linking_section/types.cc
  name_section/encoding.cc
  name_section/formatters.cc
  name_section/read.cc
  name_section/sections.cc
  name_section/types.cc
  read.cc
  read_ctx.cc
  read_module.cc
  sections.cc
  types.cc
)

target_compile_options(libwasp_binary
  PRIVATE
  ${warning_flags}
)

target_link_libraries(libwasp_binary libwasp_base)
