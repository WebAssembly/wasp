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
#include "wasp/text/read/context.h"
#include "wasp/text/read/macros.h"
#include "wasp/text/read/tokenizer.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;
using namespace ::wasp::test;

class TextReadTest : public ::testing::Test {
 protected:
  using BVT = BoundValueType;
  using I = Instruction;
  using O = Opcode;

  // Read without checking the expected result.
  template <typename Func, typename... Args>
  void Read(Func&& func, SpanU8 span, Args&&... args) {
    Tokenizer tokenizer{span};
    func(tokenizer, context, std::forward<Args>(args)...);
    ExpectNoErrors(errors);
  }

  template <typename Func, typename T, typename... Args>
  void OK(Func&& func, const T& expected, SpanU8 span, Args&&... args) {
    Tokenizer tokenizer{span};
    auto actual = func(tokenizer, context, std::forward<Args>(args)...);
    ASSERT_EQ(MakeAt(span, expected), actual);
    ExpectNoErrors(errors);
  }

  // TODO: Remove and just use OK?
  template <typename Func, typename T, typename... Args>
  void OKVector(Func&& func, const T& expected, SpanU8 span, Args&&... args) {
    Tokenizer tokenizer{span};
    auto actual = func(tokenizer, context, std::forward<Args>(args)...);
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
    func(tokenizer, context, std::forward<Args>(args)...);
    ExpectError(error, errors, span);
    errors.Clear();
  }

  template <typename Func, typename... Args>
  void Fail(Func&& func,
            const std::vector<ExpectedError>& expected_errors,
            SpanU8 span,
            Args&&... args) {
    Tokenizer tokenizer{span};
    func(tokenizer, context, std::forward<Args>(args)...);
    ExpectErrors(expected_errors, errors, span);
    errors.Clear();
  }

  TestErrors errors;
  Context context{errors};
};

// Helpers for handling InstructionList functions.

auto ReadBlockInstruction_ForTesting(Tokenizer& tokenizer, Context& context)
    -> optional<InstructionList> {
  InstructionList result;
  WASP_TRY(ReadBlockInstruction(tokenizer, context, result));
  return result;
}

auto ReadLetInstruction_ForTesting(Tokenizer& tokenizer, Context& context)
    -> optional<InstructionList> {
  InstructionList result;
  WASP_TRY(ReadLetInstruction(tokenizer, context, result));
  return result;
}

auto ReadInstructionList_ForTesting(Tokenizer& tokenizer, Context& context)
    -> optional<InstructionList> {
  InstructionList result;
  WASP_TRY(ReadInstructionList(tokenizer, context, result));
  return result;
}

auto ReadExpressionList_ForTesting(Tokenizer& tokenizer, Context& context)
    -> optional<InstructionList> {
  InstructionList result;
  WASP_TRY(ReadExpressionList(tokenizer, context, result));
  return result;
}


TEST_F(TextReadTest, Nat32) {
  OK(ReadNat32, u32{123}, "123"_su8);
}

TEST_F(TextReadTest, Int32) {
  OK(ReadInt<u32>, u32{123}, "123"_su8);
  OK(ReadInt<u32>, u32{456}, "+456"_su8);
  OK(ReadInt<u32>, u32(-789), "-789"_su8);
}

TEST_F(TextReadTest, Var_Nat32) {
  OK(ReadVar, Var{Index{123}}, "123"_su8);
}

TEST_F(TextReadTest, Var_Id) {
  OK(ReadVar, Var{"$foo"_sv}, "$foo"_su8);
}

TEST_F(TextReadTest, VarOpt_Nat32) {
  OK(ReadVarOpt, Var{Index{3141}}, "3141"_su8);
  OK(ReadVarOpt, Var{"$bar"_sv}, "$bar"_su8);
}

TEST_F(TextReadTest, BindVarOpt) {
  OK(ReadBindVarOpt, BindVar{"$bar"_sv}, "$bar"_su8);
}

TEST_F(TextReadTest, VarList) {
  auto span = "$a $b 1 2"_su8;
  std::vector<At<Var>> expected{
      MakeAt("$a"_su8, Var{"$a"_sv}),
      MakeAt("$b"_su8, Var{"$b"_sv}),
      MakeAt("1"_su8, Var{Index{1}}),
      MakeAt("2"_su8, Var{Index{2}}),
  };
  OKVector(ReadVarList, expected, span);
}

TEST_F(TextReadTest, Text) {
  OK(ReadText, Text{"\"hello\""_sv, 5}, "\"hello\""_su8);
}

TEST_F(TextReadTest, Utf8Text) {
  OK(ReadUtf8Text, Text{"\"\\ee\\b8\\96\""_sv, 3}, "\"\\ee\\b8\\96\""_su8);
  Fail(ReadUtf8Text, {{0, "Invalid UTF-8 encoding"}}, "\"\\80\""_su8);
}

TEST_F(TextReadTest, TextList) {
  auto span = "\"hello, \" \"world\" \"123\""_su8;
  std::vector<At<Text>> expected{
      MakeAt("\"hello, \""_su8, Text{"\"hello, \""_sv, 7}),
      MakeAt("\"world\""_su8, Text{"\"world\""_sv, 5}),
      MakeAt("\"123\""_su8, Text{"\"123\""_sv, 3}),
  };
  OKVector(ReadTextList, expected, span);
}

TEST_F(TextReadTest, HeapType) {
  context.features.enable_reference_types();

  OK(ReadHeapType, HT_Func, "func"_su8);
}

TEST_F(TextReadTest, HeapType_reference_types) {
  context.features.enable_reference_types();
  OK(ReadHeapType, HT_Extern, "extern"_su8);
}

TEST_F(TextReadTest, HeapType_exceptions) {
  context.features.enable_exceptions();
  OK(ReadHeapType, HT_Exn, "exn"_su8);
}

TEST_F(TextReadTest, ValueType) {
  OK(ReadValueType, VT_I32, "i32"_su8);
  OK(ReadValueType, VT_I64, "i64"_su8);
  OK(ReadValueType, VT_F32, "f32"_su8);
  OK(ReadValueType, VT_F64, "f64"_su8);

  Fail(ReadValueType, {{0, "value type v128 not allowed"}}, "v128"_su8);
  Fail(ReadValueType, {{0, "reference type funcref not allowed"}},
       "funcref"_su8);
  Fail(ReadValueType, {{0, "reference type externref not allowed"}},
       "externref"_su8);
}

TEST_F(TextReadTest, ValueType_simd) {
  context.features.enable_simd();
  OK(ReadValueType, VT_V128, "v128"_su8);
}

TEST_F(TextReadTest, ValueType_reference_types) {
  context.features.enable_reference_types();
  OK(ReadValueType, VT_Funcref, "funcref"_su8);
  OK(ReadValueType, VT_Externref, "externref"_su8);
}

TEST_F(TextReadTest, ValueType_exceptions) {
  context.features.enable_exceptions();
  OK(ReadValueType, VT_Exnref, "exnref"_su8);
}

TEST_F(TextReadTest, ValueType_function_references) {
  context.features.enable_function_references();
  OK(ReadValueType, VT_Ref0, "(ref 0)"_su8);
  OK(ReadValueType, VT_RefNull0, "(ref null 0)"_su8);
  OK(ReadValueType, VT_RefT, "(ref $t)"_su8);
  OK(ReadValueType, VT_RefNullT, "(ref null $t)"_su8);
  OK(ReadValueType, VT_RefFunc, "(ref func)"_su8);
  OK(ReadValueType, VT_RefNullFunc, "(ref null func)"_su8);
  OK(ReadValueType, VT_RefExtern, "(ref extern)"_su8);
  OK(ReadValueType, VT_RefNullExtern, "(ref null extern)"_su8);
  OK(ReadValueType, VT_Ref0, "(ref 0)"_su8);
  OK(ReadValueType, VT_RefNull0, "(ref null 0)"_su8);
  OK(ReadValueType, VT_RefT, "(ref $t)"_su8);
  OK(ReadValueType, VT_RefNullT, "(ref null $t)"_su8);
}

TEST_F(TextReadTest, ValueTypeList) {
  auto span = "i32 f32 f64 i64"_su8;
  std::vector<At<ValueType>> expected{
      MakeAt("i32"_su8, VT_I32),
      MakeAt("f32"_su8, VT_F32),
      MakeAt("f64"_su8, VT_F64),
      MakeAt("i64"_su8, VT_I64),
  };
  OKVector(ReadValueTypeList, expected, span);
}

TEST_F(TextReadTest, ReferenceType) {
  OK(ReadReferenceType, RT_Funcref, "funcref"_su8, AllowFuncref::Yes);
}

TEST_F(TextReadTest, ReferenceType_reference_types) {
  context.features.enable_reference_types();
  OK(ReadReferenceType, RT_Funcref, "funcref"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_Externref, "externref"_su8, AllowFuncref::Yes);
}

TEST_F(TextReadTest, ReferenceType_exceptions) {
  context.features.enable_exceptions();
  OK(ReadReferenceType, RT_Exnref, "exnref"_su8, AllowFuncref::Yes);
}

TEST_F(TextReadTest, ReferenceType_function_references) {
  context.features.enable_function_references();

  OK(ReadReferenceType, RT_Ref0, "(ref 0)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNull0, "(ref null 0)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefT, "(ref $t)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNullT, "(ref null $t)"_su8, AllowFuncref::Yes);

  OK(ReadReferenceType, RT_RefFunc, "(ref func)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNullFunc, "(ref null func)"_su8,
     AllowFuncref::Yes);

  OK(ReadReferenceType, RT_RefExtern, "(ref extern)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNullExtern, "(ref null extern)"_su8,
     AllowFuncref::Yes);
}

TEST_F(TextReadTest, BoundParamList) {
  auto span = "(param i32 f32) (param $foo i64) (param)"_su8;
  std::vector<At<BoundValueType>> expected{
      MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)}),
      MakeAt("f32"_su8, BVT{nullopt, MakeAt("f32"_su8, VT_F32)}),
      MakeAt("$foo i64"_su8,
             BVT{MakeAt("$foo"_su8, "$foo"_sv), MakeAt("i64"_su8, VT_I64)}),
  };

  OKVector(ReadBoundParamList, expected, span);
}

