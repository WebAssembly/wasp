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
#include <iterator>
#include <numeric>
#include <type_traits>

#include "wasp/base/errors.h"
#include "wasp/base/format.h"
#include "wasp/base/utf8.h"
#include "wasp/text/context.h"
#include "wasp/text/formatters.h"
#include "wasp/text/numeric.h"

namespace wasp {
namespace text {

namespace {

auto Expect(Tokenizer& tokenizer, Context& context, TokenType expected)
    -> optional<Token> {
  auto actual_opt = tokenizer.Match(expected);
  if (!actual_opt) {
    auto token = tokenizer.Peek();
    context.errors.OnError(token.loc,
                           format("Expected {}, got {}", expected, token.type));
    return nullopt;
  }
  return actual_opt;
}

auto ExpectLpar(Tokenizer& tokenizer, Context& context, TokenType expected)
    -> optional<Token> {
  auto actual_opt = tokenizer.MatchLpar(expected);
  if (!actual_opt) {
    auto token = tokenizer.Peek();
    context.errors.OnError(
        token.loc, format("Expected '(' {}, got {} {}", expected, token.type,
                          tokenizer.Peek(1).type));
    return nullopt;
  }
  return actual_opt;
}

}  // namespace

auto ReadNat32(Tokenizer& tokenizer, Context& context) -> At<u32> {
  auto token_opt = tokenizer.Match(TokenType::Nat);
  if (!token_opt) {
    auto token = tokenizer.Peek();
    context.errors.OnError(
        token.loc, format("Expected a natural number, got {}", token.type));
    return MakeAt(token_opt->loc, u32{0});
  }
  auto nat_opt = StrToNat<u32>(token_opt->literal_info(), token_opt->span_u8());
  if (!nat_opt) {
    context.errors.OnError(
        token_opt->loc, format("Invalid natural number, got {}", *token_opt));
    return MakeAt(token_opt->loc, u32{0});
  }
  return MakeAt(token_opt->loc, *nat_opt);
}

template <typename T>
auto ReadInt(Tokenizer& tokenizer, Context& context) -> At<T> {
  auto token = tokenizer.Peek();
  if (!(token.type == TokenType::Nat || token.type == TokenType::Int)) {
    context.errors.OnError(token.loc,
                           format("Expected an integer, got {}", token.type));
    return MakeAt(token.loc, T{0});
  }

  tokenizer.Read();
  auto int_opt = StrToInt<T>(token.literal_info(), token.span_u8());
  if (!int_opt) {
    context.errors.OnError(token.loc,
                           format("Invalid integer, got {}", token));
    return MakeAt(token.loc, T{0});
  }
  return MakeAt(token.loc, *int_opt);
}

template <typename T>
auto ReadFloat(Tokenizer& tokenizer, Context& context) -> At<T> {
  auto token = tokenizer.Peek();
  if (!(token.type == TokenType::Nat || token.type == TokenType::Int ||
        token.type == TokenType::Float)) {
    context.errors.OnError(token.loc,
                           format("Expected a float, got {}", token.type));
    return MakeAt(token.loc, T{0});
  }

  tokenizer.Read();
  auto float_opt = StrToFloat<T>(token.literal_info(), token.span_u8());
  if (!float_opt) {
    context.errors.OnError(token.loc,
                           format("Invalid float, got {}", token));
    return MakeAt(token.loc, T{0});
  }
  return MakeAt(token.loc, *float_opt);
}

auto ReadVar(Tokenizer& tokenizer, Context& context) -> At<Var> {
  auto token = tokenizer.Peek();
  auto var_opt = ReadVarOpt(tokenizer, context);
  if (!var_opt) {
    context.errors.OnError(token.loc,
                           format("Expected a variable, got {}", token.type));
    return MakeAt(token.loc, Var{u32{0}});
  }
  return *var_opt;
}

auto ReadVarOpt(Tokenizer& tokenizer, Context& context) -> OptAt<Var> {
  auto token = tokenizer.Peek();
  if (token.type == TokenType::Id) {
    tokenizer.Read();
    return MakeAt(token.loc, Var{token.as_string_view()});
  } else if (token.type == TokenType::Nat) {
    auto nat = ReadNat32(tokenizer, context);
    return MakeAt(nat.loc(), Var{nat.value()});
  } else {
    return nullopt;
  }
}

auto ReadVarList(Tokenizer& tokenizer, Context& context) -> VarList {
  VarList result;
  OptAt<Var> var_opt;
  while ((var_opt = ReadVarOpt(tokenizer, context))) {
    result.push_back(*var_opt);
  }
  return result;
}

auto ReadVarUseOpt(Tokenizer& tokenizer, Context& context, TokenType token_type)
    -> OptAt<Var> {
  LocationGuard guard{tokenizer};
  if (!tokenizer.MatchLpar(token_type)) {
    return nullopt;
  }
  auto var = ReadVar(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), var.value());
}

auto ReadTypeUseOpt(Tokenizer& tokenizer, Context& context) -> OptAt<Var> {
  return ReadVarUseOpt(tokenizer, context, TokenType::Type);
}

auto ReadFunctionTypeUse(Tokenizer& tokenizer, Context& context)
    -> FunctionTypeUse {
  auto type_use = ReadTypeUseOpt(tokenizer, context);
  auto type = ReadFunctionType(tokenizer, context);
  auto result = FunctionTypeUse{type_use, type};

  context.function_type_map.Use(result);
  return result;
}

auto ReadText(Tokenizer& tokenizer, Context& context) -> At<Text> {
  auto token_opt = tokenizer.Match(TokenType::Text);
  if (!token_opt) {
    auto token = tokenizer.Peek();
    context.errors.OnError(token.loc,
                           format("Expected quoted text, got {}", token.type));
    return MakeAt(token_opt->loc, Text{});
  }
  return MakeAt(token_opt->loc, token_opt->text());
}

auto ReadUtf8Text(Tokenizer& tokenizer, Context& context) -> At<Text> {
  auto text = ReadText(tokenizer, context);
  if (!IsValidUtf8(text->text)) {
    context.errors.OnError(text.loc(), "Invalid UTF-8 encoding");
  }
  return text;
}

auto ReadTextList(Tokenizer& tokenizer, Context& context) -> TextList {
  TextList result;
  while (tokenizer.Peek().type == TokenType::Text) {
    result.push_back(ReadText(tokenizer, context));
  }
  return result;
}

// Section 1: Type

auto ReadBindVarOpt(Tokenizer& tokenizer, Context& context, NameMap& name_map)
    -> OptAt<BindVar> {
  auto token_opt = tokenizer.Match(TokenType::Id);
  if (!token_opt) {
    name_map.NewUnbound();
    return nullopt;
  }

  auto name = token_opt->as_string_view();
  if (name_map.Has(name)) {
    context.errors.OnError(
        token_opt->loc, format("Variable {} is already bound to index {}", name,
                               name_map.Get(name)));

    // Use the previous name and treat this object as unbound.
    name_map.NewUnbound();
    return nullopt;
  }

  name_map.NewBound(name);
  return MakeAt(token_opt->loc, BindVar{name});
}

auto ReadBoundValueTypeList(Tokenizer& tokenizer,
                            Context& context,
                            NameMap& name_map,
                            TokenType token_type)
    -> BoundValueTypeList {
  BoundValueTypeList result;
  while (tokenizer.MatchLpar(token_type)) {
    if (tokenizer.Peek().type == TokenType::Id) {
      LocationGuard guard{tokenizer};
      auto bind_var_opt = ReadBindVarOpt(tokenizer, context, name_map);
      auto value_type = ReadValueType(tokenizer, context);
      result.push_back(
          MakeAt(guard.loc(), BoundValueType{bind_var_opt, value_type}));
    } else {
      for (auto& value_type : ReadValueTypeList(tokenizer, context)) {
        result.push_back(
            MakeAt(value_type.loc(), BoundValueType{nullopt, value_type}));
      }
    }
    Expect(tokenizer, context, TokenType::Rpar);
  }
  return result;
}

auto ReadBoundParamList(Tokenizer& tokenizer,
                        Context& context,
                        NameMap& name_map) -> BoundValueTypeList {
  return ReadBoundValueTypeList(tokenizer, context, name_map, TokenType::Param);
}

auto ReadUnboundValueTypeList(Tokenizer& tokenizer,
                              Context& context,
                              TokenType token_type) -> ValueTypeList {
  ValueTypeList result;
  while (tokenizer.MatchLpar(token_type)) {
    auto value_types = ReadValueTypeList(tokenizer, context);
    std::copy(value_types.begin(), value_types.end(),
              std::back_inserter(result));
    Expect(tokenizer, context, TokenType::Rpar);
  }
  return result;
}

auto ReadParamList(Tokenizer& tokenizer, Context& context) -> ValueTypeList {
  return ReadUnboundValueTypeList(tokenizer, context, TokenType::Param);
}

auto ReadResultList(Tokenizer& tokenizer, Context& context) -> ValueTypeList {
  return ReadUnboundValueTypeList(tokenizer, context, TokenType::Result);
}

auto ReadValueType(Tokenizer& tokenizer, Context& context) -> At<ValueType> {
  auto token_opt = tokenizer.Match(TokenType::ValueType);
  if (!token_opt) {
    auto token = tokenizer.Peek();
    context.errors.OnError(token.loc,
                           format("Expected value type, got {}", token.type));
    return MakeAt(token_opt->loc, ValueType::I32);
  }
  bool allowed = true;
  switch (token_opt->value_type()) {
#define WASP_V(val, Name, str)
#define WASP_FEATURE_V(val, Name, str, feature)  \
  case ValueType::Name:                          \
    if (!context.features.feature##_enabled()) { \
      allowed = false;                           \
    }                                            \
    break;
#include "wasp/base/def/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  if (!allowed) {
    context.errors.OnError(token_opt->loc, format("value type {} not allowed",
                                                  token_opt->value_type()));
  }
  return token_opt->value_type();
}

auto ReadValueTypeList(Tokenizer& tokenizer, Context& context)
    -> ValueTypeList {
  ValueTypeList result;
  while (tokenizer.Peek().type == TokenType::ValueType) {
    result.push_back(ReadValueType(tokenizer, context));
  }
  return result;
}

auto ReadBoundFunctionType(Tokenizer& tokenizer,
                           Context& context,
                           NameMap& name_map) -> At<BoundFunctionType> {
  LocationGuard guard{tokenizer};
  auto params = ReadBoundParamList(tokenizer, context, name_map);
  auto results = ReadResultList(tokenizer, context);
  return MakeAt(guard.loc(), BoundFunctionType{params, results});
}

auto ReadTypeEntry(Tokenizer& tokenizer, Context& context) -> At<TypeEntry> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Type);
  auto bind_var = ReadBindVarOpt(tokenizer, context, context.type_names);
  ExpectLpar(tokenizer, context, TokenType::Func);

  NameMap dummy_name_map;  // Bound names are not used.
  auto type = ReadBoundFunctionType(tokenizer, context, dummy_name_map);
  context.function_type_map.Define(type);

  Expect(tokenizer, context, TokenType::Rpar);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), TypeEntry{bind_var, type});
}

