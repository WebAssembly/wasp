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

#include "wasp/text/read.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>
#include <type_traits>

#include "wasp/base/concat.h"
#include "wasp/base/errors.h"
#include "wasp/base/utf8.h"
#include "wasp/text/formatters.h"
#include "wasp/text/numeric.h"
#include "wasp/text/read/location_guard.h"
#include "wasp/text/read/macros.h"
#include "wasp/text/read/read_ctx.h"

namespace wasp::text {

auto Expect(Tokenizer& tokenizer, ReadCtx& ctx, TokenType expected)
    -> optional<Token> {
  auto actual_opt = tokenizer.Match(expected);
  if (!actual_opt) {
    auto token = tokenizer.Peek();
    ctx.errors.OnError(token.loc,
                       concat("Expected ", expected, ", got ", token.type));
    return nullopt;
  }
  return actual_opt;
}

auto ExpectLpar(Tokenizer& tokenizer, ReadCtx& ctx, TokenType expected)
    -> optional<Token> {
  auto actual_opt = tokenizer.MatchLpar(expected);
  if (!actual_opt) {
    auto token = tokenizer.Peek();
    ctx.errors.OnError(
        token.loc, concat("Expected '(' ", expected, ", got ", token.type, " ",
                          tokenizer.Peek(1).type));
    return nullopt;
  }
  return actual_opt;
}

template <typename T>
auto ReadNat(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<T> {
  auto token_opt = tokenizer.Match(TokenType::Nat);
  if (!token_opt) {
    auto token = tokenizer.Peek();
    ctx.errors.OnError(token.loc,
                       concat("Expected a natural number, got ", token.type));
    return nullopt;
  }
  auto nat_opt = StrToNat<T>(token_opt->literal_info(), token_opt->span_u8());
  if (!nat_opt) {
    ctx.errors.OnError(token_opt->loc,
                       concat("Invalid natural number, got ", token_opt->type));
    return nullopt;
  }
  return At{token_opt->loc, *nat_opt};
}

auto ReadNat32(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<u32> {
  return ReadNat<u32>(tokenizer, ctx);
}

template <typename T>
auto ReadInt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<T> {
  auto token = tokenizer.Peek();
  if (!(token.type == TokenType::Nat || token.type == TokenType::Int)) {
    ctx.errors.OnError(token.loc,
                       concat("Expected an integer, got ", token.type));
    return nullopt;
  }

  tokenizer.Read();
  auto int_opt = StrToInt<T>(token.literal_info(), token.span_u8());
  if (!int_opt) {
    ctx.errors.OnError(token.loc, concat("Invalid integer, got ", token.type));
    return nullopt;
  }
  return At{token.loc, *int_opt};
}

template <typename T>
auto ReadFloat(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<T> {
  auto token = tokenizer.Peek();
  if (!(token.type == TokenType::Nat || token.type == TokenType::Int ||
        token.type == TokenType::Float)) {
    ctx.errors.OnError(token.loc, concat("Expected a float, got ", token.type));
    return nullopt;
  }

  tokenizer.Read();
  auto float_opt = StrToFloat<T>(token.literal_info(), token.span_u8());
  if (!float_opt) {
    ctx.errors.OnError(token.loc, concat("Invalid float, got ", token));
    return nullopt;
  }
  return At{token.loc, *float_opt};
}

bool IsVar(Token token) {
  return token.type == TokenType::Id || token.type == TokenType::Nat;
}

auto ReadVar(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Var> {
  auto token = tokenizer.Peek();
  auto var_opt = ReadVarOpt(tokenizer, ctx);
  if (!var_opt) {
    ctx.errors.OnError(token.loc,
                       concat("Expected a variable, got ", token.type));
    return nullopt;
  }
  return *var_opt;
}

auto ReadVarOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Var> {
  auto token = tokenizer.Peek();
  if (token.type == TokenType::Id) {
    tokenizer.Read();
    return At{token.loc, Var{token.as_string_view()}};
  } else if (token.type == TokenType::Nat) {
    WASP_TRY_READ(nat, ReadNat32(tokenizer, ctx));
    return At{nat.loc(), Var{nat.value()}};
  } else {
    return nullopt;
  }
}

auto ReadVarList(Tokenizer& tokenizer, ReadCtx& ctx) -> optional<VarList> {
  VarList result;
  OptAt<Var> var_opt;
  while ((var_opt = ReadVarOpt(tokenizer, ctx))) {
    result.push_back(*var_opt);
  }
  return result;
}

auto ReadNonEmptyVarList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<VarList> {
  VarList result;
  WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
  result.push_back(var);

  WASP_TRY_READ(var_list, ReadVarList(tokenizer, ctx));
  result.insert(result.end(), std::make_move_iterator(var_list.begin()),
                std::make_move_iterator(var_list.end()));
  return result;
}

auto ReadVarUseOpt(Tokenizer& tokenizer, ReadCtx& ctx, TokenType token_type)
    -> OptAt<Var> {
  LocationGuard guard{tokenizer};
  if (!tokenizer.MatchLpar(token_type)) {
    return nullopt;
  }
  WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), var.value()};
}

auto ReadTypeUseOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Var> {
  return ReadVarUseOpt(tokenizer, ctx, TokenType::Type);
}

auto ReadFunctionTypeUse(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<FunctionTypeUse> {
  auto type_use = ReadTypeUseOpt(tokenizer, ctx);
  WASP_TRY_READ(type, ReadFunctionType(tokenizer, ctx));
  return FunctionTypeUse{type_use, type};
}

auto ReadText(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Text> {
  auto token_opt = tokenizer.Match(TokenType::Text);
  if (!token_opt) {
    auto token = tokenizer.Peek();
    ctx.errors.OnError(token.loc,
                       concat("Expected quoted text, got ", token.type));
    return nullopt;
  }
  return At{token_opt->loc, token_opt->text()};
}

auto ReadUtf8Text(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Text> {
  WASP_TRY_READ(text, ReadText(tokenizer, ctx));
  // TODO: The lexer could validate utf-8 while reading characters.
  if (!IsValidUtf8(text->ToString())) {
    ctx.errors.OnError(text.loc(), "Invalid UTF-8 encoding");
    return nullopt;
  }
  return text;
}

auto ReadTextList(Tokenizer& tokenizer, ReadCtx& ctx) -> optional<TextList> {
  TextList result;
  while (tokenizer.Peek().type == TokenType::Text) {
    WASP_TRY_READ(text, ReadText(tokenizer, ctx));
    result.push_back(text);
  }
  return result;
}

// Section 1: Type

auto ReadBindVarOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<BindVar> {
  auto token_opt = tokenizer.Match(TokenType::Id);
  if (!token_opt) {
    return nullopt;
  }

  auto name = token_opt->as_string_view();
  return At{token_opt->loc, BindVar{name}};
}

auto ReadBoundValueTypeList(Tokenizer& tokenizer,
                            ReadCtx& ctx,
                            TokenType token_type)
    -> optional<BoundValueTypeList> {
  BoundValueTypeList result;
  while (tokenizer.MatchLpar(token_type)) {
    if (tokenizer.Peek().type == TokenType::Id) {
      LocationGuard guard{tokenizer};
      auto bind_var_opt = ReadBindVarOpt(tokenizer, ctx);
      WASP_TRY_READ(value_type, ReadValueType(tokenizer, ctx));
      result.push_back(
          At{guard.loc(), BoundValueType{bind_var_opt, value_type}});
    } else {
      WASP_TRY_READ(value_types, ReadValueTypeList(tokenizer, ctx));
      for (auto& value_type : value_types) {
        result.push_back(
            At{value_type.loc(), BoundValueType{nullopt, value_type}});
      }
    }
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  }
  return result;
}

auto ReadBoundParamList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<BoundValueTypeList> {
  return ReadBoundValueTypeList(tokenizer, ctx, TokenType::Param);
}

auto ReadUnboundValueTypeList(Tokenizer& tokenizer,
                              ReadCtx& ctx,
                              TokenType token_type) -> optional<ValueTypeList> {
  ValueTypeList result;
  while (tokenizer.MatchLpar(token_type)) {
    WASP_TRY_READ(value_types, ReadValueTypeList(tokenizer, ctx));
    std::copy(value_types.begin(), value_types.end(),
              std::back_inserter(result));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  }
  return result;
}

auto ReadParamList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<ValueTypeList> {
  return ReadUnboundValueTypeList(tokenizer, ctx, TokenType::Param);
}

auto ReadResultList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<ValueTypeList> {
  return ReadUnboundValueTypeList(tokenizer, ctx, TokenType::Result);
}

auto ReadRtt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Rtt> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Rtt));
  WASP_TRY_READ(depth, ReadNat32(tokenizer, ctx));
  WASP_TRY_READ(type, ReadHeapType(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), Rtt{depth, type}};
}

bool IsReferenceType(Tokenizer& tokenizer) {
  auto token = tokenizer.Peek();
  return token.type == TokenType::ReferenceKind ||
         (token.type == TokenType::Lpar &&
          tokenizer.Peek(1).type == TokenType::Ref);
}

bool IsValueType(Tokenizer& tokenizer) {
  auto token = tokenizer.Peek();
  return token.type == TokenType::NumericType ||
         (token.type == TokenType::Lpar &&
          tokenizer.Peek(1).type == TokenType::Rtt) ||
         IsReferenceType(tokenizer);
}

auto ReadValueType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<ValueType> {
  auto token = tokenizer.Peek();
  if (token.type == TokenType::NumericType) {
    assert(token.has_numeric_type());
    tokenizer.Read();
    auto numeric_type = token.numeric_type();

    bool allowed = true;
    switch (numeric_type) {
#define WASP_V(val, Name, str)
#define WASP_FEATURE_V(val, Name, str, feature) \
  case NumericType::Name:                       \
    if (!ctx.features.feature##_enabled()) {    \
      allowed = false;                          \
    }                                           \
    break;
#include "wasp/base/inc/numeric_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V

      default:
        break;
    }

    if (!allowed) {
      ctx.errors.OnError(token.loc,
                         concat("value type ", numeric_type, " not allowed"));
      return nullopt;
    }
    return At{token.loc, ValueType{numeric_type}};
  } else if (token.type == TokenType::Lpar &&
             tokenizer.Peek(1).type == TokenType::Rtt) {
    WASP_TRY_READ(rtt, ReadRtt(tokenizer, ctx));
    return At{rtt.loc(), ValueType{rtt}};
  } else {
    WASP_TRY_READ(reference_type,
                  ReadReferenceType(tokenizer, ctx, AllowFuncref::No));
    return At{reference_type.loc(), ValueType{reference_type}};
  }
}

auto ReadValueTypeList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<ValueTypeList> {
  ValueTypeList result;
  while (IsValueType(tokenizer)) {
    WASP_TRY_READ(value, ReadValueType(tokenizer, ctx));
    result.push_back(value);
  }
  return result;
}

auto ReadBoundFunctionType(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<BoundFunctionType> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(params, ReadBoundParamList(tokenizer, ctx));
  WASP_TRY_READ(results, ReadResultList(tokenizer, ctx));
  return At{guard.loc(), BoundFunctionType{params, results}};
}

auto ReadStorageType(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<StorageType> {
  if (auto token_opt = tokenizer.Match(TokenType::PackedType)) {
    assert(token_opt->has_packed_type());
    return At{token_opt->loc, StorageType{token_opt->packed_type()}};
  } else {
    WASP_TRY_READ(value_type, ReadValueType(tokenizer, ctx));
    return At{value_type.loc(), StorageType{value_type}};
  }
}

bool IsFieldTypeContents(Tokenizer& tokenizer) {
  auto token = tokenizer.Peek();
  return (token.type == TokenType::Lpar &&
          tokenizer.Peek(1).type == TokenType::Mut) ||
         IsValueType(tokenizer) || token.type == TokenType::PackedType;
}

auto ReadFieldTypeContents(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<FieldType> {
  LocationGuard guard{tokenizer};
  auto token_opt = tokenizer.MatchLpar(TokenType::Mut);
  WASP_TRY_READ(type, ReadStorageType(tokenizer, ctx));

  At<Mutability> mut;
  if (token_opt) {
    mut = At{token_opt->loc, Mutability::Var};
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  } else {
    mut = Mutability::Const;
  }
  return At{guard.loc(), FieldType{nullopt, type, mut}};
}

auto ReadFieldType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<FieldType> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Field));
  auto name = ReadBindVarOpt(tokenizer, ctx);
  WASP_TRY_READ(field, ReadFieldTypeContents(tokenizer, ctx));
  field->name = name;
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), field.value()};
}

