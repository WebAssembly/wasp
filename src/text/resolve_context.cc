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

#include "wasp/text/resolve_context.h"

#include <algorithm>
#include <cassert>
#include <iterator>

#include "wasp/base/macros.h"

namespace wasp::text {

ResolveContext::ResolveContext(Errors& errors) : errors{errors} {}

void ResolveContext::BeginModule() {
  type_names.Reset();
  field_names.clear();
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

void ResolveContext::BeginFunction() {
  local_names.Reset();
  label_names.Reset();
  blocks.clear();
  BeginBlock(Opcode::Unreachable);  // Begin dummy block for function.
}

void ResolveContext::BeginBlock(Opcode opcode) {
  blocks.push_back(opcode);
  label_names.Push();
  if (opcode == Opcode::Let) {
    local_names.Push();
  }
}

void ResolveContext::EndBlock() {
  assert(!blocks.empty());
  if (blocks.back() == Opcode::Let) {
    local_names.Pop();
  }
  label_names.Pop();
  blocks.pop_back();
}

auto ResolveContext::EndModule() -> DefinedTypeList {
  return function_type_map.EndModule();
}

auto ResolveContext::NewFieldNameMap(Index index) -> NameMap& {
  auto [iter, ok] = field_names.emplace(index, NameMap{});
  assert(ok);
  return iter->second;
}

auto ResolveContext::GetFieldNameMap(Index index) -> NameMap* {
  auto iter = field_names.find(index);
  if (iter == field_names.end()) {
    return nullptr;
  }
  return &iter->second;
}

void FunctionTypeMap::BeginModule() {
  list_.clear();
  deferred_list_.clear();
}

void FunctionTypeMap::Define(BoundFunctionType bound_type) {
  list_.push_back(ToFunctionType(bound_type));
}

void FunctionTypeMap::SkipIndex() {
  list_.push_back(nullopt);
}

Index FunctionTypeMap::Use(FunctionType type) {
  auto iter = FindIter(list_, type);
  if (iter != list_.end()) {
    return static_cast<Index>(iter - list_.begin());
  }

  iter = FindIter(deferred_list_, type);
  if (iter != deferred_list_.end()) {
    return static_cast<Index>(list_.size() + (iter - deferred_list_.begin()));
  }

  deferred_list_.push_back(type);
  return static_cast<Index>(list_.size() + deferred_list_.size() - 1);
}

Index FunctionTypeMap::Use(BoundFunctionType type) {
  return Use(ToFunctionType(type));
}

auto FunctionTypeMap::EndModule() -> DefinedTypeList {
  DefinedTypeList defined_types;
  for (auto&& deferred : deferred_list_) {
    assert(deferred.has_value());
    list_.push_back(*deferred);
    defined_types.push_back(ToDefinedType(*deferred));
  }
  deferred_list_.clear();
  return defined_types;
}

Index FunctionTypeMap::Size() const {
  return static_cast<Index>(list_.size());
}

optional<FunctionType> FunctionTypeMap::Get(Index index) const {
  if (index < list_.size()) {
    return list_[index];
  }

  index -= static_cast<Index>(list_.size());
  if (index < deferred_list_.size()) {
    return deferred_list_[index];
  }
  return nullopt;
}

// static
DefinedType FunctionTypeMap::ToDefinedType(const FunctionType& unbound_type) {
  BoundValueTypeList bound_params;
  for (auto param : unbound_type.params) {
    bound_params.push_back(BoundValueType{nullopt, param});
  }
  return DefinedType{nullopt,
                     BoundFunctionType{bound_params, unbound_type.results}};
}

// static
FunctionTypeMap::List::const_iterator FunctionTypeMap::FindIter(
    const List& list,
    const FunctionType& type) {
  return std::find_if(list.begin(), list.end(),
                      [&](const optional<FunctionType>& ft) {
                        return ft && IsSame(type, *ft);
                      });
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

}  // namespace wasp::text
