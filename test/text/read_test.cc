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
    ASSERT_EQ((At{span, expected}), actual);
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
      At{"$a"_su8, Var{"$a"_sv}},
      At{"$b"_su8, Var{"$b"_sv}},
      At{"1"_su8, Var{Index{1}}},
      At{"2"_su8, Var{Index{2}}},
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
      At{"\"hello, \""_su8, Text{"\"hello, \""_sv, 7}},
      At{"\"world\""_su8, Text{"\"world\""_sv, 5}},
      At{"\"123\""_su8, Text{"\"123\""_sv, 3}},
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

TEST_F(TextReadTest, HeapType_gc) {
  context.features.enable_gc();
  OK(ReadHeapType, HT_Any, "any"_su8);
  OK(ReadHeapType, HT_I31, "i31"_su8);
  OK(ReadHeapType, HT_Eq, "eq"_su8);
}

TEST_F(TextReadTest, Rtt) {
  context.features.enable_gc();
  OK(ReadRtt, RTT_0_Func, "(rtt 0 func)"_su8);
  OK(ReadRtt, RTT_0_Extern, "(rtt 0 extern)"_su8);
  OK(ReadRtt, RTT_0_Eq, "(rtt 0 eq)"_su8);
  OK(ReadRtt, RTT_0_I31, "(rtt 0 i31)"_su8);
  OK(ReadRtt, RTT_0_Any, "(rtt 0 any)"_su8);
  OK(ReadRtt, RTT_0_0, "(rtt 0 0)"_su8);
  OK(ReadRtt, RTT_0_T, "(rtt 0 $t)"_su8);
  OK(ReadRtt, RTT_1_Func, "(rtt 1 func)"_su8);
  OK(ReadRtt, RTT_1_Extern, "(rtt 1 extern)"_su8);
  OK(ReadRtt, RTT_1_Eq, "(rtt 1 eq)"_su8);
  OK(ReadRtt, RTT_1_I31, "(rtt 1 i31)"_su8);
  OK(ReadRtt, RTT_1_Any, "(rtt 1 any)"_su8);
  OK(ReadRtt, RTT_1_0, "(rtt 1 0)"_su8);
  OK(ReadRtt, RTT_1_T, "(rtt 1 $t)"_su8);
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

TEST_F(TextReadTest, ValueType_gc) {
  context.features.enable_gc();

  // New reference types
  OK(ReadValueType, VT_Eqref, "eqref"_su8);
  OK(ReadValueType, VT_I31ref, "i31ref"_su8);
  OK(ReadValueType, VT_Anyref, "anyref"_su8);
  OK(ReadValueType, VT_RefAny, "(ref any)"_su8);
  OK(ReadValueType, VT_RefNullAny, "(ref null any)"_su8);
  OK(ReadValueType, VT_RefEq, "(ref eq)"_su8);
  OK(ReadValueType, VT_RefNullEq, "(ref null eq)"_su8);
  OK(ReadValueType, VT_RefI31, "(ref i31)"_su8);
  OK(ReadValueType, VT_RefNullI31, "(ref null i31)"_su8);

  // RTT
  OK(ReadValueType, VT_RTT_0_Func, "(rtt 0 func)"_su8);
  OK(ReadValueType, VT_RTT_0_Extern, "(rtt 0 extern)"_su8);
  OK(ReadValueType, VT_RTT_0_Eq, "(rtt 0 eq)"_su8);
  OK(ReadValueType, VT_RTT_0_I31, "(rtt 0 i31)"_su8);
  OK(ReadValueType, VT_RTT_0_Any, "(rtt 0 any)"_su8);
  OK(ReadValueType, VT_RTT_0_0, "(rtt 0 0)"_su8);
  OK(ReadValueType, VT_RTT_0_T, "(rtt 0 $t)"_su8);
  OK(ReadValueType, VT_RTT_1_Func, "(rtt 1 func)"_su8);
  OK(ReadValueType, VT_RTT_1_Extern, "(rtt 1 extern)"_su8);
  OK(ReadValueType, VT_RTT_1_Eq, "(rtt 1 eq)"_su8);
  OK(ReadValueType, VT_RTT_1_I31, "(rtt 1 i31)"_su8);
  OK(ReadValueType, VT_RTT_1_Any, "(rtt 1 any)"_su8);
  OK(ReadValueType, VT_RTT_1_0, "(rtt 1 0)"_su8);
  OK(ReadValueType, VT_RTT_1_T, "(rtt 1 $t)"_su8);
}

TEST_F(TextReadTest, ValueTypeList) {
  auto span = "i32 f32 f64 i64"_su8;
  std::vector<At<ValueType>> expected{
      At{"i32"_su8, VT_I32},
      At{"f32"_su8, VT_F32},
      At{"f64"_su8, VT_F64},
      At{"i64"_su8, VT_I64},
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

TEST_F(TextReadTest, ReferenceType_gc) {
  context.features.enable_gc();
  OK(ReadReferenceType, RT_Eqref, "eqref"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_I31ref, "i31ref"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_Anyref, "anyref"_su8, AllowFuncref::Yes);

  OK(ReadReferenceType, RT_RefEq, "(ref eq)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNullEq, "(ref null eq)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefI31, "(ref i31)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNullI31, "(ref null i31)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefAny, "(ref any)"_su8, AllowFuncref::Yes);
  OK(ReadReferenceType, RT_RefNullAny, "(ref null any)"_su8, AllowFuncref::Yes);
}

TEST_F(TextReadTest, BoundParamList) {
  auto span = "(param i32 f32) (param $foo i64) (param)"_su8;
  std::vector<At<BoundValueType>> expected{
      At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}},
      At{"f32"_su8, BVT{nullopt, At{"f32"_su8, VT_F32}}},
      At{"$foo i64"_su8, BVT{At{"$foo"_su8, "$foo"_sv}, At{"i64"_su8, VT_I64}}},
  };

  OKVector(ReadBoundParamList, expected, span);
}

TEST_F(TextReadTest, ParamList) {
  auto span = "(param i32 f32) (param i64) (param)"_su8;
  std::vector<At<ValueType>> expected{
      At{"i32"_su8, VT_I32},
      At{"f32"_su8, VT_F32},
      At{"i64"_su8, VT_I64},
  };
  OKVector(ReadParamList, expected, span);
}

TEST_F(TextReadTest, ResultList) {
  auto span = "(result i32 f32) (result i64) (result)"_su8;
  std::vector<At<ValueType>> expected{
      At{"i32"_su8, VT_I32},
      At{"f32"_su8, VT_F32},
      At{"i64"_su8, VT_I64},
  };
  OKVector(ReadResultList, expected, span);
}

TEST_F(TextReadTest, LocalList) {
  auto span = "(local i32 f32) (local $foo i64) (local)"_su8;
  std::vector<At<BoundValueType>> expected{
      At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}},
      At{"f32"_su8, BVT{nullopt, At{"f32"_su8, VT_F32}}},
      At{"$foo i64"_su8, BVT{At{"$foo"_su8, "$foo"_sv}, At{"i64"_su8, VT_I64}}},
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
     FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}, "(type 0)"_su8);

  // Function type.
  OK(ReadFunctionTypeUse,
     FunctionTypeUse{nullopt, At{"(param i32 f32) (result f64)"_su8,
                                 FunctionType{{At{"i32"_su8, VT_I32},
                                               At{"f32"_su8, VT_F32}},
                                              {At{"f64"_su8, VT_F64}}}}},
     "(param i32 f32) (result f64)"_su8);

  // Type use and function type.
  OK(ReadFunctionTypeUse,
     FunctionTypeUse{
         At{"(type $t)"_su8, Var{"$t"_sv}},
         At{"(result i32)"_su8, FunctionType{{}, {At{"i32"_su8, VT_I32}}}}},
     "(type $t) (result i32)"_su8);
}

TEST_F(TextReadTest, InlineImport) {
  OK(ReadInlineImportOpt,
     InlineImport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                  At{"\"n\""_su8, Text{"\"n\""_sv, 1}}},
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
  OK(ReadInlineExport, InlineExport{At{"\"n\""_su8, Text{"\"n\""_sv, 1}}},
     R"((export "n"))"_su8);
}

TEST_F(TextReadTest, InlineExportList) {
  OKVector(ReadInlineExportList,
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}},
               At{"(export \"n\")"_su8,
                  InlineExport{At{"\"n\""_su8, Text{"\"n\""_sv, 1}}}},
           },
           R"((export "m") (export "n"))"_su8);
}

TEST_F(TextReadTest, BoundFunctionType) {
  SpanU8 span =
      "(param i32 i32) (param $t i64) (result f32 f32) (result f64)"_su8;
  OK(ReadBoundFunctionType,
     BoundFunctionType{
         {At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}},
          At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}},
          At{"$t i64"_su8, BVT{At{"$t"_su8, "$t"_sv}, At{"i64"_su8, VT_I64}}}},
         {At{"f32"_su8, VT_F32}, At{"f32"_su8, VT_F32}, At{"f64"_su8, VT_F64}}},
     span);
}

TEST_F(TextReadTest, FunctionType) {
  SpanU8 span = "(param i32 i32) (param i64) (result f32 f32) (result f64)"_su8;
  OK(ReadFunctionType,
     FunctionType{
         {At{"i32"_su8, VT_I32}, At{"i32"_su8, VT_I32}, At{"i64"_su8, VT_I64}},
         {At{"f32"_su8, VT_F32}, At{"f32"_su8, VT_F32}, At{"f64"_su8, VT_F64}}},
     span);
}

TEST_F(TextReadTest, StorageType) {
  context.features.enable_gc();
  // Numeric type
  OK(ReadStorageType, StorageType{At{"i32"_su8, VT_I32}}, "i32"_su8);

  // Reference type
  OK(ReadStorageType, StorageType{At{"funcref"_su8, VT_Funcref}},
     "funcref"_su8);

  // Packed type
  OK(ReadStorageType, StorageType{At{"i8"_su8, PackedType::I8}}, "i8"_su8);
  OK(ReadStorageType, StorageType{At{"i16"_su8, PackedType::I16}}, "i16"_su8);
}

TEST_F(TextReadTest, FieldType) {
  context.features.enable_gc();

  // No name
  OK(ReadFieldType,
     FieldType{nullopt, At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
               Mutability::Const},
     "(field i32)"_su8);

  // Name
  OK(ReadFieldType,
     FieldType{At{"$a"_su8, "$a"_sv},
               At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
               Mutability::Const},
     "(field $a i32)"_su8);

  // Mutable field
  OK(ReadFieldType,
     FieldType{nullopt, At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
               At{"mut"_su8, Mutability::Var}},
     "(field (mut i32))"_su8);

  // Packed type
  OK(ReadFieldType,
     FieldType{nullopt, At{"i8"_su8, StorageType{At{"i8"_su8, PackedType::I8}}},
               Mutability::Const},
     "(field i8)"_su8);

  // Reference type
  OK(ReadFieldType,
     FieldType{nullopt,
               At{"(ref null any)"_su8,
                  StorageType{At{"(ref null any)"_su8, VT_RefNullAny}}},
               Mutability::Const},
     "(field (ref null any))"_su8);
}

