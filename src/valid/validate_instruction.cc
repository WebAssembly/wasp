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

#include <cassert>
#include <limits>

#include "wasp/base/concat.h"
#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/errors_nop.h"
#include "wasp/base/features.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/formatters.h"
#include "wasp/valid/match.h"
#include "wasp/valid/validate.h"

namespace wasp::valid {

namespace {

using namespace ::wasp::binary;

#define STACK_TYPE_SPANS(V)                                            \
  V(i32, StackType::I32())                                             \
  V(i64, StackType::I64())                                             \
  V(f32, StackType::F32())                                             \
  V(f64, StackType::F64())                                             \
  V(v128, StackType::V128())                                           \
  V(exnref, StackType::Exnref())                                       \
  V(i31ref, StackType::I31ref())                                       \
  V(i32_i32, StackType::I32(), StackType::I32())                       \
  V(i32_i64, StackType::I32(), StackType::I64())                       \
  V(i32_f32, StackType::I32(), StackType::F32())                       \
  V(i32_f64, StackType::I32(), StackType::F64())                       \
  V(i32_v128, StackType::I32(), StackType::V128())                     \
  V(i64_i64, StackType::I64(), StackType::I64())                       \
  V(f32_f32, StackType::F32(), StackType::F32())                       \
  V(f64_f64, StackType::F64(), StackType::F64())                       \
  V(v128_i32, StackType::V128(), StackType::I32())                     \
  V(v128_i64, StackType::V128(), StackType::I64())                     \
  V(v128_f32, StackType::V128(), StackType::F32())                     \
  V(v128_f64, StackType::V128(), StackType::F64())                     \
  V(v128_v128, StackType::V128(), StackType::V128())                   \
  V(eqref_eqref, StackType::Eqref(), StackType::Eqref())               \
  V(i32_i32_i32, StackType::I32(), StackType::I32(), StackType::I32()) \
  V(i32_i32_i64, StackType::I32(), StackType::I32(), StackType::I64()) \
  V(i32_i64_i64, StackType::I32(), StackType::I64(), StackType::I64()) \
  V(v128_v128_v128, StackType::V128(), StackType::V128(), StackType::V128())

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

optional<FunctionType> GetFunctionType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.types.size(), "type index")) {
    return nullopt;
  }
  if (!context.types[index].is_function_type()) {
    context.errors->OnError(index.loc(), "Expected a function type");
    return nullopt;
  }
  return context.types[index].function_type();
}

optional<StructType> GetStructType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.types.size(), "type index")) {
    return nullopt;
  }
  if (!context.types[index].is_struct_type()) {
    context.errors->OnError(index.loc(), "Expected a struct type");
    return nullopt;
  }
  return context.types[index].struct_type();
}

optional<ArrayType> GetArrayType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.types.size(), "type index")) {
    return nullopt;
  }
  if (!context.types[index].is_array_type()) {
    context.errors->OnError(index.loc(), "Expected an array type");
    return nullopt;
  }
  return context.types[index].array_type();
}

optional<FieldType> GetStructFieldType(Context& context,
                                       const StructType& struct_type,
                                       At<Index> index) {
  if (!ValidateIndex(context, index, struct_type.fields.size(), "field index")) {
    return nullopt;
  }
  return struct_type.fields[index];
}

optional<ValueType> GetFieldValueType(Context& context,
                                      Location loc,
                                      const FieldType& field_type) {
  if (!field_type.type->is_value_type()) {
    context.errors->OnError(loc, "Expected a non-packed field type");
    return nullopt;
  }
  return field_type.type->value_type();
}

optional<PackedType> GetFieldPackedType(Context& context,
                                        Location loc,
                                        const FieldType& field_type) {
  if (!field_type.type->is_packed_type()) {
    context.errors->OnError(loc, "Expected a packed field type");
    return nullopt;
  }
  return field_type.type->packed_type();
}

optional<ValueType> GetStructFieldValueType(Context& context,
                                            Location loc,
                                            const StructType& struct_type,
                                            At<Index> index) {
  auto field_type = GetStructFieldType(context, struct_type, index);
  if (!field_type) {
    return nullopt;
  }
  return GetFieldValueType(context, loc, *field_type);
}

optional<PackedType> GetStructFieldPackedType(Context& context,
                                              Location loc,
                                              const StructType& struct_type,
                                              At<Index> index) {
  auto field_type = GetStructFieldType(context, struct_type, index);
  if (!field_type) {
    return nullopt;
  }
  return GetFieldPackedType(context, loc, *field_type);
}

optional<FunctionType> GetBlockTypeSignature(Context& context,
                                             BlockType block_type) {
  if (block_type.is_void()) {
    return FunctionType{};
  } else if (block_type.is_value_type()) {
    const auto& value_type = block_type.value_type();
    if (!Validate(context, value_type)) {
      return nullopt;
    }
    return FunctionType{{}, {value_type}};
  } else {
    assert(block_type.is_index());
    return GetFunctionType(context, block_type.index());
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

optional<Function> GetFunction(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.functions.size(),
                     "function index")) {
    return nullopt;
  }
  return context.functions[index];
}

optional<TableType> GetTableType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.tables.size(), "table index")) {
    return nullopt;
  }
  return context.tables[index];
}

optional<MemoryType> GetMemoryType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.memories.size(), "memory index")) {
    return nullopt;
  }
  return context.memories[index];
}

optional<GlobalType> GetGlobalType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.globals.size(), "global index")) {
    return nullopt;
  }
  return context.globals[index];
}

optional<EventType> GetEventType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.events.size(), "event index")) {
    return nullopt;
  }
  return context.events[index];
}

optional<ReferenceType> GetElementSegmentType(Context& context,
                                              At<Index> index) {
  if (!ValidateIndex(context, index, context.element_segments.size(),
                     "element segment index")) {
    return nullopt;
  }
  return context.element_segments[index];
}

optional<StackType> GetLocalType(Context& context, At<Index> index) {
  if (!ValidateIndex(context, index, context.locals.GetCount(),
                     "local index")) {
    return nullopt;
  }
  auto local_type = context.locals.GetType(index);
  if (!local_type) {
    return nullopt;
  }
  return StackType(*local_type);
}

bool CheckDataSegment(Context& context, At<Index> index) {
  return ValidateIndex(context, index, context.declared_data_count.value_or(0),
                       "data segment index");
}

StackType MaybeDefault(optional<StackType> value) {
  return value.value_or(StackType::I32());
}

Function MaybeDefault(optional<Function> value) {
  return value.value_or(Function{0});
}

FunctionType MaybeDefault(optional<FunctionType> value) {
  return value.value_or(FunctionType{});
}

TableType MaybeDefault(optional<TableType> value) {
  return value.value_or(
      TableType{Limits{0}, ReferenceType::Funcref_NoLocation()});
}

GlobalType MaybeDefault(optional<GlobalType> value) {
  return value.value_or(
      GlobalType{ValueType::I32_NoLocation(), Mutability::Const});
}

EventType MaybeDefault(optional<EventType> value) {
  return value.value_or(EventType{EventAttribute::Exception, 0});
}

ReferenceType MaybeDefault(optional<ReferenceType> value) {
  return value.value_or(ReferenceType::Externref_NoLocation());
}

Label MaybeDefault(const Label* value) {
  return value ? *value : Label{LabelType::Block, {}, {}, 0};
}

