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

#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"

namespace wasp {
namespace valid {

bool Validate(const At<binary::ConstantExpression>& value,
              ValueType expected_type,
              Index max_global_index,
              Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "constant_expression"};
  bool valid = true;
  ValueType actual_type;
  switch (value->instruction->opcode) {
    case Opcode::I32Const:
      actual_type = ValueType::I32;
      break;

    case Opcode::I64Const:
      actual_type = ValueType::I64;
      break;

    case Opcode::F32Const:
      actual_type = ValueType::F32;
      break;

    case Opcode::F64Const:
      actual_type = ValueType::F64;
      break;

    case Opcode::GlobalGet: {
      auto index = value->instruction->index_immediate();
      if (!ValidateIndex(index, max_global_index, "global index", context)) {
        return false;
      }

      const auto& global = context.globals[index];
      actual_type = global.valtype;

      if (context.globals[index].mut == Mutability::Var) {
        context.errors->OnError(
            value->instruction->index_immediate().loc(),
            "A constant expression cannot contain a mutable global");
        valid = false;
      }
      break;
    }

    case Opcode::RefNull:
      actual_type = ValueType::Nullref;
      break;

    case Opcode::RefFunc: {
      auto index = value->instruction->index_immediate();
      if (!ValidateIndex(index, context.functions.size(), "func index",
                         context)) {
        return false;
      }
      actual_type = ValueType::Funcref;
      break;
    }

    default:
      context.errors->OnError(
          value->instruction.loc(),
          format("Invalid instruction in constant expression: {}",
                 value->instruction));
      return false;
  }

  valid &= Validate(actual_type, expected_type, context);
  return valid;
}

bool Validate(const At<binary::DataCount>& value, Context& context) {
  context.declared_data_count = value->count;
  return true;
}

bool Validate(const At<binary::DataSegment>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "data segment"};
  bool valid = true;
  if (value->memory_index) {
    valid &= ValidateIndex(*value->memory_index, context.memories.size(),
                           "memory index", context);
  }
  if (value->offset) {
    valid &= Validate(*value->offset, ValueType::I32, context.globals.size(),
                      context);
  }
  return valid;
}

bool Validate(const At<binary::ElementExpression>& value,
              ReferenceType reftype,
              Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "element expression"};
  bool valid = true;
  ReferenceType actual_type;
  switch (value->instruction->opcode) {
    case Opcode::RefNull:
      actual_type = ReferenceType::Funcref;
      break;

    case Opcode::RefFunc: {
      actual_type = ReferenceType::Funcref;
      auto index = value->instruction->index_immediate();
      if (!ValidateIndex(index, context.functions.size(), "function index",
                         context)) {
        valid = false;
      }
      context.declared_functions.insert(index);
      break;
    }

    default:
      context.errors->OnError(
          value->instruction.loc(),
          format("Invalid instruction in element expression: {}",
                 value->instruction));
      return false;
  }

  valid &= Validate(MakeAt(value.loc(), actual_type), reftype, context);
  return valid;
}

bool Validate(const At<binary::ElementSegment>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "element segment"};
  context.element_segments.push_back(value->elemtype());
  bool valid = true;
  if (value->table_index) {
    valid &= ValidateIndex(*value->table_index, context.tables.size(),
                           "table index", context);
  }
  if (value->offset) {
    valid &= Validate(*value->offset, ValueType::I32, context.globals.size(),
                      context);
  }
  if (value->has_indexes()) {
    auto&& desc = value->indexes();
    Index max_index;
    switch (desc.kind) {
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
    }

    for (auto index : desc.init) {
      valid &= ValidateIndex(index, max_index, "index", context);
      if (desc.kind == ExternalKind::Function) {
        context.declared_functions.insert(index);
      }
    }
  } else if (value->has_expressions()) {
    auto&& desc = value->expressions();

    for (const auto& expr : desc.init) {
      valid &= Validate(expr, desc.elemtype, context);
    }
  }
  return valid;
}

bool Validate(const At<ReferenceType>& actual,
              ReferenceType expected,
              Context& context) {
  if (actual != expected) {
    context.errors->OnError(
        actual.loc(),
        format("Expected element type {}, got {}", expected, actual));
    return false;
  }
  return true;
}

