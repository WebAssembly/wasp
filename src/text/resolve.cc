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
#include "wasp/text/resolve_ctx.h"

#include "wasp/base/concat.h"

namespace wasp::text {

void Define(ResolveCtx& ctx, const OptAt<BindVar>& var, NameMap& name_map) {
  if (var) {
    auto& name = **var;
    if (!name_map.HasSinceLastPush(name)) {
      name_map.NewBound(name);
      return;
    }

    // Use the previous name and treat this object as unbound.
    ctx.errors.OnError(var->loc(),
                       concat("Variable ", name, " is already bound to index ",
                              name_map.Get(name)));
  }

  name_map.NewUnbound();
}

void Define(ResolveCtx& ctx,
            const BoundValueTypeList& bvts,
            NameMap& name_map) {
  for (auto& bvt : bvts) {
    Define(ctx, bvt->name, name_map);
  }
}

void Define(ResolveCtx& ctx, const FieldType& field_type, NameMap& name_map) {
  Define(ctx, field_type.name, name_map);
}

void Define(ResolveCtx& ctx,
            const FieldTypeList& field_type_list,
            NameMap& name_map) {
  for (const auto& field_type : field_type_list) {
    Define(ctx, field_type->name, name_map);
  }
}

void DefineTypes(ResolveCtx& ctx, const DefinedType& defined_type) {
  Index type_index = ctx.type_names.Size();
  Define(ctx, defined_type.name, ctx.type_names);

  if (!defined_type.is_function_type()) {
    auto& name_map = ctx.NewFieldNameMap(type_index);
    if (defined_type.is_struct_type()) {
      Define(ctx, defined_type.struct_type()->fields, name_map);
    } else if (defined_type.is_array_type()) {
      Define(ctx, defined_type.array_type()->field, name_map);
    }
  }
}

void Define(ResolveCtx& ctx, const DefinedType& defined_type) {
  // Resolve the type, in case the params or results have a value type that
  // uses a name, e.g. `(type (func (param (ref $T))))`.
  //
  // Since Resolve() changes its parameter, we make a copy first. The type
  // entry is resolved later below in `Resolve(ResolveCtx&, DefinedType&)`.

  if (defined_type.is_function_type()) {
    BoundFunctionType bft = defined_type.function_type();
    Resolve(ctx, bft);
    ctx.function_type_map.Define(bft);
  } else {
    ctx.function_type_map.SkipIndex();
  }
}

void Define(ResolveCtx& ctx, const FunctionDesc& desc) {
  Define(ctx, desc.name, ctx.function_names);
}

void Define(ResolveCtx& ctx, const TableDesc& desc) {
  Define(ctx, desc.name, ctx.table_names);
}

void Define(ResolveCtx& ctx, const MemoryDesc& desc) {
  Define(ctx, desc.name, ctx.memory_names);
}

void Define(ResolveCtx& ctx, const GlobalDesc& desc) {
  Define(ctx, desc.name, ctx.global_names);
}

void Define(ResolveCtx& ctx, const TagDesc& desc) {
  Define(ctx, desc.name, ctx.tag_names);
}

void Define(ResolveCtx& ctx, const Import& import) {
  switch (import.kind()) {
    case ExternalKind::Function:
      Define(ctx, import.function_desc());
      break;

    case ExternalKind::Table:
      Define(ctx, import.table_desc());
      break;

    case ExternalKind::Memory:
      Define(ctx, import.memory_desc());
      break;

    case ExternalKind::Global:
      Define(ctx, import.global_desc());
      break;

    case ExternalKind::Tag:
      Define(ctx, import.tag_desc());
      break;
  }
}

void Define(ResolveCtx& ctx, const ElementSegment& segment) {
  Define(ctx, segment.name, ctx.element_segment_names);
}

void Define(ResolveCtx& ctx, const DataSegment& segment) {
  Define(ctx, segment.name, ctx.data_segment_names);
}

void DefineTypes(ResolveCtx& ctx, const ModuleItem& item) {
  if (item.is_defined_type()) {
    DefineTypes(ctx, item.defined_type());
  }
}

void Define(ResolveCtx& ctx, const ModuleItem& item) {
  switch (item.kind()) {
    case ModuleItemKind::DefinedType:
      Define(ctx, item.defined_type());
      break;

    case ModuleItemKind::Import:
      Define(ctx, item.import());
      break;

    case ModuleItemKind::Function:
      Define(ctx, item.function()->desc);
      break;

    case ModuleItemKind::Table:
      Define(ctx, item.table()->desc);
      break;

    case ModuleItemKind::Memory:
      Define(ctx, item.memory()->desc);
      break;

    case ModuleItemKind::Global:
      Define(ctx, item.global()->desc);
      break;

    case ModuleItemKind::ElementSegment:
      Define(ctx, item.element_segment());
      break;

    case ModuleItemKind::DataSegment:
      Define(ctx, item.data_segment());
      break;

    case ModuleItemKind::Tag:
      Define(ctx, item.tag()->desc);
      break;

    case ModuleItemKind::Export:
    case ModuleItemKind::Start:
      break;
  }
}

void DefineTypes(ResolveCtx& ctx, const Module& module) {
  for (const auto& item : module) {
    DefineTypes(ctx, item);
  }
}

void Define(ResolveCtx& ctx, const Module& module) {
  for (const auto& item : module) {
    Define(ctx, item);
  }
}

void Resolve(Module& module, Errors& errors) {
  ResolveCtx ctx{errors};
  Resolve(ctx, module);
}

void Resolve(Script& script, Errors& errors) {
  ResolveCtx ctx{errors};
  Resolve(ctx, script);
}

void Resolve(ResolveCtx& ctx, At<Var>& var, NameMap& name_map) {
  if (var->is_index()) {
    return;
  }

  auto name = var->name();
  auto opt_index = name_map.Get(name);
  if (!opt_index) {
    ctx.errors.OnError(var.loc(), concat("Undefined variable ", name));
    return;
  }

  var->desc = *opt_index;
}

void Resolve(ResolveCtx& ctx, OptAt<Var>& var, NameMap& name_map) {
  if (var) {
    Resolve(ctx, *var, name_map);
  }
}

void Resolve(ResolveCtx& ctx, VarList& var_list, NameMap& name_map) {
  for (auto& var : var_list) {
    Resolve(ctx, var, name_map);
  }
}

void Resolve(ResolveCtx& ctx, HeapType& heap_type) {
  if (heap_type.is_var()) {
    Resolve(ctx, heap_type.var(), ctx.type_names);
  }
}

void Resolve(ResolveCtx& ctx, RefType& ref_type) {
  Resolve(ctx, ref_type.heap_type.value());
}

void Resolve(ResolveCtx& ctx, ReferenceType& reference_type) {
  if (reference_type.is_ref()) {
    Resolve(ctx, reference_type.ref().value());
  }
}

void Resolve(ResolveCtx& ctx, Rtt& rtt) {
  Resolve(ctx, rtt.type.value());
}

void Resolve(ResolveCtx& ctx, ValueType& value_type) {
  if (value_type.is_reference_type()) {
    Resolve(ctx, value_type.reference_type().value());
  } else if (value_type.is_rtt()) {
    Resolve(ctx, value_type.rtt().value());
  }
}

void Resolve(ResolveCtx& ctx, ValueTypeList& value_type_list) {
  for (auto& value_type : value_type_list) {
    Resolve(ctx, value_type.value());
  }
}

void Resolve(ResolveCtx& ctx, StorageType& storage_type) {
  if (storage_type.is_value_type()) {
    Resolve(ctx, storage_type.value_type().value());
  }
}

void Resolve(ResolveCtx& ctx, FunctionType& function_type) {
  Resolve(ctx, function_type.params);
  Resolve(ctx, function_type.results);
}

void Resolve(ResolveCtx& ctx, FunctionTypeUse& function_type_use) {
  auto& type_use = function_type_use.type_use;
  auto& type = function_type_use.type;

  Resolve(ctx, type_use, ctx.type_names);
  Resolve(ctx, type.value());

  if (type_use) {
    if (type_use->value().is_index()) {
      auto type_index = type_use->value().index();
      auto type_opt = ctx.function_type_map.Get(type_index);
      bool has_explicit_params_or_results =
          type->params.size() || type->results.size();
      if (type_opt) {
        if (has_explicit_params_or_results) {
          // Explicit params/results, so check that they match.
          if (type != *type_opt) {
            ctx.errors.OnError(type.loc(),
                               concat("Type use ", type_use,
                                      " does not match explicit type ", type));
          }
        } else {
          // No params/results given, so populate them.
          type = *type_opt;
        }
      } else if (has_explicit_params_or_results) {
        // We can't compare the type index to the explicit params/results, so
        // this must be considered a syntax error.
        ctx.errors.OnError(type_use->loc(),
                           concat("Invalid type index ", type_use));
      }
    }
  } else {
    auto index = ctx.function_type_map.Use(type);
    type_use = Var{index};
  }
}

void Resolve(ResolveCtx& ctx, BoundValueType& bound_value_type) {
  Resolve(ctx, bound_value_type.type.value());
}

void Resolve(ResolveCtx& ctx, BoundValueTypeList& bound_value_type_list) {
  for (auto& bound_value_type : bound_value_type_list) {
    Resolve(ctx, bound_value_type.value());
  }
}

void Resolve(ResolveCtx& ctx, BoundFunctionType& bound_function_type) {
  Resolve(ctx, bound_function_type.params);
  Resolve(ctx, bound_function_type.results);
}

// TODO: How to combine this with the function above?
void Resolve(ResolveCtx& ctx,
             OptAt<Var>& type_use,
             At<BoundFunctionType>& type) {
  Resolve(ctx, type_use, ctx.type_names);
  Resolve(ctx, type.value());
  if (type_use) {
    if (type_use->value().is_index()) {
      auto type_index = type_use->value().index();
      auto type_opt = ctx.function_type_map.Get(type_index);
      bool has_explicit_params_or_results =
          type->params.size() || type->results.size();
      if (type_opt) {
        if (has_explicit_params_or_results) {
          // Explicit params/results, so check that they match.
          if (type->params != type_opt->params ||
              type->results != type_opt->results) {
            ctx.errors.OnError(type.loc(),
                               concat("Type use ", type_use,
                                      " does not match explicit type ", type));
          }
        } else {
          // No params/results given, so populate them.
          type = ToBoundFunctionType(*type_opt);
        }
      } else if (has_explicit_params_or_results) {
        // We can't compare the type index to the explicit params/results, so
        // this must be considered a syntax error.
        ctx.errors.OnError(type_use->loc(),
                           concat("Invalid type index ", type_use));
      }
    }
  } else {
    auto index = ctx.function_type_map.Use(type);
    type_use = Var{index};
  }

  Define(ctx, type->params, ctx.local_names);
}

void Resolve(ResolveCtx& ctx, FieldType& field_type) {
  Resolve(ctx, field_type.type.value());
}

void Resolve(ResolveCtx& ctx, FieldTypeList& field_type_list) {
  for (auto& field_type : field_type_list) {
    Resolve(ctx, field_type.value());
  }
}

void Resolve(ResolveCtx& ctx, StructType& struct_type) {
  Resolve(ctx, struct_type.fields);
}

void Resolve(ResolveCtx& ctx, ArrayType& array_type) {
  Resolve(ctx, array_type.field.value());
}

void Resolve(ResolveCtx& ctx, DefinedType& defined_type) {
  if (defined_type.is_function_type()) {
    Resolve(ctx, defined_type.function_type().value());
  } else if (defined_type.is_struct_type()) {
    Resolve(ctx, defined_type.struct_type().value());
  } else {
    assert(defined_type.is_array_type());
    Resolve(ctx, defined_type.array_type().value());
  }
}

void Resolve(ResolveCtx& ctx, BlockImmediate& immediate) {
  Define(ctx, immediate.label, ctx.label_names);
  if (immediate.type.IsInlineType()) {
    // An inline type still may be `(result (ref $T))`, which needs to be
    // resolved.
    Resolve(ctx, immediate.type.type.value());
  } else {
    Resolve(ctx, immediate.type);
  }
}

void Resolve(ResolveCtx& ctx, BrOnCastImmediate& immediate) {
  Resolve(ctx, immediate.target, ctx.label_names);
  Resolve(ctx, immediate.types);
}

void Resolve(ResolveCtx& ctx, BrTableImmediate& immediate) {
  Resolve(ctx, immediate.targets, ctx.label_names);
  Resolve(ctx, immediate.default_target, ctx.label_names);
}

void Resolve(ResolveCtx& ctx, CallIndirectImmediate& immediate) {
  Resolve(ctx, immediate.table, ctx.table_names);
  Resolve(ctx, immediate.type);
}

void Resolve(ResolveCtx& ctx, CopyImmediate& immediate, NameMap& name_map) {
  Resolve(ctx, immediate.dst, name_map);
  Resolve(ctx, immediate.src, name_map);
}

void Resolve(ResolveCtx& ctx, HeapType2Immediate& immediate) {
  Resolve(ctx, immediate.parent.value());
  Resolve(ctx, immediate.child.value());
}

void Resolve(ResolveCtx& ctx,
             InitImmediate& immediate,
             NameMap& segment_name_map,
             NameMap& dst_name_map) {
  Resolve(ctx, immediate.segment, segment_name_map);
  Resolve(ctx, immediate.dst, dst_name_map);
}

void Resolve(ResolveCtx& ctx, LetImmediate& immediate) {
  Resolve(ctx, immediate.block);
  Define(ctx, immediate.locals, ctx.local_names);
  Resolve(ctx, immediate.locals);
}

void Resolve(ResolveCtx& ctx, RttSubImmediate& immediate) {
  Resolve(ctx, immediate.types);
}

void Resolve(ResolveCtx& ctx, StructFieldImmediate& immediate) {
  Resolve(ctx, immediate.struct_, ctx.type_names);
  if (immediate.struct_->is_index()) {
    if (auto* field_name_map =
            ctx.GetFieldNameMap(immediate.struct_->index())) {
      Resolve(ctx, immediate.field, *field_name_map);
    }
  }
}

void Resolve(ResolveCtx& ctx, Instruction& instruction) {
  switch (instruction.kind()) {
    case InstructionKind::None:
      if (instruction.opcode == Opcode::End) {
        ctx.EndBlock();
      }
      break;

    case InstructionKind::Var: {
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
          return Resolve(ctx, immediate, ctx.type_names);

        // Function.
        case Opcode::Call:
        case Opcode::ReturnCall:
        case Opcode::RefFunc:
          return Resolve(ctx, immediate, ctx.function_names);

        // Table.
        case Opcode::TableFill:
        case Opcode::TableGet:
        case Opcode::TableGrow:
        case Opcode::TableSet:
        case Opcode::TableSize:
          return Resolve(ctx, immediate, ctx.table_names);

        // Global.
        case Opcode::GlobalGet:
        case Opcode::GlobalSet:
          return Resolve(ctx, immediate, ctx.global_names);

        // Tag.
        case Opcode::Catch:
        case Opcode::Throw:
          return Resolve(ctx, immediate, ctx.tag_names);

        // Element Segment.
        case Opcode::ElemDrop:
          return Resolve(ctx, immediate, ctx.element_segment_names);

        // Data Segment.
        case Opcode::MemoryInit:
        case Opcode::DataDrop:
          return Resolve(ctx, immediate, ctx.data_segment_names);

        // Label.
        case Opcode::BrIf:
        case Opcode::Br:
        case Opcode::BrOnNull:
          // TODO: Keep if br_on_cast continues to use var immediate instead of
          // BrOnExnImmediate.
#if 1
        case Opcode::BrOnCast:
#endif
        case Opcode::Delegate:
        case Opcode::Rethrow:
          return Resolve(ctx, immediate, ctx.label_names);

        // Local.
        case Opcode::LocalGet:
        case Opcode::LocalSet:
        case Opcode::LocalTee:
          return Resolve(ctx, immediate, ctx.local_names);

        default:
          break;
      }
      break;
    }

    case InstructionKind::Block:
      ctx.BeginBlock(instruction.opcode);
      return Resolve(ctx, instruction.block_immediate().value());

    case InstructionKind::BrTable:
      return Resolve(ctx, instruction.br_table_immediate().value());

    case InstructionKind::CallIndirect:
      return Resolve(ctx, instruction.call_indirect_immediate().value());

    case InstructionKind::Copy: {
      auto& immediate = instruction.copy_immediate();
      if (instruction.opcode == Opcode::TableCopy) {
        return Resolve(ctx, immediate.value(), ctx.table_names);
      } else {
        assert(instruction.opcode == Opcode::MemoryCopy);
        return Resolve(ctx, immediate.value(), ctx.memory_names);
      }
    }

    case InstructionKind::Init: {
      auto& immediate = instruction.init_immediate();
      if (instruction.opcode == Opcode::MemoryInit) {
        return Resolve(ctx, immediate.value(), ctx.data_segment_names,
                       ctx.memory_names);
      } else {
        return Resolve(ctx, immediate.value(), ctx.element_segment_names,
                       ctx.table_names);
      }
    }

    case InstructionKind::Let:
      ctx.BeginBlock(instruction.opcode);
      return Resolve(ctx, instruction.let_immediate().value());

    case InstructionKind::HeapType: {
      auto& immediate = instruction.heap_type_immediate();
      if (immediate->is_var()) {
        return Resolve(ctx, immediate->var(), ctx.type_names);
      }
      break;
    }

    case InstructionKind::Select:
      return Resolve(ctx, instruction.select_immediate().value());

    case InstructionKind::FuncBind:
      return Resolve(ctx, instruction.func_bind_immediate().value());

    case InstructionKind::BrOnCast:
      return Resolve(ctx, instruction.br_on_cast_immediate().value());

    case InstructionKind::HeapType2:
      return Resolve(ctx, instruction.heap_type_2_immediate().value());

    case InstructionKind::RttSub:
      return Resolve(ctx, instruction.rtt_sub_immediate().value());

    case InstructionKind::StructField:
      return Resolve(ctx, instruction.struct_field_immediate().value());

    default:
      break;
  }
}

