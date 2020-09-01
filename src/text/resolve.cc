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

#include <cassert>

#include "wasp/base/errors.h"
#include "wasp/text/formatters.h"
#include "wasp/text/resolve_context.h"

#include "wasp/base/concat.h"

namespace wasp::text {

void Define(ResolveContext& context,
            const OptAt<BindVar>& var,
            NameMap& name_map) {
  if (var) {
    auto& name = **var;
    if (!name_map.HasSinceLastPush(name)) {
      name_map.NewBound(name);
      return;
    }

    // Use the previous name and treat this object as unbound.
    context.errors.OnError(
        var->loc(), concat("Variable ", name, " is already bound to index ",
                           name_map.Get(name)));
  }

  name_map.NewUnbound();
}

void Define(ResolveContext& context,
            const BoundValueTypeList& bvts,
            NameMap& name_map) {
  for (auto& bvt : bvts) {
    Define(context, bvt->name, name_map);
  }
}

void Define(ResolveContext& context,
            const FieldType& field_type,
            NameMap& name_map) {
  Define(context, field_type.name, name_map);
}

void Define(ResolveContext& context,
            const FieldTypeList& field_type_list,
            NameMap& name_map) {
  for (const auto& field_type : field_type_list) {
    Define(context, field_type->name, name_map);
  }
}

void DefineTypes(ResolveContext& context, const DefinedType& defined_type) {
  Index type_index = context.type_names.Size();
  Define(context, defined_type.name, context.type_names);

  if (!defined_type.is_function_type()) {
    auto& name_map = context.NewFieldNameMap(type_index);
    if (defined_type.is_struct_type()) {
      Define(context, defined_type.struct_type()->fields, name_map);
    } else if (defined_type.is_array_type()) {
      Define(context, defined_type.array_type()->field, name_map);
    }
  }
}

void Define(ResolveContext& context, const DefinedType& defined_type) {
  // Resolve the type, in case the params or results have a value type that
  // uses a name, e.g. `(type (func (param (ref $T))))`.
  //
  // Since Resolve() changes its parameter, we make a copy first. The type
  // entry is resolved later below in `Resolve(ResolveContext&, DefinedType&)`.

  if (defined_type.is_function_type()) {
    BoundFunctionType bft = defined_type.function_type();
    Resolve(context, bft);
    context.function_type_map.Define(bft);
  }
}

void Define(ResolveContext& context, const FunctionDesc& desc) {
  Define(context, desc.name, context.function_names);
}

void Define(ResolveContext& context, const TableDesc& desc) {
  Define(context, desc.name, context.table_names);
}

void Define(ResolveContext& context, const MemoryDesc& desc) {
  Define(context, desc.name, context.memory_names);
}

void Define(ResolveContext& context, const GlobalDesc& desc) {
  Define(context, desc.name, context.global_names);
}

void Define(ResolveContext& context, const EventDesc& desc) {
  Define(context, desc.name, context.event_names);
}

void Define(ResolveContext& context, const Import& import) {
  switch (import.kind()) {
    case ExternalKind::Function:
      Define(context, import.function_desc());
      break;

    case ExternalKind::Table:
      Define(context, import.table_desc());
      break;

    case ExternalKind::Memory:
      Define(context, import.memory_desc());
      break;

    case ExternalKind::Global:
      Define(context, import.global_desc());
      break;

    case ExternalKind::Event:
      Define(context, import.event_desc());
      break;
  }
}

void Define(ResolveContext& context, const ElementSegment& segment) {
  Define(context, segment.name, context.element_segment_names);
}

void Define(ResolveContext& context, const DataSegment& segment) {
  Define(context, segment.name, context.data_segment_names);
}

void DefineTypes(ResolveContext& context, const ModuleItem& item) {
  if (item.is_defined_type()) {
    DefineTypes(context, item.defined_type());
  }
}

void Define(ResolveContext& context, const ModuleItem& item) {
  switch (item.kind()) {
    case ModuleItemKind::DefinedType:
      Define(context, item.defined_type());
      break;

    case ModuleItemKind::Import:
      Define(context, item.import());
      break;

    case ModuleItemKind::Function:
      Define(context, item.function()->desc);
      break;

    case ModuleItemKind::Table:
      Define(context, item.table()->desc);
      break;

    case ModuleItemKind::Memory:
      Define(context, item.memory()->desc);
      break;

    case ModuleItemKind::Global:
      Define(context, item.global()->desc);
      break;

    case ModuleItemKind::ElementSegment:
      Define(context, item.element_segment());
      break;

    case ModuleItemKind::DataSegment:
      Define(context, item.data_segment());
      break;

    case ModuleItemKind::Event:
      Define(context, item.event()->desc);
      break;

    case ModuleItemKind::Export:
    case ModuleItemKind::Start:
      break;
  }
}

void DefineTypes(ResolveContext& context, const Module& module) {
  for (const auto& item : module) {
    DefineTypes(context, item);
  }
}

void Define(ResolveContext& context, const Module& module) {
  for (const auto& item : module) {
    Define(context, item);
  }
}

void Resolve(Module& module, Errors& errors) {
  ResolveContext context{errors};
  Resolve(context, module);
}

void Resolve(Script& script, Errors& errors) {
  ResolveContext context{errors};
  Resolve(context, script);
}

void Resolve(ResolveContext& context, At<Var>& var, NameMap& name_map) {
  if (var->is_index()) {
    return;
  }

  auto name = var->name();
  auto opt_index = name_map.Get(name);
  if (!opt_index) {
    context.errors.OnError(var.loc(), concat("Undefined variable ", name));
    return;
  }

  var->desc = *opt_index;
}

void Resolve(ResolveContext& context, OptAt<Var>& var, NameMap& name_map) {
  if (var) {
    Resolve(context, *var, name_map);
  }
}

void Resolve(ResolveContext& context, VarList& var_list, NameMap& name_map) {
  for (auto& var : var_list) {
    Resolve(context, var, name_map);
  }
}

void Resolve(ResolveContext& context, HeapType& heap_type) {
  if (heap_type.is_var()) {
    Resolve(context, heap_type.var(), context.type_names);
  }
}

void Resolve(ResolveContext& context, RefType& ref_type) {
  Resolve(context, ref_type.heap_type.value());
}

void Resolve(ResolveContext& context, ReferenceType& reference_type) {
  if (reference_type.is_ref()) {
    Resolve(context, reference_type.ref().value());
  }
}

void Resolve(ResolveContext& context, Rtt& rtt) {
  Resolve(context, rtt.type.value());
}

void Resolve(ResolveContext& context, ValueType& value_type) {
  if (value_type.is_reference_type()) {
    Resolve(context, value_type.reference_type().value());
  } else if (value_type.is_rtt()) {
    Resolve(context, value_type.rtt().value());
  }
}

void Resolve(ResolveContext& context, ValueTypeList& value_type_list) {
  for (auto& value_type : value_type_list) {
    Resolve(context, value_type.value());
  }
}

void Resolve(ResolveContext& context, StorageType& storage_type) {
  if (storage_type.is_value_type()) {
    Resolve(context, storage_type.value_type().value());
  }
}

void Resolve(ResolveContext& context, FunctionType& function_type) {
  Resolve(context, function_type.params);
  Resolve(context, function_type.results);
}

void Resolve(ResolveContext& context, FunctionTypeUse& function_type_use) {
  auto& type_use = function_type_use.type_use;
  auto& type = function_type_use.type;

  Resolve(context, type_use, context.type_names);
  Resolve(context, type.value());

  if (type_use) {
    if (type_use->value().is_index()) {
      auto type_index = type_use->value().index();
      auto type_opt = context.function_type_map.Get(type_index);
      // It's possible that this type use is invalid, but that's a validation
      // error not a parse/resolve error. We'll only check that the type use and
      // explicit function type match when the type use is valid.
      if (type_opt) {
        if (type->params.size() || type->results.size()) {
          // Explicit params/results, so check that they match.
          if (type != *type_opt) {
            context.errors.OnError(
                type.loc(), concat("Type use ", type_use,
                                   " does not match explicit type ", type));
          }
        } else {
          // No params/results given, so populate them.
          type = *type_opt;
        }
      }
    }
  } else {
    auto index = context.function_type_map.Use(type);
    type_use = Var{index};
  }
}

void Resolve(ResolveContext& context, BoundValueType& bound_value_type) {
  Resolve(context, bound_value_type.type.value());
}

void Resolve(ResolveContext& context,
             BoundValueTypeList& bound_value_type_list) {
  for (auto& bound_value_type : bound_value_type_list) {
    Resolve(context, bound_value_type.value());
  }
}

void Resolve(ResolveContext& context, BoundFunctionType& bound_function_type) {
  Resolve(context, bound_function_type.params);
  Resolve(context, bound_function_type.results);
}

// TODO: How to combine this with the function above?
void Resolve(ResolveContext& context,
             OptAt<Var>& type_use,
             At<BoundFunctionType>& type) {
  Resolve(context, type_use, context.type_names);
  Resolve(context, type.value());
  if (type_use) {
    if (type_use->value().is_index()) {
      auto type_index = type_use->value().index();
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
                type.loc(), concat("Type use ", type_use,
                                   " does not match explicit type ", type));
          }
        } else {
          // No params/results given, so populate them.
          type = ToBoundFunctionType(*type_opt);
        }
      }
    }
  } else {
    auto index = context.function_type_map.Use(type);
    type_use = Var{index};
  }

  Define(context, type->params, context.local_names);
}