auto ReadFieldTypeList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<FieldTypeList> {
  FieldTypeList result;
  while (tokenizer.MatchLpar(TokenType::Field)) {
    if (tokenizer.Peek().type == TokenType::Id) {
      // LPAR FIELD bind_var_opt (storagetype | LPAR MUT storagetype RPAR) RPAR
      LocationGuard guard{tokenizer};
      auto bind_var_opt = ReadBindVarOpt(tokenizer, ctx);
      WASP_TRY_READ(field, ReadFieldTypeContents(tokenizer, ctx));
      field->name = bind_var_opt;
      result.push_back(At{guard.loc(), field.value()});
    } else {
      // LPAR FIELD (storagetype | LPAR MUT storagetype RPAR)* RPAR
      while (IsFieldTypeContents(tokenizer)) {
        WASP_TRY_READ(field, ReadFieldTypeContents(tokenizer, ctx));
        result.push_back(field);
      }
    }
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  }
  return result;
}

auto ReadStructType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<StructType> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Struct));
  WASP_TRY_READ(fields, ReadFieldTypeList(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), StructType{fields}};
}

auto ReadArrayType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<ArrayType> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Array));
  WASP_TRY_READ(field, ReadFieldTypeContents(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), ArrayType{field}};
}

auto ReadDefinedType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<DefinedType> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Type));
  auto name = ReadBindVarOpt(tokenizer, ctx);

  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    ctx.errors.OnError(token.loc, concat("Expected '(', got ", token.type));
    return nullopt;
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Func: {
      WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Func));
      WASP_TRY_READ(func_type, ReadBoundFunctionType(tokenizer, ctx));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      return At{guard.loc(), DefinedType{name, func_type}};
    }

    case TokenType::Struct: {
      if (!ctx.features.gc_enabled()) {
        ctx.errors.OnError(token.loc, "Structs not allowed");
        return nullopt;
      }
      WASP_TRY_READ(struct_type, ReadStructType(tokenizer, ctx));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      return At{guard.loc(), DefinedType{name, struct_type}};
    }

    case TokenType::Array: {
      if (!ctx.features.gc_enabled()) {
        ctx.errors.OnError(token.loc, "Arrays not allowed");
        return nullopt;
      }
      WASP_TRY_READ(array_type, ReadArrayType(tokenizer, ctx));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      return At{guard.loc(), DefinedType{name, array_type}};
    }

    default:
      ctx.errors.OnError(
          token.loc,
          concat("Expected `func`, `struct`, or `array`, got", token.type));
      return nullopt;
  }
}

// Section 2: Import

auto ReadInlineImportOpt(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<InlineImport> {
  LocationGuard guard{tokenizer};
  auto import_token = tokenizer.MatchLpar(TokenType::Import);
  if (!import_token) {
    return nullopt;
  }

  if (ctx.seen_non_import) {
    ctx.errors.OnError(import_token->loc,
                       "Imports must occur before all non-import definitions");
    return nullopt;
  }
  WASP_TRY_READ(module, ReadUtf8Text(tokenizer, ctx));
  WASP_TRY_READ(name, ReadUtf8Text(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), InlineImport{module, name}};
}

auto ReadImport(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Import> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(import_token, ExpectLpar(tokenizer, ctx, TokenType::Import));

  if (ctx.seen_non_import) {
    ctx.errors.OnError(import_token.loc,
                       "Imports must occur before all non-import definitions");
    return nullopt;
  }

  Import result;
  WASP_TRY_READ(module, ReadUtf8Text(tokenizer, ctx));
  WASP_TRY_READ(name, ReadUtf8Text(tokenizer, ctx));
  result.module = module;
  result.name = name;

  WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::Func: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, ctx);
      auto type_use = ReadTypeUseOpt(tokenizer, ctx);
      WASP_TRY_READ(type, ReadBoundFunctionType(tokenizer, ctx));
      result.desc = FunctionDesc{name, type_use, type};
      break;
    }

    case TokenType::Table: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, ctx);
      WASP_TRY_READ(type, ReadTableType(tokenizer, ctx));
      result.desc = TableDesc{name, type};
      break;
    }

    case TokenType::Memory: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, ctx);
      WASP_TRY_READ(type, ReadMemoryType(tokenizer, ctx));
      result.desc = MemoryDesc{name, type};
      break;
    }

    case TokenType::Global: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, ctx);
      WASP_TRY_READ(type, ReadGlobalType(tokenizer, ctx));
      result.desc = GlobalDesc{name, type};
      break;
    }

    case TokenType::Tag: {
      if (!ctx.features.exceptions_enabled()) {
        ctx.errors.OnError(token.loc, "Tags not allowed");
        return nullopt;
      }
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, ctx);
      WASP_TRY_READ(type, ReadTagType(tokenizer, ctx));
      result.desc = TagDesc{name, type};
      break;
    }

    default:
      ctx.errors.OnError(
          token.loc, concat("Expected an import external kind, got ", token));
      return nullopt;
  }

  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), result};
}

// Section 3: Function

auto ReadLocalList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<BoundValueTypeList> {
  return ReadBoundValueTypeList(tokenizer, ctx, TokenType::Local);
}

auto ReadFunctionType(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<FunctionType> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(params, ReadParamList(tokenizer, ctx));
  WASP_TRY_READ(results, ReadResultList(tokenizer, ctx));
  return At{guard.loc(), FunctionType{params, results}};
}

auto ReadFunction(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Function> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Func));

  BoundValueTypeList locals;
  InstructionList instructions;

  auto name = ReadBindVarOpt(tokenizer, ctx);
  WASP_TRY_READ(exports, ReadInlineExportList(tokenizer, ctx));
  auto import_opt = ReadInlineImportOpt(tokenizer, ctx);
  ctx.seen_non_import |= !import_opt;

  auto type_use = ReadTypeUseOpt(tokenizer, ctx);
  WASP_TRY_READ(type, ReadBoundFunctionType(tokenizer, ctx));

  if (!import_opt) {
    WASP_TRY_READ(locals_, ReadLocalList(tokenizer, ctx));
    locals = locals_;
    WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
    WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
  } else {
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  }

  return At{guard.loc(), Function{FunctionDesc{name, type_use, type}, locals,
                                  instructions, import_opt, exports}};
}

// Section 4: Table

auto ReadIndexTypeOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<IndexType> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  if (ctx.features.memory64_enabled() && token.type == TokenType::NumericType) {
    if (token.numeric_type() == NumericType::I32) {
      tokenizer.Read();
      return At{token.loc, IndexType::I32};
    } else if (token.numeric_type() == NumericType::I64) {
      tokenizer.Read();
      return At{token.loc, IndexType::I64};
    }
  }
  return nullopt;
}

