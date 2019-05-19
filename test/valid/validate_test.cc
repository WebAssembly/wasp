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
#include "wasp/valid/validate_constant_expression.h"
#include "wasp/valid/validate_data_count.h"
#include "wasp/valid/validate_data_segment.h"
#include "wasp/valid/validate_element_expression.h"
#include "wasp/valid/validate_element_segment.h"
#include "wasp/valid/validate_element_type.h"
#include "wasp/valid/validate_export.h"
#include "wasp/valid/validate_function.h"
#include "wasp/valid/validate_function_type.h"
#include "wasp/valid/validate_global.h"
#include "wasp/valid/validate_global_type.h"
#include "wasp/valid/validate_import.h"
#include "wasp/valid/validate_index.h"
#include "wasp/valid/validate_limits.h"
#include "wasp/valid/validate_memory.h"
#include "wasp/valid/validate_memory_type.h"
#include "wasp/valid/validate_start.h"
#include "wasp/valid/validate_table.h"
#include "wasp/valid/validate_table_type.h"
#include "wasp/valid/validate_type_entry.h"
#include "wasp/valid/validate_value_type.h"

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
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(ConstantExpression{test.instr}, test.valtype, 0,
                         context, Features{}, errors));
  }
}

TEST(ValidateTest, ConstantExpression_Global) {
  Context context;
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::I64, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F64, Mutability::Const});
  auto max = context.globals.size();

  TestErrors errors;
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
               ValueType::I32, max, context, Features{}, errors));
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
               ValueType::I64, max, context, Features{}, errors));
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}},
               ValueType::F32, max, context, Features{}, errors));
  EXPECT_TRUE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}},
               ValueType::F64, max, context, Features{}, errors));
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
    Context context;
    TestErrors errors;
    EXPECT_FALSE(Validate(ConstantExpression{instr}, ValueType::I32, 0, context,
                          Features{}, errors));
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
    Context context;
    TestErrors errors;
    EXPECT_FALSE(Validate(ConstantExpression{test.instr}, test.valtype, 0,
                          context, Features{}, errors));
  }
}

TEST(ValidateTest, ConstantExpression_GlobalIndexOOB) {
  Context context;
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  auto max = context.globals.size();

  TestErrors errors;
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
               ValueType::I32, max, context, Features{}, errors));
}

TEST(ValidateTest, ConstantExpression_GlobalTypeMismatch) {
  Context context;
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::I64, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});
  context.globals.push_back(GlobalType{ValueType::F64, Mutability::Const});
  auto max = context.globals.size();

  TestErrors errors;
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
               ValueType::I64, max, context, Features{}, errors));
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{1}}},
               ValueType::F32, max, context, Features{}, errors));
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{2}}},
               ValueType::F64, max, context, Features{}, errors));
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{3}}},
               ValueType::I32, max, context, Features{}, errors));
}

TEST(ValidateTest, ConstantExpression_GlobalMutVar) {
  Context context;
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  auto max = context.globals.size();

  TestErrors errors;
  EXPECT_FALSE(
      Validate(ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
               ValueType::I32, max, context, Features{}, errors));
}

TEST(ValidateTest, DataCount) {
  Context context;
  TestErrors errors;
  EXPECT_TRUE(Validate(DataCount{1}, context, Features{}, errors));
  EXPECT_EQ(1, context.data_segment_count);
}

TEST(ValidateTest, DataSegment_Active) {
  Context context;
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
    TestErrors errors;
    EXPECT_TRUE(Validate(data_segment, context, Features{}, errors));
  }
}

TEST(ValidateTest, DataSegment_Active_MemoryIndexOOB) {
  Context context;
  TestErrors errors;
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, span};
  EXPECT_FALSE(Validate(data_segment, context, Features{}, errors));
}

