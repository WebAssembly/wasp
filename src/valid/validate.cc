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

#include "wasp/base/concat.h"
#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/valid/match.h"
#include "wasp/valid/valid_ctx.h"

namespace wasp::valid {

bool BeginTypeSection(ValidCtx& ctx, Index type_count) {
  ctx.defined_type_count = type_count;
  ctx.same_types.Reset(type_count);
  ctx.match_types.Reset(type_count);
  return true;
}

bool EndTypeSection(ValidCtx& ctx) {
  // Update defined_type_count to the number of types that were actually
  // defined (not just the number at the beginning of the type section.) We
  // originally use the type section's count so that we can recursively define
  // types, but after the type section is complete, there can no longer be
  // recursive types.
  // TODO: This will change when the module import feature is implemented,
  // since it allows the type and import sections (among others) to be
  // repeated.
  ctx.defined_type_count = static_cast<Index>(ctx.types.size());
  return true;
}

bool BeginCode(ValidCtx& ctx, Location loc) {
  Index func_index = ctx.imported_function_count + ctx.code_count;
  if (func_index >= ctx.functions.size()) {
    ctx.errors->OnError(loc,
                        concat("Unexpected code index ", func_index,
                               ", function count is ", ctx.functions.size()));
    return false;
  }
  ctx.code_count++;
  const binary::Function& function = ctx.functions[func_index];
  ctx.type_stack.clear();
  ctx.label_stack.clear();
  ctx.locals.Reset();
  // Don't validate the index, should have already been validated at this point.
  if (function.type_index < ctx.types.size()) {
    const auto& defined_type = ctx.types[function.type_index];
    if (!defined_type.is_function_type()) {
      ctx.errors->OnError(loc, concat("Function must have a function type."));
      return false;
    }

    assert(defined_type.is_function_type());
    const auto& function_type = defined_type.function_type();
    ctx.locals.Append(function_type->param_types);
    ctx.label_stack.push_back(
        Label{LabelType::Function, ToStackTypeList(function_type->param_types),
              ToStackTypeList(function_type->result_types), 0});
    return true;
  } else {
    // Not valid, but try to continue anyway.
    ctx.label_stack.push_back(Label{LabelType::Function, {}, {}, 0});
    return false;
  }
}

bool CheckDefaultable(ValidCtx& ctx,
                      const At<binary::ReferenceType>& value,
                      string_view desc) {
  if (!IsDefaultableType(value)) {
    ctx.errors->OnError(value.loc(),
                        concat(desc, " must be defaultable, got ", value));
    return false;
  }
  return true;
}

bool CheckDefaultable(ValidCtx& ctx,
                      const At<binary::ValueType>& value,
                      string_view desc) {
  if (!IsDefaultableType(value)) {
    ctx.errors->OnError(value.loc(),
                        concat(desc, " must be defaultable, got ", value));
    return false;
  }
  return true;
}

bool CheckDefaultable(ValidCtx& ctx,
                      const At<binary::StorageType>& value,
                      string_view desc) {
  if (!IsDefaultableType(value)) {
    ctx.errors->OnError(value.loc(),
                        concat(desc, " must be defaultable, got ", value));
    return false;
  }
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::UnpackedExpression>& value) {
  bool valid = true;
  for (auto&& instr : value->instructions) {
    valid &= Validate(ctx, instr);
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::UnpackedCode>& value) {
  bool valid = true;
  valid &= BeginCode(ctx, value.loc());
  valid &= Validate(ctx, value->locals, RequireDefaultable::Yes);
  valid &= Validate(ctx, value->body);
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::ArrayType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "array type"};
  return Validate(ctx, value->field);
}

bool Validate(ValidCtx& ctx,
              const At<binary::ConstantExpression>& value,
              binary::ValueType expected_type,
              Index max_global_index) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "constant_expression"};
  if (value->instructions.size() != 1 && !ctx.features.gc_enabled()) {
    ctx.errors->OnError(value.loc(),
                        "A constant expression must be a single instruction");
    return false;
  }

  bool valid = true;
  ValidCtx new_context{ctx};
  new_context.type_stack.clear();
  new_context.label_stack.clear();
  new_context.locals.Reset();

  // Validate as if this expression was a function that takes no parameters,
  // and returns the expected type.
  new_context.label_stack.push_back(
      Label{LabelType::Function,
            {},
            ToStackTypeList(binary::ValueTypeList{expected_type}),
            0});

  for (auto&& instruction : value->instructions) {
    switch (instruction->opcode) {
      case Opcode::I32Const:
      case Opcode::I64Const:
      case Opcode::F32Const:
      case Opcode::F64Const:
      case Opcode::V128Const:
      case Opcode::RefNull:
      case Opcode::RttCanon:
      case Opcode::RttSub:
        // Validate normally.
        break;

      case Opcode::GlobalGet: {
        // global initializers are only allowed to reference imported globals,
        // so the maximum global index is shorter than specified by the
        // ctx. All other constant expressions can use the full range.
        auto index = instruction->index_immediate();
        if (!ValidateIndex(ctx, index, max_global_index, "global index")) {
          return false;
        }

        if (new_context.globals[index].mut == Mutability::Var) {
          new_context.errors->OnError(
              instruction->index_immediate().loc(),
              "A constant expression cannot contain a mutable global");
          return false;
        }
        break;
      }

      case Opcode::RefFunc: {
        auto index = instruction->index_immediate();
        if (!ValidateFunctionIndex(new_context, index)) {
          return false;
        }

        // ref.func indexes are implicitly declared by referencing them in a
        // constant expression.
        ctx.declared_functions.insert(index);
        new_context.declared_functions.insert(index);
        break;
      }

      default:
        ctx.errors->OnError(
            instruction.loc(),
            concat("Invalid instruction in constant expression: ",
                   instruction));
        return false;
    }

    // Do normal instruction validation.
    valid &= Validate(new_context, instruction);
  }

  // Insert an implicit end instruction to check that the instruction sequence
  // actually produces a value of the expected type.
  valid &=
      Validate(new_context, At{value.loc(), binary::Instruction{Opcode::End}});
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::DataCount>& value) {
  ctx.declared_data_count = value->count;
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::DataSegment>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "data segment"};
  bool valid = true;
  optional<MemoryType> memory_type;
  if (value->memory_index) {
    if (ValidateMemoryIndex(ctx, *value->memory_index)) {
      memory_type = ctx.memories[*value->memory_index];
    } else {
      valid = false;
    }
  }
  if (value->offset) {
    const binary::ValueType index_type =
        memory_type && memory_type->limits->index_type == IndexType::I64
            ? binary::ValueType::I64_NoLocation()
            : binary::ValueType::I32_NoLocation();
    valid &= Validate(ctx, *value->offset, index_type,
                      static_cast<Index>(ctx.globals.size()));
  }
  return valid;
}

