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

#ifndef WASP_TEXT_READ_CONTEXT_H_
#define WASP_TEXT_READ_CONTEXT_H_

#include "wasp/base/features.h"
#include "wasp/base/hash.h"
#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/text/types.h"

namespace wasp {

class Errors;

namespace text {

struct NameMap {
  using Map = flat_hash_map<BindVar, Index>;

  void Reset();
  void NewUnbound();
  void NewBound(BindVar);
  void Delete(BindVar);

  bool Has(BindVar) const;
  Index Get(BindVar) const;

  Map map;
  Index next_index = 0;
};

using LabelNameStack = std::vector<OptAt<BindVar>>;

struct Context {
  explicit Context(Errors&);
  explicit Context(const Features&, Errors&);

  Features features;
  Errors& errors;

  bool seen_non_import = false;
  bool seen_start = false;

  NameMap type_names;
  NameMap function_names;
  NameMap table_names;
  NameMap memory_names;
  NameMap global_names;
  NameMap event_names;
  NameMap element_segment_names;
  NameMap data_segment_names;
  NameMap module_names;
  NameMap local_names;  // Includes params.
  NameMap label_names;
  LabelNameStack label_name_stack;
};

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_READ_CONTEXT_H_
