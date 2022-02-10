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

#include "gtest/gtest.h"

#include "test/binary/constants.h"
#include "test/valid/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/valid_ctx.h"
#include "wasp/valid/validate.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;

class ValidateTest : public ::testing::Test {
 public:
  void SetUp() { ctx.features.DisableAll(); }

 protected:
  TestErrors errors;
  ValidCtx ctx{errors};
};

TEST_F(ValidateTest, UnpackedCode) {
  UnpackedCode code{
      LocalsList{Locals{2, VT_I32}},
      UnpackedExpression{InstructionList{
          Instruction{Opcode::LocalGet, Index{0}},
          Instruction{Opcode::LocalGet, Index{1}}, Instruction{Opcode::I32Add},
          Instruction{Opcode::End}}}};
  ctx.types.push_back(DefinedType{FunctionType{{}, {VT_I32}}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(ctx, code));
}

TEST_F(ValidateTest, UnpackedCode_DefaultableLocals) {
  UnpackedCode code{
      LocalsList{Locals{1, VT_Ref0}},
      UnpackedExpression{InstructionList{Instruction{Opcode::End}}}};
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(ctx, code));
}

TEST_F(ValidateTest, ArrayType) {
  EXPECT_TRUE(Validate(
      ctx, ArrayType{FieldType{StorageType{VT_I32}, Mutability::Const}}));
}

TEST_F(ValidateTest, ArrayType_IndexOOB) {
  EXPECT_FALSE(Validate(
      ctx, ArrayType{FieldType{StorageType{VT_Ref1}, Mutability::Const}}));
}

TEST_F(ValidateTest, ConstantExpression_Const) {
  const struct {
    Instruction instr;
    ValueType valtype;
  } tests[] = {
      {Instruction{Opcode::I32Const, s32{0}}, VT_I32},
      {Instruction{Opcode::I64Const, s64{0}}, VT_I64},
      {Instruction{Opcode::F32Const, f32{0}}, VT_F32},
      {Instruction{Opcode::F64Const, f64{0}}, VT_F64},
      {Instruction{Opcode::V128Const, v128{}}, VT_V128},
  };

  for (const auto& test : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, ConstantExpression{test.instr}, test.valtype, 0));
  }
}

TEST_F(ValidateTest, ConstantExpression_Global) {
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_I64, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_F32, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_F64, Mutability::Const});
  auto max = ctx.globals.size();

  EXPECT_TRUE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, VT_I32,
      max));
  EXPECT_TRUE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}}, VT_I64,
      max));
  EXPECT_TRUE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}}, VT_F32,
      max));
  EXPECT_TRUE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}}, VT_F64,
      max));
}

TEST_F(ValidateTest, ConstantExpression_InvalidOpcode) {
  const Instruction tests[] = {
      Instruction{Opcode::Unreachable},
      Instruction{Opcode::I32Add},
      Instruction{Opcode::Br, Index{0}},
      Instruction{Opcode::LocalGet, Index{0}},
      Instruction{Opcode::V128Const, v128{}},
      Instruction{Opcode::RefNull, HT_Func},
  };

  for (const auto& instr : tests) {
    ctx.Reset();
    EXPECT_FALSE(Validate(ctx, ConstantExpression{instr}, VT_I32, 0));
  }
}

TEST_F(ValidateTest, ConstantExpression_ConstMismatch) {
  const struct {
    Instruction instr;
    ValueType valtype;
  } tests[] = {
      {Instruction{Opcode::I32Const, s32{0}}, VT_I64},
      {Instruction{Opcode::I64Const, s64{0}}, VT_F32},
      {Instruction{Opcode::F32Const, f32{0}}, VT_F64},
      {Instruction{Opcode::F64Const, f64{0}}, VT_I32},
  };

  for (const auto& test : tests) {
    ctx.Reset();
    EXPECT_FALSE(
        Validate(ctx, ConstantExpression{test.instr}, test.valtype, 0));
  }
}

TEST_F(ValidateTest, ConstantExpression_GlobalIndexOOB) {
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});
  auto max = ctx.globals.size();

  EXPECT_FALSE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}}, VT_I32,
      max));
}

TEST_F(ValidateTest, ConstantExpression_GlobalTypeMismatch) {
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_I64, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_F32, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_F64, Mutability::Const});
  auto max = ctx.globals.size();

  EXPECT_FALSE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, VT_I64,
      max));
  EXPECT_FALSE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}}, VT_F32,
      max));
  EXPECT_FALSE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}}, VT_F64,
      max));
  EXPECT_FALSE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}}, VT_I32,
      max));
}

TEST_F(ValidateTest, ConstantExpression_GlobalMutVar) {
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Var});
  auto max = ctx.globals.size();

  EXPECT_FALSE(Validate(
      ctx, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, VT_I32,
      max));
}

