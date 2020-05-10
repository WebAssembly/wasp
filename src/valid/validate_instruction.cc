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

#include <limits>

#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/errors_nop.h"
#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/formatters.h"
#include "wasp/valid/validate.h"

namespace wasp {
namespace valid {

namespace {

using namespace ::wasp::binary;

#define STACK_TYPE_SPANS(V)                                      \
  V(i32, StackType::I32)                                         \
  V(i64, StackType::I64)                                         \
  V(f32, StackType::F32)                                         \
  V(f64, StackType::F64)                                         \
  V(v128, StackType::V128)                                       \
  V(exnref, StackType::Exnref)                                   \
  V(i32_i32, StackType::I32, StackType::I32)                     \
  V(i32_i64, StackType::I32, StackType::I64)                     \
  V(i32_f32, StackType::I32, StackType::F32)                     \
  V(i32_f64, StackType::I32, StackType::F64)                     \
  V(i32_v128, StackType::I32, StackType::V128)                   \
  V(i64_i64, StackType::I64, StackType::I64)                     \
  V(f32_f32, StackType::F32, StackType::F32)                     \
  V(f64_f64, StackType::F64, StackType::F64)                     \
  V(v128_i32, StackType::V128, StackType::I32)                   \
  V(v128_i64, StackType::V128, StackType::I64)                   \
  V(v128_f32, StackType::V128, StackType::F32)                   \
  V(v128_f64, StackType::V128, StackType::F64)                   \
  V(v128_v128, StackType::V128, StackType::V128)                 \
  V(i32_i32_i32, StackType::I32, StackType::I32, StackType::I32) \
  V(i32_i32_i64, StackType::I32, StackType::I32, StackType::I64) \
  V(i32_i64_i64, StackType::I32, StackType::I64, StackType::I64) \
  V(v128_v128_v128, StackType::V128, StackType::V128, StackType::V128)

#define WASP_V(name, ...)                         \
  const StackType array_##name[] = {__VA_ARGS__}; \
  const StackTypeSpan span_##name{array_##name};
STACK_TYPE_SPANS(WASP_V)

#undef WASP_V
#undef STACK_TYPE_SPANS

bool AllTrue() { return true; }

template <typename T, typename... Args>
bool AllTrue(T first, Args... rest) {
  return !!first & AllTrue(rest...);
}

optional<FunctionType> GetFunctionType(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.types.size(), "type index", context)) {
    return nullopt;
  }
  return context.types[index].type;
}

optional<FunctionType> GetBlockTypeSignature(BlockType block_type,
                                             Context& context) {
  switch (block_type) {
    case BlockType::Void:
      return FunctionType{};

#define WASP_V(val, Name, str) \
  case BlockType::Name:        \
    return FunctionType{{}, {ValueType::Name}};
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/def/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V

    default:
      return GetFunctionType(Index(block_type), context);
  }
}

Label& TopLabel(Context& context) {
  assert(!context.label_stack.empty());
  return context.label_stack.back();
}

StackTypeSpan GetTypeStack(Context& context) {
  return StackTypeSpan{context.type_stack}.subspan(
      TopLabel(context).type_stack_limit);
}

optional<Function> GetFunction(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.functions.size(), "function index",
                     context)) {
    return nullopt;
  }
  return context.functions[index];
}

optional<TableType> GetTableType(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.tables.size(), "table index", context)) {
    return nullopt;
  }
  return context.tables[index];
}

optional<MemoryType> GetMemoryType(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.memories.size(), "memory index", context)) {
    return nullopt;
  }
  return context.memories[index];
}

optional<GlobalType> GetGlobalType(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.globals.size(), "global index", context)) {
    return nullopt;
  }
  return context.globals[index];
}

optional<EventType> GetEventType(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.events.size(), "event index", context)) {
    return nullopt;
  }
  return context.events[index];
}

optional<ReferenceType> GetElementSegmentType(At<Index> index,
                                              Context& context) {
  if (!ValidateIndex(index, context.element_segments.size(),
                     "element segment index", context)) {
    return nullopt;
  }
  return context.element_segments[index];
}

optional<StackType> GetLocalType(At<Index> index, Context& context) {
  if (!ValidateIndex(index, context.GetLocalCount(), "local index", context)) {
    return nullopt;
  }
  auto local_type = context.GetLocalType(index);
  if (!local_type) {
    return nullopt;
  }
  return StackType(*local_type);
}

bool CheckDataSegment(At<Index> index, Context& context) {
  return ValidateIndex(index, context.declared_data_count.value_or(0),
                       "data segment index", context);
}

StackType MaybeDefault(optional<StackType> value) {
  return value.value_or(StackType::I32);
}

Function MaybeDefault(optional<Function> value) {
  return value.value_or(Function{0});
}

FunctionType MaybeDefault(optional<FunctionType> value) {
  return value.value_or(FunctionType{});
}

TableType MaybeDefault(optional<TableType> value) {
  return value.value_or(TableType{Limits{0}, ReferenceType::Funcref});
}

GlobalType MaybeDefault(optional<GlobalType> value) {
  return value.value_or(GlobalType{ValueType::I32, Mutability::Const});
}

EventType MaybeDefault(optional<EventType> value) {
  return value.value_or(EventType{EventAttribute::Exception, 0});
}

ReferenceType MaybeDefault(optional<ReferenceType> value) {
  return value.value_or(ReferenceType::Externref);
}

Label MaybeDefault(const Label* value) {
  return value ? *value : Label{LabelType::Block, {}, {}, 0};
}

optional<StackType> PeekType(Location loc, Context& context) {
  auto type_stack = GetTypeStack(context);
  if (type_stack.empty()) {
    if (!TopLabel(context).unreachable) {
      context.errors->OnError(loc, "Expected stack to have 1 value, got 0");
      return nullopt;
    }
    return StackType::Any;
  }
  return type_stack[type_stack.size() - 1];
}

void PushType(StackType stack_type, Context& context) {
  context.type_stack.push_back(stack_type);
}

void PushTypes(StackTypeSpan stack_types, Context& context) {
  context.type_stack.insert(context.type_stack.end(), stack_types.begin(),
                            stack_types.end());
}

void RemovePrefixIfGreater(StackTypeSpan* lhs, StackTypeSpan rhs) {
  if (lhs->size() > rhs.size()) {
    remove_prefix(lhs, lhs->size() - rhs.size());
  }
}