TEST(ValidateTest, DataSegment_Active_GlobalIndexOOB) {
  Context context;
  context.memories.push_back(MemoryType{Limits{0}});
  TestErrors errors;
  const SpanU8 span{reinterpret_cast<const u8*>("123"), 3};
  const DataSegment data_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, span};
  EXPECT_FALSE(Validate(data_segment, context, Features{}, errors));
}

TEST(ValidateTest, ElementExpression) {
  Context context;
  context.functions.push_back(Function{0});

  const Instruction tests[] = {
      Instruction{Opcode::RefNull},
      Instruction{Opcode::RefFunc, Index{0}},
  };

  for (const auto& instr : tests) {
    TestErrors errors;
    EXPECT_TRUE(Validate(ElementExpression{instr}, ElementType::Funcref,
                         context, Features{}, errors));
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
    Context context;
    TestErrors errors;
    EXPECT_FALSE(Validate(ElementExpression{instr}, ElementType::Funcref,
                          context, Features{}, errors));
  }
}

TEST(ValidateTest, ElementExpression_FunctionIndexOOB) {
  Context context;
  context.functions.push_back(Function{0});
  TestErrors errors;
  EXPECT_FALSE(
      Validate(ElementExpression{Instruction{Opcode::RefFunc, Index{1}}},
               ElementType::Funcref, context, Features{}, errors));
}

TEST(ValidateTest, ElementSegment_Active) {
  Context context;
  context.functions.push_back(Function{0});
  context.functions.push_back(Function{0});
  context.tables.push_back(TableType{Limits{0}, ElementType::Funcref});
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});

  const ElementSegment tests[] = {
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, {0, 1}},
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, {}},
  };

  for (const auto& element_segment : tests) {
    TestErrors errors;
    EXPECT_TRUE(Validate(element_segment, context, Features{}, errors));
  }
}

TEST(ValidateTest, ElementSegment_Passive) {
  Context context;
  context.functions.push_back(Function{0});

  const ElementSegment tests[] = {
      ElementSegment{ElementType::Funcref, {}},
      ElementSegment{
          ElementType::Funcref,
          {ElementExpression{Instruction{Opcode::RefNull}},
           ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}},
  };

  for (const auto& element_segment : tests) {
    TestErrors errors;
    EXPECT_TRUE(Validate(element_segment, context, Features{}, errors));
  }
}

TEST(ValidateTest, ElementSegment_Active_TypeMismatch) {
  Context context;
  context.functions.push_back(Function{0});
  context.tables.push_back(TableType{Limits{0}, ElementType::Funcref});
  context.globals.push_back(GlobalType{ValueType::F32, Mutability::Const});

  const ElementSegment tests[] = {
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::F32Const, f32{0}}}, {}},
      ElementSegment{
          0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, {}},
  };

  for (const auto& element_segment : tests) {
    TestErrors errors;
    EXPECT_FALSE(Validate(element_segment, context, Features{}, errors));
  }
}

TEST(ValidateTest, ElementSegment_Active_TableIndexOOB) {
  Context context;
  context.functions.push_back(Function{0});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, {}};
  TestErrors errors;
  EXPECT_FALSE(Validate(element_segment, context, Features{}, errors));
}

TEST(ValidateTest, ElementSegment_Active_GlobalIndexOOB) {
  Context context;
  context.tables.push_back(TableType{Limits{0}, ElementType::Funcref});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}, {}};
  TestErrors errors;
  EXPECT_FALSE(Validate(element_segment, context, Features{}, errors));
}

TEST(ValidateTest, ElementSegment_Active_FunctionIndexOOB) {
  Context context;
  context.tables.push_back(TableType{Limits{0}, ElementType::Funcref});
  const ElementSegment element_segment{
      0, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}, {0}};
  TestErrors errors;
  EXPECT_FALSE(Validate(element_segment, context, Features{}, errors));
}

TEST(ValidateTest, ElementSegment_Passive_FunctionIndexOOB) {
  Context context;
  TestErrors errors;
  const ElementSegment element_segment{
      ElementType::Funcref,
      {ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}}};
  EXPECT_FALSE(Validate(element_segment, context, Features{}, errors));
}