TEST_F(ValidateTest, ConstantExpression_WrongInstructionCount) {
  // Too few instructions.
  EXPECT_FALSE(Validate(ctx, ConstantExpression{}, VT_I32, 0));
  // Too many instructions.
  EXPECT_FALSE(Validate(ctx,
                        ConstantExpression{InstructionList{
                            Instruction{Opcode::GlobalGet, Index{0}},
                            Instruction{Opcode::I32Const, s32{0}}}},
                        VT_I32, 0));
}

TEST_F(ValidateTest, ConstantExpression_GC) {
  ctx.features.enable_gc();

  // rtt.canon is allowed.
  EXPECT_TRUE(
      Validate(ctx, ConstantExpression{Instruction{Opcode::RttCanon, HT_Any}},
               VT_RTT_0_Any, 0));

  // Multiple instructions are allowed, and rtt.sub is too.
  EXPECT_TRUE(Validate(
      ctx,
      ConstantExpression{InstructionList{Instruction{Opcode::RttCanon, HT_Any},
                                         Instruction{Opcode::RttSub, HT_Eq}}},
      VT_RTT_1_Eq, 0));
}

TEST_F(ValidateTest, ConstantExpression_Funcref) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.functions.push_back(Function{0});

  // Using ref.func in the global section implicitly declares that function.
  EXPECT_TRUE(
      Validate(ctx, ConstantExpression{Instruction{Opcode::RefFunc, Index{0}}},
               VT_Funcref, 0));

  EXPECT_EQ(1u, ctx.declared_functions.size());
}

TEST_F(ValidateTest, DataCount) {
  EXPECT_TRUE(Validate(ctx, DataCount{1}));
  ASSERT_TRUE(ctx.declared_data_count);
  EXPECT_EQ(1u, ctx.declared_data_count);
}

TEST_F(ValidateTest, DataSegment_Active) {
  ctx.memories.push_back(MemoryType{Limits{0}});
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});

  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment tests[] = {
      DataSegment{0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
                  span},
      DataSegment{0,
                  ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
                  span},
  };

  for (const auto& data_segment : tests) {
    EXPECT_TRUE(Validate(ctx, data_segment));
  }
}

TEST_F(ValidateTest, DataSegment_Active_MemoryIndexOOB) {
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, span};
  EXPECT_FALSE(Validate(ctx, data_segment));
}

TEST_F(ValidateTest, DataSegment_Active_GlobalIndexOOB) {
  ctx.memories.push_back(MemoryType{Limits{0}});
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, span};
  EXPECT_FALSE(Validate(ctx, data_segment));
}

TEST_F(ValidateTest, ElementExpression) {
  ctx.functions.push_back(Function{0});

  const Instruction tests[] = {
      Instruction{Opcode::RefNull, HT_Func},
      Instruction{Opcode::RefFunc, Index{0}},
  };

  for (const auto& instr : tests) {
    EXPECT_TRUE(Validate(ctx, ElementExpression{instr}, RT_Funcref));
  }
}

TEST_F(ValidateTest, ElementExpression_InvalidOpcode) {
  const Instruction tests[] = {
      Instruction{Opcode::I32Const, s32{0}},
      Instruction{Opcode::I64Const, s64{0}},
      Instruction{Opcode::F32Const, f32{0}},
      Instruction{Opcode::F64Const, f64{0}},
      Instruction{Opcode::GlobalGet, Index{0}},
      Instruction{Opcode::I32Add},
      Instruction{Opcode::Br, Index{0}},
      Instruction{Opcode::LocalGet, Index{0}},
      Instruction{Opcode::V128Const, v128{}},
  };

  for (const auto& instr : tests) {
    ctx.Reset();
    EXPECT_FALSE(Validate(ctx, ElementExpression{instr}, RT_Funcref));
  }
}

TEST_F(ValidateTest, ElementExpression_FunctionIndexOOB) {
  ctx.functions.push_back(Function{0});
  EXPECT_FALSE(
      Validate(ctx, ElementExpression{Instruction{Opcode::RefFunc, Index{1}}},
               RT_Funcref));
}

TEST_F(ValidateTest, ElementSegment_Active) {
  ctx.functions.push_back(Function{0});
  ctx.functions.push_back(Function{0});
  ctx.tables.push_back(TableType{Limits{0}, RT_Funcref});
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});

  const ElementSegment tests[] = {
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
                     ElementListWithIndexes{ExternalKind::Function, {0, 1}}},
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
          ElementListWithIndexes{ExternalKind::Function, {}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(ctx, element_segment));
  }
}

