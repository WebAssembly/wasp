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

TEST(ValidateTest, ConstantExpression_Const) {
  const struct {
    Instruction instr;
    ValueType valtype;
  } tests[] = {
      {Instruction{Opcode::I32Const, s32{0}}, ValueType::I32},
      {Instruction{Opcode::I64Const, s64{0}}, ValueType::I64},
      {Instruction{Opcode::F32Const, f32{0}}, ValueType::F32},
      {Instruction{Opcode::F64Const, f64{0}}, ValueType::F64},
  };

  for (const auto& test : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(
        Validate(ConstantExpression{test.instr}, test.valtype, 0, context));
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

  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
               ValueType::I32, max, context));
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
               ValueType::I64, max, context));
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}},
               ValueType::F32, max, context));
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}},
               ValueType::F64, max, context));
}

TEST(ValidateTest, ConstantExpression_InvalidOpcode) {
  const Instruction tests[] = {
      Instruction{Opcode::Unreachable},
      Instruction{Opcode::I32Add},
      Instruction{Opcode::Br, Index{0}},
      Instruction{Opcode::LocalGet, Index{0}},
      Instruction{Opcode::V128Const, v128{}},
      Instruction{Opcode::RefNull},
  };

  for (const auto& instr : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_FALSE(
        Validate(ConstantExpression{instr}, ValueType::I32, 0, context));
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
    EXPECT_FALSE(
        Validate(ConstantExpression{test.instr}, test.valtype, 0, context));
  }
}

TEST(ValidateTest, ConstantExpression_GlobalIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  auto max = context.globals.size();

  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
               ValueType::I32, max, context));
}

TEST(ValidateTest, ConstantExpression_GlobalTypeMismatch) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::I64, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F64, Mutability::Const});
  auto max = context.globals.size();

  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
               ValueType::I64, max, context));
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
               ValueType::F32, max, context));
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}},
               ValueType::F64, max, context));
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}},
               ValueType::I32, max, context));
}

TEST(ValidateTest, ConstantExpression_GlobalMutVar) {
  TestErrors errors;
  Context context{errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  auto max = context.globals.size();

  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
               ValueType::I32, max, context));
}

TEST(ValidateTest, DataCount) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(DataCount{1}, context));
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
    EXPECT_TRUE(Validate(data_segment, context));
  }
}

TEST(ValidateTest, DataSegment_Active_MemoryIndexOOB) {
  TestErrors errors;
  Context context{errors};
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, span};
  EXPECT_FALSE(Validate(data_segment, context));
}

TEST(ValidateTest, DataSegment_Active_GlobalIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.memories.push_back(MemoryType{Limits{0}});
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, span};
  EXPECT_FALSE(Validate(data_segment, context));
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
    EXPECT_TRUE(Validate(ElementExpression{instr}, ReferenceType::Funcref,
                         context));
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
    EXPECT_FALSE(Validate(ElementExpression{instr}, ReferenceType::Funcref,
                          context));
  }
}

TEST(ValidateTest, ElementExpression_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});
  EXPECT_FALSE(
      Validate(ElementExpression{Instruction{Opcode::RefFunc, Index{1}}},
               ReferenceType::Funcref, context));
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
                     ExternalKind::Function,
                     {0, 1}},
      ElementSegment{
          0,
          ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
          ExternalKind::Function,
          {}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(element_segment, context));
  }
}

TEST(ValidateTest, ElementSegment_Passive) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{SegmentType::Passive, ReferenceType::Funcref, {}},
      ElementSegment{
          SegmentType::Passive,
          ReferenceType::Funcref,
          {ElementExpression{Instruction{Opcode::RefNull}},
           ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(element_segment, context));
  }
}

TEST(ValidateTest, ElementSegment_Declared) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{SegmentType::Declared, ExternalKind::Function, {0}},
      ElementSegment{
          SegmentType::Declared,
          ReferenceType::Funcref,
          {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}},
  };

  EXPECT_EQ(0u, context.declared_functions.count(0));
  for (const auto& element_segment : tests) {
    EXPECT_TRUE(Validate(element_segment, context));
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
                     ExternalKind::Function,
                     {}},
      ElementSegment{
          0,
          ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
          ExternalKind::Function,
          {}},
  };

  for (const auto& element_segment : tests) {
    EXPECT_FALSE(Validate(element_segment, context));
  }
}

TEST(ValidateTest, ElementSegment_Active_TableIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.functions.push_back(Function{0});
  const ElementSegment element_segment{
      0,
      ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ExternalKind::Function,
      {}};
  EXPECT_FALSE(Validate(element_segment, context));
}

TEST(ValidateTest, ElementSegment_Active_GlobalIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.tables.push_back(TableType{Limits{0}, ReferenceType::Funcref});
  const ElementSegment element_segment{
      0,
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      ExternalKind::Function,
      {}};
  EXPECT_FALSE(Validate(element_segment, context));
}