optional<StackType> PeekType(Context& context, Location loc) {
  auto type_stack = GetTypeStack(context);
  if (type_stack.empty()) {
    if (!TopLabel(context).unreachable) {
      context.errors->OnError(loc, "Expected stack to have 1 value, got 0");
      return nullopt;
    }
    return StackType{Any{}};
  }
  return type_stack[type_stack.size() - 1];
}

void PushType(Context& context, StackType stack_type) {
  context.type_stack.push_back(stack_type);
}

void PushTypes(Context& context, StackTypeSpan stack_types) {
  context.type_stack.insert(context.type_stack.end(), stack_types.begin(),
                            stack_types.end());
}

void RemovePrefixIfGreater(StackTypeSpan* lhs, StackTypeSpan rhs) {
  if (lhs->size() > rhs.size()) {
    remove_prefix(lhs, lhs->size() - rhs.size());
  }
}

bool CheckTypes(Context& context, Location loc, StackTypeSpan expected) {
  StackTypeSpan full_expected = expected;
  const auto& top_label = TopLabel(context);
  auto type_stack = GetTypeStack(context);
  RemovePrefixIfGreater(&type_stack, expected);
  if (top_label.unreachable) {
    RemovePrefixIfGreater(&expected, type_stack);
  }

  if (!IsMatch(context, expected, type_stack)) {
    // TODO proper formatting of type stack
    context.errors->OnError(
        loc, concat("Expected stack to contain ", full_expected, ", got ",
                    top_label.unreachable ? "..." : "", type_stack));
    return false;
  }
  return true;
}

Label* GetFunctionLabel(Context&);

bool CheckResultTypes(Context& context,
                      Location loc,
                      const FunctionType& function_type) {
  auto* label = GetFunctionLabel(context);
  assert(label != nullptr);
  auto caller = ToStackTypeList(function_type.result_types);
  auto callee = label->br_types();

  if (!IsMatch(context, callee, caller)) {
    context.errors->OnError(
        loc, concat("Callee's result types ", callee,
                    " must equal caller's result types ", caller));
    return false;
  }
  return true;
}

void ResetTypeStackToLimit(Context& context) {
  context.type_stack.resize(TopLabel(context).type_stack_limit);
}

bool DropTypes(Context& context,
               Location loc,
               size_t count,
               bool print_errors = true) {
  const auto& top_label = TopLabel(context);
  auto type_stack_size = context.type_stack.size() - top_label.type_stack_limit;
  if (count > type_stack_size) {
    if (print_errors) {
      context.errors->OnError(
          loc, concat("Expected stack to contain ", count, " value",
                      count == 1 ? "" : "s", ", got ", type_stack_size));
    }
    ResetTypeStackToLimit(context);
    return top_label.unreachable;
  }
  context.type_stack.resize(context.type_stack.size() - count);
  return true;
}

bool PopTypes(Context& context, Location loc, StackTypeSpan expected) {
  bool valid = CheckTypes(context, loc, expected);
  valid &= DropTypes(context, loc, expected.size(), false);
  return valid;
}

bool PopType(Context& context, Location loc, StackType type) {
  return PopTypes(context, loc, StackTypeSpan(&type, 1));
}

optional<StackType> PopReferenceType(Context& context, Location loc) {
  auto type = PeekType(context, loc);
  if (type) {
    if (!IsReferenceTypeOrAny(*type)) {
      context.errors->OnError(
          loc, concat("Expected reference type, got ", GetTypeStack(context)));
      return nullopt;
    }
    DropTypes(context, loc, 1, false);
  }
  return type;
}

auto PopRtt(Context& context, Location loc)
    -> std::pair<optional<StackType>, optional<Rtt>> {
  auto type = PeekType(context, loc);
  if (type) {
    if (!IsRttOrAny(*type)) {
      context.errors->OnError(
          loc, concat("Expected rtt type, got ", GetTypeStack(context)));
      return {nullopt, nullopt};
    }
    DropTypes(context, loc, 1, false);
    if (type->is_value_type()) {
      return {type, type->value_type().rtt()};
    }
  }
  return {type, nullopt};
}

auto PopTypedReference(Context& context, Location loc)
    -> std::pair<optional<StackType>, optional<Index>> {
  auto type = PopReferenceType(context, loc);
  if (!type) {
    return {nullopt, nullopt};
  }

  if (type->is_any()) {
    return {type, nullopt};
  }

  assert(type->is_value_type() && type->value_type().is_reference_type());

  ReferenceType ref_type = Canonicalize(type->value_type().reference_type());
  assert(ref_type.is_ref());

  if (!ref_type.ref()->heap_type->is_index()) {
    context.errors->OnError(loc,
                            concat("Expected typed function reference, got ",
                                   GetTypeStack(context)));
    return {nullopt, nullopt};
  }

  return {ToStackType(ref_type), ref_type.ref()->heap_type->index()};
}

auto PopFunctionReference(Context& context, Location loc)
    -> std::pair<optional<StackType>, std::optional<FunctionType>> {
  auto [stack_type, index] = PopTypedReference(context, loc);
  if (stack_type && !stack_type->is_any() && index) {
    return {stack_type, GetFunctionType(context, *index)};
  } else {
    return {stack_type, nullopt};
  }
}

auto PopStructReference(Context& context,
                        Location loc,
                        const At<Index>& expected)
    -> std::pair<optional<StackType>, std::optional<StructType>> {
  auto [stack_type, index] = PopTypedReference(context, loc);
  if (stack_type) {
    if (index && !IsMatch(context, HeapType{expected}, HeapType{*index})) {
      // The index deson't match. Print an error, but assume that it worked to
      // prevent knock-on errors.
      context.errors->OnError(loc, concat("Expected struct type ", expected,
                                          " but got type ", *index));
    }
    return {stack_type, GetStructType(context, expected)};
  } else {
    return {stack_type, nullopt};
  }
}

auto PopArrayReference(Context& context,
                       Location loc,
                       const At<Index>& expected)
    -> std::pair<optional<StackType>, std::optional<ArrayType>> {
  auto [stack_type, index] = PopTypedReference(context, loc);
  if (stack_type) {
    if (index && !IsMatch(context, HeapType{expected}, HeapType{*index})) {
      // The index deson't match. Print an error, but assume that it worked to
      // prevent knock-on errors.
      context.errors->OnError(loc, concat("Expected array type ", expected,
                                          " but got type ", *index));
    }
    return {stack_type, GetArrayType(context, expected)};
  } else {
    return {stack_type, nullopt};
  }
}

bool PopAndPushTypes(Context& context,
                     Location loc,
                     StackTypeSpan param_types,
                     StackTypeSpan result_types) {
  bool valid = PopTypes(context, loc, param_types);
  PushTypes(context, result_types);
  return valid;
}

bool PopAndPushTypes(Context& context,
                     Location loc,
                     const FunctionType& function_type) {
  return PopAndPushTypes(context, loc,
                         ToStackTypeList(function_type.param_types),
                         ToStackTypeList(function_type.result_types));
}

void SetUnreachable(Context& context) {
  auto& top_label = TopLabel(context);
  top_label.unreachable = true;
  ResetTypeStackToLimit(context);
}

Label* GetLabel(Context& context, At<Index> depth) {
  if (depth >= context.label_stack.size()) {
    context.errors->OnError(
        depth.loc(), concat("Invalid label ", depth, ", must be less than ",
                            context.label_stack.size()));
    return nullptr;
  }
  return &context.label_stack[context.label_stack.size() - depth - 1];
}

Label* GetFunctionLabel(Context& context) {
  return GetLabel(context, context.label_stack.size() - 1);
}

