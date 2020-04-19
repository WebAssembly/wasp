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
#include "wasp/base/errors.h"
#include "wasp/text/context.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::test;

class TextReadTest : public ::testing::Test {
 protected:
  template <typename Func, typename T>
  void ExpectRead(Func&& func,
                  SpanU8 expected_span,
                  const T& expected,
                  SpanU8 span) {
    Tokenizer tokenizer{span};
    auto actual = func(tokenizer, context);
    ASSERT_EQ(MakeAt(expected_span, expected), actual);
  }

  template <typename Func, typename T>
  void ExpectRead(Func&& func, const T& expected, SpanU8 span) {
    return ExpectRead(func, span, expected, span);
  }

  template <typename Func, typename T>
  void ExpectReadVector(Func&& func, const T& expected, SpanU8 span) {
    Tokenizer tokenizer{span};
    auto actual = func(tokenizer, context);
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
      EXPECT_EQ(expected[i], actual[i]);
    }
  }

  TestErrors errors;
  Context context{errors};
};

TEST_F(TextReadTest, Nat32) {
  ExpectRead(ReadNat32, u32{123}, "123"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Int32) {
  ExpectRead(ReadInt<u32>, u32{123}, "123"_su8);
  ExpectRead(ReadInt<u32>, u32{456}, "+456"_su8);
  ExpectRead(ReadInt<u32>, u32(-789), "-789"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Var_Nat32) {
  ExpectRead(ReadVar, Var{Index{123}}, "123"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Var_Id) {
  ExpectRead(ReadVar, Var{"$foo"_sv}, "$foo"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, VarOpt_Nat32) {
  ExpectRead(ReadVarOpt, Var{Index{3141}}, "3141"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, VarOpt_Id) {
  ExpectRead(ReadVarOpt, Var{"$bar"_sv}, "$bar"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, BindVarOpt) {
  ExpectRead(ReadBindVarOpt, BindVar{"$bar"_sv}, "$bar"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, VarList) {
  auto span = "$a $b 1 2"_su8;
  std::vector<At<Var>> expected{
      MakeAt("$a"_su8, Var{"$a"_sv}),
      MakeAt("$b"_su8, Var{"$b"_sv}),
      MakeAt("1"_su8, Var{Index{1}}),
      MakeAt("2"_su8, Var{Index{2}}),
  };
  ExpectReadVector(ReadVarList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Text) {
  ExpectRead(ReadText, Text{"\"hello\""_sv, 5}, "\"hello\""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, TextList) {
  auto span = "\"hello, \" \"world\" \"123\""_su8;
  std::vector<At<Text>> expected{
      MakeAt("\"hello, \""_su8, Text{"\"hello, \""_sv, 7}),
      MakeAt("\"world\""_su8, Text{"\"world\""_sv, 5}),
      MakeAt("\"123\""_su8, Text{"\"123\""_sv, 3}),
  };
  ExpectReadVector(ReadTextList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ValueType) {
  ExpectRead(ReadValueType, ValueType::I32, "i32"_su8);
  ExpectRead(ReadValueType, ValueType::I64, "i64"_su8);
  ExpectRead(ReadValueType, ValueType::F32, "f32"_su8);
  ExpectRead(ReadValueType, ValueType::F64, "f64"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ValueType_simd) {
  context.features.enable_simd();
  ExpectRead(ReadValueType, ValueType::V128, "v128"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ValueType_reference_types) {
  context.features.enable_reference_types();
  ExpectRead(ReadValueType, ValueType::Funcref, "funcref"_su8);
  ExpectRead(ReadValueType, ValueType::Anyref, "anyref"_su8);
  ExpectRead(ReadValueType, ValueType::Nullref, "nullref"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ValueType_exceptions) {
  context.features.enable_exceptions();
  ExpectRead(ReadValueType, ValueType::Exnref, "exnref"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ValueTypeList) {
  auto span = "i32 f32 f64 i64"_su8;
  std::vector<At<ValueType>> expected{
      MakeAt("i32"_su8, ValueType::I32),
      MakeAt("f32"_su8, ValueType::F32),
      MakeAt("f64"_su8, ValueType::F64),
      MakeAt("i64"_su8, ValueType::I64),
  };
  ExpectReadVector(ReadValueTypeList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ElementType) {
  ExpectRead(ReadElementType, ElementType::Funcref, "funcref"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ElementType_reference_types) {
  context.features.enable_reference_types();
  ExpectRead(ReadElementType, ElementType::Funcref, "funcref"_su8);
  ExpectRead(ReadElementType, ElementType::Anyref, "anyref"_su8);
  ExpectRead(ReadElementType, ElementType::Nullref, "nullref"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, BoundParamList) {
  auto span = "(param i32 f32) (param $foo i64) (param)"_su8;
  std::vector<At<BoundValueType>> expected{
      MakeAt("i32"_su8,
             BoundValueType{nullopt, MakeAt("i32"_su8, ValueType::I32)}),
      MakeAt("f32"_su8,
             BoundValueType{nullopt, MakeAt("f32"_su8, ValueType::F32)}),
      MakeAt("$foo i64"_su8, BoundValueType{MakeAt("$foo"_su8, "$foo"_sv),
                                            MakeAt("i64"_su8, ValueType::I64)}),
  };
  ExpectReadVector(ReadBoundParamList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ParamList) {
  auto span = "(param i32 f32) (param i64) (param)"_su8;
  std::vector<At<ValueType>> expected{
      MakeAt("i32"_su8, ValueType::I32),
      MakeAt("f32"_su8, ValueType::F32),
      MakeAt("i64"_su8, ValueType::I64),
  };
  ExpectReadVector(ReadParamList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ResultList) {
  auto span = "(result i32 f32) (result i64) (result)"_su8;
  std::vector<At<ValueType>> expected{
      MakeAt("i32"_su8, ValueType::I32),
      MakeAt("f32"_su8, ValueType::F32),
      MakeAt("i64"_su8, ValueType::I64),
  };
  ExpectReadVector(ReadResultList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, LocalList) {
  using VT = ValueType;
  using BVT = BoundValueType;
  auto span = "(local i32 f32) (local $foo i64) (local)"_su8;
  std::vector<At<BoundValueType>> expected{
      MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT::I32)}),
      MakeAt("f32"_su8, BVT{nullopt, MakeAt("f32"_su8, VT::F32)}),
      MakeAt("$foo i64"_su8,
             BVT{MakeAt("$foo"_su8, "$foo"_sv), MakeAt("i64"_su8, VT::I64)}),
  };
  ExpectReadVector(ReadLocalList, expected, span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, TypeUseOpt) {
  ExpectRead(ReadTypeUseOpt, Var{Index{123}}, "(type 123)"_su8);
  ExpectRead(ReadTypeUseOpt, Var{"$foo"_sv}, "(type $foo)"_su8);
  ExpectRead(ReadTypeUseOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, FunctionTypeUse) {
  using VT = ValueType;

  // Empty.
  ExpectRead(ReadFunctionTypeUse, FunctionTypeUse{}, ""_su8);

  // Type use.
  ExpectRead(ReadFunctionTypeUse,
             FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}},
             "(type 0)"_su8);

  // Function type.
  ExpectRead(ReadFunctionTypeUse,
             FunctionTypeUse{
                 nullopt, MakeAt("(param i32 f32) (result f64)"_su8,
                                 FunctionType{{MakeAt("i32"_su8, VT::I32),
                                               MakeAt("f32"_su8, VT::F32)},
                                              {MakeAt("f64"_su8, VT::F64)}})},
             "(param i32 f32) (result f64)"_su8);

  // Type use and function type.
  ExpectRead(
      ReadFunctionTypeUse,
      FunctionTypeUse{MakeAt("(type $t)"_su8, Var{"$t"_sv}),
                      MakeAt("(result i32)"_su8,
                             FunctionType{{}, {MakeAt("i32"_su8, VT::I32)}})},
      "(type $t) (result i32)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, InlineImport) {
  ExpectRead(ReadInlineImportOpt,
             InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                          MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})},
             R"((import "m" "n"))"_su8);
  ExpectRead(ReadInlineImportOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, InlineExport) {
  ExpectRead(ReadInlineExportOpt,
             InlineExport{MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})},
             R"((export "n"))"_su8);
  ExpectRead(ReadInlineExportOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, InlineExportList) {
  ExpectReadVector(
      ReadInlineExportList,
      InlineExportList{
          MakeAt("(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})}),
          MakeAt("(export \"n\")"_su8,
                 InlineExport{MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
      },
      R"((export "m") (export "n"))"_su8);
  ExpectRead(ReadInlineExportOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}


TEST_F(TextReadTest, BoundFunctionType) {
  using VT = ValueType;
  using BVT = BoundValueType;

  SpanU8 span =
      "(param i32 i32) (param $t i64) (result f32 f32) (result f64)"_su8;
  ExpectRead(ReadBoundFunctionType,
             BoundFunctionType{
                 {MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT::I32)}),
                  MakeAt("i32"_su8, BVT{nullopt, MakeAt("i32"_su8, VT::I32)}),
                  MakeAt("$t i64"_su8, BVT{MakeAt("$t"_su8, "$t"_sv),
                                           MakeAt("i64"_su8, VT::I64)})},
                 {MakeAt("f32"_su8, VT::F32), MakeAt("f32"_su8, VT::F32),
                  MakeAt("f64"_su8, VT::F64)}},
             span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, FunctionType) {
  using VT = ValueType;

  SpanU8 span = "(param i32 i32) (param i64) (result f32 f32) (result f64)"_su8;
  ExpectRead(
      ReadFunctionType,
      FunctionType{{MakeAt("i32"_su8, VT::I32), MakeAt("i32"_su8, VT::I32),
                    MakeAt("i64"_su8, VT::I64)},
                   {MakeAt("f32"_su8, VT::F32), MakeAt("f32"_su8, VT::F32),
                    MakeAt("f64"_su8, VT::F64)}},
      span);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, TypeEntry) {
  using VT = ValueType;
  using BVT = BoundValueType;

  ExpectRead(ReadTypeEntry, TypeEntry{nullopt, BoundFunctionType{{}, {}}},
             "(type (func))"_su8);

  ExpectRead(
      ReadTypeEntry,
      TypeEntry{
          MakeAt("$foo"_su8, "$foo"_sv),
          MakeAt("(param $bar i32) (result i64)"_su8,
                 BoundFunctionType{
                     {MakeAt("$bar i32"_su8, BVT{MakeAt("$bar"_su8, "$bar"_sv),
                                                 MakeAt("i32"_su8, VT::I32)})},
                     {MakeAt("i64"_su8, VT::I64)}})},
      "(type $foo (func (param $bar i32) (result i64)))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, AlignOpt) {
  ExpectRead(ReadAlignOpt, u32{256}, "align=256"_su8);
  ExpectRead(ReadAlignOpt, u32{16}, "align=0x10"_su8);
  ExpectRead(ReadAlignOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, OffsetOpt) {
  ExpectRead(ReadOffsetOpt, u32{0}, "offset=0"_su8);
  ExpectRead(ReadOffsetOpt, u32{0x123}, "offset=0x123"_su8);
  ExpectRead(ReadOffsetOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Limits) {
  ExpectRead(ReadLimits, Limits{MakeAt("1"_su8, 1u)}, "1"_su8);
  ExpectRead(ReadLimits, Limits{MakeAt("1"_su8, 1u), MakeAt("0x11"_su8, 17u)},
             "1 0x11"_su8);
  ExpectRead(ReadLimits,
             Limits{MakeAt("0"_su8, 0u), MakeAt("20"_su8, 20u),
                    MakeAt("shared"_su8, Shared::Yes)},
             "0 20 shared"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, BlockImmediate) {
  // empty block type.
  ExpectRead(ReadBlockImmediate, BlockImmediate{}, ""_su8);

  // block type w/ label.
  ExpectRead(ReadBlockImmediate,
             BlockImmediate{MakeAt("$l"_su8, BindVar{"$l"_sv}), {}}, "$l"_su8);

  // block type w/ function type use.
  ExpectRead(
      ReadBlockImmediate,
      BlockImmediate{
          nullopt, FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}},
      "(type 0)"_su8);

  // block type w/ label and function type use.
  ExpectRead(ReadBlockImmediate,
             BlockImmediate{
                 MakeAt("$l"_su8, BindVar{"$l"_sv}),
                 FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}},
             "$l (type 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_Bare) {
  using I = Instruction;
  using O = Opcode;

  ExpectRead(ReadPlainInstruction, I{MakeAt("nop"_su8, O::Nop)}, "nop"_su8);
  ExpectRead(ReadPlainInstruction, I{MakeAt("i32.add"_su8, O::I32Add)},
             "i32.add"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_Var) {
  using I = Instruction;
  using O = Opcode;

  ExpectRead(ReadPlainInstruction,
             I{MakeAt("br"_su8, O::Br), MakeAt("0"_su8, Var{Index{0}})},
             "br 0"_su8);
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("local.get"_su8, O::LocalGet), MakeAt("$x"_su8, Var{"$x"_sv})},
      "local.get $x"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_BrOnExn) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_exceptions();

  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("br_on_exn"_su8, O::BrOnExn),
        MakeAt("$l $e"_su8, BrOnExnImmediate{MakeAt("$l"_su8, Var{"$l"_sv}),
                                             MakeAt("$e"_su8, Var{"$e"_sv})})},
      "br_on_exn $l $e"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_BrTable) {
  using I = Instruction;
  using O = Opcode;

  // br_table w/ only default target.
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("br_table"_su8, O::BrTable),
        MakeAt("0"_su8, BrTableImmediate{{}, MakeAt("0"_su8, Var{Index{0}})})},
      "br_table 0"_su8);

  // br_table w/ targets and default target.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("br_table"_su8, O::BrTable),
               MakeAt("0 1 $a $b"_su8,
                      BrTableImmediate{{MakeAt("0"_su8, Var{Index{0}}),
                                        MakeAt("1"_su8, Var{Index{1}}),
                                        MakeAt("$a"_su8, Var{"$a"_sv})},
                                       MakeAt("$b"_su8, Var{"$b"_sv})})},
             "br_table 0 1 $a $b"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_CallIndirect) {
  using I = Instruction;
  using O = Opcode;

  // Bare call_indirect.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("call_indirect"_su8, O::CallIndirect),
               MakeAt(""_su8, CallIndirectImmediate{})},
             "call_indirect"_su8);

  // call_indirect w/ function type use.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("call_indirect"_su8, O::CallIndirect),
               MakeAt("(type 0)"_su8,
                      CallIndirectImmediate{
                          nullopt,
                          FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}),
                                          {}}})},
             "call_indirect (type 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_CallIndirect_reference_types) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_reference_types();
  // In the reference types proposal, the call_indirect instruction also allows
  // a table var first.

  // call_indirect w/ table.
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("call_indirect"_su8, O::CallIndirect),
        MakeAt("$t"_su8,
               CallIndirectImmediate{MakeAt("$t"_su8, Var{"$t"_sv}), {}})},
      "call_indirect $t"_su8);

  // call_indirect w/ table and type use.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("call_indirect"_su8, O::CallIndirect),
               MakeAt("0 (type 0)"_su8,
                      CallIndirectImmediate{
                          MakeAt("0"_su8, Var{Index{0}}),
                          FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}),
                                          {}}})},
             "call_indirect 0 (type 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_Const) {
  using I = Instruction;
  using O = Opcode;

  // i32.const
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("i32.const"_su8, O::I32Const), MakeAt("12"_su8, u32{12})},
             "i32.const 12"_su8);

  // i64.const
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("i64.const"_su8, O::I64Const), MakeAt("34"_su8, u64{34})},
             "i64.const 34"_su8);

  // f32.const
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("f32.const"_su8, O::F32Const), MakeAt("56"_su8, f32{56})},
             "f32.const 56"_su8);

  // f64.const
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("f64.const"_su8, O::F64Const), MakeAt("78"_su8, f64{78})},
             "f64.const 78"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_MemArg) {
  using I = Instruction;
  using O = Opcode;

  // No align, no offset.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("i32.load"_su8, O::I32Load),
               MakeAt(""_su8, MemArgImmediate{nullopt, nullopt})},
             "i32.load"_su8);

  // No align, offset.
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("f32.load"_su8, O::F32Load),
        MakeAt("offset=12"_su8,
               MemArgImmediate{nullopt, MakeAt("offset=12"_su8, u32{12})})},
      "f32.load offset=12"_su8);

  // Align, no offset.
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("i32.load8_u"_su8, O::I32Load8U),
        MakeAt("align=16"_su8,
               MemArgImmediate{MakeAt("align=16"_su8, u32{16}), nullopt})},
      "i32.load8_u align=16"_su8);

  // Align and offset.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("f64.store"_su8, O::F64Store),
               MakeAt("offset=123 align=32"_su8,
                      MemArgImmediate{MakeAt("align=32"_su8, u32{32}),
                                      MakeAt("offset=123"_su8, u32{123})})},
             "f64.store offset=123 align=32"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_Select) {
  using I = Instruction;
  using O = Opcode;

  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("select"_su8, O::Select), MakeAt(""_su8, ValueTypeList{})},
      "select"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_Select_reference_types) {
  using I = Instruction;
  using O = Opcode;
  using VT = ValueType;

  context.features.enable_reference_types();

  // select w/o types
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("select"_su8, O::Select), MakeAt(""_su8, ValueTypeList{})},
      "select"_su8);

  // select w/ one type
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("select"_su8, O::Select),
        MakeAt("(result i32)"_su8, ValueTypeList{MakeAt("i32"_su8, VT::I32)})},
      "select (result i32)"_su8);

  // select w/ multiple types
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("select"_su8, O::Select),
               MakeAt("(result i32) (result i64)"_su8,
                      ValueTypeList{MakeAt("i32"_su8, VT::I32),
                                    MakeAt("i64"_su8, VT::I64)})},
             "select (result i32) (result i64)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_SimdConst) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_simd();

  // i8x16
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("v128.const"_su8, O::V128Const),
        MakeAt("0 1 2 3 4 5 6 7 8 9 0xa 0xb 0xc 0xd 0xe 0xf"_su8,
               v128{u8x16{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe,
                          0xf}})},
      "v128.const i8x16 0 1 2 3 4 5 6 7 8 9 0xa 0xb 0xc 0xd 0xe 0xf"_su8);

  // i16x8
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("v128.const"_su8, O::V128Const),
        MakeAt("0 1 2 3 4 5 6 7"_su8, v128{u16x8{0, 1, 2, 3, 4, 5, 6, 7}})},
      "v128.const i16x8 0 1 2 3 4 5 6 7"_su8);

  // i32x4
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("v128.const"_su8, O::V128Const),
               MakeAt("0 1 2 3"_su8, v128{u32x4{0, 1, 2, 3}})},
             "v128.const i32x4 0 1 2 3"_su8);

  // i64x2
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("v128.const"_su8, O::V128Const),
               MakeAt("0 1"_su8, v128{u64x2{0, 1}})},
             "v128.const i64x2 0 1"_su8);

  // f32x4
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("v128.const"_su8, O::V128Const),
               MakeAt("0 1 2 3"_su8, v128{f32x4{0, 1, 2, 3}})},
             "v128.const f32x4 0 1 2 3"_su8);

  // f64x2
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("v128.const"_su8, O::V128Const),
               MakeAt("0 1"_su8, v128{f64x2{0, 1}})},
             "v128.const f64x2 0 1"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_SimdLane) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_simd();

  ExpectRead(ReadPlainInstruction,
             I{MakeAt("i8x16.extract_lane_s"_su8, O::I8X16ExtractLaneS),
               MakeAt("9"_su8, u32{9})},
             "i8x16.extract_lane_s 9"_su8);
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("f32x4.replace_lane"_su8, O::F32X4ReplaceLane),
               MakeAt("3"_su8, u32{3})},
             "f32x4.replace_lane 3"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_TableCopy) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_bulk_memory();

  // table.copy w/o dst and src.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("table.copy"_su8, O::TableCopy),
               MakeAt(""_su8, CopyImmediate{})},
             "table.copy"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_TableCopy_reference_types) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_reference_types();

  // table.copy w/o dst and src.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("table.copy"_su8, O::TableCopy),
               MakeAt(""_su8, CopyImmediate{})},
             "table.copy"_su8);

  // table.copy w/ dst and src
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("table.copy"_su8, O::TableCopy),
        MakeAt("$d $s"_su8, CopyImmediate{MakeAt("$d"_su8, Var{"$d"_sv}),
                                          MakeAt("$s"_su8, Var{"$s"_sv})})},
      "table.copy $d $s"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, PlainInstruction_TableInit) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_bulk_memory();

  // table.init w/ segment index and table index.
  ExpectRead(
      ReadPlainInstruction,
      I{MakeAt("table.init"_su8, O::TableInit),
        MakeAt("$t $e"_su8, InitImmediate{MakeAt("$e"_su8, Var{"$e"_sv}),
                                          MakeAt("$t"_su8, Var{"$t"_sv})})},
      "table.init $t $e"_su8);

  // table.init w/ just segment index.
  ExpectRead(ReadPlainInstruction,
             I{MakeAt("table.init"_su8, O::TableInit),
               MakeAt("2"_su8,
                      InitImmediate{MakeAt("2"_su8, Var{Index{2}}), nullopt})},
             "table.init 2"_su8);
  ExpectNoErrors(errors);
}

