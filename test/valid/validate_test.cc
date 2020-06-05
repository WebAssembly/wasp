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

#include "test/valid/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;

TEST(ValidateTest, Code) {
  Code code{LocalsList{Locals{2, ValueType::I32}},
            Expression{"\x20\x00"     // local.get 0
                       "\x20\x01"     // local.get 1
                       "\x6a"         // i32.add
                       "\x0b"_su8}};  // end
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{{}, {ValueType::I32}}});
  context.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(context, code));
}

TEST(ValidateTest, ConstantExpression_Const) {
  const struct {
    Instruction instr;
    ValueType valtype;
  } tests[] = {
      {Instruction{Opcode::I32Const, s32{0}}, ValueType::I32},
      {Instruction{Opcode::I64Const, s64{0}}, ValueType::I64},
      {Instruction{Opcode::F32Const, f32{0}}, ValueType::F32},
      {Instruction{Opcode::F64Const, f64{0}}, ValueType::F64},
      {Instruction{Opcode::V128Const, v128{}}, ValueType::V128},
  };

  for (const auto& test : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, ConstantExpression{test.instr},
                         ConstantExpressionKind::Other, test.valtype, 0));
  }
}

TEST(ValidateTest, ConstantExpression_Global) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::I64, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F64, Mutability::Const});
  auto max = context.globals.size();

  EXPECT_TRUE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      ConstantExpressionKind::GlobalInit, ValueType::I32, max));
  EXPECT_TRUE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
      ConstantExpressionKind::GlobalInit, ValueType::I64, max));
  EXPECT_TRUE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}},
      ConstantExpressionKind::GlobalInit, ValueType::F32, max));
  EXPECT_TRUE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}},
      ConstantExpressionKind::GlobalInit, ValueType::F64, max));
}

TEST(ValidateTest, ConstantExpression_InvalidOpcode) {
  const Instruction tests[] = {
      Instruction{Opcode::Unreachable},
      Instruction{Opcode::I32Add},
      Instruction{Opcode::Br, Index{0}},
      Instruction{Opcode::LocalGet, Index{0}},
      Instruction{Opcode::V128Const, v128{}},
      Instruction{Opcode::RefNull, ReferenceType::Funcref},
  };

  for (const auto& instr : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(Validate(context, ConstantExpression{instr},
                          ConstantExpressionKind::Other, ValueType::I32, 0));
  }
}

TEST(ValidateTest, ConstantExpression_ConstMismatch) {
  const struct {
    Instruction instr;
    ValueType valtype;
  } tests[] = {
      {Instruction{Opcode::I32Const, s32{0}}, ValueType::I64},
      {Instruction{Opcode::I64Const, s64{0}}, ValueType::F32},
      {Instruction{Opcode::F32Const, f32{0}}, ValueType::F64},
      {Instruction{Opcode::F64Const, f64{0}}, ValueType::I32},
  };

  for (const auto& test : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(Validate(context, ConstantExpression{test.instr},
                          ConstantExpressionKind::Other, test.valtype, 0));
  }
}

TEST(ValidateTest, ConstantExpression_GlobalIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  auto max = context.globals.size();

  EXPECT_FALSE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
      ConstantExpressionKind::Other, ValueType::I32, max));
}

TEST(ValidateTest, ConstantExpression_GlobalTypeMismatch) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::I64, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F64, Mutability::Const});
  auto max = context.globals.size();

  EXPECT_FALSE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      ConstantExpressionKind::Other, ValueType::I64, max));
  EXPECT_FALSE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
      ConstantExpressionKind::Other, ValueType::F32, max));
  EXPECT_FALSE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}},
      ConstantExpressionKind::Other, ValueType::F64, max));
  EXPECT_FALSE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}},
      ConstantExpressionKind::Other, ValueType::I32, max));
}

TEST(ValidateTest, ConstantExpression_GlobalMutVar) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  auto max = context.globals.size();

  EXPECT_FALSE(Validate(
      context, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      ConstantExpressionKind::Other, ValueType::I32, max));
}