bool PushLabel(Context& context,
               Location loc,
               LabelType label_type,
               const FunctionType& type) {
  auto stack_param_types = ToStackTypeList(type.param_types);
  auto stack_result_types = ToStackTypeList(type.result_types);
  bool valid = PopTypes(context, loc, stack_param_types);
  context.label_stack.emplace_back(label_type, stack_param_types,
                                   stack_result_types,
                                   context.type_stack.size());
  PushTypes(context, stack_param_types);
  return valid;
}

bool PushLabel(Context& context,
               Location loc,
               LabelType label_type,
               BlockType block_type) {
  auto sig = GetBlockTypeSignature(context, block_type);
  if (!sig) {
    return false;
  }
  return PushLabel(context, loc, label_type, *sig);
}

bool CheckTypeStackEmpty(Context& context, Location loc) {
  const auto& top_label = TopLabel(context);
  if (context.type_stack.size() != top_label.type_stack_limit) {
    context.errors->OnError(
        loc, concat("Expected empty stack, got ", GetTypeStack(context)));
    return false;
  }
  return true;
}

bool Catch(Context& context, Location loc) {
  auto& top_label = TopLabel(context);
  if (top_label.label_type != LabelType::Try) {
    context.errors->OnError(loc, "Got catch instruction without try");
    return false;
  }
  bool valid = PopTypes(context, loc, top_label.result_types);
  valid &= CheckTypeStackEmpty(context, loc);
  ResetTypeStackToLimit(context);
  PushTypes(context, span_exnref);
  top_label.label_type = LabelType::Catch;
  top_label.unreachable = false;
  return valid;
}

bool Else(Context& context, Location loc) {
  auto& top_label = TopLabel(context);
  if (top_label.label_type != LabelType::If) {
    context.errors->OnError(loc, "Got else instruction without if");
    return false;
  }
  bool valid = PopTypes(context, loc, top_label.result_types);
  valid &= CheckTypeStackEmpty(context, loc);
  ResetTypeStackToLimit(context);
  PushTypes(context, top_label.param_types);
  top_label.label_type = LabelType::Else;
  top_label.unreachable = false;
  return valid;
}

bool End(Context& context, Location loc) {
  auto& top_label = TopLabel(context);
  bool valid = true;
  if (top_label.label_type == LabelType::If) {
    valid &= Else(context, loc);
  } else if (top_label.label_type == LabelType::Let) {
    context.locals.Pop();
  }
  valid &= PopTypes(context, loc, top_label.result_types);
  valid &= CheckTypeStackEmpty(context, loc);
  ResetTypeStackToLimit(context);
  PushTypes(context, top_label.result_types);
  context.label_stack.pop_back();
  return valid;
}

bool Br(Context& context, Location loc, At<Index> depth) {
  const auto* label = GetLabel(context, depth);
  bool valid = PopTypes(context, loc, MaybeDefault(label).br_types());
  SetUnreachable(context);
  return AllTrue(label, valid);
}

bool BrIf(Context& context, Location loc, At<Index> depth) {
  bool valid = PopType(context, loc, StackType::I32());
  const auto* label = GetLabel(context, depth);
  auto label_ = MaybeDefault(label);
  return AllTrue(
      valid, label,
      PopAndPushTypes(context, loc, label_.br_types(), label_.br_types()));
}

bool BrTable(Context& context,
             Location loc,
             const At<BrTableImmediate>& immediate) {
  bool valid = PopType(context, loc, StackType::I32());
  const auto* default_label = GetLabel(context, immediate->default_target);
  if (!default_label) {
    return false;
  }

  StackTypeSpan br_types = default_label->br_types();
  valid &= CheckTypes(context, immediate->default_target.loc(), br_types);

  for (auto target : immediate->targets) {
    const auto* label = GetLabel(context, target);
    if (label) {
      if (context.features.function_references_enabled()) {
        if (br_types.size() != label->br_types().size()) {
          context.errors->OnError(
              target.loc(),
              concat("br_table labels must have the same arity; expected ",
                     br_types.size(), ", got ", label->br_types().size()));
          valid = false;
        }
        valid &= CheckTypes(context, target.loc(), label->br_types());
      } else {
        if (br_types != label->br_types()) {
          context.errors->OnError(
              target.loc(),
              concat("br_table labels must have the same signature; expected ",
                     br_types, ", got ", label->br_types()));
          valid = false;
        }
      }
    } else {
      valid = false;
    }
  }
  SetUnreachable(context);
  return valid;
}

bool Call(Context& context, Location loc, At<Index> function_index) {
  auto function = GetFunction(context, function_index);
  auto function_type =
      GetFunctionType(context, MaybeDefault(function).type_index);
  return AllTrue(function, function_type,
                 PopAndPushTypes(context, loc, MaybeDefault(function_type)));
}

bool CallIndirect(Context& context,
                  Location loc,
                  const At<CallIndirectImmediate>& immediate) {
  auto table_type = GetTableType(context, immediate->table_index);
  auto function_type = GetFunctionType(context, immediate->index);
  bool valid = PopType(context, loc, StackType::I32());
  return AllTrue(table_type, function_type, valid,
                 PopAndPushTypes(context, loc, MaybeDefault(function_type)));
}

bool Select(Context& context, Location loc) {
  bool valid = PopType(context, loc, StackType::I32());
  auto type = MaybeDefault(PeekType(context, loc));
  if (!((type.is_value_type() && type.value_type().is_numeric_type()) ||
        type.is_any())) {
    context.errors->OnError(
        loc, concat("select instruction without expected type can only be used "
                    "with i32, i64, f32, f64; got ",
                    type));
    return false;
  }
  const StackType pop_types[] = {type, type};
  const StackType push_type[] = {type};
  return AllTrue(valid, PopAndPushTypes(context, loc, pop_types, push_type));
}

bool SelectT(Context& context,
             Location loc,
             const At<ValueTypeList>& value_types) {
  bool valid = PopType(context, loc, StackType::I32());
  if (value_types->size() != 1) {
    context.errors->OnError(
        value_types.loc(),
        concat("select instruction must have types immediate with size 1, got ",
               value_types->size()));
    return false;
  }
  valid &= Validate(context, value_types);
  StackTypeList stack_types = ToStackTypeList(value_types);
  StackType type = stack_types[0];
  const StackType pop_types[] = {type, type};
  const StackType push_type[] = {type};
  return AllTrue(valid, PopAndPushTypes(context, loc, pop_types, push_type));
}

bool LocalGet(Context& context, At<Index> index) {
  auto local_type = GetLocalType(context, index);
  PushType(context, MaybeDefault(local_type));
  return AllTrue(local_type);
}

bool LocalSet(Context& context, Location loc, At<Index> index) {
  auto local_type = GetLocalType(context, index);
  return AllTrue(local_type, PopType(context, loc, MaybeDefault(local_type)));
}

bool LocalTee(Context& context, Location loc, At<Index> index) {
  auto local_type = GetLocalType(context, index);
  const StackType type[] = {MaybeDefault(local_type)};
  return AllTrue(local_type, PopAndPushTypes(context, loc, type, type));
}

bool GlobalGet(Context& context, At<Index> index) {
  auto global_type = GetGlobalType(context, index);
  PushType(context, StackType(*MaybeDefault(global_type).valtype));
  return AllTrue(global_type);
}