TEST_F(TextReadTest, ParamList) {
  auto span = "(param i32 f32) (param i64) (param)"_su8;
  std::vector<At<ValueType>> expected{
      MakeAt("i32"_su8, VT_I32),
      MakeAt("f32"_su8, VT_F32),
      MakeAt("i64"_su8, VT_I64),
  };
  OKVector(ReadParamList, expected, span);
}

TEST_F(TextReadTest, ResultList) {
  auto span = "(result i32 f32) (result i64) (result)"_su8;
  std::vector<At<ValueType>> expected{
      MakeAt("i32"_su8, VT_I32),
      MakeAt("f32"_su8, VT_F32),
      MakeAt("i64"_su8, VT_I64),
  };
  OKVector(ReadResultList, expected, span);
}

TEST_F(TextReadTest, LocalList) {
  auto span = "(local i32 f32) (local $foo i64) (local)"_su8;
  std::vector<At<BoundValueType>> expected{
      MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)}),
      MakeAt("f32"_su8, BVT{nullopt, MakeAt("f32"_su8, VT_F32)}),
      MakeAt("$foo i64"_su8,
             BVT{MakeAt("$foo"_su8, "$foo"_sv), MakeAt("i64"_su8, VT_I64)}),
  };

  OKVector(ReadLocalList, expected, span);
}

TEST_F(TextReadTest, TypeUseOpt) {
  OK(ReadTypeUseOpt, Var{Index{123}}, "(type 123)"_su8);
  OK(ReadTypeUseOpt, Var{"$foo"_sv}, "(type $foo)"_su8);
  OK(ReadTypeUseOpt, nullopt, ""_su8);
}

TEST_F(TextReadTest, FunctionTypeUse) {
  // Empty.
  OK(ReadFunctionTypeUse, FunctionTypeUse{}, ""_su8);

  // Type use.
  OK(ReadFunctionTypeUse,
     FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}},
     "(type 0)"_su8);

  // Function type.
  OK(ReadFunctionTypeUse,
     FunctionTypeUse{nullopt,
                     MakeAt("(param i32 f32) (result f64)"_su8,
                            FunctionType{{MakeAt("i32"_su8, VT_I32),
                                          MakeAt("f32"_su8, VT_F32)},
                                         {MakeAt("f64"_su8, VT_F64)}})},
     "(param i32 f32) (result f64)"_su8);

  // Type use and function type.
  OK(ReadFunctionTypeUse,
     FunctionTypeUse{MakeAt("(type $t)"_su8, Var{"$t"_sv}),
                     MakeAt("(result i32)"_su8,
                            FunctionType{{}, {MakeAt("i32"_su8, VT_I32)}})},
     "(type $t) (result i32)"_su8);
}

TEST_F(TextReadTest, InlineImport) {
  OK(ReadInlineImportOpt,
     InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                  MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})},
     R"((import "m" "n"))"_su8);
  OK(ReadInlineImportOpt, nullopt, ""_su8);
}

TEST_F(TextReadTest, InlineImport_AfterNonImport) {
  context.seen_non_import = true;
  Fail(ReadInlineImportOpt,
       {{1, "Imports must occur before all non-import definitions"}},
       "(import \"m\" \"n\")"_su8);
}

TEST_F(TextReadTest, InlineExport) {
  OK(ReadInlineExport, InlineExport{MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})},
     R"((export "n"))"_su8);
}

TEST_F(TextReadTest, InlineExportList) {
  OKVector(ReadInlineExportList,
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})}),
               MakeAt("(export \"n\")"_su8,
                      InlineExport{MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
           },
           R"((export "m") (export "n"))"_su8);
}


TEST_F(TextReadTest, BoundFunctionType) {
  SpanU8 span =
      "(param i32 i32) (param $t i64) (result f32 f32) (result f64)"_su8;
  OK(ReadBoundFunctionType,
     BoundFunctionType{
         {MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)}),
          MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)}),
          MakeAt("$t i64"_su8,
                 BVT{MakeAt("$t"_su8, "$t"_sv), MakeAt("i64"_su8, VT_I64)})},
         {MakeAt("f32"_su8, VT_F32), MakeAt("f32"_su8, VT_F32),
          MakeAt("f64"_su8, VT_F64)}},
     span);
}

TEST_F(TextReadTest, FunctionType) {
  SpanU8 span = "(param i32 i32) (param i64) (result f32 f32) (result f64)"_su8;
  OK(ReadFunctionType,
     FunctionType{{MakeAt("i32"_su8, VT_I32), MakeAt("i32"_su8, VT_I32),
                   MakeAt("i64"_su8, VT_I64)},
                  {MakeAt("f32"_su8, VT_F32), MakeAt("f32"_su8, VT_F32),
                   MakeAt("f64"_su8, VT_F64)}},
     span);
}

TEST_F(TextReadTest, TypeEntry) {
  OK(ReadTypeEntry, TypeEntry{nullopt, BoundFunctionType{{}, {}}},
     "(type (func))"_su8);

  OK(ReadTypeEntry,
     TypeEntry{
         MakeAt("$foo"_su8, "$foo"_sv),
         MakeAt("(param $bar i32) (result i64)"_su8,
                BoundFunctionType{
                    {MakeAt("$bar i32"_su8, BVT{MakeAt("$bar"_su8, "$bar"_sv),
                                                MakeAt("i32"_su8, VT_I32)})},
                    {MakeAt("i64"_su8, VT_I64)}})},
     "(type $foo (func (param $bar i32) (result i64)))"_su8);
}

TEST_F(TextReadTest, AlignOpt) {
  OK(ReadAlignOpt, u32{256}, "align=256"_su8);
  OK(ReadAlignOpt, u32{16}, "align=0x10"_su8);
  OK(ReadAlignOpt, nullopt, ""_su8);
}

TEST_F(TextReadTest, AlignOpt_NonPowerOfTwo) {
  Fail(ReadAlignOpt, {{0, "Alignment must be a power of two, got 3"}},
       "align=3"_su8);
}

TEST_F(TextReadTest, OffsetOpt) {
  OK(ReadOffsetOpt, u32{0}, "offset=0"_su8);
  OK(ReadOffsetOpt, u32{0x123}, "offset=0x123"_su8);
  OK(ReadOffsetOpt, nullopt, ""_su8);
}

TEST_F(TextReadTest, Limits) {
  OK(ReadLimits, Limits{MakeAt("1"_su8, 1u)}, "1"_su8);
  OK(ReadLimits, Limits{MakeAt("1"_su8, 1u), MakeAt("0x11"_su8, 17u)},
     "1 0x11"_su8);
  OK(ReadLimits,
     Limits{MakeAt("0"_su8, 0u), MakeAt("20"_su8, 20u),
            MakeAt("shared"_su8, Shared::Yes)},
     "0 20 shared"_su8);
}

TEST_F(TextReadTest, BlockImmediate) {
  // empty block type.
  OK(ReadBlockImmediate, BlockImmediate{}, ""_su8);

  // block type w/ label.
  OK(ReadBlockImmediate, BlockImmediate{MakeAt("$l"_su8, BindVar{"$l"_sv}), {}},
     "$l"_su8);

  // block type w/ function type use.
  OK(ReadBlockImmediate,
     BlockImmediate{nullopt,
                    FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}},
     "(type 0)"_su8);

  // block type w/ label and function type use.
  OK(ReadBlockImmediate,
     BlockImmediate{MakeAt("$l2"_su8, BindVar{"$l2"_sv}),
                    FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}},
     "$l2 (type 0)"_su8);
}

TEST_F(TextReadTest, BlockImmediate_InlineType) {
  OK(ReadBlockImmediate, BlockImmediate{}, ""_su8);

  struct {
    At<ValueType> value_type;
    SpanU8 span;
  } tests[] = {
      {MakeAt("i32"_su8, VT_I32), "(result i32)"_su8},
      {MakeAt("i64"_su8, VT_I64), "(result i64)"_su8},
      {MakeAt("f32"_su8, VT_F32), "(result f32)"_su8},
      {MakeAt("f64"_su8, VT_F64), "(result f64)"_su8},
  };

  for (auto& test: tests) {
    OK(ReadBlockImmediate,
       BlockImmediate{
           nullopt,
           FunctionTypeUse{
               nullopt,
               MakeAt(test.span, FunctionType{{}, {test.value_type}})}},
       test.span);
  }
}