// Section 2: Import

auto ReadInlineImportOpt(Tokenizer& tokenizer, Context& context)
    -> OptAt<InlineImport> {
  LocationGuard guard{tokenizer};
  auto import_token = tokenizer.MatchLpar(TokenType::Import);
  if (!import_token) {
    return nullopt;
  }

  if (context.seen_non_import) {
    context.errors.OnError(
        import_token->loc,
        "Imports must occur before all non-import definitions");
  }
  auto module = ReadUtf8Text(tokenizer, context);
  auto name = ReadUtf8Text(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), InlineImport{module, name});
}

auto ReadImport(Tokenizer& tokenizer, Context& context) -> At<Import> {
  LocationGuard guard{tokenizer};
  auto import_token = ExpectLpar(tokenizer, context, TokenType::Import);

  if (context.seen_non_import) {
    context.errors.OnError(
        import_token->loc,
        "Imports must occur before all non-import definitions");
  }

  Import result;
  result.module = ReadUtf8Text(tokenizer, context);
  result.name = ReadUtf8Text(tokenizer, context);

  Expect(tokenizer, context, TokenType::Lpar);

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::Func: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, context, context.function_names);
      auto type_use = ReadTypeUseOpt(tokenizer, context);
      NameMap dummy_name_map;  // Bound names are not used.
      auto type = ReadBoundFunctionType(tokenizer, context, dummy_name_map);
      context.function_type_map.Use(type_use, type);
      result.desc = FunctionDesc{name, type_use, type};
      break;
    }

    case TokenType::Table: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, context, context.table_names);
      auto type = ReadTableType(tokenizer, context);
      result.desc = TableDesc{name, type};
      break;
    }

    case TokenType::Memory: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, context, context.memory_names);
      auto type = ReadMemoryType(tokenizer, context);
      result.desc = MemoryDesc{name, type};
      break;
    }

    case TokenType::Global: {
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, context, context.global_names);
      auto type = ReadGlobalType(tokenizer, context);
      result.desc = GlobalDesc{name, type};
      break;
    }

    case TokenType::Event: {
      if (!context.features.exceptions_enabled()) {
        context.errors.OnError(token.loc, "Events not allowed");
      }
      tokenizer.Read();
      auto name = ReadBindVarOpt(tokenizer, context, context.event_names);
      auto type = ReadEventType(tokenizer, context);
      result.desc = EventDesc{name, type};
      break;
    }

    default:
      context.errors.OnError(
          token.loc, format("Expected an import external kind, got {}", token));
      break;
  }

  Expect(tokenizer, context, TokenType::Rpar);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), result);
}

// Section 3: Function

auto ReadLocalList(Tokenizer& tokenizer, Context& context, NameMap& name_map)
    -> BoundValueTypeList {
  return ReadBoundValueTypeList(tokenizer, context, name_map, TokenType::Local);
}

auto ReadFunctionType(Tokenizer& tokenizer, Context& context)
    -> At<FunctionType> {
  LocationGuard guard{tokenizer};
  auto params = ReadParamList(tokenizer, context);
  auto results = ReadResultList(tokenizer, context);
  return MakeAt(guard.loc(), FunctionType{params, results});
}

auto ReadFunction(Tokenizer& tokenizer, Context& context) -> At<Function> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Func);

  BoundValueTypeList locals;
  InstructionList instructions;

  auto name = ReadBindVarOpt(tokenizer, context, context.function_names);
  auto exports = ReadInlineExportList(tokenizer, context);
  auto import_opt = ReadInlineImportOpt(tokenizer, context);
  context.seen_non_import |= !import_opt;

  context.local_names.Reset();

  auto type_use = ReadTypeUseOpt(tokenizer, context);
  auto type = ReadBoundFunctionType(tokenizer, context, context.local_names);
  context.function_type_map.Use(type_use, type);
  if (!import_opt) {
    locals = ReadLocalList(tokenizer, context, context.local_names);
    ReadInstructionList(tokenizer, context, instructions);
  }

  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(),
                Function{FunctionDesc{name, type_use, type}, locals,
                         instructions, import_opt, exports});
}

// Section 4: Table

auto ReadLimits(Tokenizer& tokenizer, Context& context) -> At<Limits> {
  LocationGuard guard{tokenizer};
  auto min = ReadNat32(tokenizer, context);
  auto token = tokenizer.Peek();
  OptAt<u32> max_opt;
  if (token.type == TokenType::Nat) {
    max_opt = ReadNat32(tokenizer, context);
  }

  token = tokenizer.Peek();
  At<Shared> shared = Shared::No;
  if (token.type == TokenType::Shared) {
    tokenizer.Read();
    shared = MakeAt(token.loc, Shared::Yes);
  }

  return MakeAt(guard.loc(), Limits{min, max_opt, shared});
}

auto ReadElementType(Tokenizer& tokenizer, Context& context)
    -> At<ElementType> {
  auto token = tokenizer.Peek();
  auto element_type = ReadElementTypeOpt(tokenizer, context);
  if (!element_type) {
    context.errors.OnError(token.loc,
                           format("Expected element type, got {}", token.type));
    return MakeAt(token.loc, ElementType::Funcref);
  }

  return *element_type;
}