void Resolve(ResolveCtx& ctx, InstructionList& instructions) {
  for (auto& instruction : instructions) {
    Resolve(ctx, instruction.value());
  }
}

void Resolve(ResolveCtx& ctx, FunctionDesc& desc) {
  Resolve(ctx, desc.type_use, desc.type);
}

void Resolve(ResolveCtx& ctx, TableType& table_type) {
  Resolve(ctx, table_type.elemtype.value());
}

void Resolve(ResolveCtx& ctx, TableDesc& desc) {
  Resolve(ctx, desc.type.value());
}

void Resolve(ResolveCtx& ctx, GlobalType& global_type) {
  Resolve(ctx, global_type.valtype.value());
}

void Resolve(ResolveCtx& ctx, GlobalDesc& desc) {
  Resolve(ctx, desc.type.value());
}

void Resolve(ResolveCtx& ctx, TagType& tag_type) {
  Resolve(ctx, tag_type.type);
}

void Resolve(ResolveCtx& ctx, TagDesc& desc) {
  Resolve(ctx, desc.type.value());
}

void Resolve(ResolveCtx& ctx, Import& import) {
  switch (import.kind()) {
    case ExternalKind::Function:
      return Resolve(ctx, import.function_desc());

    case ExternalKind::Table:
      return Resolve(ctx, import.table_desc());

    case ExternalKind::Global:
      return Resolve(ctx, import.global_desc());

    case ExternalKind::Tag:
      return Resolve(ctx, import.tag_desc());

    default:
      break;
  }
}