bool TypesMatch(StackType expected, StackType actual) {
  // Types are the same.
  if (expected == actual) {
    return true;
  }

  // One of the types is "any" (i.e. universal supertype or subtype)
  if (expected == StackType::Any || actual == StackType::Any) {
    return true;
  }

  return false;
}

bool TypesMatch(StackTypeSpan expected, StackTypeSpan actual) {
  if (expected.size() != actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    StackType etype = *eiter;
    StackType atype = *aiter;
    if (!TypesMatch(etype, atype)) {
      return false;
    }
  }
  return true;
}

bool CheckTypes(Location loc, StackTypeSpan expected, Context& context) {
  StackTypeSpan full_expected = expected;
  const auto& top_label = TopLabel(context);
  auto type_stack = GetTypeStack(context);
  RemovePrefixIfGreater(&type_stack, expected);
  if (top_label.unreachable) {
    RemovePrefixIfGreater(&expected, type_stack);
  }

  if (!TypesMatch(expected, type_stack)) {
    // TODO proper formatting of type stack
    context.errors->OnError(
        loc, format("Expected stack to contain {}, got {}{}", full_expected,
                    top_label.unreachable ? "..." : "", type_stack));
    return false;
  }
  return true;
}

bool CheckResultTypes(Location loc,
                      StackTypeSpan caller,
                      StackTypeSpan callee,
                      Context& context) {
  if (!TypesMatch(caller, callee)) {
    context.errors->OnError(
        loc,
        format("Callee's result types {} must equal caller's result types {}",
               callee, caller));
    return false;
  }
  return true;
}

void ResetTypeStackToLimit(Context& context) {
  context.type_stack.resize(TopLabel(context).type_stack_limit);
}

bool DropTypes(Location loc,
               size_t count,
               Context& context,
               bool print_errors = true) {
  const auto& top_label = TopLabel(context);
  auto type_stack_size = context.type_stack.size() - top_label.type_stack_limit;
  if (count > type_stack_size) {
    if (print_errors) {
      context.errors->OnError(
          loc, format("Expected stack to contain {} value{}, got {}", count,
                      count == 1 ? "" : "s", type_stack_size));
    }
    ResetTypeStackToLimit(context);
    return top_label.unreachable;
  }
  context.type_stack.resize(context.type_stack.size() - count);
  return true;
}

bool PopTypes(Location loc, StackTypeSpan expected, Context& context) {
  bool valid = CheckTypes(loc, expected, context);
  valid &= DropTypes(loc, expected.size(), context, false);
  return valid;
}

bool PopType(Location loc, StackType type, Context& context) {
  return PopTypes(loc, StackTypeSpan(&type, 1), context);
}

bool PopAndPushTypes(Location loc,
                     StackTypeSpan param_types,
                     StackTypeSpan result_types,
                     Context& context) {
  bool valid = PopTypes(loc, param_types, context);
  PushTypes(result_types, context);
  return valid;
}

bool PopAndPushTypes(Location loc,
                     const FunctionType& function_type,
                     Context& context) {
  return PopAndPushTypes(loc, ToStackTypeList(function_type.param_types),
                         ToStackTypeList(function_type.result_types), context);
}

void SetUnreachable(Context& context) {
  auto& top_label = TopLabel(context);
  top_label.unreachable = true;
  ResetTypeStackToLimit(context);
}

Label* GetLabel(At<Index> depth, Context& context) {
  if (depth >= context.label_stack.size()) {
    context.errors->OnError(
        depth.loc(), format("Invalid label {}, must be less than {}", depth,
                            context.label_stack.size()));
    return nullptr;
  }
  return &context.label_stack[context.label_stack.size() - depth - 1];
}

bool PushLabel(Location loc,
               LabelType label_type,
               const FunctionType& type,
               Context& context) {
  auto stack_param_types = ToStackTypeList(type.param_types);
  auto stack_result_types = ToStackTypeList(type.result_types);
  bool valid = PopTypes(loc, stack_param_types, context);
  context.label_stack.emplace_back(label_type, stack_param_types,
                                   stack_result_types,
                                   context.type_stack.size());
  PushTypes(stack_param_types, context);
  return valid;
}

bool PushLabel(Location loc,
               LabelType label_type,
               BlockType block_type,
               Context& context) {
  auto sig = GetBlockTypeSignature(block_type, context);
  if (!sig) {
    return false;
  }
  return PushLabel(loc, label_type, *sig, context);
}

bool CheckTypeStackEmpty(Location loc, Context& context) {
  const auto& top_label = TopLabel(context);
  if (context.type_stack.size() != top_label.type_stack_limit) {
    context.errors->OnError(
        loc, format("Expected empty stack, got {}", GetTypeStack(context)));
    return false;
  }
  return true;
}

bool Catch(Location loc, Context& context) {
  auto& top_label = TopLabel(context);
  if (top_label.label_type != LabelType::Try) {
    context.errors->OnError(loc, "Got catch instruction without try");
    return false;
  }
  bool valid = PopTypes(loc, top_label.result_types, context);
  valid &= CheckTypeStackEmpty(loc, context);
  ResetTypeStackToLimit(context);
  PushTypes(span_exnref, context);
  top_label.label_type = LabelType::Catch;
  top_label.unreachable = false;
  return valid;
}

bool Else(Location loc, Context& context) {
  auto& top_label = TopLabel(context);
  if (top_label.label_type != LabelType::If) {
    context.errors->OnError(loc, "Got else instruction without if");
    return false;
  }
  bool valid = PopTypes(loc, top_label.result_types, context);
  valid &= CheckTypeStackEmpty(loc, context);
  ResetTypeStackToLimit(context);
  PushTypes(top_label.param_types, context);
  top_label.label_type = LabelType::Else;
  top_label.unreachable = false;
  return valid;
}

bool End(Location loc, Context& context) {
  auto& top_label = TopLabel(context);
  bool valid = true;
  if (top_label.label_type == LabelType::If) {
    valid &= Else(loc, context);
  }
  valid &= PopTypes(loc, top_label.result_types, context);
  valid &= CheckTypeStackEmpty(loc, context);
  ResetTypeStackToLimit(context);
  PushTypes(top_label.result_types, context);
  context.label_stack.pop_back();
  return valid;
}