auto ReadElementTypeOpt(Tokenizer& tokenizer, Context& context)
    -> OptAt<ElementType> {
  auto token_opt = tokenizer.Match(TokenType::ValueType);
  if (!token_opt) {
    return nullopt;
  }

  bool allowed = true;
  ElementType element_type;

  switch (token_opt->value_type()) {
#define WASP_V(val, Name, str)        \
  case ValueType::Name:               \
    element_type = ElementType::Name; \
    break;
#define WASP_FEATURE_V(val, Name, str, feature)  \
  case ValueType::Name:                          \
    element_type = ElementType::Name;            \
    if (!context.features.feature##_enabled()) { \
      allowed = false;                           \
    }                                            \
    break;
#include "wasp/base/def/element_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      context.errors.OnError(token_opt->loc, format("{} is not an element type",
                                                    token_opt->value_type()));
      return nullopt;
  }
  if (!allowed) {
    context.errors.OnError(token_opt->loc,
                           format("element type {} not allowed", element_type));
    // Print error, but use it anyway.
  }
  return MakeAt(token_opt->loc, element_type);
}

auto ReadTableType(Tokenizer& tokenizer, Context& context) -> At<TableType> {
  LocationGuard guard{tokenizer};
  auto limits = ReadLimits(tokenizer, context);
  auto element = ReadElementType(tokenizer, context);
  return MakeAt(guard.loc(), TableType{limits, element});
}

auto ReadTable(Tokenizer& tokenizer, Context& context) -> At<Table> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Table);

  auto name = ReadBindVarOpt(tokenizer, context, context.table_names);
  auto exports = ReadInlineExportList(tokenizer, context);
  auto import_opt = ReadInlineImportOpt(tokenizer, context);
  context.seen_non_import |= !import_opt;

  auto elemtype_opt = ReadElementTypeOpt(tokenizer, context);
  if (!import_opt && elemtype_opt) {
    // Inline element segment.
    ExpectLpar(tokenizer, context, TokenType::Elem);

    ElementList elements;
    u32 size;
    if (context.features.bulk_memory_enabled() && IsExpression(tokenizer)) {
      // Element expression list.
      auto expressions = ReadElementExpressionList(tokenizer, context);
      size = expressions.size();
      elements =
          ElementList{ElementListWithExpressions{*elemtype_opt, expressions}};
    } else {
      // Element var list.
      auto vars = ReadVarList(tokenizer, context);
      size = vars.size();
      elements = ElementList{ElementListWithVars{ExternalKind::Function, vars}};
    }

    // Implicit table type.
    auto type = TableType{Limits{size, size}, *elemtype_opt};

    Expect(tokenizer, context, TokenType::Rpar);
    Expect(tokenizer, context, TokenType::Rpar);
    return MakeAt(guard.loc(),
                  Table{TableDesc{name, type}, import_opt, exports, elements});
  } else {
    auto type = ReadTableType(tokenizer, context);
    Expect(tokenizer, context, TokenType::Rpar);
    return MakeAt(guard.loc(),
                  Table{TableDesc{name, type}, import_opt, exports, nullopt});
  }
}

// Section 5: Memory

auto ReadMemoryType(Tokenizer& tokenizer, Context& context) -> At<MemoryType> {
  auto limits = ReadLimits(tokenizer, context);
  return MakeAt(limits.loc(), MemoryType{limits});
}

auto ReadMemory(Tokenizer& tokenizer, Context& context) -> At<Memory> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Memory);

  auto name = ReadBindVarOpt(tokenizer, context, context.memory_names);
  auto exports = ReadInlineExportList(tokenizer, context);
  auto import_opt = ReadInlineImportOpt(tokenizer, context);
  context.seen_non_import |= !import_opt;

  if (tokenizer.MatchLpar(TokenType::Data)) {
    // Inline data segment.
    auto data = ReadTextList(tokenizer, context);
    auto size = std::accumulate(
        data.begin(), data.end(), u32{0},
        [](u32 total, Text text) { return total + text.byte_size; });

    // Implicit memory type.
    auto type = MemoryType{Limits{size, size}};

    Expect(tokenizer, context, TokenType::Rpar);
    Expect(tokenizer, context, TokenType::Rpar);
    return MakeAt(guard.loc(),
                  Memory{MemoryDesc{name, type}, import_opt, exports, data});
  } else {
    auto type = ReadMemoryType(tokenizer, context);
    Expect(tokenizer, context, TokenType::Rpar);
    return MakeAt(guard.loc(),
                  Memory{MemoryDesc{name, type}, import_opt, exports, nullopt});
  }
}

// Section 6: Global

auto ReadGlobalType(Tokenizer& tokenizer, Context& context) -> At<GlobalType> {
  LocationGuard guard{tokenizer};
  At<ValueType> valtype;
  At<Mutability> mut;
  auto token_opt = tokenizer.MatchLpar(TokenType::Mut);
  if (token_opt) {
    mut = MakeAt(token_opt->loc, Mutability::Var);
    valtype = ReadValueType(tokenizer, context);
    Expect(tokenizer, context, TokenType::Rpar);
  } else {
    valtype = ReadValueType(tokenizer, context);
    mut = Mutability::Const;
  }
  return MakeAt(guard.loc(), GlobalType{valtype, mut});
}

auto ReadGlobal(Tokenizer& tokenizer, Context& context) -> At<Global> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Global);

  InstructionList init;

  auto name = ReadBindVarOpt(tokenizer, context, context.global_names);
  auto exports = ReadInlineExportList(tokenizer, context);
  auto import_opt = ReadInlineImportOpt(tokenizer, context);
  context.seen_non_import |= !import_opt;

  auto type = ReadGlobalType(tokenizer, context);
  if (!import_opt) {
    ReadInstructionList(tokenizer, context, init);
  }

  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(),
                Global{GlobalDesc{name, type}, init, import_opt, exports});
}

// Section 7: Export

auto ReadInlineExportOpt(Tokenizer& tokenizer, Context& context)
    -> OptAt<InlineExport> {
  LocationGuard guard{tokenizer};
  if (!tokenizer.MatchLpar(TokenType::Export)) {
    return nullopt;
  }
  auto name = ReadUtf8Text(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), InlineExport{name});
}

auto ReadInlineExportList(Tokenizer& tokenizer, Context& context)
    -> InlineExportList {
  InlineExportList result;
  OptAt<InlineExport> export_opt;
  while ((export_opt = ReadInlineExportOpt(tokenizer, context))) {
    result.push_back(*export_opt);
  }
  return result;
}

auto ReadExport(Tokenizer& tokenizer, Context& context) -> At<Export> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Export);

  auto name = ReadUtf8Text(tokenizer, context);

  At<ExternalKind> kind;

  Expect(tokenizer, context, TokenType::Lpar);
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::Func:
      kind = MakeAt(token.loc, ExternalKind::Function);
      break;

    case TokenType::Table:
      kind = MakeAt(token.loc, ExternalKind::Table);
      break;

    case TokenType::Memory:
      kind = MakeAt(token.loc, ExternalKind::Memory);
      break;

    case TokenType::Global:
      kind = MakeAt(token.loc, ExternalKind::Global);
      break;

    case TokenType::Event:
      if (!context.features.exceptions_enabled()) {
        context.errors.OnError(token.loc, "Events not allowed");
      }
      kind = MakeAt(token.loc, ExternalKind::Event);
      break;

    default:
      context.errors.OnError(
          token.loc,
          format("Expected an import external kind, got {}", token.type));
      break;
  }

  tokenizer.Read();
  auto var = ReadVar(tokenizer, context);

  Expect(tokenizer, context, TokenType::Rpar);
  Expect(tokenizer, context, TokenType::Rpar);

  return MakeAt(guard.loc(), Export{kind, name, var});
}

// Section 8: Start

auto ReadStart(Tokenizer& tokenizer, Context& context) -> At<Start> {
  LocationGuard guard{tokenizer};
  auto start_token = ExpectLpar(tokenizer, context, TokenType::Start);

  if (context.seen_start) {
    context.errors.OnError(start_token->loc, "Multiple start functions");
  }

  auto var = ReadVar(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), Start{var});
}

