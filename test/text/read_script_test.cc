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

#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "test/text/constants.h"
#include "wasp/base/errors.h"
#include "wasp/text/formatters.h"
#include "wasp/text/read/macros.h"
#include "wasp/text/read/read_ctx.h"
#include "wasp/text/read/tokenizer.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;
using namespace ::wasp::test;

// TODO: copied from read-test.cc, share?
class TextReadScriptTest : public ::testing::Test {
 protected:
  // Read without checking the expected result.
  template <typename Func, typename... Args>
  void Read(Func&& func, SpanU8 span, Args&&... args) {
    Tokenizer tokenizer{span};
    func(tokenizer, ctx, std::forward<Args>(args)...);
    ExpectNoErrors(errors);
  }

  template <typename Func, typename T, typename... Args>
  void OK(Func&& func, const T& expected, SpanU8 span, Args&&... args) {
    Tokenizer tokenizer{span};
    auto actual = func(tokenizer, ctx, std::forward<Args>(args)...);
    ASSERT_EQ((At{span, expected}), actual);
    ExpectNoErrors(errors);
  }

  // TODO: Remove and just use OK?
  template <typename Func, typename T, typename... Args>
  void OKVector(Func&& func, const T& expected, SpanU8 span, Args&&... args) {
    Tokenizer tokenizer{span};
    auto actual = func(tokenizer, ctx, std::forward<Args>(args)...);
    ASSERT_TRUE(actual.has_value());
    ASSERT_EQ(expected.size(), actual->size());
    for (size_t i = 0; i < expected.size(); ++i) {
      EXPECT_EQ(expected[i], (*actual)[i]);
    }
    ExpectNoErrors(errors);
  }

  template <typename Func, typename... Args>
  void Fail(Func&& func,
            const ExpectedError& error,
            SpanU8 span,
            Args&&... args) {
    Tokenizer tokenizer{span};
    func(tokenizer, ctx, std::forward<Args>(args)...);
    ExpectError(error, errors, span);
    errors.Clear();
  }

  template <typename Func, typename... Args>
  void Fail(Func&& func,
            const std::vector<ExpectedError>& expected_errors,
            SpanU8 span,
            Args&&... args) {
    Tokenizer tokenizer{span};
    func(tokenizer, ctx, std::forward<Args>(args)...);
    ExpectErrors(expected_errors, errors, span);
    errors.Clear();
  }

  TestErrors errors;
  ReadCtx ctx{errors};
};

TEST_F(TextReadScriptTest, ModuleVarOpt) {
  OK(ReadModuleVarOpt, ModuleVar{"$m"_sv}, "$m"_su8);
  OK(ReadModuleVarOpt, nullopt, ""_su8);
}

TEST_F(TextReadScriptTest, ScriptModule) {
  // Text module.
  OK(ReadScriptModule, ScriptModule{nullopt, ScriptModuleKind::Text, Module{}},
     "(module)"_su8);

  // Binary module.
  OK(ReadScriptModule,
     ScriptModule{nullopt, ScriptModuleKind::Binary,
                  TextList{At{"\"\""_su8, Text{"\"\""_sv, 0}}}},
     "(module binary \"\")"_su8);

  // Quote module.
  OK(ReadScriptModule,
     ScriptModule{nullopt, ScriptModuleKind::Quote,
                  TextList{At{"\"\""_su8, Text{"\"\""_sv, 0}}}},
     "(module quote \"\")"_su8);

  // Text module w/ Name.
  OK(ReadScriptModule,
     ScriptModule{At{"$m"_su8, "$m"_sv}, ScriptModuleKind::Text, Module{}},
     "(module $m)"_su8);

  // Binary module w/ Name.
  OK(ReadScriptModule,
     ScriptModule{At{"$m"_su8, "$m"_sv}, ScriptModuleKind::Binary,
                  TextList{At{"\"\""_su8, Text{"\"\""_sv, 0}}}},
     "(module $m binary \"\")"_su8);

  // Quote module w/ Name.
  OK(ReadScriptModule,
     ScriptModule{At{"$m"_su8, "$m"_sv}, ScriptModuleKind::Quote,
                  TextList{At{"\"\""_su8, Text{"\"\""_sv, 0}}}},
     "(module $m quote \"\")"_su8);
}

