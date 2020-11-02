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

#include "wasp/base/features.h"

#include "wasp/base/hash.h"

namespace wasp {

Features::Features() {
#define WASP_V(enum_, variable, flag, default_) \
  set_##variable##_enabled(default_);
#include "wasp/base/features.def"
#undef WASP_V
}

Features::Features(Bits bits) : bits_{bits} {
  UpdateDependencies();
}

void Features::EnableAll() {
#define WASP_V(enum_, variable, flag, default_) enable_##variable();
#include "wasp/base/features.def"
#undef WASP_V
}

void Features::UpdateDependencies(){
  if (bits_ & GC) {
    bits_ |= FunctionReferences;
  }
  if (bits_ & (FunctionReferences | Exceptions)) {
    bits_ |= ReferenceTypes;
  }
  if (bits_ & ReferenceTypes) {
    bits_ |= BulkMemory;
  }
}

bool operator==(const Features& lhs, const Features& rhs) {
  return lhs.bits_ == rhs.bits_;
}

bool operator!=(const Features& lhs, const Features& rhs) {
  return !(lhs == rhs);
}

}  // namespace wasp