TEST_F(TextReadTest, LetImmediate) {
  // empty let immediate.
  OK(ReadLetImmediate, LetImmediate{}, ""_su8);

  // label, no locals
  OK(ReadLetImmediate,
     LetImmediate{BlockImmediate{MakeAt("$l"_su8, BindVar{"$l"_sv}), {}}, {}},
     "$l"_su8);

  // type use, locals
  OK(ReadLetImmediate,
     LetImmediate{BlockImmediate{nullopt, FunctionTypeUse{MakeAt("(type 0)"_su8,
                                                                 Var{Index{0}}),
                                                          {}}},
                  BoundValueTypeList{MakeAt(
                      "i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)})}},
     "(type 0) (local i32)"_su8);

  // inline type, multiple locals
  OK(ReadLetImmediate,
     LetImmediate{
         BlockImmediate{
             nullopt,
             FunctionTypeUse{
                 nullopt, MakeAt("(param i32)"_su8,
                                 FunctionType{
                                     ValueTypeList{MakeAt("i32"_su8, VT_I32)},
                                     {},
                                 })}},
         BoundValueTypeList{
             MakeAt("f32"_su8, BVT{nullopt, MakeAt("f32"_su8, VT_F32)}),
             MakeAt("f64"_su8, BVT{nullopt, MakeAt("f64"_su8, VT_F64)})}},
     "(param i32) (local f32 f64)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Bare) {
  OK(ReadPlainInstruction, I{MakeAt("nop"_su8, O::Nop)}, "nop"_su8);
  OK(ReadPlainInstruction, I{MakeAt("i32.add"_su8, O::I32Add)}, "i32.add"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Var) {
  OK(ReadPlainInstruction,
     I{MakeAt("br"_su8, O::Br), MakeAt("0"_su8, Var{Index{0}})}, "br 0"_su8);
  OK(ReadPlainInstruction,
     I{MakeAt("local.get"_su8, O::LocalGet), MakeAt("$x"_su8, Var{"$x"_sv})},
     "local.get $x"_su8);
}

TEST_F(TextReadTest, PlainInstruction_BrOnExn) {
  context.features.enable_exceptions();
  OK(ReadPlainInstruction,
     I{MakeAt("br_on_exn"_su8, O::BrOnExn),
       MakeAt("$l $e"_su8, BrOnExnImmediate{MakeAt("$l"_su8, Var{"$l"_sv}),
                                            MakeAt("$e"_su8, Var{"$e"_sv})})},
     "br_on_exn $l $e"_su8);
}

TEST_F(TextReadTest, PlainInstruction_BrTable) {
  // br_table w/ only default target.
  OK(ReadPlainInstruction,
     I{MakeAt("br_table"_su8, O::BrTable),
       MakeAt("0"_su8, BrTableImmediate{{}, MakeAt("0"_su8, Var{Index{0}})})},
     "br_table 0"_su8);

  // br_table w/ targets and default target.
  OK(ReadPlainInstruction,
     I{MakeAt("br_table"_su8, O::BrTable),
       MakeAt("0 1 $a $b"_su8,
              BrTableImmediate{{MakeAt("0"_su8, Var{Index{0}}),
                                MakeAt("1"_su8, Var{Index{1}}),
                                MakeAt("$a"_su8, Var{"$a"_sv})},
                               MakeAt("$b"_su8, Var{"$b"_sv})})},
     "br_table 0 1 $a $b"_su8);
}

TEST_F(TextReadTest, PlainInstruction_BrTable_NoVars) {
  // br_table w/ no vars
  Fail(ReadPlainInstruction, {{8, "Expected a variable, got Eof"}},
       "br_table"_su8);
}

TEST_F(TextReadTest, PlainInstruction_CallIndirect) {
  // Bare call_indirect.
  OK(ReadPlainInstruction,
     I{MakeAt("call_indirect"_su8, O::CallIndirect),
       MakeAt(""_su8, CallIndirectImmediate{})},
     "call_indirect"_su8);

  // call_indirect w/ function type use.
  OK(ReadPlainInstruction,
     I{MakeAt("call_indirect"_su8, O::CallIndirect),
       MakeAt("(type 0)"_su8,
              CallIndirectImmediate{
                  nullopt,
                  FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}})},
     "call_indirect (type 0)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_CallIndirect_reference_types) {
  // In the reference types proposal, the call_indirect instruction also allows
  // a table var first.
  context.features.enable_reference_types();

  // call_indirect w/ table.
  OK(ReadPlainInstruction,
     I{MakeAt("call_indirect"_su8, O::CallIndirect),
       MakeAt("$t"_su8,
              CallIndirectImmediate{MakeAt("$t"_su8, Var{"$t"_sv}), {}})},
     "call_indirect $t"_su8);

  // call_indirect w/ table and type use.
  OK(ReadPlainInstruction,
     I{MakeAt("call_indirect"_su8, O::CallIndirect),
       MakeAt("0 (type 0)"_su8,
              CallIndirectImmediate{
                  MakeAt("0"_su8, Var{Index{0}}),
                  FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}})},
     "call_indirect 0 (type 0)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Const) {
  // i32.const
  OK(ReadPlainInstruction,
     I{MakeAt("i32.const"_su8, O::I32Const), MakeAt("12"_su8, s32{12})},
     "i32.const 12"_su8);

  // i64.const
  OK(ReadPlainInstruction,
     I{MakeAt("i64.const"_su8, O::I64Const), MakeAt("34"_su8, s64{34})},
     "i64.const 34"_su8);

  // f32.const
  OK(ReadPlainInstruction,
     I{MakeAt("f32.const"_su8, O::F32Const), MakeAt("56"_su8, f32{56})},
     "f32.const 56"_su8);

  // f64.const
  OK(ReadPlainInstruction,
     I{MakeAt("f64.const"_su8, O::F64Const), MakeAt("78"_su8, f64{78})},
     "f64.const 78"_su8);
}

TEST_F(TextReadTest, PlainInstruction_MemArg) {
  // No align, no offset.
  OK(ReadPlainInstruction,
     I{MakeAt("i32.load"_su8, O::I32Load),
       MakeAt(""_su8, MemArgImmediate{nullopt, nullopt})},
     "i32.load"_su8);

  // No align, offset.
  OK(ReadPlainInstruction,
     I{MakeAt("f32.load"_su8, O::F32Load),
       MakeAt("offset=12"_su8,
              MemArgImmediate{nullopt, MakeAt("offset=12"_su8, u32{12})})},
     "f32.load offset=12"_su8);

  // Align, no offset.
  OK(ReadPlainInstruction,
     I{MakeAt("i32.load8_u"_su8, O::I32Load8U),
       MakeAt("align=16"_su8,
              MemArgImmediate{MakeAt("align=16"_su8, u32{16}), nullopt})},
     "i32.load8_u align=16"_su8);

  // Align and offset.
  OK(ReadPlainInstruction,
     I{MakeAt("f64.store"_su8, O::F64Store),
       MakeAt("offset=123 align=32"_su8,
              MemArgImmediate{MakeAt("align=32"_su8, u32{32}),
                              MakeAt("offset=123"_su8, u32{123})})},
     "f64.store offset=123 align=32"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Select) {
  OK(ReadPlainInstruction,
     I{MakeAt("select"_su8, O::Select), MakeAt(""_su8, SelectImmediate{})},
     "select"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Select_reference_types) {
  context.features.enable_reference_types();

  // select w/o types
  OK(ReadPlainInstruction,
     I{MakeAt("select"_su8, O::Select), MakeAt(""_su8, SelectImmediate{})},
     "select"_su8);

  // select w/ one type
  OK(ReadPlainInstruction,
     I{MakeAt("select"_su8, O::SelectT),
       MakeAt("(result i32)"_su8, SelectImmediate{MakeAt("i32"_su8, VT_I32)})},
     "select (result i32)"_su8);

  // select w/ multiple types
  OK(ReadPlainInstruction,
     I{MakeAt("select"_su8, O::SelectT),
       MakeAt("(result i32) (result i64)"_su8,
              SelectImmediate{MakeAt("i32"_su8, VT_I32),
                              MakeAt("i64"_su8, VT_I64)})},
     "select (result i32) (result i64)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_SimdConst) {
  Fail(ReadPlainInstruction, {{0, "v128.const instruction not allowed"}},
       "v128.const i32x4 0 0 0 0"_su8);

  context.features.enable_simd();

  // i8x16
  OK(ReadPlainInstruction,
     I{MakeAt("v128.const"_su8, O::V128Const),
       MakeAt("0 1 2 3 4 5 6 7 8 9 0xa 0xb 0xc 0xd 0xe 0xf"_su8,
              v128{u8x16{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe,
                         0xf}})},
     "v128.const i8x16 0 1 2 3 4 5 6 7 8 9 0xa 0xb 0xc 0xd 0xe 0xf"_su8);

  // i16x8
  OK(ReadPlainInstruction,
     I{MakeAt("v128.const"_su8, O::V128Const),
       MakeAt("0 1 2 3 4 5 6 7"_su8, v128{u16x8{0, 1, 2, 3, 4, 5, 6, 7}})},
     "v128.const i16x8 0 1 2 3 4 5 6 7"_su8);

  // i32x4
  OK(ReadPlainInstruction,
     I{MakeAt("v128.const"_su8, O::V128Const),
       MakeAt("0 1 2 3"_su8, v128{u32x4{0, 1, 2, 3}})},
     "v128.const i32x4 0 1 2 3"_su8);

  // i64x2
  OK(ReadPlainInstruction,
     I{MakeAt("v128.const"_su8, O::V128Const),
       MakeAt("0 1"_su8, v128{u64x2{0, 1}})},
     "v128.const i64x2 0 1"_su8);

  // f32x4
  OK(ReadPlainInstruction,
     I{MakeAt("v128.const"_su8, O::V128Const),
       MakeAt("0 1 2 3"_su8, v128{f32x4{0, 1, 2, 3}})},
     "v128.const f32x4 0 1 2 3"_su8);

  // f64x2
  OK(ReadPlainInstruction,
     I{MakeAt("v128.const"_su8, O::V128Const),
       MakeAt("0 1"_su8, v128{f64x2{0, 1}})},
     "v128.const f64x2 0 1"_su8);
}

TEST_F(TextReadTest, PlainInstruction_SimdLane) {
  Fail(ReadPlainInstruction,
       {{0, "i8x16.extract_lane_s instruction not allowed"}},
       "i8x16.extract_lane_s 0"_su8);

  context.features.enable_simd();

  OK(ReadPlainInstruction,
     I{MakeAt("i8x16.extract_lane_s"_su8, O::I8X16ExtractLaneS),
       MakeAt("9"_su8, SimdLaneImmediate{9})},
     "i8x16.extract_lane_s 9"_su8);
  OK(ReadPlainInstruction,
     I{MakeAt("f32x4.replace_lane"_su8, O::F32X4ReplaceLane),
       MakeAt("3"_su8, SimdLaneImmediate{3})},
     "f32x4.replace_lane 3"_su8);
}

TEST_F(TextReadTest, InvalidSimdLane) {
  Fail(ReadSimdLane, {{0, "Expected a positive integer, got Int"}},
       "-1"_su8);
  Fail(ReadSimdLane, {{0, "Invalid integer, got Nat"}}, "256"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Shuffle) {
  Fail(ReadPlainInstruction, {{0, "v8x16.shuffle instruction not allowed"}},
       "v8x16.shuffle 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_su8);

  context.features.enable_simd();

  OK(ReadPlainInstruction,
     I{MakeAt("v8x16.shuffle"_su8, O::V8X16Shuffle),
       MakeAt("0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_su8, ShuffleImmediate{})},
     "v8x16.shuffle 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_su8);
}

TEST_F(TextReadTest, PlainInstruction_MemoryCopy) {
  Fail(ReadPlainInstruction, {{0, "memory.copy instruction not allowed"}},
       "memory.copy"_su8);

  // memory.copy w/o dst and src.
  context.features.enable_bulk_memory();
  OK(ReadPlainInstruction,
     I{MakeAt("memory.copy"_su8, O::MemoryCopy), MakeAt(CopyImmediate{})},
     "memory.copy"_su8);
}

TEST_F(TextReadTest, PlainInstruction_MemoryInit) {
  Fail(ReadPlainInstruction, {{0, "memory.init instruction not allowed"}},
       "memory.init 0"_su8);

  context.features.enable_bulk_memory();

  // memory.init w/ just segment index.
  OK(ReadPlainInstruction,
     I{MakeAt("memory.init"_su8, O::MemoryInit),
       MakeAt("2"_su8, InitImmediate{MakeAt("2"_su8, Var{Index{2}}), nullopt})},
     "memory.init 2"_su8);
}

TEST_F(TextReadTest, PlainInstruction_TableCopy) {
  Fail(ReadPlainInstruction, {{0, "table.copy instruction not allowed"}},
       "table.copy"_su8);

  // table.copy w/o dst and src.
  context.features.enable_bulk_memory();
  OK(ReadPlainInstruction,
     I{MakeAt("table.copy"_su8, O::TableCopy), MakeAt(""_su8, CopyImmediate{})},
     "table.copy"_su8);
}

TEST_F(TextReadTest, PlainInstruction_TableCopy_reference_types) {
  context.features.enable_reference_types();

  // table.copy w/o dst and src.
  OK(ReadPlainInstruction,
     I{MakeAt("table.copy"_su8, O::TableCopy), MakeAt(""_su8, CopyImmediate{})},
     "table.copy"_su8);

  // table.copy w/ dst and src
  OK(ReadPlainInstruction,
     I{MakeAt("table.copy"_su8, O::TableCopy),
       MakeAt("$d $s"_su8, CopyImmediate{MakeAt("$d"_su8, Var{"$d"_sv}),
                                         MakeAt("$s"_su8, Var{"$s"_sv})})},
     "table.copy $d $s"_su8);
}

TEST_F(TextReadTest, PlainInstruction_TableInit) {
  Fail(ReadPlainInstruction, {{0, "table.init instruction not allowed"}},
       "table.init 0"_su8);

  context.features.enable_bulk_memory();

  // table.init w/ segment index and table index.
  OK(ReadPlainInstruction,
     I{MakeAt("table.init"_su8, O::TableInit),
       MakeAt("$t $e"_su8, InitImmediate{MakeAt("$e"_su8, Var{"$e"_sv}),
                                         MakeAt("$t"_su8, Var{"$t"_sv})})},
     "table.init $t $e"_su8);

  // table.init w/ just segment index.
  OK(ReadPlainInstruction,
     I{MakeAt("table.init"_su8, O::TableInit),
       MakeAt("2"_su8, InitImmediate{MakeAt("2"_su8, Var{Index{2}}), nullopt})},
     "table.init 2"_su8);
}

TEST_F(TextReadTest, PlainInstruction_RefNull) {
  Fail(ReadPlainInstruction, {{0, "ref.null instruction not allowed"}},
       "ref.null extern"_su8);

  context.features.enable_reference_types();

  OK(ReadPlainInstruction,
     I{MakeAt("ref.null"_su8, O::RefNull), MakeAt("extern"_su8, HT_Extern)},
     "ref.null extern"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Block) {
  // Empty block.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("block"_su8,
                      I{MakeAt("block"_su8, O::Block), BlockImmediate{}}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "block end"_su8);

  // block w/ multiple instructions.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("block"_su8,
                      I{MakeAt("block"_su8, O::Block), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "block nop nop end"_su8);

  // Block w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("block $l"_su8,
                      I{MakeAt("block"_su8, O::Block),
                        MakeAt("$l"_su8,
                               BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "block $l nop end"_su8);

  // Block w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("block $l2"_su8,
                 I{MakeAt("block"_su8, O::Block),
                   MakeAt("$l2"_su8,
                          BlockImmediate{MakeAt("$l2"_su8, "$l2"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "block $l2 nop end $l2"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Block_MismatchedLabels) {
  Fail(ReadBlockInstruction_ForTesting, {{10, "Unexpected label $l2"}},
       "block end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{13, "Expected label $l, got $l2"}},
       "block $l end $l2"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Loop) {
  // Empty loop.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("loop"_su8, I{MakeAt("loop"_su8, O::Loop), BlockImmediate{}}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "loop end"_su8);

  // loop w/ multiple instructions.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("loop"_su8, I{MakeAt("loop"_su8, O::Loop), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "loop nop nop end"_su8);

  // Loop w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("loop $l"_su8,
                      I{MakeAt("loop"_su8, O::Loop),
                        MakeAt("$l"_su8,
                               BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "loop $l nop end"_su8);

  // Loop w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("loop $l2"_su8,
                 I{MakeAt("loop"_su8, O::Loop),
                   MakeAt("$l2"_su8,
                          BlockImmediate{MakeAt("$l2"_su8, "$l2"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "loop $l2 nop end $l2"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Loop_MismatchedLabels) {
  Fail(ReadBlockInstruction_ForTesting, {{9, "Unexpected label $l2"}},
       "loop end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{12, "Expected label $l, got $l2"}},
       "loop $l end $l2"_su8);
}

TEST_F(TextReadTest, BlockInstruction_If) {
  // Empty if.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "if end"_su8);

  // if w/ non-empty block.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "if nop nop end"_su8);

  // if, w/ else.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "if else end"_su8);

  // if, w/ else and non-empty blocks.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "if nop nop else nop nop end"_su8);

  // If w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("if $l"_su8,
                      I{MakeAt("if"_su8, O::If),
                        MakeAt("$l"_su8,
                               BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "if $l nop end"_su8);

  // If w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if $l2"_su8,
                 I{MakeAt("if"_su8, O::If),
                   MakeAt("$l2"_su8,
                          BlockImmediate{MakeAt("$l2"_su8, "$l2"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if $l2 nop end $l2"_su8);

  // If w/ label and matching else and end labels.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if $l3"_su8,
                 I{MakeAt("if"_su8, O::If),
                   MakeAt("$l3"_su8,
                          BlockImmediate{MakeAt("$l3"_su8, "$l3"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if $l3 nop else $l3 nop end $l3"_su8);
}

TEST_F(TextReadTest, BlockInstruction_If_MismatchedLabels) {
  Fail(ReadBlockInstruction_ForTesting, {{7, "Unexpected label $l2"}},
       "if end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{8, "Unexpected label $l2"}},
       "if else $l2 end"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{10, "Expected label $l, got $l2"}},
       "if $l end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{11, "Expected label $l, got $l2"}},
       "if $l else $l2 end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{11, "Expected label $l, got $l2"}},
       "if $l else $l2 end $l"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Try) {
  Fail(ReadBlockInstruction_ForTesting, {{0, "try instruction not allowed"}},
       "try catch end"_su8);

  context.features.enable_exceptions();

  // try/catch.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "try catch end"_su8);

  // try/catch and non-empty blocks.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "try nop nop catch nop nop end"_su8);

  // try w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               MakeAt("try $l"_su8,
                      I{MakeAt("try"_su8, O::Try),
                        MakeAt("$l"_su8,
                               BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "try $l nop catch nop end"_su8);

  // try w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("try $l2"_su8,
                 I{MakeAt("try"_su8, O::Try),
                   MakeAt("$l2"_su8,
                          BlockImmediate{MakeAt("$l2"_su8, "$l2"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "try $l2 nop catch nop end $l2"_su8);

  // try w/ label and matching catch and end labels.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("try $l3"_su8,
                 I{MakeAt("try"_su8, O::Try),
                   MakeAt("$l3"_su8,
                          BlockImmediate{MakeAt("$l3"_su8, "$l3"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "try $l3 nop catch $l3 nop end $l3"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Try_MismatchedLabels) {
  context.features.enable_exceptions();

  Fail(ReadBlockInstruction_ForTesting, {{14, "Unexpected label $l2"}},
       "try catch end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{10, "Unexpected label $l2"}},
       "try catch $l2 end"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{17, "Expected label $l, got $l2"}},
       "try $l catch end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{13, "Expected label $l, got $l2"}},
       "try $l catch $l2 end $l2"_su8);
  Fail(ReadBlockInstruction_ForTesting, {{13, "Expected label $l, got $l2"}},
       "try $l catch $l2 end $l"_su8);
}

TEST_F(TextReadTest, LetInstruction) {
  // Empty Let.
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               MakeAt("let"_su8, I{MakeAt("let"_su8, O::Let), LetImmediate{}}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "let end"_su8);

  // Let w/ multiple instructions.
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               MakeAt("let"_su8,
                      I{MakeAt("let"_su8, O::Let), LetImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "let nop nop end"_su8);

  // Let w/ label.
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               MakeAt("let $l"_su8,
                      I{MakeAt("let"_su8, O::Let),
                        MakeAt("$l"_su8,
                               LetImmediate{BlockImmediate{
                                                MakeAt("$l"_su8, "$l"_sv), {}},
                                            {}})}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "let $l nop end"_su8);

  // Let w/ label and matching end label.
  OKVector(
      ReadLetInstruction_ForTesting,
      InstructionList{
          MakeAt("let $l2"_su8,
                 I{MakeAt("let"_su8, O::Let),
                   MakeAt("$l2"_su8,
                          LetImmediate{
                              BlockImmediate{MakeAt("$l2"_su8, "$l2"_sv), {}},
                              {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "let $l2 nop end $l2"_su8);

  // Let w/ locals
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               MakeAt("let (local i32)"_su8,
                      I{MakeAt("let"_su8, O::Let),
                        MakeAt("(local i32)"_su8,
                               LetImmediate{
                                   BlockImmediate{},
                                   {MakeAt("i32"_su8,
                                           BVT{nullopt,
                                               MakeAt("i32"_su8, VT_I32)})}})}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
           },
           "let (local i32) nop end"_su8);

  // Let w/ params, results, locals
  OKVector(
      ReadLetInstruction_ForTesting,
      InstructionList{
          MakeAt(
              "let (param f32) (result f64) (local i32)"_su8,
              I{MakeAt("let"_su8, O::Let),
                MakeAt(
                    "(param f32) (result f64) (local i32)"_su8,
                    LetImmediate{
                        BlockImmediate{
                            nullopt,
                            FunctionTypeUse{
                                nullopt,
                                MakeAt("(param f32) (result f64)"_su8,
                                       FunctionType{
                                           {MakeAt("f32"_su8, VT_F32)},
                                           {MakeAt("f64"_su8, VT_F64)}})}},
                        {MakeAt("i32"_su8,
                                BVT{nullopt, MakeAt("i32"_su8, VT_I32)})}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "let (param f32) (result f64) (local i32) nop end"_su8);
}

TEST_F(TextReadTest, Label_ReuseNames) {
  OK(ReadInstructionList_ForTesting,
     InstructionList{
         MakeAt(
             "block $l"_su8,
             I{MakeAt("block"_su8, O::Block),
               MakeAt("$l"_su8,
                      BlockImmediate{MakeAt("$l"_su8, BindVar{"$l"_sv}), {}})}),
         MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
         MakeAt(
             "block $l"_su8,
             I{MakeAt("block"_su8, O::Block),
               MakeAt("$l"_su8,
                      BlockImmediate{MakeAt("$l"_su8, BindVar{"$l"_sv}), {}})}),
         MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
     },
     "block $l end block $l end"_su8);
}

TEST_F(TextReadTest, Label_DuplicateNames) {
  OK(ReadInstructionList_ForTesting,
     InstructionList{
         MakeAt("block $b"_su8,
                I{MakeAt("block"_su8, O::Block),
                  MakeAt("$b"_su8,
                         BlockImmediate{MakeAt("$b"_su8, "$b"_sv), {}})}),
         MakeAt("block $b"_su8,
                I{MakeAt("block"_su8, O::Block),
                  MakeAt("$b"_su8,
                         BlockImmediate{MakeAt("$b"_su8, "$b"_sv), {}})}),
         MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
         MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
     },
     "block $b block $b end end"_su8);
}

auto ReadExpression_ForTesting(Tokenizer& tokenizer, Context& context)
    -> optional<InstructionList> {
  InstructionList result;
  WASP_TRY(ReadExpression(tokenizer, context, result));
  return result;
}

TEST_F(TextReadTest, Expression_Plain) {
  // No immediates.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
           },
           "(nop)"_su8);

  // BrTable immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("br_table 0 0 0"_su8,
                 I{MakeAt("br_table"_su8, O::BrTable),
                   MakeAt("0 0 0"_su8,
                          BrTableImmediate{{MakeAt("0"_su8, Var{Index{0}}),
                                            MakeAt("0"_su8, Var{Index{0}})},
                                           MakeAt("0"_su8, Var{Index{0}})})}),
      },
      "(br_table 0 0 0)"_su8);

  // CallIndirect immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("call_indirect (type 0)"_su8,
                 I{MakeAt("call_indirect"_su8, O::CallIndirect),
                   MakeAt("(type 0)"_su8,
                          CallIndirectImmediate{
                              nullopt, FunctionTypeUse{MakeAt("(type 0)"_su8,
                                                              Var{Index{0}}),
                                                       {}}})}),
      },
      "(call_indirect (type 0))"_su8);

  // f32 immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("f32.const 1.0"_su8, I{MakeAt("f32.const"_su8, O::F32Const),
                                        MakeAt("1.0"_su8, f32{1.0f})}),
      },
      "(f32.const 1.0)"_su8);

  // f64 immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("f64.const 2.0"_su8, I{MakeAt("f64.const"_su8, O::F64Const),
                                        MakeAt("2.0"_su8, f64{2.0})}),
      },
      "(f64.const 2.0)"_su8);

  // i32 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("i32.const 3"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                           MakeAt("3"_su8, s32{3})}),
           },
           "(i32.const 3)"_su8);

  // i64 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("i64.const 4"_su8, I{MakeAt("i64.const"_su8, O::I64Const),
                                           MakeAt("4"_su8, s64{4})}),
           },
           "(i64.const 4)"_su8);

  // MemArg immediate
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("i32.load align=1"_su8,
                      I{MakeAt("i32.load"_su8, O::I32Load),
                        MakeAt("align=1"_su8,
                               MemArgImmediate{MakeAt("align=1"_su8, u32{1}),
                                               nullopt})}),
           },
           "(i32.load align=1)"_su8);

  // Var immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("br 0"_su8, I{MakeAt("br"_su8, O::Br),
                                    MakeAt("0"_su8, Var{Index{0}})}),
           },
           "(br 0)"_su8);
}

TEST_F(TextReadTest, Expression_Plain_exceptions) {
  Fail(ReadExpression_ForTesting, {{1, "br_on_exn instruction not allowed"}},
       "(br_on_exn 0 0)"_su8);

  context.features.enable_exceptions();

  // BrOnExn immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("br_on_exn 0 0"_su8,
                 I{MakeAt("br_on_exn"_su8, O::BrOnExn),
                   MakeAt("0 0"_su8,
                          BrOnExnImmediate{MakeAt("0"_su8, Var{Index{0}}),
                                           MakeAt("0"_su8, Var{Index{0}})})}),
      },
      "(br_on_exn 0 0)"_su8);
}

TEST_F(TextReadTest, Expression_Plain_simd) {
  Fail(ReadExpression_ForTesting, {{1, "v128.const instruction not allowed"}},
       "(v128.const i32x4 0 0 0 0)"_su8);

  context.features.enable_simd();

  // v128 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("v128.const i32x4 0 0 0 0"_su8,
                      I{MakeAt("v128.const"_su8, O::V128Const),
                        MakeAt("0 0 0 0"_su8, v128{u32x4{0, 0, 0, 0}})}),
           },
           "(v128.const i32x4 0 0 0 0)"_su8);

  // FeaturesSimd lane immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("f32x4.replace_lane 3"_su8,
                      I{MakeAt("f32x4.replace_lane"_su8, O::F32X4ReplaceLane),
                        MakeAt("3"_su8, SimdLaneImmediate{3})}),
           },
           "(f32x4.replace_lane 3)"_su8);
}

TEST_F(TextReadTest, Expression_Plain_bulk_memory) {
  Fail(ReadExpression_ForTesting, {{1, "table.init instruction not allowed"}},
       "(table.init 0)"_su8);

  context.features.enable_bulk_memory();

  // Init immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("table.init 0"_su8,
                 I{MakeAt("table.init"_su8, O::TableInit),
                   MakeAt("0"_su8, InitImmediate{MakeAt("0"_su8, Var{Index{0}}),
                                                 nullopt})}),
      },
      "(table.init 0)"_su8);

  // Copy immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("table.copy"_su8,
                 I{MakeAt("table.copy"_su8, O::TableCopy), CopyImmediate{}}),
      },
      "(table.copy)"_su8);
}


TEST_F(TextReadTest, Expression_PlainFolded) {
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("i32.const 0"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                           MakeAt("0"_su8, s32{0})}),
               MakeAt("i32.add"_su8, I{MakeAt("i32.add"_su8, O::I32Add)}),
           },
           "(i32.add (i32.const 0))"_su8);

  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("i32.const 0"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                           MakeAt("0"_su8, s32{0})}),
               MakeAt("i32.const 1"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                           MakeAt("1"_su8, s32{1})}),
               MakeAt("i32.add"_su8, I{MakeAt("i32.add"_su8, O::I32Add)}),
           },
           "(i32.add (i32.const 0) (i32.const 1))"_su8);
}

TEST_F(TextReadTest, Expression_Block) {
  // Block.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("block"_su8,
                      I{MakeAt("block"_su8, O::Block), BlockImmediate{}}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(block)"_su8);

  // Loop.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("loop"_su8, I{MakeAt("loop"_su8, O::Loop), BlockImmediate{}}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(loop)"_su8);
}

TEST_F(TextReadTest, Expression_If) {
  // If then.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(if (then))"_su8);

  // If then w/ nops.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(if (then (nop)))"_su8);

  // If condition then.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(if (nop) (then (nop)))"_su8);

  // If then else.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(if (then (nop)) (else (nop)))"_su8);

  // If condition then else.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(if (nop) (then (nop)) (else (nop)))"_su8);
}

TEST_F(TextReadTest, Expression_IfNoThen) {
  Fail(ReadExpression_ForTesting, {{15, "Expected '(' Then, got Rpar Eof"}},
       "(if (nop) (nop))"_su8);
}

TEST_F(TextReadTest, Expression_IfBadElse) {
  Fail(ReadExpression_ForTesting, {{18, "Expected Else, got Func"}},
       "(if (nop) (then) (func))"_su8);
}

TEST_F(TextReadTest, Expression_Try) {
  Fail(ReadExpression_ForTesting, {{1, "try instruction not allowed"}},
       "(try (catch))"_su8);

  context.features.enable_exceptions();

  // Try catch.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(try (catch))"_su8);

  // Try catch w/ nops.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(try (nop) (catch (nop)))"_su8);
}

TEST_F(TextReadTest, Expression_Let) {
  Fail(ReadExpression_ForTesting, {{1, "let instruction not allowed"}},
       "(let)"_su8);

  context.features.enable_function_references();

  // Empty Let.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               MakeAt("let"_su8, I{MakeAt("let"_su8, O::Let), LetImmediate{}}),
               MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
           },
           "(let)"_su8);

  // Let with locals and nops.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("let (local i32 i64)"_su8,
                 I{MakeAt("let"_su8, O::Let),
                   MakeAt("(local i32 i64)"_su8,
                          LetImmediate{
                              BlockImmediate{},
                              {MakeAt("i32"_su8,
                                      BVT{nullopt, MakeAt("i32"_su8, VT_I32)}),
                               MakeAt("i64"_su8,
                                      BVT{nullopt, MakeAt("i64"_su8, VT_I64)})},
                          })}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(let (local i32 i64) nop nop)"_su8);
}


