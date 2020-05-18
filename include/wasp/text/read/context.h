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
#include "wasp/base/hashmap.h"
#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/text/types.h"

namespace wasp {

class Errors;

namespace text {

enum class NameMapKind {
  Forward,  // The oldest object has the lowest index (e.g. Functions).
  Reverse,  // The most recent object has the lowest index (e.g. Labels).
};

struct NameMap {
  using Map = flat_hash_map<BindVar, Index>;

  explicit NameMap(NameMapKind = NameMapKind::Forward);

  void Reset();
  void NewUnbound();
  void NewBound(BindVar);
  void ReplaceBound(BindVar);
  void New(OptAt<BindVar>);
  void Delete(BindVar);

  bool Has(BindVar) const;
  Index Get(BindVar) const;

 private:
  Map map_;
  Index next_index_ = 0;
  NameMapKind kind_;
};

using LabelNameStack = std::vector<OptAt<BindVar>>;

using TypeEntryList = std::vector<TypeEntry>;

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
// `deferred_set_` set below.
struct FunctionTypeMap {
  using List = std::vector<FunctionType>;

  void BeginModule();
  void Define(BoundFunctionType);
  void Use(FunctionTypeUse);
  void Use(OptAt<Var> type_use, BoundFunctionType);
  // Returns the deferred type entries.
  auto EndModule() -> TypeEntryList;

  optional<Index> Find(FunctionType);
  optional<Index> Find(BoundFunctionType);

  Index Size() const;
  optional<FunctionType> Get(Index) const;

 private:
  static TypeEntry ToTypeEntry(FunctionType);

  List list_;
  List deferred_list_;
};

struct Context {
  explicit Context(Errors&);
  explicit Context(const Features&, Errors&);

  void BeginModule();    // Reset all module-specific context.
  void BeginFunction();  // Reset all function-specific context.
  void EndBlock();
  auto EndModule() -> TypeEntryList;

  Features features;
  Errors& errors;

  // Script context.
  NameMap module_names;

  // Module context.
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
  FunctionTypeMap function_type_map;

  // Function context.
  NameMap local_names;  // Includes params.
  NameMap label_names{NameMapKind::Reverse};
  LabelNameStack label_name_stack;
};

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_READ_CONTEXT_H_