auto ReadLimits(Tokenizer& tokenizer, ReadCtx& ctx, LimitsKind kind)
    -> OptAt<Limits> {
  LocationGuard guard{tokenizer};

  At<IndexType> index_type = IndexType::I32;
  if (kind == LimitsKind::Memory) {
    index_type = ReadIndexTypeOpt(tokenizer, ctx).value_or(IndexType::I32);
  }

  WASP_TRY_READ(min, ReadNat32(tokenizer, ctx));
  auto token = tokenizer.Peek();
  OptAt<u32> max_opt;
  if (token.type == TokenType::Nat) {
    WASP_TRY_READ(max, ReadNat32(tokenizer, ctx));
    max_opt = max;
  }

  token = tokenizer.Peek();
  At<Shared> shared = Shared::No;
  if (ctx.features.threads_enabled() && kind == LimitsKind::Memory &&
      token.type == TokenType::Shared) {
    tokenizer.Read();
    shared = At{token.loc, Shared::Yes};
  }

  if (shared == Shared::Yes && index_type == IndexType::I64) {
    ctx.errors.OnError(token.loc, "limits cannot be shared and have i64 index");
    return nullopt;
  }

  return At{guard.loc(), Limits{min, max_opt, shared, index_type}};
}

auto ReadHeapType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<HeapType> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  if (token.has_heap_kind()) {
    tokenizer.Read();

    bool allowed = true;
    auto heap_kind = token.heap_kind();

    switch (heap_kind) {
#define WASP_V(val, Name, str)
#define WASP_FEATURE_V(val, Name, str, feature) \
  case HeapKind::Name:                          \
    if (!ctx.features.feature##_enabled()) {    \
      allowed = false;                          \
    }                                           \
    break;
#include "wasp/base/inc/heap_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V

      default:
        break;
    }
    if (!allowed) {
      ctx.errors.OnError(token.loc,
                         concat("heap type ", heap_kind, " not allowed"));
      return nullopt;
    }
    return At{guard.loc(), HeapType{heap_kind}};
  } else if (IsVar(token)) {
    WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
    return At{guard.loc(), HeapType{var}};
  } else if (tokenizer.MatchLpar(TokenType::Type)) {
    WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), HeapType{var}};
  } else {
    ctx.errors.OnError(token.loc,
                       concat("Expected heap type, got ", token.type));
    return nullopt;
  }
}

auto ReadRefType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<RefType> {
  LocationGuard guard{tokenizer};
  Null null = Null::No;
  if (tokenizer.Match(TokenType::Null)) {
    null = Null::Yes;
  }

  WASP_TRY_READ(heap_type, ReadHeapType(tokenizer, ctx));
  return At{guard.loc(), RefType{heap_type, null}};
}

auto ReadReferenceType(Tokenizer& tokenizer,
                       ReadCtx& ctx,
                       AllowFuncref allow_funcref) -> OptAt<ReferenceType> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  if (token.type == TokenType::ReferenceKind) {
    tokenizer.Read();

    bool allowed = true;
    auto reference_kind = token.reference_kind();

    switch (reference_kind) {
#define WASP_V(val, Name, str)
#define WASP_FEATURE_V(val, Name, str, feature) \
  case ReferenceKind::Name:                     \
    if (!ctx.features.feature##_enabled()) {    \
      allowed = false;                          \
    }                                           \
    break;
#include "wasp/base/inc/reference_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V

      default:
        break;
    }

    // Ugly check to make sure that we don't allow funcref when reading a value
    // type, unless the reference types feature is enabled.
    if (reference_kind == ReferenceKind::Funcref &&
        allow_funcref == AllowFuncref::No &&
        !ctx.features.reference_types_enabled()) {
      allowed = false;
    }

    if (!allowed) {
      ctx.errors.OnError(
          token.loc, concat("reference type ", reference_kind, " not allowed"));
      return nullopt;
    }
    return At{token.loc, ReferenceType{reference_kind}};
  } else if (tokenizer.MatchLpar(TokenType::Ref)) {
    WASP_TRY_READ(ref_type, ReadRefType(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), ReferenceType{ref_type}};
  } else {
    ctx.errors.OnError(token.loc,
                       concat("Expected reference type, got ", token.type));
    return nullopt;
  }
}

auto ReadReferenceTypeOpt(Tokenizer& tokenizer,
                          ReadCtx& ctx,
                          AllowFuncref allow_funcref) -> OptAt<ReferenceType> {
  if (!IsReferenceType(tokenizer)) {
    return nullopt;
  }

  return ReadReferenceType(tokenizer, ctx, allow_funcref);
}

auto ReadTableType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<TableType> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(limits, ReadLimits(tokenizer, ctx, LimitsKind::Table));
  WASP_TRY_READ(element, ReadReferenceType(tokenizer, ctx));
  return At{guard.loc(), TableType{limits, element}};
}

auto ReadTable(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Table> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Table));

  auto name = ReadBindVarOpt(tokenizer, ctx);
  WASP_TRY_READ(exports, ReadInlineExportList(tokenizer, ctx));
  auto import_opt = ReadInlineImportOpt(tokenizer, ctx);
  ctx.seen_non_import |= !import_opt;

  auto elemtype_opt = ReadReferenceTypeOpt(tokenizer, ctx);
  if (import_opt) {
    // Imported table.
    WASP_TRY_READ(type, ReadTableType(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), Table{TableDesc{name, type}, *import_opt, exports}};
  } else if (elemtype_opt) {
    // Inline element segment.
    WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Elem));

    ElementList elements;
    u32 size;
    if (ctx.features.bulk_memory_enabled() && IsExpression(tokenizer)) {
      // Element expression list.
      WASP_TRY_READ(expressions, ReadElementExpressionList(tokenizer, ctx));
      size = static_cast<u32>(expressions.size());
      elements =
          ElementList{ElementListWithExpressions{*elemtype_opt, expressions}};
    } else {
      // Element var list.
      WASP_TRY_READ(vars, ReadVarList(tokenizer, ctx));
      size = static_cast<u32>(vars.size());
      elements = ElementList{ElementListWithVars{ExternalKind::Function, vars}};
    }

    // Implicit table type.
    auto type = TableType{Limits{size, size}, *elemtype_opt};

    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), Table{TableDesc{name, type}, exports, elements}};
  } else {
    // Defined table.
    WASP_TRY_READ(type, ReadTableType(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), Table{TableDesc{name, type}, exports}};
  }
}

// Section 5: Memory

template <typename T>
void AppendToBuffer(Buffer& buffer, const T& value) {
  auto old_size = buffer.size();
  buffer.resize(old_size + sizeof(value));
  // TODO: Handle big endian.
  memcpy(buffer.data() + old_size, &value, sizeof(value));
}

template <typename T>
bool ReadIntsIntoBuffer(Tokenizer& tokenizer, ReadCtx& ctx, Buffer& buffer) {
  while (true) {
    auto token = tokenizer.Peek();
    if (!(token.type == TokenType::Nat || token.type == TokenType::Int)) {
      break;
    }
    WASP_TRY_READ(value, ReadInt<T>(tokenizer, ctx));
    AppendToBuffer(buffer, *value);
  }
  return true;
}

template <typename T>
bool ReadFloatsIntoBuffer(Tokenizer& tokenizer, ReadCtx& ctx, Buffer& buffer) {
  while (true) {
    auto token = tokenizer.Peek();
    if (!(token.type == TokenType::Nat || token.type == TokenType::Int ||
          token.type == TokenType::Float)) {
      break;
    }
    WASP_TRY_READ(value, ReadFloat<T>(tokenizer, ctx));
    AppendToBuffer(buffer, *value);
  }
  return true;
}