// Section 9: Elem

auto ReadOffsetExpression(Tokenizer& tokenizer, Context& context)
    -> InstructionList {
  LocationGuard guard{tokenizer};
  InstructionList instructions;
  if (tokenizer.MatchLpar(TokenType::Offset)) {
    ReadInstructionList(tokenizer, context, instructions);
    Expect(tokenizer, context, TokenType::Rpar);
  } else if (IsExpression(tokenizer)) {
    ReadExpression(tokenizer, context, instructions);
  } else {
    auto token = tokenizer.Peek();
    context.errors.OnError(
        token.loc, format("Expected offset expression, got {}", token.type));
  }
  return instructions;
}

auto ReadElementExpression(Tokenizer& tokenizer, Context& context)
    -> ElementExpression {
  LocationGuard guard{tokenizer};
  ElementExpression expression;
  if (tokenizer.MatchLpar(TokenType::Item)) {
    ReadInstructionList(tokenizer, context, expression);
    Expect(tokenizer, context, TokenType::Rpar);
  } else if (IsExpression(tokenizer)) {
    ReadExpression(tokenizer, context, expression);
  } else {
    auto token = tokenizer.Peek();
    context.errors.OnError(
        token.loc, format("Expected element expression, got {}", token.type));
  }
  return expression;
}

auto ReadElementExpressionList(Tokenizer& tokenizer, Context& context)
    -> ElementExpressionList {
  ElementExpressionList result;
  while (IsElementExpression(tokenizer)) {
    result.push_back(ReadElementExpression(tokenizer, context));
  }
  return result;
}

auto ReadTableUseOpt(Tokenizer& tokenizer, Context& context) -> OptAt<Var> {
  return ReadVarUseOpt(tokenizer, context, TokenType::Table);
}

auto ReadElementSegment(Tokenizer& tokenizer, Context& context)
    -> At<ElementSegment> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Elem);

  if (context.features.bulk_memory_enabled()) {
    // LPAR ELEM * bind_var_opt elem_list RPAR
    // LPAR ELEM * bind_var_opt table_use offset elem_list RPAR
    // LPAR ELEM * bind_var_opt DECLARE elem_list RPAR
    // LPAR ELEM * bind_var_opt offset elem_list RPAR  /* Sugar */
    // LPAR ELEM * bind_var_opt offset elem_var_list RPAR  /* Sugar */
    auto name =
        ReadBindVarOpt(tokenizer, context, context.element_segment_names);
    auto table_use_opt = ReadTableUseOpt(tokenizer, context);

    SegmentType segment_type;
    optional<InstructionList> offset_opt;
    if (table_use_opt) {
      // LPAR ELEM bind_var_opt table_use * offset elem_list RPAR
      offset_opt = ReadOffsetExpression(tokenizer, context);
      segment_type = SegmentType::Active;
    } else {
      auto token = tokenizer.Peek();
      if (token.type == TokenType::Declare) {
        // LPAR ELEM bind_var_opt * DECLARE elem_list RPAR
        tokenizer.Read();
        segment_type = SegmentType::Declared;
      } else if (token.type == TokenType::Lpar) {
        segment_type = SegmentType::Active;
        // LPAR ELEM bind_var_opt * offset elem_list RPAR
        // LPAR ELEM bind_var_opt * offset elem_var_list RPAR
        offset_opt = ReadOffsetExpression(tokenizer, context);

        token = tokenizer.Peek();
        if (token.type == TokenType::Nat || token.type == TokenType::Id) {
          // LPAR ELEM bind_var_opt offset * elem_var_list RPAR
          auto init = ReadVarList(tokenizer, context);
          Expect(tokenizer, context, TokenType::Rpar);
          return MakeAt(guard.loc(),
                        ElementSegment{
                            name, nullopt, *offset_opt,
                            ElementListWithVars{ExternalKind::Function, init}});
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
      auto kind = MakeAt(token.loc, ExternalKind::Function);
      auto init = ReadVarList(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);

      if (segment_type == SegmentType::Active) {
        assert(offset_opt.has_value());
        return MakeAt(guard.loc(),
                      ElementSegment{name, table_use_opt, *offset_opt,
                                     ElementListWithVars{kind, init}});
      } else {
        return MakeAt(guard.loc(),
                      ElementSegment{name, segment_type,
                                     ElementListWithVars{kind, init}});
      }
    } else {
      // * ref_type elem_expr_list
      auto elemtype = ReadElementType(tokenizer, context);
      auto init = ReadElementExpressionList(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);

      if (segment_type == SegmentType::Active) {
        assert(offset_opt.has_value());
        return MakeAt(
            guard.loc(),
            ElementSegment{name, table_use_opt, *offset_opt,
                           ElementListWithExpressions{elemtype, init}});
      } else {
        return MakeAt(guard.loc(), ElementSegment{name, segment_type,
                                                  ElementListWithExpressions{
                                                      elemtype, init}});
      }
    }
  } else {
    // LPAR ELEM * var offset var_list RPAR
    // LPAR ELEM * offset var_list RPAR  /* Sugar */
    auto table = ReadVarOpt(tokenizer, context);
    auto offset = ReadOffsetExpression(tokenizer, context);
    auto init = ReadVarList(tokenizer, context);
    Expect(tokenizer, context, TokenType::Rpar);
    return MakeAt(
        guard.loc(),
        ElementSegment{nullopt, table, offset,
                       ElementListWithVars{ExternalKind::Function, init}});
  }
}

// Section 10: Code

auto ReadNameEqNatOpt(Tokenizer& tokenizer,
                      Context& context,
                      TokenType token_type,
                      u32 offset) -> OptAt<u32> {
  auto token_opt = tokenizer.Match(token_type);
  if (!token_opt) {
    return nullopt;
  }

  auto nat_opt = StrToNat<u32>(token_opt->literal_info(),
                               token_opt->span_u8().subspan(offset));
  if (!nat_opt) {
    context.errors.OnError(
        token_opt->loc,
        format("Invalid natural number, got {}", token_opt->type));
    return nullopt;
  }

  return MakeAt(token_opt->loc, *nat_opt);
}

auto ReadAlignOpt(Tokenizer& tokenizer, Context& context) -> OptAt<u32> {
  auto nat_opt = ReadNameEqNatOpt(tokenizer, context, TokenType::AlignEqNat, 6);
  if (!nat_opt) {
    return nullopt;
  }

  auto value = nat_opt->value();
  if (value == 0 || (value & (value - 1)) != 0) {
    context.errors.OnError(
        nat_opt->loc(),
        format("Alignment must be a power of two, got {}", value));
    return nullopt;
  }
  return nat_opt;
}

auto ReadOffsetOpt(Tokenizer& tokenizer, Context& context) -> OptAt<u32> {
  return ReadNameEqNatOpt(tokenizer, context, TokenType::OffsetEqNat, 7);
}

auto ReadSimdLane(Tokenizer& tokenizer, Context& context) -> At<u32> {
  auto token = tokenizer.Peek();
  // TODO: This should probably be ReadNat32, but the simd tests currently
  // allow signed values here.
  auto lane = ReadInt<u32>(tokenizer, context);
  if (lane > 255) {
    context.errors.OnError(token.loc,
                           format("Invalid lane number, got {}", token.type));
  }
  return lane;
}

template <typename T, size_t N>
auto ReadSimdValues(Tokenizer& tokenizer, Context& context) -> At<v128> {
  LocationGuard guard{tokenizer};
  std::array<T, N> result;
  for (size_t lane = 0; lane < N; ++lane) {
    if constexpr (std::is_floating_point_v<T>) {
      result[lane] = ReadFloat<T>(tokenizer, context).value();
    } else {
      result[lane] = ReadInt<T>(tokenizer, context).value();
    }
  }
  return MakeAt(guard.loc(), v128{result});
}


bool IsPlainInstruction(Token token) {
  switch (token.type) {
    case TokenType::BareInstr:
    case TokenType::BrOnExnInstr:
    case TokenType::BrTableInstr:
    case TokenType::CallIndirectInstr:
    case TokenType::F32ConstInstr:
    case TokenType::F64ConstInstr:
    case TokenType::I32ConstInstr:
    case TokenType::I64ConstInstr:
    case TokenType::MemoryInstr:
    case TokenType::RefFuncInstr:
    case TokenType::RefNullInstr:
    case TokenType::SelectInstr:
    case TokenType::SimdConstInstr:
    case TokenType::SimdLaneInstr:
    case TokenType::SimdShuffleInstr:
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

bool IsExpression(Tokenizer& tokenizer) {
  return tokenizer.Peek().type == TokenType::Lpar &&
         (IsPlainInstruction(tokenizer.Peek(1)) ||
          IsBlockInstruction(tokenizer.Peek(1)));
}

bool IsInstruction(Tokenizer& tokenizer) {
  auto token = tokenizer.Peek();
  return IsPlainInstruction(token) || IsBlockInstruction(token) ||
         IsExpression(tokenizer);
}

bool IsElementExpression(Tokenizer& tokenizer) {
  return IsExpression(tokenizer) || (tokenizer.Peek().type == TokenType::Lpar &&
                                     tokenizer.Peek(1).type == TokenType::Item);
}

void CheckOpcodeEnabled(Token token, Context& context) {
  assert(token.has_opcode());
  if (!context.features.HasFeatures(Features{token.opcode_features()})) {
    context.errors.OnError(
        token.loc, format("{} instruction not allowed", token.opcode()));
  }
}

auto ReadPlainInstruction(Tokenizer& tokenizer, Context& context)
    -> At<Instruction> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::BareInstr:
    case TokenType::RefNullInstr:
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      return MakeAt(token.loc, Instruction{token.opcode()});

    case TokenType::BrOnExnInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      auto label_var = ReadVar(tokenizer, context);
      auto exn_var = ReadVar(tokenizer, context);
      auto immediate =
          MakeAt(immediate_guard.loc(), BrOnExnImmediate{label_var, exn_var});
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::BrTableInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      auto var_list = ReadVarList(tokenizer, context);
      auto default_target = var_list.back();
      var_list.pop_back();
      auto immediate = MakeAt(immediate_guard.loc(),
                              BrTableImmediate{var_list, default_target});
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::CallIndirectInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      OptAt<Var> table_var_opt;
      if (context.features.reference_types_enabled()) {
        table_var_opt = ReadVarOpt(tokenizer, context);
      }
      auto type = ReadFunctionTypeUse(tokenizer, context);
      auto immediate = MakeAt(immediate_guard.loc(),
                              CallIndirectImmediate{table_var_opt, type});
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::F32ConstInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto immediate = ReadFloat<f32>(tokenizer, context);
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::F64ConstInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto immediate = ReadFloat<f64>(tokenizer, context);
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::I32ConstInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto immediate = ReadInt<u32>(tokenizer, context);
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::I64ConstInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto immediate = ReadInt<u64>(tokenizer, context);
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::MemoryInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      auto offset_opt = ReadOffsetOpt(tokenizer, context);
      auto align_opt = ReadAlignOpt(tokenizer, context);
      auto immediate =
          MakeAt(immediate_guard.loc(), MemArgImmediate{align_opt, offset_opt});
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::SelectInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      At<ValueTypeList> immediate;
      if (context.features.reference_types_enabled()) {
        LocationGuard immediate_guard{tokenizer};
        auto value_type_list = ReadResultList(tokenizer, context);
        immediate = MakeAt(immediate_guard.loc(), value_type_list);
      }
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::SimdConstInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto simd_token = tokenizer.Peek();

      At<v128> immediate;
      switch (simd_token.type) {
        case TokenType::I8X16:
          tokenizer.Read();
          immediate = ReadSimdValues<u8, 16>(tokenizer, context);
          break;

        case TokenType::I16X8:
          tokenizer.Read();
          immediate = ReadSimdValues<u16, 8>(tokenizer, context);
          break;

        case TokenType::I32X4:
          tokenizer.Read();
          immediate = ReadSimdValues<u32, 4>(tokenizer, context);
          break;

        case TokenType::I64X2:
          tokenizer.Read();
          immediate = ReadSimdValues<u64, 2>(tokenizer, context);
          break;

        case TokenType::F32X4:
          tokenizer.Read();
          immediate = ReadSimdValues<f32, 4>(tokenizer, context);
          break;

        case TokenType::F64X2:
          tokenizer.Read();
          immediate = ReadSimdValues<f64, 2>(tokenizer, context);
          break;

        default:
          context.errors.OnError(
              token.loc,
              format("Invalid SIMD constant token, got {}", simd_token.type));
          return MakeAt(guard.loc(), Instruction{Opcode::Nop});
      }

      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::SimdLaneInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto immediate = ReadSimdLane(tokenizer, context);
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::TableCopyInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      At<CopyImmediate> immediate;
      if (context.features.reference_types_enabled()) {
        auto dst_var = ReadVarOpt(tokenizer, context);
        auto src_var = ReadVarOpt(tokenizer, context);
        immediate =
            MakeAt(immediate_guard.loc(), CopyImmediate{dst_var, src_var});
      } else {
        immediate = MakeAt(immediate_guard.loc(), CopyImmediate{});
      }
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::TableInitInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      LocationGuard immediate_guard{tokenizer};
      auto segment_var = ReadVar(tokenizer, context);
      auto table_var_opt = ReadVarOpt(tokenizer, context);
      At<InitImmediate> immediate;
      if (table_var_opt) {
        // table.init $table $elem ; so vars need to be swapped.
        immediate = MakeAt(immediate_guard.loc(),
                           InitImmediate{*table_var_opt, segment_var});
      } else {
        // table.init $elem
        immediate =
            MakeAt(immediate_guard.loc(), InitImmediate{segment_var, nullopt});
      }
      return MakeAt(guard.loc(), Instruction{token.opcode(), immediate});
    }

    case TokenType::VarInstr:
    case TokenType::RefFuncInstr: {
      CheckOpcodeEnabled(token, context);
      tokenizer.Read();
      auto var = ReadVar(tokenizer, context);
      return MakeAt(guard.loc(), Instruction{token.opcode(), var});
    }

    default:
      context.errors.OnError(
          token.loc, format("Expected plain instruction, got {}", token.type));
      return MakeAt(token.loc, Instruction{Opcode::Nop});
  }
}

