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

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/base/utf8.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"

namespace wasp {
namespace valid {

namespace {

bool ValidateUtf8(string_view s, Errors& errors) {
  if (!IsValidUtf8(s)) {
    errors.OnError("Invalid UTF-8 encoding");
    return false;
  }
  return true;
}

}  // namespace

bool Validate(const binary::ConstantExpression& value,
              binary::ValueType expected_type,
              Index max_global_index,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "constant_expression"};
  bool valid = true;
  binary::ValueType actual_type;
  switch (value.instruction.opcode) {
    case binary::Opcode::I32Const:
      actual_type = binary::ValueType::I32;
      break;

    case binary::Opcode::I64Const:
      actual_type = binary::ValueType::I64;
      break;

    case binary::Opcode::F32Const:
      actual_type = binary::ValueType::F32;
      break;

    case binary::Opcode::F64Const:
      actual_type = binary::ValueType::F64;
      break;

    case binary::Opcode::GlobalGet: {
      auto index = value.instruction.index_immediate();
      if (!ValidateIndex(index, max_global_index, "global index", errors)) {
        return false;
      }

      const auto& global = context.globals[index];
      actual_type = global.valtype;

      if (context.globals[index].mut == binary::Mutability::Var) {
        errors.OnError("A constant expression cannot contain a mutable global");
        valid = false;
      }
      break;
    }

    default:
      errors.OnError(format("Invalid instruction in constant expression: {}",
                            value.instruction));
      return false;
  }

  valid &= Validate(actual_type, expected_type, context, features, errors);
  return valid;
}

bool Validate(const binary::DataCount& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  context.data_segment_count = value.count;
  return true;
}

bool Validate(const binary::DataSegment& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "data segment"};
  bool valid = true;
  if (value.memory_index) {
    valid &= ValidateIndex(*value.memory_index, context.memories.size(),
                           "memory index", errors);
  }
  if (value.offset) {
    valid &= Validate(*value.offset, binary::ValueType::I32,
                      context.globals.size(), context, features, errors);
  }
  return valid;
}

bool Validate(const binary::ElementExpression& value,
              binary::ElementType element_type,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "element expression"};
  bool valid = true;
  binary::ElementType actual_type;
  switch (value.instruction.opcode) {
    case binary::Opcode::RefNull:
      actual_type = binary::ElementType::Funcref;
      break;

    case binary::Opcode::RefFunc: {
      actual_type = binary::ElementType::Funcref;
      if (!ValidateIndex(value.instruction.index_immediate(),
                         context.functions.size(), "function index", errors)) {
        valid = false;
      }
      break;
    }

    default:
      errors.OnError(format("Invalid instruction in element expression: {}",
                            value.instruction));
      return false;
  }

  valid &= Validate(actual_type, element_type, context, features, errors);
  return valid;
}

bool Validate(const binary::ElementSegment& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "element segment"};
  context.element_segments.push_back(value.type);
  bool valid = true;
  if (value.table_index) {
    valid &= ValidateIndex(*value.table_index, context.tables.size(),
                           "table index", errors);
  }
  if (value.offset) {
    valid &= Validate(*value.offset, binary::ValueType::I32,
                      context.globals.size(), context, features, errors);
  }
  if (value.has_indexes()) {
    auto&& desc = value.indexes();
    Index max_index;
    switch (desc.kind) {
      case binary::ExternalKind::Function:
        max_index = context.functions.size();
        break;
      case binary::ExternalKind::Table:
        max_index = context.tables.size();
        break;
      case binary::ExternalKind::Memory:
        max_index = context.memories.size();
        break;
      case binary::ExternalKind::Global:
        max_index = context.globals.size();
        break;
      case binary::ExternalKind::Event:
        max_index = 0; // TODO: context.events.size();
        break;
    }

    for (auto index : desc.init) {
      valid &= ValidateIndex(index, max_index, "index", errors);
    }
  } else if (value.has_expressions()) {
    auto&& desc = value.expressions();

    for (const auto& expr : desc.init) {
      valid &= Validate(expr, desc.element_type, context, features, errors);
    }
  }
  return valid;
}

bool Validate(binary::ElementType actual,
              binary::ElementType expected,
              Context& context,
              const Features& features,
              Errors& errors) {
  if (actual != expected) {
    errors.OnError(
        format("Expected element type {}, got {}", expected, actual));
    return false;
  }
  return true;
}

bool Validate(const binary::Export& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "export"};
  bool valid = true;
  valid &= ValidateUtf8(value.name, errors);

  if (context.export_names.find(value.name) != context.export_names.end()) {
    errors.OnError(format("Duplicate export name {}", value.name));
    valid = false;
  }
  context.export_names.insert(value.name);

  switch (value.kind) {
    case binary::ExternalKind::Function:
      valid &= ValidateIndex(value.index, context.functions.size(),
                             "function index", errors);
      break;

    case binary::ExternalKind::Table:
      valid &= ValidateIndex(value.index, context.tables.size(), "table index",
                             errors);
      break;

    case binary::ExternalKind::Memory:
      valid &= ValidateIndex(value.index, context.memories.size(),
                             "memory index", errors);
      break;

    case binary::ExternalKind::Global:
      if (ValidateIndex(value.index, context.globals.size(), "global index",
                         errors)) {
        const auto& global = context.globals[value.index];
        if (global.mut == binary::Mutability::Var &&
            !features.mutable_globals_enabled()) {
          errors.OnError("Mutable globals cannot be exported");
          valid = false;
        }
      } else {
        valid = false;
      }
      break;

    default:
      WASP_UNREACHABLE();
  }
  return valid;
}

