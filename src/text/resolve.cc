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

#include "wasp/text/resolve.h"

#include "wasp/base/errors.h"
#include "wasp/base/format.h"
#include "wasp/text/formatters.h"
#include "wasp/text/read/context.h"

namespace wasp {
namespace text {

void Resolve(Context& context, NameMap& name_map, At<Var>& var) {
  if (std::holds_alternative<Index>(var.value())) {
    return;
  }

  auto name = std::get<string_view>(var.value());
  if (!name_map.Has(name)) {
    context.errors.OnError(var.loc(), format("Undefined variable {}", name));
    return;
  }

  // TODO: Check for existence and get value at the same time?
  var = MakeAt(var.loc(), name_map.Get(name));
}

void Resolve(Context& context, NameMap& name_map, OptAt<Var>& var) {
  if (var) {
    Resolve(context, name_map, *var);
  }
}

void Resolve(Context& context, NameMap& name_map, VarList& var_list) {
  for (auto& var : var_list) {
    Resolve(context, name_map, var);
  }
}

void Resolve(Context& context, FunctionTypeUse& function_type_use) {
  Resolve(context, context.type_names, function_type_use.type_use);
  if (function_type_use.type_use) {
    auto type_index = std::get<u32>(function_type_use.type_use->value());
    auto type_opt = context.function_type_map.Get(type_index);
    // It's possible that this type use is invalid, but that's a validation
    // error not a parse/resolve error. We'll only check that the type use and
    // explicit function type match when the type use is valid.
    if (type_opt) {
      if (function_type_use.type->params.size() ||
          function_type_use.type->results.size()) {
        // Explicit params/results, so check that they match.
        if (function_type_use.type != *type_opt) {
          context.errors.OnError(
              function_type_use.type.loc(),
              format("Type use {} does not match explicit type {}",
                     function_type_use.type_use, function_type_use.type));
        }
      } else {
        // No params/results given, so populate them.
        function_type_use.type = *type_opt;
      }
    }
  } else {
    auto index_opt = context.function_type_map.Find(function_type_use.type);
    assert(index_opt.has_value());
    function_type_use.type_use = *index_opt;
  }
}

void Resolve(Context& context,
             OptAt<Var>& type_use,
             At<BoundFunctionType>& type) {
  // Same behavior as FunctionTypeUse above, but handles BoundFunctionTypes
  // (used in function definitions and imports).
  Resolve(context, context.type_names, type_use);
  if (type_use) {
    auto type_index = std::get<u32>(type_use->value());
    auto type_opt = context.function_type_map.Get(type_index);
    // It's possible that this type use is invalid, but that's a validation
    // error not a parse/resolve error. We'll only check that the type use and
    // explicit function type match when the type use is valid.
    if (type_opt) {
      if (type->params.size() || type->results.size()) {
        // Explicit params/results, so check that they match.
        if (type->params != type_opt->params ||
            type->results != type_opt->results) {
          context.errors.OnError(
              type.loc(), format("Type use {} does not match explicit type {}",
                                 type_use, type));
        }
      } else {
        // No params/results given, so populate them.
        std::transform(
            type_opt->params.begin(), type_opt->params.end(),
            std::back_inserter(type->params), [](const At<ValueType>& vt) {
              return MakeAt(vt.loc(), BoundValueType{nullopt, vt.value()});
            });
        type->results = type_opt->results;
      }
    }
  } else {
    auto index_opt = context.function_type_map.Find(type);
    assert(index_opt.has_value());
    type_use = *index_opt;
  }
}

void Resolve(Context& context, BlockImmediate& immediate) {
  if (!immediate.type.IsInlineType()) {
    Resolve(context, immediate.type);
  }
}

void Resolve(Context& context, BrOnExnImmediate& immediate) {
  Resolve(context, context.label_names, immediate.target);
  Resolve(context, context.event_names, immediate.event);
}

void Resolve(Context& context, BrTableImmediate& immediate) {
  Resolve(context, context.label_names, immediate.targets);
  Resolve(context, context.label_names, immediate.default_target);
}

void Resolve(Context& context, CallIndirectImmediate& immediate) {
  Resolve(context, context.table_names, immediate.table);
  Resolve(context, immediate.type);
}

void Resolve(Context& context, NameMap& name_map, CopyImmediate& immediate) {
  Resolve(context, name_map, immediate.dst);
  Resolve(context, name_map, immediate.src);
}

void Resolve(Context& context,
             NameMap& segment_name_map,
             NameMap& dst_name_map,
             InitImmediate& immediate) {
  Resolve(context, segment_name_map, immediate.segment);
  Resolve(context, dst_name_map, immediate.dst);
}

void Resolve(Context& context, Instruction& instruction) {
  switch (instruction.immediate.index()) {
    case 0:  // monostate
      if (instruction.opcode == Opcode::End) {
        context.EndBlock();
      }
      break;

    case 6: {  // Var
      auto& immediate = std::get<At<Var>>(instruction.immediate);
      switch (instruction.opcode) {
        // Function.
        case Opcode::Call:
        case Opcode::ReturnCall:
        case Opcode::RefFunc:
          return Resolve(context, context.function_names, immediate);

        // Table.
        case Opcode::TableFill:
        case Opcode::TableGet:
        case Opcode::TableGrow:
        case Opcode::TableSet:
        case Opcode::TableSize:
          return Resolve(context, context.table_names, immediate);

        // Global.
        case Opcode::GlobalGet:
        case Opcode::GlobalSet:
          return Resolve(context, context.global_names, immediate);

        // Event.
        case Opcode::Throw:
          return Resolve(context, context.event_names, immediate);

        // Element Segment.
        case Opcode::ElemDrop:
          return Resolve(context, context.element_segment_names, immediate);

        // Data Segment.
        case Opcode::MemoryInit:
        case Opcode::DataDrop:
          return Resolve(context, context.data_segment_names, immediate);

        // Label.
        case Opcode::BrIf:
        case Opcode::Br:
          return Resolve(context, context.label_names, immediate);

        // Local.
        case Opcode::LocalGet:
        case Opcode::LocalSet:
        case Opcode::LocalTee:
          return Resolve(context, context.local_names, immediate);

        default:
          break;
      }
      break;
    }

    case 7: { // BlockImmediate
      auto& immediate = std::get<At<BlockImmediate>>(instruction.immediate);
      // TODO: share w/ code in read.cc
      if (immediate->label) {
        context.label_names.ReplaceBound(*immediate->label);
      } else {
        context.label_names.NewUnbound();
      }
      context.label_name_stack.push_back(immediate->label);
      return Resolve(context, *immediate);
    }

    case 8: // BrOnExnImmediate
      return Resolve(
          context,
          std::get<At<BrOnExnImmediate>>(instruction.immediate).value());

    case 9: // BrTableImmediate
      return Resolve(
          context,
          std::get<At<BrTableImmediate>>(instruction.immediate).value());

    case 10: // CallIndirectImmediate
      return Resolve(
          context,
          std::get<At<CallIndirectImmediate>>(instruction.immediate).value());

    case 11: { // CopyImmediate
      auto& immediate = std::get<At<CopyImmediate>>(instruction.immediate);
      if (instruction.opcode == Opcode::TableCopy) {
        return Resolve(context, context.table_names, immediate.value());
      } else {
        assert(instruction.opcode == Opcode::MemoryCopy);
        return Resolve(context, context.memory_names, immediate.value());
      }
    }

    case 12: // InitImmediate
      return Resolve(
          context, context.element_segment_names, context.table_names,
          std::get<At<InitImmediate>>(instruction.immediate).value());

    default:
      break;
  }
}

void Resolve(Context& context, InstructionList& instructions) {
  for (auto& instruction : instructions) {
    Resolve(context, instruction.value());
  }
}

void Resolve(Context& context, FunctionDesc& desc) {
  Resolve(context, desc.type_use, desc.type);
}

void Resolve(Context& context, EventType& event_type) {
  Resolve(context, event_type.type);
}

void Resolve(Context& context, EventDesc& desc) {
  Resolve(context, desc.type.value());
}

void Resolve(Context& context, Import& import) {
  switch (import.desc.index()) {
    case 0: // FunctionDesc.
      return Resolve(context, std::get<FunctionDesc>(import.desc));

    case 4: // EventDesc..
      return Resolve(context, std::get<EventDesc>(import.desc));

    default:
      break;
  }
}

void Resolve(Context& context, Function& function) {
  Resolve(context, function.desc);

  context.BeginFunction();
  for (auto& param : function.desc.type->params) {
    context.local_names.New(param->name);
  }
  for (auto& local : function.locals) {
    context.local_names.New(local->name);
  }

  Resolve(context, function.instructions);
}

void Resolve(Context& context, ElementExpressionList& expression_list) {
  for (auto& expression : expression_list) {
    Resolve(context, expression);
  }
}

void Resolve(Context& context, ElementListWithExpressions& element_list) {
  Resolve(context, element_list.list);
}

void Resolve(Context& context, ElementListWithVars& element_list) {
  switch (element_list.kind) {
    case ExternalKind::Function:
      return Resolve(context, context.function_names, element_list.list);

    default:
      // Other external kinds not currently supported.
      break;
  }
}

void Resolve(Context& context, ElementList& element_list) {
  if (std::holds_alternative<ElementListWithVars>(element_list)) {
    Resolve(context, std::get<ElementListWithVars>(element_list));
  } else {
    Resolve(context, std::get<ElementListWithExpressions>(element_list));
  }
}

void Resolve(Context& context, Table& table) {
  if (table.elements) {
    Resolve(context, *table.elements);
  }
}

void Resolve(Context& context, Global& global) {
  Resolve(context, global.init);
}

void Resolve(Context& context, Export& export_) {
  switch (export_.kind) {
    case ExternalKind::Function:
      return Resolve(context, context.function_names, export_.var);

    case ExternalKind::Table:
      return Resolve(context, context.table_names, export_.var);

    case ExternalKind::Memory:
      return Resolve(context, context.memory_names, export_.var);

    case ExternalKind::Global:
      return Resolve(context, context.global_names, export_.var);

    case ExternalKind::Event:
      return Resolve(context, context.event_names, export_.var);

    default:
      break;
  }
}

void Resolve(Context& context, Start& start) {
  Resolve(context, context.function_names, start.var);
}

void Resolve(Context& context, ElementSegment& segment) {
  Resolve(context, context.table_names, segment.table);
  if (segment.offset) {
    Resolve(context, *segment.offset);
  }
  Resolve(context, segment.elements);
}

void Resolve(Context& context, DataSegment& segment) {
  Resolve(context, context.memory_names, segment.memory);
  if (segment.offset) {
    Resolve(context, *segment.offset);
  }
}

void Resolve(Context& context, Event& event) {
  Resolve(context, event.desc);
}

void Resolve(Context& context, ModuleItem& item) {
  switch (item.index()) {
    case 1:
      return Resolve(context, std::get<Import>(item));

    case 2:
      return Resolve(context, std::get<Function>(item));

    case 3:
      return Resolve(context, std::get<Table>(item));

    case 5:
      return Resolve(context, std::get<Global>(item));

    case 6:
      return Resolve(context, std::get<Export>(item));

    case 7:
      return Resolve(context, std::get<Start>(item));

    case 8:
      return Resolve(context, std::get<ElementSegment>(item));

    case 9:
      return Resolve(context, std::get<DataSegment>(item));

    case 10:
      return Resolve(context, std::get<Event>(item));

    default:
      break;
  }
}

void Resolve(Context& context, Module& module) {
  for (auto& item : module) {
    Resolve(context, item.value());
  }
}

void Resolve(Context& context, ScriptModule& script_module) {
  if (std::holds_alternative<Module>(script_module.module)) {
    Resolve(context, std::get<Module>(script_module.module));
  }
}

}  // namespace text
}  // namespace wasp
