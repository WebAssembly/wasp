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

#ifndef WASP_VALID_LOCAL_MAP_H_
#define WASP_VALID_LOCAL_MAP_H_

#include <utility>
#include <vector>

#include "wasp/base/optional.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace valid {

struct LocalMap {
  explicit LocalMap();

  void Reset();

  auto GetCount() const -> Index;
  auto GetType(Index) const -> optional<binary::ValueType>;
  bool Append(Index count, binary::ValueType);
  bool Append(const binary::ValueTypeList&);

 private:
  bool CanAppend(Index count) const;

  // Index is a partial sum, so the vector can be binary-searched, e.g.
  //
  //   {i32, i32, f32, f32, f32, i64}
  //
  // would be represented as:
  //
  //   {{i32, 2}, {f32, 5}, {i64, 6}}
  using Pair = std::pair<binary::ValueType, Index>;
  std::vector<Pair> types_;
};

}  // namespace valid
}  // namespace wasp

#endif // WASP_VALID_LOCAL_MAP_H_