TEST_F(TextReadScriptTest, Const) {
  // i32.const
  OK(ReadConst, Const{u32{0}}, "(i32.const 0)"_su8);

  // i64.const
  OK(ReadConst, Const{u64{0}}, "(i64.const 0)"_su8);

  // f32.const
  OK(ReadConst, Const{f32{0}}, "(f32.const 0)"_su8);

  // f64.const
  OK(ReadConst, Const{f64{0}}, "(f64.const 0)"_su8);
}

TEST_F(TextReadScriptTest, Const_simd) {
  Fail(ReadConst, {{1, "Simd values not allowed"}},
       "(v128.const i32x4 0 0 0 0)"_su8);

  ctx.features.enable_simd();

  OK(ReadConst, Const{v128{}}, "(v128.const i32x4 0 0 0 0)"_su8);
}

TEST_F(TextReadScriptTest, Const_reference_types) {
  Fail(ReadConst, {{1, "ref.null not allowed"}}, "(ref.null func)"_su8);
  Fail(ReadConst, {{1, "ref.null not allowed"}}, "(ref.null extern)"_su8);
  Fail(ReadConst, {{1, "ref.extern not allowed"}}, "(ref.extern 0)"_su8);

  ctx.features.enable_reference_types();

  OK(ReadConst, Const{RefNullConst{HT_Func}}, "(ref.null func)"_su8);
  OK(ReadConst, Const{RefNullConst{HT_Extern}}, "(ref.null extern)"_su8);
  OK(ReadConst, Const{RefExternConst{At{"0"_su8, u32{0}}}},
     "(ref.extern 0)"_su8);
}

TEST_F(TextReadScriptTest, ConstList) {
  OKVector(ReadConstList, ConstList{}, ""_su8);

  OKVector(ReadConstList,
           ConstList{
               At{"(i32.const 0)"_su8, Const{u32{0}}},
               At{"(i64.const 1)"_su8, Const{u64{1}}},
           },
           "(i32.const 0) (i64.const 1)"_su8);
}

TEST_F(TextReadScriptTest, InvokeAction) {
  // Name.
  OK(ReadInvokeAction,
     InvokeAction{nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}},
     "(invoke \"a\")"_su8);

  // Module.
  OK(ReadInvokeAction,
     InvokeAction{
         At{"$m"_su8, "$m"_sv}, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}},
     "(invoke $m \"a\")"_su8);

  // Const list.
  OK(ReadInvokeAction,
     InvokeAction{nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                  ConstList{At{"(i32.const 0)"_su8, Const{u32{0}}}}},
     "(invoke \"a\" (i32.const 0))"_su8);
}

TEST_F(TextReadScriptTest, GetAction) {
  // Name.
  OK(ReadGetAction, GetAction{nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}},
     "(get \"a\")"_su8);

  // Module.
  OK(ReadGetAction,
     GetAction{At{"$m"_su8, "$m"_sv}, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}},
     "(get $m \"a\")"_su8);
}

TEST_F(TextReadScriptTest, Action) {
  // Get action.
  OK(ReadAction,
     Action{GetAction{nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}}},
     "(get \"a\")"_su8);

  // Invoke action.
  OK(ReadAction,
     Action{InvokeAction{nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}},
     "(invoke \"a\")"_su8);
}

TEST_F(TextReadScriptTest, ModuleAssertion) {
  OK(ReadModuleAssertion,
     ModuleAssertion{
         At{"(module)"_su8,
            ScriptModule{nullopt, ScriptModuleKind::Text, Module{}}},
         At{"\"msg\""_su8, Text{"\"msg\"", 3}}},
     "(module) \"msg\""_su8);
}