bool Validate(ValidCtx& ctx,
              const At<binary::ElementExpression>& value,
              binary::ReferenceType reftype) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "element expression"};
  if (value->instructions.size() != 1) {
    ctx.errors->OnError(value.loc(),
                        "An element expression must be a single instruction");
    return false;
  }

  bool valid = true;
  auto&& instruction = value->instructions[0];
  optional<binary::ReferenceType> actual_type;
  switch (instruction->opcode) {
    case Opcode::RefNull:
      actual_type = binary::ReferenceType{
          binary::RefType{instruction->heap_type_immediate(), Null::Yes}};
      break;

    case Opcode::RefFunc: {
      actual_type = binary::ReferenceType::Funcref_NoLocation();
      auto index = instruction->index_immediate();
      if (!ValidateFunctionIndex(ctx, index)) {
        valid = false;
      }
      ctx.declared_functions.insert(index);
      break;
    }

    default:
      ctx.errors->OnError(
          instruction.loc(),
          concat("Invalid instruction in element expression: ", instruction));
      return false;
  }

  assert(actual_type.has_value());
  valid &= Validate(ctx, reftype, At{value.loc(), *actual_type});
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::ElementSegment>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "element segment"};
  ctx.element_segments.push_back(value->elemtype());
  bool valid = true;
  if (value->table_index) {
    valid &= ValidateTableIndex(ctx, *value->table_index);
  }
  if (value->offset) {
    valid &= Validate(ctx, *value->offset, binary::ValueType::I32_NoLocation(),
                      static_cast<Index>(ctx.globals.size()));
  }
  if (value->has_indexes()) {
    auto&& elements = value->indexes();
    Index max_index;
    switch (elements.kind) {
      case ExternalKind::Function:
        max_index = static_cast<Index>(ctx.functions.size());
        break;
      case ExternalKind::Table:
        max_index = static_cast<Index>(ctx.tables.size());
        break;
      case ExternalKind::Memory:
        max_index = static_cast<Index>(ctx.memories.size());
        break;
      case ExternalKind::Global:
        max_index = static_cast<Index>(ctx.globals.size());
        break;
      case ExternalKind::Tag:
        max_index = static_cast<Index>(ctx.tags.size());
        break;
      default:
        WASP_UNREACHABLE();
    }

    for (auto index : elements.list) {
      valid &= ValidateIndex(ctx, index, max_index, "index");
      if (elements.kind == ExternalKind::Function) {
        ctx.declared_functions.insert(index);
      }
    }
  } else if (value->has_expressions()) {
    auto&& elements = value->expressions();

    valid &= Validate(ctx, elements.elemtype);
    for (const auto& expr : elements.list) {
      valid &= Validate(ctx, expr, elements.elemtype);
    }
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::Export>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "export"};
  bool valid = true;

  if (ctx.export_names.find(value->name) != ctx.export_names.end()) {
    ctx.errors->OnError(value.loc(),
                        concat("Duplicate export name ", value->name));
    valid = false;
  }
  ctx.export_names.insert(value->name);

  switch (value->kind) {
    case ExternalKind::Function:
      valid &= ValidateFunctionIndex(ctx, value->index);
      ctx.declared_functions.insert(value->index);
      break;

    case ExternalKind::Table:
      valid &= ValidateTableIndex(ctx, value->index);
      break;

    case ExternalKind::Memory:
      valid &= ValidateMemoryIndex(ctx, value->index);
      break;

    case ExternalKind::Global:
      if (ValidateGlobalIndex(ctx, value->index)) {
        const auto& global = ctx.globals[value->index];
        if (global.mut == Mutability::Var &&
            !ctx.features.mutable_globals_enabled()) {
          ctx.errors->OnError(value->index.loc(),
                              "Mutable globals cannot be exported");
          valid = false;
        }
      } else {
        valid = false;
      }
      break;

    case ExternalKind::Tag:
      valid &= ValidateTagIndex(ctx, value->index);
      break;

    default:
      WASP_UNREACHABLE();
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::Tag>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "tag"};
  return Validate(ctx, value->tag_type);
}