TEST_F(ValidateTest, ElementSegment_Passive) {
  ctx.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{SegmentType::Passive,
                     ElementListWithExpressions{RT_Funcref, {}}},
      ElementSegment{
          SegmentType::Passive,
          ElementListWithExpressions{
              RT_Funcref,
              {ElementExpression{Instruction{Opcode::RefNull, HT_Func}},
               ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(ctx, element_segment));
  }
}

TEST_F(ValidateTest, ElementSegment_Declared) {
  ctx.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{SegmentType::Declared,
                     ElementListWithIndexes{ExternalKind::Function, {0}}},
      ElementSegment{
          SegmentType::Declared,
          ElementListWithExpressions{
              RT_Funcref,
              {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}}},
  };

  EXPECT_EQ(0u, ctx.declared_functions.count(0));
  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(ctx, element_segment));
  }
  EXPECT_EQ(1u, ctx.declared_functions.count(0));
}

TEST_F(ValidateTest, ElementSegment_RefType) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;

  ElementSegment element_segment{SegmentType::Passive,
                                 ElementListWithExpressions{RT_Ref0, {}}};
  EXPECT_TRUE(Validate(ctx, element_segment));
}

TEST_F(ValidateTest, ElementSegment_RefType_IndexOOB) {
  ElementSegment element_segment{SegmentType::Passive,
                                 ElementListWithExpressions{RT_Ref0, {}}};
  EXPECT_FALSE(Validate(ctx, element_segment));
}

TEST_F(ValidateTest, ElementSegment_Active_TypeMismatch) {
  ctx.functions.push_back(Function{0});
  ctx.tables.push_back(TableType{Limits{0}, RT_Funcref});
  ctx.globals.push_back(GlobalType{VT_F32, Mutability::Const});

  const ElementSegment tests[] = {
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::F32Const, f32{0}}},
                     ElementListWithIndexes{ExternalKind::Function, {}}},
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
          ElementListWithIndexes{ExternalKind::Function, {}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_FALSE(Validate(ctx, element_segment));
  }
}

TEST_F(ValidateTest, ElementSegment_Active_TableIndexOOB) {
  ctx.functions.push_back(Function{0});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ElementListWithIndexes{ExternalKind::Function, {}}};
  EXPECT_FALSE(Validate(ctx, element_segment));
}

TEST_F(ValidateTest, ElementSegment_Active_GlobalIndexOOB) {
  ctx.tables.push_back(TableType{Limits{0}, RT_Funcref});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      ElementListWithIndexes{ExternalKind::Function, {}}};
  EXPECT_FALSE(Validate(ctx, element_segment));
}

TEST_F(ValidateTest, ElementSegment_Active_FunctionIndexOOB) {
  ctx.tables.push_back(TableType{Limits{0}, RT_Funcref});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ElementListWithIndexes{ExternalKind::Function, {0}}};
  EXPECT_FALSE(Validate(ctx, element_segment));
}

TEST_F(ValidateTest, ElementSegment_Passive_FunctionIndexOOB) {
  const ElementSegment element_segment{
      SegmentType::Passive,
      ElementListWithExpressions{
          RT_Funcref,
          {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}}};
  EXPECT_FALSE(Validate(ctx, element_segment));
}

TEST_F(ValidateTest, ReferenceType) {
  EXPECT_TRUE(Validate(ctx, RT_Funcref, RT_Funcref));
}

TEST_F(ValidateTest, Export) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  ctx.tables.push_back(TableType{Limits{1}, RT_Funcref});
  ctx.memories.push_back(MemoryType{Limits{1}});
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});
  ctx.events.push_back(EventType{EventAttribute::Exception, Index{0}});

  const Export tests[] = {
      Export{ExternalKind::Function, "f", 0},
      Export{ExternalKind::Table, "t", 0},
      Export{ExternalKind::Memory, "m", 0},
      Export{ExternalKind::Global, "g", 0},
      Export{ExternalKind::Event, "e", 0},
  };

  for (const auto& export_ : tests) {
    EXPECT_TRUE(Validate(ctx, export_));
  }

  // Exporting a function marks it as declared.
  EXPECT_EQ(1u, ctx.declared_functions.size());
}

TEST_F(ValidateTest, Export_IndexOOB) {
  const Export tests[] = {
      Export{ExternalKind::Function, "", 0},
      Export{ExternalKind::Table, "", 0},
      Export{ExternalKind::Memory, "", 0},
      Export{ExternalKind::Global, "", 0},
      Export{ExternalKind::Event, "", 0},
  };

  for (const auto& export_ : tests) {
    ctx.Reset();
    EXPECT_FALSE(Validate(ctx, export_));
  }
}

TEST_F(ValidateTest, Export_GlobalMutVar_MVP) {
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Var});
  EXPECT_FALSE(Validate(ctx, Export{ExternalKind::Global, "", 0}));
}