TEST_F(TextReadScriptTest, ActionAssertion) {
  OK(ReadActionAssertion,
     ActionAssertion{
         At{"(invoke \"a\")"_su8,
            Action{InvokeAction{
                nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}}},
         At{"\"msg\""_su8, Text{"\"msg\"", 3}},
     },
     "(invoke \"a\") \"msg\""_su8);
}

TEST_F(TextReadScriptTest, FloatResult) {
  OK(ReadFloatResult<f32>, F32Result{f32{0}}, "0"_su8);
  OK(ReadFloatResult<f32>, F32Result{NanKind::Arithmetic},
     "nan:arithmetic"_su8);
  OK(ReadFloatResult<f32>, F32Result{NanKind::Canonical}, "nan:canonical"_su8);

  OK(ReadFloatResult<f64>, F64Result{f64{0}}, "0"_su8);
  OK(ReadFloatResult<f64>, F64Result{NanKind::Arithmetic},
     "nan:arithmetic"_su8);
  OK(ReadFloatResult<f64>, F64Result{NanKind::Canonical}, "nan:canonical"_su8);
}

TEST_F(TextReadScriptTest, SimdFloatResult) {
  OK(ReadSimdFloatResult<f32, 4>,
     ReturnResult{F32x4Result{
         F32Result{f32{0}},
         F32Result{f32{0}},
         F32Result{f32{0}},
         F32Result{f32{0}},
     }},
     "0 0 0 0"_su8);

  OK(ReadSimdFloatResult<f32, 4>,
     ReturnResult{F32x4Result{
         F32Result{f32{0}},
         F32Result{NanKind::Arithmetic},
         F32Result{f32{0}},
         F32Result{NanKind::Canonical},
     }},
     "0 nan:arithmetic 0 nan:canonical"_su8);

  OK(ReadSimdFloatResult<f64, 2>,
     ReturnResult{F64x2Result{
         F64Result{f64{0}},
         F64Result{f64{0}},
     }},
     "0 0"_su8);

  OK(ReadSimdFloatResult<f64, 2>,
     ReturnResult{F64x2Result{
         F64Result{NanKind::Arithmetic},
         F64Result{f64{0}},
     }},
     "nan:arithmetic 0"_su8);
}

TEST_F(TextReadScriptTest, ReturnResult) {
  OK(ReadReturnResult, ReturnResult{u32{0}}, "(i32.const 0)"_su8);

  OK(ReadReturnResult, ReturnResult{u64{0}}, "(i64.const 0)"_su8);

  OK(ReadReturnResult, ReturnResult{F32Result{f32{0}}}, "(f32.const 0)"_su8);
  OK(ReadReturnResult, ReturnResult{F32Result{NanKind::Arithmetic}},
     "(f32.const nan:arithmetic)"_su8);
  OK(ReadReturnResult, ReturnResult{F32Result{NanKind::Canonical}},
     "(f32.const nan:canonical)"_su8);

  OK(ReadReturnResult, ReturnResult{F64Result{f64{0}}}, "(f64.const 0)"_su8);
  OK(ReadReturnResult, ReturnResult{F64Result{NanKind::Arithmetic}},
     "(f64.const nan:arithmetic)"_su8);
  OK(ReadReturnResult, ReturnResult{F64Result{NanKind::Canonical}},
     "(f64.const nan:canonical)"_su8);
}

