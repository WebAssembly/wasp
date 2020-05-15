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

#include "wasp/text/desugar.h"

#include <algorithm>
#include <iterator>

namespace wasp {
namespace text {

template <typename T>
void AppendImport(ModuleItemList& module_items, const At<T>& value) {
  auto import_opt = value->ToImport();
  // Add either the import or the defined item, but not both.
  if (import_opt) {
    module_items.push_back(
        MakeAt(import_opt->loc(), ModuleItem{import_opt->value()}));
  } else {
    module_items.push_back(MakeAt(value.loc(), ModuleItem{*value}));
  }
}

template <typename T>
void AppendExports(ModuleItemList& module_items,
                   const At<T>& value,
                   Index this_index) {
  auto exports = value->ToExports(this_index);
  module_items.insert(module_items.end(),
                      std::make_move_iterator(exports.begin()),
                      std::make_move_iterator(exports.end()));
}

auto Desugar(const At<Function>& value, Index this_index) -> ModuleItemList {
  ModuleItemList result;
  AppendImport(result, value);
  AppendExports(result, value, this_index);
  return result;
}

auto Desugar(const At<Table>& value, Index this_index) -> ModuleItemList {
  ModuleItemList result;
  AppendImport(result, value);
  AppendExports(result, value, this_index);
  auto segment_opt = value->ToElementSegment(this_index);
  if (segment_opt) {
    result.push_back(
        MakeAt(segment_opt->loc(), ModuleItem{segment_opt->value()}));
  }
  return result;
}

auto Desugar(const At<Memory>& value, Index this_index) -> ModuleItemList {
  ModuleItemList result;
  AppendImport(result, value);
  AppendExports(result, value, this_index);
  auto segment_opt = value->ToDataSegment(this_index);
  if (segment_opt) {
    result.push_back(
        MakeAt(segment_opt->loc(), ModuleItem{segment_opt->value()}));
  }
  return result;
}

auto Desugar(const At<Global>& value, Index this_index) -> ModuleItemList {
  ModuleItemList result;
  AppendImport(result, value);
  AppendExports(result, value, this_index);
  return result;
}

auto Desugar(const At<Event>& value, Index this_index) -> ModuleItemList {
  ModuleItemList result;
  AppendImport(result, value);
  AppendExports(result, value, this_index);
  return result;
}

void Desugar(Module& module) {
  Index function_index = 0;
  Index table_index = 0;
  Index memory_index = 0;
  Index global_index = 0;
  Index event_index = 0;

  for (auto&& item : module) {
    switch (item->index()) {
      case 1: { // Import
        auto import = get<Import>(*item);
        switch (import.index()) {
          case 0: // Function
            function_index++;
            break;

          case 1: // Table
            table_index++;
            break;

          case 2: // Memory
            memory_index++;
            break;

          case 3: // Global
            global_index++;
            break;

          case 4: // Event
            event_index++;
            break;
        }
        break;
      }
      case 2: { // Function
      }
      case 3: { // Table
      }
      case 4: { // Memory
      }
      case 5: { // Global
      }
    }
  }
}

}  // namespace text
}  // namespace wasp