TEST(ValidateTest, ConstantExpression_WrongInstructionCount) {
  TestErrors errors;
  Context context{errors};

  // Too few instructions.
  EXPECT_FALSE(Validate(context, ConstantExpression{},
                        ConstantExpressionKind::Other, ValueType::I32, 0));
  // Too many instructions.
  EXPECT_FALSE(Validate(context,
                        ConstantExpression{InstructionList{
                            Instruction{Opcode::GlobalGet, Index{0}},
                            Instruction{Opcode::I32Const, s32{0}}}},
                        ConstantExpressionKind::Other, ValueType::I32, 0));
}

TEST(ValidateTest, ConstantExpression_Funcref) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});

  // Using ref.func in the global section implicitly declares that function.
  EXPECT_TRUE(Validate(
      context, ConstantExpression{Instruction{Opcode::RefFunc, Index{0}}},
      ConstantExpressionKind::GlobalInit, ValueType::Funcref, 0));

  EXPECT_EQ(1u, context.declared_functions.size());
}

TEST(ValidateTest, DataCount) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(context, DataCount{1}));
  ASSERT_TRUE(context.declared_data_count);
  EXPECT_EQ(1u, context.declared_data_count);
}

TEST(ValidateTest, DataSegment_Active) {
  TestErrors errors;
  Context context{errors};
  context.memories.push_back(MemoryType{Limits{0}});
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});

  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment tests[] = {
      DataSegment{0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
                  span},
      DataSegment{0,
                  ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
                  span},
  };

  for (const auto& data_segment : tests) {
    EXPECT_TRUE(Validate(context, data_segment));
  }
}

TEST(ValidateTest, DataSegment_Active_MemoryIndexOOB) {
  TestErrors errors;
  Context context{errors};
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, span};
  EXPECT_FALSE(Validate(context, data_segment));
}

TEST(ValidateTest, DataSegment_Active_GlobalIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.memories.push_back(MemoryType{Limits{0}});
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, span};
  EXPECT_FALSE(Validate(context, data_segment));
}

TEST(ValidateTest, ElementExpression) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});

  const Instruction tests[] = {
      Instruction{Opcode::RefNull},
      Instruction{Opcode::RefFunc, Index{0}},
  };

  for (const auto& instr : tests) {
    EXPECT_TRUE(
        Validate(context, ElementExpression{instr}, ReferenceType::Funcref));
  }
}

TEST(ValidateTest, ElementExpression_InvalidOpcode) {
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
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(
        Validate(context, ElementExpression{instr}, ReferenceType::Funcref));
  }
}

TEST(ValidateTest, ElementExpression_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(
      context, ElementExpression{Instruction{Opcode::RefFunc, Index{1}}},
      ReferenceType::Funcref));
}

TEST(ValidateTest, ElementSegment_Active) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});
  context.functions.push_back(Function{0});
  context.tables.push_back(TableType{Limits{0}, ReferenceType::Funcref});
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});

  const ElementSegment tests[] = {
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
                     ElementListWithIndexes{ExternalKind::Function, {0, 1}}},
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
          ElementListWithIndexes{ExternalKind::Function, {}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(context, element_segment));
  }
}

TEST(ValidateTest, ElementSegment_Passive) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{SegmentType::Passive,
                     ElementListWithExpressions{ReferenceType::Funcref, {}}},
      ElementSegment{
          SegmentType::Passive,
          ElementListWithExpressions{
              ReferenceType::Funcref,
              {ElementExpression{Instruction{Opcode::RefNull}},
               ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(context, element_segment));
  }
}

TEST(ValidateTest, ElementSegment_Declared) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{SegmentType::Declared,
                     ElementListWithIndexes{ExternalKind::Function, {0}}},
      ElementSegment{
          SegmentType::Declared,
          ElementListWithExpressions{
              ReferenceType::Funcref,
              {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}}},
  };

  EXPECT_EQ(0u, context.declared_functions.count(0));
  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(context, element_segment));
  }
  EXPECT_EQ(1u, context.declared_functions.count(0));
}

