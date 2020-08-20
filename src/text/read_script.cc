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

#include <type_traits>

#include "wasp/base/concat.h"
#include "wasp/base/errors.h"
#include "wasp/text/formatters.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/location_guard.h"
#include "wasp/text/read/macros.h"
#include "wasp/text/resolve.h"

namespace wasp::text {

auto ReadModuleVarOpt(Tokenizer& tokenizer, Context& context)
    -> OptAt<ModuleVar> {
  auto token_opt = tokenizer.Match(TokenType::Id);
  if (!token_opt) {
    return nullopt;
  }
  return At{token_opt->loc, ModuleVar{token_opt->as_string_view()}};
}

auto ReadScriptModule(Tokenizer& tokenizer, Context& context)
    -> OptAt<ScriptModule> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, context, TokenType::Module));
  auto name_opt = ReadModuleVarOpt(tokenizer, context);
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::Binary: {
      tokenizer.Read();
      WASP_TRY_READ(text_list, ReadTextList(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(),
                ScriptModule{name_opt, ScriptModuleKind::Binary, text_list}};
    }

    case TokenType::Quote: {
      tokenizer.Read();
      WASP_TRY_READ(text_list, ReadTextList(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(),
                ScriptModule{name_opt, ScriptModuleKind::Quote, text_list}};
    }

    default: {
      WASP_TRY_READ(module, ReadModule(tokenizer, context));
      Expect(tokenizer, context, TokenType::Rpar);
      return At{guard.loc(),
                ScriptModule{name_opt, ScriptModuleKind::Text, module}};
    }
  }
}

bool IsConst(Tokenizer& tokenizer) {
  if (tokenizer.Peek().type != TokenType::Lpar) {
    return false;
  }

  auto token = tokenizer.Peek(1);
  return token.type == TokenType::F32ConstInstr ||
         token.type == TokenType::F64ConstInstr ||
         token.type == TokenType::I32ConstInstr ||
         token.type == TokenType::I64ConstInstr ||
         token.type == TokenType::SimdConstInstr ||
         token.type == TokenType::RefNullInstr ||
         token.type == TokenType::RefExtern;
}

auto ReadConst(Tokenizer& tokenizer, Context& context) -> OptAt<Const> {
  // TODO: Share with ReadPlainInstruction above?
  LocationGuard guard{tokenizer};
  WASP_TRY(Expect(tokenizer, context, TokenType::Lpar));

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::F32ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(literal, ReadFloat<f32>(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{literal.value()}};
    }

    case TokenType::F64ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(literal, ReadFloat<f64>(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{literal.value()}};
    }

    case TokenType::I32ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(literal, ReadInt<u32>(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{literal.value()}};
    }

    case TokenType::I64ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(literal, ReadInt<u64>(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{literal.value()}};
    }

    case TokenType::SimdConstInstr: {
      if (!context.features.simd_enabled()) {
        context.errors.OnError(token.loc, "Simd values not allowed");
        return nullopt;
      }
      tokenizer.Read();
      auto simd_token = tokenizer.Peek();

      At<v128> literal;
      switch (simd_token.type) {
        case TokenType::I8X16: {
          tokenizer.Read();
          WASP_TRY_READ(literal_, (ReadSimdValues<u8, 16>(tokenizer, context)));
          literal = literal_;
          break;
        }

        case TokenType::I16X8: {
          tokenizer.Read();
          WASP_TRY_READ(literal_, (ReadSimdValues<u16, 8>(tokenizer, context)));
          literal = literal_;
          break;
        }

        case TokenType::I32X4: {
          tokenizer.Read();
          WASP_TRY_READ(literal_, (ReadSimdValues<u32, 4>(tokenizer, context)));
          literal = literal_;
          break;
        }

        case TokenType::I64X2: {
          tokenizer.Read();
          WASP_TRY_READ(literal_, (ReadSimdValues<u64, 2>(tokenizer, context)));
          literal = literal_;
          break;
        }

        case TokenType::F32X4: {
          tokenizer.Read();
          WASP_TRY_READ(literal_, (ReadSimdValues<f32, 4>(tokenizer, context)));
          literal = literal_;
          break;
        }

        case TokenType::F64X2: {
          tokenizer.Read();
          WASP_TRY_READ(literal_, (ReadSimdValues<f64, 2>(tokenizer, context)));
          literal = literal_;
          break;
        }

        default:
          context.errors.OnError(
              simd_token.loc,
              concat("Invalid SIMD constant token, got ", simd_token.type));
          return nullopt;
      }

      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{literal.value()}};
    }

    case TokenType::RefNullInstr: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.null not allowed");
        return nullopt;
      }
      tokenizer.Read();
      WASP_TRY_READ(type, ReadHeapType(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{RefNullConst{type}}};
    }

    case TokenType::RefExtern: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.extern not allowed");
        return nullopt;
      }
      tokenizer.Read();
      WASP_TRY_READ(nat, ReadNat32(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Const{RefExternConst{nat}}};
    }

    default:
      context.errors.OnError(token.loc,
                             concat("Invalid constant, got ", token.type));
      return nullopt;
  }
}

