//
// Copyright 2018 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef WASP_BASE_MACROS_H_
#define WASP_BASE_MACROS_H_

#if defined(__GNUC__) || defined(__clang__)

#define WASP_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)

#define WASP_UNREACHABLE() __assume(0)

#else

#error Unknown compiler!

#endif

#define WASP_USE(x) static_cast<void>(x)

#endif  // WASP_BASE_MACROS_H_
