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

#include "wasp/base/errors.h"
#include "wasp/base/format.h"
#include "wasp/text/formatters.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/location_guard.h"
#include "wasp/text/resolve.h"

namespace wasp {
namespace text {

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
      // Resolve the module; this has to be done since the module resolver
      // relies on information that is gathered during parsing. If a new module
      // is read, then this information will be lost.
      //
      // TODO: The information needed should be factored out into a separate
      // object, so the resolve step can be done independent from reading.
      Resolve(context, module);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(
          guard.loc(),
          ScriptModule{name_opt, ScriptModuleKind::Text, module});
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

    case TokenType::RefNullInstr: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.null not allowed");
      }
      tokenizer.Read();
      auto type = ReadReferenceKind(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{RefNullConst{type}});
    }

    case TokenType::RefExtern: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.extern not allowed");
      }
      tokenizer.Read();
      auto nat = ReadNat32(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), Const{RefExternConst{nat}});
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
  while (IsConst(tokenizer)) {
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

    case TokenType::RefNullInstr: {
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.null not allowed");
      }
      tokenizer.Read();
      auto type = ReadReferenceKind(tokenizer, context);
      Expect(tokenizer, context, TokenType::Rpar);
      return MakeAt(guard.loc(), ReturnResult{RefNullConst{type}});
    }

    case TokenType::RefExtern:
      if (!context.features.reference_types_enabled()) {
        context.errors.OnError(token.loc, "ref.extern not allowed");
      }
      tokenizer.Read();
      if (tokenizer.Peek().type == TokenType::Nat) {
        auto nat = ReadNat32(tokenizer, context);
        Expect(tokenizer, context, TokenType::Rpar);
        return MakeAt(guard.loc(), ReturnResult{RefExternConst{nat}});
      } else {
        Expect(tokenizer, context, TokenType::Rpar);
        return MakeAt(guard.loc(), ReturnResult{RefExternResult{}});
      }

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
  while (IsReturnResult(tokenizer)) {
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

    default: {
      if (IsModuleItem(tokenizer)) {
        // Read an inline module (one without a wrapping `(module ...)` as a
        // script.
        LocationGuard guard{tokenizer};
        auto module = ReadModule(tokenizer, context);
        Resolve(context, module);
        auto script_module = MakeAt(
            guard.loc(), ScriptModule{nullopt, ScriptModuleKind::Text, module});
        return MakeAt(script_module.loc(), Command{script_module.value()});
      } else {
        context.errors.OnError(token.loc,
                               format("Invalid command, got {}", token.type));
        return MakeAt(token.loc, Command{});
      }
    }
  }
}

auto ReadScript(Tokenizer& tokenizer, Context& context) -> Script {
  Script result;
  while (IsCommand(tokenizer)) {
    result.push_back(ReadCommand(tokenizer, context));
  }
  return result;
}


// Explicit instantiations.
template auto ReadFloatResult<f32>(Tokenizer&, Context&) -> At<FloatResult<f32>>;
template auto ReadFloatResult<f64>(Tokenizer&, Context&) -> At<FloatResult<f64>>;
template auto ReadSimdFloatResult<f32, 4>(Tokenizer&, Context&) -> At<ReturnResult>;
template auto ReadSimdFloatResult<f64, 2>(Tokenizer&, Context&) -> At<ReturnResult>;

}  // namespace text
}  // namespace wasp