TEST_F(TextReadTest, FieldTypeList) {
  context.features.enable_gc();

  // Single field
  OK(ReadFieldTypeList,
     FieldTypeList{At{
         "i32"_su8,
         FieldType{nullopt, At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                   Mutability::Const}}},
     "(field i32)"_su8);

  // Packed field
  OK(ReadFieldTypeList,
     FieldTypeList{
         At{"i8"_su8,
            FieldType{nullopt,
                      At{"i8"_su8, StorageType{At{"i8"_su8, PackedType::I8}}},
                      Mutability::Const}}},
     "(field i8)"_su8);

  // Combined fields
  OK(ReadFieldTypeList,
     FieldTypeList{
         At{"i32"_su8,
            FieldType{nullopt,
                      At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                      Mutability::Const}},
         At{"i64"_su8,
            FieldType{nullopt,
                      At{"i64"_su8, StorageType{At{"i64"_su8, VT_I64}}},
                      Mutability::Const}}},
     "(field i32 i64)"_su8);

  // Separate fields
  OK(ReadFieldTypeList,
     FieldTypeList{
         At{"i32"_su8,
            FieldType{nullopt,
                      At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                      Mutability::Const}},
         At{"i64"_su8,
            FieldType{nullopt,
                      At{"i64"_su8, StorageType{At{"i64"_su8, VT_I64}}},
                      Mutability::Const}}},
     "(field i32) (field i64)"_su8);

  // Bound fields
  OK(ReadFieldTypeList,
     FieldTypeList{
         At{"$a i32"_su8,
            FieldType{At{"$a"_su8, "$a"_sv},
                      At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                      Mutability::Const}},
         At{"$b i64"_su8,
            FieldType{At{"$b"_su8, "$b"_sv},
                      At{"i64"_su8, StorageType{At{"i64"_su8, VT_I64}}},
                      Mutability::Const}}},
     "(field $a i32) (field $b i64)"_su8);
}

TEST_F(TextReadTest, StructType) {
  OK(ReadStructType,
     StructType{FieldTypeList{
         At{"i32"_su8,
            FieldType{nullopt,
                      At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                      Mutability::Const}},
         At{"f32"_su8,
            FieldType{nullopt,
                      At{"f32"_su8, StorageType{At{"f32"_su8, VT_F32}}},
                      Mutability::Const}}}},
     "(struct (field i32 f32))"_su8);
}

TEST_F(TextReadTest, ArrayType) {
  OK(ReadArrayType,
     ArrayType{At{
         "(field i32)"_su8,
         FieldType{nullopt, At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                   Mutability::Const}}},
     "(array (field i32))"_su8);
}

TEST_F(TextReadTest, DefinedType) {
  OK(ReadDefinedType, DefinedType{nullopt, BoundFunctionType{{}, {}}},
     "(type (func))"_su8);

  OK(ReadDefinedType,
     DefinedType{
         At{"$foo"_su8, "$foo"_sv},
         At{"(param $bar i32) (result i64)"_su8,
            BoundFunctionType{{At{"$bar i32"_su8, BVT{At{"$bar"_su8, "$bar"_sv},
                                                      At{"i32"_su8, VT_I32}}}},
                              {At{"i64"_su8, VT_I64}}}}},
     "(type $foo (func (param $bar i32) (result i64)))"_su8);
}

TEST_F(TextReadTest, DefinedType_GC) {
  context.features.enable_gc();

  // Empty struct
  OK(ReadDefinedType, DefinedType{nullopt, At{"(struct)"_su8, StructType{}}},
     "(type (struct))"_su8);

  // Simple array
  OK(ReadDefinedType,
     DefinedType{
         nullopt,
         At{"(array (field i32))"_su8,
            ArrayType{
                At{"(field i32)"_su8,
                   FieldType{nullopt,
                             At{"i32"_su8, StorageType{At{"i32"_su8, VT_I32}}},
                             Mutability::Const}}}}},
     "(type (array (field i32)))"_su8);

  // Recursive types
  OK(ReadDefinedType,
     DefinedType{At{"$t"_su8, "$t"_sv},
                 At{"(struct (field (ref $t)))"_su8,
                    StructType{FieldTypeList{At{
                        "(ref $t)"_su8,
                        FieldType{nullopt,
                                  At{"(ref $t)"_su8,
                                     StorageType{At{"(ref $t)"_su8, VT_RefT}}},
                                  Mutability::Const}}}}}},
     "(type $t (struct (field (ref $t))))"_su8);

  OK(ReadDefinedType,
     DefinedType{
         At{"$t"_su8, "$t"_sv},
         At{"(array (field (ref $t)))"_su8,
            ArrayType{At{"(field (ref $t))"_su8,
                         FieldType{nullopt,
                                   At{"(ref $t)"_su8,
                                      StorageType{At{"(ref $t)"_su8, VT_RefT}}},
                                   Mutability::Const}}}}},
     "(type $t (array (field (ref $t))))"_su8);
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
  OK(ReadLimits, Limits{At{"1"_su8, 1u}}, "1"_su8, LimitsKind::Memory);
  OK(ReadLimits, Limits{At{"1"_su8, 1u}, At{"0x11"_su8, 17u}}, "1 0x11"_su8,
     LimitsKind::Memory);
}

TEST_F(TextReadTest, Limits_threads) {
  context.features.enable_threads();

  OK(ReadLimits,
     Limits{At{"0"_su8, 0u}, At{"20"_su8, 20u}, At{"shared"_su8, Shared::Yes}},
     "0 20 shared"_su8, LimitsKind::Memory);
}

TEST_F(TextReadTest, Limits_memory64) {
  Fail(ReadLimits, {{0, "Expected a natural number, got NumericType"}},
       "i32 1"_su8, LimitsKind::Memory);

  context.features.enable_memory64();

  OK(ReadLimits,
     Limits{At{"1"_su8, 1u}, nullopt, Shared::No,
            At{"i32"_su8, IndexType::I32}},
     "i32 1"_su8, LimitsKind::Memory);

  OK(ReadLimits,
     Limits{At{"1"_su8, 1u}, At{"2"_su8, 2u}, Shared::No,
            At{"i32"_su8, IndexType::I32}},
     "i32 1 2"_su8, LimitsKind::Memory);

  OK(ReadLimits,
     Limits{At{"1"_su8, 1u}, nullopt, Shared::No,
            At{"i64"_su8, IndexType::I64}},
     "i32 1"_su8, LimitsKind::Memory);

  OK(ReadLimits,
     Limits{At{"1"_su8, 1u}, At{"2"_su8, 2u}, Shared::No,
            At{"i64"_su8, IndexType::I64}},
     "i32 1 2"_su8, LimitsKind::Memory);
}

TEST_F(TextReadTest, Limits_No64BitShared) {
  context.features.enable_threads();
  context.features.enable_memory64();

  Fail(ReadLimits, {{8, "limits cannot be shared and have i64 index"}},
       "i64 1 2 shared"_su8, LimitsKind::Memory);
}

