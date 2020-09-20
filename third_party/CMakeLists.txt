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

## fmt
add_library(libfmt
  fmt/src/format.cc
)

if (MSVC)
  target_compile_options(libfmt
    PRIVATE
    /Zc:preprocessor-
  )
endif ()

target_include_directories(libfmt
  PUBLIC
  ${wasp_SOURCE_DIR}/third_party/fmt/include
)


## gtest
if (BUILD_TESTS)
  add_subdirectory(gtest)
endif ()


## gdtoa
add_library(libgdtoa
  gdtoa/gdtoa.h
  gdtoa/gdtoaimp.h

  gdtoa/strtorf.c
  gdtoa/strtord.c
  gdtoa/strtodg.c
  gdtoa/dmisc.c
  gdtoa/misc.c
  gdtoa/gmisc.c
  gdtoa/smisc.c
  gdtoa/gethex.c
  gdtoa/sum.c
  gdtoa/hd_init.c
  gdtoa/g_ffmt.c
  gdtoa/g_dfmt.c
  gdtoa/g__fmt.c
  gdtoa/gdtoa.c
)

if (NOT MSVC)
  target_compile_options(libgdtoa
    PRIVATE
    ${warning_flags}
    -Wno-parentheses
    -Wno-implicit-fallthrough
    -Wno-unused-label
    -Wno-sign-compare
  )
endif ()

target_include_directories(libgdtoa
  PUBLIC
  ${wasp_SOURCE_DIR}/include/gdtoa
)
