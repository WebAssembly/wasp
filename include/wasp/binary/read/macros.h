//
// Copyright 2019 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_MACROS_H_
#define WASP_BINARY_MACROS_H_

#include "wasp/base/format.h"
#include "wasp/base/span.h"
#include "wasp/binary/errors_context_guard.h"

#define WASP_TRY_READ(var, call) \
  auto opt_##var = call;         \
  if (!opt_##var) {              \
    return nullopt;              \
  }                              \
  auto var = *opt_##var /* No semicolon. */

#define WASP_TRY_READ_CONTEXT(var, call, desc)                 \
  ErrorsContextGuard<Errors> guard_##var(errors, *data, desc); \
  WASP_TRY_READ(var, call);                                    \
  guard_##var.PopContext() /* No semicolon. */

#define WASP_TRY_DECODE(out_var, in_var, Type, name)               \
  auto out_var = encoding::Type::Decode(in_var);                   \
  if (!out_var) {                                                  \
    errors.OnError(*data, format("Unknown " name ": {}", in_var)); \
    return nullopt;                                                \
  }

#define WASP_TRY_DECODE_WITH_FEATURES(out_var, in_var, Type, name, features) \
  auto out_var = encoding::Type::Decode(in_var, features);                   \
  if (!out_var) {                                                            \
    errors.OnError(*data, format("Unknown " name ": {}", in_var));           \
    return nullopt;                                                          \
  }

#endif  // WASP_BINARY_MACROS_H_