bool Br(Location loc, At<Index> depth, Context& context) {
  const auto* label = GetLabel(depth, context);
  bool valid = PopTypes(loc, MaybeDefault(label).br_types(), context);
  SetUnreachable(context);
  return AllTrue(label, valid);
}

bool BrIf(Location loc, At<Index> depth, Context& context) {
  bool valid = PopType(loc, StackType::I32, context);
  const auto* label = GetLabel(depth, context);
  auto label_ = MaybeDefault(label);
  return AllTrue(
      valid, label,
      PopAndPushTypes(loc, label_.br_types(), label_.br_types(), context));
}

bool BrTable(Location loc,
             const At<BrTableImmediate>& immediate,
             Context& context) {
  bool valid = PopType(loc, StackType::I32, context);
  optional<StackTypeSpan> br_types;
  auto handle_target = [&](At<Index> target) {
    const auto* label = GetLabel(target, context);
    if (label) {
      StackTypeSpan label_br_types{label->br_types()};
      if (br_types) {
        if (*br_types != label_br_types) {
          context.errors->OnError(
              target.loc(),
              format("br_table labels must have the same signature; expected "
                     "{}, got {}",
                     *br_types, label_br_types));
          valid = false;
        }
      } else {
        br_types = label_br_types;
        valid &= CheckTypes(target.loc(), *br_types, context);
      }
    } else {
      valid = false;
    }
  };

  handle_target(immediate->default_target);
  for (auto target : immediate->targets) {
    handle_target(target);
  }
  SetUnreachable(context);
  return valid;
}

bool Call(Location loc, At<Index> function_index, Context& context) {
  auto function = GetFunction(function_index, context);
  auto function_type =
      GetFunctionType(MaybeDefault(function).type_index, context);
  return AllTrue(function, function_type,
                 PopAndPushTypes(loc, MaybeDefault(function_type), context));
}

bool CallIndirect(Location loc,
                  const At<CallIndirectImmediate>& immediate,
                  Context& context) {
  auto table_type = GetTableType(immediate->table_index, context);
  auto function_type = GetFunctionType(immediate->index, context);
  bool valid = PopType(loc, StackType::I32, context);
  return AllTrue(table_type, function_type, valid,
                 PopAndPushTypes(loc, MaybeDefault(function_type), context));
}

bool Select(Location loc, Context& context) {
  bool valid = PopType(loc, StackType::I32, context);
  auto type = MaybeDefault(PeekType(loc, context));
  if (!(type == StackType::I32 || type == StackType::I64 ||
        type == StackType::F32 || type == StackType::F64 ||
        type == StackType::Any)) {
    context.errors->OnError(
        loc, format("select instruction without expected type can only be used "
                    "with i32, i64, f32, f64; got {}",
                    type));
    return false;
  }
  const StackType pop_types[] = {type, type};
  const StackType push_type[] = {type};
  return AllTrue(valid, PopAndPushTypes(loc, pop_types, push_type, context));
}

bool SelectT(Location loc,
             const At<ValueTypeList>& value_types,
             Context& context) {
  bool valid = PopType(loc, StackType::I32, context);
  if (value_types->size() != 1) {
    context.errors->OnError(
        value_types.loc(),
        format(
            "select instruction must have types immediate with size 1, got {}",
            value_types->size()));
    return false;
  }
  StackTypeList stack_types = ToStackTypeList(value_types);
  StackType type = stack_types[0];
  const StackType pop_types[] = {type, type};
  const StackType push_type[] = {type};
  return AllTrue(valid, PopAndPushTypes(loc, pop_types, push_type, context));
}

bool LocalGet(At<Index> index, Context& context) {
  auto local_type = GetLocalType(index, context);
  PushType(MaybeDefault(local_type), context);
  return AllTrue(local_type);
}

bool LocalSet(Location loc, At<Index> index, Context& context) {
  auto local_type = GetLocalType(index, context);
  return AllTrue(local_type,
                 PopType(loc, MaybeDefault(local_type), context));
}

bool LocalTee(Location loc, At<Index> index, Context& context) {
  auto local_type = GetLocalType(index, context);
  const StackType type[] = {MaybeDefault(local_type)};
  return AllTrue(local_type, PopAndPushTypes(loc, type, type, context));
}

bool GlobalGet(At<Index> index, Context& context) {
  auto global_type = GetGlobalType(index, context);
  PushType(StackType(*MaybeDefault(global_type).valtype), context);
  return AllTrue(global_type);
}

bool GlobalSet(Location loc, At<Index> index, Context& context) {
  auto global_type = GetGlobalType(index, context);
  auto type = MaybeDefault(global_type);
  bool valid = true;
  if (type.mut == Mutability::Const) {
    context.errors->OnError(
        index.loc(),
        format("global.set is invalid on immutable global {}", index));
    valid = false;
  }
  return AllTrue(valid, PopType(loc, StackType(*type.valtype), context));
}

bool TableGet(Location loc, At<Index> index, Context& context) {
  auto table_type = GetTableType(index, context);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType type[] = {stack_type};
  return AllTrue(table_type, PopAndPushTypes(loc, span_i32, type, context));
}

bool TableSet(Location loc, At<Index> index, Context& context) {
  auto table_type = GetTableType(index, context);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType types[] = {StackType::I32, stack_type};
  return AllTrue(table_type, PopTypes(loc, types, context));
}

bool RefFunc(Location loc, At<Index> index, Context& context) {
  if (context.declared_functions.find(index) ==
      context.declared_functions.end()) {
    context.errors->OnError(loc,
                            format("Undeclared function reference {}", index));
    return false;
  }
  PushType(StackType::Funcref, context);
  return true;
}

bool CheckAlignment(const At<Instruction>& instruction,
                    u32 max_align,
                    Context& context) {
  if (instruction->mem_arg_immediate()->align_log2 > max_align) {
    context.errors->OnError(instruction.loc(),
                            format("Invalid alignment {}", instruction));
    return false;
  }
  return true;
}

