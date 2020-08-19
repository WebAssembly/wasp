//
// Copyright 2019 WebAssembly Community Group participants
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

#include "wasp/valid/validate.h"

#include <cassert>

#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/valid/context.h"
#include "wasp/valid/match.h"

namespace wasp {
namespace valid {

bool BeginTypeSection(Context& context, Index type_count) {
  context.defined_type_count = type_count;
  context.equivalent_types.Reset(type_count);
  return true;
}

bool BeginCode(Context& context, Location loc) {
  Index func_index = context.imported_function_count + context.code_count;
  if (func_index >= context.functions.size()) {
    context.errors->OnError(
        loc, format("Unexpected code index ", func_index,
                    ", function count is ", context.functions.size()));
    return false;
  }
  context.code_count++;
  const binary::Function& function = context.functions[func_index];
  context.type_stack.clear();
  context.label_stack.clear();
  context.locals.Reset();
  // Don't validate the index, should have already been validated at this point.
  if (function.type_index < context.defined_type_count) {
    const binary::TypeEntry& type_entry = context.types[function.type_index];
    context.locals.Append(type_entry.type->param_types);
    context.label_stack.push_back(Label{
        LabelType::Function, ToStackTypeList(type_entry.type->param_types),
        ToStackTypeList(type_entry.type->result_types), 0});
    return true;
  } else {
    // Not valid, but try to continue anyway.
    context.label_stack.push_back(Label{LabelType::Function, {}, {}, 0});
    return false;
  }
}

bool CheckDefaultable(Context& context,
                      const At<binary::ReferenceType>& value,
                      string_view desc) {
  if (!IsDefaultableType(value)) {
    context.errors->OnError(
        value.loc(), format(desc, " must be defaultable, got ", value->type));
    return false;
  }
  return true;
}

bool CheckDefaultable(Context& context,
                      const At<binary::ValueType>& value,
                      string_view desc) {
  if (!IsDefaultableType(value)) {
    context.errors->OnError(
        value.loc(), format(desc, " must be defaultable, got ", value->type));
    return false;
  }
  return true;
}

bool Validate(Context& context, const At<binary::UnpackedExpression>& value) {
  bool valid = true;
  for (auto&& instr : value->instructions) {
    valid &= Validate(context, instr);
  }
  return valid;
}

bool Validate(Context& context, const At<binary::UnpackedCode>& value) {
  bool valid = true;
  valid &= BeginCode(context, value.loc());
  valid &= Validate(context, value->locals, RequireDefaultable::Yes);
  valid &= Validate(context, value->body);
  return valid;
}

bool Validate(Context& context,
              const At<binary::ConstantExpression>& value,
              ConstantExpressionKind kind,
              binary::ValueType expected_type,
              Index max_global_index) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "constant_expression"};
  if (value->instructions.size() != 1) {
    context.errors->OnError(
        value.loc(), "A constant expression must be a single instruction");
    return false;
  }

  bool valid = true;
  auto&& instruction = value->instructions[0];
  optional<binary::ValueType> actual_type;
  switch (instruction->opcode) {
    case Opcode::I32Const:
      actual_type = binary::ValueType::I32_NoLocation();
      break;

    case Opcode::I64Const:
      actual_type = binary::ValueType::I64_NoLocation();
      break;

    case Opcode::F32Const:
      actual_type = binary::ValueType::F32_NoLocation();
      break;

    case Opcode::F64Const:
      actual_type = binary::ValueType::F64_NoLocation();
      break;

    case Opcode::V128Const:
      actual_type = binary::ValueType::V128_NoLocation();
      break;

    case Opcode::GlobalGet: {
      auto index = instruction->index_immediate();
      if (!ValidateIndex(context, index, max_global_index, "global index")) {
        return false;
      }

      const auto& global = context.globals[index];
      actual_type = global.valtype;

      if (context.globals[index].mut == Mutability::Var) {
        context.errors->OnError(
            instruction->index_immediate().loc(),
            "A constant expression cannot contain a mutable global");
        valid = false;
      }
      break;
    }

    case Opcode::RefNull:
      actual_type = ToValueType(instruction->heap_type_immediate());
      break;

    case Opcode::RefFunc: {
      auto index = instruction->index_immediate();
      // ref.func indexes are implicitly declared by referencing them in a
      // constant expression.
      context.declared_functions.insert(index);
      if (!ValidateIndex(context, index, context.functions.size(),
                         "func index")) {
        return false;
      }

      assert(index < context.functions.size());
      auto& function = context.functions[index];
      actual_type = ToValueType(
          binary::RefType{binary::HeapType{function.type_index}, Null::No});
      break;
    }

    default:
      context.errors->OnError(
          instruction.loc(),
          format("Invalid instruction in constant expression: ", instruction));
      return false;
  }