TEST_F(TextReadScriptTest, ReturnResult_simd) {
  Fail(ReadConst, {{1, "Simd values not allowed"}},
       "(v128.const i32x4 0 0 0 0)"_su8);

  ctx.features.enable_simd();

  OK(ReadReturnResult, ReturnResult{v128{}},
     "(v128.const i8x16 0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0)"_su8);
  OK(ReadReturnResult, ReturnResult{v128{}},
     "(v128.const i16x8 0 0 0 0  0 0 0 0)"_su8);
  OK(ReadReturnResult, ReturnResult{v128{}}, "(v128.const i32x4 0 0 0 0)"_su8);
  OK(ReadReturnResult, ReturnResult{v128{}}, "(v128.const i64x2 0 0)"_su8);
  OK(ReadReturnResult, ReturnResult{F32x4Result{}},
     "(v128.const f32x4 0 0 0 0)"_su8);
  OK(ReadReturnResult, ReturnResult{F64x2Result{}},
     "(v128.const f64x2 0 0)"_su8);

  OK(ReadReturnResult,
     ReturnResult{F32x4Result{
         F32Result{f32{0}},
         F32Result{NanKind::Arithmetic},
         F32Result{f32{0}},
         F32Result{NanKind::Canonical},
     }},
     "(v128.const f32x4 0 nan:arithmetic 0 nan:canonical)"_su8);

  OK(ReadReturnResult,
     ReturnResult{F64x2Result{
         F64Result{f64{0}},
         F64Result{NanKind::Arithmetic},
     }},
     "(v128.const f64x2 0 nan:arithmetic)"_su8);
}

TEST_F(TextReadScriptTest, ReturnResult_reference_types) {
  Fail(ReadReturnResult, {{1, "ref.null not allowed"}}, "(ref.null func)"_su8);
  Fail(ReadReturnResult, {{1, "ref.null not allowed"}},
       "(ref.null extern)"_su8);
  Fail(ReadReturnResult, {{1, "ref.extern not allowed"}}, "(ref.extern 0)"_su8);
  Fail(ReadReturnResult, {{1, "ref.extern not allowed"}}, "(ref.extern)"_su8);
  Fail(ReadReturnResult, {{1, "ref.func not allowed"}}, "(ref.func)"_su8);

  ctx.features.enable_reference_types();

  OK(ReadReturnResult, ReturnResult{RefNullConst{HT_Func}},
     "(ref.null func)"_su8);
  OK(ReadReturnResult, ReturnResult{RefNullConst{HT_Extern}},
     "(ref.null extern)"_su8);
  OK(ReadReturnResult, ReturnResult{RefExternConst{At{"0"_su8, u32{0}}}},
     "(ref.extern 0)"_su8);
  OK(ReadReturnResult, ReturnResult{RefExternResult{}}, "(ref.extern)"_su8);
  OK(ReadReturnResult, ReturnResult{RefFuncResult{}}, "(ref.func)"_su8);
}

TEST_F(TextReadScriptTest, ReturnResultList) {
  OK(ReadReturnResultList, ReturnResultList{}, ""_su8);

  OK(ReadReturnResultList,
     ReturnResultList{
         At{"(i32.const 0)"_su8, ReturnResult{u32{0}}},
         At{"(f32.const nan:canonical)"_su8,
            ReturnResult{F32Result{NanKind::Canonical}}},
     },
     "(i32.const 0) (f32.const nan:canonical)"_su8);
}

TEST_F(TextReadScriptTest, ReturnAssertion) {
  OK(ReadReturnAssertion,
     ReturnAssertion{
         At{"(invoke \"a\")"_su8,
            Action{InvokeAction{
                nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}}},
         {}},
     "(invoke \"a\")"_su8);

  OK(ReadReturnAssertion,
     ReturnAssertion{
         At{"(invoke \"a\" (i32.const 0))"_su8,
            Action{InvokeAction{
                nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                ConstList{At{"(i32.const 0)"_su8, Const{u32{0}}}}}}},
         ReturnResultList{At{"(i32.const 1)"_su8, ReturnResult{u32{1}}}}},
     "(invoke \"a\" (i32.const 0)) (i32.const 1)"_su8);
}