bool Load(Location loc, const At<Instruction>& instruction, Context& context) {
  auto memory_type = GetMemoryType(0, context);
  StackTypeSpan span;
  u32 max_align;
  switch (instruction->opcode) {
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
    case Opcode::V8X16LoadSplat:   span = span_v128; max_align = 0; break;
    case Opcode::V16X8LoadSplat:   span = span_v128; max_align = 1; break;
    case Opcode::V32X4LoadSplat:   span = span_v128; max_align = 2; break;
    case Opcode::V64X2LoadSplat:   span = span_v128; max_align = 3; break;
    case Opcode::I16X8Load8X8S:    span = span_v128; max_align = 3; break;
    case Opcode::I16X8Load8X8U:    span = span_v128; max_align = 3; break;
    case Opcode::I32X4Load16X4S:   span = span_v128; max_align = 3; break;
    case Opcode::I32X4Load16X4U:   span = span_v128; max_align = 3; break;
    case Opcode::I64X2Load32X2S:   span = span_v128; max_align = 3; break;
    case Opcode::I64X2Load32X2U:   span = span_v128; max_align = 3; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAlignment(instruction, max_align, context);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(loc, span_i32, span, context));
}

bool Store(Location loc, const At<Instruction>& instruction, Context& context) {
  auto memory_type = GetMemoryType(0, context);
  StackTypeSpan span;
  u32 max_align;
  switch (instruction->opcode) {
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

  bool valid = CheckAlignment(instruction, max_align, context);
  return AllTrue(memory_type, valid, PopTypes(loc, span, context));
}

bool MemorySize(Context& context) {
  auto memory_type = GetMemoryType(0, context);
  PushType(StackType::I32, context);
  return AllTrue(memory_type);
}

bool MemoryGrow(Location loc, Context& context) {
  auto memory_type = GetMemoryType(0, context);
  return AllTrue(memory_type,
                 PopAndPushTypes(loc, span_i32, span_i32, context));
}

bool MemoryInit(Location loc,
                const At<InitImmediate>& immediate,
                Context& context) {
  auto memory_type = GetMemoryType(0, context);
  bool valid = CheckDataSegment(immediate->segment_index, context);
  return AllTrue(memory_type, valid, PopTypes(loc, span_i32_i32_i32, context));
}

bool DataDrop(At<Index> segment_index, Context& context) {
  return CheckDataSegment(segment_index, context);
}

bool MemoryCopy(Location loc,
                const At<CopyImmediate>& immediate,
                Context& context) {
  auto memory_type = GetMemoryType(0, context);
  return AllTrue(memory_type, PopTypes(loc, span_i32_i32_i32, context));
}

bool MemoryFill(Location loc, Context& context) {
  auto memory_type = GetMemoryType(0, context);
  return AllTrue(memory_type, PopTypes(loc, span_i32_i32_i32, context));
}

bool CheckReferenceType(ReferenceType expected,
                        At<ReferenceType> actual,
                        Context& context) {
  if (!TypesMatch(ToStackType(expected), ToStackType(actual))) {
    context.errors->OnError(
        actual.loc(),
        format("Expected reference type {}, got {}", expected, actual));
    return false;
  }
  return true;
}

bool TableInit(Location loc,
               const At<InitImmediate>& immediate,
               Context& context) {
  auto table_type = GetTableType(immediate->dst_index, context);
  auto elemtype = GetElementSegmentType(immediate->segment_index, context);
  bool valid = CheckReferenceType(MaybeDefault(table_type).elemtype,
                                  MaybeDefault(elemtype), context);
  return AllTrue(table_type, elemtype, valid,
                 PopTypes(loc, span_i32_i32_i32, context));
}

bool ElemDrop(At<Index> segment_index, Context& context) {
  auto elem_type = GetElementSegmentType(segment_index, context);
  return AllTrue(elem_type);
}

bool TableCopy(Location loc,
               const At<CopyImmediate>& immediate,
               Context& context) {
  auto dst_table_type = GetTableType(immediate->dst_index, context);
  auto src_table_type = GetTableType(immediate->src_index, context);
  bool valid =
      CheckReferenceType(MaybeDefault(dst_table_type).elemtype,
                         MaybeDefault(src_table_type).elemtype, context);
  return AllTrue(dst_table_type, src_table_type, valid,
                 PopTypes(loc, span_i32_i32_i32, context));
}

bool TableGrow(Location loc, At<Index> index, Context& context) {
  auto table_type = GetTableType(index, context);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType types[] = {stack_type, StackType::I32};
  return AllTrue(table_type, PopAndPushTypes(loc, types, span_i32, context));
}

bool TableSize(At<Index> index, Context& context) {
  auto table_type = GetTableType(0, context);
  PushTypes(span_i32, context);
  return AllTrue(table_type);
}

bool TableFill(Location loc, At<Index> index, Context& context) {
  auto table_type = GetTableType(index, context);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType types[] = {StackType::I32, stack_type, StackType::I32};
  return AllTrue(table_type, PopTypes(loc, types, context));
}

bool CheckAtomicAlignment(const At<Instruction>& instruction,
                          u32 align,
                          Context& context) {
  if (instruction->mem_arg_immediate()->align_log2 != align) {
    context.errors->OnError(instruction.loc(),
                            format("Invalid atomic alignment {}", instruction));
    return false;
  }
  return true;
}

bool CheckSharedMemory(const At<Instruction>& instruction,
                       const optional<MemoryType>& memory_type,
                       Context& context) {
  if (memory_type && memory_type->limits->shared == Shared::No) {
    context.errors->OnError(
        instruction.loc(), format("Memory must be shared for {}", instruction));
    return false;
  }
  return true;
}

bool MemoryAtomicNotify(Location loc,
                        const At<Instruction>& instruction,
                        Context& context) {
  const u32 align = 2;
  auto memory_type = GetMemoryType(0, context);
  bool valid = CheckAtomicAlignment(instruction, align, context);
  valid &= CheckSharedMemory(instruction, memory_type, context);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(loc, span_i32_i32, span_i32, context));
}

bool MemoryAtomicWait(Location loc,
                      const At<Instruction>& instruction,
                      Context& context) {
  auto memory_type = GetMemoryType(0, context);
  StackTypeSpan span;
  u32 align;
  switch (instruction->opcode) {
    case Opcode::MemoryAtomicWait32: span = span_i32_i32_i64; align = 2; break;
    case Opcode::MemoryAtomicWait64: span = span_i32_i64_i64; align = 3; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAtomicAlignment(instruction, align, context);
  valid &= CheckSharedMemory(instruction, memory_type, context);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(loc, span, span_i32, context));
}