auto ReadSimdConst(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<v128> {
  auto shape_token = tokenizer.Match(TokenType::SimdShape);
  if (!shape_token) {
    ctx.errors.OnError(shape_token->loc, concat("Invalid SIMD shape, got ",
                                                tokenizer.Peek().type));
    return nullopt;
  }

  At<v128> literal;
  switch (shape_token->simd_shape()) {
    case SimdShape::I8X16: {
      WASP_TRY_READ(literal_, (ReadSimdValues<u8, 16>(tokenizer, ctx)));
      literal = literal_;
      break;
    }

    case SimdShape::I16X8: {
      WASP_TRY_READ(literal_, (ReadSimdValues<u16, 8>(tokenizer, ctx)));
      literal = literal_;
      break;
    }

    case SimdShape::I32X4: {
      WASP_TRY_READ(literal_, (ReadSimdValues<u32, 4>(tokenizer, ctx)));
      literal = literal_;
      break;
    }

    case SimdShape::I64X2: {
      WASP_TRY_READ(literal_, (ReadSimdValues<u64, 2>(tokenizer, ctx)));
      literal = literal_;
      break;
    }

    case SimdShape::F32X4: {
      WASP_TRY_READ(literal_, (ReadSimdValues<f32, 4>(tokenizer, ctx)));
      literal = literal_;
      break;
    }

    case SimdShape::F64X2: {
      WASP_TRY_READ(literal_, (ReadSimdValues<f64, 2>(tokenizer, ctx)));
      literal = literal_;
      break;
    }

    default:
      WASP_UNREACHABLE();
  }

  return literal;
}

bool ReadSimdConstsIntoBuffer(Tokenizer& tokenizer,
                              ReadCtx& ctx,
                              Buffer& buffer) {
  while (true) {
    if (tokenizer.Peek().type != TokenType::SimdShape) {
      break;
    }
    WASP_TRY_READ(value, ReadSimdConst(tokenizer, ctx));
    AppendToBuffer(buffer, *value);
  }
  return true;
}

auto ReadNumericData(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<NumericData> {
  LocationGuard guard{tokenizer};
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));

  NumericDataType type;
  Buffer buffer;
  auto token = tokenizer.Peek();
  if (token.has_numeric_type()) {
    tokenizer.Read();
    switch (token.numeric_type()) {
      case NumericType::I32:
        type = NumericDataType::I32;
        WASP_TRY(ReadIntsIntoBuffer<s32>(tokenizer, ctx, buffer));
        break;
      case NumericType::I64:
        type = NumericDataType::I64;
        WASP_TRY(ReadIntsIntoBuffer<s64>(tokenizer, ctx, buffer));
        break;
      case NumericType::F32:
        type = NumericDataType::F32;
        WASP_TRY(ReadFloatsIntoBuffer<f32>(tokenizer, ctx, buffer));
        break;
      case NumericType::F64:
        type = NumericDataType::F64;
        WASP_TRY(ReadFloatsIntoBuffer<f64>(tokenizer, ctx, buffer));
        break;
      case NumericType::V128:
        type = NumericDataType::V128;
        WASP_TRY(ReadSimdConstsIntoBuffer(tokenizer, ctx, buffer));
        break;
      default:
        WASP_UNREACHABLE();
    }
  } else if (token.has_packed_type()) {
    tokenizer.Read();
    switch (token.packed_type()) {
      case PackedType::I8:
        type = NumericDataType::I8;
        WASP_TRY(ReadIntsIntoBuffer<s8>(tokenizer, ctx, buffer));
        break;
      case PackedType::I16:
        type = NumericDataType::I16;
        WASP_TRY(ReadIntsIntoBuffer<s16>(tokenizer, ctx, buffer));
        break;
      default:
        WASP_UNREACHABLE();
    }
  } else {
    ctx.errors.OnError(token.loc,
                       "Expected a numeric type: i8, i16, i32, i64, f32, f64");
    return nullopt;
  }
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), NumericData{type, std::move(buffer)}};
}

bool IsDataItem(Tokenizer& tokenizer) {
  auto token = tokenizer.Peek();
  return token.type == TokenType::Text ||
         (token.type == TokenType::Lpar &&
          (tokenizer.Peek(1).type == TokenType::PackedType ||
           tokenizer.Peek(1).type == TokenType::NumericType));
}

auto ReadDataItem(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<DataItem> {
  auto token = tokenizer.Peek();
  if (token.type == TokenType::Text) {
    tokenizer.Read();
    return At{token.loc, DataItem{token.text()}};
  } else if (ctx.features.numeric_values_enabled()) {
    WASP_TRY_READ(numeric_data, ReadNumericData(tokenizer, ctx));
    return At{numeric_data.loc(), DataItem{numeric_data}};
  } else {
    ctx.errors.OnError(token.loc, "Numeric values not allowed");
    return nullopt;
  }
}

auto ReadDataItemList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<DataItemList> {
  DataItemList result;
  while (IsDataItem(tokenizer)) {
    WASP_TRY_READ(value, ReadDataItem(tokenizer, ctx));
    result.push_back(value);
  }
  return result;
}

auto ReadMemoryType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<MemoryType> {
  WASP_TRY_READ(limits, ReadLimits(tokenizer, ctx, LimitsKind::Memory));
  return At{limits.loc(), MemoryType{limits}};
}

auto ReadMemory(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Memory> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Memory));

  auto name = ReadBindVarOpt(tokenizer, ctx);
  WASP_TRY_READ(exports, ReadInlineExportList(tokenizer, ctx));
  auto import_opt = ReadInlineImportOpt(tokenizer, ctx);
  ctx.seen_non_import |= !import_opt;

  if (import_opt) {
    // Imported memory.
    WASP_TRY_READ(type, ReadMemoryType(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(),
              Memory{MemoryDesc{name, type}, *import_opt, exports}};
  } else {
    // MEMORY * index_type? LPAR DATA ...
    // MEMORY * index_type? nat ...
    if ((ctx.features.memory64_enabled() &&
         tokenizer.Peek().type == TokenType::NumericType &&
         tokenizer.Peek(1).type == TokenType::Lpar) ||
        tokenizer.Peek().type == TokenType::Lpar) {
      // Inline data segment.
      auto index_type =
          ReadIndexTypeOpt(tokenizer, ctx).value_or(IndexType::I32);
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Data));
      WASP_TRY_READ(data, ReadDataItemList(tokenizer, ctx));
      auto size = std::accumulate(
          data.begin(), data.end(), u32{0},
          [](u32 total, DataItem item) { return total + item.byte_size(); });

      // Implicit memory type.
      auto type = MemoryType{Limits{size, size, Shared::No, index_type}};

      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      return At{guard.loc(), Memory{MemoryDesc{name, type}, exports, data}};
    } else {
      // Defined memory.
      WASP_TRY_READ(type, ReadMemoryType(tokenizer, ctx));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
      return At{guard.loc(), Memory{MemoryDesc{name, type}, exports}};
    }
  }
}

// Section 6: Global

auto ReadConstantExpression(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<ConstantExpression> {
  LocationGuard guard{tokenizer};
  InstructionList instructions;
  WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
  return At{guard.loc(), ConstantExpression{instructions}};
}

auto ReadGlobalType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<GlobalType> {
  LocationGuard guard{tokenizer};

  auto token_opt = tokenizer.MatchLpar(TokenType::Mut);
  WASP_TRY_READ(valtype, ReadValueType(tokenizer, ctx));

  At<Mutability> mut;
  if (token_opt) {
    mut = At{token_opt->loc, Mutability::Var};
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  } else {
    mut = Mutability::Const;
  }
  return At{guard.loc(), GlobalType{valtype, mut}};
}

auto ReadGlobal(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Global> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Global));

  auto name = ReadBindVarOpt(tokenizer, ctx);
  WASP_TRY_READ(exports, ReadInlineExportList(tokenizer, ctx));
  auto import_opt = ReadInlineImportOpt(tokenizer, ctx);
  ctx.seen_non_import |= !import_opt;

  WASP_TRY_READ(type, ReadGlobalType(tokenizer, ctx));

  At<ConstantExpression> init;
  if (!import_opt) {
    WASP_TRY_READ(init_, ReadConstantExpression(tokenizer, ctx));
    init = init_;
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), Global{GlobalDesc{name, type}, init, exports}};
  }

  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), Global{GlobalDesc{name, type}, *import_opt, exports}};
}

// Section 7: Export

auto ReadInlineExport(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<InlineExport> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Export));
  WASP_TRY_READ(name, ReadUtf8Text(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), InlineExport{name}};
}

auto ReadInlineExportList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<InlineExportList> {
  InlineExportList result;
  while (tokenizer.Peek().type == TokenType::Lpar &&
         tokenizer.Peek(1).type == TokenType::Export) {
    WASP_TRY_READ(export_, ReadInlineExport(tokenizer, ctx));
    result.push_back(export_);
  }
  return result;
}

auto ReadExport(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Export> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Export));

  WASP_TRY_READ(name, ReadUtf8Text(tokenizer, ctx));

  At<ExternalKind> kind;

  WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::Func:
      kind = At{token.loc, ExternalKind::Function};
      break;

    case TokenType::Table:
      kind = At{token.loc, ExternalKind::Table};
      break;

    case TokenType::Memory:
      kind = At{token.loc, ExternalKind::Memory};
      break;

    case TokenType::Global:
      kind = At{token.loc, ExternalKind::Global};
      break;

    case TokenType::Tag:
      if (!ctx.features.exceptions_enabled()) {
        ctx.errors.OnError(token.loc, "Tags not allowed");
        return nullopt;
      }
      kind = At{token.loc, ExternalKind::Tag};
      break;

    default:
      ctx.errors.OnError(
          token.loc,
          concat("Expected an import external kind, got ", token.type));
      return nullopt;
  }

  tokenizer.Read();
  WASP_TRY_READ(var, ReadVar(tokenizer, ctx));

  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));

  return At{guard.loc(), Export{kind, name, var}};
}

// Section 8: Start

auto ReadStart(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Start> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(start_token, ExpectLpar(tokenizer, ctx, TokenType::Start));

  if (ctx.seen_start) {
    ctx.errors.OnError(start_token.loc, "Multiple start functions");
    return nullopt;
  }
  ctx.seen_start = true;

  WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), Start{var}};
}

// Section 9: Elem