TEST_F(TextReadTest, BlockImmediate) {
  // empty block type.
  OK(ReadBlockImmediate, BlockImmediate{}, ""_su8);

  // block type w/ label.
  OK(ReadBlockImmediate, BlockImmediate{At{"$l"_su8, BindVar{"$l"_sv}}, {}},
     "$l"_su8);

  // block type w/ function type use.
  OK(ReadBlockImmediate,
     BlockImmediate{nullopt,
                    FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
     "(type 0)"_su8);

  // block type w/ label and function type use.
  OK(ReadBlockImmediate,
     BlockImmediate{At{"$l2"_su8, BindVar{"$l2"_sv}},
                    FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
     "$l2 (type 0)"_su8);
}

TEST_F(TextReadTest, BlockImmediate_InlineType) {
  OK(ReadBlockImmediate, BlockImmediate{}, ""_su8);

  struct {
    At<ValueType> value_type;
    SpanU8 span;
  } tests[] = {
      {At{"i32"_su8, VT_I32}, "(result i32)"_su8},
      {At{"i64"_su8, VT_I64}, "(result i64)"_su8},
      {At{"f32"_su8, VT_F32}, "(result f32)"_su8},
      {At{"f64"_su8, VT_F64}, "(result f64)"_su8},
  };

  for (auto& test : tests) {
    OK(ReadBlockImmediate,
       BlockImmediate{
           nullopt,
           FunctionTypeUse{nullopt,
                           At{test.span, FunctionType{{}, {test.value_type}}}}},
       test.span);
  }
}

TEST_F(TextReadTest, LetImmediate) {
  // empty let immediate.
  OK(ReadLetImmediate, LetImmediate{}, ""_su8);

  // label, no locals
  OK(ReadLetImmediate,
     LetImmediate{BlockImmediate{At{"$l"_su8, BindVar{"$l"_sv}}, {}}, {}},
     "$l"_su8);

  // type use, locals
  OK(ReadLetImmediate,
     LetImmediate{
         BlockImmediate{nullopt,
                        FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
         BoundValueTypeList{
             At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}}}},
     "(type 0) (local i32)"_su8);

  // inline type, multiple locals
  OK(ReadLetImmediate,
     LetImmediate{BlockImmediate{
                      nullopt,
                      FunctionTypeUse{
                          nullopt, At{"(param i32)"_su8,
                                      FunctionType{
                                          ValueTypeList{At{"i32"_su8, VT_I32}},
                                          {},
                                      }}}},
                  BoundValueTypeList{
                      At{"f32"_su8, BVT{nullopt, At{"f32"_su8, VT_F32}}},
                      At{"f64"_su8, BVT{nullopt, At{"f64"_su8, VT_F64}}}}},
     "(param i32) (local f32 f64)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Bare) {
  OK(ReadPlainInstruction, I{At{"nop"_su8, O::Nop}}, "nop"_su8);
  OK(ReadPlainInstruction, I{At{"i32.add"_su8, O::I32Add}}, "i32.add"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Var) {
  OK(ReadPlainInstruction, I{At{"br"_su8, O::Br}, At{"0"_su8, Var{Index{0}}}},
     "br 0"_su8);
  OK(ReadPlainInstruction,
     I{At{"local.get"_su8, O::LocalGet}, At{"$x"_su8, Var{"$x"_sv}}},
     "local.get $x"_su8);
}

TEST_F(TextReadTest, PlainInstruction_BrOnExn) {
  context.features.enable_exceptions();
  OK(ReadPlainInstruction,
     I{At{"br_on_exn"_su8, O::BrOnExn},
       At{"$l $e"_su8, BrOnExnImmediate{At{"$l"_su8, Var{"$l"_sv}},
                                        At{"$e"_su8, Var{"$e"_sv}}}}},
     "br_on_exn $l $e"_su8);
}

TEST_F(TextReadTest, PlainInstruction_BrTable) {
  // br_table w/ only default target.
  OK(ReadPlainInstruction,
     I{At{"br_table"_su8, O::BrTable},
       At{"0"_su8, BrTableImmediate{{}, At{"0"_su8, Var{Index{0}}}}}},
     "br_table 0"_su8);

  // br_table w/ targets and default target.
  OK(ReadPlainInstruction,
     I{At{"br_table"_su8, O::BrTable},
       At{"0 1 $a $b"_su8, BrTableImmediate{{At{"0"_su8, Var{Index{0}}},
                                             At{"1"_su8, Var{Index{1}}},
                                             At{"$a"_su8, Var{"$a"_sv}}},
                                            At{"$b"_su8, Var{"$b"_sv}}}}},
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
     I{At{"call_indirect"_su8, O::CallIndirect},
       At{""_su8, CallIndirectImmediate{}}},
     "call_indirect"_su8);

  // call_indirect w/ function type use.
  OK(ReadPlainInstruction,
     I{At{"call_indirect"_su8, O::CallIndirect},
       At{"(type 0)"_su8,
          CallIndirectImmediate{
              nullopt,
              FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}}}},
     "call_indirect (type 0)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_CallIndirect_reference_types) {
  // In the reference types proposal, the call_indirect instruction also allows
  // a table var first.
  context.features.enable_reference_types();

  // call_indirect w/ table.
  OK(ReadPlainInstruction,
     I{At{"call_indirect"_su8, O::CallIndirect},
       At{"$t"_su8, CallIndirectImmediate{At{"$t"_su8, Var{"$t"_sv}}, {}}}},
     "call_indirect $t"_su8);

  // call_indirect w/ table and type use.
  OK(ReadPlainInstruction,
     I{At{"call_indirect"_su8, O::CallIndirect},
       At{"0 (type 0)"_su8,
          CallIndirectImmediate{
              At{"0"_su8, Var{Index{0}}},
              FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}}}},
     "call_indirect 0 (type 0)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Const) {
  // i32.const
  OK(ReadPlainInstruction,
     I{At{"i32.const"_su8, O::I32Const}, At{"12"_su8, s32{12}}},
     "i32.const 12"_su8);

  // i64.const
  OK(ReadPlainInstruction,
     I{At{"i64.const"_su8, O::I64Const}, At{"34"_su8, s64{34}}},
     "i64.const 34"_su8);

  // f32.const
  OK(ReadPlainInstruction,
     I{At{"f32.const"_su8, O::F32Const}, At{"56"_su8, f32{56}}},
     "f32.const 56"_su8);

  // f64.const
  OK(ReadPlainInstruction,
     I{At{"f64.const"_su8, O::F64Const}, At{"78"_su8, f64{78}}},
     "f64.const 78"_su8);
}

TEST_F(TextReadTest, PlainInstruction_FuncBind) {
  context.features.enable_function_references();

  // Bare func.bind
  OK(ReadPlainInstruction,
     I{At{"func.bind"_su8, O::FuncBind}, At{""_su8, FuncBindImmediate{}}},
     "func.bind"_su8);

  // func.bind w/ function type use.
  OK(ReadPlainInstruction,
     I{At{"func.bind"_su8, O::FuncBind},
       At{"(type 0)"_su8, FuncBindImmediate{FunctionTypeUse{
                              At{"(type 0)"_su8, Var{Index{0}}}, {}}}}},
     "func.bind (type 0)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_MemArg) {
  // No align, no offset.
  OK(ReadPlainInstruction,
     I{At{"i32.load"_su8, O::I32Load},
       At{""_su8, MemArgImmediate{nullopt, nullopt}}},
     "i32.load"_su8);

  // No align, offset.
  OK(ReadPlainInstruction,
     I{At{"f32.load"_su8, O::F32Load},
       At{"offset=12"_su8,
          MemArgImmediate{nullopt, At{"offset=12"_su8, u32{12}}}}},
     "f32.load offset=12"_su8);

  // Align, no offset.
  OK(ReadPlainInstruction,
     I{At{"i32.load8_u"_su8, O::I32Load8U},
       At{"align=16"_su8,
          MemArgImmediate{At{"align=16"_su8, u32{16}}, nullopt}}},
     "i32.load8_u align=16"_su8);

  // Align and offset.
  OK(ReadPlainInstruction,
     I{At{"f64.store"_su8, O::F64Store},
       At{"offset=123 align=32"_su8,
          MemArgImmediate{At{"align=32"_su8, u32{32}},
                          At{"offset=123"_su8, u32{123}}}}},
     "f64.store offset=123 align=32"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Select) {
  OK(ReadPlainInstruction,
     I{At{"select"_su8, O::Select}, At{""_su8, SelectImmediate{}}},
     "select"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Select_reference_types) {
  context.features.enable_reference_types();

  // select w/o types
  OK(ReadPlainInstruction,
     I{At{"select"_su8, O::Select}, At{""_su8, SelectImmediate{}}},
     "select"_su8);

  // select w/ one type
  OK(ReadPlainInstruction,
     I{At{"select"_su8, O::SelectT},
       At{"(result i32)"_su8, SelectImmediate{At{"i32"_su8, VT_I32}}}},
     "select (result i32)"_su8);

  // select w/ multiple types
  OK(ReadPlainInstruction,
     I{At{"select"_su8, O::SelectT},
       At{"(result i32) (result i64)"_su8,
          SelectImmediate{At{"i32"_su8, VT_I32}, At{"i64"_su8, VT_I64}}}},
     "select (result i32) (result i64)"_su8);
}

TEST_F(TextReadTest, PlainInstruction_SimdConst) {
  Fail(ReadPlainInstruction, {{0, "v128.const instruction not allowed"}},
       "v128.const i32x4 0 0 0 0"_su8);

  context.features.enable_simd();

  // i8x16
  OK(ReadPlainInstruction,
     I{At{"v128.const"_su8, O::V128Const},
       At{"0 1 2 3 4 5 6 7 8 9 0xa 0xb 0xc 0xd 0xe 0xf"_su8,
          v128{u8x16{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe,
                     0xf}}}},
     "v128.const i8x16 0 1 2 3 4 5 6 7 8 9 0xa 0xb 0xc 0xd 0xe 0xf"_su8);

  // i16x8
  OK(ReadPlainInstruction,
     I{At{"v128.const"_su8, O::V128Const},
       At{"0 1 2 3 4 5 6 7"_su8, v128{u16x8{0, 1, 2, 3, 4, 5, 6, 7}}}},
     "v128.const i16x8 0 1 2 3 4 5 6 7"_su8);

  // i32x4
  OK(ReadPlainInstruction,
     I{At{"v128.const"_su8, O::V128Const},
       At{"0 1 2 3"_su8, v128{u32x4{0, 1, 2, 3}}}},
     "v128.const i32x4 0 1 2 3"_su8);

  // i64x2
  OK(ReadPlainInstruction,
     I{At{"v128.const"_su8, O::V128Const}, At{"0 1"_su8, v128{u64x2{0, 1}}}},
     "v128.const i64x2 0 1"_su8);

  // f32x4
  OK(ReadPlainInstruction,
     I{At{"v128.const"_su8, O::V128Const},
       At{"0 1 2 3"_su8, v128{f32x4{0, 1, 2, 3}}}},
     "v128.const f32x4 0 1 2 3"_su8);

  // f64x2
  OK(ReadPlainInstruction,
     I{At{"v128.const"_su8, O::V128Const}, At{"0 1"_su8, v128{f64x2{0, 1}}}},
     "v128.const f64x2 0 1"_su8);
}

TEST_F(TextReadTest, PlainInstruction_SimdLane) {
  Fail(ReadPlainInstruction,
       {{0, "i8x16.extract_lane_s instruction not allowed"}},
       "i8x16.extract_lane_s 0"_su8);

  context.features.enable_simd();

  OK(ReadPlainInstruction,
     I{At{"i8x16.extract_lane_s"_su8, O::I8X16ExtractLaneS},
       At{"9"_su8, SimdLaneImmediate{9}}},
     "i8x16.extract_lane_s 9"_su8);
  OK(ReadPlainInstruction,
     I{At{"f32x4.replace_lane"_su8, O::F32X4ReplaceLane},
       At{"3"_su8, SimdLaneImmediate{3}}},
     "f32x4.replace_lane 3"_su8);
}

TEST_F(TextReadTest, InvalidSimdLane) {
  Fail(ReadSimdLane, {{0, "Expected a natural number, got Int"}}, "-1"_su8);
  Fail(ReadSimdLane, {{0, "Invalid natural number, got Nat"}}, "256"_su8);
}

TEST_F(TextReadTest, PlainInstruction_Shuffle) {
  Fail(ReadPlainInstruction, {{0, "i8x16.shuffle instruction not allowed"}},
       "i8x16.shuffle 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_su8);

  context.features.enable_simd();

  OK(ReadPlainInstruction,
     I{At{"i8x16.shuffle"_su8, O::I8X16Shuffle},
       At{"0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_su8, ShuffleImmediate{}}},
     "i8x16.shuffle 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_su8);
}

TEST_F(TextReadTest, PlainInstruction_MemoryCopy) {
  Fail(ReadPlainInstruction, {{0, "memory.copy instruction not allowed"}},
       "memory.copy"_su8);

  // memory.copy w/o dst and src.
  context.features.enable_bulk_memory();
  OK(ReadPlainInstruction,
     I{At{"memory.copy"_su8, O::MemoryCopy}, At{CopyImmediate{}}},
     "memory.copy"_su8);
}

TEST_F(TextReadTest, PlainInstruction_MemoryInit) {
  Fail(ReadPlainInstruction, {{0, "memory.init instruction not allowed"}},
       "memory.init 0"_su8);

  context.features.enable_bulk_memory();

  // memory.init w/ just segment index.
  OK(ReadPlainInstruction,
     I{At{"memory.init"_su8, O::MemoryInit},
       At{"2"_su8, InitImmediate{At{"2"_su8, Var{Index{2}}}, nullopt}}},
     "memory.init 2"_su8);
}

TEST_F(TextReadTest, PlainInstruction_TableCopy) {
  Fail(ReadPlainInstruction, {{0, "table.copy instruction not allowed"}},
       "table.copy"_su8);

  // table.copy w/o dst and src.
  context.features.enable_bulk_memory();
  OK(ReadPlainInstruction,
     I{At{"table.copy"_su8, O::TableCopy}, At{""_su8, CopyImmediate{}}},
     "table.copy"_su8);
}

TEST_F(TextReadTest, PlainInstruction_TableCopy_reference_types) {
  context.features.enable_reference_types();

  // table.copy w/o dst and src.
  OK(ReadPlainInstruction,
     I{At{"table.copy"_su8, O::TableCopy}, At{""_su8, CopyImmediate{}}},
     "table.copy"_su8);

  // table.copy w/ dst and src
  OK(ReadPlainInstruction,
     I{At{"table.copy"_su8, O::TableCopy},
       At{"$d $s"_su8, CopyImmediate{At{"$d"_su8, Var{"$d"_sv}},
                                     At{"$s"_su8, Var{"$s"_sv}}}}},
     "table.copy $d $s"_su8);
}

TEST_F(TextReadTest, PlainInstruction_TableInit) {
  Fail(ReadPlainInstruction, {{0, "table.init instruction not allowed"}},
       "table.init 0"_su8);

  context.features.enable_bulk_memory();

  // table.init w/ segment index and table index.
  OK(ReadPlainInstruction,
     I{At{"table.init"_su8, O::TableInit},
       At{"$t $e"_su8, InitImmediate{At{"$e"_su8, Var{"$e"_sv}},
                                     At{"$t"_su8, Var{"$t"_sv}}}}},
     "table.init $t $e"_su8);

  // table.init w/ just segment index.
  OK(ReadPlainInstruction,
     I{At{"table.init"_su8, O::TableInit},
       At{"2"_su8, InitImmediate{At{"2"_su8, Var{Index{2}}}, nullopt}}},
     "table.init 2"_su8);
}

TEST_F(TextReadTest, PlainInstruction_RefNull) {
  Fail(ReadPlainInstruction, {{0, "ref.null instruction not allowed"}},
       "ref.null extern"_su8);

  context.features.enable_reference_types();

  OK(ReadPlainInstruction,
     I{At{"ref.null"_su8, O::RefNull}, At{"extern"_su8, HT_Extern}},
     "ref.null extern"_su8);
}

TEST_F(TextReadTest, PlainInstruction_BrOnCast) {
#if 0
  Fail(ReadPlainInstruction, {{0, "br_on_cast instruction not allowed"}},
       "br_on_cast 0 0 0"_su8);

  context.features.enable_gc();

  OK(ReadPlainInstruction,
     I{At{"br_on_cast"_su8, O::BrOnCast},
       At{"0 0 0"_su8,
          BrOnCastImmediate{
              At{"0"_su8, Var{0u}},
              At{"0 0"_su8,
                 HeapType2Immediate{At{"0"_su8, HT_0}, At{"0"_su8, HT_0}}}}}},
     "br_on_cast 0 0 0"_su8);

  OK(ReadPlainInstruction,
     I{At{"br_on_cast"_su8, O::BrOnCast},
       At{"$d $t $t"_su8,
          BrOnCastImmediate{
              At{"$d"_su8, Var{"$d"_sv}},
              At{"$t $t"_su8,
                 HeapType2Immediate{At{"$t"_su8, HT_T}, At{"$t"_su8, HT_T}}}}}},
     "br_on_cast $d $t $t"_su8);
#else
  Fail(ReadPlainInstruction, {{0, "br_on_cast instruction not allowed"}},
       "br_on_cast 0"_su8);

  context.features.enable_gc();

  OK(ReadPlainInstruction,
     I{At{"br_on_cast"_su8, O::BrOnCast}, At{"0"_su8, Var{0u}}},
     "br_on_cast 0"_su8);

  OK(ReadPlainInstruction,
     I{At{"br_on_cast"_su8, O::BrOnCast}, At{"$d"_su8, Var{"$d"_sv}}},
     "br_on_cast $d"_su8);

#endif
}

TEST_F(TextReadTest, PlainInstruction_HeapType2) {
  Fail(ReadPlainInstruction, {{0, "ref.test instruction not allowed"}},
       "ref.test 0 0"_su8);

  context.features.enable_gc();

  OK(ReadPlainInstruction,
     I{At{"ref.test"_su8, O::RefTest},
       At{"0 0"_su8, HeapType2Immediate{At{"0"_su8, HT_0}, At{"0"_su8, HT_0}}}},
     "ref.test 0 0"_su8);

  OK(ReadPlainInstruction,
     I{At{"ref.test"_su8, O::RefTest},
       At{"$t $t"_su8,
          HeapType2Immediate{At{"$t"_su8, HT_T}, At{"$t"_su8, HT_T}}}},
     "ref.test $t $t"_su8);
}

TEST_F(TextReadTest, PlainInstruction_RttSub) {
#if 0
  Fail(ReadPlainInstruction, {{0, "rtt.sub instruction not allowed"}},
       "rtt.sub 0 0 0"_su8);

  context.features.enable_gc();

  OK(ReadPlainInstruction,
     I{At{"rtt.sub"_su8, O::RttSub},
       At{"0 0 0"_su8,
          RttSubImmediate{
              At{"0"_su8, 0u},
              At{"0 0"_su8,
                 HeapType2Immediate{At{"0"_su8, HT_0}, At{"0"_su8, HT_0}}}}}},
     "rtt.sub 0 0 0"_su8);

  OK(ReadPlainInstruction,
     I{At{"rtt.sub"_su8, O::RttSub},
       At{"0 $t $t"_su8,
          RttSubImmediate{
              At{"0"_su8, 0u},
              At{"$t $t"_su8,
                 HeapType2Immediate{At{"$t"_su8, HT_T}, At{"$t"_su8, HT_T}}}}}},
     "rtt.sub 0 $t $t"_su8);
#else
  Fail(ReadPlainInstruction, {{0, "rtt.sub instruction not allowed"}},
       "rtt.sub 0"_su8);

  context.features.enable_gc();

  OK(ReadPlainInstruction, I{At{"rtt.sub"_su8, O::RttSub}, At{"0"_su8, HT_0}},
     "rtt.sub 0"_su8);

  OK(ReadPlainInstruction, I{At{"rtt.sub"_su8, O::RttSub}, At{"$t"_su8, HT_T}},
     "rtt.sub $t"_su8);
#endif
}

TEST_F(TextReadTest, PlainInstruction_StructField) {
  Fail(ReadPlainInstruction, {{0, "struct.get instruction not allowed"}},
       "struct.get 0 0"_su8);

  context.features.enable_gc();

  OK(ReadPlainInstruction,
     I{At{"struct.get"_su8, O::StructGet},
       At{"0 0"_su8,
          StructFieldImmediate{At{"0"_su8, Var{0u}}, At{"0"_su8, Var{0u}}}}},
     "struct.get 0 0"_su8);

  OK(ReadPlainInstruction,
     I{At{"struct.get"_su8, O::StructGet},
       At{"$t $t"_su8, StructFieldImmediate{At{"$t"_su8, Var{"$t"_sv}},
                                            At{"$t"_su8, Var{"$t"_sv}}}}},
     "struct.get $t $t"_su8);
}

TEST_F(TextReadTest, BlockInstruction_Block) {
  // Empty block.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"block"_su8, I{At{"block"_su8, O::Block}, BlockImmediate{}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "block end"_su8);

  // block w/ multiple instructions.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"block"_su8, I{At{"block"_su8, O::Block}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "block nop nop end"_su8);

  // Block w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"block $l"_su8,
                  I{At{"block"_su8, O::Block},
                    At{"$l"_su8, BlockImmediate{At{"$l"_su8, "$l"_sv}, {}}}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "block $l nop end"_su8);

  // Block w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          At{"block $l2"_su8,
             I{At{"block"_su8, O::Block},
               At{"$l2"_su8, BlockImmediate{At{"$l2"_su8, "$l2"_sv}, {}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
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
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"loop"_su8, I{At{"loop"_su8, O::Loop}, BlockImmediate{}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "loop end"_su8);

  // loop w/ multiple instructions.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"loop"_su8, I{At{"loop"_su8, O::Loop}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "loop nop nop end"_su8);

  // Loop w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"loop $l"_su8,
                  I{At{"loop"_su8, O::Loop},
                    At{"$l"_su8, BlockImmediate{At{"$l"_su8, "$l"_sv}, {}}}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "loop $l nop end"_su8);

  // Loop w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          At{"loop $l2"_su8,
             I{At{"loop"_su8, O::Loop},
               At{"$l2"_su8, BlockImmediate{At{"$l2"_su8, "$l2"_sv}, {}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
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
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "if end"_su8);

  // if w/ non-empty block.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "if nop nop end"_su8);

  // if, w/ else.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"else"_su8, I{At{"else"_su8, O::Else}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "if else end"_su8);

  // if, w/ else and non-empty blocks.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"else"_su8, I{At{"else"_su8, O::Else}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "if nop nop else nop nop end"_su8);

  // If w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"if $l"_su8,
                  I{At{"if"_su8, O::If},
                    At{"$l"_su8, BlockImmediate{At{"$l"_su8, "$l"_sv}, {}}}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "if $l nop end"_su8);

  // If w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          At{"if $l2"_su8,
             I{At{"if"_su8, O::If},
               At{"$l2"_su8, BlockImmediate{At{"$l2"_su8, "$l2"_sv}, {}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
      },
      "if $l2 nop end $l2"_su8);

  // If w/ label and matching else and end labels.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          At{"if $l3"_su8,
             I{At{"if"_su8, O::If},
               At{"$l3"_su8, BlockImmediate{At{"$l3"_su8, "$l3"_sv}, {}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"else"_su8, I{At{"else"_su8, O::Else}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
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
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"try"_su8, I{At{"try"_su8, O::Try}, BlockImmediate{}}},
               At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "try catch end"_su8);

  // try/catch and non-empty blocks.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"try"_su8, I{At{"try"_su8, O::Try}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "try nop nop catch nop nop end"_su8);

  // try w/ label.
  OKVector(ReadBlockInstruction_ForTesting,
           InstructionList{
               At{"try $l"_su8,
                  I{At{"try"_su8, O::Try},
                    At{"$l"_su8, BlockImmediate{At{"$l"_su8, "$l"_sv}, {}}}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "try $l nop catch nop end"_su8);

  // try w/ label and matching end label.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          At{"try $l2"_su8,
             I{At{"try"_su8, O::Try},
               At{"$l2"_su8, BlockImmediate{At{"$l2"_su8, "$l2"_sv}, {}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
      },
      "try $l2 nop catch nop end $l2"_su8);

  // try w/ label and matching catch and end labels.
  OKVector(
      ReadBlockInstruction_ForTesting,
      InstructionList{
          At{"try $l3"_su8,
             I{At{"try"_su8, O::Try},
               At{"$l3"_su8, BlockImmediate{At{"$l3"_su8, "$l3"_sv}, {}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
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
               At{"let"_su8, I{At{"let"_su8, O::Let}, LetImmediate{}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "let end"_su8);

  // Let w/ multiple instructions.
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               At{"let"_su8, I{At{"let"_su8, O::Let}, LetImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "let nop nop end"_su8);

  // Let w/ label.
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               At{"let $l"_su8,
                  I{At{"let"_su8, O::Let},
                    At{"$l"_su8,
                       LetImmediate{BlockImmediate{At{"$l"_su8, "$l"_sv}, {}},
                                    {}}}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "let $l nop end"_su8);

  // Let w/ label and matching end label.
  OKVector(ReadLetInstruction_ForTesting,
           InstructionList{
               At{"let $l2"_su8,
                  I{At{"let"_su8, O::Let},
                    At{"$l2"_su8,
                       LetImmediate{BlockImmediate{At{"$l2"_su8, "$l2"_sv}, {}},
                                    {}}}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"end"_su8, I{At{"end"_su8, O::End}}},
           },
           "let $l2 nop end $l2"_su8);

  // Let w/ locals
  OKVector(
      ReadLetInstruction_ForTesting,
      InstructionList{
          At{"let (local i32)"_su8,
             I{At{"let"_su8, O::Let},
               At{"(local i32)"_su8,
                  LetImmediate{
                      BlockImmediate{},
                      {At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}}}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
      },
      "let (local i32) nop end"_su8);

  // Let w/ params, results, locals
  OKVector(
      ReadLetInstruction_ForTesting,
      InstructionList{
          At{"let (param f32) (result f64) (local i32)"_su8,
             I{At{"let"_su8, O::Let},
               At{"(param f32) (result f64) (local i32)"_su8,
                  LetImmediate{
                      BlockImmediate{
                          nullopt,
                          FunctionTypeUse{
                              nullopt,
                              At{"(param f32) (result f64)"_su8,
                                 FunctionType{{At{"f32"_su8, VT_F32}},
                                              {At{"f64"_su8, VT_F64}}}}}},
                      {At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}}}}}}},
          At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
          At{"end"_su8, I{At{"end"_su8, O::End}}},
      },
      "let (param f32) (result f64) (local i32) nop end"_su8);
}

TEST_F(TextReadTest, Label_ReuseNames) {
  OK(ReadInstructionList_ForTesting,
     InstructionList{
         At{"block $l"_su8,
            I{At{"block"_su8, O::Block},
              At{"$l"_su8,
                 BlockImmediate{At{"$l"_su8, BindVar{"$l"_sv}}, {}}}}},
         At{"end"_su8, I{At{"end"_su8, O::End}}},
         At{"block $l"_su8,
            I{At{"block"_su8, O::Block},
              At{"$l"_su8,
                 BlockImmediate{At{"$l"_su8, BindVar{"$l"_sv}}, {}}}}},
         At{"end"_su8, I{At{"end"_su8, O::End}}},
     },
     "block $l end block $l end"_su8);
}

TEST_F(TextReadTest, Label_DuplicateNames) {
  OK(ReadInstructionList_ForTesting,
     InstructionList{
         At{"block $b"_su8,
            I{At{"block"_su8, O::Block},
              At{"$b"_su8, BlockImmediate{At{"$b"_su8, "$b"_sv}, {}}}}},
         At{"block $b"_su8,
            I{At{"block"_su8, O::Block},
              At{"$b"_su8, BlockImmediate{At{"$b"_su8, "$b"_sv}, {}}}}},
         At{"end"_su8, I{At{"end"_su8, O::End}}},
         At{"end"_su8, I{At{"end"_su8, O::End}}},
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
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
           },
           "(nop)"_su8);

  // BrTable immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          At{"br_table 0 0 0"_su8,
             I{At{"br_table"_su8, O::BrTable},
               At{"0 0 0"_su8, BrTableImmediate{{At{"0"_su8, Var{Index{0}}},
                                                 At{"0"_su8, Var{Index{0}}}},
                                                At{"0"_su8, Var{Index{0}}}}}}},
      },
      "(br_table 0 0 0)"_su8);

  // CallIndirect immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"call_indirect (type 0)"_su8,
                  I{At{"call_indirect"_su8, O::CallIndirect},
                    At{"(type 0)"_su8,
                       CallIndirectImmediate{
                           nullopt,
                           FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}},
                                           {}}}}}},
           },
           "(call_indirect (type 0))"_su8);

  // f32 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"f32.const 1.0"_su8, I{At{"f32.const"_su8, O::F32Const},
                                         At{"1.0"_su8, f32{1.0f}}}},
           },
           "(f32.const 1.0)"_su8);

  // f64 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"f64.const 2.0"_su8,
                  I{At{"f64.const"_su8, O::F64Const}, At{"2.0"_su8, f64{2.0}}}},
           },
           "(f64.const 2.0)"_su8);

  // i32 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"i32.const 3"_su8,
                  I{At{"i32.const"_su8, O::I32Const}, At{"3"_su8, s32{3}}}},
           },
           "(i32.const 3)"_su8);

  // i64 immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"i64.const 4"_su8,
                  I{At{"i64.const"_su8, O::I64Const}, At{"4"_su8, s64{4}}}},
           },
           "(i64.const 4)"_su8);

  // MemArg immediate
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"i32.load align=1"_su8,
                  I{At{"i32.load"_su8, O::I32Load},
                    At{"align=1"_su8,
                       MemArgImmediate{At{"align=1"_su8, u32{1}}, nullopt}}}},
           },
           "(i32.load align=1)"_su8);

  // Var immediate.
  OKVector(
      ReadExpression_ForTesting,
      InstructionList{
          At{"br 0"_su8, I{At{"br"_su8, O::Br}, At{"0"_su8, Var{Index{0}}}}},
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
          At{"br_on_exn 0 0"_su8,
             I{At{"br_on_exn"_su8, O::BrOnExn},
               At{"0 0"_su8, BrOnExnImmediate{At{"0"_su8, Var{Index{0}}},
                                              At{"0"_su8, Var{Index{0}}}}}}},
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
               At{"v128.const i32x4 0 0 0 0"_su8,
                  I{At{"v128.const"_su8, O::V128Const},
                    At{"0 0 0 0"_su8, v128{u32x4{0, 0, 0, 0}}}}},
           },
           "(v128.const i32x4 0 0 0 0)"_su8);

  // FeaturesSimd lane immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"f32x4.replace_lane 3"_su8,
                  I{At{"f32x4.replace_lane"_su8, O::F32X4ReplaceLane},
                    At{"3"_su8, SimdLaneImmediate{3}}}},
           },
           "(f32x4.replace_lane 3)"_su8);
}

TEST_F(TextReadTest, Expression_Plain_bulk_memory) {
  Fail(ReadExpression_ForTesting, {{1, "table.init instruction not allowed"}},
       "(table.init 0)"_su8);

  context.features.enable_bulk_memory();

  // Init immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"table.init 0"_su8,
                  I{At{"table.init"_su8, O::TableInit},
                    At{"0"_su8,
                       InitImmediate{At{"0"_su8, Var{Index{0}}}, nullopt}}}},
           },
           "(table.init 0)"_su8);

  // Copy immediate.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"table.copy"_su8,
                  I{At{"table.copy"_su8, O::TableCopy}, CopyImmediate{}}},
           },
           "(table.copy)"_su8);
}

