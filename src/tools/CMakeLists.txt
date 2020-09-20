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

if (NOT MSVC)
  # TODO: Link stdc++fs conditionally, only when needed.
  set(filesystem_lib "stdc++fs")
endif ()

add_library(wasp_tool
  argparser.h
  binary_errors.h
  text_errors.h

  argparser.cc
  binary_errors.cc
  text_errors.cc
)

target_include_directories(wasp_tool
  PUBLIC
  ${wasp_SOURCE_DIR}
)

target_link_libraries(wasp_tool
  PUBLIC
  libwasp_convert
  libwasp_valid
  libwasp_text
  libwasp_binary
  libwasp_base
  libfmt
  ${filesystem_lib}
)

add_executable(wasp
  callgraph.h
  cfg.h
  dfg.h
  dump.h
  pattern.h
  validate.h
  wat2wasm.h

  callgraph.cc
  cfg.cc
  dfg.cc
  dump.cc
  pattern.cc
  validate.cc
  wasp.cc
  wat2wasm.cc
)

target_compile_options(wasp
  PRIVATE
  ${warning_flags}
)

target_link_libraries(wasp wasp_tool)