bool GlobalSet(Context& context, Location loc, At<Index> index) {
  auto global_type = GetGlobalType(context, index);
  auto type = MaybeDefault(global_type);
  bool valid = true;
  if (type.mut == Mutability::Const) {
    context.errors->OnError(
        index.loc(),
        concat("global.set is invalid on immutable global ", index));
    valid = false;
  }
  return AllTrue(valid, PopType(context, loc, StackType(*type.valtype)));
}

bool TableGet(Context& context, Location loc, At<Index> index) {
  auto table_type = GetTableType(context, index);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType type[] = {stack_type};
  return AllTrue(table_type, PopAndPushTypes(context, loc, span_i32, type));
}

bool TableSet(Context& context, Location loc, At<Index> index) {
  auto table_type = GetTableType(context, index);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType types[] = {StackType::I32(), stack_type};
  return AllTrue(table_type, PopTypes(context, loc, types));
}

bool RefFunc(Context& context, Location loc, At<Index> index) {
  if (context.declared_functions.find(index) ==
      context.declared_functions.end()) {
    context.errors->OnError(loc,
                            concat("Undeclared function reference ", index));
    return false;
  }
  assert(index < context.functions.size());
  auto& function = context.functions[index];
  PushType(context,
           ToStackType(RefType{HeapType{function.type_index}, Null::No}));
  return true;
}

bool CheckAlignment(Context& context,
                    const At<Instruction>& instruction,
                    u32 max_align) {
  if (instruction->mem_arg_immediate()->align_log2 > max_align) {
    context.errors->OnError(instruction.loc(),
                            concat("Invalid alignment ", instruction));
    return false;
  }
  return true;
}

bool Load(Context& context, Location loc, const At<Instruction>& instruction) {
  auto memory_type = GetMemoryType(context, 0);
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

  bool valid = CheckAlignment(context, instruction, max_align);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(context, loc, span_i32, span));
}

bool Store(Context& context, Location loc, const At<Instruction>& instruction) {
  auto memory_type = GetMemoryType(context, 0);
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

  bool valid = CheckAlignment(context, instruction, max_align);
  return AllTrue(memory_type, valid, PopTypes(context, loc, span));
}

bool MemorySize(Context& context) {
  auto memory_type = GetMemoryType(context, 0);
  PushType(context, StackType::I32());
  return AllTrue(memory_type);
}

bool MemoryGrow(Context& context, Location loc) {
  auto memory_type = GetMemoryType(context, 0);
  return AllTrue(memory_type,
                 PopAndPushTypes(context, loc, span_i32, span_i32));
}

bool MemoryInit(Context& context,
                Location loc,
                const At<InitImmediate>& immediate) {
  auto memory_type = GetMemoryType(context, 0);
  bool valid = CheckDataSegment(context, immediate->segment_index);
  return AllTrue(memory_type, valid, PopTypes(context, loc, span_i32_i32_i32));
}

bool DataDrop(Context& context, At<Index> segment_index) {
  return CheckDataSegment(context, segment_index);
}

bool MemoryCopy(Context& context,
                Location loc,
                const At<CopyImmediate>& immediate) {
  auto memory_type = GetMemoryType(context, 0);
  return AllTrue(memory_type, PopTypes(context, loc, span_i32_i32_i32));
}

bool MemoryFill(Context& context, Location loc) {
  auto memory_type = GetMemoryType(context, 0);
  return AllTrue(memory_type, PopTypes(context, loc, span_i32_i32_i32));
}

bool CheckReferenceType(Context& context,
                        ReferenceType expected,
                        At<ReferenceType> actual) {
  if (!IsMatch(context, ToStackType(expected), ToStackType(actual))) {
    context.errors->OnError(actual.loc(), concat("Expected reference type ",
                                                 expected, ", got ", actual));
    return false;
  }
  return true;
}

bool TableInit(Context& context,
               Location loc,
               const At<InitImmediate>& immediate) {
  auto table_type = GetTableType(context, immediate->dst_index);
  auto elemtype = GetElementSegmentType(context, immediate->segment_index);
  bool valid = CheckReferenceType(context, MaybeDefault(table_type).elemtype,
                                  MaybeDefault(elemtype));
  return AllTrue(table_type, elemtype, valid,
                 PopTypes(context, loc, span_i32_i32_i32));
}

bool ElemDrop(Context& context, At<Index> segment_index) {
  auto elem_type = GetElementSegmentType(context, segment_index);
  return AllTrue(elem_type);
}

bool TableCopy(Context& context,
               Location loc,
               const At<CopyImmediate>& immediate) {
  auto dst_table_type = GetTableType(context, immediate->dst_index);
  auto src_table_type = GetTableType(context, immediate->src_index);
  bool valid =
      CheckReferenceType(context, MaybeDefault(dst_table_type).elemtype,
                         MaybeDefault(src_table_type).elemtype);
  return AllTrue(dst_table_type, src_table_type, valid,
                 PopTypes(context, loc, span_i32_i32_i32));
}

bool TableGrow(Context& context, Location loc, At<Index> index) {
  auto table_type = GetTableType(context, index);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType types[] = {stack_type, StackType::I32()};
  return AllTrue(table_type, PopAndPushTypes(context, loc, types, span_i32));
}

bool TableSize(Context& context, At<Index> index) {
  auto table_type = GetTableType(context, 0);
  PushTypes(context, span_i32);
  return AllTrue(table_type);
}

bool TableFill(Context& context, Location loc, At<Index> index) {
  auto table_type = GetTableType(context, index);
  auto stack_type = ToStackType(MaybeDefault(table_type).elemtype);
  const StackType types[] = {StackType::I32(), stack_type, StackType::I32()};
  return AllTrue(table_type, PopTypes(context, loc, types));
}

bool CheckAtomicAlignment(Context& context,
                          const At<Instruction>& instruction,
                          u32 align) {
  if (instruction->mem_arg_immediate()->align_log2 != align) {
    context.errors->OnError(instruction.loc(),
                            concat("Invalid atomic alignment ", instruction));
    return false;
  }
  return true;
}

bool MemoryAtomicNotify(Context& context,
                        Location loc,
                        const At<Instruction>& instruction) {
  const u32 align = 2;
  auto memory_type = GetMemoryType(context, 0);
  bool valid = CheckAtomicAlignment(context, instruction, align);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(context, loc, span_i32_i32, span_i32));
}

bool MemoryAtomicWait(Context& context,
                      Location loc,
                      const At<Instruction>& instruction) {
  auto memory_type = GetMemoryType(context, 0);
  StackTypeSpan span;
  u32 align;
  switch (instruction->opcode) {
    case Opcode::MemoryAtomicWait32: span = span_i32_i32_i64; align = 2; break;
    case Opcode::MemoryAtomicWait64: span = span_i32_i64_i64; align = 3; break;
    default:
      WASP_UNREACHABLE();
  }

  bool valid = CheckAtomicAlignment(context, instruction, align);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(context, loc, span, span_i32));
}

bool AtomicLoad(Context& context,
                Location loc,
                const At<Instruction>& instruction) {
  auto memory_type = GetMemoryType(context, 0);
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

  bool valid = CheckAtomicAlignment(context, instruction, align);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(context, loc, span_i32, span));
}

bool AtomicStore(Context& context,
                 Location loc,
                 const At<Instruction>& instruction) {
  auto memory_type = GetMemoryType(context, 0);
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

  bool valid = CheckAtomicAlignment(context, instruction, align);
  return AllTrue(memory_type, valid, PopTypes(context, loc, span));
}