bool Validate(const binary::Function& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "function"};
  context.functions.push_back(value);
  return ValidateIndex(value.type_index, context.types.size(),
                       "function type index", errors);
}

bool Validate(const binary::FunctionType& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "function type"};
  if (value.result_types.size() > 1 && !features.multi_value_enabled()) {
    errors.OnError(format("Expected result type count of 0 or 1, got {}",
                          value.result_types.size()));
    return false;
  }
  return true;
}

bool Validate(const binary::Global& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "global"};
  context.globals.push_back(value.global_type);
  bool valid = true;
  valid &= Validate(value.global_type, context, features, errors);
  // Only imported globals can be used in a global's constant expression.
  valid &= Validate(value.init, value.global_type.valtype,
                    context.imported_global_count, context, features, errors);
  return valid;
}

bool Validate(const binary::GlobalType& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  return true;
}

bool Validate(const binary::Import& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "import"};
  bool valid = true;
  valid &= ValidateUtf8(value.module, errors);
  valid &= ValidateUtf8(value.name, errors);

  switch (value.kind()) {
    case binary::ExternalKind::Function:
      valid &=
          Validate(binary::Function{value.index()}, context, features, errors);
      context.imported_function_count++;
      break;

    case binary::ExternalKind::Table:
      valid &= Validate(binary::Table{value.table_type()}, context, features,
                        errors);
      break;

    case binary::ExternalKind::Memory:
      valid &= Validate(binary::Memory{value.memory_type()}, context, features,
                        errors);
      break;

    case binary::ExternalKind::Global:
      context.globals.push_back(value.global_type());
      context.imported_global_count++;
      valid &= Validate(value.global_type(), context, features, errors);
      if (value.global_type().mut == binary::Mutability::Var &&
          !features.mutable_globals_enabled()) {
        errors.OnError("Mutable globals cannot be imported");
        valid = false;
      }
      break;

    default:
      WASP_UNREACHABLE();
      break;
  }
  return valid;
}

bool ValidateIndex(Index index, Index max, string_view desc, Errors& errors) {
  if (index >= max) {
    errors.OnError(
        format("Invalid {} {}, must be less than {}", desc, index, max));
    return false;
  }
  return true;
}

bool Validate(const binary::Limits& value,
              Index max,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "limits"};
  bool valid = true;
  if (value.min > max) {
    errors.OnError(format("Expected minimum {} to be <= {}", value.min, max));
    valid = false;
  }
  if (value.max.has_value()) {
    if (*value.max > max) {
      errors.OnError(
          format("Expected maximum {} to be <= {}", *value.max, max));
      valid = false;
    }
    if (value.min > *value.max) {
      errors.OnError(format("Expected minimum {} to be <= maximum {}",
                            value.min, *value.max));
      valid = false;
    }
  }
  return valid;
}

bool Validate(const binary::Memory& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "memory"};
  context.memories.push_back(value.memory_type);
  bool valid = Validate(value.memory_type, context, features, errors);
  if (context.memories.size() > 1) {
    errors.OnError("Too many memories, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(const binary::MemoryType& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "memory type"};
  constexpr Index kMaxPages = 65536;
  bool valid = Validate(value.limits, kMaxPages, context, features, errors);
  if (value.limits.shared == binary::Shared::Yes &&
      !features.threads_enabled()) {
    errors.OnError("Memories cannot be shared");
    valid = false;
  }
  return valid;
}

bool Validate(const binary::Section& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  if (value.is_known()) {
    auto id = value.known().id;
    if (context.last_section_id && *context.last_section_id >= id) {
      errors.OnError(format("Section out of order: {} cannot occur after {}",
                            id, *context.last_section_id));
      return false;
    }
    context.last_section_id = id;
    return true;
  } else {
    return ValidateUtf8(value.custom().name, errors);
  }
}

bool Validate(const binary::Start& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "start"};
  if (!ValidateIndex(value.func_index, context.functions.size(),
                     "function index", errors)) {
    return false;
  }

  bool valid = true;
  auto function = context.functions[value.func_index];
  if (function.type_index < context.types.size()) {
    const auto& type_entry = context.types[function.type_index];
    if (type_entry.type.param_types.size() != 0) {
      errors.OnError(format("Expected start function to have 0 params, got {}",
                            type_entry.type.param_types.size()));
      valid = false;
    }

    if (type_entry.type.result_types.size() != 0) {
      errors.OnError(format("Expected start function to have 0 results, got {}",
                            type_entry.type.result_types.size()));
      valid = false;
    }
  }
  return valid;
}

bool Validate(const binary::Table& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "table"};
  context.tables.push_back(value.table_type);
  bool valid = Validate(value.table_type, context, features, errors);
  if (context.tables.size() > 1 && !features.reference_types_enabled()) {
    errors.OnError("Too many tables, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(const binary::TableType& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "table type"};
  constexpr Index kMaxElements = std::numeric_limits<Index>::max();
  bool valid = Validate(value.limits, kMaxElements, context, features, errors);
  if (value.limits.shared == binary::Shared::Yes) {
    errors.OnError("Tables cannot be shared");
    valid = false;
  }
  return valid;
}

bool Validate(const binary::TypeEntry& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "type entry"};
  context.types.push_back(value);
  return Validate(value.type, context, features, errors);
}

bool Validate(binary::ValueType actual,
              binary::ValueType expected,
              Context& context,
              const Features& features,
              Errors& errors) {
  if (actual != expected) {
    errors.OnError(format("Expected value type {}, got {}", expected, actual));
    return false;
  }
  return true;
}

}  // namespace valid
}  // namespace wasp