TEST(ValidateTest, ElementSegment_Active_TypeMismatch) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});
  context.tables.push_back(TableType{Limits{0}, ReferenceType::Funcref});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});

  const ElementSegment tests[] = {
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::F32Const, f32{0}}},
                     ElementListWithIndexes{ExternalKind::Function, {}}},
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
          ElementListWithIndexes{ExternalKind::Function, {}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_FALSE(Validate(context, element_segment));
  }
}

TEST(ValidateTest, ElementSegment_Active_TableIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ElementListWithIndexes{ExternalKind::Function, {}}};
  EXPECT_FALSE(Validate(context, element_segment));
}

TEST(ValidateTest, ElementSegment_Active_GlobalIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.tables.push_back(TableType{Limits{0}, ReferenceType::Funcref});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      ElementListWithIndexes{ExternalKind::Function, {}}};
  EXPECT_FALSE(Validate(context, element_segment));
}

TEST(ValidateTest, ElementSegment_Active_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.tables.push_back(TableType{Limits{0}, ReferenceType::Funcref});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ElementListWithIndexes{ExternalKind::Function, {0}}};
  EXPECT_FALSE(Validate(context, element_segment));
}

TEST(ValidateTest, ElementSegment_Passive_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  const ElementSegment element_segment{
      SegmentType::Passive,
      ElementListWithExpressions{
          ReferenceType::Funcref,
          {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}}};
  EXPECT_FALSE(Validate(context, element_segment));
}

TEST(ValidateTest, ReferenceType) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(
      Validate(context, ReferenceType::Funcref, ReferenceType::Funcref));
}

TEST(ValidateTest, Export) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  context.tables.push_back(TableType{Limits{1}, ReferenceType::Funcref});
  context.memories.push_back(MemoryType{Limits{1}});
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.events.push_back(EventType{EventAttribute::Exception, Index{0}});

  const Export tests[] = {
      Export{ExternalKind::Function, "f", 0},
      Export{ExternalKind::Table, "t", 0},
      Export{ExternalKind::Memory, "m", 0},
      Export{ExternalKind::Global, "g", 0},
      Export{ExternalKind::Event, "e", 0},
  };

  for (const auto& export_ : tests) {
    EXPECT_TRUE(Validate(context, export_));
  }

  // Exporting a function marks it as declared.
  EXPECT_EQ(1u, context.declared_functions.size());
}

TEST(ValidateTest, Export_IndexOOB) {
  const Export tests[] = {
      Export{ExternalKind::Function, "", 0},
      Export{ExternalKind::Table, "", 0},
      Export{ExternalKind::Memory, "", 0},
      Export{ExternalKind::Global, "", 0},
      Export{ExternalKind::Event, "", 0},
  };

  for (const auto& export_ : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(Validate(context, export_));
  }
}

TEST(ValidateTest, Export_GlobalMutVar_MVP) {
  Features features;
  features.disable_mutable_globals();
  TestErrors errors;
  Context context{features, errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  EXPECT_FALSE(Validate(context, Export{ExternalKind::Global, "", 0}));
}

TEST(ValidateTest, Export_GlobalMutVar_MutableGlobals) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  EXPECT_TRUE(Validate(context, Export{ExternalKind::Global, "", 0}));
}

TEST(ValidateTest, Export_Duplicate) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(context, Export{ExternalKind::Function, "hi", 0}));
  EXPECT_FALSE(Validate(context, Export{ExternalKind::Function, "hi", 0}));
}

TEST(ValidateTest, Event) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  EXPECT_TRUE(
      Validate(context, Event{EventType{EventAttribute::Exception, Index{0}}}));
}

TEST(ValidateTest, EventType) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{{ValueType::I32}, {}}});
  EXPECT_TRUE(
      Validate(context, EventType{EventAttribute::Exception, Index{0}}));
}

TEST(ValidateTest, EventType_IndexOOB) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  EXPECT_FALSE(
      Validate(context, EventType{EventAttribute::Exception, Index{0}}));
}