bool Validate(const At<binary::Export>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "export"};
  bool valid = true;

  if (context.export_names.find(value->name) != context.export_names.end()) {
    context.errors->OnError(value.loc(),
                            format("Duplicate export name {}", value->name));
    valid = false;
  }
  context.export_names.insert(value->name);

  switch (value->kind) {
    case ExternalKind::Function:
      valid &= ValidateIndex(value->index, context.functions.size(),
                             "function index", context);
      break;

    case ExternalKind::Table:
      valid &= ValidateIndex(value->index, context.tables.size(), "table index",
                             context);
      break;

    case ExternalKind::Memory:
      valid &= ValidateIndex(value->index, context.memories.size(),
                             "memory index", context);
      break;

    case ExternalKind::Global:
      if (ValidateIndex(value->index, context.globals.size(), "global index",
                        context)) {
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
      valid &= ValidateIndex(value->index, context.events.size(), "event index",
                             context);
      break;

    default:
      WASP_UNREACHABLE();
  }
  return valid;
}

bool Validate(const At<binary::Event>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "event"};
  return Validate(value->event_type, context);
}

bool Validate(const At<binary::EventType>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "event type"};
  context.events.push_back(value);
  if (!ValidateIndex(value->type_index, context.types.size(),
                     "event type index", context)) {
    return false;
  }

  auto&& entry = context.types[value->type_index];
  if (!entry.type->result_types.empty()) {
    context.errors->OnError(
        value.loc(), format("Expected an empty exception result type, got {}",
                            entry.type->result_types));
    return false;
  }
  return true;
}

bool Validate(const At<binary::Function>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "function"};
  context.functions.push_back(value);
  return ValidateIndex(value->type_index, context.types.size(),
                       "function type index", context);
}

bool Validate(const At<binary::FunctionType>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "function type"};
  if (value->result_types.size() > 1 &&
      !context.features.multi_value_enabled()) {
    context.errors->OnError(
        value.loc(), format("Expected result type count of 0 or 1, got {}",
                            value->result_types.size()));
    return false;
  }
  return true;
}

bool Validate(const At<binary::Global>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "global"};
  context.globals.push_back(value->global_type);
  bool valid = true;
  valid &= Validate(value->global_type, context);
  // Only imported globals can be used in a global's constant expression.
  valid &= Validate(value->init, value->global_type->valtype,
                    context.imported_global_count, context);
  // ref.func indexes cannot be validated until after they are declared in the
  // element segment.
  if (value->init->instruction->opcode == Opcode::RefFunc) {
    context.deferred_function_references.push_back(
        value->init->instruction->index_immediate());
  }
  return valid;
}

bool Validate(const At<binary::GlobalType>& value, Context& context) {
  return true;
}

bool Validate(const At<binary::Import>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "import"};
  bool valid = true;

  switch (value->kind()) {
    case ExternalKind::Function:
      valid &= Validate(binary::Function{value->index()}, context);
      context.imported_function_count++;
      break;

    case ExternalKind::Table:
      valid &= Validate(binary::Table{value->table_type()}, context);
      break;

    case ExternalKind::Memory:
      valid &= Validate(binary::Memory{value->memory_type()}, context);
      break;

    case ExternalKind::Global:
      context.globals.push_back(value->global_type());
      context.imported_global_count++;
      valid &= Validate(value->global_type(), context);
      if (value->global_type()->mut == Mutability::Var &&
          !context.features.mutable_globals_enabled()) {
        context.errors->OnError(value->global_type().loc(),
                                "Mutable globals cannot be imported");
        valid = false;
      }
      break;

    case ExternalKind::Event:
      valid &= Validate(binary::Event{value->event_type()}, context);
      break;

    default:
      WASP_UNREACHABLE();
      break;
  }
  return valid;
}

bool ValidateIndex(const At<Index>& index,
                   Index max,
                   string_view desc,
                   Context& context) {
  if (index >= max) {
    context.errors->OnError(
        index.loc(),
        format("Invalid {} {}, must be less than {}", desc, index, max));
    return false;
  }
  return true;
}