bool Validate(ValidCtx& ctx, const At<binary::TagType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "tag type"};
  ctx.tags.push_back(value);
  if (!ValidateTypeIndex(ctx, value->type_index)) {
    return false;
  }

  assert(value->type_index < ctx.types.size());
  const auto& defined_type = ctx.types[value->type_index];
  if (!defined_type.is_function_type()) {
    ctx.errors->OnError(value.loc(),
                        concat("Tag type must be a function type."));
    return false;
  }

  const auto& function_type = defined_type.function_type();

  if (!function_type->result_types.empty()) {
    ctx.errors->OnError(value.loc(),
                        concat("Expected an empty exception result type, got ",
                               function_type->result_types));
    return false;
  }
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::FieldType>& value) {
  return Validate(ctx, value->type);
}

bool Validate(ValidCtx& ctx, const binary::FieldTypeList& values) {
  bool valid = true;
  for (const auto& value : values) {
    valid &= Validate(ctx, value);
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::Function>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "function"};
  ctx.functions.push_back(value);
  if (!ValidateTypeIndex(ctx, value->type_index)) {
    return false;
  }

  assert(value->type_index < ctx.types.size());
  const auto& defined_type = ctx.types[value->type_index];
  if (!defined_type.is_function_type()) {
    ctx.errors->OnError(value.loc(),
                        concat("Function must have function type"));
    return false;
  }
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::FunctionType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "function type"};
  bool valid = true;
  if (value->result_types.size() > 1 && !ctx.features.multi_value_enabled()) {
    ctx.errors->OnError(value.loc(),
                        concat("Expected result type count of 0 or 1, got ",
                               value->result_types.size()));
    valid = false;
  }
  valid &= Validate(ctx, value->param_types);
  valid &= Validate(ctx, value->result_types);
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::Global>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "global"};
  ctx.globals.push_back(value->global_type);
  bool valid = true;
  valid &= Validate(ctx, value->global_type);
  // Normally only imported globals can be used in a global's constant
  // expression, but the gc proposal extends this to allow any global.
  auto global_count = ctx.features.gc_enabled() ? ctx.globals.size()
                                                : ctx.imported_global_count;
  valid &=
      Validate(ctx, value->init, value->global_type->valtype, global_count);
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::GlobalType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "global type"};
  return Validate(ctx, value->valtype);
}