auto ReadLabelOpt(Tokenizer& tokenizer, Context& context) -> OptAt<BindVar> {
  auto bind_var = ReadBindVarOpt(tokenizer, context, context.label_names);
  context.label_name_stack.push_back(bind_var);
  return bind_var;
}

void ReadEndLabelOpt(Tokenizer& tokenizer,
                     Context& context,
                     OptAt<BindVar> label) {
  NameMap dummy_name_map;
  auto end_label = ReadBindVarOpt(tokenizer, context, dummy_name_map);
  if (end_label) {
    if (!label) {
      context.errors.OnError(end_label->loc(),
                             format("Unexpected label {}", end_label));
    } else if (*label != *end_label) {
      context.errors.OnError(
          end_label->loc(),
          format("Expected label {}, got {}", *label, *end_label));
    }
  }
}

auto ReadBlockImmediate(Tokenizer& tokenizer, Context& context)
    -> At<BlockImmediate> {
  LocationGuard guard{tokenizer};
  auto label = ReadLabelOpt(tokenizer, context);
  auto type = ReadFunctionTypeUse(tokenizer, context);
  return MakeAt(guard.loc(), BlockImmediate{label, type});
}

bool ReadOpcodeOpt(Tokenizer& tokenizer,
                   Context& context,
                   InstructionList& instructions,
                   TokenType token_type) {
  auto token_opt = tokenizer.Match(token_type);
  if (!token_opt) {
    return false;
  }
  instructions.push_back(
      MakeAt(token_opt->loc, Instruction{token_opt->opcode()}));
  return true;
}

void ExpectOpcode(Tokenizer& tokenizer,
                  Context& context,
                  InstructionList& instructions,
                  TokenType token_type) {
  auto token = tokenizer.Peek();
  if (!ReadOpcodeOpt(tokenizer, context, instructions, token_type)) {
    context.errors.OnError(
        token.loc, format("Expected {}, got {}", token_type, token.type));
    instructions.push_back(MakeAt(token.loc, Instruction{token.opcode()}));
  }
}