TEST(ValidateTest, ElementType) {
  Context context;
  TestErrors errors;
  EXPECT_TRUE(Validate(ElementType::Funcref, ElementType::Funcref, context,
                       Features{}, errors));
}

TEST(ValidateTest, Export) {
  Context context;
  context.functions.push_back(Function{0});
  context.tables.push_back(TableType{Limits{1}, ElementType::Funcref});
  context.memories.push_back(MemoryType{Limits{1}});
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Const});

  const Export tests[] = {
      Export{ExternalKind::Function, "", 0},
      Export{ExternalKind::Table, "", 0},
      Export{ExternalKind::Memory, "", 0},
      Export{ExternalKind::Global, "", 0},
  };

  for (const auto& export_ : tests) {
    TestErrors errors;
    EXPECT_TRUE(Validate(export_, context, Features{}, errors));
  }
}

TEST(ValidateTest, Export_IndexOOB) {
  const Export tests[] = {
      Export{ExternalKind::Function, "", 0},
      Export{ExternalKind::Table, "", 0},
      Export{ExternalKind::Memory, "", 0},
      Export{ExternalKind::Global, "", 0},
  };

  for (const auto& export_ : tests) {
    Context context;
    TestErrors errors;
    EXPECT_FALSE(Validate(export_, context, Features{}, errors));
  }
}

TEST(ValidateTest, Export_GlobalMutVar_MVP) {
  Features features;
  features.disable_mutable_globals();
  Context context;
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  TestErrors errors;
  EXPECT_FALSE(
      Validate(Export{ExternalKind::Global, "", 0}, context, features, errors));
}

TEST(ValidateTest, Export_GlobalMutVar_MutableGlobals) {
  Features features;
  Context context;
  context.globals.push_back(GlobalType{ValueType::I32, Mutability::Var});
  TestErrors errors;
  EXPECT_TRUE(
      Validate(Export{ExternalKind::Global, "", 0}, context, features, errors));
}

TEST(ValidateTest, Function) {
  Context context;
  context.types.push_back(TypeEntry{FunctionType{}});
  TestErrors errors;
  EXPECT_TRUE(Validate(Function{0}, context, Features{}, errors));
}

TEST(ValidateTest, Function_IndexOOB) {
  Context context;
  TestErrors errors;
  EXPECT_FALSE(Validate(Function{0}, context, Features{}, errors));
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
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(function_type, context, Features{}, errors));
  }
}

TEST(ValidateTest, FunctionType_MultiReturn_MVP) {
  const FunctionType tests[] = {
      FunctionType{{}, {ValueType::I32, ValueType::I32}},
      FunctionType{{}, {ValueType::I32, ValueType::I64, ValueType::F32}},
  };

  for (const auto& function_type : tests) {
    Context context;
    TestErrors errors;
    EXPECT_FALSE(Validate(function_type, context, Features{}, errors));
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
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(function_type, context, features, errors));
  }
}

TEST(ValidateTest, Global) {
  Context context;
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
    TestErrors errors;
    EXPECT_TRUE(Validate(global, context, Features{}, errors));
  }
}

TEST(ValidateTest, Global_TypeMismatch) {
  Context context;
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
    TestErrors errors;
    EXPECT_FALSE(Validate(global, context, Features{}, errors));
  }
}

TEST(ValidateTest, Global_GlobalGetIndexOOB) {
  Context context;
  TestErrors errors;
  const Global global{
      GlobalType{ValueType::I32, Mutability::Const},
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}}};
  EXPECT_FALSE(Validate(global, context, Features{}, errors));
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
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(global_type, context, Features{}, errors));
  }
}