bool AtomicLoad(Location loc,
                const At<Instruction>& instruction,
                Context& context) {
  auto memory_type = GetMemoryType(0, context);
  StackTypeSpan span;
  u32 align;
  switch (instruction->opcode) {
    case Opcode::I32AtomicLoad:    span = span_i32; align = 2; break;
    case Opcode::I64AtomicLoad:    span = span_i64; align = 3; break;
    case Opcode::I32AtomicLoad8U:  span = span_i32; align = 0; break;
    case Opcode::I32AtomicLoad16U: span = span_i32; align = 1; break;
    case Opcode::I64AtomicLoad8U:  span = span_i64; align = 0; break;
    case Opcode::I64AtomicLoad16U: span = span_i64; align = 1; break;
    case Opcode::I64AtomicLoad32U: span = span_i64; align = 2; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAtomicAlignment(instruction, align, context);
  valid &= CheckSharedMemory(instruction, memory_type, context);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(loc, span_i32, span, context));
}

bool AtomicStore(Location loc,
                 const At<Instruction>& instruction,
                 Context& context) {
  auto memory_type = GetMemoryType(0, context);
  StackTypeSpan span;
  u32 align;
  switch (instruction->opcode) {
    case Opcode::I32AtomicStore:   span = span_i32_i32; align = 2; break;
    case Opcode::I64AtomicStore:   span = span_i32_i64; align = 3; break;
    case Opcode::I32AtomicStore8:  span = span_i32_i32; align = 0; break;
    case Opcode::I32AtomicStore16: span = span_i32_i32; align = 1; break;
    case Opcode::I64AtomicStore8:  span = span_i32_i64; align = 0; break;
    case Opcode::I64AtomicStore16: span = span_i32_i64; align = 1; break;
    case Opcode::I64AtomicStore32: span = span_i32_i64; align = 2; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAtomicAlignment(instruction, align, context);
  valid &= CheckSharedMemory(instruction, memory_type, context);
  return AllTrue(memory_type, valid, PopTypes(loc, span, context));
}

bool AtomicRmw(Location loc,
               const At<Instruction>& instruction,
               Context& context) {
  auto memory_type = GetMemoryType(0, context);
  StackTypeSpan params, results;
  u32 align;
  switch (instruction->opcode) {
    case Opcode::I32AtomicRmwAdd:
    case Opcode::I32AtomicRmwSub:
    case Opcode::I32AtomicRmwAnd:
    case Opcode::I32AtomicRmwOr:
    case Opcode::I32AtomicRmwXor:
    case Opcode::I32AtomicRmwXchg:    align = 2; goto rmw32;
    case Opcode::I32AtomicRmw16AddU:
    case Opcode::I32AtomicRmw16SubU:
    case Opcode::I32AtomicRmw16AndU:
    case Opcode::I32AtomicRmw16OrU:
    case Opcode::I32AtomicRmw16XorU:
    case Opcode::I32AtomicRmw16XchgU: align = 1; goto rmw32;
    case Opcode::I32AtomicRmw8AddU:
    case Opcode::I32AtomicRmw8SubU:
    case Opcode::I32AtomicRmw8AndU:
    case Opcode::I32AtomicRmw8OrU:
    case Opcode::I32AtomicRmw8XorU:
    case Opcode::I32AtomicRmw8XchgU:  align = 0; goto rmw32;

    rmw32:
      params = span_i32_i32, results = span_i32;
      break;

    case Opcode::I64AtomicRmwAdd:
    case Opcode::I64AtomicRmwSub:
    case Opcode::I64AtomicRmwAnd:
    case Opcode::I64AtomicRmwOr:
    case Opcode::I64AtomicRmwXor:
    case Opcode::I64AtomicRmwXchg:    align = 3; goto rmw64;
    case Opcode::I64AtomicRmw8AddU:
    case Opcode::I64AtomicRmw8SubU:
    case Opcode::I64AtomicRmw8AndU:
    case Opcode::I64AtomicRmw8OrU:
    case Opcode::I64AtomicRmw8XorU:
    case Opcode::I64AtomicRmw8XchgU:  align = 0; goto rmw64;
    case Opcode::I64AtomicRmw16AddU:
    case Opcode::I64AtomicRmw16SubU:
    case Opcode::I64AtomicRmw16AndU:
    case Opcode::I64AtomicRmw16OrU:
    case Opcode::I64AtomicRmw16XorU:
    case Opcode::I64AtomicRmw16XchgU: align = 1; goto rmw64;
    case Opcode::I64AtomicRmw32AddU:
    case Opcode::I64AtomicRmw32SubU:
    case Opcode::I64AtomicRmw32AndU:
    case Opcode::I64AtomicRmw32OrU:
    case Opcode::I64AtomicRmw32XorU:
    case Opcode::I64AtomicRmw32XchgU: align = 2; goto rmw64;

    rmw64:
      params = span_i32_i64, results = span_i64;
      break;

    case Opcode::I32AtomicRmwCmpxchg:    align = 2; goto cmpxchg32;
    case Opcode::I32AtomicRmw8CmpxchgU:  align = 0; goto cmpxchg32;
    case Opcode::I32AtomicRmw16CmpxchgU: align = 1; goto cmpxchg32;

    cmpxchg32:
      params = span_i32_i32_i32, results = span_i32;
      break;

    case Opcode::I64AtomicRmwCmpxchg:    align = 3; goto cmpxchg64;
    case Opcode::I64AtomicRmw8CmpxchgU:  align = 0; goto cmpxchg64;
    case Opcode::I64AtomicRmw16CmpxchgU: align = 1; goto cmpxchg64;
    case Opcode::I64AtomicRmw32CmpxchgU: align = 2; goto cmpxchg64;

    cmpxchg64:
      params = span_i32_i64_i64, results = span_i64;
      break;

    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAtomicAlignment(instruction, align, context);
  valid &= CheckSharedMemory(instruction, memory_type, context);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(loc, params, results, context));
}

bool ReturnCall(Location loc, At<Index> function_index, Context& context) {
  auto function = GetFunction(function_index, context);
  auto function_type =
      GetFunctionType(MaybeDefault(function).type_index, context);
  auto* label = GetLabel(context.label_stack.size() - 1, context);
  bool valid = CheckResultTypes(
      loc, ToStackTypeList(MaybeDefault(function_type).result_types),
      MaybeDefault(label).br_types(), context);
  valid &= PopTypes(
      loc, ToStackTypeList(MaybeDefault(function_type).param_types), context);
  SetUnreachable(context);
  return AllTrue(function, function_type, valid);
}

bool ReturnCallIndirect(Location loc,
                        const At<CallIndirectImmediate>& immediate,
                        Context& context) {
  auto table_type = GetTableType(0, context);
  auto function_type = GetFunctionType(immediate->index, context);
  auto* label = GetLabel(context.label_stack.size() - 1, context);
  bool valid = CheckResultTypes(
      loc, ToStackTypeList(MaybeDefault(function_type).result_types),
      MaybeDefault(label).br_types(), context);
  valid &= PopType(loc, StackType::I32, context);
  valid &= PopTypes(
      loc, ToStackTypeList(MaybeDefault(function_type).param_types), context);
  SetUnreachable(context);
  return AllTrue(table_type, function_type, valid);
}

bool Throw(Location loc, At<Index> index, Context& context) {
  auto event_type = GetEventType(index, context);
  auto function_type =
      GetFunctionType(MaybeDefault(event_type).type_index, context);
  bool valid = PopTypes(
      loc, ToStackTypeList(MaybeDefault(function_type).param_types), context);
  SetUnreachable(context);
  return AllTrue(event_type, function_type, valid);
}

bool Rethrow(Location loc, Context& context) {
  bool valid = PopTypes(loc, span_exnref, context);
  SetUnreachable(context);
  return valid;
}

bool BrOnExn(Location loc,
             const At<BrOnExnImmediate>& immediate,
             Context& context) {
  auto event_type = GetEventType(immediate->event_index, context);
  auto function_type =
      GetFunctionType(MaybeDefault(event_type).type_index, context);
  auto* label = GetLabel(immediate->target, context);
  bool valid =
      TypesMatch(ToStackTypeList(MaybeDefault(function_type).param_types),
                 MaybeDefault(label).br_types());
  valid &= PopAndPushTypes(loc, span_exnref, span_exnref, context);
  return AllTrue(event_type, function_type, label, valid);
}

}  // namespace

