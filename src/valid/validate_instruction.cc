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
#include "wasp/binary/block_type.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_locals.h"

namespace wasp {
namespace valid {

namespace {

using binary::ValueType;
using binary::ValueTypes;
using ValueTypeSpan = span<const ValueType>;

optional<binary::FunctionType> GetBlockTypeSignature(
    binary::BlockType block_type,
    Context& context,
    Errors& errors) {
  switch (block_type) {
    case binary::BlockType::Void:
      return binary::FunctionType{};

#define WASP_V(val, Name, str)  \
  case binary::BlockType::Name: \
    return binary::FunctionType{{}, {ValueType::Name}};
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

void Push(ValueType value_type, Context& context) {
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

bool PopTypes(ValueTypeSpan expected, Context& context, Errors& errors) {
  ValueTypeSpan full_expected = expected;
  const auto& top_label = TopLabel(context);
  auto type_stack = GetTypeStack(top_label, context);
  RemovePrefixIfGreater(&type_stack, expected);
  if (top_label.unreachable) {
    RemovePrefixIfGreater(&expected, type_stack);
  }

  auto drop_count = expected.size();
  bool valid = true;
  if (expected != type_stack) {
    // TODO proper formatting of type stack
    errors.OnError(format("Expected stack to contain {}, got {}{}",
                          full_expected, top_label.unreachable ? "..." : "",
                          type_stack));
    drop_count = std::min(drop_count, type_stack.size());
    valid = false;
  }
  DropTypes(drop_count, context, errors);
  return valid;
}

bool PopType(ValueType type, Context& context, Errors& errors) {
  return PopTypes(ValueTypeSpan(&type, 1), context, errors);
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
               const binary::FunctionType& type,
               Context& context) {
  context.label_stack.emplace_back(label_type, type.param_types,
                                   type.result_types,
                                   context.type_stack.size());
  PushTypes(type.param_types, context);
}

bool PushLabel(LabelType label_type,
               binary::BlockType block_type,
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
  return label ? PopTypes(label->br_types(), context, errors) : false;
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

}  // namespace

bool Validate(const binary::Locals& value,
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

bool Validate(const binary::Instruction& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "instruction"};
  if (context.label_stack.empty()) {
    errors.OnError("Unexpected instruction after function end");
    return false;
  }

  switch (value.opcode) {
    case binary::Opcode::Unreachable:
      SetUnreachable(context);
      return true;

    case binary::Opcode::Nop:
      return true;

    case binary::Opcode::Block:
      return PushLabel(LabelType::Block, value.block_type_immediate(), context,
                       errors);

    case binary::Opcode::Loop:
      return PushLabel(LabelType::Loop, value.block_type_immediate(), context,
                       errors);

    case binary::Opcode::End:
      return PopLabel(context, errors);

    case binary::Opcode::Br: {
      bool valid = Br(value.index_immediate(), context, errors);
      SetUnreachable(context);
      return valid;
    }

    case binary::Opcode::BrIf:
      return BrIf(value.index_immediate(), context, errors);

    case binary::Opcode::Drop:
      return DropTypes(1, context, errors);

    case binary::Opcode::I32Const:
      Push(ValueType::I32, context);
      return true;

    case binary::Opcode::I64Const:
      Push(ValueType::I64, context);
      return true;

    case binary::Opcode::F32Const:
      Push(ValueType::F32, context);
      return true;

    case binary::Opcode::F64Const:
      Push(ValueType::F64, context);
      return true;

    default:
      WASP_UNREACHABLE();
      return false;
  }
}

}  // namespace valid
}  // namespace wasp