auto ReadBlockInstruction_ForTesting(Tokenizer& tokenizer, Context& context)
    -> InstructionList {
  InstructionList result;
  ReadBlockInstruction(tokenizer, context, result);
  return result;
}

TEST_F(TextReadTest, BlockInstruction_Block) {
  using I = Instruction;
  using O = Opcode;

  // Empty block.
  ExpectReadVector(ReadBlockInstruction_ForTesting,
                   InstructionList{
                       MakeAt("block"_su8, I{MakeAt("block"_su8, O::Block),
                                             BlockImmediate{}}),
                       MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
                   },
                   "block end"_su8);

  // block w/ multiple instructions.
  ExpectReadVector(ReadBlockInstruction_ForTesting,
                   InstructionList{
                       MakeAt("block"_su8, I{MakeAt("block"_su8, O::Block),
                                             BlockImmediate{}}),
                       MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                       MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                       MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
                   },
                   "block nop nop end"_su8);

  // Block w/ label.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("block $l"_su8,
                 I{MakeAt("block"_su8, O::Block),
                   MakeAt("$l"_su8,
                          BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "block $l nop end $l"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, BlockInstruction_Loop) {
  using I = Instruction;
  using O = Opcode;

  // Empty loop.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("loop"_su8, I{MakeAt("loop"_su8, O::Loop), BlockImmediate{}}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "loop end"_su8);

  // loop w/ multiple instructions.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("loop"_su8, I{MakeAt("loop"_su8, O::Loop), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "loop nop nop end"_su8);

  // Loop w/ label.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("loop $l"_su8,
                 I{MakeAt("loop"_su8, O::Loop),
                   MakeAt("$l"_su8,
                          BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "loop $l nop end $l"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, BlockInstruction_If) {
  using I = Instruction;
  using O = Opcode;

  // Empty if.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if end"_su8);

  // if w/ non-empty block.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if nop nop end"_su8);

  // if, w/ else.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if else end"_su8);

  // if, w/ else and non-empty blocks.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if $l"_su8,
                 I{MakeAt("if"_su8, O::If),
                   MakeAt("$l"_su8,
                          BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if $l nop end $l"_su8);

  // If w/ label and matching else and end labels.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("if $l"_su8,
                 I{MakeAt("if"_su8, O::If),
                   MakeAt("$l"_su8,
                          BlockImmediate{MakeAt("$l"_su8, "$l"_sv), {}})}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "if $l nop else $l nop end $l"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, BlockInstruction_Try) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_exceptions();

  // try/catch.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("end"_su8, I{MakeAt("end"_su8, O::End)}),
      },
      "try catch end"_su8);

  // try/catch and non-empty blocks.
  ExpectReadVector(
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
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
      "try $l nop catch nop end $l"_su8);

  // try w/ label and matching catch and end labels.
  ExpectReadVector(
      ReadBlockInstruction_ForTesting,
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
      "try $l nop catch $l nop end $l"_su8);

  ExpectNoErrors(errors);
}

auto ReadExpression_ForTesting(Tokenizer& tokenizer, Context& context)
    -> InstructionList {
  InstructionList result;
  ReadExpression(tokenizer, context, result);
  return result;
}

TEST_F(TextReadTest, Expression_Plain) {
  using I = Instruction;
  using O = Opcode;

  // No immediates.
  ExpectReadVector(ReadExpression_ForTesting,
                   InstructionList{
                       MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                   },
                   "(nop)"_su8);

  // BrTable immediate.
  ExpectReadVector(
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
  ExpectReadVector(
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
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("f32.const 1.0"_su8, I{MakeAt("f32.const"_su8, O::F32Const),
                                        MakeAt("1.0"_su8, f32{1.0f})}),
      },
      "(f32.const 1.0)"_su8);

  // f64 immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("f64.const 2.0"_su8, I{MakeAt("f64.const"_su8, O::F64Const),
                                        MakeAt("2.0"_su8, f64{2.0})}),
      },
      "(f64.const 2.0)"_su8);

  // i32 immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("i32.const 3"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                      MakeAt("3"_su8, u32{3})}),
      },
      "(i32.const 3)"_su8);

  // i64 immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("i64.const 4"_su8, I{MakeAt("i64.const"_su8, O::I64Const),
                                      MakeAt("4"_su8, u64{4})}),
      },
      "(i64.const 4)"_su8);

  // MemArg immediate
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("i32.load align=1"_su8,
                 I{MakeAt("i32.load"_su8, O::I32Load),
                   MakeAt("align=1"_su8,
                          MemArgImmediate{MakeAt("align=1"_su8, u32{1}),
                                          nullopt})}),
      },
      "(i32.load align=1)"_su8);

  // Var immediate.
  ExpectReadVector(ReadExpression_ForTesting,
                   InstructionList{
                       MakeAt("br 0"_su8, I{MakeAt("br"_su8, O::Br),
                                            MakeAt("0"_su8, Var{Index{0}})}),
                   },
                   "(br 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Expression_Plain_exceptions) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_exceptions();

  // BrOnExn immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("br_on_exn 0 0"_su8,
                 I{MakeAt("br_on_exn"_su8, O::BrOnExn),
                   MakeAt("0 0"_su8,
                          BrOnExnImmediate{MakeAt("0"_su8, Var{Index{0}}),
                                           MakeAt("0"_su8, Var{Index{0}})})}),
      },
      "(br_on_exn 0 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Expression_Plain_simd) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_simd();

  // v128 immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("v128.const i32x4 0 0 0 0"_su8,
                 I{MakeAt("v128.const"_su8, O::V128Const),
                   MakeAt("0 0 0 0"_su8, v128{u32x4{0, 0, 0, 0}})}),
      },
      "(v128.const i32x4 0 0 0 0)"_su8);

  // Simd lane immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("f32x4.replace_lane 3"_su8,
                 I{MakeAt("f32x4.replace_lane"_su8, O::F32X4ReplaceLane),
                   MakeAt("3"_su8, u32{3})}),
      },
      "(f32x4.replace_lane 3)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Expression_Plain_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_bulk_memory();

  // Init immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("table.init 0"_su8,
                 I{MakeAt("table.init"_su8, O::TableInit),
                   MakeAt("0"_su8, InitImmediate{MakeAt("0"_su8, Var{Index{0}}),
                                                 nullopt})}),
      },
      "(table.init 0)"_su8);

  // Copy immediate.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("table.copy"_su8,
                 I{MakeAt("table.copy"_su8, O::TableCopy), CopyImmediate{}}),
      },
      "(table.copy)"_su8);

  ExpectNoErrors(errors);
}