TEST_F(ValidateTest, Export_GlobalMutVar_MutableGlobals) {
  ctx.features.enable_mutable_globals();
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Var});
  EXPECT_TRUE(Validate(ctx, Export{ExternalKind::Global, "", 0}));
}

TEST_F(ValidateTest, Export_Duplicate) {
  ctx.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(ctx, Export{ExternalKind::Function, "hi", 0}));
  EXPECT_FALSE(Validate(ctx, Export{ExternalKind::Function, "hi", 0}));
}

TEST_F(ValidateTest, Event) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  EXPECT_TRUE(
      Validate(ctx, Event{EventType{EventAttribute::Exception, Index{0}}}));
}

TEST_F(ValidateTest, Event_InvalidType) {
  ctx.types.push_back(DefinedType{StructType{}});
  ctx.defined_type_count = 1;
  EXPECT_FALSE(
      Validate(ctx, Event{EventType{EventAttribute::Exception, Index{0}}}));
}

TEST_F(ValidateTest, FieldType) {
  EXPECT_TRUE(Validate(ctx, FieldType{StorageType{VT_I32}, Mutability::Const}));
}

TEST_F(ValidateTest, FieldType_IndexOOB) {
  EXPECT_FALSE(
      Validate(ctx, FieldType{StorageType{VT_Ref1}, Mutability::Const}));
}

TEST_F(ValidateTest, FieldTypeList) {
  EXPECT_TRUE(Validate(
      ctx,
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const},
                    FieldType{StorageType{PackedType::I8}, Mutability::Var}}));
}

TEST_F(ValidateTest, EventType) {
  ctx.types.push_back(DefinedType{FunctionType{{VT_I32}, {}}});
  ctx.defined_type_count = 1;
  EXPECT_TRUE(Validate(ctx, EventType{EventAttribute::Exception, Index{0}}));
}

TEST_F(ValidateTest, EventType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, EventType{EventAttribute::Exception, Index{0}}));
}

TEST_F(ValidateTest, EventType_NonEmptyResult) {
  ctx.types.push_back(DefinedType{FunctionType{{}, {VT_I32}}});
  ctx.defined_type_count = 1;
  EXPECT_FALSE(Validate(ctx, EventType{EventAttribute::Exception, Index{0}}));
}

TEST_F(ValidateTest, Function) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  EXPECT_TRUE(Validate(ctx, Function{0}));
}

TEST_F(ValidateTest, Function_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, Function{0}));
}

TEST_F(ValidateTest, Function_InvalidType) {
  ctx.types.push_back(DefinedType{StructType{}});
  ctx.defined_type_count = 1;
  EXPECT_FALSE(Validate(ctx, Function{0}));
}

TEST_F(ValidateTest, FunctionType) {
  const FunctionType tests[] = {
      FunctionType{},
      FunctionType{{VT_I32}, {}},
      FunctionType{{VT_F32}, {}},
      FunctionType{{VT_F64}, {}},
      FunctionType{{VT_I64}, {VT_I32}},
      FunctionType{{VT_I64, VT_F32}, {VT_F32}},
      FunctionType{{}, {VT_F64}},
      FunctionType{{VT_I64, VT_I64, VT_I64}, {VT_I64}},
  };

  for (const auto& function_type : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, function_type));
  }
}

TEST_F(ValidateTest, FunctionType_MultiReturn_MVP) {
  const FunctionType tests[] = {
      FunctionType{{}, {VT_I32, VT_I32}},
      FunctionType{{}, {VT_I32, VT_I64, VT_F32}},
  };

  for (const auto& function_type : tests) {
    ctx.Reset();
    EXPECT_FALSE(Validate(ctx, function_type));
  }
}

TEST_F(ValidateTest, FunctionType_MultiReturn) {
  ctx.features.enable_multi_value();

  const FunctionType tests[] = {
      FunctionType{{}, {VT_I32, VT_I32}},
      FunctionType{{}, {VT_I32, VT_I64, VT_F32}},
  };

  for (const auto& function_type : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, function_type));
  }
}

TEST_F(ValidateTest, FunctionType_RefType) {
  FunctionType function_type{{VT_Ref0}, {VT_RefNull0}};

  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  EXPECT_TRUE(Validate(ctx, function_type));
}

TEST_F(ValidateTest, FunctionType_RefType_IndexOOB) {
  FunctionType function_type{{VT_Ref0}, {VT_RefNull0}};

  EXPECT_FALSE(Validate(ctx, function_type));
}