TEST(ValidateTest, ElementSegment_Active_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.tables.push_back(TableType{Limits{0}, ReferenceType::Funcref});
  const ElementSegment element_segment{
      0,
      ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      ExternalKind::Function,
      {0}};
  EXPECT_FALSE(Validate(element_segment, context));
}

TEST(ValidateTest, ElementSegment_Passive_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  const ElementSegment element_segment{
      SegmentType::Passive,
      ReferenceType::Funcref,
      {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}};
  EXPECT_FALSE(Validate(element_segment, context));
}

TEST(ValidateTest, ReferenceType) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(ReferenceType::Funcref, ReferenceType::Funcref, context));
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
    EXPECT_TRUE(Validate(export_, context));
  }
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
    EXPECT_FALSE(Validate(export_, context));
  }
}

TEST(ValidateTest, Export_GlobalMutVar_MVP) {
  Features features;
  features.disable_mutable_globals();
  TestErrors errors;
  Context context{features, errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  EXPECT_FALSE(
      Validate(Export{ExternalKind::Global, "", 0}, context));
}

TEST(ValidateTest, Export_GlobalMutVar_MutableGlobals) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  EXPECT_TRUE(
      Validate(Export{ExternalKind::Global, "", 0}, context));
}

TEST(ValidateTest, Export_Duplicate) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(Export{ExternalKind::Function, "hi", 0}, context));
  EXPECT_FALSE(Validate(Export{ExternalKind::Function, "hi", 0}, context));
}

TEST(ValidateTest, Event) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  EXPECT_TRUE(Validate(Event{EventType{EventAttribute::Exception, Index{0}}},
                       context));
}

TEST(ValidateTest, EventType) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{{ValueType::I32}, {}}});
  EXPECT_TRUE(
      Validate(EventType{EventAttribute::Exception, Index{0}}, context));
}

TEST(ValidateTest, EventType_IndexOOB) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  EXPECT_FALSE(
      Validate(EventType{EventAttribute::Exception, Index{0}}, context));
}

TEST(ValidateTest, EventType_NonEmptyResult) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{{}, {ValueType::I32}}});
  EXPECT_FALSE(
      Validate(EventType{EventAttribute::Exception, Index{0}}, context));
}

TEST(ValidateTest, Function) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  EXPECT_TRUE(Validate(Function{0}, context));
}

TEST(ValidateTest, Function_IndexOOB) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(Function{0}, context));
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
    EXPECT_TRUE(Validate(function_type, context));
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
    EXPECT_FALSE(Validate(function_type, context));
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
    EXPECT_TRUE(Validate(function_type, context));
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
    EXPECT_TRUE(Validate(global, context));
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
    EXPECT_FALSE(Validate(global, context));
  }
}

TEST(ValidateTest, Global_GlobalGetIndexOOB) {
  TestErrors errors;
  Context context{errors};
  const Global global{
      GlobalType{ValueType::I32, Mutability::Const},
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}};
  EXPECT_FALSE(Validate(global, context));
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
    EXPECT_TRUE(Validate(global_type, context));
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
    EXPECT_TRUE(Validate(import, context));
  }
}

TEST(ValidateTest, Import_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(Import{"", "", Index{0}}, context));
}

TEST(ValidateTest, Import_TooManyTables) {
  TestErrors errors;
  Context context{errors};
  const TableType table_type{Limits{0}, ReferenceType::Funcref};
  context.tables.push_back(table_type);

  EXPECT_FALSE(
      Validate(Import{"", "", table_type}, context));
}

TEST(ValidateTest, Import_TooManyMemories) {
  TestErrors errors;
  Context context{errors};
  const MemoryType memory_type{Limits{0}};
  context.memories.push_back(memory_type);

  EXPECT_FALSE(
      Validate(Import{"", "", memory_type}, context));
}

TEST(ValidateTest, Import_GlobalMutVar_MVP) {
  Features features;
  features.disable_mutable_globals();
  TestErrors errors;
  Context context{features, errors};
  EXPECT_FALSE(Validate(
      Import{"", "", GlobalType{ValueType::I32, Mutability::Var}}, context));
}

TEST(ValidateTest, Import_GlobalMutVar_MutableGlobals) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(
      Import{"", "", GlobalType{ValueType::I32, Mutability::Var}}, context));
}

TEST(ValidateTest, Import_Event_IndexOOB) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  EXPECT_FALSE(Validate(
      Import{"", "", EventType{EventAttribute::Exception, Index{0}}}, context));
}

TEST(ValidateTest, Import_Event_NonEmptyResult) {
  Features features;
  TestErrors errors;
  Context context{features, errors};
  context.types.push_back(TypeEntry{FunctionType{{}, {ValueType::F32}}});
  EXPECT_FALSE(Validate(
      Import{"", "", EventType{EventAttribute::Exception, Index{0}}}, context));
}