TEST_F(TextReadScriptTest, Assertion) {
  // assert_malformed
  OK(ReadAssertion,
     Assertion{
         AssertionKind::Malformed,
         ModuleAssertion{At{"(module)"_su8,
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
                         At{"\"msg\""_su8, Text{"\"msg\"", 3}}}},
     "(assert_malformed (module) \"msg\")"_su8);

  // assert_invalid
  OK(ReadAssertion,
     Assertion{
         AssertionKind::Invalid,
         ModuleAssertion{At{"(module)"_su8,
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
                         At{"\"msg\""_su8, Text{"\"msg\"", 3}}}},
     "(assert_invalid (module) \"msg\")"_su8);

  // assert_unlinkable
  OK(ReadAssertion,
     Assertion{
         AssertionKind::Unlinkable,
         ModuleAssertion{At{"(module)"_su8,
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
                         At{"\"msg\""_su8, Text{"\"msg\"", 3}}}},
     "(assert_unlinkable (module) \"msg\")"_su8);

  // assert_trap (module)
  OK(ReadAssertion,
     Assertion{
         AssertionKind::ModuleTrap,
         ModuleAssertion{At{"(module)"_su8,
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
                         At{"\"msg\""_su8, Text{"\"msg\"", 3}}}},
     "(assert_trap (module) \"msg\")"_su8);

  // assert_return
  OK(ReadAssertion,
     Assertion{AssertionKind::Return,
               ReturnAssertion{
                   At{"(invoke \"a\")"_su8,
                      Action{InvokeAction{
                          nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}}},
                   {}}},
     "(assert_return (invoke \"a\"))"_su8);

  // assert_trap (action)
  OK(ReadAssertion,
     Assertion{AssertionKind::ActionTrap,
               ActionAssertion{
                   At{"(invoke \"a\")"_su8,
                      Action{InvokeAction{
                          nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}}},
                   At{"\"msg\""_su8, Text{"\"msg\""_sv, 3}}}},
     "(assert_trap (invoke \"a\") \"msg\")"_su8);

  // assert_exhaustion
  OK(ReadAssertion,
     Assertion{AssertionKind::Exhaustion,
               ActionAssertion{
                   At{"(invoke \"a\")"_su8,
                      Action{InvokeAction{
                          nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}}},
                   At{"\"msg\""_su8, Text{"\"msg\""_sv, 3}}}},
     "(assert_exhaustion (invoke \"a\") \"msg\")"_su8);
}

TEST_F(TextReadScriptTest, Register) {
  OK(ReadRegister, Register{At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, nullopt},
     "(register \"a\")"_su8);

  OK(ReadRegister,
     Register{At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, At{"$m"_su8, "$m"_sv}},
     "(register \"a\" $m)"_su8);
}

TEST_F(TextReadScriptTest, Command) {
  // Module.
  OK(ReadCommand, Command{ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
     "(module)"_su8);

  // Action.
  OK(ReadCommand,
     Command{InvokeAction{nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}},
     "(invoke \"a\")"_su8);

  // Assertion.
  OK(ReadCommand,
     Command{Assertion{
         AssertionKind::Invalid,
         ModuleAssertion{At{"(module)"_su8,
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
                         At{"\"msg\""_su8, Text{"\"msg\"", 3}}}}},
     "(assert_invalid (module) \"msg\")"_su8);

  // Register.
  OK(ReadCommand,
     Command{Register{At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, nullopt}},
     "(register \"a\")"_su8);
}

TEST_F(TextReadScriptTest, Script) {
  OKVector(ReadScript,
           Script{
               At{"(module)"_su8,
                  Command{ScriptModule{nullopt, ScriptModuleKind::Text, {}}}},
               At{"(invoke \"a\")"_su8,
                  Command{InvokeAction{
                      nullopt, At{"\"a\""_su8, Text{"\"a\""_sv, 1}}, {}}}},
               At{"(assert_invalid (module) \"msg\")"_su8,
                  Command{Assertion{
                      AssertionKind::Invalid,
                      ModuleAssertion{
                          At{"(module)"_su8,
                             ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
                          At{"\"msg\""_su8, Text{"\"msg\"", 3}}}}}},
           },
           "(module) (invoke \"a\") (assert_invalid (module) \"msg\")"_su8);
}
