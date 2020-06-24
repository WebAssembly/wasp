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

#include "wasp/text/read/context.h"

#include <algorithm>
#include <cassert>
#include <iterator>

#include "wasp/base/macros.h"

namespace wasp {
namespace text {

Context::Context(Errors& errors) : errors{errors} {}

Context::Context(const Features& features, Errors& errors)
    : features{features}, errors{errors} {}

void Context::BeginModule() {
  seen_non_import = false;
  seen_start = false;
  type_names.Reset();
  function_names.Reset();
  table_names.Reset();
  memory_names.Reset();
  global_names.Reset();
  event_names.Reset();
  element_segment_names.Reset();
  data_segment_names.Reset();
  function_type_map.BeginModule();
  BeginFunction();
}

void Context::BeginFunction() {
  local_names.Reset();
  label_names.Reset();
}

void Context::EndBlock() {
  label_names.Pop();
}

auto Context::EndModule() -> TypeEntryList {
  return function_type_map.EndModule();
}

void FunctionTypeMap::BeginModule() {
  list_.clear();
  deferred_list_.clear();
}

void FunctionTypeMap::Define(BoundFunctionType bound_type) {
  list_.push_back(ToFunctionType(bound_type));
}

void FunctionTypeMap::Use(FunctionTypeUse type) {
  if (type.type_use) {
    return;
  }

  if (FindIter(list_, type.type) != list_.end()) {
    return;
  }

  if (FindIter(deferred_list_, type.type) != deferred_list_.end()) {
    return;
  }

  deferred_list_.push_back(type.type);
}

void FunctionTypeMap::Use(OptAt<Var> type_use, BoundFunctionType type) {
  return Use(FunctionTypeUse{type_use, ToFunctionType(type)});
}

auto FunctionTypeMap::EndModule() -> TypeEntryList {
  TypeEntryList type_entries;
  for (auto&& deferred : deferred_list_) {
    // Only add deferred types if they aren't already in the list. This could
    // happen if the type was defined after the use, e.g.:
    //
    // (func (type $foo))
    // (type $foo (func))
    if (!Find(deferred)) {
      list_.push_back(deferred);
      type_entries.push_back(ToTypeEntry(deferred));
    }
  }
  deferred_list_.clear();
  return type_entries;
}

optional<Index> FunctionTypeMap::Find(FunctionType type) {
  auto iter = FindIter(list_, type);
  if (iter == list_.end()) {
    return nullopt;
  }
  return iter - list_.begin();
}

optional<Index> FunctionTypeMap::Find(BoundFunctionType type) {
  return Find(ToFunctionType(type));
}

Index FunctionTypeMap::Size() const {
  return list_.size();
}

optional<FunctionType> FunctionTypeMap::Get(Index index) const {
  if (index >= list_.size()) {
    return nullopt;
  }
  return list_[index];
}

// static
TypeEntry FunctionTypeMap::ToTypeEntry(const FunctionType& unbound_type) {
  BoundValueTypeList bound_params;
  for (auto param : unbound_type.params) {
    bound_params.push_back(BoundValueType{nullopt, param});
  }
  return TypeEntry{nullopt,
                   BoundFunctionType{bound_params, unbound_type.results}};
}

// static
FunctionTypeMap::List::const_iterator FunctionTypeMap::FindIter(
    const List& list,
    const FunctionType& type) {
  return std::find_if(list.begin(), list.end(),
                      [&](const FunctionType& ft) { return IsSame(type, ft); });
}

// static
bool FunctionTypeMap::IsSame(const FunctionType& lhs, const FunctionType& rhs) {
  // Note: FunctionTypes already have an operator==, but that also checks
  // whether the locations are the same too.
  return IsSame(lhs.params, rhs.params) && IsSame(lhs.results, rhs.results);
}

// static
bool FunctionTypeMap::IsSame(const ValueTypeList& lhs,
                             const ValueTypeList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                    [](const At<ValueType>& lhs, const At<ValueType>& rhs) {
                      return lhs.value() == rhs.value();
                    });
}

}  // namespace text
}  // namespace wasp