void Resolve(ResolveContext& context, FieldType& field_type) {
  Resolve(context, field_type.type.value());
}

void Resolve(ResolveContext& context, FieldTypeList& field_type_list) {
  for (auto& field_type : field_type_list) {
    Resolve(context, field_type.value());
  }
}

void Resolve(ResolveContext& context, StructType& struct_type) {
  Resolve(context, struct_type.fields);
}

void Resolve(ResolveContext& context, ArrayType& array_type) {
  Resolve(context, array_type.field.value());
}

void Resolve(ResolveContext& context, DefinedType& defined_type) {
  if (defined_type.is_function_type()) {
    Resolve(context, defined_type.function_type().value());
  } else if (defined_type.is_struct_type()) {
    Resolve(context, defined_type.struct_type().value());
  } else {
    assert(defined_type.is_array_type());
    Resolve(context, defined_type.array_type().value());
  }
}

void Resolve(ResolveContext& context, BlockImmediate& immediate) {
  Define(context, immediate.label, context.label_names);
  if (immediate.type.IsInlineType()) {
    // An inline type still may be `(result (ref $T))`, which needs to be
    // resolved.
    Resolve(context, immediate.type.type.value());
  } else {
    Resolve(context, immediate.type);
  }
}

void Resolve(ResolveContext& context, BrOnCastImmediate& immediate) {
  Resolve(context, immediate.target, context.label_names);
  Resolve(context, immediate.types);
}

