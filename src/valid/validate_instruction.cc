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

#define VALUE_TYPE_SPANS(V)                      \
  V(i32, ValueType::I32)                         \
  V(i64, ValueType::I64)                         \
  V(f32, ValueType::F32)                         \
  V(f64, ValueType::F64)                         \
  V(v128, ValueType::V128)                       \
  V(i32_i32, ValueType::I32, ValueType::I32)     \
  V(i32_i64, ValueType::I32, ValueType::I64)     \
  V(i32_f32, ValueType::I32, ValueType::F32)     \
  V(i32_f64, ValueType::I32, ValueType::F64)     \
  V(i32_v128, ValueType::I32, ValueType::V128)   \
  V(i64_i64, ValueType::I64, ValueType::I64)     \
  V(f32_f32, ValueType::F32, ValueType::F32)     \
  V(f64_f64, ValueType::F64, ValueType::F64)     \
  V(v128_i32, ValueType::V128, ValueType::I32)   \
  V(v128_i64, ValueType::V128, ValueType::I64)   \
  V(v128_f32, ValueType::V128, ValueType::F32)   \
  V(v128_f64, ValueType::V128, ValueType::F64)   \
  V(v128_v128, ValueType::V128, ValueType::V128) \
  V(v128_v128_v128, ValueType::V128, ValueType::V128, ValueType::V128)

#define WASP_V(name, ...)                         \
  const ValueType array_##name[] = {__VA_ARGS__}; \
  const ValueTypeSpan span_##name{array_##name};
VALUE_TYPE_SPANS(WASP_V)

#undef WASP_V
#undef VALUE_TYPE_SPANS

bool AllTrue() { return true; }

template <typename T, typename... Args>
bool AllTrue(T first, Args... rest) {
  return !!first & AllTrue(rest...);
}

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

Label& TopLabel(Context& context) {
  assert(!context.label_stack.empty());
  return context.label_stack.back();
}

ValueTypeSpan GetTypeStack(Context& context) {
  return ValueTypeSpan{context.type_stack}.subspan(
      TopLabel(context).type_stack_limit);
}

optional<Function> GetFunction(Index index, Context& context, Errors& errors) {
  if (!ValidateIndex(index, context.functions.size(), "function index",
                     errors)) {
    return nullopt;
  }
  return context.functions[index];
}

optional<FunctionType> GetFunctionType(Index index,
                                       Context& context,
                                       Errors& errors) {
  if (!ValidateIndex(index, context.types.size(), "type index", errors)) {
    return nullopt;
  }
  return context.types[index].type;
}

optional<TableType> GetTableType(Index index,
                                 Context& context,
                                 Errors& errors) {
  if (!ValidateIndex(index, context.tables.size(), "table index", errors)) {
    return nullopt;
  }
  return context.tables[index];
}

optional<MemoryType> GetMemoryType(Index index,
                                   Context& context,
                                   Errors& errors) {
  if (!ValidateIndex(index, context.memories.size(), "memory index", errors)) {
    return nullopt;
  }
  return context.memories[index];
}

optional<GlobalType> GetGlobalType(Index index,
                                   Context& context,
                                   Errors& errors) {
  if (!ValidateIndex(index, context.globals.size(), "global index", errors)) {
    return nullopt;
  }
  return context.globals[index];
}

optional<ValueType> GetLocalType(Index index,
                                 Context& context,
                                 Errors& errors) {
  if (!ValidateIndex(index, context.locals.size(), "local index", errors)) {
    return nullopt;
  }
  return context.locals[index];
}

ValueType MaybeDefault(optional<ValueType> value) {
  return value.value_or(ValueType::I32);
}

Function MaybeDefault(optional<Function> value) {
  return value.value_or(Function{0});
}

FunctionType MaybeDefault(optional<FunctionType> value) {
  return value.value_or(FunctionType{});
}

GlobalType MaybeDefault(optional<GlobalType> value) {
  return value.value_or(GlobalType{ValueType::I32, Mutability::Const});
}

Label MaybeDefault(const Label* value) {
  return value ? *value : Label{LabelType::Block, {}, {}, 0};
}

optional<ValueType> PeekType(Context& context, Errors& errors) {
  auto type_stack = GetTypeStack(context);
  if (type_stack.empty()) {
    errors.OnError("Expected stack to have 1 value, got 0");
    return nullopt;
  }
  return type_stack[type_stack.size() - 1];
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
  auto type_stack = GetTypeStack(context);
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
    errors.OnError(
        format("Expected empty stack, got {}", GetTypeStack(context)));
    return false;
  }
  return true;
}

