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

#include <map>
#include <set>
#include <vector>

#include "wasp/base/errors.h"
#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/types.h"
#include "wasp/valid/disjoint_set.h"
#include "wasp/valid/local_map.h"
#include "wasp/valid/types.h"

namespace wasp::valid {

enum class LabelType {
  Function,
  Block,
  Loop,
  If,
  Else,
  Try,
  Catch,
  Let,
};

struct Label {
  Label(LabelType,
        StackTypeSpan param_types,
        StackTypeSpan result_types,
        Index type_stack_limit);

  const StackTypeList& br_types() const {
    return label_type == LabelType::Loop ? param_types : result_types;
  }

  LabelType label_type;
  StackTypeList param_types;
  StackTypeList result_types;
  Index type_stack_limit;
  bool unreachable;
};

class SameTypes {
 public:
  void Reset(Index);

  auto Get(Index, Index) -> optional<bool>;
  void Assume(Index, Index);
  void Resolve(Index, Index, bool);

 private:
  void MaybeSwapIndexes(Index&, Index&);

  DisjointSet disjoint_set_;
  std::map<std::pair<Index, Index>, bool> assume_;
};

class MatchTypes {
 public:
  void Reset();

  auto Get(Index, Index) -> optional<bool>;
  void Assume(Index, Index);
  void Resolve(Index, Index, bool);

 private:
  std::map<std::pair<Index, Index>, bool> assume_;
};

struct ValidCtx {
  ValidCtx(Errors&);
  ValidCtx(const Features&, Errors&);
  ValidCtx(const ValidCtx&, Errors&);

  void Reset();

  bool IsStackPolymorphic() const;
  bool IsFunctionType(Index) const;
  bool IsStructType(Index) const;
  bool IsArrayType(Index) const;

  Features features;
  Errors* errors;

  std::vector<binary::DefinedType> types;
  std::vector<binary::Function> functions;
  std::vector<binary::TableType> tables;
  std::vector<MemoryType> memories;
  std::vector<binary::GlobalType> globals;
  std::vector<binary::EventType> events;
  std::vector<binary::ReferenceType> element_segments;
  Index defined_type_count = 0;
  Index imported_function_count = 0;
  Index imported_global_count = 0;
  optional<Index> declared_data_count;
  Index code_count = 0;
  LocalMap locals;
  StackTypeList type_stack;
  std::vector<Label> label_stack;
  std::set<string_view> export_names;
  std::set<Index> declared_functions;

  SameTypes same_types;
  MatchTypes match_types;
};

}  // namespace wasp::valid

#endif  // WASP_VALID_CONTEXT_H_