auto ReadConstList(Tokenizer& tokenizer, Context& context)
    -> optional<ConstList> {
  ConstList result;
  while (IsConst(tokenizer)) {
    WASP_TRY_READ(const_, ReadConst(tokenizer, context));
    result.push_back(const_);
  }
  return result;
}

auto ReadInvokeAction(Tokenizer& tokenizer, Context& context)
    -> OptAt<InvokeAction> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, context, TokenType::Invoke));
  auto module_opt = ReadModuleVarOpt(tokenizer, context);
  WASP_TRY_READ(name, ReadUtf8Text(tokenizer, context));
  WASP_TRY_READ(const_list, ReadConstList(tokenizer, context));
  WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
  return At{guard.loc(), InvokeAction{module_opt, name, const_list}};
}

auto ReadGetAction(Tokenizer& tokenizer, Context& context) -> OptAt<GetAction> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, context, TokenType::Get));
  auto module_opt = ReadModuleVarOpt(tokenizer, context);
  WASP_TRY_READ(name, ReadUtf8Text(tokenizer, context));
  WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
  return At{guard.loc(), GetAction{module_opt, name}};
}

auto ReadAction(Tokenizer& tokenizer, Context& context) -> OptAt<Action> {
  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    context.errors.OnError(token.loc, concat("Expected '(', got ", token.type));
    return nullopt;
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Invoke: {
      WASP_TRY_READ(action, ReadInvokeAction(tokenizer, context));
      return At{action.loc(), Action{action.value()}};
    }

    case TokenType::Get: {
      WASP_TRY_READ(action, ReadGetAction(tokenizer, context));
      return At{action.loc(), Action{action.value()}};
    }

    default:
      context.errors.OnError(token.loc,
                             concat("Invalid action type, got ", token.type));
      return nullopt;
  }
}

auto ReadModuleAssertion(Tokenizer& tokenizer, Context& context)
    -> OptAt<ModuleAssertion> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(module, ReadScriptModule(tokenizer, context));
  WASP_TRY_READ(text, ReadText(tokenizer, context));
  return At{guard.loc(), ModuleAssertion{module, text}};
}

auto ReadActionAssertion(Tokenizer& tokenizer, Context& context)
    -> OptAt<ActionAssertion> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(action, ReadAction(tokenizer, context));
  WASP_TRY_READ(text, ReadText(tokenizer, context));
  return At{guard.loc(), ActionAssertion{action, text}};
}

