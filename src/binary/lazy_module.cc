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

#include "wasp/binary/lazy_module.h"

#include "wasp/base/formatters.h"
#include "wasp/binary/encoding.h"  // XXX
#include "wasp/binary/read.h"

namespace wasp::binary {

namespace {

constexpr SpanU8 kMagicSpan{encoding::Magic};
constexpr SpanU8 kVersionSpan{encoding::Version};

}  // namespace

LazyModule::LazyModule(SpanU8 data, const Features& features, Errors& errors)
    : data{data},
      context{features, errors},
      magic{ReadBytesExpected(&data, kMagicSpan, context, "magic")},
      version{ReadBytesExpected(&data, kVersionSpan, context, "version")},
      sections{data, context} {}

LazyModule ReadModule(SpanU8 data, const Features& features, Errors& errors) {
  return LazyModule{data, features, errors};
}

}  // namespace wasp::binary