bool AtomicRmw(Context& context,
               Location loc,
               const At<Instruction>& instruction) {
  auto memory_type = GetMemoryType(context, 0);
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

  bool valid = CheckAtomicAlignment(context, instruction, align);
  return AllTrue(memory_type, valid,
                 PopAndPushTypes(context, loc, params, results));
}

bool ReturnCall(Context& context, Location loc, At<Index> function_index) {
  auto function = GetFunction(context, function_index);
  auto function_type =
      GetFunctionType(context, MaybeDefault(function).type_index);
  bool valid = CheckResultTypes(context, loc, MaybeDefault(function_type));
  valid &= PopTypes(context,
      loc, ToStackTypeList(MaybeDefault(function_type).param_types));
  SetUnreachable(context);
  return AllTrue(function, function_type, valid);
}

bool ReturnCallIndirect(Context& context,
                        Location loc,
                        const At<CallIndirectImmediate>& immediate) {
  auto table_type = GetTableType(context, 0);
  auto function_type = GetFunctionType(context, immediate->index);
  bool valid = CheckResultTypes(context, loc, MaybeDefault(function_type));
  valid &= PopType(context, loc, StackType::I32());
  valid &= PopTypes(context,
      loc, ToStackTypeList(MaybeDefault(function_type).param_types));
  SetUnreachable(context);
  return AllTrue(table_type, function_type, valid);
}

bool Throw(Context& context, Location loc, At<Index> index) {
  auto event_type = GetEventType(context, index);
  auto function_type =
      GetFunctionType(context, MaybeDefault(event_type).type_index);
  bool valid = PopTypes(context,
      loc, ToStackTypeList(MaybeDefault(function_type).param_types));
  SetUnreachable(context);
  return AllTrue(event_type, function_type, valid);
}

bool Rethrow(Context& context, Location loc) {
  bool valid = PopTypes(context, loc, span_exnref);
  SetUnreachable(context);
  return valid;
}

bool BrOnExn(Context& context,
             Location loc,
             const At<BrOnExnImmediate>& immediate) {
  auto event_type = GetEventType(context, immediate->event_index);
  auto function_type =
      GetFunctionType(context, MaybeDefault(event_type).type_index);
  auto* label = GetLabel(context, immediate->target);
  bool valid =
      IsMatch(context, ToStackTypeList(MaybeDefault(function_type).param_types),
              MaybeDefault(label).br_types());
  valid &= PopAndPushTypes(context, loc, span_exnref, span_exnref);
  return AllTrue(event_type, function_type, label, valid);
}

bool BrOnNull(Context& context, Location loc, const At<Index>& depth) {
  bool valid = true;
  auto type_opt = PopReferenceType(context, loc);
  auto type = MaybeDefault(type_opt);

  const auto* label = GetLabel(context, depth);
  auto label_ = MaybeDefault(label);
  valid &= PopAndPushTypes(context, loc, label_.br_types(), label_.br_types());

  if (IsNullableType(type)) {
    PushType(context, AsNonNullableType(type));
  } else {
    context.errors->OnError(loc, concat(type, " is not a nullable type"));
    valid = false;
  }
  return AllTrue(valid, type_opt, label);
}

bool RefAsNonNull(Context& context, Location loc) {
  bool valid = true;
  auto type_opt = PopReferenceType(context, loc);
  auto type = MaybeDefault(type_opt);

  if (IsNullableType(type)) {
    PushType(context, AsNonNullableType(type));
  } else {
    context.errors->OnError(loc, concat(type, " is not a nullable type"));
    valid = false;
  }
  return AllTrue(valid, type_opt);
}

bool CallRef(Context& context, Location loc) {
  auto [stack_type, function_type] = PopFunctionReference(context, loc);
  if (!stack_type) {
    return false;
  }
  if (stack_type && stack_type->is_any()) {
    return true;
  }

  return AllTrue(function_type,
                 PopAndPushTypes(context, loc, MaybeDefault(function_type)));
}

bool ReturnCallRef(Context& context, Location loc) {
  auto [stack_type, function_type] = PopFunctionReference(context, loc);
  if (!stack_type) {
    return false;
  }
  if (stack_type && stack_type->is_any()) {
    return true;
  }

  bool valid = CheckResultTypes(context, loc, MaybeDefault(function_type));
  valid &= PopTypes(context, loc,
                    ToStackTypeList(MaybeDefault(function_type).param_types));
  SetUnreachable(context);
  return AllTrue(function_type, valid);
}

bool FuncBind(Context& context, Location loc, At<Index> new_type_index) {
  auto [stack_type, old_function_type] = PopFunctionReference(context, loc);
  if (!stack_type) {
    return false;
  }
  if (stack_type && stack_type->is_any()) {
    // The result type is always known, so make sure we push the new function
    // reference even for an unreachable stack.
    PushType(context, ToStackType(RefType{HeapType{new_type_index}, Null::No}));
    return true;
  }

  auto new_function_type = GetFunctionType(context, new_type_index);
  if (!old_function_type || !new_function_type) {
    return false;
  }

  auto& old_params = old_function_type->param_types;
  auto& new_params = new_function_type->param_types;
  auto& old_results = old_function_type->result_types;
  auto& new_results = new_function_type->result_types;

  // If the original function type is   [t0* t1*] -> [t2*],
  // then the new function type must be    [t1'*] -> [t2'*],
  // where t1'* <: t1* and t2* <: t2'*
  //
  // So the new function type must have fewer parameters than the old function
  // type.
  if (old_params.size() < new_params.size()) {
    context.errors->OnError(
        loc, concat("new type ", *new_function_type,
                    " has more params than old type ", *old_function_type));
    return false;
  }

  Index bound_param_count = old_params.size() - new_params.size();
  ValueTypeList bound_params(old_params.begin(),
                             old_params.begin() + bound_param_count);
  ValueTypeList unbound_params(old_params.begin() + bound_param_count,
                               old_params.end());

  bool valid = true;
  if (!IsMatch(context, new_params, unbound_params)) {
    context.errors->OnError(loc, concat("bind params ", new_params,
                                        " does not match ", unbound_params));
    valid = false;
  }

  if (!IsMatch(context, old_results, new_results)) {
    context.errors->OnError(
        loc, concat("results ", old_results, " does not match bind results ",
                    new_results));
    valid = false;
  }

  StackTypeList stack_results{
      ToStackType(RefType{HeapType{new_type_index}, Null::No})};

  return AllTrue(valid,
                 PopAndPushTypes(context, loc, ToStackTypeList(bound_params),
                                 stack_results));
}

bool Let(Context& context, Location loc, const At<LetImmediate>& immediate) {
  bool valid = PopTypes(context, loc, ToStackTypeList(immediate->locals));
  valid &= PushLabel(context, loc, LabelType::Let, immediate->block_type);
  context.locals.Push();
  valid &= Validate(context, immediate->locals, RequireDefaultable::No);
  return valid;
}

