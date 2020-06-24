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

#ifndef WASP_TEXT_READ_NAME_MAP_H_
#define WASP_TEXT_READ_NAME_MAP_H_

#include "wasp/base/string_view.h"
#include "wasp/text/types.h"

namespace wasp {
namespace text {

struct NameMap {
  explicit NameMap();

  void Reset();
  void NewUnbound();
  bool NewBound(BindVar);
  bool New(OptAt<BindVar>);
  void Push();
  void Pop();

  bool Has(BindVar) const;
  bool HasSinceLastPush(BindVar) const;
  optional<Index> Get(BindVar) const;

 private:
  optional<size_t> FindInRange(size_t begin, size_t end, BindVar) const;

  std::vector<optional<BindVar>> names_;
  std::vector<size_t> stack_;
};

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_READ_NAME_MAP_H_