  assert(actual_type.has_value());
  valid &= Validate(context, expected_type, MakeAt(value.loc(), *actual_type));
  return valid;
}

bool Validate(Context& context, const At<binary::DataCount>& value) {
  context.declared_data_count = value->count;
  return true;
}

bool Validate(Context& context, const At<binary::DataSegment>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "data segment"};
  bool valid = true;
  if (value->memory_index) {
    valid &= ValidateIndex(context, *value->memory_index,
                           context.memories.size(), "memory index");
  }
  if (value->offset) {
    valid &=
        Validate(context, *value->offset, ConstantExpressionKind::Other,
                 binary::ValueType::I32_NoLocation(), context.globals.size());
  }
  return valid;
}

bool Validate(Context& context,
              const At<binary::ElementExpression>& value,
              binary::ReferenceType reftype) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "element expression"};
  if (value->instructions.size() != 1) {
    context.errors->OnError(
        value.loc(), "An element expression must be a single instruction");
    return false;
  }

  bool valid = true;
  auto&& instruction = value->instructions[0];
  optional<binary::ReferenceType> actual_type;
  switch (instruction->opcode) {
    case Opcode::RefNull:
      actual_type = binary::ReferenceType::Funcref_NoLocation();
      break;

    case Opcode::RefFunc: {
      actual_type = binary::ReferenceType::Funcref_NoLocation();
      auto index = instruction->index_immediate();
      if (!ValidateIndex(context, index, context.functions.size(),
                         "function index")) {
        valid = false;
      }
      context.declared_functions.insert(index);
      break;
    }

    default:
      context.errors->OnError(
          instruction.loc(),
          format("Invalid instruction in element expression: ", instruction));
      return false;
  }

  assert(actual_type.has_value());
  valid &= Validate(context, reftype, MakeAt(value.loc(), *actual_type));
  return valid;
}

bool Validate(Context& context, const At<binary::ElementSegment>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "element segment"};
  context.element_segments.push_back(value->elemtype());
  bool valid = true;
  if (value->table_index) {
    valid &= ValidateIndex(context, *value->table_index, context.tables.size(),
                           "table index");
  }
  if (value->offset) {
    valid &=
        Validate(context, *value->offset, ConstantExpressionKind::GlobalInit,
                 binary::ValueType::I32_NoLocation(), context.globals.size());
  }
  if (value->has_indexes()) {
    auto&& elements = value->indexes();
    Index max_index;
    switch (elements.kind) {
      case ExternalKind::Function:
        max_index = context.functions.size();
        break;
      case ExternalKind::Table:
        max_index = context.tables.size();
        break;
      case ExternalKind::Memory:
        max_index = context.memories.size();
        break;
      case ExternalKind::Global:
        max_index = context.globals.size();
        break;
      case ExternalKind::Event:
        max_index = context.events.size();
        break;
      default:
        WASP_UNREACHABLE();
    }

    for (auto index : elements.list) {
      valid &= ValidateIndex(context, index, max_index, "index");
      if (elements.kind == ExternalKind::Function) {
        context.declared_functions.insert(index);
      }
    }
  } else if (value->has_expressions()) {
    auto&& elements = value->expressions();

    valid &= Validate(context, elements.elemtype);
    for (const auto& expr : elements.list) {
      valid &= Validate(context, expr, elements.elemtype);
    }
  }
  return valid;
}

bool Validate(Context& context, const At<binary::Export>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "export"};
  bool valid = true;

  if (context.export_names.find(value->name) != context.export_names.end()) {
    context.errors->OnError(value.loc(),
                            format("Duplicate export name ", value->name));
    valid = false;
  }
  context.export_names.insert(value->name);

  switch (value->kind) {
    case ExternalKind::Function:
      valid &= ValidateIndex(context, value->index, context.functions.size(),
                             "function index");
      context.declared_functions.insert(value->index);
      break;

    case ExternalKind::Table:
      valid &= ValidateIndex(context, value->index, context.tables.size(),
                             "table index");
      break;

    case ExternalKind::Memory:
      valid &= ValidateIndex(context, value->index, context.memories.size(),
                             "memory index");
      break;

    case ExternalKind::Global:
      if (ValidateIndex(context, value->index, context.globals.size(),
                        "global index")) {
        const auto& global = context.globals[value->index];
        if (global.mut == Mutability::Var &&
            !context.features.mutable_globals_enabled()) {
          context.errors->OnError(value->index.loc(),
                                  "Mutable globals cannot be exported");
          valid = false;
        }
      } else {
        valid = false;
      }
      break;

    case ExternalKind::Event:
      valid &= ValidateIndex(context, value->index, context.events.size(),
                             "event index");
      break;

    default:
      WASP_UNREACHABLE();
  }
  return valid;
}