bool Else(Context& context, Errors& errors) {
  auto& top_label = TopLabel(context);
  if (top_label.label_type != LabelType::If) {
    errors.OnError("Got else instruction without if");
    return false;
  }
  bool valid = PopTypes(top_label.result_types, context, errors);
  valid &= CheckTypeStackEmpty(context, errors);
  ResetTypeStackToLimit(context);
  PushTypes(top_label.param_types, context);
  top_label.label_type = LabelType::Else;
  top_label.unreachable = false;
  return valid;
}

bool End(Context& context, Errors& errors) {
  auto& top_label = TopLabel(context);
  bool valid = true;
  if (top_label.label_type == LabelType::If) {
    valid &= Else(context, errors);
  }
  valid &= PopTypes(top_label.result_types, context, errors);
  valid &= CheckTypeStackEmpty(context, errors);
  ResetTypeStackToLimit(context);
  PushTypes(top_label.result_types, context);
  context.label_stack.pop_back();
  return valid;
}

bool Br(Index depth, Context& context, Errors& errors) {
  const auto* label = GetLabel(depth, context, errors);
  bool valid = PopTypes(MaybeDefault(label).br_types(), context, errors);
  SetUnreachable(context);
  return AllTrue(label, valid);
}

bool BrIf(Index depth, Context& context, Errors& errors) {
  bool valid = PopType(ValueType::I32, context, errors);
  const auto* label = GetLabel(depth, context, errors);
  auto label_ = MaybeDefault(label);
  return AllTrue(
      valid, label,
      PopAndPushTypes(label_.br_types(), label_.br_types(), context, errors));
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
  auto function = GetFunction(function_index, context, errors);
  auto function_type =
      GetFunctionType(MaybeDefault(function).type_index, context, errors);
  return AllTrue(function, function_type,
                 PopAndPushTypes(MaybeDefault(function_type), context, errors));
}

bool CallIndirect(const CallIndirectImmediate& immediate,
                  Context& context,
                  Errors& errors) {
  auto table_type = GetTableType(0, context, errors);
  auto function_type = GetFunctionType(immediate.index, context, errors);
  bool valid = PopType(ValueType::I32, context, errors);
  return AllTrue(table_type, function_type, valid,
                 PopAndPushTypes(MaybeDefault(function_type), context, errors));
}

bool Select(Context& context, Errors& errors) {
  bool valid = PopType(ValueType::I32, context, errors);
  auto type = PeekType(context, errors);
  const ValueType types[] = {MaybeDefault(type), MaybeDefault(type)};
  return AllTrue(valid, type, PopTypes(types, context, errors));
}

bool LocalGet(Index index, Context& context, Errors& errors) {
  auto local_type = GetLocalType(index, context, errors);
  PushType(MaybeDefault(local_type), context);
  return AllTrue(local_type);
}

bool LocalSet(Index index, Context& context, Errors& errors) {
  auto local_type = GetLocalType(index, context, errors);
  return AllTrue(local_type,
                 PopType(MaybeDefault(local_type), context, errors));
}

bool LocalTee(Index index, Context& context, Errors& errors) {
  auto local_type = GetLocalType(index, context, errors);
  const ValueType type[] = {MaybeDefault(local_type)};
  return AllTrue(local_type, PopAndPushTypes(type, type, context, errors));
}

bool GlobalGet(Index index, Context& context, Errors& errors) {
  auto global_type = GetGlobalType(index, context, errors);
  PushType(MaybeDefault(global_type).valtype, context);
  return AllTrue(global_type);
}

bool GlobalSet(Index index, Context& context, Errors& errors) {
  auto global_type = GetGlobalType(index, context, errors);
  auto type = MaybeDefault(global_type);
  bool valid = true;
  if (type.mut == Mutability::Const) {
    errors.OnError(
        format("global.set is invalid on immutable global {}", index));
    valid = false;
  }
  return AllTrue(valid, PopType(type.valtype, context, errors));
}

bool CheckAlignment(const Instruction& instruction,
                    uint32_t max_align,
                    Errors& errors) {
  if (instruction.mem_arg_immediate().align_log2 > max_align) {
    errors.OnError(format("Invalid alignment {}", instruction));
    return false;
  }
  return true;
}

