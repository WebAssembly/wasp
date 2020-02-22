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

#ifndef WASP_VALID_CONTEXT_H_
#define WASP_VALID_CONTEXT_H_

#include <set>
#include <vector>

#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/types.h"
#include "wasp/valid/types.h"

namespace wasp {
namespace valid {

enum class LabelType {
  Function,
  Block,
  Loop,
  If,
  Else,
  Try,
  Catch,
};

using ValueTypeSpan = span<const binary::ValueType>;

struct Label {
  Label(LabelType,
        StackTypeSpan param_types,
        StackTypeSpan result_types,
        Index type_stack_limit);

  const StackTypes& br_types() const {
    return label_type == LabelType::Loop ? param_types : result_types;
  }

  LabelType label_type;
  StackTypes param_types;
  StackTypes result_types;
  Index type_stack_limit;
  bool unreachable;
};

struct Context {
  Index GetLocalCount() const;
  optional<binary::ValueType> GetLocalType(Index) const;
  bool AppendLocals(Index count, binary::ValueType);
  bool AppendLocals(const binary::ValueTypes&);

  std::vector<binary::TypeEntry> types;
  std::vector<binary::Function> functions;
  std::vector<binary::TableType> tables;
  std::vector<binary::MemoryType> memories;
  std::vector<binary::GlobalType> globals;
  std::vector<binary::EventType> events;
  std::vector<binary::SegmentType> element_segments;
  Index imported_function_count = 0;
  Index imported_global_count = 0;
  optional<Index> declared_data_count;
  Index code_count = 0;
  std::vector<Index> locals_partial_sum;
  binary::ValueTypes locals;
  StackTypes type_stack;
  std::vector<Label> label_stack;
  std::set<string_view> export_names;
};

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_CONTEXT_H_