template <typename T>
auto ReadFloatResult(Tokenizer& tokenizer, Context& context)
    -> OptAt<FloatResult<T>> {
  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::NanArithmetic:
      tokenizer.Read();
      return At{token.loc, FloatResult<T>{NanKind::Arithmetic}};

    case TokenType::NanCanonical:
      tokenizer.Read();
      return At{token.loc, FloatResult<T>{NanKind::Canonical}};

    default: {
      WASP_TRY_READ(literal, (ReadFloat<T>(tokenizer, context)));
      return At{literal.loc(), FloatResult<T>{literal.value()}};
    }
  }
}

template <typename T, size_t N>
auto ReadSimdFloatResult(Tokenizer& tokenizer, Context& context)
    -> OptAt<ReturnResult> {
  static_assert(std::is_floating_point_v<T>, "T must be floating point.");
  LocationGuard guard{tokenizer};
  std::array<FloatResult<T>, N> result;
  for (size_t lane = 0; lane < N; ++lane) {
    WASP_TRY_READ(value, (ReadFloatResult<T>(tokenizer, context)));
    result[lane] = value;
  }
  return At{guard.loc(), ReturnResult{result}};
}

bool IsReturnResult(Tokenizer& tokenizer) {
  if (tokenizer.Peek().type != TokenType::Lpar) {
    return false;
  }

  auto token = tokenizer.Peek(1);
  return token.type == TokenType::F32ConstInstr ||
         token.type == TokenType::F64ConstInstr ||
         token.type == TokenType::I32ConstInstr ||
         token.type == TokenType::I64ConstInstr ||
         token.type == TokenType::SimdConstInstr ||
         token.type == TokenType::RefNullInstr ||
         token.type == TokenType::RefExtern ||
         token.type == TokenType::RefFuncInstr;
}

auto ReadReturnResult(Tokenizer& tokenizer, Context& context)
    -> OptAt<ReturnResult> {
  LocationGuard guard{tokenizer};
  WASP_TRY(Expect(tokenizer, context, TokenType::Lpar));

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::F32ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(result, (ReadFloatResult<f32>(tokenizer, context)));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{result.value()}};
    }

    case TokenType::F64ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(result, (ReadFloatResult<f64>(tokenizer, context)));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{result.value()}};
    }

    case TokenType::I32ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(result, (ReadInt<u32>(tokenizer, context)));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{result.value()}};
    }

    case TokenType::I64ConstInstr: {
      tokenizer.Read();
      WASP_TRY_READ(result, (ReadInt<u64>(tokenizer, context)));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{result.value()}};
    }

    case TokenType::SimdConstInstr: {
      if (!context.features.simd_enabled()) {
        context.errors.OnError(token.loc, "Simd values not allowed");
        return nullopt;
      }
      tokenizer.Read();
      auto simd_token = tokenizer.Peek();

      At<ReturnResult> result;
      switch (simd_token.type) {
        case TokenType::I8X16: {
          tokenizer.Read();
          WASP_TRY_READ(result_, (ReadSimdValues<u8, 16>(tokenizer, context)));
          result = result_;
          break;
        }

        case TokenType::I16X8: {
          tokenizer.Read();
          WASP_TRY_READ(result_, (ReadSimdValues<u16, 8>(tokenizer, context)));
          result = result_;
          break;
        }

        case TokenType::I32X4: {
          tokenizer.Read();
          WASP_TRY_READ(result_, (ReadSimdValues<u32, 4>(tokenizer, context)));
          result = result_;
          break;
        }

        case TokenType::I64X2: {
          tokenizer.Read();
          WASP_TRY_READ(result_, (ReadSimdValues<u64, 2>(tokenizer, context)));
          result = result_;
          break;
        }

        case TokenType::F32X4: {
          tokenizer.Read();
          WASP_TRY_READ(result_,
                        (ReadSimdFloatResult<f32, 4>(tokenizer, context)));
          result = result_;
          break;
        }

        case TokenType::F64X2: {
          tokenizer.Read();
          WASP_TRY_READ(result_,
                        (ReadSimdFloatResult<f64, 2>(tokenizer, context)));
          result = result_;
          break;
        }

        default:
          context.errors.OnError(
              simd_token.loc,
              concat("Invalid SIMD constant token, got ", simd_token.type));
          return nullopt;
      }

      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{result.value()}};
    }

    case TokenType::RefNullInstr: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.null not allowed");
        return nullopt;
      }
      tokenizer.Read();
      WASP_TRY_READ(type, ReadHeapType(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{RefNullConst{type}}};
    }

    case TokenType::RefExtern:
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.extern not allowed");
        return nullopt;
      }
      tokenizer.Read();
      if (tokenizer.Peek().type == TokenType::Nat) {
        WASP_TRY_READ(nat, ReadNat32(tokenizer, context));
        WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
        return At{guard.loc(), ReturnResult{RefExternConst{nat}}};
      } else {
        WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
        return At{guard.loc(), ReturnResult{RefExternResult{}}};
      }

    case TokenType::RefFuncInstr:
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.func not allowed");
        return nullopt;
      }
      tokenizer.Read();
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), ReturnResult{RefFuncResult{}}};

    default:
      context.errors.OnError(token.loc,
                             concat("Invalid result, got ", token.type));
      return nullopt;
  }
}