bool Load(const Instruction& instruction, Context& context, Errors& errors) {
  auto memory_type = GetMemoryType(0, context, errors);
  ValueTypeSpan span;
  uint32_t max_align;
  switch (instruction.opcode) {
    case Opcode::I32Load:    span = span_i32; max_align = 2; break;
    case Opcode::I64Load:    span = span_i64; max_align = 3; break;
    case Opcode::F32Load:    span = span_f32; max_align = 2; break;
    case Opcode::F64Load:    span = span_f64; max_align = 3; break;
    case Opcode::I32Load8S:  span = span_i32; max_align = 0; break;
    case Opcode::I32Load8U:  span = span_i32; max_align = 0; break;
    case Opcode::I32Load16S: span = span_i32; max_align = 1; break;
    case Opcode::I32Load16U: span = span_i32; max_align = 1; break;
    case Opcode::I64Load8S:  span = span_i64; max_align = 0; break;
    case Opcode::I64Load8U:  span = span_i64; max_align = 0; break;
    case Opcode::I64Load16S: span = span_i64; max_align = 1; break;
    case Opcode::I64Load16U: span = span_i64; max_align = 1; break;
    case Opcode::I64Load32S: span = span_i64; max_align = 2; break;
    case Opcode::I64Load32U: span = span_i64; max_align = 2; break;
    case Opcode::V128Load:   span = span_v128; max_align = 4; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAlignment(instruction, max_align, errors);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(span_i32, span, context, errors));
}