bool SimdLane(Context& context,
              Location loc,
              const At<Instruction>& instruction) {
  StackTypeSpan params, results;
  u8 num_lanes;
  switch (instruction->opcode) {
    case Opcode::I8X16ExtractLaneS:
    case Opcode::I8X16ExtractLaneU: num_lanes = 16; goto extract_i32;
    case Opcode::I16X8ExtractLaneS:
    case Opcode::I16X8ExtractLaneU: num_lanes = 8; goto extract_i32;
    case Opcode::I32X4ExtractLane:  num_lanes = 4; goto extract_i32;

    extract_i32:
      params = span_v128, results = span_i32;
      break;

    case Opcode::I64X2ExtractLane:
      params = span_v128, results = span_i64;
      num_lanes = 2;
      break;

    case Opcode::F32X4ExtractLane:
      params = span_v128, results = span_f32;
      num_lanes = 4;
      break;

    case Opcode::F64X2ExtractLane:
      params = span_v128, results = span_f64;
      num_lanes = 2;
      break;

    case Opcode::I8X16ReplaceLane: num_lanes = 16; goto replace_i32;
    case Opcode::I16X8ReplaceLane: num_lanes = 8; goto replace_i32;
    case Opcode::I32X4ReplaceLane: num_lanes = 4; goto replace_i32;

    replace_i32:
      params = span_v128_i32, results = span_v128;
      break;

    case Opcode::I64X2ReplaceLane:
      params = span_v128_i64, results = span_v128;
      num_lanes = 2;
      break;

    case Opcode::F32X4ReplaceLane:
      params = span_v128_f32, results = span_v128;
      num_lanes = 4;
      break;

    case Opcode::F64X2ReplaceLane:
      params = span_v128_f64, results = span_v128;
      num_lanes = 2;
      break;

    default:
      WASP_UNREACHABLE();
  }

  bool valid = true;
  if (instruction->simd_lane_immediate() >= num_lanes) {
    context.errors->OnError(
        instruction.loc(),
        concat("Invalid lane immediate ", instruction->simd_lane_immediate()));
    valid = false;
  }
  return AllTrue(valid, PopAndPushTypes(context, loc, params, results));
}

bool SimdShuffle(Context& context,
                 Location loc,
                 const At<ShuffleImmediate>& immediate) {
  const SimdLaneImmediate max_lane = 32;  // Two i8x16 values.
  bool valid = true;
  for (auto lane : *immediate) {
    if (lane >= max_lane) {
      context.errors->OnError(immediate.loc(),
                              concat("Invalid shuffle immediate ", lane));
      valid = false;
    }
  }

  StackTypeSpan params = span_v128_v128;
  StackTypeSpan results = span_v128;
  return AllTrue(valid, PopAndPushTypes(context, loc, params, results));
}

bool RttCanon(Context& context, Location loc, const At<HeapType>& immediate) {
  u32 depth = immediate->is_heap_kind(HeapKind::Any) ? 0 : 1;
  PushType(context, ToStackType(ValueType{Rtt{depth, immediate}}));
  return true;
}

bool RttSub(Context& context, Location loc, const At<HeapType>& immediate) {
  auto [type_opt, old_rtt] = PopRtt(context, loc);
  if (!type_opt) {
    return false;
  }
  if (type_opt && type_opt->is_any()) {
    return true;
  }
  u32 new_depth = old_rtt->depth + 1;
  if (new_depth == 0) {
    context.errors->OnError(loc, concat("Invalid rtt depth", old_rtt->depth));
    return false;
  }
  Rtt new_rtt{new_depth, immediate};
  if (!IsMatch(context, old_rtt->type, new_rtt.type)) {
    context.errors->OnError(
        loc, concat(new_rtt.type, " is not a subtype of ", old_rtt->type));
    return false;
  }
  PushType(context, ToStackType(ValueType{new_rtt}));
  return true;
}

bool CheckSame(Context& context,
               const At<HeapType>& expected,
               const At<HeapType>& actual) {
  if (!IsSame(context, expected, actual)) {
    context.errors->OnError(actual.loc(),
                            concat(actual, " is not equal to ", expected));
    return false;
  }
  return true;
}

bool CheckSubtype(Context& context,
                  const At<HeapType>& expected,
                  const At<HeapType>& actual) {
  if (!IsMatch(context, expected, actual)) {
    context.errors->OnError(actual.loc(),
                            concat(actual, " is not a subtype of ", expected));
    return false;
  }
  return true;
}

bool RefTest(Context& context,
             Location loc,
             const At<HeapType2Immediate>& immediate) {
  bool valid = CheckSubtype(context, immediate->parent, immediate->child);
  StackTypeList stack_types{StackType{ValueType{ReferenceType{
                                RefType{immediate->parent, Null::Yes}}}},
                            StackType{ValueType{Rtt{0, immediate->child}}}};
  return AllTrue(valid, PopAndPushTypes(context, loc, stack_types, span_i32));
}

bool RefCast(Context& context,
             Location loc,
             const At<HeapType2Immediate>& immediate) {
  bool valid = CheckSubtype(context, immediate->parent, immediate->child);
  StackTypeList params{StackType{ValueType{ReferenceType{
                           RefType{immediate->parent, Null::Yes}}}},
                       StackType{ValueType{Rtt{0, immediate->child}}}};
  StackTypeList results{
      StackType{ValueType{ReferenceType{RefType{immediate->child, Null::No}}}}};
  return AllTrue(valid, PopAndPushTypes(context, loc, params, results));
}

bool BrOnCast(Context& context, Location loc, const At<Index>& immediate) {
  // TODO cleanup
  bool valid = true;
  auto [type_opt, rtt_opt] = PopRtt(context, loc);
  if (!type_opt) {
    return false;
  }
  if (type_opt && type_opt->is_any()) {
    return true;
  }

  assert(rtt_opt);
  StackTypeList sub_type{
      StackType{ValueType{ReferenceType{RefType{rtt_opt->type, Null::Yes}}}}};

  auto* label = GetLabel(context, immediate);
  auto label_types = MaybeDefault(label).br_types();
  if (!IsMatch(context, sub_type, label_types)) {
    context.errors->OnError(
        loc, concat("Label type is ", label_types, ", got ", sub_type));
    valid = false;
  }

  type_opt = PopReferenceType(context, loc);
  if (!type_opt) {
    return false;
  }
  if (type_opt && type_opt->is_any()) {
    return valid;
  }

  auto reference_type = Canonicalize(type_opt->value_type().reference_type());
  valid &=
      CheckSubtype(context, reference_type.ref()->heap_type, rtt_opt->type);
  PushType(context, *type_opt);
  return valid;
}

bool StructNewWithRtt(Context& context,
                      Location loc,
                      const At<Index>& immediate) {
  bool valid = true;
  auto [type_opt, rtt_opt] = PopRtt(context, loc);
  if (!type_opt) {
    return false;
  }
  HeapType heap_type{immediate};
  if (!type_opt->is_any()) {
    valid &= CheckSame(context, rtt_opt->type, heap_type);

    auto struct_type = GetStructType(context, immediate);
    if (struct_type) {
      StackTypeList stack_types;
      for (auto& field : struct_type->fields) {
        stack_types.push_back(ToStackType(field->type));
      }
      valid &= PopTypes(context, loc, stack_types);
    }
  }

  PushType(context,
           StackType{ValueType{ReferenceType{RefType{heap_type, Null::No}}}});
  return valid;
}

bool StructNewDefaultWithRtt(Context& context,
                             Location loc,
                             const At<Index>& immediate) {
  bool valid = true;
  auto [type_opt, rtt_opt] = PopRtt(context, loc);
  if (!type_opt) {
    return false;
  }
  HeapType heap_type{immediate};
  if (!type_opt->is_any()) {
    valid &= CheckSame(context, rtt_opt->type, heap_type);

    auto struct_type = GetStructType(context, immediate);
    if (struct_type) {
      for (auto& field : struct_type->fields) {
        valid &= CheckDefaultable(context, field->type, "field type");
      }
    }
  }

  PushType(context,
           StackType{ValueType{ReferenceType{RefType{heap_type, Null::No}}}});
  return valid;
}