auto ReadOffsetExpression(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<ConstantExpression> {
  LocationGuard guard{tokenizer};
  InstructionList instructions;
  if (tokenizer.MatchLpar(TokenType::Offset)) {
    WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  } else if (IsExpression(tokenizer)) {
    WASP_TRY(ReadExpression(tokenizer, ctx, instructions));
  } else {
    auto token = tokenizer.Peek();
    ctx.errors.OnError(token.loc,
                       concat("Expected offset expression, got ", token.type));
    return nullopt;
  }
  return At{guard.loc(), ConstantExpression{instructions}};
}

auto ReadElementExpression(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<ElementExpression> {
  LocationGuard guard{tokenizer};
  InstructionList instructions;

  // Element expressions were first added in the bulk memory proposal, so it
  // shouldn't be read (and this function shouldn't be called) if that feature
  // is not enabled.
  assert(ctx.features.bulk_memory_enabled());
  // The only valid instructions are enabled by the reference types proposal,
  // but their encoding is still used by the bulk memory proposal.
  Features new_features;
  new_features.enable_reference_types();
  ReadCtx new_context{new_features, ctx.errors};

  if (tokenizer.MatchLpar(TokenType::Item)) {
    WASP_TRY(ReadInstructionList(tokenizer, new_context, instructions));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  } else if (IsExpression(tokenizer)) {
    WASP_TRY(ReadExpression(tokenizer, new_context, instructions));
  } else {
    auto token = tokenizer.Peek();
    ctx.errors.OnError(token.loc,
                       concat("Expected element expression, got ", token.type));
    return nullopt;
  }
  return At{guard.loc(), ElementExpression{instructions}};
}

auto ReadElementExpressionList(Tokenizer& tokenizer, ReadCtx& ctx)
    -> optional<ElementExpressionList> {
  ElementExpressionList result;
  while (IsElementExpression(tokenizer)) {
    WASP_TRY_READ(expression, ReadElementExpression(tokenizer, ctx));
    result.push_back(expression);
  }
  return result;
}

auto ReadTableUseOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Var> {
  return ReadVarUseOpt(tokenizer, ctx, TokenType::Table);
}

auto ReadElementSegment(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<ElementSegment> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Elem));

  if (ctx.features.bulk_memory_enabled()) {
    // LPAR ELEM * bind_var_opt elem_list RPAR
    // LPAR ELEM * bind_var_opt table_use offset elem_list RPAR
    // LPAR ELEM * bind_var_opt DECLARE elem_list RPAR
    // LPAR ELEM * bind_var_opt offset elem_list RPAR  /* Sugar */
    // LPAR ELEM * bind_var_opt offset elem_var_list RPAR  /* Sugar */
    auto name = ReadBindVarOpt(tokenizer, ctx);
    auto table_use_opt = ReadTableUseOpt(tokenizer, ctx);

    SegmentType segment_type;
    OptAt<ConstantExpression> offset_opt;
    if (table_use_opt) {
      // LPAR ELEM bind_var_opt table_use * offset elem_list RPAR
      WASP_TRY_READ(offset, ReadOffsetExpression(tokenizer, ctx));
      offset_opt = offset;
      segment_type = SegmentType::Active;
    } else {
      auto token = tokenizer.Peek();
      if (token.type == TokenType::Declare) {
        // LPAR ELEM bind_var_opt * DECLARE elem_list RPAR
        tokenizer.Read();
        segment_type = SegmentType::Declared;
      } else if (token.type == TokenType::Lpar &&
                 tokenizer.Peek(1).type != TokenType::Ref) {
        segment_type = SegmentType::Active;
        // LPAR ELEM bind_var_opt * offset elem_list RPAR
        // LPAR ELEM bind_var_opt * offset elem_var_list RPAR
        WASP_TRY_READ(offset, ReadOffsetExpression(tokenizer, ctx));
        offset_opt = offset;

        token = tokenizer.Peek();
        if (token.type == TokenType::Nat || token.type == TokenType::Id ||
            token.type == TokenType::Rpar) {
          // LPAR ELEM bind_var_opt offset * elem_var_list RPAR
          WASP_TRY_READ(init, ReadVarList(tokenizer, ctx));
          WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
          return At{guard.loc(),
                    ElementSegment{
                        name, nullopt, *offset_opt,
                        ElementListWithVars{ExternalKind::Function, init}}};
        }

        // LPAR ELEM bind_var_opt offset * elem_list RPAR
      } else {
        // LPAR ELEM bind_var_opt * elem_list RPAR
        segment_type = SegmentType::Passive;
      }
    }
    // ... * elem_list RPAR
    auto token = tokenizer.Peek();
    if (token.type == TokenType::Func) {
      tokenizer.Read();
      // * elem_kind elem_var_list
      auto kind = At{token.loc, ExternalKind::Function};
      WASP_TRY_READ(init, ReadVarList(tokenizer, ctx));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));

      if (segment_type == SegmentType::Active) {
        assert(offset_opt.has_value());
        return At{guard.loc(), ElementSegment{name, table_use_opt, *offset_opt,
                                              ElementListWithVars{kind, init}}};
      } else {
        return At{guard.loc(), ElementSegment{name, segment_type,
                                              ElementListWithVars{kind, init}}};
      }
    } else {
      // * ref_type elem_expr_list
      WASP_TRY_READ(elemtype, ReadReferenceType(tokenizer, ctx));
      WASP_TRY_READ(init, ReadElementExpressionList(tokenizer, ctx));
      WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));

      if (segment_type == SegmentType::Active) {
        assert(offset_opt.has_value());
        return At{guard.loc(),
                  ElementSegment{name, table_use_opt, *offset_opt,
                                 ElementListWithExpressions{elemtype, init}}};
      } else {
        return At{guard.loc(),
                  ElementSegment{name, segment_type,
                                 ElementListWithExpressions{elemtype, init}}};
      }
    }
  } else {
    // LPAR ELEM * var offset var_list RPAR
    // LPAR ELEM * offset var_list RPAR  /* Sugar */
    auto table = ReadVarOpt(tokenizer, ctx);
    WASP_TRY_READ(offset, ReadOffsetExpression(tokenizer, ctx));
    WASP_TRY_READ(init, ReadVarList(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), ElementSegment{nullopt, table, offset,
                                          ElementListWithVars{
                                              ExternalKind::Function, init}}};
  }
}

// Section 10: Code

auto ReadNameEqNatOpt(Tokenizer& tokenizer,
                      ReadCtx& ctx,
                      TokenType token_type,
                      u32 offset) -> OptAt<u32> {
  auto token_opt = tokenizer.Match(token_type);
  if (!token_opt) {
    return nullopt;
  }

  auto nat_opt = StrToNat<u32>(token_opt->literal_info(),
                               token_opt->span_u8().subspan(offset));
  if (!nat_opt) {
    ctx.errors.OnError(token_opt->loc,
                       concat("Invalid natural number, got ", token_opt->type));
    return nullopt;
  }

  return At{token_opt->loc, *nat_opt};
}

auto ReadAlignOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<u32> {
  auto nat_opt = ReadNameEqNatOpt(tokenizer, ctx, TokenType::AlignEqNat, 6);
  if (!nat_opt) {
    return nullopt;
  }

  auto value = nat_opt->value();
  if (value == 0 || (value & (value - 1)) != 0) {
    ctx.errors.OnError(nat_opt->loc(),
                       concat("Alignment must be a power of two, got ", value));
    return nullopt;
  }
  return nat_opt;
}

auto ReadOffsetOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<u32> {
  return ReadNameEqNatOpt(tokenizer, ctx, TokenType::OffsetEqNat, 7);
}