TEST_F(ValidateTest, Global) {
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});
  ctx.imported_global_count = 1;

  const Global tests[] = {
      Global{GlobalType{VT_I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{VT_I64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{VT_F32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{VT_F64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{VT_I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},

      Global{GlobalType{VT_I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{VT_I64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{VT_F32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{VT_F64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{VT_I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},
  };

  for (const auto& global : tests) {
    EXPECT_TRUE(Validate(ctx, global));
  }
}

TEST_F(ValidateTest, Global_TypeMismatch) {
  ctx.globals.push_back(GlobalType{VT_F32, Mutability::Const});
  ctx.imported_global_count = 1;

  const Global tests[] = {
      Global{GlobalType{VT_F32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{VT_F64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{VT_I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{VT_I64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{VT_I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},

      Global{GlobalType{VT_F32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{VT_F64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{VT_I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{VT_I64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{VT_I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},
  };

  for (const auto& global : tests) {
    EXPECT_FALSE(Validate(ctx, global));
  }
}

TEST_F(ValidateTest, Global_GlobalGetIndexOOB) {
  const Global global{
      GlobalType{VT_I32, Mutability::Const},
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}};
  EXPECT_FALSE(Validate(ctx, global));
}

TEST_F(ValidateTest, Global_GlobalGet_gc) {
  // The gc proposal allows global.get to reference any immutable global (not
  // just imported ones).
  ctx.features.enable_gc();

  ctx.imported_global_count = 0;
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Const});
  ctx.globals.push_back(GlobalType{VT_I32, Mutability::Var});

  // global.get on immutable global is OK.
  EXPECT_TRUE(Validate(ctx, Global{GlobalType{VT_I32, Mutability::Const},
                                   ConstantExpression{Instruction{
                                       Opcode::GlobalGet, Index{0}}}}));

  // global.get on mutable global is not OK.
  EXPECT_FALSE(Validate(ctx, Global{GlobalType{VT_I32, Mutability::Const},
                                    ConstantExpression{Instruction{
                                        Opcode::GlobalGet, Index{1}}}}));
}

TEST_F(ValidateTest, GlobalType) {
  const GlobalType tests[] = {
      GlobalType{VT_I32, Mutability::Const},
      GlobalType{VT_I64, Mutability::Const},
      GlobalType{VT_F32, Mutability::Const},
      GlobalType{VT_F64, Mutability::Const},
      GlobalType{VT_V128, Mutability::Const},
      GlobalType{VT_Funcref, Mutability::Const},
      GlobalType{VT_Externref, Mutability::Const},
      GlobalType{VT_Exnref, Mutability::Const},
      GlobalType{VT_I32, Mutability::Var},
      GlobalType{VT_I64, Mutability::Var},
      GlobalType{VT_F32, Mutability::Var},
      GlobalType{VT_F64, Mutability::Var},
      GlobalType{VT_Funcref, Mutability::Var},
      GlobalType{VT_Externref, Mutability::Var},
      GlobalType{VT_Exnref, Mutability::Var},
  };

  for (const auto& global_type : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, global_type));
  }
}

TEST_F(ValidateTest, GlobalType_RefType) {
  const GlobalType tests[] = {
      GlobalType{VT_Ref0, Mutability::Const},
      GlobalType{VT_RefNull0, Mutability::Const},
      GlobalType{VT_RefFunc, Mutability::Const},
      GlobalType{VT_RefNullFunc, Mutability::Const},
      GlobalType{VT_Ref0, Mutability::Var},
      GlobalType{VT_RefNull0, Mutability::Var},
      GlobalType{VT_RefFunc, Mutability::Var},
      GlobalType{VT_RefNullFunc, Mutability::Var},
  };

  for (const auto& global_type : tests) {
    ctx.Reset();
    ctx.types.push_back(DefinedType{FunctionType{}});
    ctx.defined_type_count = 1;
    EXPECT_TRUE(Validate(ctx, global_type));
  }
}

TEST_F(ValidateTest, GlobalType_RefType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, GlobalType{VT_Ref0, Mutability::Const}));
}

TEST_F(ValidateTest, Import) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;

  const Import tests[] = {
      Import{"", "", Index{0}},
      Import{"", "", TableType{Limits{0}, RT_Funcref}},
      Import{"", "", MemoryType{Limits{0}}},
      Import{"", "", GlobalType{VT_I32, Mutability::Const}},
      Import{"", "", EventType{EventAttribute::Exception, Index{0}}},
  };

  for (const auto& import : tests) {
    EXPECT_TRUE(Validate(ctx, import));
  }
}

TEST_F(ValidateTest, Import_FunctionIndexOOB) {
  EXPECT_FALSE(Validate(ctx, Import{"", "", Index{0}}));
}

TEST_F(ValidateTest, Import_TooManyTables) {
  const TableType table_type{Limits{0}, RT_Funcref};
  ctx.tables.push_back(table_type);

  EXPECT_FALSE(Validate(ctx, Import{"", "", table_type}));
}

TEST_F(ValidateTest, Import_TooManyMemories) {
  const MemoryType memory_type{Limits{0}};
  ctx.memories.push_back(memory_type);

  EXPECT_FALSE(Validate(ctx, Import{"", "", memory_type}));
}

TEST_F(ValidateTest, Import_GlobalMutVar_MVP) {
  EXPECT_FALSE(
      Validate(ctx, Import{"", "", GlobalType{VT_I32, Mutability::Var}}));
}

TEST_F(ValidateTest, Import_GlobalMutVar_MutableGlobals) {
  ctx.features.enable_mutable_globals();
  EXPECT_TRUE(
      Validate(ctx, Import{"", "", GlobalType{VT_I32, Mutability::Var}}));
}

TEST_F(ValidateTest, Import_Event_IndexOOB) {
  EXPECT_FALSE(Validate(
      ctx, Import{"", "", EventType{EventAttribute::Exception, Index{0}}}));
}

TEST_F(ValidateTest, Import_Event_NonEmptyResult) {
  ctx.types.push_back(DefinedType{FunctionType{{}, {VT_F32}}});
  ctx.defined_type_count = 1;
  EXPECT_FALSE(Validate(
      ctx, Import{"", "", EventType{EventAttribute::Exception, Index{0}}}));
}

TEST_F(ValidateTest, Index) {
  EXPECT_TRUE(ValidateIndex(ctx, 1, 3, "index"));
  EXPECT_FALSE(ValidateIndex(ctx, 3, 3, "index"));
  EXPECT_FALSE(ValidateIndex(ctx, 0, 0, "index"));
}

TEST_F(ValidateTest, Limits) {
  EXPECT_TRUE(Validate(ctx, Limits{0}, 10));
  EXPECT_TRUE(Validate(ctx, Limits{9, 10}, 10));
  // Test that the value is compared, not the string.
  EXPECT_TRUE(
      Validate(ctx, Limits{At{"9"_su8, u32{9}}, At{"10"_su8, u32{10}}}, 10));
}

TEST_F(ValidateTest, Limits_Invalid) {
  EXPECT_FALSE(Validate(ctx, Limits{11}, 10));
  EXPECT_FALSE(Validate(ctx, Limits{9, 11}, 10));
  EXPECT_FALSE(Validate(ctx, Limits{5, 3}, 10));
}

TEST_F(ValidateTest, Locals) {
  EXPECT_TRUE(Validate(ctx, Locals{1, VT_I32}, RequireDefaultable::No));
}

TEST_F(ValidateTest, Locals_Defaultable) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  EXPECT_FALSE(Validate(ctx, Locals{1, VT_Ref0}, RequireDefaultable::Yes));
  EXPECT_TRUE(Validate(ctx, Locals{1, VT_Ref0}, RequireDefaultable::No));
}

TEST_F(ValidateTest, Locals_RefType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, Locals{1, VT_RefNull0}, RequireDefaultable::Yes));
}

TEST_F(ValidateTest, Memory) {
  const Memory tests[] = {
      Memory{MemoryType{Limits{0}}},
      Memory{MemoryType{Limits{1, 10}}},
  };

  for (const auto& memory : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, memory));
  }
}

TEST_F(ValidateTest, Memory_TooManyMemories) {
  ctx.memories.push_back(MemoryType{Limits{0}});
  EXPECT_FALSE(Validate(ctx, Memory{MemoryType{Limits{0}}}));
}

TEST_F(ValidateTest, MemoryType) {
  const MemoryType tests[] = {
      MemoryType{Limits{0}},          MemoryType{Limits{1000}},
      MemoryType{Limits{100, 12345}}, MemoryType{Limits{0, 65535}},
      MemoryType{Limits{0, 65536}},
  };

  for (const auto& memory_type : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, memory_type));
  }
}

TEST_F(ValidateTest, MemoryType_TooLarge) {
  const MemoryType tests[] = {
      MemoryType{Limits{65537}},
      MemoryType{Limits{0, 65537}},
      MemoryType{Limits{0xffffffffu, 0xffffffffu}},
  };

  for (const auto& memory_type : tests) {
    ctx.Reset();
    EXPECT_FALSE(Validate(ctx, memory_type));
  }
}

TEST_F(ValidateTest, MemoryType_Shared_MVP) {
  EXPECT_FALSE(Validate(ctx, MemoryType{Limits{0, 100, Shared::Yes}}));
}

TEST_F(ValidateTest, MemoryType_Shared_Threads) {
  ctx.features.enable_threads();
  EXPECT_TRUE(Validate(ctx, MemoryType{Limits{0, 100, Shared::Yes}}));
}

TEST_F(ValidateTest, MemoryType_Shared_NoMax) {
  ctx.features.enable_threads();
  EXPECT_FALSE(Validate(ctx, MemoryType{Limits{0, nullopt, Shared::Yes}}));
}

TEST_F(ValidateTest, Rtt) {
  EXPECT_TRUE(Validate(ctx, Rtt{0, HT_Any}));
  EXPECT_TRUE(Validate(ctx, Rtt{0, HT_Func}));
  EXPECT_TRUE(Validate(ctx, Rtt{0, HT_Extern}));
  EXPECT_TRUE(Validate(ctx, Rtt{0, HT_I31}));
  EXPECT_TRUE(Validate(ctx, Rtt{0, HT_Eq}));
  EXPECT_TRUE(Validate(ctx, Rtt{0, HT_0}));
  EXPECT_TRUE(Validate(ctx, Rtt{1, HT_Any}));
  EXPECT_TRUE(Validate(ctx, Rtt{1, HT_Func}));
  EXPECT_TRUE(Validate(ctx, Rtt{1, HT_Extern}));
  EXPECT_TRUE(Validate(ctx, Rtt{1, HT_I31}));
  EXPECT_TRUE(Validate(ctx, Rtt{1, HT_Eq}));
  EXPECT_TRUE(Validate(ctx, Rtt{1, HT_0}));
  EXPECT_TRUE(Validate(ctx, Rtt{123, HT_Any}));
  EXPECT_TRUE(Validate(ctx, Rtt{123, HT_Func}));
  EXPECT_TRUE(Validate(ctx, Rtt{123, HT_Extern}));
  EXPECT_TRUE(Validate(ctx, Rtt{123, HT_I31}));
  EXPECT_TRUE(Validate(ctx, Rtt{123, HT_Eq}));
  EXPECT_TRUE(Validate(ctx, Rtt{123, HT_0}));
}

TEST_F(ValidateTest, Start) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(ctx, Start{0}));
}

TEST_F(ValidateTest, Start_FunctionIndexOOB) {
  EXPECT_FALSE(Validate(ctx, Start{0}));
}

TEST_F(ValidateTest, Start_InvalidParamCount) {
  FunctionType function_type{{VT_I32}, {}};
  ctx.types.push_back(DefinedType{function_type});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(ctx, Start{0}));
}

TEST_F(ValidateTest, Start_InvalidResultCount) {
  const FunctionType function_type{{}, {VT_I32}};
  ctx.types.push_back(DefinedType{function_type});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(ctx, Start{0}));
}