TEST(ValidateTest, Import) {
  Context context;
  context.types.push_back(TypeEntry{FunctionType{}});

  const Import tests[] = {
      Import{"", "", Index{0}},
      Import{"", "", TableType{Limits{0}, ElementType::Funcref}},
      Import{"", "", MemoryType{Limits{0}}},
      Import{"", "", GlobalType{ValueType::I32, Mutability::Const}},
  };

  for (const auto& import : tests) {
    TestErrors errors;
    EXPECT_TRUE(Validate(import, context, Features{}, errors));
  }
}

TEST(ValidateTest, Import_FunctionIndexOOB) {
  Context context;
  TestErrors errors;
  EXPECT_FALSE(Validate(Import{"", "", Index{0}}, context, Features{}, errors));
}

TEST(ValidateTest, Import_TooManyTables) {
  const TableType table_type{Limits{0}, ElementType::Funcref};
  Context context;
  context.tables.push_back(table_type);

  TestErrors errors;
  EXPECT_FALSE(
      Validate(Import{"", "", table_type}, context, Features{}, errors));
}

TEST(ValidateTest, Import_TooManyMemories) {
  const MemoryType memory_type{Limits{0}};
  Context context;
  context.memories.push_back(memory_type);

  TestErrors errors;
  EXPECT_FALSE(
      Validate(Import{"", "", memory_type}, context, Features{}, errors));
}

TEST(ValidateTest, Import_GlobalMutVar_MVP) {
  Features features;
  features.disable_mutable_globals();
  Context context;
  TestErrors errors;
  EXPECT_FALSE(
      Validate(Import{"", "", GlobalType{ValueType::I32, Mutability::Var}},
               context, features, errors));
}

TEST(ValidateTest, Import_GlobalMutVar_MutableGlobals) {
  Context context;
  TestErrors errors;
  EXPECT_TRUE(
      Validate(Import{"", "", GlobalType{ValueType::I32, Mutability::Var}},
               context, Features{}, errors));
}

TEST(ValidateTest, Index) {
  TestErrors errors;
  EXPECT_TRUE(ValidateIndex(1, 3, "index", errors));
  EXPECT_FALSE(ValidateIndex(3, 3, "index", errors));
  EXPECT_FALSE(ValidateIndex(0, 0, "index", errors));
}

TEST(ValidateTest, Limits) {
  Context context;
  TestErrors errors;
  EXPECT_TRUE(Validate(Limits{0}, 10, context, Features{}, errors));
  EXPECT_TRUE(Validate(Limits{9, 10}, 10, context, Features{}, errors));
}

TEST(ValidateTest, Limits_Invalid) {
  Context context;
  TestErrors errors;
  EXPECT_FALSE(Validate(Limits{11}, 10, context, Features{}, errors));
  EXPECT_FALSE(Validate(Limits{9, 11}, 10, context, Features{}, errors));
  EXPECT_FALSE(Validate(Limits{5, 3}, 10, context, Features{}, errors));
}

TEST(ValidateTest, Memory) {
  const Memory tests[] = {
      Memory{MemoryType{Limits{0}}},
      Memory{MemoryType{Limits{1, 10}}},
  };

  for (const auto& memory : tests) {
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(memory, context, Features{}, errors));
  }
}

TEST(ValidateTest, Memory_TooManyMemories) {
  Context context;
  context.memories.push_back(MemoryType{Limits{0}});
  TestErrors errors;
  EXPECT_FALSE(
      Validate(Memory{MemoryType{Limits{0}}}, context, Features{}, errors));
}

TEST(ValidateTest, MemoryType) {
  const MemoryType tests[] = {
      MemoryType{Limits{0}},
      MemoryType{Limits{1000}},
      MemoryType{Limits{100, 12345}},
      MemoryType{Limits{0, 65535}},
  };

  for (const auto& memory_type : tests) {
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(memory_type, context, Features{}, errors));
  }
}

