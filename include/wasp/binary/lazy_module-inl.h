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
#include "wasp/binary/read/read_bytes.h"
#include "wasp/binary/read/read_section.h"

namespace wasp {
namespace binary {

template <typename Errors>
LazyModule<Errors>::LazyModule(SpanU8 data, Errors& errors)
    : data{data},
      magic{ReadBytes(&data, 4, errors)},
      version{ReadBytes(&data, 4, errors)},
      sections{data, errors} {
  const SpanU8 kMagic{encoding::Magic};
  const SpanU8 kVersion{encoding::Version};

  if (magic != kMagic) {
    errors.OnError(
        data, format("Magic mismatch: expected {}, got {}", kMagic, *magic));
  }

  if (version != kVersion) {
    errors.OnError(data, format("Version mismatch: expected {}, got {}",
                                kVersion, *version));
  }
}

template <typename Errors>
LazyModule<Errors> ReadModule(SpanU8 data, Errors& errors) {
  return LazyModule<Errors>{data, errors};
}

}  // namespace binary
}  // namespace wasp