bool StructGet(Context& context,
               Location loc,
               const At<StructFieldImmediate>& immediate) {
  auto [stack_type, struct_type] =
      PopStructReference(context, loc, immediate->struct_);
  if (!struct_type) {
    return false;
  }

  auto value_type =
      GetStructFieldValueType(context, loc, *struct_type, immediate->field);
  if (!value_type) {
    return false;
  }

  PushType(context, StackType{*value_type});
  return true;
}

bool StructGetPacked(Context& context,
                     Location loc,
                     const At<StructFieldImmediate>& immediate) {
  auto [stack_type, struct_type] =
      PopStructReference(context, loc, immediate->struct_);
  if (!struct_type) {
    return false;
  }

  auto packed_type =
      GetStructFieldPackedType(context, loc, *struct_type, immediate->field);
  if (!packed_type) {
    return false;
  }

  PushType(context, StackType::I32());
  return true;
}

bool StructSet(Context& context,
               Location loc,
               const At<StructFieldImmediate>& immediate) {
  auto struct_type = GetStructType(context, immediate->struct_);
  if (!struct_type) {
    return false;
  }

  auto field_type = GetStructFieldType(context, *struct_type, immediate->field);
  if (!field_type) {
    return false;
  }

  bool valid = true;
  if (field_type->mut == Mutability::Const) {
    context.errors->OnError(
        loc, concat("Cannot set immutable field ", immediate->field));
    valid = false;
  }

  StackTypeList stack_types{StackType{ValueType{ReferenceType{RefType{
                                HeapType{immediate->struct_}, Null::Yes}}}},
                            ToStackType(field_type->type)};
  return AllTrue(valid, PopTypes(context, loc, stack_types));
}

bool ArrayNewWithRtt(Context& context,
                     Location loc,
                     const At<Index>& immediate) {
  bool valid = true;
  auto [type_opt, rtt_opt] = PopRtt(context, loc);
  if (!type_opt) {
    return false;
  }
  HeapType heap_type{immediate};
  if (!type_opt->is_any()) {
    valid &= CheckSame(context, rtt_opt->type, heap_type);

    auto array_type = GetArrayType(context, immediate);
    if (array_type) {
      StackTypeList stack_types{ToStackType(array_type->field->type),
                                StackType::I32()};
      valid &= PopTypes(context, loc, stack_types);
    }
  }

  PushType(context,
           StackType{ValueType{ReferenceType{RefType{heap_type, Null::No}}}});
  return valid;
}

bool ArrayNewDefaultWithRtt(Context& context,
                            Location loc,
                            const At<Index>& immediate) {
  bool valid = true;
  auto [type_opt, rtt_opt] = PopRtt(context, loc);
  if (!type_opt) {
    return false;
  }
  HeapType heap_type{immediate};
  if (!type_opt->is_any()) {
    valid &= CheckSame(context, rtt_opt->type, heap_type);

    auto array_type = GetArrayType(context, immediate);
    if (array_type) {
      valid &= CheckDefaultable(context, array_type->field->type, "field type");
      valid &= PopType(context, loc, StackType::I32());
    }
  }

  PushType(context,
           StackType{ValueType{ReferenceType{RefType{heap_type, Null::No}}}});
  return valid;
}

bool ArrayGet(Context& context, Location loc, const At<Index>& immediate) {
  auto [stack_type, array_type] = PopArrayReference(context, loc, immediate);
  if (!array_type) {
    return false;
  }

  auto value_type = GetFieldValueType(context, loc, array_type->field);
  if (!value_type) {
    return false;
  }

  PushType(context, StackType{*value_type});
  return true;
}

bool ArrayGetPacked(Context& context,
                    Location loc,
                    const At<Index>& immediate) {
  auto [stack_type, array_type] = PopArrayReference(context, loc, immediate);
  if (!array_type) {
    return false;
  }

  auto packed_type = GetFieldPackedType(context, loc, array_type->field);
  if (!packed_type) {
    return false;
  }

  PushType(context, StackType::I32());
  return true;
}

bool ArraySet(Context& context, Location loc, const At<Index>& immediate) {
  auto array_type = GetArrayType(context, immediate);
  if (!array_type) {
    return false;
  }

  bool valid = true;
  if (array_type->field->mut == Mutability::Const) {
    context.errors->OnError(
        loc, concat("Cannot set immutable field ", array_type->field->mut));
    valid = false;
  }

  StackTypeList stack_types{StackType{ValueType{ReferenceType{
                                RefType{HeapType{immediate}, Null::Yes}}}},
                            ToStackType(array_type->field->type)};
  return AllTrue(valid, PopTypes(context, loc, stack_types));
}

bool ArrayLen(Context& context, Location loc, const At<Index>& immediate) {
  auto array_type = GetArrayType(context, immediate);
  if (!array_type) {
    return false;
  }

  StackTypeList params{StackType{ValueType{ReferenceType{
                           RefType{HeapType{immediate}, Null::Yes}}}}};
  return PopAndPushTypes(context, loc, params, span_i32);
}

}  // namespace

bool Validate(Context& context,
              const At<Locals>& value,
              RequireDefaultable require_defaultable) {
  ErrorsContextGuard guard{*context.errors, value.loc(), "locals"};
  bool valid = true;
  if (require_defaultable == RequireDefaultable::Yes) {
    valid &= CheckDefaultable(context, value->type, "local type");
  }
  valid &= Validate(context, value->type);

  if (!context.locals.Append(value->count, value->type)) {
    const Index max = std::numeric_limits<Index>::max();
    context.errors->OnError(
        value.loc(),
        concat("Too many locals; max is ", max, ", got ",
               static_cast<u64>(context.locals.GetCount()) + value->count));
    valid = false;
  }
  return valid;
}

bool Validate(Context& context,
              const At<LocalsList>& value,
              RequireDefaultable require_defaultable) {
  bool valid = true;
  for (auto&& locals : *value) {
    valid &= Validate(context, locals, require_defaultable);
  }
  return valid;
}