TEST_F(TextReadTest, Expression_PlainFolded) {
  using I = Instruction;
  using O = Opcode;

  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("i32.const 0"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                      MakeAt("0"_su8, u32{0})}),
          MakeAt("i32.add"_su8, I{MakeAt("i32.add"_su8, O::I32Add)}),
      },
      "(i32.add (i32.const 0))"_su8);

  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("i32.const 0"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                      MakeAt("0"_su8, u32{0})}),
          MakeAt("i32.const 1"_su8, I{MakeAt("i32.const"_su8, O::I32Const),
                                      MakeAt("1"_su8, u32{1})}),
          MakeAt("i32.add"_su8, I{MakeAt("i32.add"_su8, O::I32Add)}),
      },
      "(i32.add (i32.const 0) (i32.const 1))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Expression_Block) {
  using I = Instruction;
  using O = Opcode;

  // Block.
  ExpectReadVector(ReadExpression_ForTesting,
                   InstructionList{
                       MakeAt("block"_su8, I{MakeAt("block"_su8, O::Block),
                                             BlockImmediate{}}),
                       MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
                   },
                   "(block)"_su8);

  // Loop.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("loop"_su8, I{MakeAt("loop"_su8, O::Loop), BlockImmediate{}}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(loop)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Expression_If) {
  using I = Instruction;
  using O = Opcode;

  // If then.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(if (then))"_su8);

  // If then w/ nops.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(if (then (nop)))"_su8);

  // If condition then.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(if (nop) (then (nop)))"_su8);

  // If then else.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(if (then (nop)) (else (nop)))"_su8);

  // If condition then else.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("if"_su8, I{MakeAt("if"_su8, O::If), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("else"_su8, I{MakeAt("else"_su8, O::Else)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(if (nop) (then (nop)) (else (nop)))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Expression_Try) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_exceptions();

  // Try catch.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(try (catch))"_su8);

  // Try catch w/ nops.
  ExpectReadVector(
      ReadExpression_ForTesting,
      InstructionList{
          MakeAt("try"_su8, I{MakeAt("try"_su8, O::Try), BlockImmediate{}}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt("catch"_su8, I{MakeAt("catch"_su8, O::Catch)}),
          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
          MakeAt(")"_su8, I{MakeAt(")"_su8, O::End)}),
      },
      "(try (nop) (catch (nop)))"_su8);

  ExpectNoErrors(errors);
}