bool Validate(ValidCtx& ctx, const At<binary::HeapType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "heap type"};
  if (value->is_index()) {
    return ValidateTypeIndex(ctx, value->index());
  }
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::Import>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "import"};
  bool valid = true;

  switch (value->kind()) {
    case ExternalKind::Function:
      valid &= Validate(ctx, binary::Function{value->index()});
      ctx.imported_function_count++;
      break;

    case ExternalKind::Table:
      valid &= Validate(ctx, binary::Table{value->table_type()});
      break;

    case ExternalKind::Memory:
      valid &= Validate(ctx, binary::Memory{value->memory_type()});
      break;

    case ExternalKind::Global:
      ctx.globals.push_back(value->global_type());
      ctx.imported_global_count++;
      valid &= Validate(ctx, value->global_type());
      if (value->global_type()->mut == Mutability::Var &&
          !ctx.features.mutable_globals_enabled()) {
        ctx.errors->OnError(value->global_type().loc(),
                            "Mutable globals cannot be imported");
        valid = false;
      }
      break;

    case ExternalKind::Tag:
      valid &= Validate(ctx, binary::Tag{value->tag_type()});
      break;

    default:
      WASP_UNREACHABLE();
      break;
  }
  return valid;
}

bool ValidateIndex(ValidCtx& ctx,
                   const At<Index>& index,
                   Index max,
                   string_view desc) {
  if (index >= max) {
    ctx.errors->OnError(index.loc(), concat("Invalid ", desc, " ", index,
                                            ", must be less than ", max));
    return false;
  }
  return true;
}

bool ValidateTypeIndex(ValidCtx& ctx, const At<Index>& index) {
  // The defined_type_count is used here instead of ctx.types.size(), since the
  // type section can reference type indexes that not have yet been defined.
  // After the type section is finished, this value is updated to the final
  // type count.
  return ValidateIndex(ctx, index, ctx.defined_type_count, "type index");
}

bool ValidateFunctionIndex(ValidCtx& ctx, const At<Index>& index) {
  return ValidateIndex(ctx, index, static_cast<Index>(ctx.functions.size()),
                       "function index");
}

bool ValidateMemoryIndex(ValidCtx& ctx, const At<Index>& index) {
  return ValidateIndex(ctx, index, static_cast<Index>(ctx.memories.size()),
                       "memory index");
}

bool ValidateTableIndex(ValidCtx& ctx, const At<Index>& index) {
  return ValidateIndex(ctx, index, static_cast<Index>(ctx.tables.size()),
                       "table index");
}

bool ValidateGlobalIndex(ValidCtx& ctx, const At<Index>& index) {
  return ValidateIndex(ctx, index, static_cast<Index>(ctx.globals.size()),
                       "global index");
}

bool ValidateTagIndex(ValidCtx& ctx, const At<Index>& index) {
  return ValidateIndex(ctx, index, static_cast<Index>(ctx.tags.size()),
                       "tag index");
}

bool Validate(ValidCtx& ctx, const At<Limits>& value, Index max) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "limits"};
  bool valid = true;
  if (value->min > max) {
    ctx.errors->OnError(
        value->min.loc(),
        concat("Expected minimum ", value->min, " to be <= ", max));
    valid = false;
  }
  if (value->max.has_value()) {
    if (*value->max > max) {
      ctx.errors->OnError(
          value->max->loc(),
          concat("Expected maximum ", *value->max, " to be <= ", max));
      valid = false;
    }
    if (value->min.value() > value->max->value()) {
      ctx.errors->OnError(value->min.loc(),
                          concat("Expected minimum ", value->min,
                                 " to be <= maximum ", *value->max));
      valid = false;
    }
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::Memory>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "memory"};
  ctx.memories.push_back(value->memory_type);
  bool valid = Validate(ctx, value->memory_type);
  if (ctx.memories.size() > 1) {
    ctx.errors->OnError(value.loc(), "Too many memories, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<MemoryType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "memory type"};
  constexpr Index kMaxPages = 65536;
  bool valid = Validate(ctx, value->limits, kMaxPages);
  if (value->limits->shared == Shared::Yes) {
    if (!ctx.features.threads_enabled()) {
      ctx.errors->OnError(value.loc(), "Memories cannot be shared");
      valid = false;
    }

    if (!value->limits->max) {
      ctx.errors->OnError(value.loc(), "Shared memories must have a maximum");
      valid = false;
    }
  }
  return valid;
}

bool Validate(ValidCtx& ctx,
              binary::ReferenceType expected,
              const At<binary::ReferenceType>& actual) {
  if (!IsMatch(ctx, actual, expected)) {
    ctx.errors->OnError(actual.loc(), concat("Expected reference type ",
                                             expected, ", got ", actual));
    return false;
  }
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::ReferenceType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "reference type"};
  if (value->is_ref()) {
    return Validate(ctx, value->ref());
  }
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::Rtt>& value) {
  return true;
}

bool Validate(ValidCtx& ctx, const At<binary::RefType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "ref type"};
  return Validate(ctx, value->heap_type);
}