TEST_F(TextReadTest, Expression_PlainFolded) {
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"i32.const 0"_su8,
                  I{At{"i32.const"_su8, O::I32Const}, At{"0"_su8, s32{0}}}},
               At{"i32.add"_su8, I{At{"i32.add"_su8, O::I32Add}}},
           },
           "(i32.add (i32.const 0))"_su8);

  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"i32.const 0"_su8,
                  I{At{"i32.const"_su8, O::I32Const}, At{"0"_su8, s32{0}}}},
               At{"i32.const 1"_su8,
                  I{At{"i32.const"_su8, O::I32Const}, At{"1"_su8, s32{1}}}},
               At{"i32.add"_su8, I{At{"i32.add"_su8, O::I32Add}}},
           },
           "(i32.add (i32.const 0) (i32.const 1))"_su8);
}

TEST_F(TextReadTest, Expression_Block) {
  // Block.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"block"_su8, I{At{"block"_su8, O::Block}, BlockImmediate{}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(block)"_su8);

  // Loop.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"loop"_su8, I{At{"loop"_su8, O::Loop}, BlockImmediate{}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(loop)"_su8);
}

TEST_F(TextReadTest, Expression_If) {
  // If then.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(if (then))"_su8);

  // If then w/ nops.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(if (then (nop)))"_su8);

  // If condition then.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(if (nop) (then (nop)))"_su8);

  // If then else.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"else"_su8, I{At{"else"_su8, O::Else}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(if (then (nop)) (else (nop)))"_su8);

  // If condition then else.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"if"_su8, I{At{"if"_su8, O::If}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"else"_su8, I{At{"else"_su8, O::Else}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
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
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"try"_su8, I{At{"try"_su8, O::Try}, BlockImmediate{}}},
               At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(try (catch))"_su8);

  // Try catch w/ nops.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"try"_su8, I{At{"try"_su8, O::Try}, BlockImmediate{}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"catch"_su8, I{At{"catch"_su8, O::Catch}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
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
               At{"let"_su8, I{At{"let"_su8, O::Let}, LetImmediate{}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(let)"_su8);

  // Let with locals and nops.
  OKVector(ReadExpression_ForTesting,
           InstructionList{
               At{"let (local i32 i64)"_su8,
                  I{At{"let"_su8, O::Let},
                    At{"(local i32 i64)"_su8,
                       LetImmediate{
                           BlockImmediate{},
                           {At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}},
                            At{"i64"_su8, BVT{nullopt, At{"i64"_su8, VT_I64}}}},
                       }}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{")"_su8, I{At{")"_su8, O::End}}},
           },
           "(let (local i32 i64) nop nop)"_su8);
}

TEST_F(TextReadTest, ExpressionList) {
  OKVector(ReadExpressionList_ForTesting,
           InstructionList{
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
               At{"drop"_su8, I{At{"drop"_su8, O::Drop}}},
           },
           "(nop) (drop (nop))"_su8);
}

TEST_F(TextReadTest, TableType) {
  OK(ReadTableType,
     TableType{At{"1 2"_su8, Limits{At{"1"_su8, u32{1}}, At{"2"_su8, u32{2}}}},
               At{"funcref"_su8, RT_Funcref}},
     "1 2 funcref"_su8);
}

TEST_F(TextReadTest, TableType_memory64) {
  context.features.enable_memory64();

  Fail(ReadTableType, {{0, "Expected a natural number, got NumericType"}},
       "i64 1 2 funcref"_su8);
}

TEST_F(TextReadTest, MemoryType) {
  OK(ReadMemoryType,
     MemoryType{
         At{"1 2"_su8, Limits{At{"1"_su8, u32{1}}, At{"2"_su8, u32{2}}}}},
     "1 2"_su8);
}

TEST_F(TextReadTest, MemoryType_memory64) {
  context.features.enable_memory64();

  OK(ReadMemoryType,
     MemoryType{
         At{"i64 1 2"_su8, Limits{At{"1"_su8, u32{1}}, At{"2"_su8, u32{2}},
                                  Shared::No, At{"i64"_su8, IndexType::I64}}}},
     "i64 1 2"_su8);
}

TEST_F(TextReadTest, GlobalType) {
  OK(ReadGlobalType,
     GlobalType{At{"i32"_su8, At{"i32"_su8, VT_I32}}, Mutability::Const},
     "i32"_su8);

  OK(ReadGlobalType,
     GlobalType{At{"(mut i32)"_su8, At{"i32"_su8, VT_I32}},
                At{"mut"_su8, Mutability::Var}},
     "(mut i32)"_su8);
}

TEST_F(TextReadTest, EventType) {
  // Empty event type.
  OK(ReadEventType, EventType{EventAttribute::Exception, {}}, ""_su8);

  // Function type use.
  OK(ReadEventType,
     EventType{EventAttribute::Exception,
               FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
     "(type 0)"_su8);
}

TEST_F(TextReadTest, Function) {
  // Empty func.
  OK(ReadFunction, Function{{}, {}, {At{")"_su8, I{At{")"_su8, O::End}}}}, {}},
     "(func)"_su8);

  // Name.
  OK(ReadFunction,
     Function{FunctionDesc{At{"$f"_su8, "$f"_sv}, nullopt, {}},
              {},
              {At{")"_su8, I{At{")"_su8, O::End}}}},
              {}},
     "(func $f)"_su8);

  // Inline export.
  OK(ReadFunction,
     Function{{},
              {},
              {At{")"_su8, I{At{")"_su8, O::End}}}},
              InlineExportList{
                  At{"(export \"e\")"_su8,
                     InlineExport{At{"\"e\""_su8, Text{"\"e\""_sv, 1}}}}}},
     "(func (export \"e\"))"_su8);

  // Locals.
  OK(ReadFunction,
     Function{{},
              BoundValueTypeList{
                  At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}},
                  At{"i64"_su8, BVT{nullopt, At{"i64"_su8, VT_I64}}},
              },
              {At{")"_su8, I{At{")"_su8, O::End}}}},
              {}},
     "(func (local i32 i64))"_su8);

  // Instructions.
  OK(ReadFunction,
     Function{{},
              {},
              InstructionList{
                  At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
                  At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
                  At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
                  At{")"_su8, I{At{")"_su8, O::End}}},
              },
              {}},
     "(func nop nop nop)"_su8);

  // Everything for defined Function.
  OK(ReadFunction,
     Function{
         FunctionDesc{At{"$f2"_su8, "$f2"_sv}, nullopt, {}},
         BoundValueTypeList{At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}}},
         InstructionList{
             At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
             At{")"_su8, I{At{")"_su8, O::End}}},
         },
         InlineExportList{
             At{"(export \"m\")"_su8,
                InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(func $f2 (export \"m\") (local i32) nop)"_su8);
}

TEST_F(TextReadTest, FunctionInlineImport) {
  // Import.
  OK(ReadFunction,
     Function{{},
              At{"(import \"m\" \"n\")"_su8,
                 InlineImport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                              At{"\"n\""_su8, Text{"\"n\""_sv, 1}}}},
              {}},
     "(func (import \"m\" \"n\"))"_su8);

  // Everything for imported Function.
  OK(ReadFunction,
     Function{
         FunctionDesc{
             At{"$f"_su8, "$f"_sv}, nullopt,
             At{"(param i32)"_su8,
                BoundFunctionType{
                    {At{"i32"_su8, BVT{nullopt, At{"i32"_su8, VT_I32}}}}, {}}}},
         At{"(import \"a\" \"b\")"_su8,
            InlineImport{At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                         At{"\"b\""_su8, Text{"\"b\""_sv, 1}}}},
         InlineExportList{
             At{"(export \"m\")"_su8,
                InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(func $f (export \"m\") (import \"a\" \"b\") (param i32))"_su8);
}

TEST_F(TextReadTest, Table) {
  // Simplest table.
  OK(ReadTable,
     Table{TableDesc{{},
                     At{"0 funcref"_su8,
                        TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                  At{"funcref"_su8, RT_Funcref}}}},
           {}},
     "(table 0 funcref)"_su8);

  // Name.
  OK(ReadTable,
     Table{TableDesc{At{"$t"_su8, "$t"_sv},
                     At{"0 funcref"_su8,
                        TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                  At{"funcref"_su8, RT_Funcref}}}},
           {}},
     "(table $t 0 funcref)"_su8);

  // Inline export.
  OK(ReadTable,
     Table{TableDesc{{},
                     At{"0 funcref"_su8,
                        TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                  At{"funcref"_su8, RT_Funcref}}}},
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(table (export \"m\") 0 funcref)"_su8);

  // Name and inline export.
  OK(ReadTable,
     Table{TableDesc{At{"$t2"_su8, "$t2"_sv},
                     At{"0 funcref"_su8,
                        TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                  At{"funcref"_su8, RT_Funcref}}}},
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(table $t2 (export \"m\") 0 funcref)"_su8);

  // Inline element var list.
  OK(ReadTable,
     Table{TableDesc{{},
                     TableType{Limits{u32{3}, u32{3}},
                               At{"funcref"_su8, RT_Funcref}}},
           {},
           ElementListWithVars{ExternalKind::Function,
                               VarList{
                                   At{"0"_su8, Var{Index{0}}},
                                   At{"1"_su8, Var{Index{1}}},
                                   At{"2"_su8, Var{Index{2}}},
                               }}},
     "(table funcref (elem 0 1 2))"_su8);
}

TEST_F(TextReadTest, TableInlineImport) {
  // Inline import.
  OK(ReadTable,
     Table{TableDesc{{},
                     At{"0 funcref"_su8,
                        TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                  At{"funcref"_su8, RT_Funcref}}}},
           At{"(import \"m\" \"n\")"_su8,
              InlineImport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                           At{"\"n\""_su8, Text{"\"n\""_sv, 1}}}},
           {}},
     "(table (import \"m\" \"n\") 0 funcref)"_su8);

  // Everything for Table import.
  OK(ReadTable,
     Table{TableDesc{At{"$t"_su8, "$t"_sv},
                     At{"0 funcref"_su8,
                        TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                  At{"funcref"_su8, RT_Funcref}}}},
           At{"(import \"a\" \"b\")"_su8,
              InlineImport{At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                           At{"\"b\""_su8, Text{"\"b\""_sv, 1}}}},
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(table $t (export \"m\") (import \"a\" \"b\") 0 funcref)"_su8);
}

TEST_F(TextReadTest, Table_bulk_memory) {
  Fail(ReadTable, {{21, "Expected Rpar, got Lpar"}},
       "(table funcref (elem (nop)))"_su8);

  context.features.enable_bulk_memory();

  // Inline element var list.
  OK(ReadTable,
     Table{
         TableDesc{
             {},
             TableType{Limits{u32{2}, u32{2}}, At{"funcref"_su8, RT_Funcref}}},
         {},
         ElementListWithExpressions{
             At{"funcref"_su8, RT_Funcref},
             ElementExpressionList{
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
             }}},
     "(table funcref (elem (nop) (nop)))"_su8);
}

TEST_F(TextReadTest, NumericData) {
  struct {
    NumericDataType type;
    SpanU8 output;
    SpanU8 input;
  } tests[] = {
      {NumericDataType::I8, "\x80\xff\x00\xff"_su8, "(i8 -128 -1 0 255)"_su8},
      {NumericDataType::I16, "\x00\x80\xff\xff\x00\x00\xff\xff"_su8,
       "(i16 -32768 -1 0 65535)"_su8},
      {NumericDataType::I32,
       "\x00\x00\x00\x80"
       "\xff\xff\xff\xff"
       "\x00\x00\x00\x00"
       "\xff\xff\xff\xff"_su8,
       "(i32 -2147483648 -1 0 4294967295)"_su8},
      {NumericDataType::I64,
       "\x00\x00\x00\x00\x00\x00\x00\x80"
       "\xff\xff\xff\xff\xff\xff\xff\xff"
       "\x00\x00\x00\x00\x00\x00\x00\x00"
       "\xff\xff\xff\xff\xff\xff\xff\xff"_su8,
       "(i64 -9223372036854775808 -1 0 18446744073709551615)"_su8},
      {NumericDataType::F32,
       "\x00\x00\x00\x00"
       "\x00\x00\x80\x3f"
       "\x00\x00\x80\x7f"
       "\x00\x00\xc0\x7f"_su8,
       "(f32 0 1.0 inf nan)"_su8},
      {NumericDataType::F64,
       "\x00\x00\x00\x00\x00\x00\x00\x00"
       "\x00\x00\x00\x00\x00\x00\xf0\x3f"
       "\x00\x00\x00\x00\x00\x00\xf0\x7f"
       "\x00\x00\x00\x00\x00\x00\xf8\x7f"_su8,
       "(f64 0 1.0 inf nan)"_su8},
      {NumericDataType::V128,
       "\x01\x00\x00\x00\x00\x00\x00\x00"
       "\xff\xff\xff\xff\xff\xff\xff\xff"
       "\x00\x00\x80\x3f"
       "\x00\x00\x80\x3f"
       "\x00\x00\x80\x3f"
       "\x00\x00\x80\x3f"_su8,
       "(v128 i64x2 1 -1 f32x4 1 1 1 1)"_su8},
  };

  for (auto&& test : tests) {
    OK(ReadNumericData, NumericData{test.type, ToBuffer(test.output)},
       test.input);
  }
}

TEST_F(TextReadTest, DataItem) {
  context.features.enable_numeric_values();

  OK(ReadDataItem,
     DataItem{
         NumericData{NumericDataType::I32, ToBuffer("\x05\x00\x00\x00"_su8)}},
     "(i32 5)"_su8);

  OK(ReadDataItem, DataItem{Text{"\"text\""_sv, 4}}, "\"text\""_su8);
}

TEST_F(TextReadTest, Memory) {
  // Simplest memory.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       At{"0"_su8, MemoryType{At{
                                       "0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
            {}},
     "(memory 0)"_su8);

  // Name.
  OK(ReadMemory,
     Memory{MemoryDesc{At{"$m"_su8, "$m"_sv},
                       At{"0"_su8, MemoryType{At{
                                       "0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
            {}},
     "(memory $m 0)"_su8);

  // Inline export.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       At{"0"_su8, MemoryType{At{
                                       "0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(memory (export \"m\") 0)"_su8);

  // Name and inline export.
  OK(ReadMemory,
     Memory{MemoryDesc{At{"$t"_su8, "$t"_sv},
                       At{"0"_su8, MemoryType{At{
                                       "0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(memory $t (export \"m\") 0)"_su8);

  // Inline data segment.
  OK(ReadMemory,
     Memory{MemoryDesc{{}, MemoryType{Limits{u32{10}, u32{10}}}},
            {},
            DataItemList{
                At{"\"hello\""_su8, DataItem{Text{"\"hello\""_sv, 5}}},
                At{"\"world\""_su8, DataItem{Text{"\"world\""_sv, 5}}},
            }},
     "(memory (data \"hello\" \"world\"))"_su8);
}

TEST_F(TextReadTest, Memory_numeric_values) {
  Fail(ReadMemory, {{14, "Numeric values not allowed"}},
       "(memory (data (i32 1 2 3)))"_su8);

  context.features.enable_numeric_values();

  OK(ReadMemory,
     Memory{MemoryDesc{{}, MemoryType{Limits{u32{12}, u32{12}}}},
            {},
            DataItemList{
                At{"(i32 1 2 3)"_su8,
                   DataItem{NumericData{NumericDataType::I32,
                                        ToBuffer("\x01\x00\x00\x00"
                                                 "\x02\x00\x00\x00"
                                                 "\x03\x00\x00\x00"_su8)}}}}},
     "(memory (data (i32 1 2 3)))"_su8);
}

TEST_F(TextReadTest, MemoryInlineImport) {
  // Inline import.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       At{"0"_su8, MemoryType{At{
                                       "0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
            At{"(import \"m\" \"n\")"_su8,
               InlineImport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                            At{"\"n\""_su8, Text{"\"n\""_sv, 1}}}},
            {}},
     "(memory (import \"m\" \"n\") 0)"_su8);

  // Everything for Memory import.
  OK(ReadMemory,
     Memory{MemoryDesc{At{"$t"_su8, "$t"_sv},
                       At{"0"_su8, MemoryType{At{
                                       "0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
            At{"(import \"a\" \"b\")"_su8,
               InlineImport{At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                            At{"\"b\""_su8, Text{"\"b\""_sv, 1}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(memory $t (export \"m\") (import \"a\" \"b\") 0)"_su8);
}

TEST_F(TextReadTest, Memory_memory64) {
  context.features.enable_memory64();

  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       At{"i64 0"_su8,
                          MemoryType{At{
                              "i64 0"_su8,
                              Limits{At{"0"_su8, u32{0}}, nullopt, Shared::No,
                                     At{"i64"_su8, IndexType::I64}}}}}},
            {}},
     "(memory i64 0)"_su8);

  // Name.
  OK(ReadMemory,
     Memory{MemoryDesc{At{"$m"_su8, "$m"_sv},
                       At{"i64 0"_su8,
                          MemoryType{At{
                              "i64 0"_su8,
                              Limits{At{"0"_su8, u32{0}}, nullopt, Shared::No,
                                     At{"i64"_su8, IndexType::I64}}}}}},
            {}},
     "(memory $m i64 0)"_su8);

  // Inline export.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       At{"i64 0"_su8,
                          MemoryType{At{
                              "i64 0"_su8,
                              Limits{At{"0"_su8, u32{0}}, nullopt, Shared::No,
                                     At{"i64"_su8, IndexType::I64}}}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(memory (export \"m\") i64 0)"_su8);

  // Name and inline export.
  OK(ReadMemory,
     Memory{MemoryDesc{At{"$t"_su8, "$t"_sv},
                       At{"i64 0"_su8,
                          MemoryType{At{
                              "i64 0"_su8,
                              Limits{At{"0"_su8, u32{0}}, nullopt, Shared::No,
                                     At{"i64"_su8, IndexType::I64}}}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(memory $t (export \"m\") i64 0)"_su8);

  // Inline data segment.
  OK(ReadMemory,
     Memory{MemoryDesc{{},
                       MemoryType{Limits{u32{10}, u32{10}, Shared::No,
                                         At{"i64"_su8, IndexType::I64}}}},
            {},
            DataItemList{At{"\"hello\""_su8, DataItem{Text{"\"hello\""_sv, 5}}},
                         At{
                             "\"world\""_su8,
                             DataItem{Text{"\"world\""_sv, 5}},
                         }}},
     "(memory i64 (data \"hello\" \"world\"))"_su8);
}

TEST_F(TextReadTest, Global) {
  // Simplest global.
  OK(ReadGlobal,
     Global{GlobalDesc{{},
                       At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                Mutability::Const}}},
            At{"nop"_su8,
               ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
            {}},
     "(global i32 nop)"_su8);

  // Name.
  OK(ReadGlobal,
     Global{GlobalDesc{At{"$g"_su8, "$g"_sv},
                       At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                Mutability::Const}}},
            At{"nop"_su8,
               ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
            {}},
     "(global $g i32 nop)"_su8);

  // Inline export.
  OK(ReadGlobal,
     Global{GlobalDesc{{},
                       At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                Mutability::Const}}},
            At{"nop"_su8,
               ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(global (export \"m\") i32 nop)"_su8);

  // Name and inline export.
  OK(ReadGlobal,
     Global{GlobalDesc{At{"$g2"_su8, "$g2"_sv},
                       At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                Mutability::Const}}},
            At{"nop"_su8,
               ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(global $g2 (export \"m\") i32 nop)"_su8);
}

TEST_F(TextReadTest, GlobalInlineImport) {
  // Inline import.
  OK(ReadGlobal,
     Global{GlobalDesc{{},
                       At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                Mutability::Const}}},
            At{"(import \"m\" \"n\")"_su8,
               InlineImport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                            At{"\"n\""_su8, Text{"\"n\""_sv, 1}}}},
            {}},
     "(global (import \"m\" \"n\") i32)"_su8);

  // Everything for Global import.
  OK(ReadGlobal,
     Global{GlobalDesc{At{"$g"_su8, "$g"_sv},
                       At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                Mutability::Const}}},
            At{"(import \"a\" \"b\")"_su8,
               InlineImport{At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                            At{"\"b\""_su8, Text{"\"b\""_sv, 1}}}},
            InlineExportList{
                At{"(export \"m\")"_su8,
                   InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(global $g (export \"m\") (import \"a\" \"b\") i32)"_su8);
}

TEST_F(TextReadTest, Event) {
  Fail(ReadEvent, {{0, "Events not allowed"}}, "(event)"_su8);

  context.features.enable_exceptions();

  // Simplest event.
  OK(ReadEvent, Event{}, "(event)"_su8);

  // Name.
  OK(ReadEvent, Event{EventDesc{At{"$e"_su8, "$e"_sv}, {}}, {}},
     "(event $e)"_su8);

  // Inline export.
  OK(ReadEvent,
     Event{EventDesc{},
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(event (export \"m\"))"_su8);

  // Name and inline export.
  OK(ReadEvent,
     Event{EventDesc{At{"$e2"_su8, "$e2"_sv}, {}},
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(event $e2 (export \"m\"))"_su8);
}

TEST_F(TextReadTest, EventInlineImport) {
  Fail(ReadEvent, {{0, "Events not allowed"}},
       "(event (import \"m\" \"n\"))"_su8);

  context.features.enable_exceptions();

  // Inline import.
  OK(ReadEvent,
     Event{EventDesc{},
           At{"(import \"m\" \"n\")"_su8,
              InlineImport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                           At{"\"n\""_su8, Text{"\"n\""_sv, 1}}}},
           {}},
     "(event (import \"m\" \"n\"))"_su8);

  // Everything for event import.
  OK(ReadEvent,
     Event{EventDesc{At{"$e"_su8, "$e"_sv}, {}},
           At{"(import \"a\" \"b\")"_su8,
              InlineImport{At{"\"a\""_su8, Text{"\"a\""_sv, 1}},
                           At{"\"b\""_su8, Text{"\"b\""_sv, 1}}}},
           InlineExportList{
               At{"(export \"m\")"_su8,
                  InlineExport{At{"\"m\""_su8, Text{"\"m\""_sv, 1}}}}}},
     "(event $e (export \"m\") (import \"a\" \"b\"))"_su8);
}

TEST_F(TextReadTest, Import) {
  // Function.
  OK(ReadImport,
     Import{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
            At{"\"n\""_su8, Text{"\"n\""_sv, 1}}, FunctionDesc{}},
     "(import \"m\" \"n\" (func))"_su8);

  // Table.
  OK(ReadImport,
     Import{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
            At{"\"n\""_su8, Text{"\"n\""_sv, 1}},
            TableDesc{nullopt,
                      At{"1 funcref"_su8,
                         TableType{At{"1"_su8, Limits{At{"1"_su8, u32{1}}}},
                                   At{"funcref"_su8, RT_Funcref}}}}},
     "(import \"m\" \"n\" (table 1 funcref))"_su8);

  // Memory.
  OK(ReadImport,
     Import{
         At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
         At{"\"n\""_su8, Text{"\"n\""_sv, 1}},
         MemoryDesc{nullopt,
                    At{"1"_su8,
                       MemoryType{At{"1"_su8, Limits{At{"1"_su8, u32{1}}}}}}}},
     "(import \"m\" \"n\" (memory 1))"_su8);

  // Global.
  OK(ReadImport,
     Import{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
            At{"\"n\""_su8, Text{"\"n\""_sv, 1}},
            GlobalDesc{nullopt, At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                         Mutability::Const}}}},
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
     Import{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
            At{"\"n\""_su8, Text{"\"n\""_sv, 1}}, EventDesc{}},
     "(import \"m\" \"n\" (event))"_su8);
}

TEST_F(TextReadTest, Export) {
  // Function.
  OK(ReadExport,
     Export{At{"func"_su8, ExternalKind::Function},
            At{"\"m\""_su8, Text{"\"m\""_sv, 1}}, At{"0"_su8, Var{Index{0}}}},
     "(export \"m\" (func 0))"_su8);

  // Table.
  OK(ReadExport,
     Export{At{"table"_su8, ExternalKind::Table},
            At{"\"m\""_su8, Text{"\"m\""_sv, 1}}, At{"0"_su8, Var{Index{0}}}},
     "(export \"m\" (table 0))"_su8);

  // Memory.
  OK(ReadExport,
     Export{At{"memory"_su8, ExternalKind::Memory},
            At{"\"m\""_su8, Text{"\"m\""_sv, 1}}, At{"0"_su8, Var{Index{0}}}},
     "(export \"m\" (memory 0))"_su8);

  // Global.
  OK(ReadExport,
     Export{At{"global"_su8, ExternalKind::Global},
            At{"\"m\""_su8, Text{"\"m\""_sv, 1}}, At{"0"_su8, Var{Index{0}}}},
     "(export \"m\" (global 0))"_su8);
}

TEST_F(TextReadTest, Export_exceptions) {
  Fail(ReadExport, {{13, "Events not allowed"}},
       "(export \"m\" (event 0))"_su8);

  context.features.enable_exceptions();

  // Event.
  OK(ReadExport,
     Export{At{"event"_su8, ExternalKind::Event},
            At{"\"m\""_su8, Text{"\"m\""_sv, 1}}, At{"0"_su8, Var{Index{0}}}},
     "(export \"m\" (event 0))"_su8);
}

TEST_F(TextReadTest, Start) {
  OK(ReadStart, Start{At{"0"_su8, Var{Index{0}}}}, "(start 0)"_su8);
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
         At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
     },
     "(item nop)"_su8);

  // Expression.
  OK(ReadElementExpression,
     ElementExpression{
         At{"nop"_su8, I{At{"nop"_su8, O::Nop}}},
     },
     "(nop)"_su8);
}

TEST_F(TextReadTest, OffsetExpression) {
  // Expression.
  OK(ReadOffsetExpression,
     ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}, "(nop)"_su8);

  // Offset keyword.
  OK(ReadOffsetExpression,
     ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}},
     "(offset nop)"_su8);
}

TEST_F(TextReadTest, ElementExpressionList) {
  context.features.enable_bulk_memory();

  // Item list.
  OKVector(ReadElementExpressionList,
           ElementExpressionList{
               At{"(item nop)"_su8,
                  ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
               At{"(item nop)"_su8,
                  ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
           },
           "(item nop) (item nop)"_su8);

  // Expression list.
  OKVector(ReadElementExpressionList,
           ElementExpressionList{
               At{"(nop)"_su8,
                  ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
               At{"(nop)"_su8,
                  ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}}},
           "(nop) (nop)"_su8);
}

TEST_F(TextReadTest, TableUseOpt) {
  OK(ReadTableUseOpt, Var{Index{0}}, "(table 0)"_su8);
  OK(ReadTableUseOpt, nullopt, ""_su8);
}

TEST_F(TextReadTest, ElementSegment_MVP) {
  // No table var, empty var list.
  OK(ReadElementSegment,
     ElementSegment{nullopt, nullopt,
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    ElementListWithVars{ExternalKind::Function, {}}},
     "(elem (nop))"_su8);

  // No table var, var list.
  OK(ReadElementSegment,
     ElementSegment{nullopt, nullopt,
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    ElementListWithVars{ExternalKind::Function,
                                        VarList{At{"0"_su8, Var{Index{0}}},
                                                At{"1"_su8, Var{Index{1}}},
                                                At{"2"_su8, Var{Index{2}}}}}},
     "(elem (nop) 0 1 2)"_su8);

  // Table var.
  OK(ReadElementSegment,
     ElementSegment{nullopt, At{"0"_su8, Var{Index{0}}},
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    ElementListWithVars{ExternalKind::Function, {}}},
     "(elem 0 (nop))"_su8);

  // Table var as Id.
  OK(ReadElementSegment,
     ElementSegment{nullopt, At{"$t"_su8, Var{"$t"_sv}},
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
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
             At{"funcref"_su8, RT_Funcref},
             ElementExpressionList{
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
             }}},
     "(elem funcref (nop) (nop))"_su8);

  // Passive, w/ var list.
  OK(ReadElementSegment,
     ElementSegment{nullopt, SegmentType::Passive,
                    ElementListWithVars{At{"func"_su8, ExternalKind::Function},
                                        VarList{
                                            At{"0"_su8, Var{Index{0}}},
                                            At{"$e"_su8, Var{"$e"_sv}},
                                        }}},
     "(elem func 0 $e)"_su8);

  // Passive w/ name.
  OK(ReadElementSegment,
     ElementSegment{
         At{"$e"_su8, "$e"_sv}, SegmentType::Passive,
         ElementListWithVars{At{"func"_su8, ExternalKind::Function}, {}}},
     "(elem $e func)"_su8);

  // Declared, w/ expression list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, SegmentType::Declared,
         ElementListWithExpressions{
             At{"funcref"_su8, RT_Funcref},
             ElementExpressionList{
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
             }}},
     "(elem declare funcref (nop) (nop))"_su8);

  // Declared, w/ var list.
  OK(ReadElementSegment,
     ElementSegment{nullopt, SegmentType::Declared,
                    ElementListWithVars{At{"func"_su8, ExternalKind::Function},
                                        VarList{
                                            At{"0"_su8, Var{Index{0}}},
                                            At{"$e"_su8, Var{"$e"_sv}},
                                        }}},
     "(elem declare func 0 $e)"_su8);

  // Declared w/ name.
  OK(ReadElementSegment,
     ElementSegment{
         At{"$e2"_su8, "$e2"_sv}, SegmentType::Declared,
         ElementListWithVars{At{"func"_su8, ExternalKind::Function}, {}}},
     "(elem $e2 declare func)"_su8);

  // Active legacy, empty
  OK(ReadElementSegment,
     ElementSegment{nullopt,
                    nullopt,
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    {}},
     "(elem (nop))"_su8);

  // Active legacy (i.e. no element type or external kind).
  OK(ReadElementSegment,
     ElementSegment{nullopt, nullopt,
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    ElementListWithVars{ExternalKind::Function,
                                        VarList{
                                            At{"0"_su8, Var{Index{0}}},
                                            At{"$e"_su8, Var{"$e"_sv}},
                                        }}},
     "(elem (nop) 0 $e)"_su8);

  // Active, w/ var list.
  OK(ReadElementSegment,
     ElementSegment{nullopt, nullopt,
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    ElementListWithVars{At{"func"_su8, ExternalKind::Function},
                                        VarList{
                                            At{"0"_su8, Var{Index{0}}},
                                            At{"$e"_su8, Var{"$e"_sv}},
                                        }}},
     "(elem (nop) func 0 $e)"_su8);

  // Active, w/ expression list.
  OK(ReadElementSegment,
     ElementSegment{
         nullopt, nullopt,
         At{"(nop)"_su8,
            ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
         ElementListWithExpressions{
             At{"funcref"_su8, RT_Funcref},
             ElementExpressionList{
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 At{"(nop)"_su8,
                    ElementExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
             }}},
     "(elem (nop) funcref (nop) (nop))"_su8);

  // Active w/ table use.
  OK(ReadElementSegment,
     ElementSegment{nullopt, At{"(table 0)"_su8, Var{Index{0}}},
                    At{"(nop)"_su8, ConstantExpression{At{
                                        "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                    ElementListWithVars{At{"func"_su8, ExternalKind::Function},
                                        VarList{
                                            At{"1"_su8, Var{Index{1}}},
                                        }}},
     "(elem (table 0) (nop) func 1)"_su8);

  // Active w/ name.
  OK(ReadElementSegment,
     ElementSegment{
         At{"$e3"_su8, "$e3"_sv}, nullopt,
         At{"(nop)"_su8,
            ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
         ElementListWithVars{At{"func"_su8, ExternalKind::Function}, {}}},
     "(elem $e3 (nop) func)"_su8);
}

TEST_F(TextReadTest, DataSegment_MVP) {
  // No memory var, empty text list.
  OK(ReadDataSegment,
     DataSegment{nullopt,
                 nullopt,
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 {}},
     "(data (nop))"_su8);

  // No memory var, text list.
  OK(ReadDataSegment,
     DataSegment{
         nullopt, nullopt,
         At{"(nop)"_su8,
            ConstantExpression{At{"nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
         DataItemList{At{"\"hi\""_su8, DataItem{Text{"\"hi\""_sv, 2}}}}},
     "(data (nop) \"hi\")"_su8);

  // Memory var.
  OK(ReadDataSegment,
     DataSegment{nullopt,
                 At{"0"_su8, Var{Index{0}}},
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 {}},
     "(data 0 (nop))"_su8);

  // Memory var as Id.
  OK(ReadDataSegment,
     DataSegment{nullopt,
                 At{"$m"_su8, Var{"$m"_sv}},
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
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
                 DataItemList{
                     At{"\"hi\""_su8, DataItem{Text{"\"hi\""_sv, 2}}},
                 }},
     "(data \"hi\")"_su8);

  // Passive w/ name.
  OK(ReadDataSegment, DataSegment{At{"$d"_su8, "$d"_sv}, {}}, "(data $d)"_su8);

  // Active, w/ text list.
  OK(ReadDataSegment,
     DataSegment{nullopt, nullopt,
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 DataItemList{
                     At{"\"hi\""_su8, DataItem{Text{"\"hi\""_sv, 2}}},
                 }},
     "(data (nop) \"hi\")"_su8);

  // Active w/ memory use.
  OK(ReadDataSegment,
     DataSegment{nullopt, At{"(memory 0)"_su8, Var{Index{0}}},
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 DataItemList{
                     At{"\"hi\""_su8, DataItem{Text{"\"hi\""_sv, 2}}},
                 }},
     "(data (memory 0) (nop) \"hi\")"_su8);

  // Active w/ name.
  OK(ReadDataSegment,
     DataSegment{At{"$d2"_su8, "$d2"_sv},
                 nullopt,
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 {}},
     "(data $d2 (nop))"_su8);
}

TEST_F(TextReadTest, DataSegment_numeric_values) {
  Fail(ReadDataSegment, {{12, "Numeric values not allowed"}},
       "(data (nop) (i8 1))"_su8);

  context.features.enable_numeric_values();

  // No memory var, text list.
  OK(ReadDataSegment,
     DataSegment{nullopt, nullopt,
                 At{"(nop)"_su8, ConstantExpression{At{
                                     "nop"_su8, I{At{"nop"_su8, O::Nop}}}}},
                 DataItemList{At{"(i8 1)"_su8,
                                 DataItem{NumericData{NumericDataType::I8,
                                                      ToBuffer("\x01"_su8)}}}}},
     "(data (nop) (i8 1))"_su8);
}

TEST_F(TextReadTest, ModuleItem) {
  // Type.
  OK(ReadModuleItem, ModuleItem{DefinedType{nullopt, BoundFunctionType{}}},
     "(type (func))"_su8);

  // Import.
  OK(ReadModuleItem,
     ModuleItem{Import{At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
                       At{"\"n\""_su8, Text{"\"n\""_sv, 1}}, FunctionDesc{}}},
     "(import \"m\" \"n\" (func))"_su8);

  // Func.
  OK(ReadModuleItem,
     ModuleItem{Function{{}, {}, {At{")"_su8, I{At{")"_su8, O::End}}}}, {}}},
     "(func)"_su8);

  // Table.
  OK(ReadModuleItem,
     ModuleItem{
         Table{TableDesc{nullopt,
                         At{"0 funcref"_su8,
                            TableType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}},
                                      At{"funcref"_su8, RT_Funcref}}}},
               {}}},
     "(table 0 funcref)"_su8);

  // Memory.
  OK(ReadModuleItem,
     ModuleItem{Memory{
         MemoryDesc{
             nullopt,
             At{"0"_su8, MemoryType{At{"0"_su8, Limits{At{"0"_su8, u32{0}}}}}}},
         {}}},
     "(memory 0)"_su8);

  // Global.
  OK(ReadModuleItem,
     ModuleItem{Global{
         GlobalDesc{nullopt, At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32},
                                                      Mutability::Const}}},
         At{"(nop)"_su8,
            ConstantExpression{
                At{"nop"_su8, Instruction{At{"nop"_su8, Opcode::Nop}}}}},
         {}}},
     "(global i32 (nop))"_su8);

  // Export.
  OK(ReadModuleItem,
     ModuleItem{Export{
         At{"func"_su8, ExternalKind::Function},
         At{"\"m\""_su8, Text{"\"m\""_sv, 1}},
         At{"0"_su8, Var{Index{0}}},
     }},
     "(export \"m\" (func 0))"_su8);

  // Start.
  OK(ReadModuleItem, ModuleItem{Start{At{"0"_su8, Var{Index{0}}}}},
     "(start 0)"_su8);

  // Elem.
  OK(ReadModuleItem,
     ModuleItem{ElementSegment{
         nullopt,
         nullopt,
         At{"(nop)"_su8,
            ConstantExpression{
                At{"nop"_su8, Instruction{At{"nop"_su8, Opcode::Nop}}}}},
         {}}},
     "(elem (nop))"_su8);

  // Data.
  OK(ReadModuleItem,
     ModuleItem{DataSegment{
         nullopt,
         nullopt,
         At{"(nop)"_su8,
            ConstantExpression{
                At{"nop"_su8, Instruction{At{"nop"_su8, Opcode::Nop}}}}},
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
     Module{At{"(type (func))"_su8,
               ModuleItem{DefinedType{nullopt, BoundFunctionType{}}}},
            At{"(func nop)"_su8,
               ModuleItem{Function{
                   FunctionDesc{},
                   {},
                   InstructionList{
                       At{"nop"_su8, Instruction{At{"nop"_su8, Opcode::Nop}}},
                       At{")"_su8, I{At{")"_su8, O::End}}},
                   },
                   {}

               }}},
            At{"(start 0)"_su8, ModuleItem{Start{At{"0"_su8, Var{Index{0}}}}}}},
     "(type (func)) (func nop) (start 0)"_su8);
}

TEST_F(TextReadTest, Module_MultipleStart) {
  Fail(ReadModule, {{11, "Multiple start functions"}},
       "(start 0) (start 0)"_su8);
}

TEST_F(TextReadTest, SingleModule) {
  // Can be optionally wrapped in (module).
  OK(ReadSingleModule, Module{}, "(module)"_su8);

  // Can also have optional module name.
  OK(ReadSingleModule, Module{}, "(module $mod)"_su8);

  // module keyword can be omitted.
  OK(ReadSingleModule,
     Module{
         At{"(start 0)"_su8, ModuleItem{Start{At{"0"_su8, Var{Index{0}}}}}},
     },
     "(start 0)"_su8);
}