TEST(ValidateTest, EventType_NonEmptyResult) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{{}, {ValueType::I32}}});
  EXPECT_FALSE(
      Validate(context, EventType{EventAttribute::Exception, Index{0}}));
}

TEST(ValidateTest, Function) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  EXPECT_TRUE(Validate(context, Function{0}));
}

TEST(ValidateTest, Function_IndexOOB) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(context, Function{0}));
}

TEST(ValidateTest, FunctionType) {
  const FunctionType tests[] = {
      FunctionType{},
      FunctionType{{ValueType::I32}, {}},
      FunctionType{{ValueType::F32}, {}},
      FunctionType{{ValueType::F64}, {}},
      FunctionType{{ValueType::I64}, {ValueType::I32}},
      FunctionType{{ValueType::I64, ValueType::F32}, {ValueType::F32}},
      FunctionType{{}, {ValueType::F64}},
      FunctionType{{ValueType::I64, ValueType::I64, ValueType::I64},
                   {ValueType::I64}},
  };

  for (const auto& function_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, function_type));
  }
}

TEST(ValidateTest, FunctionType_MultiReturn_MVP) {
  const FunctionType tests[] = {
      FunctionType{{}, {ValueType::I32, ValueType::I32}},
      FunctionType{{}, {ValueType::I32, ValueType::I64, ValueType::F32}},
  };

  for (const auto& function_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(Validate(context, function_type));
  }
}

TEST(ValidateTest, FunctionType_MultiReturn) {
  Features features;
  features.enable_multi_value();

  const FunctionType tests[] = {
      FunctionType{{}, {ValueType::I32, ValueType::I32}},
      FunctionType{{}, {ValueType::I32, ValueType::I64, ValueType::F32}},
  };

  for (const auto& function_type : tests) {
    TestErrors errors;
    Context context{features, errors};
    EXPECT_TRUE(Validate(context, function_type));
  }
}

TEST(ValidateTest, Global) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.imported_global_count = 1;

  const Global tests[] = {
      Global{GlobalType{ValueType::I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{ValueType::I64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{ValueType::F32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{ValueType::F64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{ValueType::I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},

      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{ValueType::I64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{ValueType::F32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{ValueType::F64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},
  };

  for (const auto& global : tests) {
    EXPECT_TRUE(Validate(context, global));
  }
}

TEST(ValidateTest, Global_TypeMismatch) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});
  context.imported_global_count = 1;

  const Global tests[] = {
      Global{GlobalType{ValueType::F32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{ValueType::F64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{ValueType::I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{ValueType::I64, Mutability::Const},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{ValueType::I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},

      Global{GlobalType{ValueType::F32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
      Global{GlobalType{ValueType::F64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}},
      Global{GlobalType{ValueType::I64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::F64Const, f64{0}}}},
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}},
  };

  for (const auto& global : tests) {
    EXPECT_FALSE(Validate(context, global));
  }
}

TEST(ValidateTest, Global_GlobalGetIndexOOB) {
  TestErrors errors;
  Context context{errors};
  const Global global{
      GlobalType{ValueType::I32, Mutability::Const},
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}};
  EXPECT_FALSE(Validate(context, global));
}

TEST(ValidateTest, GlobalType) {
  const GlobalType tests[] = {
      GlobalType{ValueType::I32, Mutability::Const},
      GlobalType{ValueType::I64, Mutability::Const},
      GlobalType{ValueType::F32, Mutability::Const},
      GlobalType{ValueType::F64, Mutability::Const},
      GlobalType{ValueType::I32, Mutability::Var},
      GlobalType{ValueType::I64, Mutability::Var},
      GlobalType{ValueType::F32, Mutability::Var},
      GlobalType{ValueType::F64, Mutability::Var},
  };

  for (const auto& global_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, global_type));
  }
}

TEST(ValidateTest, Import) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});

  const Import tests[] = {
      Import{"", "", Index{0}},
      Import{"", "", TableType{Limits{0}, ReferenceType::Funcref}},
      Import{"", "", MemoryType{Limits{0}}},
      Import{"", "", GlobalType{ValueType::I32, Mutability::Const}},
      Import{"", "", EventType{EventAttribute::Exception, Index{0}}},
  };

  for (const auto& import : tests) {
    EXPECT_TRUE(Validate(context, import));
  }
}