void Resolve(ResolveContext& context, BrOnExnImmediate& immediate) {
  Resolve(context, immediate.target, context.label_names);
  Resolve(context, immediate.event, context.event_names);
}

void Resolve(ResolveContext& context, BrTableImmediate& immediate) {
  Resolve(context, immediate.targets, context.label_names);
  Resolve(context, immediate.default_target, context.label_names);
}

void Resolve(ResolveContext& context, CallIndirectImmediate& immediate) {
  Resolve(context, immediate.table, context.table_names);
  Resolve(context, immediate.type);
}

void Resolve(ResolveContext& context,
             CopyImmediate& immediate,
             NameMap& name_map) {
  Resolve(context, immediate.dst, name_map);
  Resolve(context, immediate.src, name_map);
}

void Resolve(ResolveContext& context, HeapType2Immediate& immediate) {
  Resolve(context, immediate.parent.value());
  Resolve(context, immediate.child.value());
}

void Resolve(ResolveContext& context,
             InitImmediate& immediate,
             NameMap& segment_name_map,
             NameMap& dst_name_map) {
  Resolve(context, immediate.segment, segment_name_map);
  Resolve(context, immediate.dst, dst_name_map);
}

void Resolve(ResolveContext& context, LetImmediate& immediate) {
  Resolve(context, immediate.block);
  Define(context, immediate.locals, context.local_names);
  Resolve(context, immediate.locals);
}

