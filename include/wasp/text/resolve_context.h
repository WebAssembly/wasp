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

#ifndef WASP_TEXT_RESOLVE_CONTEXT_H_
#define WASP_TEXT_RESOLVE_CONTEXT_H_

#include <map>
#include <vector>

#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/text/read/name_map.h"
#include "wasp/text/types.h"

namespace wasp {

class Errors;

namespace text {

using DefinedTypeList = std::vector<DefinedType>;

// In all places function types are used, they can be specified with:
//   a type use:    `(type $var)`
//   or explicitly: `(param i32) (result i32)`
//   or both:       `(type $var) (param i32) (result i32)`
//
// If both are given, then the type variable will be looked-up, and checked
// against the explicit type params/results, and an error will be given if
// they don't match.
//
// If the type is given explicitly without a type use, then it will be added
// after all defined function types. It's as if they were added to the end of
// the module, in the order they were used. That's the purpose of the
// `deferred_list_` set below.
class FunctionTypeMap {
 public:
  using List = std::vector<optional<FunctionType>>;

  void BeginModule();
  void Define(BoundFunctionType);
  void SkipIndex();
  Index Use(FunctionType);
  Index Use(BoundFunctionType);
  // Returns the deferred defined types.
  auto EndModule() -> DefinedTypeList;

  Index Size() const;
  optional<FunctionType> Get(Index) const;

 private:
  static DefinedType ToDefinedType(const FunctionType&);
  static List::const_iterator FindIter(const List&, const FunctionType&);
  static bool IsSame(const FunctionType&, const FunctionType&);
  static bool IsSame(const ValueTypeList&, const ValueTypeList&);

  List list_;
  List deferred_list_;
};

struct ResolveContext {
  explicit ResolveContext(Errors&);

  void BeginModule();    // Reset all module-specific context.
  void BeginFunction();  // Reset all function-specific context.
  void BeginBlock(Opcode);
  void EndBlock();
  auto EndModule() -> DefinedTypeList;

  // Used for struct and array field names.
  auto NewFieldNameMap(Index) -> NameMap&;
  auto GetFieldNameMap(Index) -> NameMap*;

  Errors& errors;

  // Script context.
  NameMap module_names;

  // Module context.
  NameMap type_names;
  std::map<Index, NameMap> field_names;
  NameMap function_names;
  NameMap table_names;
  NameMap memory_names;
  NameMap global_names;
  NameMap event_names;
  NameMap element_segment_names;
  NameMap data_segment_names;
  FunctionTypeMap function_type_map;

  // Function context.
  NameMap local_names;  // Includes params.
  NameMap label_names;
  std::vector<Opcode> blocks;
};

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_RESOLVE_CONTEXT_H_