TEST_F(ValidateTest, Start_InvalidType) {
  ctx.types.push_back(DefinedType{StructType{}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(ctx, Start{0}));
}

TEST_F(ValidateTest, StorageType) {
  EXPECT_TRUE(Validate(ctx, StorageType{VT_I32}));
  EXPECT_TRUE(Validate(ctx, StorageType{PackedType::I8}));
  EXPECT_TRUE(Validate(ctx, StorageType{PackedType::I16}));
}

TEST_F(ValidateTest, StorageType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, StorageType{VT_Ref1}));
}

TEST_F(ValidateTest, StructType) {
  EXPECT_TRUE(
      Validate(ctx, StructType{FieldTypeList{
                        FieldType{StorageType{VT_I32}, Mutability::Const},
                        FieldType{StorageType{VT_I64}, Mutability::Var}}}));
}

TEST_F(ValidateTest, StructType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, StructType{FieldTypeList{FieldType{
                                 StorageType{VT_Ref1}, Mutability::Const}}}));
}

TEST_F(ValidateTest, Table) {
  const Table tests[] = {
      Table{TableType{Limits{0}, RT_Funcref}},
      Table{TableType{Limits{1, 10}, RT_Funcref}},
  };

  for (const auto& table : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, table));
  }
}

TEST_F(ValidateTest, Table_TooManyTables) {
  const TableType table_type{Limits{0}, RT_Funcref};
  ctx.tables.push_back(table_type);
  EXPECT_FALSE(Validate(ctx, Table{table_type}));
}

