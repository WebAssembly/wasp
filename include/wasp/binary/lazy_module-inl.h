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

#include "wasp/base/formatters.h"
#include "wasp/binary/encoding.h"  // XXX
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_bytes_expected.h"
#include "wasp/binary/read/read_section.h"

namespace wasp {
namespace binary {

namespace {

constexpr SpanU8 kMagicSpan{encoding::Magic};
constexpr SpanU8 kVersionSpan{encoding::Version};

}  // namespace

inline LazyModule::LazyModule(SpanU8 data,
                              const Features& features,
                              Errors& errors)
    : data{data},
      magic{ReadBytesExpected(&data, kMagicSpan, features, errors, "magic")},
      version{
          ReadBytesExpected(&data, kVersionSpan, features, errors, "version")},
      sections{data, features, errors} {}

inline LazyModule ReadModule(SpanU8 data,
                             const Features& features,
                             Errors& errors) {
  return LazyModule{data, features, errors};
}

}  // namespace binary
}  // namespace wasp
