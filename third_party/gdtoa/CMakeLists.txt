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

add_library(libgdtoa
  arith.h
  gd_qnan.h
  gdtoa.h
  gdtoaimp.h

  dmisc.c
  g_dfmt.c
  gdtoa.c
  gethex.c
  g_ffmt.c
  g__fmt.c
  gmisc.c
  hd_init.c
  misc.c
  smisc.c
  strtodg.c
  strtord.c
  strtorf.c
  sum.c
)

if (MSVC)
  target_compile_options(libgdtoa
    PRIVATE
    ${warning_flags}
    -wd4018  # signed/unsigned mismatch
    -wd4244  # conversion from x to y; possible loss of data
    -wd4554  # check operator predence for possible error
  )
else ()
  target_compile_options(libgdtoa
    PRIVATE
    ${warning_flags}
    -Wno-parentheses
    -Wno-implicit-fallthrough
    -Wno-unused-label
    -Wno-sign-compare
  )
endif ()