void Resolve(ResolveCtx& ctx, Function& function) {
  ctx.BeginFunction();
  Resolve(ctx, function.desc);
  Define(ctx, function.locals, ctx.local_names);
  Resolve(ctx, function.locals);
  Resolve(ctx, function.instructions);
}

void Resolve(ResolveCtx& ctx, ConstantExpression& expression) {
  Resolve(ctx, expression.instructions);
}

void Resolve(ResolveCtx& ctx, ElementExpression& expression) {
  Resolve(ctx, expression.instructions);
}

void Resolve(ResolveCtx& ctx, ElementExpressionList& expression_list) {
  for (auto& expression : expression_list) {
    Resolve(ctx, *expression);
  }
}

void Resolve(ResolveCtx& ctx, ElementListWithExpressions& element_list) {
  Resolve(ctx, element_list.elemtype.value());
  Resolve(ctx, element_list.list);
}

void Resolve(ResolveCtx& ctx, ElementListWithVars& element_list) {
  switch (element_list.kind) {
    case ExternalKind::Function:
      return Resolve(ctx, element_list.list, ctx.function_names);

    default:
      // Other external kinds not currently supported.
      break;
  }
}

void Resolve(ResolveCtx& ctx, ElementList& element_list) {
  if (holds_alternative<ElementListWithVars>(element_list)) {
    Resolve(ctx, get<ElementListWithVars>(element_list));
  } else {
    Resolve(ctx, get<ElementListWithExpressions>(element_list));
  }
}