bool Validate(const At<Locals>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "locals"};
  if (!context.AppendLocals(value->count, value->type)) {
    const Index max = std::numeric_limits<Index>::max();
    context.errors->OnError(
        value.loc(),
        format("Too many locals; max is {}, got {}", max,
               static_cast<u64>(context.GetLocalCount()) + value->count));
    return false;
  }
  return true;
}

bool Validate(const At<Instruction>& value, Context& context) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "instruction"};
  if (context.label_stack.empty()) {
    context.errors->OnError(value.loc(),
                            "Unexpected instruction after function end");
    return false;
  }

  Location loc = value.loc();

  StackTypeSpan params, results;
  switch (value->opcode) {
    case Opcode::Unreachable:
      SetUnreachable(context);
      return true;

    case Opcode::Nop:
      return true;

    case Opcode::Block:
      return PushLabel(loc, LabelType::Block, value->block_type_immediate(),
                       context);

    case Opcode::Loop:
      return PushLabel(loc, LabelType::Loop, value->block_type_immediate(),
                       context);

    case Opcode::If: {
      bool valid = PopType(loc, StackType::I32, context);
      valid &=
          PushLabel(loc, LabelType::If, value->block_type_immediate(), context);
      return valid;
    }

    case Opcode::Else:
      return Else(loc, context);

    case Opcode::End:
      return End(loc, context);

    case Opcode::Try:
      return PushLabel(loc, LabelType::Try, value->block_type_immediate(),
                       context);

    case Opcode::Catch:
      return Catch(loc, context);

    case Opcode::Throw:
      return Throw(loc, value->index_immediate(), context);

    case Opcode::Rethrow:
      return Rethrow(loc, context);

    case Opcode::BrOnExn:
      return BrOnExn(loc, value->br_on_exn_immediate(), context);

    case Opcode::Br:
      return Br(loc, value->index_immediate(), context);

    case Opcode::BrIf:
      return BrIf(loc, value->index_immediate(), context);

    case Opcode::BrTable:
      return BrTable(loc, value->br_table_immediate(), context);

    case Opcode::Return:
      return Br(loc, context.label_stack.size() - 1, context);

    case Opcode::Call:
      return Call(loc, value->index_immediate(), context);

    case Opcode::CallIndirect:
      return CallIndirect(loc, value->call_indirect_immediate(), context);

    case Opcode::Drop:
      return DropTypes(loc, 1, context);

    case Opcode::Select:
      return Select(loc, context);

    case Opcode::SelectT:
      return SelectT(loc, value->value_type_list_immediate(), context);

    case Opcode::LocalGet:
      return LocalGet(value->index_immediate(), context);

    case Opcode::LocalSet:
      return LocalSet(loc, value->index_immediate(), context);

    case Opcode::LocalTee:
      return LocalTee(loc, value->index_immediate(), context);

    case Opcode::GlobalGet:
      return GlobalGet(value->index_immediate(), context);

    case Opcode::GlobalSet:
      return GlobalSet(loc, value->index_immediate(), context);

    case Opcode::TableGet:
      return TableGet(loc, value->index_immediate(), context);

    case Opcode::TableSet:
      return TableSet(loc, value->index_immediate(), context);

    case Opcode::RefNull:
      PushType(ToStackType(value->reference_type_immediate()), context);
      return true;

    case Opcode::RefIsNull: {
      bool valid =
          PopType(loc, ToStackType(value->reference_type_immediate()), context);
      PushType(StackType::I32, context);
      return valid;
    }

    case Opcode::RefFunc:
      return RefFunc(loc, value->index_immediate(), context);

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
    case Opcode::V8X16LoadSplat:
    case Opcode::V16X8LoadSplat:
    case Opcode::V32X4LoadSplat:
    case Opcode::V64X2LoadSplat:
    case Opcode::I16X8Load8X8S:
    case Opcode::I16X8Load8X8U:
    case Opcode::I32X4Load16X4S:
    case Opcode::I32X4Load16X4U:
    case Opcode::I64X2Load32X2S:
    case Opcode::I64X2Load32X2U:
      return Load(loc, value, context);

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
      return Store(loc, value, context);

    case Opcode::MemorySize:
      return MemorySize(context);

    case Opcode::MemoryGrow:
      return MemoryGrow(loc, context);

    case Opcode::I32Const:
      PushType(StackType::I32, context);
      return true;

    case Opcode::I64Const:
      PushType(StackType::I64, context);
      return true;

    case Opcode::F32Const:
      PushType(StackType::F32, context);
      return true;

    case Opcode::F64Const:
      PushType(StackType::F64, context);
      return true;

    case Opcode::I32Eqz:
    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
    case Opcode::I32Extend8S:
    case Opcode::I32Extend16S:
      params = span_i32, results = span_i32;
      break;

    case Opcode::I64Eqz:
    case Opcode::I32WrapI64:
      params = span_i64, results = span_i32;
      break;

    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
    case Opcode::I64Extend8S:
    case Opcode::I64Extend16S:
    case Opcode::I64Extend32S:
      params = span_i64, results = span_i64;
      break;

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
      params = span_i32_i32, results = span_i32;
      break;

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
      params = span_i64_i64, results = span_i32;
      break;

    case Opcode::F32Eq:
    case Opcode::F32Ne:
    case Opcode::F32Lt:
    case Opcode::F32Gt:
    case Opcode::F32Le:
    case Opcode::F32Ge:
      params = span_f32_f32, results = span_i32;
      break;

    case Opcode::F64Eq:
    case Opcode::F64Ne:
    case Opcode::F64Lt:
    case Opcode::F64Gt:
    case Opcode::F64Le:
    case Opcode::F64Ge:
      params = span_f64_f64, results = span_i32;
      break;

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
      params = span_i64_i64, results = span_i64;
      break;

    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
      params = span_f32, results = span_f32;
      break;

    case Opcode::F32Add:
    case Opcode::F32Sub:
    case Opcode::F32Mul:
    case Opcode::F32Div:
    case Opcode::F32Min:
    case Opcode::F32Max:
    case Opcode::F32Copysign:
      params = span_f32_f32, results = span_f32;
      break;

    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
      params = span_f64, results = span_f64;
      break;

    case Opcode::F64Add:
    case Opcode::F64Sub:
    case Opcode::F64Mul:
    case Opcode::F64Div:
    case Opcode::F64Min:
    case Opcode::F64Max:
    case Opcode::F64Copysign:
      params = span_f64_f64, results = span_f64;
      break;

    case Opcode::I32TruncF32S:
    case Opcode::I32TruncF32U:
    case Opcode::I32ReinterpretF32:
    case Opcode::I32TruncSatF32S:
    case Opcode::I32TruncSatF32U:
      params = span_f32, results = span_i32;
      break;

    case Opcode::I32TruncF64S:
    case Opcode::I32TruncF64U:
    case Opcode::I32TruncSatF64S:
    case Opcode::I32TruncSatF64U:
      params = span_f64, results = span_i32;
      break;

    case Opcode::I64ExtendI32S:
    case Opcode::I64ExtendI32U:
      params = span_i32, results = span_i64;
      break;

    case Opcode::I64TruncF32S:
    case Opcode::I64TruncF32U:
    case Opcode::I64TruncSatF32S:
    case Opcode::I64TruncSatF32U:
      params = span_f32, results = span_i64;
      break;

    case Opcode::I64TruncF64S:
    case Opcode::I64TruncF64U:
    case Opcode::I64ReinterpretF64:
    case Opcode::I64TruncSatF64S:
    case Opcode::I64TruncSatF64U:
      params = span_f64, results = span_i64;
      break;

    case Opcode::F32ConvertI32S:
    case Opcode::F32ConvertI32U:
    case Opcode::F32ReinterpretI32:
      params = span_i32, results = span_f32;
      break;

    case Opcode::F32ConvertI64S:
    case Opcode::F32ConvertI64U:
      params = span_i64, results = span_f32;
      break;

    case Opcode::F32DemoteF64:
      params = span_f64, results = span_f32;
      break;

    case Opcode::F64ConvertI32S:
    case Opcode::F64ConvertI32U:
      params = span_i32, results = span_f64;
      break;

    case Opcode::F64ConvertI64S:
    case Opcode::F64ConvertI64U:
    case Opcode::F64ReinterpretI64:
      params = span_i64, results = span_f64;
      break;

    case Opcode::F64PromoteF32:
      params = span_f32, results = span_f64;
      break;

    case Opcode::ReturnCall:
      return ReturnCall(loc, value->index_immediate(), context);

    case Opcode::ReturnCallIndirect:
      return ReturnCallIndirect(loc, value->call_indirect_immediate(), context);

    case Opcode::MemoryInit:
      return MemoryInit(loc, value->init_immediate(), context);

    case Opcode::DataDrop:
      return DataDrop(value->index_immediate(), context);

    case Opcode::MemoryCopy:
      return MemoryCopy(loc, value->copy_immediate(), context);

    case Opcode::MemoryFill:
      return MemoryFill(loc, context);

    case Opcode::TableInit:
      return TableInit(loc, value->init_immediate(), context);

    case Opcode::ElemDrop:
      return ElemDrop(value->index_immediate(), context);

    case Opcode::TableCopy:
      return TableCopy(loc, value->copy_immediate(), context);

    case Opcode::TableGrow:
      return TableGrow(loc, value->index_immediate(), context);

    case Opcode::TableSize:
      return TableSize(value->index_immediate(), context);

    case Opcode::TableFill:
      return TableFill(loc, value->index_immediate(), context);

    case Opcode::V128Const:
      PushType(StackType::V128, context);
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
    case Opcode::F32X4ConvertI32X4S:
    case Opcode::F32X4ConvertI32X4U:
    case Opcode::I16X8WidenLowI8X16S:
    case Opcode::I16X8WidenHighI8X16S:
    case Opcode::I16X8WidenLowI8X16U:
    case Opcode::I16X8WidenHighI8X16U:
    case Opcode::I32X4WidenLowI16X8S:
    case Opcode::I32X4WidenHighI16X8S:
    case Opcode::I32X4WidenLowI16X8U:
    case Opcode::I32X4WidenHighI16X8U:
    case Opcode::I8X16Abs:
    case Opcode::I16X8Abs:
    case Opcode::I32X4Abs:
      params = span_v128, results = span_v128;
      break;

    case Opcode::V128BitSelect:
      params = span_v128_v128_v128, results = span_v128;
      break;

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
    case Opcode::I8X16MinS:
    case Opcode::I8X16MinU:
    case Opcode::I8X16MaxS:
    case Opcode::I8X16MaxU:
    case Opcode::I16X8Add:
    case Opcode::I16X8AddSaturateS:
    case Opcode::I16X8AddSaturateU:
    case Opcode::I16X8Sub:
    case Opcode::I16X8SubSaturateS:
    case Opcode::I16X8SubSaturateU:
    case Opcode::I16X8Mul:
    case Opcode::I16X8MinS:
    case Opcode::I16X8MinU:
    case Opcode::I16X8MaxS:
    case Opcode::I16X8MaxU:
    case Opcode::I32X4Add:
    case Opcode::I32X4Sub:
    case Opcode::I32X4Mul:
    case Opcode::I32X4MinS:
    case Opcode::I32X4MinU:
    case Opcode::I32X4MaxS:
    case Opcode::I32X4MaxU:
    case Opcode::I64X2Add:
    case Opcode::I64X2Sub:
    case Opcode::I64X2Mul:
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
    case Opcode::V8X16Shuffle:
    case Opcode::V8X16Swizzle:
    case Opcode::I8X16NarrowI16X8S:
    case Opcode::I8X16NarrowI16X8U:
    case Opcode::I16X8NarrowI32X4S:
    case Opcode::I16X8NarrowI32X4U:
    case Opcode::V128Andnot:
    case Opcode::I8X16AvgrU:
    case Opcode::I16X8AvgrU:
      params = span_v128_v128, results = span_v128;
      break;

    case Opcode::I8X16Splat:
    case Opcode::I16X8Splat:
    case Opcode::I32X4Splat:
      params = span_i32, results = span_v128;
      break;

    case Opcode::I64X2Splat:
      params = span_i64, results = span_v128;
      break;

    case Opcode::F32X4Splat:
      params = span_f32, results = span_v128;
      break;

    case Opcode::F64X2Splat:
      params = span_f64, results = span_v128;
      break;

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
      params = span_v128, results = span_i32;
      break;

    case Opcode::I64X2ExtractLane:
      params = span_v128, results = span_i64;
      break;

    case Opcode::F32X4ExtractLane:
      params = span_v128, results = span_f32;
      break;

    case Opcode::F64X2ExtractLane:
      params = span_v128, results = span_f64;
      break;

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
      params = span_v128_i32, results = span_v128;
      break;

    case Opcode::I64X2ReplaceLane:
      params = span_v128_i64, results = span_v128;
      break;

    case Opcode::F32X4ReplaceLane:
      params = span_v128_f32, results = span_v128;
      break;

    case Opcode::F64X2ReplaceLane:
      params = span_v128_f64, results = span_v128;
      break;

    case Opcode::MemoryAtomicNotify:
      return MemoryAtomicNotify(loc, value, context);

    case Opcode::MemoryAtomicWait32:
    case Opcode::MemoryAtomicWait64:
      return MemoryAtomicWait(loc, value, context);

    case Opcode::I32AtomicLoad:
    case Opcode::I64AtomicLoad:
    case Opcode::I32AtomicLoad8U:
    case Opcode::I32AtomicLoad16U:
    case Opcode::I64AtomicLoad8U:
    case Opcode::I64AtomicLoad16U:
    case Opcode::I64AtomicLoad32U:
      return AtomicLoad(loc, value, context);

    case Opcode::I32AtomicStore:
    case Opcode::I64AtomicStore:
    case Opcode::I32AtomicStore8:
    case Opcode::I32AtomicStore16:
    case Opcode::I64AtomicStore8:
    case Opcode::I64AtomicStore16:
    case Opcode::I64AtomicStore32:
      return AtomicStore(loc, value, context);

    case Opcode::I32AtomicRmwAdd:
    case Opcode::I32AtomicRmw8AddU:
    case Opcode::I32AtomicRmw16AddU:
    case Opcode::I32AtomicRmwSub:
    case Opcode::I32AtomicRmw8SubU:
    case Opcode::I32AtomicRmw16SubU:
    case Opcode::I32AtomicRmwAnd:
    case Opcode::I32AtomicRmw8AndU:
    case Opcode::I32AtomicRmw16AndU:
    case Opcode::I32AtomicRmwOr:
    case Opcode::I32AtomicRmw8OrU:
    case Opcode::I32AtomicRmw16OrU:
    case Opcode::I32AtomicRmwXor:
    case Opcode::I32AtomicRmw8XorU:
    case Opcode::I32AtomicRmw16XorU:
    case Opcode::I32AtomicRmwXchg:
    case Opcode::I32AtomicRmw8XchgU:
    case Opcode::I32AtomicRmw16XchgU:
    case Opcode::I64AtomicRmwAdd:
    case Opcode::I64AtomicRmw8AddU:
    case Opcode::I64AtomicRmw16AddU:
    case Opcode::I64AtomicRmw32AddU:
    case Opcode::I64AtomicRmwSub:
    case Opcode::I64AtomicRmw8SubU:
    case Opcode::I64AtomicRmw16SubU:
    case Opcode::I64AtomicRmw32SubU:
    case Opcode::I64AtomicRmwAnd:
    case Opcode::I64AtomicRmw8AndU:
    case Opcode::I64AtomicRmw16AndU:
    case Opcode::I64AtomicRmw32AndU:
    case Opcode::I64AtomicRmwOr:
    case Opcode::I64AtomicRmw8OrU:
    case Opcode::I64AtomicRmw16OrU:
    case Opcode::I64AtomicRmw32OrU:
    case Opcode::I64AtomicRmwXor:
    case Opcode::I64AtomicRmw8XorU:
    case Opcode::I64AtomicRmw16XorU:
    case Opcode::I64AtomicRmw32XorU:
    case Opcode::I64AtomicRmwXchg:
    case Opcode::I64AtomicRmw8XchgU:
    case Opcode::I64AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw32XchgU:
    case Opcode::I32AtomicRmwCmpxchg:
    case Opcode::I64AtomicRmwCmpxchg:
    case Opcode::I32AtomicRmw8CmpxchgU:
    case Opcode::I32AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw8CmpxchgU:
    case Opcode::I64AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw32CmpxchgU:
      return AtomicRmw(loc, value, context);

    default:
      context.errors->OnError(loc,
                              format("Unimplemented instruction {}", value));
      return false;
  }

  return PopAndPushTypes(loc, params, results, context);
}

}  // namespace valid
}  // namespace wasp
