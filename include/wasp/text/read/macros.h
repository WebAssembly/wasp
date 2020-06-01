//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_TEXT_MACROS_H_
#define WASP_TEXT_MACROS_H_

#define WASP_TRY_READ(var, call) \
  auto opt_##var = call;         \
  if (!opt_##var) {              \
    return {};                   \
  }                              \
  auto var = *opt_##var /* No semicolon. */

#define WASP_TRY(call) \
  if (!call) {         \
    return {};         \
  }

#endif  // WASP_TEXT_MACROS_H_
