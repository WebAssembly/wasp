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

namespace wasp {
namespace binary {

template <typename Traits>
bool operator==(const Import<Traits>& lhs, const Import<Traits>& rhs) {
  return lhs.module == rhs.module && lhs.name == rhs.name &&
         lhs.desc == rhs.desc;
}

template <typename Traits>
bool operator!=(const Import<Traits>& lhs, const Import<Traits>& rhs) {
  return !(lhs == rhs);
}

template <typename Traits>
bool operator==(const ConstantExpression<Traits>& lhs,
                const ConstantExpression<Traits>& rhs) {
  return lhs.data == rhs.data;
}

template <typename Traits>
bool operator!=(const ConstantExpression<Traits>& lhs,
                const ConstantExpression<Traits>& rhs) {
  return !(lhs == rhs);
}

template <typename Traits>
bool operator==(const Global<Traits>& lhs, const Global<Traits>& rhs) {
  return lhs.global_type == rhs.global_type && lhs.init == rhs.init;
}

template <typename Traits>
bool operator!=(const Global<Traits>& lhs, const Global<Traits>& rhs) {
  return !(lhs == rhs);
}

template <typename Traits>
bool operator==(const Export<Traits>& lhs, const Export<Traits>& rhs) {
  return lhs.kind == rhs.kind && lhs.name == rhs.name && lhs.index == rhs.index;
}

template <typename Traits>
bool operator!=(const Export<Traits>& lhs, const Export<Traits>& rhs) {
  return !(lhs == rhs);
}

}  // namespace binary
}  // namespace wasp