auto ReadMemArgImmediate(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<MemArgImmediate> {
  LocationGuard guard{tokenizer};
  OptAt<Var> memory_opt;
  if (ctx.features.multi_memory_enabled()) {
    memory_opt = ReadVarOpt(tokenizer, ctx);
  }
  auto offset_opt = ReadOffsetOpt(tokenizer, ctx);
  auto align_opt = ReadAlignOpt(tokenizer, ctx);
  return At{guard.loc(), MemArgImmediate{align_opt, offset_opt, memory_opt}};
}

auto ReadSimdLane(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<u8> {
  return ReadNat<u8>(tokenizer, ctx);
}

auto ReadSimdShuffleImmediate(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<ShuffleImmediate> {
  LocationGuard guard{tokenizer};
  ShuffleImmediate result;
  for (size_t lane = 0; lane < result.size(); ++lane) {
    WASP_TRY_READ(value, ReadSimdLane(tokenizer, ctx));
    result[lane] = value;
  }
  return At{guard.loc(), result};
}

template <typename T, size_t N>
auto ReadSimdValues(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<v128> {
  LocationGuard guard{tokenizer};
  std::array<T, N> result;
  for (size_t lane = 0; lane < N; ++lane) {
    if constexpr (std::is_floating_point_v<T>) {
      WASP_TRY_READ(float_, ReadFloat<T>(tokenizer, ctx));
      result[lane] = float_;
    } else {
      WASP_TRY_READ(int_, ReadInt<T>(tokenizer, ctx));
      result[lane] = int_;
    }
  }
  return At{guard.loc(), v128{result}};
}

auto ReadHeapType2Immediate(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<HeapType2Immediate> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(ht1, ReadHeapType(tokenizer, ctx));
  WASP_TRY_READ(ht2, ReadHeapType(tokenizer, ctx));
  return At{guard.loc(), HeapType2Immediate{ht1, ht2}};
}

bool IsPlainInstruction(Token token) {
  switch (token.type) {
    case TokenType::BareInstr:
    case TokenType::BrOnCastInstr:
    case TokenType::BrTableInstr:
    case TokenType::CallIndirectInstr:
    case TokenType::F32ConstInstr:
    case TokenType::F64ConstInstr:
    case TokenType::FuncBindInstr:
    case TokenType::HeapTypeInstr:
    case TokenType::HeapType2Instr:
    case TokenType::I32ConstInstr:
    case TokenType::I64ConstInstr:
    case TokenType::MemoryInstr:
    case TokenType::MemoryCopyInstr:
    case TokenType::MemoryInitInstr:
    case TokenType::MemoryOptInstr:
    case TokenType::RefFuncInstr:
    case TokenType::RefNullInstr:
    case TokenType::RttSubInstr:
    case TokenType::SelectInstr:
    case TokenType::SimdConstInstr:
    case TokenType::SimdLaneInstr:
    case TokenType::SimdMemoryLaneInstr:
    case TokenType::SimdShuffleInstr:
    case TokenType::StructFieldInstr:
    case TokenType::TableCopyInstr:
    case TokenType::TableInitInstr:
    case TokenType::VarInstr:
      return true;

    default:
      return false;
  }
}

bool IsBlockInstruction(Token token) {
  return token.type == TokenType::BlockInstr;
}

bool IsLetInstruction(Token token) {
  return token.type == TokenType::LetInstr;
}

bool IsExpression(Tokenizer& tokenizer) {
  return tokenizer.Peek().type == TokenType::Lpar &&
         (IsPlainInstruction(tokenizer.Peek(1)) ||
          IsBlockInstruction(tokenizer.Peek(1)) ||
          IsLetInstruction(tokenizer.Peek(1)));
}

bool IsInstruction(Tokenizer& tokenizer) {
  auto token = tokenizer.Peek();
  return IsPlainInstruction(token) || IsBlockInstruction(token) ||
         IsLetInstruction(token) || IsExpression(tokenizer);
}

bool IsElementExpression(Tokenizer& tokenizer) {
  return IsExpression(tokenizer) || (tokenizer.Peek().type == TokenType::Lpar &&
                                     tokenizer.Peek(1).type == TokenType::Item);
}

bool CheckOpcodeEnabled(Token token, ReadCtx& ctx) {
  assert(token.has_opcode());
  if (!ctx.features.HasFeatures(Features{token.opcode_features()})) {
    ctx.errors.OnError(token.loc,
                       concat(token.opcode(), " instruction not allowed"));
    return false;
  }
  return true;
}

auto ReadPlainInstruction(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<Instruction> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::BareInstr:
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      return At{token.loc, Instruction{token.opcode()}};

    case TokenType::HeapTypeInstr:
    case TokenType::RefNullInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(type, ReadHeapType(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), type}};
    }

    case TokenType::BrOnCastInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
      // TODO: Determine whether this instruction should have heap type
      // immediates.
#if 0
      WASP_TRY_READ(types, ReadHeapType2Immediate(tokenizer, ctx));
      auto immediate = At{immediate_guard.loc(), BrOnCastImmediate{var, types}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
#else
      return At{guard.loc(), Instruction{token.opcode(), var}};
#endif
    }

    case TokenType::BrTableInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      WASP_TRY_READ(var_list, ReadNonEmptyVarList(tokenizer, ctx));
      auto default_target = var_list.back();
      var_list.pop_back();
      auto immediate =
          At{immediate_guard.loc(), BrTableImmediate{var_list, default_target}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::CallIndirectInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      OptAt<Var> table_var_opt;
      if (ctx.features.reference_types_enabled()) {
        table_var_opt = ReadVarOpt(tokenizer, ctx);
      }
      WASP_TRY_READ(type, ReadFunctionTypeUse(tokenizer, ctx));
      auto immediate =
          At{immediate_guard.loc(), CallIndirectImmediate{table_var_opt, type}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::F32ConstInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadFloat<f32>(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::F64ConstInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadFloat<f64>(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::FuncBindInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      WASP_TRY_READ(type, ReadFunctionTypeUse(tokenizer, ctx));
      auto immediate = At{immediate_guard.loc(), FuncBindImmediate{type}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::HeapType2Instr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadHeapType2Immediate(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::I32ConstInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadInt<s32>(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::I64ConstInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadInt<s64>(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::MemoryInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadMemArgImmediate(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::MemoryCopyInstr: {
      CheckOpcodeEnabled(token, ctx);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      At<CopyImmediate> immediate;
      if (ctx.features.multi_memory_enabled()) {
        auto dst_var = ReadVarOpt(tokenizer, ctx);
        auto src_var = ReadVarOpt(tokenizer, ctx);
        immediate = At{immediate_guard.loc(), CopyImmediate{dst_var, src_var}};
      } else {
        immediate = At{immediate_guard.loc(), CopyImmediate{}};
      }
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::MemoryInitInstr: {
      CheckOpcodeEnabled(token, ctx);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      WASP_TRY_READ(segment_var, ReadVar(tokenizer, ctx));
      auto memory_var_opt = ReadVarOpt(tokenizer, ctx);
      At<InitImmediate> immediate;
      if (memory_var_opt) {
        // memory.init $memory $elem ; so vars need to be swapped.
        immediate = At{immediate_guard.loc(),
                       InitImmediate{*memory_var_opt, segment_var}};
      } else {
        // memory.init $elem
        immediate =
            At{immediate_guard.loc(), InitImmediate{segment_var, nullopt}};
      }
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::MemoryOptInstr: {
      CheckOpcodeEnabled(token, ctx);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      At<MemOptImmediate> immediate;
      if (ctx.features.multi_memory_enabled()) {
        auto memory_var_opt = ReadVarOpt(tokenizer, ctx);
        immediate = At{immediate_guard.loc(), MemOptImmediate{memory_var_opt}};
      } else {
        immediate = At{immediate_guard.loc(), MemOptImmediate{nullopt}};
      }
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::RttSubInstr: {
      CheckOpcodeEnabled(token, ctx);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      // TODO: Determine whether this instruction should have heap type
      // immediates.
#if 0
      WASP_TRY_READ(depth, ReadNat32(tokenizer, ctx));
      WASP_TRY_READ(types, ReadHeapType2Immediate(tokenizer, ctx));
      auto immediate = At{immediate_guard.loc(), RttSubImmediate{depth, types}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
#else
      WASP_TRY_READ(type, ReadHeapType(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), type}};
#endif
    }

    case TokenType::SelectInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      At<Opcode> opcode = token.opcode();
      if (ctx.features.reference_types_enabled()) {
        LocationGuard immediate_guard{tokenizer};
        WASP_TRY_READ(value_type_list, ReadResultList(tokenizer, ctx));

        if (!value_type_list.empty()) {
          // Typed select has a different opcode.
          return At{guard.loc(),
                    Instruction{At{opcode.loc(), Opcode::SelectT},
                                At{immediate_guard.loc(), value_type_list}}};
        }
      }
      return At{guard.loc(), Instruction{opcode}};
    }

    case TokenType::SimdConstInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadSimdConst(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::SimdLaneInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadSimdLane(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::SimdMemoryLaneInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      auto peek0 = tokenizer.Peek();
      auto peek1 = tokenizer.Peek(1);
      At<MemArgImmediate> memarg;
      if ((peek0.type == TokenType::Nat &&
           (peek1.type == TokenType::Nat ||           // <mem:nat> <lane:nat>
            peek1.type == TokenType::OffsetEqNat ||   // <mem:nat> offset=<nat>
            peek1.type == TokenType::AlignEqNat)) ||  // <mem:nat> align=<nat>
          peek0.type == TokenType::Id ||              // <mem:id>
          peek0.type == TokenType::OffsetEqNat ||     // offset=<nat>
          peek0.type == TokenType::AlignEqNat) {      // align=<nat>
        WASP_TRY_READ(memarg_, ReadMemArgImmediate(tokenizer, ctx));
        memarg = memarg_;
      }
      WASP_TRY_READ(lane, ReadSimdLane(tokenizer, ctx));
      auto immediate =
          At{immediate_guard.loc(), SimdMemoryLaneImmediate{memarg, lane}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::SimdShuffleInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(immediate, ReadSimdShuffleImmediate(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::StructFieldInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      WASP_TRY_READ(struct_var, ReadVar(tokenizer, ctx));
      WASP_TRY_READ(field_var, ReadVar(tokenizer, ctx));
      auto immediate = At{immediate_guard.loc(),
                          StructFieldImmediate{struct_var, field_var}};
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::TableCopyInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      At<CopyImmediate> immediate;
      if (ctx.features.reference_types_enabled()) {
        auto dst_var = ReadVarOpt(tokenizer, ctx);
        auto src_var = ReadVarOpt(tokenizer, ctx);
        immediate = At{immediate_guard.loc(), CopyImmediate{dst_var, src_var}};
      } else {
        immediate = At{immediate_guard.loc(), CopyImmediate{}};
      }
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::TableInitInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      WASP_TRY_READ(segment_var, ReadVar(tokenizer, ctx));
      auto table_var_opt = ReadVarOpt(tokenizer, ctx);
      At<InitImmediate> immediate;
      if (table_var_opt) {
        // table.init $table $elem ; so vars need to be swapped.
        immediate = At{immediate_guard.loc(),
                       InitImmediate{*table_var_opt, segment_var}};
      } else {
        // table.init $elem
        immediate =
            At{immediate_guard.loc(), InitImmediate{segment_var, nullopt}};
      }
      return At{guard.loc(), Instruction{token.opcode(), immediate}};
    }

    case TokenType::VarInstr:
    case TokenType::RefFuncInstr: {
      WASP_TRY(CheckOpcodeEnabled(token, ctx));
      tokenizer.Read();
      WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
      return At{guard.loc(), Instruction{token.opcode(), var}};
    }

    default:
      ctx.errors.OnError(
          token.loc, concat("Expected plain instruction, got ", token.type));
      return nullopt;
  }
}

auto ReadLabelOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<BindVar> {
  auto token_opt = tokenizer.Match(TokenType::Id);
  if (!token_opt) {
    return nullopt;
  }

  return At{token_opt->loc, BindVar{token_opt->as_string_view()}};
}

bool ReadEndLabelOpt(Tokenizer& tokenizer, ReadCtx& ctx, OptAt<BindVar> label) {
  auto end_label = ReadBindVarOpt(tokenizer, ctx);
  if (end_label) {
    if (!label) {
      ctx.errors.OnError(end_label->loc(),
                         concat("Unexpected label ", end_label));
      return false;
    } else if (*label != *end_label) {
      ctx.errors.OnError(end_label->loc(), concat("Expected label ", *label,
                                                  ", got ", *end_label));
      return false;
    }
  }
  return true;
}

auto ReadBlockImmediate(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<BlockImmediate> {
  LocationGuard guard{tokenizer};
  auto label = ReadLabelOpt(tokenizer, ctx);

  // Don't use ReadFunctionTypeUse, since that always marks the type signature
  // as being used, even if it is an inline signature.
  auto type_use = ReadTypeUseOpt(tokenizer, ctx);
  WASP_TRY_READ(type, ReadFunctionType(tokenizer, ctx));
  auto ftu = FunctionTypeUse{type_use, type};
  return At{guard.loc(), BlockImmediate{label, ftu}};
}

auto ReadLetImmediate(Tokenizer& tokenizer, ReadCtx& ctx)
    -> OptAt<LetImmediate> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(block, ReadBlockImmediate(tokenizer, ctx));
  WASP_TRY_READ(locals, ReadLocalList(tokenizer, ctx));
  return At{guard.loc(), LetImmediate{block, locals}};
}

bool ReadOpcodeOpt(Tokenizer& tokenizer,
                   ReadCtx& ctx,
                   InstructionList& instructions,
                   TokenType token_type) {
  auto token_opt = tokenizer.Match(token_type);
  if (!token_opt) {
    return false;
  }
  instructions.push_back(At{token_opt->loc, Instruction{token_opt->opcode()}});
  return true;
}

bool ExpectOpcode(Tokenizer& tokenizer,
                  ReadCtx& ctx,
                  InstructionList& instructions,
                  TokenType token_type) {
  auto token = tokenizer.Peek();
  if (!ReadOpcodeOpt(tokenizer, ctx, instructions, token_type)) {
    ctx.errors.OnError(token.loc,
                       concat("Expected ", token_type, ", got ", token.type));
    return false;
  }
  return true;
}

bool ReadBlockInstruction(Tokenizer& tokenizer,
                          ReadCtx& ctx,
                          InstructionList& instructions) {
  LocationGuard guard{tokenizer};
  auto token_opt = tokenizer.Match(TokenType::BlockInstr);
  // Shouldn't be called when the TokenType is not a BlockInstr.
  assert(token_opt.has_value());

  WASP_TRY_READ(block, ReadBlockImmediate(tokenizer, ctx));
  instructions.push_back(
      At{guard.loc(), Instruction{token_opt->opcode(), block}});
  WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));

  bool allow_end = true;

  switch (token_opt->opcode()) {
    case Opcode::If:
      if (ReadOpcodeOpt(tokenizer, ctx, instructions, TokenType::Else)) {
        WASP_TRY(ReadEndLabelOpt(tokenizer, ctx, block->label));
        WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
      }
      break;

    case Opcode::Try: {
      if (!ctx.features.exceptions_enabled()) {
        ctx.errors.OnError(token_opt->loc, "try instruction not allowed");
        return false;
      }

      auto token = tokenizer.Peek();
      switch (token.type) {
        case TokenType::Catch:
          // Read zero or more catch blocks
          do {
            LocationGuard guard{tokenizer};
            auto token = tokenizer.Read();
            WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
            instructions.push_back(
                At{guard.loc(), Instruction{token.opcode(), var}});
            WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
          } while (tokenizer.Peek().type == TokenType::Catch);

          // Allow optional trailing catch_all
          if (ReadOpcodeOpt(tokenizer, ctx, instructions,
                            TokenType::CatchAll)) {
            WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
          }
          break;

        case TokenType::CatchAll:
          WASP_TRY(
              ExpectOpcode(tokenizer, ctx, instructions, TokenType::CatchAll));
          WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
          break;

        case TokenType::Delegate: {
          LocationGuard guard{tokenizer};
          auto token = tokenizer.Read();
          WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
          instructions.push_back(
              At{guard.loc(), Instruction{token.opcode(), var}});
          allow_end = false;
          break;
        }

        default:
          ctx.errors.OnError(
              token.loc,
              concat("Expected 'catch', 'catch_all' or 'delegate', got ",
                     token.type));
          break;
      }
      break;
    }

    case Opcode::Block:
    case Opcode::Loop:
      break;

    default:
      WASP_UNREACHABLE();
  }

  if (allow_end) {
    WASP_TRY(ExpectOpcode(tokenizer, ctx, instructions, TokenType::End));
    WASP_TRY(ReadEndLabelOpt(tokenizer, ctx, block->label));
  }
  return true;
}

bool ReadLetInstruction(Tokenizer& tokenizer,
                        ReadCtx& ctx,
                        InstructionList& instructions) {
  LocationGuard guard{tokenizer};
  auto token_opt = tokenizer.Match(TokenType::LetInstr);
  // Shouldn't be called when the TokenType is not a LetInstr.
  assert(token_opt.has_value());

  WASP_TRY_READ(immediate, ReadLetImmediate(tokenizer, ctx));
  instructions.push_back(
      At{guard.loc(), Instruction{token_opt->opcode(), immediate}});
  WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
  WASP_TRY(ExpectOpcode(tokenizer, ctx, instructions, TokenType::End));
  WASP_TRY(ReadEndLabelOpt(tokenizer, ctx, immediate->block.label));
  return true;
}

bool ReadInstruction(Tokenizer& tokenizer,
                     ReadCtx& ctx,
                     InstructionList& instructions) {
  auto token = tokenizer.Peek();
  if (IsPlainInstruction(token)) {
    WASP_TRY_READ(instruction, ReadPlainInstruction(tokenizer, ctx));
    instructions.push_back(instruction);
  } else if (IsBlockInstruction(token)) {
    WASP_TRY(ReadBlockInstruction(tokenizer, ctx, instructions));
  } else if (IsLetInstruction(token)) {
    WASP_TRY(ReadLetInstruction(tokenizer, ctx, instructions));
  } else if (IsExpression(tokenizer)) {
    WASP_TRY(ReadExpression(tokenizer, ctx, instructions));
  } else {
    ctx.errors.OnError(token.loc,
                       concat("Expected instruction, got ", token.type));
    return false;
  }
  return true;
}

bool ReadInstructionList(Tokenizer& tokenizer,
                         ReadCtx& ctx,
                         InstructionList& instructions) {
  while (IsInstruction(tokenizer)) {
    WASP_TRY(ReadInstruction(tokenizer, ctx, instructions));
  }
  return true;
}

bool ReadRparAsEndInstruction(Tokenizer& tokenizer,
                              ReadCtx& ctx,
                              InstructionList& instructions) {
  // Read final `)` and use its location as the `end` instruction.
  auto rpar = tokenizer.Peek();
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  instructions.push_back(At{rpar.loc, Instruction{At{rpar.loc, Opcode::End}}});
  return true;
}

bool ReadExpression(Tokenizer& tokenizer,
                    ReadCtx& ctx,
                    InstructionList& instructions) {
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));

  auto token = tokenizer.Peek();

  if (IsPlainInstruction(token)) {
    WASP_TRY_READ(plain, ReadPlainInstruction(tokenizer, ctx));
    // Reorder the instructions, so `(A (B) (C))` becomes `(B) (C) (A)`.
    WASP_TRY(ReadExpressionList(tokenizer, ctx, instructions));
    instructions.push_back(plain);
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  } else if (IsBlockInstruction(token)) {
    LocationGuard guard{tokenizer};
    tokenizer.Read();
    WASP_TRY_READ(block, ReadBlockImmediate(tokenizer, ctx));
    auto block_instr = At{guard.loc(), Instruction{token.opcode(), block}};

    switch (token.opcode()) {
      case Opcode::Block:
      case Opcode::Loop:
        instructions.push_back(block_instr);
        WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
        WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
        break;

      case Opcode::If:
        // Read condition, if any. It doesn't need to exist, since the folded
        // `if` syntax is extremely flexible.
        WASP_TRY(ReadExpressionList(tokenizer, ctx, instructions));

        // The `if` instruction must come after the condition.
        instructions.push_back(block_instr);

        // Read then block.
        WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Then));
        WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
        WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));

        // Read else block, if any.
        if (tokenizer.Match(TokenType::Lpar)) {
          WASP_TRY(ExpectOpcode(tokenizer, ctx, instructions, TokenType::Else));
          WASP_TRY(ReadEndLabelOpt(tokenizer, ctx, block->label));
          WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
          WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
        }
        WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
        break;

      case Opcode::Try: {
        if (!ctx.features.exceptions_enabled()) {
          ctx.errors.OnError(token.loc, "try instruction not allowed");
          return false;
        }

        instructions.push_back(block_instr);

        if (tokenizer.Peek(0).type == TokenType::Lpar &&
            tokenizer.Peek(1).type == TokenType::Do) {
          WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Do));
          WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
          WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
        }

        if (tokenizer.Peek().type == TokenType::Lpar) {
          auto token = tokenizer.Peek(1);
          switch (token.type) {
            case TokenType::Catch:
              do {
                WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));
                LocationGuard guard{tokenizer};
                auto token = tokenizer.Read();
                WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
                instructions.push_back(
                    At{guard.loc(), Instruction{token.opcode(), var}});
                WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
                WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
              } while (tokenizer.Peek().type == TokenType::Lpar &&
                       tokenizer.Peek(1).type == TokenType::Catch);

              // Allow optional trailing catch_all
              if (tokenizer.Peek().type == TokenType::Lpar &&
                  tokenizer.Peek(1).type == TokenType::CatchAll) {
                WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));
                WASP_TRY(ExpectOpcode(tokenizer, ctx, instructions,
                                      TokenType::CatchAll));
                WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
                WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
              }
              WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
              break;

            case TokenType::CatchAll:
              WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));
              WASP_TRY(ExpectOpcode(tokenizer, ctx, instructions,
                                    TokenType::CatchAll));
              WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
              WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
              WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
              break;

            case TokenType::Delegate: {
              // Read 'delegate <label>' instead of 'end'
              WASP_TRY(Expect(tokenizer, ctx, TokenType::Lpar));
              LocationGuard guard{tokenizer};
              auto token = tokenizer.Read();
              WASP_TRY_READ(var, ReadVar(tokenizer, ctx));
              instructions.push_back(
                  At{guard.loc(), Instruction{token.opcode(), var}});
              WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));  // delegate
              WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));  // try
              break;
            }

            default:
              ctx.errors.OnError(
                  token.loc,
                  concat("Expected 'catch', 'catch_all', or 'delegate', got ",
                         token.type));
              break;
          }
        } else {
          // No '(' found, it should be closing the 'try' block.
          WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
        }
        break;
      }

      default:
        WASP_UNREACHABLE();
    }
  } else if (IsLetInstruction(token)) {
    LocationGuard guard{tokenizer};
    tokenizer.Read();
    if (!ctx.features.function_references_enabled()) {
      ctx.errors.OnError(token.loc, "let instruction not allowed");
      return false;
    }

    WASP_TRY_READ(immediate, ReadLetImmediate(tokenizer, ctx));
    instructions.push_back(
        At{guard.loc(), Instruction{token.opcode(), immediate}});
    WASP_TRY(ReadInstructionList(tokenizer, ctx, instructions));
    WASP_TRY(ReadRparAsEndInstruction(tokenizer, ctx, instructions));
  } else {
    ctx.errors.OnError(token.loc,
                           concat("Expected expression, got ", token.type));
    return false;
  }
  return true;
}

