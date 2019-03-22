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

#ifndef WASP_BINARY_READ_H_
#define WASP_BINARY_READ_H_

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"

namespace wasp {
namespace binary {

class Errors;

template <typename T>
struct Tag {};

template <typename T>
optional<T> Read(SpanU8* data,
                 const Features& features,
                 Errors& errors,
                 Tag<T>);

template <typename T>
optional<T> Read(SpanU8* data, const Features& features, Errors& errors) {
  return Read(data, features, errors, Tag<T>{});
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_H_
