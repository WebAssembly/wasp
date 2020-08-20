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

#include "wasp/valid/formatters.h"

#include <cassert>

#include "wasp/base/formatter_macros.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/binary/formatters.h"

namespace wasp::valid {

std::ostream& operator<<(std::ostream& os, const ::wasp::valid::Any& self) {
  return os << "any";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::valid::StackType& self) {
  if (self.is_value_type()) {
    os << self.value_type();
  } else {
    assert(self.is_any());
    os << "any";
  }
  return os;
}

}  // namespace wasp::valid