void Resolve(ResolveCtx& ctx, Table& table) {
  Resolve(ctx, table.desc);
  if (table.elements) {
    Resolve(ctx, *table.elements);
  }
}

void Resolve(ResolveCtx& ctx, Global& global) {
  Resolve(ctx, global.desc);
  if (global.init) {
    Resolve(ctx, global.init->value());
  }
}

void Resolve(ResolveCtx& ctx, Export& export_) {
  switch (export_.kind) {
    case ExternalKind::Function:
      return Resolve(ctx, export_.var, ctx.function_names);

    case ExternalKind::Table:
      return Resolve(ctx, export_.var, ctx.table_names);

    case ExternalKind::Memory:
      return Resolve(ctx, export_.var, ctx.memory_names);

    case ExternalKind::Global:
      return Resolve(ctx, export_.var, ctx.global_names);

    case ExternalKind::Tag:
      return Resolve(ctx, export_.var, ctx.tag_names);

    default:
      break;
  }
}

void Resolve(ResolveCtx& ctx, Start& start) {
  Resolve(ctx, start.var, ctx.function_names);
}

void Resolve(ResolveCtx& ctx, ElementSegment& segment) {
  Resolve(ctx, segment.table, ctx.table_names);
  if (segment.offset) {
    Resolve(ctx, segment.offset->value());
  }
  Resolve(ctx, segment.elements);
}