void Resolve(ResolveContext& context, RttSubImmediate& immediate) {
  Resolve(context, immediate.types);
}

void Resolve(ResolveContext& context, StructFieldImmediate& immediate) {
  Resolve(context, immediate.struct_, context.type_names);
  if (immediate.struct_->is_index()) {
    if (auto* field_name_map =
            context.GetFieldNameMap(immediate.struct_->index())) {
      Resolve(context, immediate.field, *field_name_map);
    }
  }
}

void Resolve(ResolveContext& context, Instruction& instruction) {
  switch (instruction.immediate.index()) {
    case 0:  // monostate
      if (instruction.opcode == Opcode::End) {
        context.EndBlock();
      }
      break;

    case 6: {  // Var
      auto& immediate = instruction.var_immediate();
      switch (instruction.opcode) {
        // Type.
        case Opcode::ArrayNewWithRtt:
        case Opcode::ArrayNewDefaultWithRtt:
        case Opcode::ArrayGet:
        case Opcode::ArrayGetS:
        case Opcode::ArrayGetU:
        case Opcode::ArraySet:
        case Opcode::ArrayLen:
        case Opcode::StructNewWithRtt:
        case Opcode::StructNewDefaultWithRtt:
          return Resolve(context, immediate, context.type_names);

        // Function.
        case Opcode::Call:
        case Opcode::ReturnCall:
        case Opcode::RefFunc:
          return Resolve(context, immediate, context.function_names);

        // Table.
        case Opcode::TableFill:
        case Opcode::TableGet:
        case Opcode::TableGrow:
        case Opcode::TableSet:
        case Opcode::TableSize:
          return Resolve(context, immediate, context.table_names);

        // Global.
        case Opcode::GlobalGet:
        case Opcode::GlobalSet:
          return Resolve(context, immediate, context.global_names);

        // Event.
        case Opcode::Throw:
          return Resolve(context, immediate, context.event_names);

        // Element Segment.
        case Opcode::ElemDrop:
          return Resolve(context, immediate, context.element_segment_names);

        // Data Segment.
        case Opcode::MemoryInit:
        case Opcode::DataDrop:
          return Resolve(context, immediate, context.data_segment_names);

        // Label.
        case Opcode::BrIf:
        case Opcode::Br:
        case Opcode::BrOnNull:
          return Resolve(context, immediate, context.label_names);

        // Local.
        case Opcode::LocalGet:
        case Opcode::LocalSet:
        case Opcode::LocalTee:
          return Resolve(context, immediate, context.local_names);

        default:
          break;
      }
      break;
    }

    case 7: // BlockImmediate
      context.BeginBlock(instruction.opcode);
      return Resolve(context, instruction.block_immediate().value());

    case 8: // BrOnExnImmediate
      return Resolve(context, instruction.br_on_exn_immediate().value());

    case 9: // BrTableImmediate
      return Resolve(context, instruction.br_table_immediate().value());

    case 10: // CallIndirectImmediate
      return Resolve(context, instruction.call_indirect_immediate().value());

    case 11: { // CopyImmediate
      auto& immediate = instruction.copy_immediate();
      if (instruction.opcode == Opcode::TableCopy) {
        return Resolve(context, immediate.value(), context.table_names);
      } else {
        assert(instruction.opcode == Opcode::MemoryCopy);
        return Resolve(context, immediate.value(), context.memory_names);
      }
    }

    case 12: { // InitImmediate
      auto& immediate = instruction.init_immediate();
      if (instruction.opcode == Opcode::MemoryInit) {
        return Resolve(context, immediate.value(), context.data_segment_names,
                       context.memory_names);
      } else {
        return Resolve(context, immediate.value(),
                       context.element_segment_names, context.table_names);
      }
    }

    case 13: // LetImmediate
      context.BeginBlock(instruction.opcode);
      return Resolve(context, instruction.let_immediate().value());

    case 15: { // HeapType
      auto& immediate = instruction.heap_type_immediate();
      if (immediate->is_var()) {
        return Resolve(context, immediate->var(), context.type_names);
      }
      break;
    }

    case 16: // SelectImmediate
      return Resolve(context, instruction.select_immediate().value());

    case 19: // FuncBindImmediate
      return Resolve(context, instruction.func_bind_immediate().value());

    case 20: // BrOnCastImmediate
      return Resolve(context, instruction.br_on_cast_immediate().value());

    case 21: // HeapType2Immediate
      return Resolve(context, instruction.heap_type_2_immediate().value());

    case 22: // RttSubImmediate
      return Resolve(context, instruction.rtt_sub_immediate().value());

    case 23: // StructFieldImmediate
      return Resolve(context, instruction.struct_field_immediate().value());

    default:
      break;
  }
}