TEST(ValidateTest, Index) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(ValidateIndex(1, 3, "index", context));
  EXPECT_FALSE(ValidateIndex(3, 3, "index", context));
  EXPECT_FALSE(ValidateIndex(0, 0, "index", context));
}

TEST(ValidateTest, Limits) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(Limits{0}, 10, context));
  EXPECT_TRUE(Validate(Limits{9, 10}, 10, context));
}

TEST(ValidateTest, Limits_Invalid) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(Limits{11}, 10, context));
  EXPECT_FALSE(Validate(Limits{9, 11}, 10, context));
  EXPECT_FALSE(Validate(Limits{5, 3}, 10, context));
}

TEST(ValidateTest, Memory) {
  const Memory tests[] = {
      Memory{MemoryType{Limits{0}}},
      Memory{MemoryType{Limits{1, 10}}},
  };

  for (const auto& memory : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(memory, context));
  }
}

TEST(ValidateTest, Memory_TooManyMemories) {
  TestErrors errors;
  Context context{errors};
  context.memories.push_back(MemoryType{Limits{0}});
  EXPECT_FALSE(Validate(Memory{MemoryType{Limits{0}}}, context));
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
    EXPECT_TRUE(Validate(memory_type, context));
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
    EXPECT_FALSE(Validate(memory_type, context));
  }
}

TEST(ValidateTest, MemoryType_Shared_MVP) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(MemoryType{Limits{0, 100, Shared::Yes}}, context));
}

TEST(ValidateTest, MemoryType_Shared_Threads) {
  Features features;
  features.enable_threads();
  TestErrors errors;
  Context context{features, errors};
  EXPECT_TRUE(Validate(MemoryType{Limits{0, 100, Shared::Yes}}, context));
}

TEST(ValidateTest, Start) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  EXPECT_TRUE(Validate(Start{0}, context));
}

TEST(ValidateTest, Start_FunctionIndexOOB) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(Start{0}, context));
}

TEST(ValidateTest, Start_InvalidParamCount) {
  FunctionType function_type{{ValueType::I32}, {}};
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{function_type});
  context.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(Start{0}, context));
}

TEST(ValidateTest, Start_InvalidResultCount) {
  TestErrors errors;
  Context context{errors};
  const FunctionType function_type{{}, {ValueType::I32}};
  context.types.push_back(TypeEntry{function_type});
  context.functions.push_back(Function{0});
  EXPECT_FALSE(Validate(Start{0}, context));
}

TEST(ValidateTest, Table) {
  const Table tests[] = {
      Table{TableType{Limits{0}, ReferenceType::Funcref}},
      Table{TableType{Limits{1, 10}, ReferenceType::Funcref}},
  };

  for (const auto& table : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(table, context));
  }
}

TEST(ValidateTest, Table_TooManyTables) {
  TestErrors errors;
  Context context{errors};
  const TableType table_type{Limits{0}, ReferenceType::Funcref};
  context.tables.push_back(table_type);
  EXPECT_FALSE(Validate(Table{table_type}, context));
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
    EXPECT_TRUE(Validate(table_type, context));
  }
}

TEST(ValidateTest, TableType_Shared) {
  TestErrors errors;
  Context context{errors};
  EXPECT_FALSE(Validate(
      TableType{Limits{0, 100, Shared::Yes}, ReferenceType::Funcref}, context));
}

TEST(ValidateTest, TypeEntry) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(TypeEntry{FunctionType{}}, context));
}

TEST(ValidateTest, ValueType) {
  const ValueType tests[] = {
      ValueType::I32, ValueType::I64,  ValueType::F32,
      ValueType::F64, ValueType::V128, ValueType::Anyref,
  };

  for (auto value_type : tests) {
    TestErrors errors;
    Context context{errors};
    EXPECT_TRUE(Validate(value_type, value_type, context));
  }
}

TEST(ValidateTest, ValueType_Mismatch) {
  const ValueType tests[] = {
      ValueType::I32, ValueType::I64,  ValueType::F32,
      ValueType::F64, ValueType::V128, ValueType::Anyref,
  };

  for (auto value_type1 : tests) {
    for (auto value_type2 : tests) {
      if (value_type1 == value_type2) {
        continue;
      }
      TestErrors errors;
      Context context{errors};
      EXPECT_FALSE(Validate(value_type1, value_type2, context));
    }
  }
}

TEST(ValidateTest, EndModule) {
  TestErrors errors;
  Context context{errors};
  context.deferred_function_references.push_back(0);
  context.declared_functions.insert(0);
  EXPECT_TRUE(EndModule(context));
}

TEST(ValidateTest, EndModule_UndeclaredFunctionReference) {
  TestErrors errors;
  Context context{errors};
  context.deferred_function_references.push_back(0);
  EXPECT_FALSE(EndModule(context));
}
