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

#ifndef WASP_BINARY_LAZY_MODULE_H
#define WASP_BINARY_LAZY_MODULE_H

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace binary {

/// ---
template <typename Errors>
class LazyModule {
 public:
  explicit LazyModule(SpanU8, Errors&);

  SpanU8 data;
  optional<SpanU8> magic;
  optional<SpanU8> version;
  LazySequence<Section, Errors> sections;
};

template <typename Errors>
LazyModule<Errors> ReadModule(SpanU8 data, Errors&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/lazy_module-inl.h"

#endif // WASP_BINARY_LAZY_MODULE_H