bool Validate(const At<Limits>& value, Index max, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "limits"};
  bool valid = true;
  if (value->min > max) {
    context.errors->OnError(
        value->min.loc(),
        format("Expected minimum {} to be <= {}", value->min, max));
    valid = false;
  }
  if (value->max.has_value()) {
    if (*value->max > max) {
      context.errors->OnError(
          value->max->loc(),
          format("Expected maximum {} to be <= {}", *value->max, max));
      valid = false;
    }
    if (value->min > *value->max) {
      context.errors->OnError(value->min.loc(),
                              format("Expected minimum {} to be <= maximum {}",
                                     value->min, *value->max));
      valid = false;
    }
  }
  return valid;
}

bool Validate(const At<binary::Memory>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "memory"};
  context.memories.push_back(value->memory_type);
  bool valid = Validate(value->memory_type, context);
  if (context.memories.size() > 1) {
    context.errors->OnError(value.loc(),
                            "Too many memories, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(const At<binary::MemoryType>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "memory type"};
  constexpr Index kMaxPages = 65536;
  bool valid = Validate(value->limits, kMaxPages, context);
  if (value->limits->shared == Shared::Yes &&
      !context.features.threads_enabled()) {
    context.errors->OnError(value.loc(), "Memories cannot be shared");
    valid = false;
  }
  return valid;
}

bool Validate(const At<binary::Start>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "start"};
  if (!ValidateIndex(value->func_index, context.functions.size(),
                     "function index", context)) {
    return false;
  }

  bool valid = true;
  auto function = context.functions[value->func_index];
  if (function.type_index < context.types.size()) {
    const auto& type_entry = context.types[function.type_index];
    if (type_entry.type->param_types.size() != 0) {
      context.errors->OnError(
          value.loc(),
          format("Expected start function to have 0 params, got {}",
                 type_entry.type->param_types.size()));
      valid = false;
    }

    if (type_entry.type->result_types.size() != 0) {
      context.errors->OnError(
          value.loc(),
          format("Expected start function to have 0 results, got {}",
                 type_entry.type->result_types.size()));
      valid = false;
    }
  }
  return valid;
}

bool Validate(const At<binary::Table>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "table"};
  context.tables.push_back(value->table_type);
  bool valid = Validate(value->table_type, context);
  if (context.tables.size() > 1 &&
      !context.features.reference_types_enabled()) {
    context.errors->OnError(value.loc(), "Too many tables, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(const At<binary::TableType>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "table type"};
  constexpr Index kMaxElements = std::numeric_limits<Index>::max();
  bool valid = Validate(value->limits, kMaxElements, context);
  if (value->limits->shared == Shared::Yes) {
    context.errors->OnError(value.loc(), "Tables cannot be shared");
    valid = false;
  }
  return valid;
}

bool Validate(const At<binary::TypeEntry>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "type entry"};
  context.types.push_back(value);
  return Validate(value->type, context);
}

namespace {

bool IsReferenceType(ValueType type) {
  return type == ValueType::Anyref || type == ValueType::Funcref ||
         type == ValueType::Nullref;
}

// TODO: Almost the same as in TypesMatch (minus StackType::Any); how to
// share?
bool TypesMatch(ValueType expected, ValueType actual) {
  // Types are the same.
  if (expected == actual) {
    return true;
  }

  // Anyref is a super type of all reference types.
  if (expected == ValueType::Anyref && IsReferenceType(actual)) {
    return true;
  }

  // Nullref is a subtype of all reference types.
  if (IsReferenceType(expected) && actual == ValueType::Nullref) {
    return true;
  }

  return false;
}

}  // namespace

bool Validate(const At<ValueType>& actual,
              ValueType expected,
              Context& context) {
  if (!TypesMatch(expected, actual)) {
    context.errors->OnError(
        actual.loc(),
        format("Expected value type {}, got {}", expected, actual));
    return false;
  }
  return true;
}

bool EndModule(Context& context) {
  // Check that all functions referenced by a ref.func initializer in a global
  // are declared in an element segment. This can't be done in the global
  // section since the element section occurs later. It can't be done after the
  // element section either, since there might not be an element section.
  bool valid = true;
  for (auto index : context.deferred_function_references) {
    if (context.declared_functions.find(index) ==
        context.declared_functions.end()) {
      context.errors->OnError(
          index.loc(), format("Undeclared function reference {}", index));
      valid = false;
    }
  }
  return valid;
}

}  // namespace valid
}  // namespace wasp
