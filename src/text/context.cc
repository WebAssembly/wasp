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
  label_name_stack.clear();
}

void Context::EndBlock() {
  assert(!label_name_stack.empty());
  auto name_opt = label_name_stack.back();
  if (name_opt) {
    label_names.Delete(*name_opt);
  }
  label_name_stack.pop_back();
}

void Context::EndModule() {
  function_type_map.EndModule();
}

NameMap::NameMap(NameMapKind kind) : kind_{kind} {}

void NameMap::Reset() {
  map_.clear();
  next_index_ = 0;
}

void NameMap::NewUnbound() {
  next_index_++;
}

void NameMap::NewBound(BindVar var) {
  assert(!Has(var));
  map_.emplace(var, next_index_++);
}

void NameMap::ReplaceBound(BindVar var) {
  map_.emplace(var, next_index_++);
}

void NameMap::New(OptAt<BindVar> var) {
  if (var) {
    NewBound(*var);
  } else {
    NewUnbound();
  }
}

void NameMap::Delete(BindVar var) {
  map_.erase(var);
}

bool NameMap::Has(BindVar var) const {
  return map_.find(var) != map_.end();
}

Index NameMap::Get(BindVar var) const {
  auto iter = map_.find(var);
  assert(iter != map_.end());
  switch (kind_) {
    case NameMapKind::Forward:
      return iter->second;

    case NameMapKind::Reverse:
      return next_index_ - iter->second - 1;

    default:
      WASP_UNREACHABLE();
  }
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

  if (std::find(list_.begin(), list_.end(), type.type) != list_.end()) {
    return;
  }

  if (std::find(deferred_list_.begin(), deferred_list_.end(), type.type) !=
      deferred_list_.end()) {
    return;
  }

  deferred_list_.push_back(type.type);
}

void FunctionTypeMap::Use(OptAt<Var> type_use, BoundFunctionType type) {
  return Use(FunctionTypeUse{type_use, ToFunctionType(type)});
}

void FunctionTypeMap::EndModule() {
  list_.insert(list_.end(), std::make_move_iterator(deferred_list_.begin()),
               std::make_move_iterator(deferred_list_.end()));
  deferred_list_.clear();
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

FunctionType FunctionTypeMap::ToFunctionType(BoundFunctionType bound_type) {
  FunctionType unbound_type;
  for (auto &param : bound_type.params) {
    unbound_type.params.push_back(param->type);
  }
  for (auto &result : bound_type.results) {
    unbound_type.results.push_back(result);
  }
  return unbound_type;
}

}  // namespace text
}  // namespace wasp