bool Validate(Context& context, const At<Instruction>& value) {
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
      return PushLabel(context, loc, LabelType::Block,
                       value->block_type_immediate());

    case Opcode::Loop:
      return PushLabel(context, loc, LabelType::Loop,
                       value->block_type_immediate());

    case Opcode::If: {
      bool valid = PopType(context, loc, StackType::I32());
      valid &=
          PushLabel(context, loc, LabelType::If, value->block_type_immediate());
      return valid;
    }

    case Opcode::Else:
      return Else(context, loc);

    case Opcode::End:
      return End(context, loc);

    case Opcode::Try:
      return PushLabel(context, loc, LabelType::Try,
                       value->block_type_immediate());

    case Opcode::Catch:
      return Catch(context, loc);

    case Opcode::Throw:
      return Throw(context, loc, value->index_immediate());

    case Opcode::Rethrow:
      return Rethrow(context, loc);

    case Opcode::BrOnExn:
      return BrOnExn(context, loc, value->br_on_exn_immediate());

    case Opcode::Br:
      return Br(context, loc, value->index_immediate());

    case Opcode::BrIf:
      return BrIf(context, loc, value->index_immediate());

    case Opcode::BrTable:
      return BrTable(context, loc, value->br_table_immediate());

    case Opcode::Return:
      return Br(context, loc, context.label_stack.size() - 1);

    case Opcode::Call:
      return Call(context, loc, value->index_immediate());

    case Opcode::CallIndirect:
      return CallIndirect(context, loc, value->call_indirect_immediate());

    case Opcode::Drop:
      return DropTypes(context, loc, 1);

    case Opcode::Select:
      return Select(context, loc);

    case Opcode::SelectT:
      return SelectT(context, loc, value->select_immediate());

    case Opcode::LocalGet:
      return LocalGet(context, value->index_immediate());

    case Opcode::LocalSet:
      return LocalSet(context, loc, value->index_immediate());

    case Opcode::LocalTee:
      return LocalTee(context, loc, value->index_immediate());

    case Opcode::GlobalGet:
      return GlobalGet(context, value->index_immediate());

    case Opcode::GlobalSet:
      return GlobalSet(context, loc, value->index_immediate());

    case Opcode::TableGet:
      return TableGet(context, loc, value->index_immediate());

    case Opcode::TableSet:
      return TableSet(context, loc, value->index_immediate());

    case Opcode::RefNull:
      PushType(context, ToStackType(value->heap_type_immediate()));
      return true;

    case Opcode::RefIsNull: {
      auto type = PopReferenceType(context, loc);
      PushType(context, StackType::I32());
      return AllTrue(type);
    }

    case Opcode::RefFunc:
      return RefFunc(context, loc, value->index_immediate());

    case Opcode::BrOnNull:
      return BrOnNull(context, loc, value->index_immediate());

    case Opcode::RefAsNonNull:
      return RefAsNonNull(context, loc);

    case Opcode::CallRef:
      return CallRef(context, loc);

    case Opcode::ReturnCallRef:
      return ReturnCallRef(context, loc);

    case Opcode::FuncBind:
      return FuncBind(context, loc, value->index_immediate());

    case Opcode::Let:
      return Let(context, loc, value->let_immediate());

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
      return Load(context, loc, value);

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
      return Store(context, loc, value);

    case Opcode::MemorySize:
      return MemorySize(context);

    case Opcode::MemoryGrow:
      return MemoryGrow(context, loc);

    case Opcode::I32Const:
      PushType(context, StackType::I32());
      return true;

    case Opcode::I64Const:
      PushType(context, StackType::I64());
      return true;

    case Opcode::F32Const:
      PushType(context, StackType::F32());
      return true;

    case Opcode::F64Const:
      PushType(context, StackType::F64());
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
      return ReturnCall(context, loc, value->index_immediate());

    case Opcode::ReturnCallIndirect:
      return ReturnCallIndirect(context, loc, value->call_indirect_immediate());

    case Opcode::MemoryInit:
      return MemoryInit(context, loc, value->init_immediate());

    case Opcode::DataDrop:
      return DataDrop(context, value->index_immediate());

    case Opcode::MemoryCopy:
      return MemoryCopy(context, loc, value->copy_immediate());

    case Opcode::MemoryFill:
      return MemoryFill(context, loc);

    case Opcode::TableInit:
      return TableInit(context, loc, value->init_immediate());

    case Opcode::ElemDrop:
      return ElemDrop(context, value->index_immediate());

    case Opcode::TableCopy:
      return TableCopy(context, loc, value->copy_immediate());

    case Opcode::TableGrow:
      return TableGrow(context, loc, value->index_immediate());

    case Opcode::TableSize:
      return TableSize(context, value->index_immediate());

    case Opcode::TableFill:
      return TableFill(context, loc, value->index_immediate());

    case Opcode::V128Const:
      PushType(context, StackType::V128());
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

    case Opcode::V8X16Shuffle:
      return SimdShuffle(context, loc, value->shuffle_immediate());

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
    case Opcode::I64X2ExtractLane:
    case Opcode::F32X4ExtractLane:
    case Opcode::F64X2ExtractLane:
    case Opcode::I8X16ReplaceLane:
    case Opcode::I16X8ReplaceLane:
    case Opcode::I32X4ReplaceLane:
    case Opcode::I64X2ReplaceLane:
    case Opcode::F32X4ReplaceLane:
    case Opcode::F64X2ReplaceLane:
      return SimdLane(context, loc, value);

    case Opcode::I8X16AnyTrue:
    case Opcode::I8X16AllTrue:
    case Opcode::I16X8AnyTrue:
    case Opcode::I16X8AllTrue:
    case Opcode::I32X4AnyTrue:
    case Opcode::I32X4AllTrue:
      params = span_v128, results = span_i32;
      break;

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

    case Opcode::MemoryAtomicNotify:
      return MemoryAtomicNotify(context, loc, value);

    case Opcode::MemoryAtomicWait32:
    case Opcode::MemoryAtomicWait64:
      return MemoryAtomicWait(context, loc, value);

    case Opcode::I32AtomicLoad:
    case Opcode::I64AtomicLoad:
    case Opcode::I32AtomicLoad8U:
    case Opcode::I32AtomicLoad16U:
    case Opcode::I64AtomicLoad8U:
    case Opcode::I64AtomicLoad16U:
    case Opcode::I64AtomicLoad32U:
      return AtomicLoad(context, loc, value);

    case Opcode::I32AtomicStore:
    case Opcode::I64AtomicStore:
    case Opcode::I32AtomicStore8:
    case Opcode::I32AtomicStore16:
    case Opcode::I64AtomicStore8:
    case Opcode::I64AtomicStore16:
    case Opcode::I64AtomicStore32:
      return AtomicStore(context, loc, value);

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
      return AtomicRmw(context, loc, value);

    case Opcode:: RefEq:
      params = span_eqref_eqref, results = span_i32;
      break;

    case Opcode:: I31New:
      params = span_i32, results = span_i31ref;
      break;

    case Opcode:: I31GetS:
    case Opcode:: I31GetU:
      params = span_i31ref, results = span_i32;
      break;

    case Opcode:: RttCanon:
      return RttCanon(context, loc, value->heap_type_immediate());

    case Opcode:: RttSub:
      return RttSub(context, loc, value->heap_type_immediate());

    case Opcode:: RefTest:
      return RefTest(context, loc, value->heap_type_2_immediate());

    case Opcode:: RefCast:
      return RefCast(context, loc, value->heap_type_2_immediate());

    case Opcode:: BrOnCast:
      return BrOnCast(context, loc, value->index_immediate());

    case Opcode:: StructNewWithRtt:
      return StructNewWithRtt(context, loc, value->index_immediate());

    case Opcode:: StructNewDefaultWithRtt:
      return StructNewDefaultWithRtt(context, loc, value->index_immediate());

    case Opcode:: StructGet:
      return StructGet(context, loc, value->struct_field_immediate());

    case Opcode:: StructGetS:
    case Opcode:: StructGetU:
      return StructGetPacked(context, loc, value->struct_field_immediate());

    case Opcode:: StructSet:
      return StructSet(context, loc, value->struct_field_immediate());

    case Opcode:: ArrayNewWithRtt:
      return ArrayNewWithRtt(context, loc, value->index_immediate());

    case Opcode:: ArrayNewDefaultWithRtt:
      return ArrayNewDefaultWithRtt(context, loc, value->index_immediate());

    case Opcode:: ArrayGet:
      return ArrayGet(context, loc, value->index_immediate());

    case Opcode:: ArrayGetS:
    case Opcode:: ArrayGetU:
      return ArrayGetPacked(context, loc, value->index_immediate());

    case Opcode:: ArraySet:
      return ArraySet(context, loc, value->index_immediate());

    case Opcode:: ArrayLen:
      return ArrayLen(context, loc, value->index_immediate());
  }

  return PopAndPushTypes(context, loc, params, results);
}

}  // namespace wasp::valid