bool Validate(Context& context, const At<binary::Event>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "event"};
  return Validate(context, value->event_type);
}

bool Validate(Context& context, const At<binary::EventType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "event type"};
  context.events.push_back(value);
  if (!ValidateIndex(context, value->type_index, context.defined_type_count,
                     "event type index")) {
    return false;
  }

  auto&& entry = context.types[value->type_index];
  if (!entry.type->result_types.empty()) {
    context.errors->OnError(
        value.loc(), format("Expected an empty exception result type, got ",
                            entry.type->result_types));
    return false;
  }
  return true;
}

bool Validate(Context& context, const At<binary::Function>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "function"};
  context.functions.push_back(value);
  return ValidateIndex(context, value->type_index, context.defined_type_count,
                       "function type index");
}

bool Validate(Context& context, const At<binary::FunctionType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "function type"};
  bool valid = true;
  if (value->result_types.size() > 1 &&
      !context.features.multi_value_enabled()) {
    context.errors->OnError(value.loc(),
                            format("Expected result type count of 0 or 1, got ",
                                   value->result_types.size()));
    valid = false;
  }
  valid &= Validate(context, value->param_types);
  valid &= Validate(context, value->result_types);
  return valid;
}

bool Validate(Context& context, const At<binary::Global>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "global"};
  context.globals.push_back(value->global_type);
  bool valid = true;
  valid &= Validate(context, value->global_type);
  // Only imported globals can be used in a global's constant expression.
  valid &= Validate(context, value->init, ConstantExpressionKind::GlobalInit,
                    value->global_type->valtype, context.imported_global_count);
  return valid;
}

bool Validate(Context& context, const At<binary::GlobalType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "global type"};
  return Validate(context, value->valtype);
}

bool Validate(Context& context, const At<binary::HeapType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "heap type"};
  if (value->is_index()) {
    return ValidateIndex(context, value->index(), context.defined_type_count,
                         "heap type index");
  }
  return true;
}

bool Validate(Context& context, const At<binary::Import>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "import"};
  bool valid = true;

  switch (value->kind()) {
    case ExternalKind::Function:
      valid &= Validate(context, binary::Function{value->index()});
      context.imported_function_count++;
      break;

    case ExternalKind::Table:
      valid &= Validate(context, binary::Table{value->table_type()});
      break;

    case ExternalKind::Memory:
      valid &= Validate(context, binary::Memory{value->memory_type()});
      break;

    case ExternalKind::Global:
      context.globals.push_back(value->global_type());
      context.imported_global_count++;
      valid &= Validate(context, value->global_type());
      if (value->global_type()->mut == Mutability::Var &&
          !context.features.mutable_globals_enabled()) {
        context.errors->OnError(value->global_type().loc(),
                                "Mutable globals cannot be imported");
        valid = false;
      }
      break;

    case ExternalKind::Event:
      valid &= Validate(context, binary::Event{value->event_type()});
      break;

    default:
      WASP_UNREACHABLE();
      break;
  }
  return valid;
}

bool ValidateIndex(Context& context,
                   const At<Index>& index,
                   Index max,
                   string_view desc) {
  if (index >= max) {
    context.errors->OnError(index.loc(), format("Invalid ", desc, " ", index,
                                                ", must be less than ", max));
    return false;
  }
  return true;
}

bool Validate(Context& context, const At<Limits>& value, Index max) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "limits"};
  bool valid = true;
  if (value->min > max) {
    context.errors->OnError(
        value->min.loc(),
        format("Expected minimum ", value->min, " to be <= ", max));
    valid = false;
  }
  if (value->max.has_value()) {
    if (*value->max > max) {
      context.errors->OnError(
          value->max->loc(),
          format("Expected maximum ", *value->max, " to be <= ", max));
      valid = false;
    }
    if (value->min.value() > value->max->value()) {
      context.errors->OnError(value->min.loc(),
                              format("Expected minimum ", value->min,
                                     " to be <= maximum ", *value->max));
      valid = false;
    }
  }
  return valid;
}