TEST_F(ValidateTest, TableType) {
  const TableType tests[] = {
      TableType{Limits{0}, RT_Funcref},
      TableType{Limits{1000}, RT_Funcref},
      TableType{Limits{100, 12345}, RT_Funcref},
      TableType{Limits{0, 0xffffffff}, RT_Funcref},
  };

  for (const auto& table_type : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, table_type));
  }
}

TEST_F(ValidateTest, TableType_RefType) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  EXPECT_TRUE(Validate(ctx, TableType{Limits{0}, RT_RefNull0}));
}

TEST_F(ValidateTest, TableType_RefType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, TableType{Limits{0}, RT_RefNull0}));
}

TEST_F(ValidateTest, TableType_Shared) {
  EXPECT_FALSE(
      Validate(ctx, TableType{Limits{0, 100, Shared::Yes}, RT_Funcref}));
}

TEST_F(ValidateTest, TableType_Defaultable) {
  EXPECT_FALSE(Validate(ctx, TableType{Limits{0}, RT_Ref0}));
}

TEST_F(ValidateTest, DefinedType) {
  EXPECT_TRUE(Validate(ctx, DefinedType{FunctionType{}}));
}

TEST_F(ValidateTest, DefinedType_gc) {
  EXPECT_TRUE(Validate(ctx, DefinedType{ArrayType{FieldType{
                                StorageType{VT_I32}, Mutability::Const}}}));

  EXPECT_TRUE(Validate(ctx, DefinedType{StructType{FieldTypeList{FieldType{
                                StorageType{VT_I32}, Mutability::Const}}}}));
}

