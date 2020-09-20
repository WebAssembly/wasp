#
# Copyright 2018 WebAssembly Community Group participants
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

cmake_minimum_required(VERSION 3.1)
project(wasp)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_TOOLS "Build tools" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
  # TODO: Different flags for other compilers.
  set(warning_flags -Wall -Wextra -Wno-unused-parameter)
else ()
  # /Zc:preprocessor enables C++99 and C11 conformance for preprocessor.
  # -wd5105 disables "warning C5105: macro expansion producing 'defined' has undefined behavior
  set(CMAKE_CXX_FLAGS "/Zc:preprocessor -wd5105 ${CMAKE_CXX_FLAGS}")
  set(warning_flags -W3)
endif ()


add_subdirectory(src/base)
add_subdirectory(src/binary)
add_subdirectory(src/valid)
add_subdirectory(src/text)
add_subdirectory(src/convert)
add_subdirectory(third_party)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()

if (BUILD_TOOLS)
  add_subdirectory(src/tools)
endif ()