TEST(ValidateTest, Import_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(context, Import{"", "", Index{0}}));
}

TEST(ValidateTest, Import_TooManyTables) {
  TestErrors errors;
  Context context{errors};
  const TableType table_type{Limits{0}, ReferenceType::Funcref};
  context.tables.push_back(table_type);

  EXPECT_FALSE(Validate(context, Import{"", "", table_type}));
}

TEST(ValidateTest, Import_TooManyMemories) {
  TestErrors errors;
  Context context{errors};
  const MemoryType memory_type{Limits{0}};
  context.memories.push_back(memory_type);

  EXPECT_FALSE(Validate(context, Import{"", "", memory_type}));
}

TEST(ValidateTest, Import_GlobalMutVar_MVP) {
  Features features;
  features.disable_mutable_globals();
  TestErrors errors;
  Context context{features, errors};
  EXPECT_FALSE(Validate(
      context, Import{"", "", GlobalType{ValueType::I32, Mutability::Var}}));
}

TEST(ValidateTest, Import_GlobalMutVar_MutableGlobals) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(
      context, Import{"", "", GlobalType{ValueType::I32, Mutability::Var}}));
}

TEST(ValidateTest, Import_Event_IndexOOB) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  EXPECT_FALSE(Validate(
      context, Import{"", "", EventType{EventAttribute::Exception, Index{0}}}));
}

TEST(ValidateTest, Import_Event_NonEmptyResult) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{{}, {ValueType::F32}}});
  EXPECT_FALSE(Validate(
      context, Import{"", "", EventType{EventAttribute::Exception, Index{0}}}));
}

TEST(ValidateTest, Index) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(ValidateIndex(context, 1, 3, "index"));
  EXPECT_FALSE(ValidateIndex(context, 3, 3, "index"));
  EXPECT_FALSE(ValidateIndex(context, 0, 0, "index"));
}

TEST(ValidateTest, Limits) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(context, Limits{0}, 10));
  EXPECT_TRUE(Validate(context, Limits{9, 10}, 10));
  // Test that the value is compared, not the string.
  EXPECT_TRUE(Validate(
      context, Limits{MakeAt("9"_su8, u32{9}), MakeAt("10"_su8, u32{10})}, 10));
}

TEST(ValidateTest, Limits_Invalid) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(context, Limits{11}, 10));
  EXPECT_FALSE(Validate(context, Limits{9, 11}, 10));
  EXPECT_FALSE(Validate(context, Limits{5, 3}, 10));
}

TEST(ValidateTest, Memory) {
  const Memory tests[] = {
      Memory{MemoryType{Limits{0}}},
      Memory{MemoryType{Limits{1, 10}}},
  };

  for (const auto& memory : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, memory));
  }
}

TEST(ValidateTest, Memory_TooManyMemories) {
  TestErrors errors;
  Context context{errors};
  context.memories.push_back(MemoryType{Limits{0}});
  EXPECT_FALSE(Validate(context, Memory{MemoryType{Limits{0}}}));
}

TEST(ValidateTest, MemoryType) {
  const MemoryType tests[] = {
      MemoryType{Limits{0}},
      MemoryType{Limits{1000}},
      MemoryType{Limits{100, 12345}},
      MemoryType{Limits{0, 65535}},
      MemoryType{Limits{0, 65536}},
  };

  for (const auto& memory_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, memory_type));
  }
}

TEST(ValidateTest, MemoryType_TooLarge) {
  const MemoryType tests[] = {
      MemoryType{Limits{65537}},
      MemoryType{Limits{0, 65537}},
      MemoryType{Limits{0xffffffffu, 0xffffffffu}},
  };

  for (const auto& memory_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(Validate(context, memory_type));
  }
}

TEST(ValidateTest, MemoryType_Shared_MVP) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(context, MemoryType{Limits{0, 100, Shared::Yes}}));
}