TEST_F(ValidateTest, ValueType) {
  const ValueType tests[] = {
      VT_I32, VT_I64, VT_F32, VT_F64, VT_V128, VT_Externref,
  };

  for (auto value_type : tests) {
    ctx.Reset();
    EXPECT_TRUE(Validate(ctx, value_type, value_type));
  }
}

TEST_F(ValidateTest, ValueType_Mismatch) {
  const ValueType tests[] = {
      VT_I32, VT_I64, VT_F32, VT_F64, VT_V128, VT_Externref,
  };

  for (auto value_type1 : tests) {
    for (auto value_type2 : tests) {
      if (value_type1 == value_type2) {
        continue;
      }
      ctx.Reset();
      EXPECT_FALSE(Validate(ctx, value_type1, value_type2));
    }
  }
}

TEST_F(ValidateTest, ValueType_RefType) {
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  EXPECT_TRUE(Validate(ctx, VT_Ref0));
}

TEST_F(ValidateTest, ValueType_RefType_IndexOOB) {
  EXPECT_FALSE(Validate(ctx, VT_Ref0));
}

TEST_F(ValidateTest, ValueType_FuncrefSubtyping) {
  ctx.types.push_back(DefinedType{FunctionType{}});

  // ref null 0 is a supertype of ref 0.
  EXPECT_TRUE(Validate(ctx, VT_RefNull0, VT_Ref0));

  // funcref (aka ref null func) is a supertype of ref N.
  EXPECT_TRUE(Validate(ctx, VT_Funcref, VT_RefNullFunc));
  EXPECT_TRUE(Validate(ctx, VT_Funcref, VT_RefNull0));
  EXPECT_TRUE(Validate(ctx, VT_Funcref, VT_Ref0));
  EXPECT_TRUE(Validate(ctx, VT_RefNullFunc, VT_RefNull0));
  EXPECT_TRUE(Validate(ctx, VT_RefNullFunc, VT_Ref0));
  EXPECT_TRUE(Validate(ctx, VT_RefFunc, VT_Ref0));
}

TEST_F(ValidateTest, Module) {
  Module module;
  module.types.push_back(DefinedType{FunctionType{}});
  module.imports.push_back(Import{"a"_sv, "b"_sv, Index{0}});
  module.functions.push_back(Function{Index{0}});
  module.tables.push_back(Table{TableType{Limits{0}, RT_Funcref}});
  module.memories.push_back(Memory{MemoryType{Limits{0}}});
  module.globals.push_back(
      Global{GlobalType{VT_I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}});
  module.events.push_back(
      Event{EventType{EventAttribute::Exception, Index{0}}});
  module.exports.push_back(Export{ExternalKind::Function, "c"_sv, 0});
  module.start = Start{Index{0}};
  module.element_segments.push_back(ElementSegment{
      Index{0}, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ElementList{ElementListWithIndexes{ExternalKind::Function, {0, 0}}}});
  module.codes.push_back(UnpackedCode{
      LocalsList{},
      UnpackedExpression{InstructionList{Instruction{Opcode::End}}}});
  module.data_segments.push_back(DataSegment{
      Index{0}, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      "hi"_su8});

  EXPECT_TRUE(Validate(ctx, module));
}

TEST_F(ValidateTest, TypeIndexOOBAfterTypeSection) {
  // Declare two types, but don't define them. This could happen if the types
  // aren't actually defined, or if they could not be parsed.
  BeginTypeSection(ctx, 2);
  EndTypeSection(ctx);

  // Make sure that anything that references the type section will correctly
  // fail to validate.
  EXPECT_FALSE(Validate(
      ctx, Import{"", "", EventType{EventAttribute::Exception, Index{0}}}));

  EXPECT_FALSE(Validate(ctx, Function{0}));

  EXPECT_FALSE(Validate(ctx, VT_Ref0));
}