TEST_F(TextReadTest, ExpressionList) {
  OKVector(ReadExpressionList_ForTesting,
           InstructionList{
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
               MakeAt("drop"_su8, I{MakeAt("drop"_su8, O::Drop)}),
           },
           "(nop) (drop (nop))"_su8);
}

TEST_F(TextReadTest, TableType) {
  OK(ReadTableType,
     TableType{MakeAt("1 2"_su8,
                      Limits{MakeAt("1"_su8, u32{1}), MakeAt("2"_su8, u32{2})}),
               MakeAt("funcref"_su8, RT_Funcref)},
     "1 2 funcref"_su8);
}

TEST_F(TextReadTest, MemoryType) {
  OK(ReadMemoryType,
     MemoryType{MakeAt(
         "1 2"_su8, Limits{MakeAt("1"_su8, u32{1}), MakeAt("2"_su8, u32{2})})},
     "1 2"_su8);
}

TEST_F(TextReadTest, GlobalType) {
  OK(ReadGlobalType,
     GlobalType{MakeAt("i32"_su8, MakeAt("i32"_su8, VT_I32)),
                Mutability::Const},
     "i32"_su8);

  OK(ReadGlobalType,
     GlobalType{MakeAt("(mut i32)"_su8, MakeAt("i32"_su8, VT_I32)),
                MakeAt("mut"_su8, Mutability::Var)},
     "(mut i32)"_su8);
}