bool Validate(Context& context, const At<binary::Memory>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "memory"};
  context.memories.push_back(value->memory_type);
  bool valid = Validate(context, value->memory_type);
  if (context.memories.size() > 1) {
    context.errors->OnError(value.loc(),
                            "Too many memories, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(Context& context, const At<MemoryType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "memory type"};
  constexpr Index kMaxPages = 65536;
  bool valid = Validate(context, value->limits, kMaxPages);
  if (value->limits->shared == Shared::Yes) {
    if (!context.features.threads_enabled()) {
      context.errors->OnError(value.loc(), "Memories cannot be shared");
      valid = false;
    }

    if (!value->limits->max) {
      context.errors->OnError(value.loc(),
                              "Shared memories must have a maximum");
      valid = false;
    }
  }
  return valid;
}

bool Validate(Context& context,
              binary::ReferenceType expected,
              const At<binary::ReferenceType>& actual) {
  if (!IsMatch(context, actual, expected)) {
    context.errors->OnError(actual.loc(), format("Expected reference type ",
                                                 expected, ", got ", actual));
    return false;
  }
  return true;
}

bool Validate(Context& context, const At<binary::ReferenceType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "reference type"};
  if (value->is_ref()) {
    return Validate(context, value->ref());
  }
  return true;
}

bool Validate(Context& context, const At<binary::RefType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "ref type"};
  return Validate(context, value->heap_type);
}

bool Validate(Context& context, const At<binary::Start>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "start"};
  if (!ValidateIndex(context, value->func_index, context.functions.size(),
                     "function index")) {
    return false;
  }

  bool valid = true;
  auto function = context.functions[value->func_index];
  if (function.type_index < context.defined_type_count) {
    const auto& type_entry = context.types[function.type_index];
    if (type_entry.type->param_types.size() != 0) {
      context.errors->OnError(
          value.loc(), format("Expected start function to have 0 params, got ",
                              type_entry.type->param_types.size()));
      valid = false;
    }

    if (type_entry.type->result_types.size() != 0) {
      context.errors->OnError(
          value.loc(), format("Expected start function to have 0 results, got ",
                              type_entry.type->result_types.size()));
      valid = false;
    }
  }
  return valid;
}

bool Validate(Context& context, const At<binary::Table>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "table"};
  context.tables.push_back(value->table_type);
  bool valid = Validate(context, value->table_type);
  if (context.tables.size() > 1 &&
      !context.features.reference_types_enabled()) {
    context.errors->OnError(value.loc(), "Too many tables, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(Context& context, const At<binary::TableType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "table type"};
  constexpr Index kMaxElements = std::numeric_limits<Index>::max();
  bool valid = Validate(context, value->limits, kMaxElements);
  valid &= Validate(context, value->elemtype);
  valid &= CheckDefaultable(context, value->elemtype, "local type");
  if (value->limits->shared == Shared::Yes) {
    context.errors->OnError(value.loc(), "Tables cannot be shared");
    valid = false;
  }
  return valid;
}

bool Validate(Context& context, const At<binary::TypeEntry>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "type entry"};
  context.types.push_back(value);
  return Validate(context, value->type);
}

bool Validate(Context& context, const At<binary::ValueType>& value) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "value type"};
  if (value->is_reference_type()) {
    return Validate(context, value->reference_type());
  }
  return true;
}

bool Validate(Context& context,
              binary::ValueType expected,
              const At<binary::ValueType>& actual) {
  if (!IsMatch(context, expected, actual)) {
    context.errors->OnError(actual.loc(), format("Expected value type ",
                                                 expected, ", got ", actual));
    return false;
  }
  return true;
}

bool Validate(Context& context, const binary::ValueTypeList& values) {
  bool valid = true;
  for (const auto& value : values) {
    valid &= Validate(context, value);
  }
  return valid;
}

template <typename T>
bool ValidateKnownSection(Context& context, const std::vector<T>& values) {
  bool valid = true;
  for (auto& value : values) {
    valid &= Validate(context, value);
  }
  return valid;
}

template <typename T>
bool ValidateKnownSection(Context& context, const optional<T>& value) {
  bool valid = true;
  if (value) {
    valid &= Validate(context, *value);
  }
  return valid;
}

bool Validate(Context& context, const binary::Module& value) {
  bool valid = true;
  valid &= BeginTypeSection(context, value.types.size());
  valid &= ValidateKnownSection(context, value.types);
  valid &= ValidateKnownSection(context, value.imports);
  valid &= ValidateKnownSection(context, value.functions);
  valid &= ValidateKnownSection(context, value.tables);
  valid &= ValidateKnownSection(context, value.memories);
  valid &= ValidateKnownSection(context, value.globals);
  valid &= ValidateKnownSection(context, value.events);
  valid &= ValidateKnownSection(context, value.exports);
  valid &= ValidateKnownSection(context, value.start);
  valid &= ValidateKnownSection(context, value.element_segments);
  valid &= ValidateKnownSection(context, value.data_count);
  valid &= ValidateKnownSection(context, value.codes);
  valid &= ValidateKnownSection(context, value.data_segments);
  return valid;
}

}  // namespace valid
}  // namespace wasp