auto ReadExpressionList_ForTesting(Tokenizer& tokenizer, Context& context)
    -> InstructionList {
  InstructionList result;
  ReadExpressionList(tokenizer, context, result);
  return result;
}

TEST_F(TextReadTest, ExpressionList) {
  using I = Instruction;
  using O = Opcode;

  ExpectReadVector(ReadExpressionList_ForTesting,
                   InstructionList{
                       MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                       MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                       MakeAt("drop"_su8, I{MakeAt("drop"_su8, O::Drop)}),
                   },
                   "(nop) (drop (nop))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, TableType) {
  ExpectRead(ReadTableType,
             TableType{MakeAt("1 2"_su8, Limits{MakeAt("1"_su8, u32{1}),
                                                MakeAt("2"_su8, u32{2})}),
                       MakeAt("funcref"_su8, ElementType::Funcref)},
             "1 2 funcref"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, MemoryType) {
  ExpectRead(ReadMemoryType,
             MemoryType{MakeAt("1 2"_su8, Limits{MakeAt("1"_su8, u32{1}),
                                                 MakeAt("2"_su8, u32{2})})},
             "1 2"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, GlobalType) {
  ExpectRead(ReadGlobalType,
             GlobalType{MakeAt("i32"_su8, MakeAt("i32"_su8, ValueType::I32)),
                        Mutability::Const},
             "i32"_su8);

  ExpectRead(
      ReadGlobalType,
      GlobalType{MakeAt("(mut i32)"_su8, MakeAt("i32"_su8, ValueType::I32)),
                 MakeAt("mut"_su8, Mutability::Var)},
      "(mut i32)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, EventType) {
  // Empty event type.
  ExpectRead(ReadEventType, EventType{EventAttribute::Exception, {}}, ""_su8);

  // Function type use.
  ExpectRead(
      ReadEventType,
      EventType{EventAttribute::Exception,
                FunctionTypeUse{MakeAt("(type 0)"_su8, Var{Index{0}}), {}}},
      "(type 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Function) {
  using I = Instruction;
  using O = Opcode;

  // Empty func.
  ExpectRead(ReadFunction, Function{}, "(func)"_su8);

  // Name.
  ExpectRead(
      ReadFunction,
      Function{
          FunctionDesc{MakeAt("$f"_su8, "$f"_sv), nullopt, {}}, {}, {}, {}, {}},
      "(func $f)"_su8);

  // Inline export.
  ExpectRead(
      ReadFunction,
      Function{{},
               {},
               {},
               {},
               InlineExportList{MakeAt(
                   "(export \"e\")"_su8,
                   InlineExport{MakeAt("\"e\""_su8, Text{"\"e\""_sv, 1})})}},
      "(func (export \"e\"))"_su8);

  // Locals.
  ExpectRead(
      ReadFunction,
      Function{{},
               BoundValueTypeList{
                   MakeAt("i32"_su8,
                          BoundValueType{nullopt,
                                         MakeAt("i32"_su8, ValueType::I32)}),
                   MakeAt("i64"_su8,
                          BoundValueType{nullopt,
                                         MakeAt("i64"_su8, ValueType::I64)}),
               },
               {},
               {},
               {}},
      "(func (local i32 i64))"_su8);

  // Instructions.
  ExpectRead(ReadFunction,
             Function{{},
                      {},
                      InstructionList{
                          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                          MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
                      },
                      {},
                      {}},
             "(func nop nop nop)"_su8);

  // Everything for defined Function.
  ExpectRead(
      ReadFunction,
      Function{FunctionDesc{MakeAt("$f"_su8, "$f"_sv), nullopt, {}},
               BoundValueTypeList{MakeAt(
                   "i32"_su8,
                   BoundValueType{nullopt, MakeAt("i32"_su8, ValueType::I32)})},
               InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
               {},
               InlineExportList{MakeAt(
                   "(export \"m\")"_su8,
                   InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(func $f (export \"m\") (local i32) nop)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, FunctionInlineImport) {
  // Import.
  ExpectRead(
      ReadFunction,
      Function{{},
               {},
               {},
               MakeAt("(import \"m\" \"n\")"_su8,
                      InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                   MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
               {}},
      "(func (import \"m\" \"n\"))"_su8);

  // Everything for imported Function.
  ExpectRead(
      ReadFunction,
      Function{
          FunctionDesc{
              MakeAt("$f"_su8, "$f"_sv), nullopt,
              MakeAt(
                  "(param i32)"_su8,
                  BoundFunctionType{
                      {MakeAt("i32"_su8,
                              BoundValueType{
                                  nullopt, MakeAt("i32"_su8, ValueType::I32)})},
                      {}})},
          {},
          {},
          MakeAt("(import \"a\" \"b\")"_su8,
                 InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                              MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
          InlineExportList{
              MakeAt("(export \"m\")"_su8,
                     InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(func $f (export \"m\") (import \"a\" \"b\") (param i32))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Table) {
  // Simplest table.
  ExpectRead(
      ReadTable,
      Table{
          TableDesc{
              {},
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          nullopt,
          {},
          {}},
      "(table 0 funcref)"_su8);

  // Name.
  ExpectRead(
      ReadTable,
      Table{
          TableDesc{
              MakeAt("$t"_su8, "$t"_sv),
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          nullopt,
          {},
          {}},
      "(table $t 0 funcref)"_su8);

  // Inline export.
  ExpectRead(
      ReadTable,
      Table{
          TableDesc{
              {},
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          nullopt,
          InlineExportList{
              MakeAt("(export \"m\")"_su8,
                     InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})},
          {}},
      "(table (export \"m\") 0 funcref)"_su8);

  // Name and inline export.
  ExpectRead(
      ReadTable,
      Table{
          TableDesc{
              MakeAt("$t"_su8, "$t"_sv),
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          nullopt,
          InlineExportList{
              MakeAt("(export \"m\")"_su8,
                     InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})},
          {}},
      "(table $t (export \"m\") 0 funcref)"_su8);

  // Inline element var list.
  ExpectRead(
      ReadTable,
      Table{TableDesc{{},
                      TableType{Limits{u32{3}, u32{3}},
                                MakeAt("funcref"_su8, ElementType::Funcref)}},
            nullopt,
            {},
            ElementListWithVars{ExternalKind::Function,
                                VarList{
                                    MakeAt("0"_su8, Var{Index{0}}),
                                    MakeAt("1"_su8, Var{Index{1}}),
                                    MakeAt("2"_su8, Var{Index{2}}),
                                }}},
      "(table funcref (elem 0 1 2))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, TableInlineImport) {
  // Inline import.
  ExpectRead(
      ReadTable,
      Table{
          TableDesc{
              {},
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          MakeAt("(import \"m\" \"n\")"_su8,
                 InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                              MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
          {},
          {}},
      "(table (import \"m\" \"n\") 0 funcref)"_su8);

  // Everything for Table import.
  ExpectRead(
      ReadTable,
      Table{
          TableDesc{
              MakeAt("$t"_su8, "$t"_sv),
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          MakeAt("(import \"a\" \"b\")"_su8,
                 InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                              MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
          InlineExportList{
              MakeAt("(export \"m\")"_su8,
                     InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})},
          {}},
      "(table $t (export \"m\") (import \"a\" \"b\") 0 funcref)"_su8);


  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Table_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_bulk_memory();

  // Inline element var list.
  ExpectRead(
      ReadTable,
      Table{TableDesc{{},
                      TableType{Limits{u32{2}, u32{2}},
                                MakeAt("funcref"_su8, ElementType::Funcref)}},
            nullopt,
            {},
            ElementListWithExpressions{
                MakeAt("funcref"_su8, ElementType::Funcref),
                ElementExpressionList{
                    ElementExpression{
                        MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                    ElementExpression{
                        MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                }}},
      "(table funcref (elem (nop) (nop)))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Memory) {
  // Simplest memory.
  ExpectRead(
      ReadMemory,
      Memory{MemoryDesc{{},
                        MakeAt("0"_su8,
                               MemoryType{MakeAt(
                                   "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
             nullopt,
             {},
             {}},
      "(memory 0)"_su8);

  // Name.
  ExpectRead(
      ReadMemory,
      Memory{MemoryDesc{MakeAt("$m"_su8, "$m"_sv),
                        MakeAt("0"_su8,
                               MemoryType{MakeAt(
                                   "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
             nullopt,
             {},
             {}},
      "(memory $m 0)"_su8);

  // Inline export.
  ExpectRead(
      ReadMemory,
      Memory{MemoryDesc{{},
                        MakeAt("0"_su8,
                               MemoryType{MakeAt(
                                   "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
             nullopt,
             InlineExportList{MakeAt(
                 "(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})},
             {}},
      "(memory (export \"m\") 0)"_su8);

  // Name and inline export.
  ExpectRead(
      ReadMemory,
      Memory{MemoryDesc{MakeAt("$t"_su8, "$t"_sv),
                        MakeAt("0"_su8,
                               MemoryType{MakeAt(
                                   "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
             nullopt,
             InlineExportList{MakeAt(
                 "(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})},
             {}},
      "(memory $t (export \"m\") 0)"_su8);

  // Inline data segment.
  ExpectRead(ReadMemory,
             Memory{MemoryDesc{{}, MemoryType{Limits{u32{10}, u32{10}}}},
                    nullopt,
                    {},
                    TextList{
                        MakeAt("\"hello\""_su8, Text{"\"hello\""_sv, 5}),
                        MakeAt("\"world\""_su8, Text{"\"world\""_sv, 5}),
                    }},
             "(memory (data \"hello\" \"world\"))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, MemoryInlineImport) {
  // Inline import.
  ExpectRead(
      ReadMemory,
      Memory{MemoryDesc{{},
                        MakeAt("0"_su8,
                               MemoryType{MakeAt(
                                   "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
             MakeAt("(import \"m\" \"n\")"_su8,
                    InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                 MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
             {},
             {}},
      "(memory (import \"m\" \"n\") 0)"_su8);

  // Everything for Memory import.
  ExpectRead(
      ReadMemory,
      Memory{MemoryDesc{MakeAt("$t"_su8, "$t"_sv),
                        MakeAt("0"_su8,
                               MemoryType{MakeAt(
                                   "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
             MakeAt("(import \"a\" \"b\")"_su8,
                    InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                                 MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
             InlineExportList{MakeAt(
                 "(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})},
             {}},
      "(memory $t (export \"m\") (import \"a\" \"b\") 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Global) {
  using I = Instruction;
  using O = Opcode;

  // Simplest global.
  ExpectRead(
      ReadGlobal,
      Global{GlobalDesc{
                 {},
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
             InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
             nullopt,
             {}},
      "(global i32 nop)"_su8);

  // Name.
  ExpectRead(
      ReadGlobal,
      Global{GlobalDesc{
                 MakeAt("$g"_su8, "$g"_sv),
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
             InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
             nullopt,
             {}},
      "(global $g i32 nop)"_su8);

  // Inline export.
  ExpectRead(
      ReadGlobal,
      Global{GlobalDesc{
                 {},
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
             InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
             nullopt,
             InlineExportList{MakeAt(
                 "(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(global (export \"m\") i32 nop)"_su8);

  // Name and inline export.
  ExpectRead(
      ReadGlobal,
      Global{GlobalDesc{
                 MakeAt("$g"_su8, "$g"_sv),
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
             InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
             nullopt,
             InlineExportList{MakeAt(
                 "(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(global $g (export \"m\") i32 nop)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, GlobalInlineImport) {
  // Inline import.
  ExpectRead(
      ReadGlobal,
      Global{GlobalDesc{
                 {},
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
             {},
             MakeAt("(import \"m\" \"n\")"_su8,
                    InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                 MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
             {}},
      "(global (import \"m\" \"n\") i32)"_su8);

  // Everything for Global import.
  ExpectRead(
      ReadGlobal,
      Global{GlobalDesc{
                 MakeAt("$g"_su8, "$g"_sv),
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
             {},
             MakeAt("(import \"a\" \"b\")"_su8,
                    InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                                 MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
             InlineExportList{MakeAt(
                 "(export \"m\")"_su8,
                 InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(global $g (export \"m\") (import \"a\" \"b\") i32)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Event) {
  // Simplest event.
  ExpectRead(ReadEvent, Event{}, "(event)"_su8);

  // Name.
  ExpectRead(ReadEvent, Event{EventDesc{MakeAt("$e"_su8, "$e"_sv), {}}, {}, {}},
             "(event $e)"_su8);

  // Inline export.
  ExpectRead(
      ReadEvent,
      Event{EventDesc{}, nullopt,
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(event (export \"m\"))"_su8);

  // Name and inline export.
  ExpectRead(
      ReadEvent,
      Event{EventDesc{MakeAt("$e"_su8, "$e"_sv), {}}, nullopt,
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(event $e (export \"m\"))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, EventInlineImport) {
  // Inline import.
  ExpectRead(
      ReadEvent,
      Event{EventDesc{},
            MakeAt("(import \"m\" \"n\")"_su8,
                   InlineImport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                                MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1})}),
            {}},
      "(event (import \"m\" \"n\"))"_su8);

  // Everything for event import.
  ExpectRead(
      ReadEvent,
      Event{EventDesc{MakeAt("$e"_su8, "$e"_sv), {}},
            MakeAt("(import \"a\" \"b\")"_su8,
                   InlineImport{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                                MakeAt("\"b\""_su8, Text{"\"b\""_sv, 1})}),
            InlineExportList{MakeAt(
                "(export \"m\")"_su8,
                InlineExport{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1})})}},
      "(event $e (export \"m\") (import \"a\" \"b\"))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Import) {
  // Function.
  ExpectRead(ReadImport,
             Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}), FunctionDesc{}},
             "(import \"m\" \"n\" (func))"_su8);

  // Table.
  ExpectRead(
      ReadImport,
      Import{
          MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
          MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
          TableDesc{
              nullopt,
              MakeAt("1 funcref"_su8,
                     TableType{MakeAt("1"_su8, Limits{MakeAt("1"_su8, u32{1})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})}},
      "(import \"m\" \"n\" (table 1 funcref))"_su8);

  // Memory.
  ExpectRead(
      ReadImport,
      Import{
          MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
          MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
          MemoryDesc{
              nullopt,
              MakeAt("1"_su8, MemoryType{MakeAt(
                                  "1"_su8, Limits{MakeAt("1"_su8, u32{1})})})}},
      "(import \"m\" \"n\" (memory 1))"_su8);

  // Global.
  ExpectRead(
      ReadImport,
      Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
             MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
             GlobalDesc{
                 nullopt,
                 MakeAt("i32"_su8, GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})}},
      "(import \"m\" \"n\" (global i32))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Import_exceptions) {
  context.features.enable_exceptions();

  // Event.
  ExpectRead(ReadImport,
             Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}), EventDesc{}},
             "(import \"m\" \"n\" (event))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Export) {
  // Function.
  ExpectRead(ReadExport,
             Export{MakeAt("func"_su8, ExternalKind::Function),
                    MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("0"_su8, Var{Index{0}})},
             "(export \"m\" (func 0))"_su8);

  // Table.
  ExpectRead(ReadExport,
             Export{MakeAt("table"_su8, ExternalKind::Table),
                    MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("0"_su8, Var{Index{0}})},
             "(export \"m\" (table 0))"_su8);

  // Memory.
  ExpectRead(ReadExport,
             Export{MakeAt("memory"_su8, ExternalKind::Memory),
                    MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("0"_su8, Var{Index{0}})},
             "(export \"m\" (memory 0))"_su8);

  // Global.
  ExpectRead(ReadExport,
             Export{MakeAt("global"_su8, ExternalKind::Global),
                    MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("0"_su8, Var{Index{0}})},
             "(export \"m\" (global 0))"_su8);
}

TEST_F(TextReadTest, Export_exceptions) {
  context.features.enable_exceptions();

  // Event.
  ExpectRead(ReadExport,
             Export{MakeAt("event"_su8, ExternalKind::Event),
                    MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                    MakeAt("0"_su8, Var{Index{0}})},
             "(export \"m\" (event 0))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Start) {
  ExpectRead(ReadStart, Start{MakeAt("0"_su8, Var{Index{0}})},
             "(start 0)"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ElementExpression) {
  using I = Instruction;
  using O = Opcode;

  // Item.
  ExpectRead(ReadElementExpression,
             ElementExpression{
                 MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
             },
             "(item nop)"_su8);

  // Expression.
  ExpectRead(ReadElementExpression,
             ElementExpression{
                 MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)}),
             },
             "(nop)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, OffsetExpression) {
  using I = Instruction;
  using O = Opcode;

  // Expression.
  ExpectRead(ReadOffsetExpression,
             InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
             "(nop)"_su8);

  // Offset keyword.
  ExpectRead(ReadOffsetExpression,
             InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
             "(offset nop)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ElementExpressionList) {
  using I = Instruction;
  using O = Opcode;

  // Item list.
  ExpectReadVector(
      ReadElementExpressionList,
      ElementExpressionList{
          ElementExpression{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementExpression{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
      },
      "(item nop) (item nop)"_su8);

  // Expression list.
  ExpectReadVector(
      ReadElementExpressionList,
      ElementExpressionList{
          ElementExpression{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementExpression{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
      },
      "(nop) (nop)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, TableUseOpt) {
  ExpectRead(ReadTableUseOpt, Var{Index{0}}, "(table 0)"_su8);
  ExpectRead(ReadTableUseOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ElementSegment_MVP) {
  using I = Instruction;
  using O = Opcode;

  // No table var, empty var list.
  ExpectRead(ReadElementSegment,
             ElementSegment{nullopt, nullopt,
                            InstructionList{MakeAt(
                                "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                            ElementListWithVars{ExternalKind::Function, {}}},
             "(elem (nop))"_su8);

  // No table var, var list.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          nullopt, nullopt,
          InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementListWithVars{ExternalKind::Function,
                              VarList{MakeAt("0"_su8, Var{Index{0}}),
                                      MakeAt("1"_su8, Var{Index{1}}),
                                      MakeAt("2"_su8, Var{Index{2}})}}},
      "(elem (nop) 0 1 2)"_su8);

  // Table var.
  ExpectRead(ReadElementSegment,
             ElementSegment{nullopt, MakeAt("0"_su8, Var{Index{0}}),
                            InstructionList{MakeAt(
                                "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                            ElementListWithVars{ExternalKind::Function, {}}},
             "(elem 0 (nop))"_su8);

  // Table var as Id.
  ExpectRead(ReadElementSegment,
             ElementSegment{nullopt, MakeAt("$t"_su8, Var{"$t"_sv}),
                            InstructionList{MakeAt(
                                "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                            ElementListWithVars{ExternalKind::Function, {}}},
             "(elem $t (nop))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ElementSegment_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_bulk_memory();

  // Passive, w/ expression list.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{nullopt, SegmentType::Passive,
                     ElementListWithExpressions{
                         MakeAt("funcref"_su8, ElementType::Funcref),
                         ElementExpressionList{
                             ElementExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                             ElementExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         }}},
      "(elem funcref (nop) (nop))"_su8);

  // Passive, w/ var list.
  ExpectRead(ReadElementSegment,
             ElementSegment{
                 nullopt, SegmentType::Passive,
                 ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                                     VarList{
                                         MakeAt("0"_su8, Var{Index{0}}),
                                         MakeAt("$e"_su8, Var{"$e"_sv}),
                                     }}},
             "(elem func 0 $e)"_su8);

  // Passive w/ name.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          MakeAt("$e"_su8, "$e"_sv), SegmentType::Passive,
          ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function), {}}},
      "(elem $e func)"_su8);

  // Declared, w/ expression list.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{nullopt, SegmentType::Declared,
                     ElementListWithExpressions{
                         MakeAt("funcref"_su8, ElementType::Funcref),
                         ElementExpressionList{
                             ElementExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                             ElementExpression{MakeAt(
                                 "nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         }}},
      "(elem declare funcref (nop) (nop))"_su8);

  // Declared, w/ var list.
  ExpectRead(ReadElementSegment,
             ElementSegment{
                 nullopt, SegmentType::Declared,
                 ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                                     VarList{
                                         MakeAt("0"_su8, Var{Index{0}}),
                                         MakeAt("$e"_su8, Var{"$e"_sv}),
                                     }}},
             "(elem declare func 0 $e)"_su8);

  // Declared w/ name.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          MakeAt("$e"_su8, "$e"_sv), SegmentType::Declared,
          ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function), {}}},
      "(elem $e declare func)"_su8);

  // Active legacy (i.e. no element type or external kind).
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          nullopt, nullopt,
          InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementListWithVars{ExternalKind::Function,
                              VarList{
                                  MakeAt("0"_su8, Var{Index{0}}),
                                  MakeAt("$e"_su8, Var{"$e"_sv}),
                              }}},
      "(elem (nop) 0 $e)"_su8);

  // Active, w/ var list.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          nullopt, nullopt,
          InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                              VarList{
                                  MakeAt("0"_su8, Var{Index{0}}),
                                  MakeAt("$e"_su8, Var{"$e"_sv}),
                              }}},
      "(elem (nop) func 0 $e)"_su8);

  // Active, w/ expression list.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          nullopt, nullopt,
          InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementListWithExpressions{
              MakeAt("funcref"_su8, ElementType::Funcref),
              ElementExpressionList{
                  ElementExpression{
                      MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                  ElementExpression{
                      MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
              }}},
      "(elem (nop) funcref (nop) (nop))"_su8);

  // Active w/ table use.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          nullopt, MakeAt("(table 0)"_su8, Var{Index{0}}),
          InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function),
                              VarList{
                                  MakeAt("1"_su8, Var{Index{1}}),
                              }}},
      "(elem (table 0) (nop) func 1)"_su8);

  // Active w/ name.
  ExpectRead(
      ReadElementSegment,
      ElementSegment{
          MakeAt("$e"_su8, "$e"_sv), nullopt,
          InstructionList{MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
          ElementListWithVars{MakeAt("func"_su8, ExternalKind::Function), {}}},
      "(elem $e (nop) func)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, DataSegment_MVP) {
  using I = Instruction;
  using O = Opcode;

  // No memory var, empty text list.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt,
                         nullopt,
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         {}},
             "(data (nop))"_su8);

  // No memory var, text list.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt, nullopt,
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         TextList{MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2})}},
             "(data (nop) \"hi\")"_su8);

  // Memory var.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt,
                         MakeAt("0"_su8, Var{Index{0}}),
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         {}},
             "(data 0 (nop))"_su8);

  // Memory var as Id.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt,
                         MakeAt("$m"_su8, Var{"$m"_sv}),
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         {}},
             "(data $m (nop))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, DataSegment_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  context.features.enable_bulk_memory();

  // Passive, w/ text list.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt,
                         TextList{
                             MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2}),
                         }},
             "(data \"hi\")"_su8);

  // Passive w/ name.
  ExpectRead(ReadDataSegment, DataSegment{MakeAt("$d"_su8, "$d"_sv), {}},
             "(data $d)"_su8);

  // Active, w/ text list.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt, nullopt,
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         TextList{
                             MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2}),
                         }},
             "(data (nop) \"hi\")"_su8);

  // Active w/ memory use.
  ExpectRead(ReadDataSegment,
             DataSegment{nullopt, MakeAt("(memory 0)"_su8, Var{Index{0}}),
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         TextList{
                             MakeAt("\"hi\""_su8, Text{"\"hi\""_sv, 2}),
                         }},
             "(data (memory 0) (nop) \"hi\")"_su8);

  // Active w/ name.
  ExpectRead(ReadDataSegment,
             DataSegment{MakeAt("$d"_su8, "$d"_sv),
                         nullopt,
                         InstructionList{
                             MakeAt("nop"_su8, I{MakeAt("nop"_su8, O::Nop)})},
                         {}},
             "(data $d (nop))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ModuleItem) {
  // Type.
  ExpectRead(ReadModuleItem,
             ModuleItem{TypeEntry{nullopt, BoundFunctionType{}}},
             "(type (func))"_su8);

  // Import.
  ExpectRead(ReadModuleItem,
             ModuleItem{Import{MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                               MakeAt("\"n\""_su8, Text{"\"n\""_sv, 1}),
                               FunctionDesc{}}},
             "(import \"m\" \"n\" (func))"_su8);

  // Func.
  ExpectRead(ReadModuleItem, ModuleItem{Function{}}, "(func)"_su8);

  // Table.
  ExpectRead(
      ReadModuleItem,
      ModuleItem{Table{
          TableDesc{
              nullopt,
              MakeAt("0 funcref"_su8,
                     TableType{MakeAt("0"_su8, Limits{MakeAt("0"_su8, u32{0})}),
                               MakeAt("funcref"_su8, ElementType::Funcref)})},
          nullopt,
          {},
          nullopt}},
      "(table 0 funcref)"_su8);

  // Memory.
  ExpectRead(
      ReadModuleItem,
      ModuleItem{Memory{
          MemoryDesc{
              nullopt,
              MakeAt("0"_su8, MemoryType{MakeAt(
                                  "0"_su8, Limits{MakeAt("0"_su8, u32{0})})})},
          nullopt,
          {},
          nullopt}},
      "(memory 0)"_su8);

  // Global.
  ExpectRead(ReadModuleItem,
             ModuleItem{Global{
                 GlobalDesc{nullopt,
                            MakeAt("i32"_su8,
                                   GlobalType{MakeAt("i32"_su8, ValueType::I32),
                                              Mutability::Const})},
                 InstructionList{MakeAt(
                     "nop"_su8, Instruction{MakeAt("nop"_su8, Opcode::Nop)})},
                 nullopt,
                 {}}},
             "(global i32 (nop))"_su8);

  // Export.
  ExpectRead(ReadModuleItem,
             ModuleItem{Export{
                 MakeAt("func"_su8, ExternalKind::Function),
                 MakeAt("\"m\""_su8, Text{"\"m\""_sv, 1}),
                 MakeAt("0"_su8, Var{Index{0}}),
             }},
             "(export \"m\" (func 0))"_su8);

  // Start.
  ExpectRead(ReadModuleItem,
             ModuleItem{Start{MakeAt("0"_su8, Var{Index{0}})}},
             "(start 0)"_su8);

  // Elem.
  ExpectRead(ReadModuleItem,
             ModuleItem{ElementSegment{
                 nullopt,
                 nullopt,
                 InstructionList{MakeAt(
                     "nop"_su8, Instruction{MakeAt("nop"_su8, Opcode::Nop)})},
                 {}}},
             "(elem (nop))"_su8);

  // Data.
  ExpectRead(ReadModuleItem,
             ModuleItem{DataSegment{
                 nullopt,
                 nullopt,
                 InstructionList{MakeAt(
                     "nop"_su8, Instruction{MakeAt("nop"_su8, Opcode::Nop)})},
                 {}}},
             "(data (nop))"_su8);

  // Event.
  ExpectRead(ReadModuleItem,
             ModuleItem{Event{
                 EventDesc{nullopt, EventType{EventAttribute::Exception,
                                              FunctionTypeUse{nullopt, {}}}},
                 nullopt,
                 {}}},
             "(event)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Module) {
  ExpectRead(ReadModule,
             Module{MakeAt("(type (func))"_su8,
                           ModuleItem{TypeEntry{nullopt, BoundFunctionType{}}}),
                    MakeAt("(func nop)"_su8,
                           ModuleItem{Function{
                               FunctionDesc{},
                               {},
                               InstructionList{MakeAt(
                                   "nop"_su8, Instruction{MakeAt(
                                                  "nop"_su8, Opcode::Nop)})},
                               nullopt,
                               {}

                           }}),
                    MakeAt("(start 0)"_su8,
                           ModuleItem{Start{MakeAt("0"_su8, Var{Index{0}})}})},
             "(type (func)) (func nop) (start 0)"_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ModuleVarOpt) {
  ExpectRead(ReadModuleVarOpt, ModuleVar{"$m"_sv}, "$m"_su8);
  ExpectRead(ReadModuleVarOpt, nullopt, ""_su8);
  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ScriptModule) {
  // Text module.
  ExpectRead(ReadScriptModule,
             ScriptModule{nullopt, ScriptModuleKind::Text, Module{}},
             "(module)"_su8);

  // Binary module.
  ExpectRead(ReadScriptModule,
             ScriptModule{nullopt, ScriptModuleKind::Binary,
                          TextList{MakeAt("\"\""_su8, Text{"\"\""_sv, 0})}},
             "(module binary \"\")"_su8);

  // Quote module.
  ExpectRead(ReadScriptModule,
             ScriptModule{nullopt, ScriptModuleKind::Quote,
                          TextList{MakeAt("\"\""_su8, Text{"\"\""_sv, 0})}},
             "(module quote \"\")"_su8);

  // Text module w/ Name.
  ExpectRead(
      ReadScriptModule,
      ScriptModule{MakeAt("$m"_su8, "$m"_sv), ScriptModuleKind::Text, Module{}},
      "(module $m)"_su8);

  // Binary module w/ Name.
  ExpectRead(ReadScriptModule,
             ScriptModule{MakeAt("$m"_su8, "$m"_sv), ScriptModuleKind::Binary,
                          TextList{MakeAt("\"\""_su8, Text{"\"\""_sv, 0})}},
             "(module $m binary \"\")"_su8);

  // Quote module w/ Name.
  ExpectRead(ReadScriptModule,
             ScriptModule{MakeAt("$m"_su8, "$m"_sv), ScriptModuleKind::Quote,
                          TextList{MakeAt("\"\""_su8, Text{"\"\""_sv, 0})}},
             "(module $m quote \"\")"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Const) {
  // i32.const
  ExpectRead(ReadConst, Const{u32{0}}, "(i32.const 0)"_su8);

  // i64.const
  ExpectRead(ReadConst, Const{u64{0}}, "(i64.const 0)"_su8);

  // f32.const
  ExpectRead(ReadConst, Const{f32{0}}, "(f32.const 0)"_su8);

  // f64.const
  ExpectRead(ReadConst, Const{f64{0}}, "(f64.const 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Const_simd) {
  context.features.enable_simd();

  ExpectRead(ReadConst, Const{v128{}}, "(v128.const i32x4 0 0 0 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Const_reference_types) {
  context.features.enable_reference_types();

  ExpectRead(ReadConst, Const{RefNullConst{}}, "(ref.null)"_su8);
  ExpectRead(ReadConst, Const{RefHostConst{MakeAt("0"_su8, u32{0})}},
             "(ref.host 0)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ConstList) {
  ExpectReadVector(ReadConstList, ConstList{}, ""_su8);

  ExpectReadVector(ReadConstList,
                   ConstList{
                       MakeAt("(i32.const 0)"_su8, Const{u32{0}}),
                       MakeAt("(i64.const 1)"_su8, Const{u64{1}}),
                   },
                   "(i32.const 0) (i64.const 1)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, InvokeAction) {
  // Name.
  ExpectRead(
      ReadInvokeAction,
      InvokeAction{nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}},
      "(invoke \"a\")"_su8);

  // Module.
  ExpectRead(ReadInvokeAction,
             InvokeAction{MakeAt("$m"_su8, "$m"_sv),
                          MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                          {}},
             "(invoke $m \"a\")"_su8);

  // Const list.
  ExpectRead(
      ReadInvokeAction,
      InvokeAction{nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                   ConstList{MakeAt("(i32.const 0)"_su8, Const{u32{0}})}},
      "(invoke \"a\" (i32.const 0))"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, GetAction) {
  // Name.
  ExpectRead(ReadGetAction,
             GetAction{nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1})},
             "(get \"a\")"_su8);

  // Module.
  ExpectRead(ReadGetAction,
             GetAction{MakeAt("$m"_su8, "$m"_sv),
                       MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1})},
             "(get $m \"a\")"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Action) {
  // Get action.
  ExpectRead(
      ReadAction,
      Action{GetAction{nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1})}},
      "(get \"a\")"_su8);

  // Invoke action.
  ExpectRead(ReadAction,
             Action{InvokeAction{
                 nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}},
             "(invoke \"a\")"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ModuleAssertion) {
  ExpectRead(
      ReadModuleAssertion,
      ModuleAssertion{
          MakeAt("(module)"_su8,
                 ScriptModule{nullopt, ScriptModuleKind::Text, Module{}}),
          MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})},
      "(module) \"msg\""_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, FloatResult) {
  ExpectRead(ReadFloatResult<f32>, F32Result{f32{0}}, "0"_su8);
  ExpectRead(ReadFloatResult<f32>, F32Result{NanKind::Arithmetic},
             "nan:arithmetic"_su8);
  ExpectRead(ReadFloatResult<f32>, F32Result{NanKind::Canonical},
             "nan:canonical"_su8);

  ExpectRead(ReadFloatResult<f64>, F64Result{f64{0}}, "0"_su8);
  ExpectRead(ReadFloatResult<f64>, F64Result{NanKind::Arithmetic},
             "nan:arithmetic"_su8);
  ExpectRead(ReadFloatResult<f64>, F64Result{NanKind::Canonical},
             "nan:canonical"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, SimdFloatResult) {
  ExpectRead(ReadSimdFloatResult<f32, 4>,
             ReturnResult{F32x4Result{
                 F32Result{f32{0}},
                 F32Result{f32{0}},
                 F32Result{f32{0}},
                 F32Result{f32{0}},
             }},
             "0 0 0 0"_su8);

  ExpectRead(ReadSimdFloatResult<f32, 4>,
             ReturnResult{F32x4Result{
                 F32Result{f32{0}},
                 F32Result{NanKind::Arithmetic},
                 F32Result{f32{0}},
                 F32Result{NanKind::Canonical},
             }},
             "0 nan:arithmetic 0 nan:canonical"_su8);

  ExpectRead(ReadSimdFloatResult<f64, 2>,
             ReturnResult{F64x2Result{
                 F64Result{f64{0}},
                 F64Result{f64{0}},
             }},
             "0 0"_su8);

  ExpectRead(ReadSimdFloatResult<f64, 2>,
             ReturnResult{F64x2Result{
                 F64Result{NanKind::Arithmetic},
                 F64Result{f64{0}},
             }},
             "nan:arithmetic 0"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ReturnResult) {
  ExpectRead(ReadReturnResult, ReturnResult{u32{0}}, "(i32.const 0)"_su8);

  ExpectRead(ReadReturnResult, ReturnResult{u64{0}}, "(i64.const 0)"_su8);

  ExpectRead(ReadReturnResult, ReturnResult{F32Result{f32{0}}},
             "(f32.const 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{F32Result{NanKind::Arithmetic}},
             "(f32.const nan:arithmetic)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{F32Result{NanKind::Canonical}},
             "(f32.const nan:canonical)"_su8);

  ExpectRead(ReadReturnResult, ReturnResult{F64Result{f64{0}}},
             "(f64.const 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{F64Result{NanKind::Arithmetic}},
             "(f64.const nan:arithmetic)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{F64Result{NanKind::Canonical}},
             "(f64.const nan:canonical)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ReturnResult_simd) {
  context.features.enable_simd();

  ExpectRead(ReadReturnResult, ReturnResult{v128{}},
             "(v128.const i8x16 0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{v128{}},
             "(v128.const i16x8 0 0 0 0  0 0 0 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{v128{}},
             "(v128.const i32x4 0 0 0 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{v128{}},
             "(v128.const i64x2 0 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{F32x4Result{}},
             "(v128.const f32x4 0 0 0 0)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{F64x2Result{}},
             "(v128.const f64x2 0 0)"_su8);

  ExpectRead(ReadReturnResult,
             ReturnResult{F32x4Result{
                 F32Result{0},
                 F32Result{NanKind::Arithmetic},
                 F32Result{0},
                 F32Result{NanKind::Canonical},
             }},
             "(v128.const f32x4 0 nan:arithmetic 0 nan:canonical)"_su8);

  ExpectRead(ReadReturnResult,
             ReturnResult{F64x2Result{
                 F64Result{0},
                 F64Result{NanKind::Arithmetic},
             }},
             "(v128.const f64x2 0 nan:arithmetic)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ReturnResult_reference_types) {
  context.features.enable_reference_types();

  ExpectRead(ReadReturnResult, ReturnResult{RefAnyResult{}}, "(ref.any)"_su8);
  ExpectRead(ReadReturnResult, ReturnResult{RefFuncResult{}}, "(ref.func)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ReturnResultList) {
  ExpectRead(ReadReturnResultList, ReturnResultList{}, ""_su8);

  ExpectRead(ReadReturnResultList,
             ReturnResultList{
                 MakeAt("(i32.const 0)"_su8, ReturnResult{u32{0}}),
                 MakeAt("(f32.const nan:canonical)"_su8,
                        ReturnResult{F32Result{NanKind::Canonical}}),
             },
             "(i32.const 0) (f32.const nan:canonical)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, ReturnAssertion) {
  ExpectRead(
      ReadReturnAssertion,
      ReturnAssertion{
          MakeAt("(invoke \"a\")"_su8,
                 Action{InvokeAction{
                     nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}}),
          {}},
      "(invoke \"a\")"_su8);

  ExpectRead(
      ReadReturnAssertion,
      ReturnAssertion{
          MakeAt("(invoke \"a\" (i32.const 0))"_su8,
                 Action{InvokeAction{
                     nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                     ConstList{MakeAt("(i32.const 0)"_su8, Const{u32{0}})}}}),
          ReturnResultList{MakeAt("(i32.const 1)"_su8, ReturnResult{u32{1}})}},
      "(invoke \"a\" (i32.const 0)) (i32.const 1)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Assertion) {
  // assert_malformed
  ExpectRead(
      ReadAssertion,
      Assertion{AssertionKind::Malformed,
                ModuleAssertion{
                    MakeAt("(module)"_su8,
                           ScriptModule{nullopt, ScriptModuleKind::Text, {}}),
                    MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})}},
      "(assert_malformed (module) \"msg\")"_su8);

  // assert_invalid
  ExpectRead(
      ReadAssertion,
      Assertion{AssertionKind::Invalid,
                ModuleAssertion{
                    MakeAt("(module)"_su8,
                           ScriptModule{nullopt, ScriptModuleKind::Text, {}}),
                    MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})}},
      "(assert_invalid (module) \"msg\")"_su8);

  // assert_unlinkable
  ExpectRead(
      ReadAssertion,
      Assertion{AssertionKind::Unlinkable,
                ModuleAssertion{
                    MakeAt("(module)"_su8,
                           ScriptModule{nullopt, ScriptModuleKind::Text, {}}),
                    MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})}},
      "(assert_unlinkable (module) \"msg\")"_su8);

  // assert_trap (module)
  ExpectRead(
      ReadAssertion,
      Assertion{AssertionKind::ModuleTrap,
                ModuleAssertion{
                    MakeAt("(module)"_su8,
                           ScriptModule{nullopt, ScriptModuleKind::Text, {}}),
                    MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})}},
      "(assert_trap (module) \"msg\")"_su8);

  // assert_return
  ExpectRead(
      ReadAssertion,
      Assertion{
          AssertionKind::Return,
          ReturnAssertion{
              MakeAt(
                  "(invoke \"a\")"_su8,
                  Action{InvokeAction{
                      nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}}),
              {}}},
      "(assert_return (invoke \"a\"))"_su8);

  // assert_trap (action)
  ExpectRead(
      ReadAssertion,
      Assertion{
          AssertionKind::ActionTrap,
          ActionAssertion{
              MakeAt(
                  "(invoke \"a\")"_su8,
                  Action{InvokeAction{
                      nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}}),
              MakeAt("\"msg\""_su8, Text{"\"msg\""_sv, 3})}},
      "(assert_trap (invoke \"a\") \"msg\")"_su8);

  // assert_exhaustion
  ExpectRead(
      ReadAssertion,
      Assertion{
          AssertionKind::Exhaustion,
          ActionAssertion{
              MakeAt(
                  "(invoke \"a\")"_su8,
                  Action{InvokeAction{
                      nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}}),
              MakeAt("\"msg\""_su8, Text{"\"msg\""_sv, 3})}},
      "(assert_exhaustion (invoke \"a\") \"msg\")"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Register) {
  ExpectRead(ReadRegister,
             Register{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), nullopt},
             "(register \"a\")"_su8);

  ExpectRead(ReadRegister,
             Register{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}),
                      MakeAt("$m"_su8, "$m"_sv)},
             "(register \"a\" $m)"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Command) {
  // Module.
  ExpectRead(ReadCommand,
             Command{ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
             "(module)"_su8);

  // Action.
  ExpectRead(ReadCommand,
             Command{InvokeAction{
                 nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}},
             "(invoke \"a\")"_su8);

  // Assertion.
  ExpectRead(ReadCommand,
             Command{Assertion{
                 AssertionKind::Invalid,
                 ModuleAssertion{
                     MakeAt("(module)"_su8,
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}}),
                     MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})}}},
             "(assert_invalid (module) \"msg\")"_su8);

  // Register.
  ExpectRead(
      ReadCommand,
      Command{Register{MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), nullopt}},
      "(register \"a\")"_su8);

  ExpectNoErrors(errors);
}

TEST_F(TextReadTest, Script) {
  ExpectReadVector(
      ReadScript,
      Script{
          MakeAt("(module)"_su8,
                 Command{ScriptModule{nullopt, ScriptModuleKind::Text, {}}}),
          MakeAt("(invoke \"a\")"_su8,
                 Command{InvokeAction{
                     nullopt, MakeAt("\"a\""_su8, Text{"\"a\""_sv, 1}), {}}}),
          MakeAt(
              "(assert_invalid (module) \"msg\")"_su8,
              Command{Assertion{
                  AssertionKind::Invalid,
                  ModuleAssertion{
                      MakeAt("(module)"_su8,
                             ScriptModule{nullopt, ScriptModuleKind::Text, {}}),
                      MakeAt("\"msg\""_su8, Text{"\"msg\"", 3})}}}),
      },
      "(module) (invoke \"a\") (assert_invalid (module) \"msg\")"_su8);

  ExpectNoErrors(errors);
}