void Resolve(ResolveContext& context, InstructionList& instructions) {
  for (auto& instruction : instructions) {
    Resolve(context, instruction.value());
  }
}

void Resolve(ResolveContext& context, FunctionDesc& desc) {
  Resolve(context, desc.type_use, desc.type);
}

void Resolve(ResolveContext& context, TableType& table_type) {
  Resolve(context, table_type.elemtype.value());
}

void Resolve(ResolveContext& context, TableDesc& desc) {
  Resolve(context, desc.type.value());
}

void Resolve(ResolveContext& context, GlobalType& global_type) {
  Resolve(context, global_type.valtype.value());
}

void Resolve(ResolveContext& context, GlobalDesc& desc) {
  Resolve(context, desc.type.value());
}

void Resolve(ResolveContext& context, EventType& event_type) {
  Resolve(context, event_type.type);
}

void Resolve(ResolveContext& context, EventDesc& desc) {
  Resolve(context, desc.type.value());
}

void Resolve(ResolveContext& context, Import& import) {
  switch (import.kind()) {
    case ExternalKind::Function:
      return Resolve(context, import.function_desc());

    case ExternalKind::Table:
      return Resolve(context, import.table_desc());

    case ExternalKind::Global:
      return Resolve(context, import.global_desc());

    case ExternalKind::Event:
      return Resolve(context, import.event_desc());

    default:
      break;
  }
}

void Resolve(ResolveContext& context, Function& function) {
  context.BeginFunction();
  Resolve(context, function.desc);
  Define(context, function.locals, context.local_names);
  Resolve(context, function.locals);
  Resolve(context, function.instructions);
}

void Resolve(ResolveContext& context, ConstantExpression& expression) {
  Resolve(context, expression.instructions);
}

void Resolve(ResolveContext& context, ElementExpression& expression) {
  Resolve(context, expression.instructions);
}

void Resolve(ResolveContext& context, ElementExpressionList& expression_list) {
  for (auto& expression : expression_list) {
    Resolve(context, *expression);
  }
}

void Resolve(ResolveContext& context,
             ElementListWithExpressions& element_list) {
  Resolve(context, element_list.elemtype.value());
  Resolve(context, element_list.list);
}

void Resolve(ResolveContext& context, ElementListWithVars& element_list) {
  switch (element_list.kind) {
    case ExternalKind::Function:
      return Resolve(context, element_list.list, context.function_names);

    default:
      // Other external kinds not currently supported.
      break;
  }
}