void Resolve(ResolveCtx& ctx, DataSegment& segment) {
  Resolve(ctx, segment.memory, ctx.memory_names);
  if (segment.offset) {
    Resolve(ctx, segment.offset->value());
  }
}

void Resolve(ResolveCtx& ctx, Tag& tag) {
  Resolve(ctx, tag.desc);
}

void Resolve(ResolveCtx& ctx, ModuleItem& item) {
  switch (item.kind()) {
    case ModuleItemKind::DefinedType:
      return Resolve(ctx, *item.defined_type());

    case ModuleItemKind::Import:
      return Resolve(ctx, *item.import());

    case ModuleItemKind::Function:
      return Resolve(ctx, *item.function());

    case ModuleItemKind::Table:
      return Resolve(ctx, *item.table());

    case ModuleItemKind::Global:
      return Resolve(ctx, *item.global());

    case ModuleItemKind::Export:
      return Resolve(ctx, *item.export_());

    case ModuleItemKind::Start:
      return Resolve(ctx, *item.start());

    case ModuleItemKind::ElementSegment:
      return Resolve(ctx, *item.element_segment());

    case ModuleItemKind::DataSegment:
      return Resolve(ctx, *item.data_segment());

    case ModuleItemKind::Tag:
      return Resolve(ctx, *item.tag());

    default:
      break;
  }
}