bool ReadExpressionList(Tokenizer& tokenizer,
                        ReadCtx& ctx,
                        InstructionList& instructions) {
  while (IsExpression(tokenizer)) {
    WASP_TRY(ReadExpression(tokenizer, ctx, instructions));
  }
  return true;
}

// Section 11: Data

auto ReadMemoryUseOpt(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Var> {
  return ReadVarUseOpt(tokenizer, ctx, TokenType::Memory);
}

auto ReadDataSegment(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<DataSegment> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Data));

  if (ctx.features.bulk_memory_enabled()) {
    // LPAR DATA * bind_var_opt string_list RPAR
    // LPAR DATA * bind_var_opt memory_use offset string_list RPAR
    // LPAR DATA * bind_var_opt offset string_list RPAR  /* Sugar */
    auto name = ReadBindVarOpt(tokenizer, ctx);
    auto memory_use_opt = ReadMemoryUseOpt(tokenizer, ctx);

    SegmentType segment_type;
    OptAt<ConstantExpression> offset_opt;
    if (memory_use_opt || tokenizer.Peek().type == TokenType::Lpar) {
      // LPAR DATA bind_var_opt memory_use * offset string_list RPAR
      // LPAR DATA bind_var_opt * offset string_list RPAR  /* Sugar */
      WASP_TRY_READ(offset, ReadOffsetExpression(tokenizer, ctx));
      offset_opt = offset;
      segment_type = SegmentType::Active;
    } else {
      // LPAR DATA bind_var_opt * string_list RPAR
      segment_type = SegmentType::Passive;
    }
    // ... * string_list RPAR
    WASP_TRY_READ(data, ReadDataItemList(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));

    if (segment_type == SegmentType::Active) {
      assert(offset_opt.has_value());
      return At{guard.loc(),
                DataSegment{name, memory_use_opt, *offset_opt, data}};
    } else {
      return At{guard.loc(), DataSegment{name, data}};
    }
  } else {
    // LPAR DATA var offset string_list RPAR
    // LPAR DATA offset string_list RPAR  /* Sugar */
    auto memory = ReadVarOpt(tokenizer, ctx);
    WASP_TRY_READ(offset, ReadOffsetExpression(tokenizer, ctx));
    WASP_TRY_READ(data, ReadDataItemList(tokenizer, ctx));
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
    return At{guard.loc(), DataSegment{nullopt, memory, offset, data}};
  }
}