TEST_F(TextReadTest, EventType) {
  // Empty event type.
  OK(ReadEventType, EventType{EventAttribute::Exception, {}}, ""_su8);

  // Function type use.
  OK(ReadEventType,
     EventType{EventAttribute::Exception,
               FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}},
     "(type 0)"_su8);
}

TEST_F(TextReadTest, Function) {
  // Empty func.
  OK(ReadFunction,
     Function{{}, {}, {MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)})}, {}},
     "(func)"_su8);

  // Name.
  OK(ReadFunction,
     Function{FunctionDesc{MakeAt("$f"_su8, "$f"_sv), nullopt, {}},
              {},
              {MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)})},
              {}},
     "(func $f)"_su8);

  // Inline export.
  OK(ReadFunction,
     Function{{},
              {},
              {MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)})},
              InlineExportList{MakeAt(
                  "(export \"e\")"_su8,
                  InlineExport{MakeAt("\"e\""_su8, Text{"\"e\""_sv, 1})})}},
     "(func (export \"e\"))"_su8);

  // Locals.
  OK(ReadFunction,
     Function{{},
              BoundValueTypeList{
                  MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)}),
                  MakeAt("i64"_su8, BVT{nullopt, MakeAt("i64"_su8, VT_I64)}),
              },
              {MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)})},
              {}},
     "(func (local i32 i64))"_su8);

  // Instructions.
  OK(ReadFunction,
     Function{{},
              {},
              InstructionList{
                  MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                  MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                  MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                  MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
              },
              {}},
     "(func nop nop nop)"_su8);

  // Everything for defined Function.
  OK(ReadFunction,
     Function{FunctionDesc{MakeAt("$f2"_su8, "$f2"_sv), nullopt, {}},
              BoundValueTypeList{
                  MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT_I32)})},
              InstructionList{
                  MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                  MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
              },
              InlineExportList{MakeAt(
                  "(export \"m\")"_su8,
                  InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(func $f2 (export \"m\") (local i32) nop)"_su8);
}

TEST_F(TextReadTest, FunctionInlineImport) {
  // Import.
  OK(ReadFunction,
     Function{{},
              MakeAt("(import \"m\" \"n\")"_su8,
                     InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                  MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
              {}},
     "(func (import \"m\" \"n\"))"_su8);

  // Everything for imported Function.
  OK(ReadFunction,
     Function{FunctionDesc{
                  MakeAt("$f"_su8, "$f"_sv), nullopt,
                  MakeAt("(param i32)"_su8,
                         BoundFunctionType{
                             {MakeAt("i32"_su8,
                                     BVT{nullopt, MakeAt("i32"_su8, VT_I32)})},
                             {}})},
              MakeAt("(import \"a\" \"b\")"_su8,
                     InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                                  MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
              InlineExportList{MakeAt(
                  "(export \"m\")"_su8,
                  InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(func $f (export \"m\") (import \"a\" \"b\") (param i32))"_su8);
}

TEST_F(TextReadTest, Table) {
  // Simplest table.
  OK(ReadTable,
     Table{TableDesc{{},
                     MakeAt("0 funcref"_su8,
                            TableType{MakeAt("0"_su8,
                                             Limits{MakeAt("0"_su8, u32{0})}),
                                      MakeAt("funcref"_su8, RT_Funcref)})},
           {}},
     "(table 0 funcref)"_su8);

  // Name.
  OK(ReadTable,
     Table{TableDesc{MakeAt("$t"_su8, "$t"_sv),
                     MakeAt("0 funcref"_su8,
                            TableType{MakeAt("0"_su8,
                                             Limits{MakeAt("0"_su8, u32{0})}),
                                      MakeAt("funcref"_su8, RT_Funcref)})},
           {}},
     "(table $t 0 funcref)"_su8);

  // Inline export.
  OK(ReadTable,
     Table{TableDesc{{},
                     MakeAt("0 funcref"_su8,
                            TableType{MakeAt("0"_su8,
                                             Limits{MakeAt("0"_su8, u32{0})}),
                                      MakeAt("funcref"_su8, RT_Funcref)})},
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(table (export \"m\") 0 funcref)"_su8);

  // Name and inline export.
  OK(ReadTable,
     Table{TableDesc{MakeAt("$t2"_su8, "$t2"_sv),
                     MakeAt("0 funcref"_su8,
                            TableType{MakeAt("0"_su8,
                                             Limits{MakeAt("0"_su8, u32{0})}),
                                      MakeAt("funcref"_su8, RT_Funcref)})},
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(table $t2 (export \"m\") 0 funcref)"_su8);

  // Inline element var list.
  OK(ReadTable,
     Table{TableDesc{{},
                     TableType{Limits{u32{3}, u32{3}},
                               MakeAt("funcref"_su8, RT_Funcref)}},
           {},
           ElementListWithVars{ExternalKind::Function,
                               VarList{
                                   MakeAt("0"_su8, Var{Index{0}}),
                                   MakeAt("1"_su8, Var{Index{1}}),
                                   MakeAt("2"_su8, Var{Index{2}}),
                               }}},
     "(table funcref (elem 0 1 2))"_su8);
}

TEST_F(TextReadTest, TableInlineImport) {
  // Inline import.
  OK(ReadTable,
     Table{TableDesc{{},
                     MakeAt("0 funcref"_su8,
                            TableType{MakeAt("0"_su8,
                                             Limits{MakeAt("0"_su8, u32{0})}),
                                      MakeAt("funcref"_su8, RT_Funcref)})},
           MakeAt("(import \"m\" \"n\")"_su8,
                  InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                               MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
           {}},
     "(table (import \"m\" \"n\") 0 funcref)"_su8);

  // Everything for Table import.
  OK(ReadTable,
     Table{TableDesc{MakeAt("$t"_su8, "$t"_sv),
                     MakeAt("0 funcref"_su8,
                            TableType{MakeAt("0"_su8,
                                             Limits{MakeAt("0"_su8, u32{0})}),
                                      MakeAt("funcref"_su8, RT_Funcref)})},
           MakeAt("(import \"a\" \"b\")"_su8,
                  InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                               MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(table $t (export \"m\") (import \"a\" \"b\") 0 funcref)"_su8);
}

TEST_F(TextReadTest, Table_bulk_memory) {
  Fail(ReadTable, {{21, "Expected Rpar, got Lpar"}},
       "(table funcref (elem (nop)))"_su8);

  context.features.enable_bulk_memory();

  // Inline element var list.
  OK(ReadTable,
     Table{TableDesc{{},
                     TableType{Limits{u32{2}, u32{2}},
                               MakeAt("funcref"_su8, RT_Funcref)}},
           {},
           ElementListWithExpressions{
               MakeAt("funcref"_su8, RT_Funcref),
               ElementExpressionList{
                   MakeAt("(nop)"_su8,
                          ElementExpression{
                              MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
                   MakeAt("(nop)"_su8,
                          ElementExpression{
                              MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
               }}},
     "(table funcref (elem (nop) (nop)))"_su8);
}

TEST_F(TextReadTest, Memory) {
  // Simplest memory.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       MakeAt("0"_su8,
                              MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
            {}},
     "(memory 0)"_su8);

  // Name.
  OK(ReadMemory,
     Memory{MemoryDesc{MakeAt("$m"_su8, "$m"_sv),
                       MakeAt("0"_su8,
                              MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
            {}},
     "(memory $m 0)"_su8);

  // Inline export.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       MakeAt("0"_su8,
                              MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(memory (export \"m\") 0)"_su8);

  // Name and inline export.
  OK(ReadMemory,
     Memory{MemoryDesc{MakeAt("$t"_su8, "$t"_sv),
                       MakeAt("0"_su8,
                              MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(memory $t (export \"m\") 0)"_su8);

  // Inline data segment.
  OK(ReadMemory,
     Memory{MemoryDesc{{}, MemoryType{Limits{u32{10}, u32{10}}}},
            {},
            TextList{
                MakeAt("\"hello\""_su8, Text{"\"hello\""_sv, 5}),
                MakeAt("\"world\""_su8, Text{"\"world\""_sv, 5}),
            }},
     "(memory (data \"hello\" \"world\"))"_su8);
}

TEST_F(TextReadTest, MemoryInlineImport) {
  // Inline import.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       MakeAt("0"_su8,
                              MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
            MakeAt("(import \"m\" \"n\")"_su8,
                   InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
            {}},
     "(memory (import \"m\" \"n\") 0)"_su8);

  // Everything for Memory import.
  OK(ReadMemory,
     Memory{MemoryDesc{MakeAt("$t"_su8, "$t"_sv),
                       MakeAt("0"_su8,
                              MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
            MakeAt("(import \"a\" \"b\")"_su8,
                   InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                                MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(memory $t (export \"m\") (import \"a\" \"b\") 0)"_su8);
}

TEST_F(TextReadTest, Global) {
  // Simplest global.
  OK(ReadGlobal,
     Global{GlobalDesc{{},
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})},
            MakeAt("nop"_su8, ConstantExpression{MakeAt(
                                  "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
            {}},
     "(global i32 nop)"_su8);

  // Name.
  OK(ReadGlobal,
     Global{GlobalDesc{MakeAt("$g"_su8, "$g"_sv),
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})},
            MakeAt("nop"_su8, ConstantExpression{MakeAt(
                                  "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
            {}},
     "(global $g i32 nop)"_su8);

  // Inline export.
  OK(ReadGlobal,
     Global{GlobalDesc{{},
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})},
            MakeAt("nop"_su8, ConstantExpression{MakeAt(
                                  "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(global (export \"m\") i32 nop)"_su8);

  // Name and inline export.
  OK(ReadGlobal,
     Global{GlobalDesc{MakeAt("$g2"_su8, "$g2"_sv),
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})},
            MakeAt("nop"_su8, ConstantExpression{MakeAt(
                                  "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(global $g2 (export \"m\") i32 nop)"_su8);
}

TEST_F(TextReadTest, GlobalInlineImport) {
  // Inline import.
  OK(ReadGlobal,
     Global{GlobalDesc{{},
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})},
            MakeAt("(import \"m\" \"n\")"_su8,
                   InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
            {}},
     "(global (import \"m\" \"n\") i32)"_su8);

  // Everything for Global import.
  OK(ReadGlobal,
     Global{GlobalDesc{MakeAt("$g"_su8, "$g"_sv),
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})},
            MakeAt("(import \"a\" \"b\")"_su8,
                   InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                                MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(global $g (export \"m\") (import \"a\" \"b\") i32)"_su8);
}

TEST_F(TextReadTest, Event) {
  Fail(ReadEvent, {{0, "Events not allowed"}}, "(event)"_su8);

  context.features.enable_exceptions();

  // Simplest event.
  OK(ReadEvent, Event{}, "(event)"_su8);

  // Name.
  OK(ReadEvent, Event{EventDesc{MakeAt("$e"_su8, "$e"_sv), {}}, {}},
     "(event $e)"_su8);

  // Inline export.
  OK(ReadEvent,
     Event{EventDesc{},
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(event (export \"m\"))"_su8);

  // Name and inline export.
  OK(ReadEvent,
     Event{EventDesc{MakeAt("$e2"_su8, "$e2"_sv), {}},
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(event $e2 (export \"m\"))"_su8);
}

TEST_F(TextReadTest, EventInlineImport) {
  Fail(ReadEvent, {{0, "Events not allowed"}},
       "(event (import \"m\" \"n\"))"_su8);

  context.features.enable_exceptions();

  // Inline import.
  OK(ReadEvent,
     Event{EventDesc{},
           MakeAt("(import \"m\" \"n\")"_su8,
                  InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                               MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
           {}},
     "(event (import \"m\" \"n\"))"_su8);

  // Everything for event import.
  OK(ReadEvent,
     Event{EventDesc{MakeAt("$e"_su8, "$e"_sv), {}},
           MakeAt("(import \"a\" \"b\")"_su8,
                  InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                               MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
           InlineExportList{
               MakeAt("(export \"m\")"_su8,
                      InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
     "(event $e (export \"m\") (import \"a\" \"b\"))"_su8);
}

TEST_F(TextReadTest, Import) {
  // Function.
  OK(ReadImport,
     Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}), FunctionDesc{}},
     "(import \"m\" \"n\" (func))"_su8);

  // Table.
  OK(ReadImport,
     Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
            TableDesc{nullopt,
                      MakeAt("1 funcref"_su8,
                             TableType{MakeAt("1"_su8,
                                              Limits{MakeAt("1"_su8, u32{1})}),
                                       MakeAt("funcref"_su8, RT_Funcref)})}},
     "(import \"m\" \"n\" (table 1 funcref))"_su8);

  // Memory.
  OK(ReadImport,
     Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
            MemoryDesc{nullopt,
                       MakeAt("1"_su8,
                              MemoryType{MakeAt(
                                  "1"_su8, Limits{MakeAt("1"_su8, u32{1})})})}},
     "(import \"m\" \"n\" (memory 1))"_su8);

  // Global.
  OK(ReadImport,
     Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
            GlobalDesc{nullopt,
                       MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                    Mutability::Const})}},
     "(import \"m\" \"n\" (global i32))"_su8);
}

TEST_F(TextReadTest, Import_AfterNonImport) {
  context.seen_non_import = true;
  Fail(ReadImport,
       {{1, "Imports must occur before all non-import definitions"}},
       "(import \"m\" \"n\" (func))"_su8);
}

TEST_F(TextReadTest, Import_exceptions) {
  Fail(ReadImport, {{17, "Events not allowed"}},
       "(import \"m\" \"n\" (event))"_su8);

  context.features.enable_exceptions();

  // Event.
  OK(ReadImport,
     Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}), EventDesc{}},
     "(import \"m\" \"n\" (event))"_su8);
}

TEST_F(TextReadTest, Export) {
  // Function.
  OK(ReadExport,
     Export{MakeAt("func"_su8, ExternalKind::Function),
            MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("0"_su8, Var{Index{0}})},
     "(export \"m\" (func 0))"_su8);

  // Table.
  OK(ReadExport,
     Export{MakeAt("table"_su8, ExternalKind::Table),
            MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("0"_su8, Var{Index{0}})},
     "(export \"m\" (table 0))"_su8);

  // Memory.
  OK(ReadExport,
     Export{MakeAt("memory"_su8, ExternalKind::Memory),
            MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("0"_su8, Var{Index{0}})},
     "(export \"m\" (memory 0))"_su8);

  // Global.
  OK(ReadExport,
     Export{MakeAt("global"_su8, ExternalKind::Global),
            MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("0"_su8, Var{Index{0}})},
     "(export \"m\" (global 0))"_su8);
}

TEST_F(TextReadTest, Export_exceptions) {
  Fail(ReadExport, {{13, "Events not allowed"}},
       "(export \"m\" (event 0))"_su8);

  context.features.enable_exceptions();

  // Event.
  OK(ReadExport,
     Export{MakeAt("event"_su8, ExternalKind::Event),
            MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
            MakeAt("0"_su8, Var{Index{0}})},
     "(export \"m\" (event 0))"_su8);
}

TEST_F(TextReadTest, Start) {
  OK(ReadStart, Start{MakeAt("0"_su8, Var{Index{0}})}, "(start 0)"_su8);
}

TEST_F(TextReadTest, Start_Multiple) {
  context.seen_start = true;
  Fail(ReadStart, {{1, "Multiple start functions"}}, "(start 0)"_su8);
}

TEST_F(TextReadTest, ElementExpression) {
  context.features.enable_bulk_memory();

  // Item.
  OK(ReadElementExpression,
     ElementExpression{
         MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
     },
     "(item nop)"_su8);

  // Expression.
  OK(ReadElementExpression,
     ElementExpression{
         MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
     },
     "(nop)"_su8);
}

TEST_F(TextReadTest, OffsetExpression) {
  // Expression.
  OK(ReadOffsetExpression,
     ConstantExpression{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
     "(nop)"_su8);

  // Offset keyword.
  OK(ReadOffsetExpression,
     ConstantExpression{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
     "(offset nop)"_su8);
}

TEST_F(TextReadTest, ElementExpressionList) {
  context.features.enable_bulk_memory();

  // Item list.
  OKVector(ReadElementExpressionList,
           ElementExpressionList{
               MakeAt("(item nop)"_su8,
                      ElementExpression{
                          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
               MakeAt("(item nop)"_su8,
                      ElementExpression{
                          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
           },
           "(item nop) (item nop)"_su8);

  // Expression list.
  OKVector(
      ReadElementExpressionList,
      ElementExpressionList{
          MakeAt("(nop)"_su8, ElementExpression{MakeAt(
                                  "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
          MakeAt("(nop)"_su8, ElementExpression{MakeAt(
                                  "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})})},
      "(nop) (nop)"_su8);
}

TEST_F(TextReadTest, TableUseOpt) {
  OK(ReadTableUseOpt, Var{Index{0}}, "(table 0)"_su8);
  OK(ReadTableUseOpt, nullopt, ""_su8);
}

TEST_F(TextReadTest, ElementSegment_MVP) {
  // No table var, empty var list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{ExternalKind::Function, {}}},
     "(elem (nop))"_su8);

  // No table var, var list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{ExternalKind::Function,
                             VarList{MakeAt("0"_su8, Var{Index{0}}),
                                     MakeAt("1"_su8, Var{Index{1}}),
                                     MakeAt("2"_su8, Var{Index{2}})}}},
     "(elem (nop) 0 1 2)"_su8);

  // Table var.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, MakeAt("0"_su8, Var{Index{0}}),
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{ExternalKind::Function, {}}},
     "(elem 0 (nop))"_su8);

  // Table var as Id.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, MakeAt("$t"_su8, Var{"$t"_sv}),
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{ExternalKind::Function, {}}},
     "(elem $t (nop))"_su8);
}

TEST_F(TextReadTest, ElementSegment_bulk_memory) {
  Fail(ReadElementSegment,
       {{6, "Expected offset expression, got ReferenceKind"}},
       "(elem funcref)"_su8);

  Fail(ReadElementSegment, {{6, "Expected offset expression, got Func"}},
       "(elem func)"_su8);

  context.features.enable_bulk_memory();

  // Passive, w/ expression list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, SegmentType::Passive,
         ElementListWithExpressions{
             MakeAt("funcref"_su8, RT_Funcref),
             ElementExpressionList{
                 MakeAt("(nop)"_su8,
                        ElementExpression{
                            MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
                 MakeAt("(nop)"_su8,
                        ElementExpression{
                            MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
             }}},
     "(elem funcref (nop) (nop))"_su8);

  // Passive, w/ var list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, SegmentType::Passive,
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                             VarList{
                                 MakeAt("0"_su8, Var{Index{0}}),
                                 MakeAt("$e"_su8, Var{"$e"_sv}),
                             }}},
     "(elem func 0 $e)"_su8);

  // Passive w/ name.
  OK(ReadElementSegment,
     ElementSegment{
         MakeAt("$e"_su8, "$e"_sv), SegmentType::Passive,
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function), {}}},
     "(elem $e func)"_su8);

  // Declared, w/ expression list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, SegmentType::Declared,
         ElementListWithExpressions{
             MakeAt("funcref"_su8, RT_Funcref),
             ElementExpressionList{
                 MakeAt("(nop)"_su8,
                        ElementExpression{
                            MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
                 MakeAt("(nop)"_su8,
                        ElementExpression{
                            MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
             }}},
     "(elem declare funcref (nop) (nop))"_su8);

  // Declared, w/ var list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, SegmentType::Declared,
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                             VarList{
                                 MakeAt("0"_su8, Var{Index{0}}),
                                 MakeAt("$e"_su8, Var{"$e"_sv}),
                             }}},
     "(elem declare func 0 $e)"_su8);

  // Declared w/ name.
  OK(ReadElementSegment,
     ElementSegment{
         MakeAt("$e2"_su8, "$e2"_sv), SegmentType::Declared,
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function), {}}},
     "(elem $e2 declare func)"_su8);

  // Active legacy, empty
  OK(ReadElementSegment,
     ElementSegment{
         nullopt,
         nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         {}},
     "(elem (nop))"_su8);

  // Active legacy (i.e. no element type or external kind).
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{ExternalKind::Function,
                             VarList{
                                 MakeAt("0"_su8, Var{Index{0}}),
                                 MakeAt("$e"_su8, Var{"$e"_sv}),
                             }}},
     "(elem (nop) 0 $e)"_su8);

  // Active, w/ var list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                             VarList{
                                 MakeAt("0"_su8, Var{Index{0}}),
                                 MakeAt("$e"_su8, Var{"$e"_sv}),
                             }}},
     "(elem (nop) func 0 $e)"_su8);

  // Active, w/ expression list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithExpressions{
             MakeAt("funcref"_su8, RT_Funcref),
             ElementExpressionList{
                 MakeAt("(nop)"_su8,
                        ElementExpression{
                            MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
                 MakeAt("(nop)"_su8,
                        ElementExpression{
                            MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
             }}},
     "(elem (nop) funcref (nop) (nop))"_su8);

  // Active w/ table use.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, MakeAt("(table 0)"_su8, Var{Index{0}}),
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                             VarList{
                                 MakeAt("1"_su8, Var{Index{1}}),
                             }}},
     "(elem (table 0) (nop) func 1)"_su8);

  // Active w/ name.
  OK(ReadElementSegment,
     ElementSegment{
         MakeAt("$e3"_su8, "$e3"_sv), nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function), {}}},
     "(elem $e3 (nop) func)"_su8);
}

TEST_F(TextReadTest, DataSegment_MVP) {
  // No memory var, empty text list.
  OK(ReadDataSegment,
     DataSegment{
         nullopt,
         nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         {}},
     "(data (nop))"_su8);

  // No memory var, text list.
  OK(ReadDataSegment,
     DataSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         TextList{MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2})}},
     "(data (nop) \"hi\")"_su8);

  // Memory var.
  OK(ReadDataSegment,
     DataSegment{
         nullopt,
         MakeAt("0"_su8, Var{Index{0}}),
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         {}},
     "(data 0 (nop))"_su8);

  // Memory var as Id.
  OK(ReadDataSegment,
     DataSegment{
         nullopt,
         MakeAt("$m"_su8, Var{"$m"_sv}),
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         {}},
     "(data $m (nop))"_su8);
}

TEST_F(TextReadTest, DataSegment_bulk_memory) {
  Fail(ReadDataSegment, {{5, "Expected offset expression, got Rpar"}},
       "(data)"_su8);

  context.features.enable_bulk_memory();

  // Passive, w/ text list.
  OK(ReadDataSegment,
     DataSegment{nullopt,
                 TextList{
                     MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2}),
                 }},
     "(data \"hi\")"_su8);

  // Passive w/ name.
  OK(ReadDataSegment, DataSegment{MakeAt("$d"_su8, "$d"_sv), {}},
     "(data $d)"_su8);

  // Active, w/ text list.
  OK(ReadDataSegment,
     DataSegment{
         nullopt, nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         TextList{
             MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2}),
         }},
     "(data (nop) \"hi\")"_su8);

  // Active w/ memory use.
  OK(ReadDataSegment,
     DataSegment{
         nullopt, MakeAt("(memory 0)"_su8, Var{Index{0}}),
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         TextList{
             MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2}),
         }},
     "(data (memory 0) (nop) \"hi\")"_su8);

  // Active w/ name.
  OK(ReadDataSegment,
     DataSegment{
         MakeAt("$d2"_su8, "$d2"_sv),
         nullopt,
         MakeAt("(nop)"_su8, ConstantExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})}),
         {}},
     "(data $d2 (nop))"_su8);
}

TEST_F(TextReadTest, ModuleItem) {
  // Type.
  OK(ReadModuleItem, ModuleItem{TypeEntry{nullopt, BoundFunctionType{}}},
     "(type (func))"_su8);

  // Import.
  OK(ReadModuleItem,
     ModuleItem{Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                       MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
                       FunctionDesc{}}},
     "(import \"m\" \"n\" (func))"_su8);

  // Func.
  OK(ReadModuleItem,
     ModuleItem{
         Function{{}, {}, {MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)})}, {}}},
     "(func)"_su8);

  // Table.
  OK(ReadModuleItem,
     ModuleItem{Table{
         TableDesc{
             nullopt,
             MakeAt("0 funcref"_su8,
                    TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                              MakeAt("funcref"_su8, RT_Funcref)})},
         {}}},
     "(table 0 funcref)"_su8);

  // Memory.
  OK(ReadModuleItem,
     ModuleItem{Memory{
         MemoryDesc{
             nullopt,
             MakeAt("0"_su8, MemoryType{MakeAt(
                                 "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
         {}}},
     "(memory 0)"_su8);

  // Global.
  OK(ReadModuleItem,
     ModuleItem{Global{
         GlobalDesc{nullopt,
                    MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, VT_I32),
                                                 Mutability::Const})},
         MakeAt("(nop)"_su8,
                ConstantExpression{MakeAt(
                    "nop"_su8, Instruction{MakeAt("nop"_su8, Opcode::Nop)})}),
         {}}},
     "(global i32 (nop))"_su8);

  // Export.
  OK(ReadModuleItem,
     ModuleItem{Export{
         MakeAt("func"_su8, ExternalKind::Function),
         MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
         MakeAt("0"_su8, Var{Index{0}}),
     }},
     "(export \"m\" (func 0))"_su8);

  // Start.
  OK(ReadModuleItem, ModuleItem{Start{MakeAt("0"_su8, Var{Index{0}})}},
     "(start 0)"_su8);

  // Elem.
  OK(ReadModuleItem,
     ModuleItem{ElementSegment{
         nullopt,
         nullopt,
         MakeAt("(nop)"_su8,
                ConstantExpression{MakeAt(
                    "nop"_su8, Instruction{MakeAt("nop"_su8, Opcode::Nop)})}),
         {}}},
     "(elem (nop))"_su8);

  // Data.
  OK(ReadModuleItem,
     ModuleItem{DataSegment{
         nullopt,
         nullopt,
         MakeAt("(nop)"_su8,
                ConstantExpression{MakeAt(
                    "nop"_su8, Instruction{MakeAt("nop"_su8, Opcode::Nop)})}),
         {}}},
     "(data (nop))"_su8);
}

TEST_F(TextReadTest, ModuleItem_exceptions) {
  Fail(ReadModuleItem, {{0, "Events not allowed"}}, "(event)"_su8);

  context.features.enable_exceptions();

  // Event.
  OK(ReadModuleItem,
     ModuleItem{
         Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                            FunctionTypeUse{nullopt, {}}}},
               {}}},
     "(event)"_su8);
}

TEST_F(TextReadTest, Module) {
  OK(ReadModule,
     Module{MakeAt("(type (func))"_su8,
                   ModuleItem{TypeEntry{nullopt, BoundFunctionType{}}}),
            MakeAt("(func nop)"_su8,
                   ModuleItem{Function{
                       FunctionDesc{},
                       {},
                       InstructionList{
                           MakeAt("nop"_su8,
                                  Instruction{MakeAt("nop"_su8, Opcode::Nop)}),
                           MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
                       },
                       {}

                   }}),
            MakeAt("(start 0)"_su8,
                   ModuleItem{Start{MakeAt("0"_su8, Var{Index{0}})}})},
     "(type (func)) (func nop) (start 0)"_su8);
}

TEST_F(TextReadTest, Module_MultipleStart) {
  Fail(ReadModule, {{11, "Multiple start functions"}},
       "(start 0) (start 0)"_su8);
}
