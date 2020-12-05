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

include(CheckCXXSourceCompiles)

set(FILESYSTEM_SOURCE "
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
  argc--;
  argv++;
  for (auto& dir : std::filesystem::recursive_directory_iterator(argv[0])) {
    std::cout << dir.path();
  }
  return 0;
}
")

if (NOT MSVC)
  set(CMAKE_REQUIRED_FLAGS "-std=c++17")
  check_cxx_source_compiles("${FILESYSTEM_SOURCE}" HAS_FILESYSTEM_NO_FLAGS)
  if (NOT HAS_FILESYSTEM_NO_FLAGS)
    set(CMAKE_REQUIRED_LIBRARIES "-lstdc++fs")
    check_cxx_source_compiles("${FILESYSTEM_SOURCE}" HAS_FILESYSTEM_LSTDCPPFS)
    if (HAS_FILESYSTEM_LSTDCPPFS)
      set(filesystem_lib "-lstdc++fs")
    else ()
      message(FATAL_ERROR "std::filesystem not available")
    endif ()
  endif ()
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
  absl::raw_hash_set
  absl::str_format
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
  wasm2wat.cc
  wat2wasm.cc
)

target_compile_options(wasp
  PRIVATE
  ${warning_flags}
)

target_link_libraries(wasp wasp_tool)
