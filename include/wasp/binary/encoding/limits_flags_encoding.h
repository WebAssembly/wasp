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

#ifndef WASP_BINARY_LIMITS_FLAGS_ENCODING_H
#define WASP_BINARY_LIMITS_FLAGS_ENCODING_H

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"

namespace wasp {
namespace binary {
namespace encoding {

enum class HasMax { No, Yes };

struct DecodedLimitsFlags {
  HasMax has_max;
  Shared shared;
};

struct LimitsFlags {
  static constexpr u8 NoMax = 0;
  static constexpr u8 HasMax = 1;
  static constexpr u8 HasMaxAndShared = 3;

  static optional<DecodedLimitsFlags> Decode(u8, const Features&);
};

// static
inline optional<DecodedLimitsFlags> LimitsFlags::Decode(
    u8 flags,
    const Features& features) {
  switch (flags) {
    case LimitsFlags::NoMax:
      return {{HasMax::No, Shared::No}};

    case LimitsFlags::HasMax:
      return {{HasMax::Yes, Shared::No}};

    case LimitsFlags::HasMaxAndShared:
      if (features.threads_enabled()) {
        return {{HasMax::Yes, Shared::Yes}};
      } else {
        return nullopt;
      }

    default:
      return nullopt;
  }
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_LIMITS_FLAGS_ENCODING_H