TEST(ValidateTest, MemoryType_Shared_Threads) {
  Features features;
  features.enable_threads();
  TestErrors errors;
  Context context{features, errors};
  EXPECT_TRUE(Validate(context, MemoryType{Limits{0, 100, Shared::Yes}}));
}

TEST(ValidateTest, Start) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(context, Start{0}));
}

TEST(ValidateTest, Start_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(context, Start{0}));
}

TEST(ValidateTest, Start_InvalidParamCount) {
  FunctionType function_type{{ValueType::I32}, {}};
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{function_type});
  context.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(context, Start{0}));
}

TEST(ValidateTest, Start_InvalidResultCount) {
  TestErrors errors;
  Context context{errors};
  const FunctionType function_type{{}, {ValueType::I32}};
  context.types.push_back(TypeEntry{function_type});
  context.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(context, Start{0}));
}

TEST(ValidateTest, Table) {
  const Table tests[] = {
      Table{TableType{Limits{0}, ReferenceType::Funcref}},
      Table{TableType{Limits{1, 10}, ReferenceType::Funcref}},
  };

  for (const auto& table : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, table));
  }
}

TEST(ValidateTest, Table_TooManyTables) {
  TestErrors errors;
  Context context{errors};
  const TableType table_type{Limits{0}, ReferenceType::Funcref};
  context.tables.push_back(table_type);
  EXPECT_FALSE(Validate(context, Table{table_type}));
}

TEST(ValidateTest, TableType) {
  const TableType tests[] = {
      TableType{Limits{0}, ReferenceType::Funcref},
      TableType{Limits{1000}, ReferenceType::Funcref},
      TableType{Limits{100, 12345}, ReferenceType::Funcref},
      TableType{Limits{0, 0xffffffff}, ReferenceType::Funcref},
  };

  for (const auto& table_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, table_type));
  }
}

TEST(ValidateTest, TableType_Shared) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(
      context, TableType{Limits{0, 100, Shared::Yes}, ReferenceType::Funcref}));
}

TEST(ValidateTest, TypeEntry) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(context, TypeEntry{FunctionType{}}));
}

TEST(ValidateTest, ValueType) {
  const ValueType tests[] = {
      ValueType::I32, ValueType::I64,  ValueType::F32,
      ValueType::F64, ValueType::V128, ValueType::Externref,
  };

  for (auto value_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(context, value_type, value_type));
  }
}

TEST(ValidateTest, ValueType_Mismatch) {
  const ValueType tests[] = {
      ValueType::I32, ValueType::I64,  ValueType::F32,
      ValueType::F64, ValueType::V128, ValueType::Externref,
  };

  for (auto value_type1 : tests) {
    for (auto value_type2 : tests) {
      if (value_type1 == value_type2) {
        continue;
      }
      TestErrors errors;
      Context context{errors};
      EXPECT_FALSE(Validate(context, value_type1, value_type2));
    }
  }
}

TEST(ValidateTest, Module) {
  TestErrors errors;
  Context context{errors};

  Module module;
  module.types.push_back(TypeEntry{});
  module.imports.push_back(Import{"a"_sv, "b"_sv, Index{0}});
  module.functions.push_back(Function{Index{0}});
  module.tables.push_back(Table{TableType{Limits{0}, ReferenceType::Funcref}});
  module.memories.push_back(Memory{MemoryType{Limits{0}}});
  module.globals.push_back(
      Global{GlobalType{ValueType::I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}});
  module.events.push_back(
      Event{EventType{EventAttribute::Exception, Index{0}}});
  module.exports.push_back(Export{ExternalKind::Function, "c"_sv, 0});
  module.start = Start{Index{0}};
  module.element_segments.push_back(ElementSegment{
      Index{0}, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ElementList{ElementListWithIndexes{ExternalKind::Function, {0, 0}}}});
  module.codes.push_back(Code{LocalsList{}, Expression{"\x0b"_su8}});
  module.data_segments.push_back(DataSegment{
      Index{0}, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      "hi"_su8});

  EXPECT_TRUE(Validate(context, module));
}