bool Validate(ValidCtx& ctx, const At<binary::Start>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "start"};
  if (!ValidateFunctionIndex(ctx, value->func_index)) {
    return false;
  }

  bool valid = true;
  auto function = ctx.functions[value->func_index];
  if (function.type_index < ctx.types.size()) {
    const auto& defined_type = ctx.types[function.type_index];
    if (!defined_type.is_function_type()) {
      ctx.errors->OnError(value.loc(),
                          concat("Start function must have function type"));
      return false;
    }

    const auto& function_type = defined_type.function_type();

    if (function_type->param_types.size() != 0) {
      ctx.errors->OnError(
          value.loc(), concat("Expected start function to have 0 params, got ",
                              function_type->param_types.size()));
      valid = false;
    }

    if (function_type->result_types.size() != 0) {
      ctx.errors->OnError(
          value.loc(), concat("Expected start function to have 0 results, got ",
                              function_type->result_types.size()));
      valid = false;
    }
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::StorageType>& value) {
  if (value->is_value_type()) {
    return Validate(ctx, value->value_type());
  } else {
    return true;
  }
}

bool Validate(ValidCtx& ctx, const At<binary::StructType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "struct type"};
  return Validate(ctx, value->fields);
}

bool Validate(ValidCtx& ctx, const At<binary::Table>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "table"};
  ctx.tables.push_back(value->table_type);
  bool valid = Validate(ctx, value->table_type);
  if (ctx.tables.size() > 1 && !ctx.features.reference_types_enabled()) {
    ctx.errors->OnError(value.loc(), "Too many tables, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::TableType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "table type"};
  constexpr Index kMaxElements = std::numeric_limits<Index>::max();
  bool valid = Validate(ctx, value->limits, kMaxElements);
  valid &= Validate(ctx, value->elemtype);
  valid &= CheckDefaultable(ctx, value->elemtype, "local type");
  if (value->limits->shared == Shared::Yes) {
    ctx.errors->OnError(value.loc(), "Tables cannot be shared");
    valid = false;
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const At<binary::DefinedType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "defined type"};
  ctx.types.push_back(value);

  if (value->is_function_type()) {
    return Validate(ctx, value->function_type());
  } else if (value->is_struct_type()) {
    return Validate(ctx, value->struct_type());
  } else {
    assert(value->is_array_type());
    return Validate(ctx, value->array_type());
  }
}

bool Validate(ValidCtx& ctx, const At<binary::ValueType>& value) {
  ErrorsContextGuard guard{*ctx.errors, value.loc(), "value type"};
  if (value->is_reference_type()) {
    return Validate(ctx, value->reference_type());
  }
  return true;
}

bool Validate(ValidCtx& ctx,
              binary::ValueType expected,
              const At<binary::ValueType>& actual) {
  if (!IsMatch(ctx, expected, actual)) {
    ctx.errors->OnError(actual.loc(), concat("Expected value type ", expected,
                                             ", got ", actual));
    return false;
  }
  return true;
}

bool Validate(ValidCtx& ctx, const binary::ValueTypeList& values) {
  bool valid = true;
  for (const auto& value : values) {
    valid &= Validate(ctx, value);
  }
  return valid;
}

template <typename T>
bool ValidateKnownSection(ValidCtx& ctx, const std::vector<T>& values) {
  bool valid = true;
  for (auto& value : values) {
    valid &= Validate(ctx, value);
  }
  return valid;
}

template <typename T>
bool ValidateKnownSection(ValidCtx& ctx, const optional<T>& value) {
  bool valid = true;
  if (value) {
    valid &= Validate(ctx, *value);
  }
  return valid;
}

bool Validate(ValidCtx& ctx, const binary::Module& value) {
  bool valid = true;
  valid &= BeginTypeSection(ctx, static_cast<Index>(value.types.size()));
  valid &= ValidateKnownSection(ctx, value.types);
  valid &= EndTypeSection(ctx);
  valid &= ValidateKnownSection(ctx, value.imports);
  valid &= ValidateKnownSection(ctx, value.functions);
  valid &= ValidateKnownSection(ctx, value.tables);
  valid &= ValidateKnownSection(ctx, value.memories);
  valid &= ValidateKnownSection(ctx, value.globals);
  valid &= ValidateKnownSection(ctx, value.tags);
  valid &= ValidateKnownSection(ctx, value.exports);
  valid &= ValidateKnownSection(ctx, value.start);
  valid &= ValidateKnownSection(ctx, value.element_segments);
  valid &= ValidateKnownSection(ctx, value.data_count);
  valid &= ValidateKnownSection(ctx, value.codes);
  valid &= ValidateKnownSection(ctx, value.data_segments);
  return valid;
}

}  // namespace wasp::valid