void ReadBlockInstruction(Tokenizer& tokenizer,
                          Context& context,
                          InstructionList& instructions) {
  LocationGuard guard{tokenizer};
  auto token_opt = tokenizer.Match(TokenType::BlockInstr);
  // Shouldn't be called when the TokenType is not a BlockInstr.
  assert(token_opt.has_value());

  auto block = ReadBlockImmediate(tokenizer, context);
  instructions.push_back(
      MakeAt(guard.loc(), Instruction{token_opt->opcode(), block}));
  ReadInstructionList(tokenizer, context, instructions);

  switch (token_opt->opcode()) {
    case Opcode::If:
      if (ReadOpcodeOpt(tokenizer, context, instructions, TokenType::Else)) {
        ReadEndLabelOpt(tokenizer, context, block->label);
        ReadInstructionList(tokenizer, context, instructions);
      }
      break;

    case Opcode::Try:
      if (!context.features.exceptions_enabled()) {
        context.errors.OnError(token_opt->loc, "try instruction not allowed");
      }
      ExpectOpcode(tokenizer, context, instructions, TokenType::Catch);
      ReadEndLabelOpt(tokenizer, context, block->label);
      ReadInstructionList(tokenizer, context, instructions);
      break;

    case Opcode::Block:
    case Opcode::Loop:
      break;

    default:
      WASP_UNREACHABLE();
  }

  ExpectOpcode(tokenizer, context, instructions, TokenType::End);
  ReadEndLabelOpt(tokenizer, context, block->label);
  context.EndBlock();
}

void ReadInstruction(Tokenizer& tokenizer,
                     Context& context,
                     InstructionList& instructions) {
  auto token = tokenizer.Peek();
  if (IsPlainInstruction(token)) {
    instructions.push_back(ReadPlainInstruction(tokenizer, context));
  } else if (IsBlockInstruction(token)) {
    ReadBlockInstruction(tokenizer, context, instructions);
  } else if (IsExpression(tokenizer)) {
    ReadExpression(tokenizer, context, instructions);
  } else {
    context.errors.OnError(token.loc,
                           format("Expected instruction, got {}", token.type));
  }
}

void ReadInstructionList(Tokenizer& tokenizer,
                         Context& context,
                         InstructionList& instructions) {
  while (IsInstruction(tokenizer)) {
    ReadInstruction(tokenizer, context, instructions);
  }
}

void ReadExpression(Tokenizer& tokenizer,
                    Context& context,
                    InstructionList& instructions) {
  Expect(tokenizer, context, TokenType::Lpar);

  auto token = tokenizer.Peek();

  if (IsPlainInstruction(token)) {
    auto plain = ReadPlainInstruction(tokenizer, context);
    // Reorder the instructions, so `(A (B) (C))` becomes `(B) (C) (A)`.
    ReadExpressionList(tokenizer, context, instructions);
    instructions.push_back(plain);
    Expect(tokenizer, context, TokenType::Rpar);
  } else if (IsBlockInstruction(token)) {
    LocationGuard guard{tokenizer};
    tokenizer.Read();
    auto block = ReadBlockImmediate(tokenizer, context);
    auto block_instr = MakeAt(guard.loc(), Instruction{token.opcode(), block});

    switch (token.opcode()) {
      case Opcode::Block:
      case Opcode::Loop:
        instructions.push_back(block_instr);
        ReadInstructionList(tokenizer, context, instructions);
        break;

      case Opcode::If:
        // Read condition, if any. It doesn't need to exist, since the folded
        // `if` syntax is extremely flexible.
        ReadExpressionList(tokenizer, context, instructions);

        // The `if` instruction must come after the condition.
        instructions.push_back(block_instr);

        // Read then block.
        if (tokenizer.MatchLpar(TokenType::Then)) {
          ReadInstructionList(tokenizer, context, instructions);
          Expect(tokenizer, context, TokenType::Rpar);
        }

        // Read else block, if any.
        if (tokenizer.Match(TokenType::Lpar)) {
          ExpectOpcode(tokenizer, context, instructions, TokenType::Else);
          ReadEndLabelOpt(tokenizer, context, block->label);
          ReadInstructionList(tokenizer, context, instructions);
          Expect(tokenizer, context, TokenType::Rpar);
        }
        break;

      case Opcode::Try:
        if (!context.features.exceptions_enabled()) {
          context.errors.OnError(token.loc, "try instruction not allowed");
        }
        instructions.push_back(block_instr);
        ReadInstructionList(tokenizer, context, instructions);

        // Read catch block.
        Expect(tokenizer, context, TokenType::Lpar);
        ExpectOpcode(tokenizer, context, instructions, TokenType::Catch);
        ReadEndLabelOpt(tokenizer, context, block->label);
        ReadInstructionList(tokenizer, context, instructions);
        Expect(tokenizer, context, TokenType::Rpar);
        break;

      default:
        WASP_UNREACHABLE();
    }

    // Read final `)` and use its location as the `end` instruction.
    auto rpar = tokenizer.Peek();
    Expect(tokenizer, context, TokenType::Rpar);
    instructions.push_back(
        MakeAt(rpar.loc, Instruction{MakeAt(rpar.loc, Opcode::End)}));
    context.EndBlock();
  } else {
    context.errors.OnError(token.loc,
                           format("Expected expression, got {}", token.type));
  }
}

void ReadExpressionList(Tokenizer& tokenizer,
                        Context& context,
                        InstructionList& instructions) {
  while (IsExpression(tokenizer)) {
    ReadExpression(tokenizer, context, instructions);
  }
}

// Section 11: Data

auto ReadMemoryUseOpt(Tokenizer& tokenizer, Context& context) -> OptAt<Var> {
  return ReadVarUseOpt(tokenizer, context, TokenType::Memory);
}

auto ReadDataSegment(Tokenizer& tokenizer, Context& context)
    -> At<DataSegment> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Data);

  if (context.features.bulk_memory_enabled()) {
    // LPAR DATA * bind_var_opt string_list RPAR
    // LPAR DATA * bind_var_opt memory_use offset string_list RPAR
    // LPAR DATA * bind_var_opt offset string_list RPAR  /* Sugar */
    auto name = ReadBindVarOpt(tokenizer, context, context.data_segment_names);
    auto memory_use_opt = ReadMemoryUseOpt(tokenizer, context);

    SegmentType segment_type;
    optional<InstructionList> offset_opt;
    if (memory_use_opt || tokenizer.Peek().type == TokenType::Lpar) {
      // LPAR DATA bind_var_opt memory_use * offset string_list RPAR
      // LPAR DATA bind_var_opt * offset string_list RPAR  /* Sugar */
      offset_opt = ReadOffsetExpression(tokenizer, context);
      segment_type = SegmentType::Active;
    } else {
      // LPAR DATA bind_var_opt * string_list RPAR
      segment_type = SegmentType::Passive;
    }
    // ... * string_list RPAR
    auto data = ReadTextList(tokenizer, context);
    Expect(tokenizer, context, TokenType::Rpar);

    if (segment_type == SegmentType::Active) {
      assert(offset_opt.has_value());
      return MakeAt(guard.loc(),
                    DataSegment{name, memory_use_opt, *offset_opt, data});
    } else {
      return MakeAt(guard.loc(), DataSegment{name, data});
    }
  } else {
    // LPAR DATA var offset string_list RPAR
    // LPAR DATA offset string_list RPAR  /* Sugar */
    auto memory = ReadVarOpt(tokenizer, context);
    auto offset = ReadOffsetExpression(tokenizer, context);
    auto data = ReadTextList(tokenizer, context);
    Expect(tokenizer, context, TokenType::Rpar);
    return MakeAt(guard.loc(), DataSegment{nullopt, memory, offset, data});
  }
}

// Section 13: Event

auto ReadEventType(Tokenizer& tokenizer, Context& context) -> At<EventType> {
  LocationGuard guard{tokenizer};
  auto attribute = EventAttribute::Exception;
  auto type = ReadFunctionTypeUse(tokenizer, context);
  return MakeAt(guard.loc(), EventType{attribute, type});
}