void Resolve(ResolveCtx& ctx, Module& module) {
  ctx.BeginModule();
  DefineTypes(ctx, module);
  Define(ctx, module);
  for (auto& item : module) {
    Resolve(ctx, item);
  }
  auto deferred_types = ctx.EndModule();
  for (auto& defined_type : deferred_types) {
    module.push_back(ModuleItem{defined_type});
  }
}

void Resolve(ResolveCtx& ctx, ScriptModule& script_module) {
  if (script_module.has_module()) {
    Resolve(ctx, script_module.module());
  }
}

void Resolve(ResolveCtx& ctx, ModuleAssertion& module_assertion) {
  Resolve(ctx, *module_assertion.module);
}

void Resolve(ResolveCtx& ctx, Assertion& assertion) {
  if (assertion.is_module_assertion()) {
    Resolve(ctx, assertion.module_assertion());
  }
}

void Resolve(ResolveCtx& ctx, Command& command) {
  if (command.is_script_module()) {
    Resolve(ctx, command.script_module());
  } else if (command.is_assertion()) {
    Resolve(ctx, command.assertion());
  }
}

void Resolve(ResolveCtx& ctx, Script& script) {
  for (auto& command : script) {
    Resolve(ctx, *command);
  }
}

}  // namespace wasp::text
