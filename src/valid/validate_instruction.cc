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

#include "wasp/valid/validate_instruction.h"

#include <limits>

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/errors_nop.h"
#include "wasp/valid/validate_index.h"
#include "wasp/valid/validate_locals.h"

namespace wasp {
namespace valid {

namespace {

using namespace ::wasp::binary;
using ValueTypeSpan = span<const ValueType>;

#define VALUE_TYPE_SPANS(V)                  \
  V(i32, ValueType::I32)                     \
  V(i64, ValueType::I64)                     \
  V(f32, ValueType::F32)                     \
  V(f64, ValueType::F64)                     \
  V(i32_i32, ValueType::I32, ValueType::I32) \
  V(i64_i64, ValueType::I64, ValueType::I64) \
  V(f32_f32, ValueType::F32, ValueType::F32) \
  V(f64_f64, ValueType::F64, ValueType::F64)

#define WASP_V(name, ...)                         \
  const ValueType array_##name[] = {__VA_ARGS__}; \
  ValueTypeSpan span_##name{array_##name};
VALUE_TYPE_SPANS(WASP_V)

#undef WASP_V
#undef VALUE_TYPE_SPANS

optional<FunctionType> GetBlockTypeSignature(BlockType block_type,
                                             Context& context,
                                             Errors& errors) {
  switch (block_type) {
    case BlockType::Void:
      return FunctionType{};

#define WASP_V(val, Name, str) \
  case BlockType::Name:        \
    return FunctionType{{}, {ValueType::Name}};
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V

    default:
      // TODO multi-value returns
      return nullopt;
  }
}

ValueTypeSpan GetTypeStack(const Label& label, Context& context) {
  return ValueTypeSpan{context.type_stack}.subspan(label.type_stack_limit);
}

Label& TopLabel(Context& context) {
  assert(!context.label_stack.empty());
  return context.label_stack.back();
}

void PushType(ValueType value_type, Context& context) {
  context.type_stack.push_back(value_type);
}

void PushTypes(ValueTypeSpan value_types, Context& context) {
  context.type_stack.insert(context.type_stack.end(), value_types.begin(),
                            value_types.end());
}

void RemovePrefixIfGreater(ValueTypeSpan* lhs, ValueTypeSpan rhs) {
  if (lhs->size() > rhs.size()) {
    remove_prefix(lhs, lhs->size() - rhs.size());
  }
}

void ResetTypeStackToLimit(Context& context) {
  context.type_stack.resize(TopLabel(context).type_stack_limit);
}

bool DropTypes(size_t count, Context& context, Errors& errors) {
  const auto& top_label = TopLabel(context);
  auto type_stack_size = context.type_stack.size() - top_label.type_stack_limit;
  if (count > type_stack_size) {
    errors.OnError(format("Expected stack to contain {} value{}, got {}", count,
                          count == 1 ? "" : "s", type_stack_size));
    ResetTypeStackToLimit(context);
    return top_label.unreachable;
  }
  context.type_stack.resize(context.type_stack.size() - count);
  return true;
}

bool CheckTypes(ValueTypeSpan expected, Context& context, Errors& errors) {
  ValueTypeSpan full_expected = expected;
  const auto& top_label = TopLabel(context);
  auto type_stack = GetTypeStack(top_label, context);
  RemovePrefixIfGreater(&type_stack, expected);
  if (top_label.unreachable) {
    RemovePrefixIfGreater(&expected, type_stack);
  }

  if (expected != type_stack) {
    // TODO proper formatting of type stack
    errors.OnError(format("Expected stack to contain {}, got {}{}",
                          full_expected, top_label.unreachable ? "..." : "",
                          type_stack));
    return false;
  }
  return true;
}

bool PopTypes(ValueTypeSpan expected, Context& context, Errors& errors) {
  ErrorsNop errors_nop{};
  bool valid = CheckTypes(expected, context, errors);
  valid &= DropTypes(expected.size(), context, errors_nop);
  return valid;
}

bool PopType(ValueType type, Context& context, Errors& errors) {
  return PopTypes(ValueTypeSpan(&type, 1), context, errors);
}

bool PopAndPushTypes(ValueTypeSpan param_types,
                     ValueTypeSpan result_types,
                     Context& context,
                     Errors& errors) {
  bool valid = PopTypes(param_types, context, errors);
  PushTypes(result_types, context);
  return valid;
}

bool PopAndPushTypes(const FunctionType& function_type,
                     Context& context,
                     Errors& errors) {
  return PopAndPushTypes(function_type.param_types, function_type.result_types,
                         context, errors);
}

void SetUnreachable(Context& context) {
  auto& top_label = TopLabel(context);
  top_label.unreachable = true;
  ResetTypeStackToLimit(context);
}

Label* GetLabel(Index depth, Context& context, Errors& errors) {
  if (depth >= context.label_stack.size()) {
    errors.OnError(format("Invalid label {}, must be less than {}", depth,
                          context.label_stack.size()));
    return nullptr;
  }
  return &context.label_stack[context.label_stack.size() - depth - 1];
}

void PushLabel(LabelType label_type,
               const FunctionType& type,
               Context& context) {
  context.label_stack.emplace_back(label_type, type.param_types,
                                   type.result_types,
                                   context.type_stack.size());
  PushTypes(type.param_types, context);
}

bool PushLabel(LabelType label_type,
               BlockType block_type,
               Context& context,
               Errors& errors) {
  auto sig = GetBlockTypeSignature(block_type, context, errors);
  if (!sig) {
    return false;
  }
  PushLabel(label_type, *sig, context);
  return true;
}

bool CheckTypeStackEmpty(Context& context, Errors& errors) {
  const auto& top_label = TopLabel(context);
  if (context.type_stack.size() != top_label.type_stack_limit) {
    errors.OnError(format("Expected empty stack, got {}",
                          GetTypeStack(top_label, context)));
    return false;
  }
  return true;
}

bool PopLabel(Context& context, Errors& errors) {
  const auto& top_label = TopLabel(context);
  bool valid = PopTypes(top_label.result_types, context, errors);
  valid &= CheckTypeStackEmpty(context, errors);
  PushTypes(top_label.result_types, context);
  context.label_stack.pop_back();
  return valid;
}

bool Br(Index depth, Context& context, Errors& errors) {
  const auto* label = GetLabel(depth, context, errors);
  bool valid = label ? PopTypes(label->br_types(), context, errors) : false;
  SetUnreachable(context);
  return valid;
}

bool BrIf(Index depth, Context& context, Errors& errors) {
  bool valid = PopType(ValueType::I32, context, errors);
  const auto* label = GetLabel(depth, context, errors);
  if (!label) {
    return false;
  }
  valid &= PopTypes(label->br_types(), context, errors);
  PushTypes(label->br_types(), context);
  return valid;
}

bool BrTable(const BrTableImmediate& immediate,
             Context& context,
             Errors& errors) {
  bool valid = PopType(ValueType::I32, context, errors);
  optional<ValueTypeSpan> br_types;
  auto handle_target = [&](Index target) {
    const auto* label = GetLabel(target, context, errors);
    if (label) {
      ValueTypeSpan label_br_types{label->br_types()};
      if (br_types) {
        if (*br_types != label_br_types) {
          errors.OnError(
              format("br_table labels must have the same signature; expected "
                     "{}, got {}",
                     *br_types, label_br_types));
          valid = false;
        }
      } else {
        br_types = label_br_types;
      }
      valid &= CheckTypes(*br_types, context, errors);
    } else {
      valid = false;
    }
  };

  handle_target(immediate.default_target);
  for (auto target : immediate.targets) {
    handle_target(target);
  }
  SetUnreachable(context);
  return valid;
}

bool Call(Index function_index, Context& context, Errors& errors) {
  if (!ValidateIndex(function_index, context.functions.size(), "function index",
                     errors)) {
    return false;
  }
  const auto& function = context.functions[function_index];
  if (!ValidateIndex(function.type_index, context.types.size(), "type index",
                     errors)) {
    return false;
  }
  return PopAndPushTypes(context.types[function.type_index].type, context,
                         errors);
}

bool CallIndirect(const CallIndirectImmediate& immediate,
                  Context& context,
                  Errors& errors) {
  if (!ValidateIndex(0, context.tables.size(), "table index", errors)) {
    return false;
  }
  if (!ValidateIndex(immediate.index, context.types.size(), "type index",
                     errors)) {
    return false;
  }
  bool valid = PopType(ValueType::I32, context, errors);
  valid &=
      PopAndPushTypes(context.types[immediate.index].type, context, errors);
  return valid;
}

bool LocalGet(Index index, Context& context, Errors& errors) {
  if (!ValidateIndex(index, context.locals.size(), "local index", errors)) {
    return false;
  }
  PushType(context.locals[index], context);
  return true;
}

bool LocalSet(Index index, Context& context, Errors& errors) {
  if (!ValidateIndex(index, context.locals.size(), "local index", errors)) {
    return false;
  }
  return PopType(context.locals[index], context, errors);
}

bool LocalTee(Index index, Context& context, Errors& errors) {
  if (!ValidateIndex(index, context.locals.size(), "local index", errors)) {
    return false;
  }
  auto type = context.locals[index];
  bool valid = PopType(type, context, errors);
  PushType(type, context);
  return valid;
}

}  // namespace

bool Validate(const Locals& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "locals"};
  const size_t old_count = context.locals.size();
  const Index max = std::numeric_limits<Index>::max();
  if (old_count > max - value.count) {
    errors.OnError(format("Too many locals; max is {}, got {}", max,
                          static_cast<u64>(old_count) + value.count));
    return false;
  }
  const size_t new_count = old_count + value.count;
  context.locals.reserve(new_count);
  for (Index i = 0; i < value.count; ++i) {
    context.locals.push_back(value.type);
  }
  return true;
}

bool Validate(const Instruction& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "instruction"};
  if (context.label_stack.empty()) {
    errors.OnError("Unexpected instruction after function end");
    return false;
  }