auto ReadEvent(Tokenizer& tokenizer, Context& context) -> At<Event> {
  LocationGuard guard{tokenizer};
  auto token = tokenizer.Peek();
  ExpectLpar(tokenizer, context, TokenType::Event);

  if (!context.features.exceptions_enabled()) {
    context.errors.OnError(token.loc, "Events not allowed");
  }

  auto name = ReadBindVarOpt(tokenizer, context, context.event_names);
  auto exports = ReadInlineExportList(tokenizer, context);
  auto import_opt = ReadInlineImportOpt(tokenizer, context);
  context.seen_non_import |= !import_opt;

  auto type = ReadEventType(tokenizer, context);

  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), Event{EventDesc{name, type}, import_opt, exports});
}

// Module

auto ReadModuleItem(Tokenizer& tokenizer, Context& context)
    -> At<ModuleItem> {
  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    context.errors.OnError(token.loc,
                           format("Expected '(', got {}", token.type));
    return MakeAt(token.loc, ModuleItem{});
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Type: {
      auto item = ReadTypeEntry(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Import: {
      auto item = ReadImport(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Func: {
      auto item = ReadFunction(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Table: {
      auto item = ReadTable(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Memory: {
      auto item = ReadMemory(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Global: {
      auto item = ReadGlobal(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Export: {
      auto item = ReadExport(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Start: {
      auto item = ReadStart(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Elem: {
      auto item = ReadElementSegment(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Data: {
      auto item = ReadDataSegment(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    case TokenType::Event: {
      auto item = ReadEvent(tokenizer, context);
      return MakeAt(item.loc(), ModuleItem{*item});
    }

    default:
      context.errors.OnError(
          token.loc,
          format(
              "Expected 'type', 'import', 'func', 'table', 'memory', 'global', "
              "'export', 'start', 'elem', 'data', or 'event', got {}",
              token.type));
      return MakeAt(token.loc, ModuleItem{});
  }
}

auto ReadModule(Tokenizer& tokenizer, Context& context) -> Module {
  context.BeginModule();
  Module module;
  while (tokenizer.Peek().type == TokenType::Lpar) {
    module.push_back(ReadModuleItem(tokenizer, context));
  }
  context.EndModule();
  return module;
}

// Script

auto ReadModuleVarOpt(Tokenizer& tokenizer, Context& context)
    -> OptAt<ModuleVar> {
  auto token_opt = tokenizer.Match(TokenType::Id);
  if (!token_opt) {
    return nullopt;
  }
  return MakeAt(token_opt->loc, ModuleVar{token_opt->as_string_view()});
}

auto ReadScriptModule(Tokenizer& tokenizer, Context& context)
    -> At<ScriptModule> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Module);
  auto name_opt = ReadModuleVarOpt(tokenizer, context);
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::Binary: {
      tokenizer.Read();
      auto text_list = ReadTextList(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(
          guard.loc(),
          ScriptModule{name_opt, ScriptModuleKind::Binary, text_list});
    }

    case TokenType::Quote: {
      tokenizer.Read();
      auto text_list = ReadTextList(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(),
                    ScriptModule{name_opt, ScriptModuleKind::Quote, text_list});
    }

    default: {
      auto module = ReadModule(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(
          guard.loc(),
          ScriptModule{name_opt, ScriptModuleKind::Text, module});
    }
  }
}

auto ReadConst(Tokenizer& tokenizer, Context& context) -> At<Const> {
  // TODO: Share with ReadPlainInstruction above?
  LocationGuard guard{tokenizer};
  Expect(tokenizer, context, TokenType::Lpar);

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::F32ConstInstr: {
      tokenizer.Read();
      auto literal = ReadFloat<f32>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{literal.value()});
    }

    case TokenType::F64ConstInstr: {
      tokenizer.Read();
      auto literal = ReadFloat<f64>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{literal.value()});
    }

    case TokenType::I32ConstInstr: {
      tokenizer.Read();
      auto literal = ReadInt<u32>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{literal.value()});
    }

    case TokenType::I64ConstInstr: {
      tokenizer.Read();
      auto literal = ReadInt<u64>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{literal.value()});
    }

    case TokenType::SimdConstInstr: {
      if (!context.features.simd_enabled()) {
        context.errors.OnError(token.loc, "Simd values not allowed");
      }
      tokenizer.Read();
      auto simd_token = tokenizer.Peek();

      At<v128> literal;
      switch (simd_token.type) {
        case TokenType::I8X16:
          tokenizer.Read();
          literal = ReadSimdValues<u8, 16>(tokenizer, context);
          break;

        case TokenType::I16X8:
          tokenizer.Read();
          literal = ReadSimdValues<u16, 8>(tokenizer, context);
          break;

        case TokenType::I32X4:
          tokenizer.Read();
          literal = ReadSimdValues<u32, 4>(tokenizer, context);
          break;

        case TokenType::I64X2:
          tokenizer.Read();
          literal = ReadSimdValues<u64, 2>(tokenizer, context);
          break;

        case TokenType::F32X4:
          tokenizer.Read();
          literal = ReadSimdValues<f32, 4>(tokenizer, context);
          break;

        case TokenType::F64X2:
          tokenizer.Read();
          literal = ReadSimdValues<f64, 2>(tokenizer, context);
          break;

        default:
          context.errors.OnError(
              simd_token.loc,
              format("Invalid SIMD constant token, got {}", simd_token.type));
          return MakeAt(guard.loc(), Const{});
      }

      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{literal.value()});
    }

    case TokenType::RefNullInstr:
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.null not allowed");
      }
      tokenizer.Read();
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{RefNullConst{}});

    case TokenType::RefHost: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.host not allowed");
      }
      tokenizer.Read();
      auto nat = ReadNat32(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{RefHostConst{nat}});
    }

    default:
      context.errors.OnError(token.loc,
                             format("Invalid constant, got {}", token.type));
      return MakeAt(guard.loc(), Const{});
  }
}

auto ReadConstList(Tokenizer& tokenizer, Context& context)
    -> ConstList {
  ConstList result;
  while (tokenizer.Peek().type == TokenType::Lpar) {
    result.push_back(ReadConst(tokenizer, context));
  }
  return result;
}

auto ReadInvokeAction(Tokenizer& tokenizer, Context& context)
    -> At<InvokeAction> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Invoke);
  auto module_opt = ReadModuleVarOpt(tokenizer, context);
  auto name = ReadUtf8Text(tokenizer, context);
  auto const_list = ReadConstList(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), InvokeAction{module_opt, name, const_list});
}

auto ReadGetAction(Tokenizer& tokenizer, Context& context) -> At<GetAction> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Get);
  auto module_opt = ReadModuleVarOpt(tokenizer, context);
  auto name = ReadUtf8Text(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), GetAction{module_opt, name});
}

auto ReadAction(Tokenizer& tokenizer, Context& context) -> At<Action> {
  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    context.errors.OnError(token.loc,
                           format("Expected '(', got {}", token.type));
    return MakeAt(token.loc, Action{});
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Invoke: {
      auto action = ReadInvokeAction(tokenizer, context);
      return MakeAt(action.loc(), Action{action.value()});
    }

    case TokenType::Get: {
      auto action = ReadGetAction(tokenizer, context);
      return MakeAt(action.loc(), Action{action.value()});
    }

    default:
      context.errors.OnError(token.loc,
                             format("Invalid action type, got {}", token.type));
      return MakeAt(token.loc, Action{});
  }
}

auto ReadModuleAssertion(Tokenizer& tokenizer, Context& context)
    -> At<ModuleAssertion> {
  LocationGuard guard{tokenizer};
  auto module = ReadScriptModule(tokenizer, context);
  auto text = ReadText(tokenizer, context);
  return MakeAt(guard.loc(), ModuleAssertion{module, text});
}

auto ReadActionAssertion(Tokenizer& tokenizer, Context& context)
    -> At<ActionAssertion> {
  LocationGuard guard{tokenizer};
  auto action = ReadAction(tokenizer, context);
  auto text = ReadText(tokenizer, context);
  return MakeAt(guard.loc(), ActionAssertion{action, text});
}

template <typename T>
auto ReadFloatResult(Tokenizer& tokenizer, Context& context)
    -> At<FloatResult<T>> {
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::NanArithmetic:
      tokenizer.Read();
      return MakeAt(token.loc, FloatResult<T>{NanKind::Arithmetic});

    case TokenType::NanCanonical:
      tokenizer.Read();
      return MakeAt(token.loc, FloatResult<T>{NanKind::Canonical});

    default: {
      auto literal = ReadFloat<T>(tokenizer, context);
      return MakeAt(literal.loc(), FloatResult<T>{literal.value()});
    }
  }
}