bool Store(const Instruction& instruction, Context& context, Errors& errors) {
  auto memory_type = GetMemoryType(0, context, errors);
  ValueTypeSpan span;
  uint32_t max_align;
  switch (instruction.opcode) {
    case Opcode::I32Store:   span = span_i32_i32; max_align = 2; break;
    case Opcode::I64Store:   span = span_i32_i64; max_align = 3; break;
    case Opcode::F32Store:   span = span_i32_f32; max_align = 2; break;
    case Opcode::F64Store:   span = span_i32_f64; max_align = 3; break;
    case Opcode::I32Store8:  span = span_i32_i32; max_align = 0; break;
    case Opcode::I32Store16: span = span_i32_i32; max_align = 1; break;
    case Opcode::I64Store8:  span = span_i32_i64; max_align = 0; break;
    case Opcode::I64Store16: span = span_i32_i64; max_align = 1; break;
    case Opcode::I64Store32: span = span_i32_i64; max_align = 2; break;
    case Opcode::V128Store:  span = span_i32_v128; max_align = 4; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAlignment(instruction, max_align, errors);
  return AllTrue(memory_type, valid, PopTypes(span, context, errors));
}

bool MemorySize(Context& context, Errors& errors) {
  auto memory_type = GetMemoryType(0, context, errors);
  PushType(ValueType::I32, context);
  return AllTrue(memory_type);
}

bool MemoryGrow(Context& context, Errors& errors) {
  auto memory_type = GetMemoryType(0, context, errors);
  return AllTrue(memory_type,
                 PopAndPushTypes(span_i32, span_i32, context, errors));
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

    case Opcode::If: {
      bool valid = PopType(ValueType::I32, context, errors);
      valid &= PushLabel(LabelType::If, value.block_type_immediate(), context,
                         errors);
      return valid;
    }

    case Opcode::Else:
      return Else(context, errors);

    case Opcode::End:
      return End(context, errors);

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

    case Opcode::Select:
      return Select(context, errors);

    case Opcode::LocalGet:
      return LocalGet(value.index_immediate(), context, errors);

    case Opcode::LocalSet:
      return LocalSet(value.index_immediate(), context, errors);

    case Opcode::LocalTee:
      return LocalTee(value.index_immediate(), context, errors);

    case Opcode::GlobalGet:
      return GlobalGet(value.index_immediate(), context, errors);

    case Opcode::GlobalSet:
      return GlobalSet(value.index_immediate(), context, errors);

    case Opcode::I32Load:
    case Opcode::I64Load:
    case Opcode::F32Load:
    case Opcode::F64Load:
    case Opcode::I32Load8S:
    case Opcode::I32Load8U:
    case Opcode::I32Load16S:
    case Opcode::I32Load16U:
    case Opcode::I64Load8S:
    case Opcode::I64Load8U:
    case Opcode::I64Load16S:
    case Opcode::I64Load16U:
    case Opcode::I64Load32S:
    case Opcode::I64Load32U:
    case Opcode::V128Load:
      return Load(value, context, errors);

    case Opcode::I32Store:
    case Opcode::I64Store:
    case Opcode::F32Store:
    case Opcode::F64Store:
    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32:
    case Opcode::V128Store:
      return Store(value, context, errors);

    case Opcode::MemorySize:
      return MemorySize(context, errors);

    case Opcode::MemoryGrow:
      return MemoryGrow(context, errors);

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
    case Opcode::I32Extend8S:
    case Opcode::I32Extend16S:
      return PopAndPushTypes(span_i32, span_i32, context, errors);

    case Opcode::I64Eqz:
      return PopAndPushTypes(span_i64, span_i32, context, errors);

    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
    case Opcode::I64Extend8S:
    case Opcode::I64Extend16S:
    case Opcode::I64Extend32S:
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

    case Opcode::I64Eq:
    case Opcode::I64Ne:
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
    case Opcode::I32TruncSatF32S:
    case Opcode::I32TruncSatF32U:
      return PopAndPushTypes(span_f32, span_i32, context, errors);

    case Opcode::I32TruncF64S:
    case Opcode::I32TruncF64U:
    case Opcode::I32TruncSatF64S:
    case Opcode::I32TruncSatF64U:
      return PopAndPushTypes(span_f64, span_i32, context, errors);

    case Opcode::I64ExtendI32S:
    case Opcode::I64ExtendI32U:
      return PopAndPushTypes(span_i32, span_i64, context, errors);

    case Opcode::I64TruncF32S:
    case Opcode::I64TruncF32U:
    case Opcode::I64TruncSatF32S:
    case Opcode::I64TruncSatF32U:
      return PopAndPushTypes(span_f32, span_i64, context, errors);

    case Opcode::I64TruncF64S:
    case Opcode::I64TruncF64U:
    case Opcode::I64ReinterpretF64:
    case Opcode::I64TruncSatF64S:
    case Opcode::I64TruncSatF64U:
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

    case Opcode::V128Const:
      PushType(ValueType::V128, context);
      return true;

    case Opcode::V128Not:
    case Opcode::I8X16Neg:
    case Opcode::I16X8Neg:
    case Opcode::I32X4Neg:
    case Opcode::I64X2Neg:
    case Opcode::F32X4Abs:
    case Opcode::F32X4Neg:
    case Opcode::F32X4Sqrt:
    case Opcode::F64X2Abs:
    case Opcode::F64X2Neg:
    case Opcode::F64X2Sqrt:
    case Opcode::I32X4TruncSatF32X4S:
    case Opcode::I32X4TruncSatF32X4U:
    case Opcode::I64X2TruncSatF64X2S:
    case Opcode::I64X2TruncSatF64X2U:
    case Opcode::F32X4ConvertI32X4S:
    case Opcode::F32X4ConvertI32X4U:
    case Opcode::F64X2ConvertI64X2S:
    case Opcode::F64X2ConvertI64X2U:
      return PopAndPushTypes(span_v128, span_v128, context, errors);

    case Opcode::V128BitSelect:
      return PopAndPushTypes(span_v128_v128_v128, span_v128, context, errors);

    case Opcode::V8X16Shuffle:
    case Opcode::I8X16Eq:
    case Opcode::I8X16Ne:
    case Opcode::I8X16LtS:
    case Opcode::I8X16LtU:
    case Opcode::I8X16GtS:
    case Opcode::I8X16GtU:
    case Opcode::I8X16LeS:
    case Opcode::I8X16LeU:
    case Opcode::I8X16GeS:
    case Opcode::I8X16GeU:
    case Opcode::I16X8Eq:
    case Opcode::I16X8Ne:
    case Opcode::I16X8LtS:
    case Opcode::I16X8LtU:
    case Opcode::I16X8GtS:
    case Opcode::I16X8GtU:
    case Opcode::I16X8LeS:
    case Opcode::I16X8LeU:
    case Opcode::I16X8GeS:
    case Opcode::I16X8GeU:
    case Opcode::I32X4Eq:
    case Opcode::I32X4Ne:
    case Opcode::I32X4LtS:
    case Opcode::I32X4LtU:
    case Opcode::I32X4GtS:
    case Opcode::I32X4GtU:
    case Opcode::I32X4LeS:
    case Opcode::I32X4LeU:
    case Opcode::I32X4GeS:
    case Opcode::I32X4GeU:
    case Opcode::F32X4Eq:
    case Opcode::F32X4Ne:
    case Opcode::F32X4Lt:
    case Opcode::F32X4Gt:
    case Opcode::F32X4Le:
    case Opcode::F32X4Ge:
    case Opcode::F64X2Eq:
    case Opcode::F64X2Ne:
    case Opcode::F64X2Lt:
    case Opcode::F64X2Gt:
    case Opcode::F64X2Le:
    case Opcode::F64X2Ge:
    case Opcode::V128And:
    case Opcode::V128Or:
    case Opcode::V128Xor:
    case Opcode::I8X16Add:
    case Opcode::I8X16AddSaturateS:
    case Opcode::I8X16AddSaturateU:
    case Opcode::I8X16Sub:
    case Opcode::I8X16SubSaturateS:
    case Opcode::I8X16SubSaturateU:
    case Opcode::I8X16Mul:
    case Opcode::I16X8Add:
    case Opcode::I16X8AddSaturateS:
    case Opcode::I16X8AddSaturateU:
    case Opcode::I16X8Sub:
    case Opcode::I16X8SubSaturateS:
    case Opcode::I16X8SubSaturateU:
    case Opcode::I16X8Mul:
    case Opcode::I32X4Add:
    case Opcode::I32X4Sub:
    case Opcode::I32X4Mul:
    case Opcode::I64X2Add:
    case Opcode::I64X2Sub:
    case Opcode::F32X4Add:
    case Opcode::F32X4Sub:
    case Opcode::F32X4Mul:
    case Opcode::F32X4Div:
    case Opcode::F32X4Min:
    case Opcode::F32X4Max:
    case Opcode::F64X2Add:
    case Opcode::F64X2Sub:
    case Opcode::F64X2Mul:
    case Opcode::F64X2Div:
    case Opcode::F64X2Min:
    case Opcode::F64X2Max:
      return PopAndPushTypes(span_v128_v128, span_v128, context, errors);

    case Opcode::I8X16Splat:
    case Opcode::I16X8Splat:
    case Opcode::I32X4Splat:
      return PopAndPushTypes(span_i32, span_v128, context, errors);

    case Opcode::I64X2Splat:
      return PopAndPushTypes(span_i64, span_v128, context, errors);

    case Opcode::F32X4Splat:
      return PopAndPushTypes(span_f32, span_v128, context, errors);

    case Opcode::F64X2Splat:
      return PopAndPushTypes(span_f64, span_v128, context, errors);

    case Opcode::I8X16ExtractLaneS:
    case Opcode::I8X16ExtractLaneU:
    case Opcode::I16X8ExtractLaneS:
    case Opcode::I16X8ExtractLaneU:
    case Opcode::I32X4ExtractLane:
    case Opcode::I8X16AnyTrue:
    case Opcode::I8X16AllTrue:
    case Opcode::I16X8AnyTrue:
    case Opcode::I16X8AllTrue:
    case Opcode::I32X4AnyTrue:
    case Opcode::I32X4AllTrue:
    case Opcode::I64X2AnyTrue:
    case Opcode::I64X2AllTrue:
      return PopAndPushTypes(span_v128, span_i32, context, errors);

    case Opcode::I64X2ExtractLane:
      return PopAndPushTypes(span_v128, span_i64, context, errors);

    case Opcode::F32X4ExtractLane:
      return PopAndPushTypes(span_v128, span_f32, context, errors);

    case Opcode::F64X2ExtractLane:
      return PopAndPushTypes(span_v128, span_f64, context, errors);

    case Opcode::I8X16ReplaceLane:
    case Opcode::I16X8ReplaceLane:
    case Opcode::I32X4ReplaceLane:
    case Opcode::I8X16Shl:
    case Opcode::I8X16ShrS:
    case Opcode::I8X16ShrU:
    case Opcode::I16X8Shl:
    case Opcode::I16X8ShrS:
    case Opcode::I16X8ShrU:
    case Opcode::I32X4Shl:
    case Opcode::I32X4ShrS:
    case Opcode::I32X4ShrU:
    case Opcode::I64X2Shl:
    case Opcode::I64X2ShrS:
    case Opcode::I64X2ShrU:
      return PopAndPushTypes(span_v128_i32, span_v128, context, errors);

    case Opcode::I64X2ReplaceLane:
      return PopAndPushTypes(span_v128_i64, span_v128, context, errors);

    case Opcode::F32X4ReplaceLane:
      return PopAndPushTypes(span_v128_f32, span_v128, context, errors);

    case Opcode::F64X2ReplaceLane:
      return PopAndPushTypes(span_v128_f64, span_v128, context, errors);

    default:
      WASP_UNREACHABLE();
      return false;
  }
}

}  // namespace valid
}  // namespace wasp