void Resolve(ResolveContext& context, ElementList& element_list) {
  if (holds_alternative<ElementListWithVars>(element_list)) {
    Resolve(context, get<ElementListWithVars>(element_list));
  } else {
    Resolve(context, get<ElementListWithExpressions>(element_list));
  }
}

void Resolve(ResolveContext& context, Table& table) {
  Resolve(context, table.desc);
  if (table.elements) {
    Resolve(context, *table.elements);
  }
}

void Resolve(ResolveContext& context, Global& global) {
  Resolve(context, global.desc);
  if (global.init) {
    Resolve(context, global.init->value());
  }
}

void Resolve(ResolveContext& context, Export& export_) {
  switch (export_.kind) {
    case ExternalKind::Function:
      return Resolve(context, export_.var, context.function_names);

    case ExternalKind::Table:
      return Resolve(context, export_.var, context.table_names);

    case ExternalKind::Memory:
      return Resolve(context, export_.var, context.memory_names);

    case ExternalKind::Global:
      return Resolve(context, export_.var, context.global_names);

    case ExternalKind::Event:
      return Resolve(context, export_.var, context.event_names);

    default:
      break;
  }
}

void Resolve(ResolveContext& context, Start& start) {
  Resolve(context, start.var, context.function_names);
}

void Resolve(ResolveContext& context, ElementSegment& segment) {
  Resolve(context, segment.table, context.table_names);
  if (segment.offset) {
    Resolve(context, segment.offset->value());
  }
  Resolve(context, segment.elements);
}

void Resolve(ResolveContext& context, DataSegment& segment) {
  Resolve(context, segment.memory, context.memory_names);
  if (segment.offset) {
    Resolve(context, segment.offset->value());
  }
}

void Resolve(ResolveContext& context, Event& event) {
  Resolve(context, event.desc);
}

void Resolve(ResolveContext& context, ModuleItem& item) {
  switch (item.kind()) {
    case ModuleItemKind::DefinedType:
      return Resolve(context, *item.defined_type());

    case ModuleItemKind::Import:
      return Resolve(context, *item.import());

    case ModuleItemKind::Function:
      return Resolve(context, *item.function());

    case ModuleItemKind::Table:
      return Resolve(context, *item.table());

    case ModuleItemKind::Global:
      return Resolve(context, *item.global());

    case ModuleItemKind::Export:
      return Resolve(context, *item.export_());

    case ModuleItemKind::Start:
      return Resolve(context, *item.start());

    case ModuleItemKind::ElementSegment:
      return Resolve(context, *item.element_segment());

    case ModuleItemKind::DataSegment:
      return Resolve(context, *item.data_segment());

    case ModuleItemKind::Event:
      return Resolve(context, *item.event());

    default:
      break;
  }
}

void Resolve(ResolveContext& context, Module& module) {
  context.BeginModule();
  DefineTypes(context, module);
  Define(context, module);
  for (auto& item : module) {
    Resolve(context, item);
  }
  auto deferred_types = context.EndModule();
  for (auto& defined_type : deferred_types) {
    module.push_back(ModuleItem{defined_type});
  }
}

void Resolve(ResolveContext& context, ScriptModule& script_module) {
  if (script_module.has_module()) {
    Resolve(context, script_module.module());
  }
}

void Resolve(ResolveContext& context, ModuleAssertion& module_assertion) {
  Resolve(context, *module_assertion.module);
}

void Resolve(ResolveContext& context, Assertion& assertion) {
  if (assertion.is_module_assertion()) {
    Resolve(context, assertion.module_assertion());
  }
}

void Resolve(ResolveContext& context, Command& command) {
  if (command.is_script_module()) {
    Resolve(context, command.script_module());
  } else if (command.is_assertion()) {
    Resolve(context, command.assertion());
  }
}

void Resolve(ResolveContext& context, Script& script) {
  for (auto& command: script) {
    Resolve(context, *command);
  }
}

}  // namespace wasp::text
