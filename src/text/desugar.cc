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

using ModuleItemList = std::vector<ModuleItem>;

struct DesugarContext {
  Index function_count = 0;
  Index table_count = 0;
  Index memory_count = 0;
  Index global_count = 0;
  Index event_count = 0;
  ModuleItemList new_items;
};

void ReplaceImportOpt(ModuleItem& item, const OptAt<Import>& import_opt){
  if (import_opt) {
    item = ModuleItem{*import_opt};
  }
}

template <typename T>
void AppendExports(ModuleItemList& items, At<T>& value, Index this_index) {
  for (auto & export_: value->ToExports(this_index)) {
    items.push_back(ModuleItem{std::move(export_)});
  }
  value->exports.clear();
}

void Desugar(Module& module) {
  DesugarContext context;

  for (auto&& item : module) {
    switch (item.kind()) {
      case ModuleItemKind::Import: {
        auto import = item.import();
        switch (import->kind()) {
          case ExternalKind::Function: context.function_count++; break;
          case ExternalKind::Table: context.table_count++; break;
          case ExternalKind::Memory: context.memory_count++; break;
          case ExternalKind::Global: context.global_count++; break;
          case ExternalKind::Event: context.event_count++; break;
        }
        break;
      }

      case ModuleItemKind::Function: {
        auto& function = item.function();
        AppendExports(context.new_items, function, context.function_count);
        ReplaceImportOpt(item, function->ToImport());
        context.function_count++;
        break;
      }

      case ModuleItemKind::Table: {
        auto& table = item.table();
        auto segment_opt = table->ToElementSegment(context.table_count);
        if (segment_opt) {
          context.new_items.push_back(ModuleItem{*segment_opt});
          table->elements = nullopt;
        }
        AppendExports(context.new_items, table, context.table_count);
        ReplaceImportOpt(item, table->ToImport());
        context.table_count++;
        break;
      }

      case ModuleItemKind::Memory: {
        auto& memory = item.memory();
        auto segment_opt = memory->ToDataSegment(context.memory_count);
        if (segment_opt) {
          context.new_items.push_back(ModuleItem{*segment_opt});
          memory->data = nullopt;
        }
        AppendExports(context.new_items, memory, context.memory_count);
        ReplaceImportOpt(item, memory->ToImport());
        context.memory_count++;
        break;
      }

      case ModuleItemKind::Global: {
        auto& global = item.global();
        AppendExports(context.new_items, global, context.global_count);
        ReplaceImportOpt(item, global->ToImport());
        context.global_count++;
        break;
      }

      case ModuleItemKind::Event: {
        auto& event = item.event();
        AppendExports(context.new_items, event, context.event_count);
        ReplaceImportOpt(item, event->ToImport());
        context.event_count++;
        break;
      }

      default:
        break;
    }
  }

  module.insert(module.end(),
                std::make_move_iterator(context.new_items.begin()),
                std::make_move_iterator(context.new_items.end()));
}

}  // namespace text
}  // namespace wasp