auto ReadReturnResultList(Tokenizer& tokenizer, Context& context)
    -> optional<ReturnResultList> {
  ReturnResultList result;
  while (IsReturnResult(tokenizer)) {
    WASP_TRY_READ(value, ReadReturnResult(tokenizer, context));
    result.push_back(value);
  }
  return result;
}

auto ReadReturnAssertion(Tokenizer& tokenizer, Context& context)
    -> OptAt<ReturnAssertion> {
  LocationGuard guard{tokenizer};
  WASP_TRY_READ(action, ReadAction(tokenizer, context));
  WASP_TRY_READ(results, ReadReturnResultList(tokenizer, context));
  return At{guard.loc(), ReturnAssertion{action, results}};
}

auto ReadAssertion(Tokenizer& tokenizer, Context& context) -> OptAt<Assertion> {
  LocationGuard guard{tokenizer};
  WASP_TRY(Expect(tokenizer, context, TokenType::Lpar));

  auto token = tokenizer.Peek();
  switch (token.type) {
    case TokenType::AssertMalformed: {
      tokenizer.Read();
      WASP_TRY_READ(module, ReadModuleAssertion(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Assertion{AssertionKind::Malformed, module}};
    }

    case TokenType::AssertInvalid: {
      tokenizer.Read();
      WASP_TRY_READ(module, ReadModuleAssertion(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Assertion{AssertionKind::Invalid, module}};
    }

    case TokenType::AssertUnlinkable: {
      tokenizer.Read();
      WASP_TRY_READ(module, ReadModuleAssertion(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Assertion{AssertionKind::Unlinkable, module}};
    }

    case TokenType::AssertTrap: {
      tokenizer.Read();
      // Don't bother checking for Lpar here; it will be checked in
      // ReadModuleAssertion or ReadActionAssertion below.
      if (tokenizer.Peek(1).type == TokenType::Module) {
        WASP_TRY_READ(module, ReadModuleAssertion(tokenizer, context));
        WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
        return At{guard.loc(), Assertion{AssertionKind::ModuleTrap, module}};
      } else {
        WASP_TRY_READ(action, ReadActionAssertion(tokenizer, context));
        WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
        return At{guard.loc(), Assertion{AssertionKind::ActionTrap, action}};
      }
    }

    case TokenType::AssertReturn: {
      tokenizer.Read();
      WASP_TRY_READ(action, ReadReturnAssertion(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Assertion{AssertionKind::Return, action}};
    }

    case TokenType::AssertExhaustion: {
      tokenizer.Read();
      WASP_TRY_READ(action, ReadActionAssertion(tokenizer, context));
      WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
      return At{guard.loc(), Assertion{AssertionKind::Exhaustion, action}};
    }

    default:
      context.errors.OnError(token.loc,
                             concat("Invalid action type, got ", token.type));
      return nullopt;
  }
}

auto ReadRegister(Tokenizer& tokenizer, Context& context) -> OptAt<Register> {
  LocationGuard guard{tokenizer};
  WASP_TRY(ExpectLpar(tokenizer, context, TokenType::Register));
  WASP_TRY_READ(name, ReadText(tokenizer, context));
  auto module_opt = ReadModuleVarOpt(tokenizer, context);
  WASP_TRY(Expect(tokenizer, context, TokenType::Rpar));
  return At{guard.loc(), Register{name, module_opt}};
}

bool IsCommand(Tokenizer& tokenizer) {
  if (tokenizer.Peek().type != TokenType::Lpar) {
    return false;
  }

  auto token = tokenizer.Peek(1);
  return IsModuleItem(tokenizer) ||  // For an inline-module.
         token.type == TokenType::Module || token.type == TokenType::Invoke ||
         token.type == TokenType::Get || token.type == TokenType::Register ||
         token.type == TokenType::AssertMalformed ||
         token.type == TokenType::AssertInvalid ||
         token.type == TokenType::AssertUnlinkable ||
         token.type == TokenType::AssertTrap ||
         token.type == TokenType::AssertReturn ||
         token.type == TokenType::AssertExhaustion;
}

auto ReadCommand(Tokenizer& tokenizer, Context& context) -> OptAt<Command> {
  auto token = tokenizer.Peek();
  if (token.type != TokenType::Lpar) {
    context.errors.OnError(token.loc, concat("Expected '(', got ", token.type));
    return nullopt;
  }

  token = tokenizer.Peek(1);
  switch (token.type) {
    case TokenType::Module: {
      WASP_TRY_READ(item, ReadScriptModule(tokenizer, context));
      return At{item.loc(), Command{item.value()}};
    }

    case TokenType::Invoke:
    case TokenType::Get: {
      WASP_TRY_READ(item, ReadAction(tokenizer, context));
      return At{item.loc(), Command{item.value()}};
    }

    case TokenType::Register: {
      WASP_TRY_READ(item, ReadRegister(tokenizer, context));
      return At{item.loc(), Command{item.value()}};
    }

    case TokenType::AssertMalformed:
    case TokenType::AssertInvalid:
    case TokenType::AssertUnlinkable:
    case TokenType::AssertTrap:
    case TokenType::AssertReturn:
    case TokenType::AssertExhaustion: {
      WASP_TRY_READ(item, ReadAssertion(tokenizer, context));
      return At{item.loc(), Command{item.value()}};
    }

    default: {
      if (IsModuleItem(tokenizer)) {
        // Read an inline module (one without a wrapping `(module ...)` as a
        // script.
        LocationGuard guard{tokenizer};
        WASP_TRY_READ(module, ReadModule(tokenizer, context));
        auto script_module = At{
            guard.loc(), ScriptModule{nullopt, ScriptModuleKind::Text, module}};
        return At{script_module.loc(), Command{script_module.value()}};
      } else {
        context.errors.OnError(token.loc,
                               concat("Invalid command, got ", token.type));
        return nullopt;
      }
    }
  }
}

auto ReadScript(Tokenizer& tokenizer, Context& context) -> optional<Script> {
  Script result;
  while (IsCommand(tokenizer)) {
    WASP_TRY_READ(command, ReadCommand(tokenizer, context));
    result.push_back(command);
  }
  return result;
}


// Explicit instantiations.
template auto ReadFloatResult<f32>(Tokenizer&, Context&) -> OptAt<FloatResult<f32>>;
template auto ReadFloatResult<f64>(Tokenizer&, Context&) -> OptAt<FloatResult<f64>>;
template auto ReadSimdFloatResult<f32, 4>(Tokenizer&, Context&) -> OptAt<ReturnResult>;
template auto ReadSimdFloatResult<f64, 2>(Tokenizer&, Context&) -> OptAt<ReturnResult>;

}  // namespace wasp::text