TEST(ValidateTest, MemoryType_TooLarge) {
  const MemoryType tests[] = {
      MemoryType{Limits{65536}},
      MemoryType{Limits{0, 65536}},
      MemoryType{Limits{0xffffffffu, 0xffffffffu}},
  };

  for (const auto& memory_type : tests) {
    Context context;
    TestErrors errors;
    EXPECT_FALSE(Validate(memory_type, context, Features{}, errors));
  }
}

TEST(ValidateTest, MemoryType_Shared_MVP) {
  Context context;
  TestErrors errors;
  EXPECT_FALSE(Validate(MemoryType{Limits{0, 100, Shared::Yes}}, context,
                        Features{}, errors));
}

TEST(ValidateTest, MemoryType_Shared_Threads) {
  Features features;
  features.enable_threads();
  Context context;
  TestErrors errors;
  EXPECT_TRUE(Validate(MemoryType{Limits{0, 100, Shared::Yes}}, context,
                       features, errors));
}

TEST(ValidateTest, Start) {
  Context context;
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  TestErrors errors;
  EXPECT_TRUE(Validate(Start{0}, context, Features{}, errors));
}

TEST(ValidateTest, Start_FunctionIndexOOB) {
  Context context;
  TestErrors errors;
  EXPECT_FALSE(Validate(Start{0}, context, Features{}, errors));
}

TEST(ValidateTest, Start_InvalidParamCount) {
  FunctionType function_type{{ValueType::I32}, {}};
  Context context;
  context.types.push_back(TypeEntry{function_type});
  context.functions.push_back(Function{0});
  TestErrors errors;
  EXPECT_FALSE(Validate(Start{0}, context, Features{}, errors));
}

TEST(ValidateTest, Start_InvalidResultCount) {
  FunctionType function_type{{}, {ValueType::I32}};
  Context context;
  context.types.push_back(TypeEntry{function_type});
  context.functions.push_back(Function{0});
  TestErrors errors;
  EXPECT_FALSE(Validate(Start{0}, context, Features{}, errors));
}

TEST(ValidateTest, Table) {
  const Table tests[] = {
      Table{TableType{Limits{0}, ElementType::Funcref}},
      Table{TableType{Limits{1, 10}, ElementType::Funcref}},
  };

  for (const auto& table : tests) {
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(table, context, Features{}, errors));
  }
}

TEST(ValidateTest, Table_TooManyTables) {
  TableType table_type{Limits{0}, ElementType::Funcref};
  Context context;
  context.tables.push_back(table_type);
  TestErrors errors;
  EXPECT_FALSE(Validate(Table{table_type}, context, Features{}, errors));
}

TEST(ValidateTest, TableType) {
  const TableType tests[] = {
      TableType{Limits{0}, ElementType::Funcref},
      TableType{Limits{1000}, ElementType::Funcref},
      TableType{Limits{100, 12345}, ElementType::Funcref},
      TableType{Limits{0, 0xffffffff}, ElementType::Funcref},
  };

  for (const auto& table_type : tests) {
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(table_type, context, Features{}, errors));
  }
}

TEST(ValidateTest, TableType_Shared) {
  Context context;
  TestErrors errors;
  EXPECT_FALSE(
      Validate(TableType{Limits{0, 100, Shared::Yes}, ElementType::Funcref},
               context, Features{}, errors));
}

TEST(ValidateTest, TypeEntry) {
  Context context;
  TestErrors errors;
  EXPECT_TRUE(Validate(TypeEntry{FunctionType{}}, context, Features{}, errors));
}

TEST(ValidateTest, ValueType) {
  const ValueType tests[] = {
      ValueType::I32, ValueType::I64,  ValueType::F32,
      ValueType::F64, ValueType::V128, ValueType::Anyref,
  };

  for (auto value_type : tests) {
    Context context;
    TestErrors errors;
    EXPECT_TRUE(Validate(value_type, value_type, context, Features{}, errors));
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
      Context context;
      TestErrors errors;
      EXPECT_FALSE(
          Validate(value_type1, value_type2, context, Features{}, errors));
    }
  }
}