template <typename T, size_t N>
auto ReadSimdFloatResult(Tokenizer& tokenizer, Context& context)
    -> At<ReturnResult> {
  static_assert(std::is_floating_point_v<T>, "T must be floating point.");
  LocationGuard guard{tokenizer};
  std::array<FloatResult<T>, N> result;
  for (size_t lane = 0; lane < N; ++lane) {
    result[lane] = ReadFloatResult<T>(tokenizer, context).value();
  }
  return MakeAt(guard.loc(), ReturnResult{result});
}

auto ReadReturnResult(Tokenizer& tokenizer, Context& context)
    -> At<ReturnResult> {
  LocationGuard guard{tokenizer};
  Expect(tokenizer, context, TokenType::Lpar);

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::F32ConstInstr: {
      tokenizer.Read();
      auto result = ReadFloatResult<f32>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{result.value()});
    }

    case TokenType::F64ConstInstr: {
      tokenizer.Read();
      auto result = ReadFloatResult<f64>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{result.value()});
    }

    case TokenType::I32ConstInstr: {
      tokenizer.Read();
      auto result = ReadInt<u32>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{result.value()});
    }

    case TokenType::I64ConstInstr: {
      tokenizer.Read();
      auto result = ReadInt<u64>(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{result.value()});
    }

    case TokenType::SimdConstInstr: {
      if (!context.features.simd_enabled()) {
        context.errors.OnError(token.loc, "Simd values not allowed");
      }
      tokenizer.Read();
      auto simd_token = tokenizer.Peek();

      At<ReturnResult> result;
      switch (simd_token.type) {
        case TokenType::I8X16:
          tokenizer.Read();
          result = ReadSimdValues<u8, 16>(tokenizer, context);
          break;

        case TokenType::I16X8:
          tokenizer.Read();
          result = ReadSimdValues<u16, 8>(tokenizer, context);
          break;

        case TokenType::I32X4:
          tokenizer.Read();
          result = ReadSimdValues<u32, 4>(tokenizer, context);
          break;

        case TokenType::I64X2:
          tokenizer.Read();
          result = ReadSimdValues<u64, 2>(tokenizer, context);
          break;

        case TokenType::F32X4:
          tokenizer.Read();
          result = ReadSimdFloatResult<f32, 4>(tokenizer, context);
          break;

        case TokenType::F64X2:
          tokenizer.Read();
          result = ReadSimdFloatResult<f64, 2>(tokenizer, context);
          break;

        default:
          context.errors.OnError(
              simd_token.loc,
              format("Invalid SIMD constant token, got {}", simd_token.type));
          return MakeAt(guard.loc(), ReturnResult{});
      }

      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{result.value()});
    }

    case TokenType::RefAny:
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.any not allowed");
      }
      tokenizer.Read();
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{RefAnyResult{}});

    case TokenType::RefFuncInstr:
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.func not allowed");
      }
      tokenizer.Read();
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{RefFuncResult{}});

    default:
      context.errors.OnError(token.loc,
                             format("Invalid result, got {}", token.type));
      return MakeAt(guard.loc(), ReturnResult{});
  }
}

auto ReadReturnResultList(Tokenizer& tokenizer, Context& context)
    -> ReturnResultList {
  ReturnResultList result;
  while (tokenizer.Peek().type == TokenType::Lpar) {
    result.push_back(ReadReturnResult(tokenizer, context));
  }
  return result;
}

auto ReadReturnAssertion(Tokenizer& tokenizer, Context& context)
    -> At<ReturnAssertion> {
  LocationGuard guard{tokenizer};
  auto action = ReadAction(tokenizer, context);
  auto results = ReadReturnResultList(tokenizer, context);
  return MakeAt(guard.loc(), ReturnAssertion{action, results});
}

auto ReadAssertion(Tokenizer& tokenizer, Context& context) -> At<Assertion> {
  LocationGuard guard{tokenizer};
  Expect(tokenizer, context, TokenType::Lpar);

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::AssertMalformed: {
      tokenizer.Read();
      auto module = ReadModuleAssertion(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Assertion{AssertionKind::Malformed, module});
    }

    case TokenType::AssertInvalid: {
      tokenizer.Read();
      auto module = ReadModuleAssertion(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Assertion{AssertionKind::Invalid, module});
    }

    case TokenType::AssertUnlinkable: {
      tokenizer.Read();
      auto module = ReadModuleAssertion(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Assertion{AssertionKind::Unlinkable, module});
    }

    case TokenType::AssertTrap: {
      tokenizer.Read();
      // Don't bother checking for Lpar here; it will be checked in
      // ReadModuleAssertion or ReadActionAssertion below.
      if (tokenizer.Peek(1).type == TokenType::Module) {
        auto module = ReadModuleAssertion(tokenizer, context);
        Expect(tokenizer, context, TokenType::Rpar);
        return MakeAt(guard.loc(),
                      Assertion{AssertionKind::ModuleTrap, module});
      } else {
        auto action = ReadActionAssertion(tokenizer, context);
        Expect(tokenizer, context, TokenType::Rpar);
        return MakeAt(guard.loc(),
                      Assertion{AssertionKind::ActionTrap, action});
      }
    }

    case TokenType::AssertReturn: {
      tokenizer.Read();
      auto action = ReadReturnAssertion(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Assertion{AssertionKind::Return, action});
    }

    case TokenType::AssertExhaustion: {
      tokenizer.Read();
      auto action = ReadActionAssertion(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Assertion{AssertionKind::Exhaustion, action});
    }

    default:
      context.errors.OnError(token.loc,
                             format("Invalid action type, got {}", token.type));
      return MakeAt(token.loc, Assertion{});
  }
}

auto ReadRegister(Tokenizer& tokenizer, Context& context) -> At<Register> {
  LocationGuard guard{tokenizer};
  ExpectLpar(tokenizer, context, TokenType::Register);
  auto name = ReadText(tokenizer, context);
  auto module_opt = ReadModuleVarOpt(tokenizer, context);
  Expect(tokenizer, context, TokenType::Rpar);
  return MakeAt(guard.loc(), Register{name, module_opt});
}

auto ReadCommand(Tokenizer& tokenizer, Context& context) -> At<Command> {
  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    context.errors.OnError(token.loc,
                           format("Expected '(', got {}", token.type));
    return MakeAt(token.loc, Command{});
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Module: {
      auto item = ReadScriptModule(tokenizer, context);
      return MakeAt(item.loc(), Command{item.value()});
    }

    case TokenType::Invoke:
    case TokenType::Get: {
      auto item = ReadAction(tokenizer, context);
      return MakeAt(item.loc(), Command{item.value()});
    }

    case TokenType::Register: {
      auto item = ReadRegister(tokenizer, context);
      return MakeAt(item.loc(), Command{item.value()});
    }

    case TokenType::AssertMalformed:
    case TokenType::AssertInvalid:
    case TokenType::AssertUnlinkable:
    case TokenType::AssertTrap:
    case TokenType::AssertReturn:
    case TokenType::AssertExhaustion: {
      auto item = ReadAssertion(tokenizer, context);
      return MakeAt(item.loc(), Command{item.value()});
    }

    default:
      context.errors.OnError(token.loc,
                             format("Invalid command, got {}", token.type));
      return MakeAt(token.loc, Command{});
  }
}

auto ReadScript(Tokenizer& tokenizer, Context& context) -> Script {
  Script result;
  while (tokenizer.Peek().type == TokenType::Lpar) {
    result.push_back(ReadCommand(tokenizer, context));
  }
  return result;
}

}  // namespace text
}  // namespace wasp
