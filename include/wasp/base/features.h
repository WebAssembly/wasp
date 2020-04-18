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

#ifndef WASP_BASE_FEATURES_H_
#define WASP_BASE_FEATURES_H_

#include "wasp/base/types.h"

namespace wasp {

class Features {
 public:
  using Bits = u64;

  // Enum variables:   0, 1, ... N
  // Names look like:  Features::BulkMemoryIndex
  enum {
#define WASP_V(enum_, variable, flag, default_) enum_##Index,
#include "wasp/base/features.def"
#undef WASP_V
  };

  // Enum variables:   1, 2, 4, 8, ... 2**N
  // Names look like:  Features::BulkMemory
  enum {
#define WASP_V(enum_, variable, flag, default_) \
  enum_ = u64{1} << int(enum_##Index),
#include "wasp/base/features.def"
#undef WASP_V
  };

  explicit Features();
  explicit Features(Bits);

  void EnableAll();

  bool HasFeatures(Features features) const {
    return (bits_ & features.bits_) == features.bits_;
  }

#define WASP_V(enum_, variable, flag, default_)                  \
  bool variable##_enabled() const { return bits_ & enum_; }      \
  void enable_##variable() { set_##variable##_enabled(true); }   \
  void disable_##variable() { set_##variable##_enabled(false); } \
  void set_##variable##_enabled(bool value) {                    \
    if (value) {                                                 \
      bits_ |= enum_;                                            \
    } else {                                                     \
      bits_ &= ~enum_;                                           \
    }                                                            \
    UpdateDependencies();                                        \
  }
#include "wasp/base/features.def"
#undef WASP_V

  friend bool operator==(const Features& lhs, const Features& rhs);
  friend bool operator!=(const Features& lhs, const Features& rhs);

 private:
  void UpdateDependencies();

  Bits bits_ = 0;
};


}  // namespace wasp

#endif  // WASP_BASE_FEATURES_H_