  switch (value.opcode) {
    case Opcode::Unreachable:
      SetUnreachable(context);
      return true;

    case Opcode::Nop:
      return true;

    case Opcode::Block:
      return PushLabel(LabelType::Block, value.block_type_immediate(), context,
                       errors);

    case Opcode::Loop:
      return PushLabel(LabelType::Loop, value.block_type_immediate(), context,
                       errors);

    case Opcode::End:
      return PopLabel(context, errors);

    case Opcode::Br:
      return Br(value.index_immediate(), context, errors);

    case Opcode::BrIf:
      return BrIf(value.index_immediate(), context, errors);

    case Opcode::BrTable:
      return BrTable(value.br_table_immediate(), context, errors);

    case Opcode::Return:
      return Br(context.label_stack.size() - 1, context, errors);

    case Opcode::Call:
      return Call(value.index_immediate(), context, errors);

    case Opcode::CallIndirect:
      return CallIndirect(value.call_indirect_immediate(), context, errors);

    case Opcode::Drop:
      return DropTypes(1, context, errors);

    case Opcode::LocalGet:
      return LocalGet(value.index_immediate(), context, errors);

    case Opcode::LocalSet:
      return LocalSet(value.index_immediate(), context, errors);

    case Opcode::LocalTee:
      return LocalTee(value.index_immediate(), context, errors);

    case Opcode::I32Const:
      PushType(ValueType::I32, context);
      return true;

    case Opcode::I64Const:
      PushType(ValueType::I64, context);
      return true;

    case Opcode::F32Const:
      PushType(ValueType::F32, context);
      return true;

    case Opcode::F64Const:
      PushType(ValueType::F64, context);
      return true;

    case Opcode::I32Eqz:
    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
      return PopAndPushTypes(span_i32, span_i32, context, errors);

    case Opcode::I64Eqz:
      return PopAndPushTypes(span_i64, span_i32, context, errors);

    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
      return PopAndPushTypes(span_i64, span_i64, context, errors);

    case Opcode::I32Eq:
    case Opcode::I32Ne:
    case Opcode::I32LtS:
    case Opcode::I32LtU:
    case Opcode::I32GtS:
    case Opcode::I32GtU:
    case Opcode::I32LeS:
    case Opcode::I32LeU:
    case Opcode::I32GeS:
    case Opcode::I32GeU:
    case Opcode::I32Add:
    case Opcode::I32Sub:
    case Opcode::I32Mul:
    case Opcode::I32DivS:
    case Opcode::I32DivU:
    case Opcode::I32RemS:
    case Opcode::I32RemU:
    case Opcode::I32And:
    case Opcode::I32Or:
    case Opcode::I32Xor:
    case Opcode::I32Shl:
    case Opcode::I32ShrS:
    case Opcode::I32ShrU:
    case Opcode::I32Rotl:
    case Opcode::I32Rotr:
      return PopAndPushTypes(span_i32_i32, span_i32, context, errors);

    case Opcode::I64LtS:
    case Opcode::I64LtU:
    case Opcode::I64GtS:
    case Opcode::I64GtU:
    case Opcode::I64LeS:
    case Opcode::I64LeU:
    case Opcode::I64GeS:
    case Opcode::I64GeU:
      return PopAndPushTypes(span_i64_i64, span_i32, context, errors);

    case Opcode::F32Eq:
    case Opcode::F32Ne:
    case Opcode::F32Lt:
    case Opcode::F32Gt:
    case Opcode::F32Le:
    case Opcode::F32Ge:
      return PopAndPushTypes(span_f32_f32, span_i32, context, errors);

    case Opcode::F64Eq:
    case Opcode::F64Ne:
    case Opcode::F64Lt:
    case Opcode::F64Gt:
    case Opcode::F64Le:
    case Opcode::F64Ge:
      return PopAndPushTypes(span_f64_f64, span_i32, context, errors);

    case Opcode::I64Add:
    case Opcode::I64Sub:
    case Opcode::I64Mul:
    case Opcode::I64DivS:
    case Opcode::I64DivU:
    case Opcode::I64RemS:
    case Opcode::I64RemU:
    case Opcode::I64And:
    case Opcode::I64Or:
    case Opcode::I64Xor:
    case Opcode::I64Shl:
    case Opcode::I64ShrS:
    case Opcode::I64ShrU:
    case Opcode::I64Rotl:
    case Opcode::I64Rotr:
      return PopAndPushTypes(span_i64_i64, span_i64, context, errors);

    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
      return PopAndPushTypes(span_f32, span_f32, context, errors);

    case Opcode::F32Add:
    case Opcode::F32Sub:
    case Opcode::F32Mul:
    case Opcode::F32Div:
    case Opcode::F32Min:
    case Opcode::F32Max:
    case Opcode::F32Copysign:
      return PopAndPushTypes(span_f32_f32, span_f32, context, errors);

    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
      return PopAndPushTypes(span_f64, span_f64, context, errors);

    case Opcode::F64Add:
    case Opcode::F64Sub:
    case Opcode::F64Mul:
    case Opcode::F64Div:
    case Opcode::F64Min:
    case Opcode::F64Max:
    case Opcode::F64Copysign:
      return PopAndPushTypes(span_f64_f64, span_f64, context, errors);

    case Opcode::I32TruncF32S:
    case Opcode::I32TruncF32U:
    case Opcode::I32ReinterpretF32:
      return PopAndPushTypes(span_f32, span_i32, context, errors);

    case Opcode::I32TruncF64S:
    case Opcode::I32TruncF64U:
      return PopAndPushTypes(span_f64, span_i32, context, errors);

    case Opcode::I64ExtendI32S:
    case Opcode::I64ExtendI32U:
      return PopAndPushTypes(span_i32, span_i64, context, errors);

    case Opcode::I64TruncF32S:
    case Opcode::I64TruncF32U:
      return PopAndPushTypes(span_f32, span_i64, context, errors);

    case Opcode::I64TruncF64S:
    case Opcode::I64TruncF64U:
    case Opcode::I64ReinterpretF64:
      return PopAndPushTypes(span_f64, span_i64, context, errors);

    case Opcode::F32ConvertI32S:
    case Opcode::F32ConvertI32U:
    case Opcode::F32ReinterpretI32:
      return PopAndPushTypes(span_i32, span_f32, context, errors);

    case Opcode::F32ConvertI64S:
    case Opcode::F32ConvertI64U:
      return PopAndPushTypes(span_i64, span_f32, context, errors);

    case Opcode::F32DemoteF64:
      return PopAndPushTypes(span_f64, span_f32, context, errors);

    case Opcode::F64ConvertI32S:
    case Opcode::F64ConvertI32U:
      return PopAndPushTypes(span_i32, span_f64, context, errors);

    case Opcode::F64ConvertI64S:
    case Opcode::F64ConvertI64U:
    case Opcode::F64ReinterpretI64:
      return PopAndPushTypes(span_i64, span_f64, context, errors);

    case Opcode::F64PromoteF32:
      return PopAndPushTypes(span_f32, span_f64, context, errors);

    default:
      WASP_UNREACHABLE();
      return false;
  }
}

}  // namespace valid
}  // namespace wasp