// Section 13: Tag

auto ReadTagType(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<TagType> {
  LocationGuard guard{tokenizer};
  auto attribute = TagAttribute::Exception;
  WASP_TRY_READ(type, ReadFunctionTypeUse(tokenizer, ctx));
  return At{guard.loc(), TagType{attribute, type}};
}

auto ReadTag(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<Tag> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  WASP_TRY(ExpectLpar(tokenizer, ctx, TokenType::Tag));

  if (!ctx.features.exceptions_enabled()) {
    ctx.errors.OnError(token.loc, "Tags not allowed");
    return nullopt;
  }

  auto name = ReadBindVarOpt(tokenizer, ctx);
  WASP_TRY_READ(exports, ReadInlineExportList(tokenizer, ctx));
  auto import_opt = ReadInlineImportOpt(tokenizer, ctx);
  ctx.seen_non_import |= !import_opt;

  WASP_TRY_READ(type, ReadTagType(tokenizer, ctx));
  WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  return At{guard.loc(), Tag{TagDesc{name, type}, import_opt, exports}};
}

// Module

bool IsModuleItem(Tokenizer& tokenizer) {
  if (tokenizer.Peek().type != TokenType::Lpar) {
    return false;
  }

  auto token = tokenizer.Peek(1);
  return token.type == TokenType::Type || token.type == TokenType::Import ||
         token.type == TokenType::Func || token.type == TokenType::Table ||
         token.type == TokenType::Memory || token.type == TokenType::Global ||
         token.type == TokenType::Export || token.type == TokenType::Start ||
         token.type == TokenType::Elem || token.type == TokenType::Data ||
         token.type == TokenType::Tag;
}

auto ReadModuleItem(Tokenizer& tokenizer, ReadCtx& ctx) -> OptAt<ModuleItem> {
  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    ctx.errors.OnError(token.loc, concat("Expected '(', got ", token.type));
    return nullopt;
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Type: {
      WASP_TRY_READ(item, ReadDefinedType(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Import: {
      WASP_TRY_READ(item, ReadImport(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Func: {
      WASP_TRY_READ(item, ReadFunction(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Table: {
      WASP_TRY_READ(item, ReadTable(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Memory: {
      WASP_TRY_READ(item, ReadMemory(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Global: {
      WASP_TRY_READ(item, ReadGlobal(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Export: {
      WASP_TRY_READ(item, ReadExport(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Start: {
      WASP_TRY_READ(item, ReadStart(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Elem: {
      WASP_TRY_READ(item, ReadElementSegment(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Data: {
      WASP_TRY_READ(item, ReadDataSegment(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    case TokenType::Tag: {
      WASP_TRY_READ(item, ReadTag(tokenizer, ctx));
      return At{item.loc(), ModuleItem{item}};
    }

    default:
      ctx.errors.OnError(
          token.loc,
          concat(
              "Expected 'type', 'import', 'func', 'table', 'memory', 'global', "
              "'export', 'start', 'elem', 'data', or 'tag', got ",
              token.type));
      return nullopt;
  }
}

auto ReadModule(Tokenizer& tokenizer, ReadCtx& ctx) -> optional<Module> {
  ctx.BeginModule();
  Module module;
  while (IsModuleItem(tokenizer)) {
    WASP_TRY_READ(item, ReadModuleItem(tokenizer, ctx));
    module.push_back(item);
  }
  return module;
}

auto ReadSingleModule(Tokenizer& tokenizer, ReadCtx& ctx) -> optional<Module> {
  // Check whether it's wrapped in (module... )
  bool in_module = false;
  if (tokenizer.MatchLpar(TokenType::Module).has_value()) {
    in_module = true;
    // Read optional module name, but discard it.
    ReadModuleVarOpt(tokenizer, ctx);
  }

  auto module = ReadModule(tokenizer, ctx);

  if (in_module) {
    WASP_TRY(Expect(tokenizer, ctx, TokenType::Rpar));
  }
  return module;
}

// Explicit instantiations.
template auto ReadInt<s8>(Tokenizer&, ReadCtx&) -> OptAt<s8>;
template auto ReadInt<u8>(Tokenizer&, ReadCtx&) -> OptAt<u8>;
template auto ReadInt<s16>(Tokenizer&, ReadCtx&) -> OptAt<s16>;
template auto ReadInt<u16>(Tokenizer&, ReadCtx&) -> OptAt<u16>;
template auto ReadInt<s32>(Tokenizer&, ReadCtx&) -> OptAt<s32>;
template auto ReadInt<u32>(Tokenizer&, ReadCtx&) -> OptAt<u32>;
template auto ReadInt<s64>(Tokenizer&, ReadCtx&) -> OptAt<s64>;
template auto ReadInt<u64>(Tokenizer&, ReadCtx&) -> OptAt<u64>;
template auto ReadFloat<f32>(Tokenizer&, ReadCtx&) -> OptAt<f32>;
template auto ReadFloat<f64>(Tokenizer&, ReadCtx&) -> OptAt<f64>;
template auto ReadSimdValues<s8, 16>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<u8, 16>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<s16, 8>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<u16, 8>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<s32, 4>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<u32, 4>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<s64, 2>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<u64, 2>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<f32, 4>(Tokenizer&, ReadCtx&) -> OptAt<v128>;
template auto ReadSimdValues<f64, 2>(Tokenizer&, ReadCtx&) -> OptAt<v128>;

}  // namespace wasp::text
