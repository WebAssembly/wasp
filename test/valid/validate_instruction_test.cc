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

#include "wasp/base/features.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/begin_code.h"
#include "wasp/valid/context.h"
#include "wasp/valid/test_utils.h"
#include "wasp/valid/validate_instruction.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;

class ValidateInstructionTest : public ::testing::Test {
 protected:
  using I = Instruction;
  using O = Opcode;

  virtual void SetUp() {
    BeginFunction(FunctionType{});
  }

  virtual void TearDown() {}

  void BeginFunction(const FunctionType& function_type) {
    context = Context{};
    AddFunction(function_type);
    EXPECT_TRUE(BeginCode(context, features, errors));
  }

  template <typename T>
  Index AddItem(std::vector<T>& vec, const T& type) {
    vec.push_back(type);
    return vec.size() - 1;
  }

  Index AddFunctionType(const FunctionType& function_type) {
    return AddItem(context.types, TypeEntry{function_type});
  }

  Index AddFunction(const FunctionType& function_type) {
    Index type_index = AddFunctionType(function_type);
    return AddItem(context.functions, Function{type_index});
  }

  Index AddTable(const TableType& table_type) {
    return AddItem(context.tables, table_type);
  }

  Index AddMemory(const MemoryType& memory_type) {
    return AddItem(context.memories, memory_type);
  }

  Index AddGlobal(const GlobalType& global_type) {
    return AddItem(context.globals, global_type);
  }

  Index AddLocal(const ValueType& value_type) {
    return AddItem(context.locals, value_type);
  }

  void Step(const Instruction& instruction) {
    EXPECT_TRUE(Validate(instruction, context, features, errors))
        << format("{}", instruction);
  }

  void Fail(const Instruction& instruction) {
    EXPECT_FALSE(Validate(instruction, context, features, errors))
        << format("{}", instruction);
  }

  Context context;
  Features features;
  TestErrors errors;
};

struct ValueTypeInfo {
  ValueType value_type;
  BlockType block_type;
  Instruction instruction;
};
const ValueTypeInfo all_value_types[] = {
    {ValueType::I32, BlockType::I32, Instruction{Opcode::I32Const, s32{}}},
    {ValueType::I64, BlockType::I64, Instruction{Opcode::I64Const, s64{}}},
    {ValueType::F32, BlockType::F32, Instruction{Opcode::F32Const, f32{}}},
    {ValueType::F64, BlockType::F64, Instruction{Opcode::F64Const, f64{}}},
#if 0
    {ValueType::V128, BlockType::V128, Instruction{Opcode::V128Const, v128{}}},
    {ValueType::Anyref, BlockType::Anyref, Instruction{Opcode::RefNull}},
#endif
};

TEST_F(ValidateInstructionTest, Unreachable) {
  Step(I{O::Unreachable});
}

TEST_F(ValidateInstructionTest, Nop) {
  Step(I{O::Nop});
}

TEST_F(ValidateInstructionTest, Block_Void) {
  Step(I{O::Block, BlockType::Void});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::Block, info.block_type});
    Step(info.instruction);
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, Loop_Void) {
  Step(I{O::Loop, BlockType::Void});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::Loop, info.block_type});
    Step(info.instruction);
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, If_End_Void) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::Void});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_Void) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::Void});
  Step(I{O::Else});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::I32Const, s32{}});
    Step(I{O::If, info.block_type});
    Step(info.instruction);
    Step(I{O::Else});
    Step(info.instruction);
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, If_End_Void_Unreachable) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::Void});
  Step(I{O::Unreachable});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_Void_Unreachable) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::Void});
  Step(I{O::Unreachable});
  Step(I{O::Else});
  Step(I{O::End});

  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::Void});
  Step(I{O::Else});
  Step(I{O::Unreachable});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_SingleResult_Unreachable) {
  for (const auto& info : all_value_types) {
    Step(I{O::I32Const, s32{}});
    Step(I{O::If, info.block_type});
    Step(I{O::Unreachable});
    Step(I{O::Else});
    Step(info.instruction);
    Step(I{O::End});
  }

  for (const auto& info : all_value_types) {
    Step(I{O::I32Const, s32{}});
    Step(I{O::If, info.block_type});
    Step(info.instruction);
    Step(I{O::Else});
    Step(I{O::Unreachable});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, If_EmptyStack) {
  Fail(I{O::If, BlockType::Void});
}

TEST_F(ValidateInstructionTest, If_CondTypeMismatch) {
  Step(I{O::F32Const, f32{}});
  Fail(I{O::If, BlockType::Void});
}

TEST_F(ValidateInstructionTest, If_End_I32) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, If_End_I32_Unreachable) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::I32});
  Step(I{O::Unreachable});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_TypeMismatch) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Step(I{O::Else});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::End});

  Step(I{O::I32Const, s32{}});
  Step(I{O::If, BlockType::I32});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::Else});
  Step(I{O::I32Const, s32{}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Else_NoIf) {
  Fail(I{O::Else});

  Step(I{O::Block, BlockType::Void});
  Fail(I{O::Else});
}

TEST_F(ValidateInstructionTest, End) {
  Step(I{O::Block, BlockType::Void});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, End_Unreachable) {
  Step(I{O::Block, BlockType::Void});
  Step(I{O::Unreachable});
  Step(I{O::End});

  Step(I{O::Block, BlockType::I32});
  Step(I{O::Unreachable});
  Step(I{O::End});

  Step(I{O::Block, BlockType::I32});
  Step(I{O::Unreachable});
  Step(I{O::I32Const, s32{}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, End_Unreachable_TypeMismatch) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::Unreachable});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_EmptyLabelStack) {
  Step(I{O::End});  // This `end` ends the function.
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_EmptyTypeStack) {
  Step(I{O::Block, BlockType::I32});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_TypeMismatch) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_TooManyValues) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_Unreachable_TooManyValues) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::Unreachable});
  Step(I{O::I32Const, s32{}});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, Br_Void) {
  Step(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::Block, info.block_type});
    Step(info.instruction);
    Step(I{O::Br, Index{0}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, Br_EmptyStack) {
  Step(I{O::Block, BlockType::I32});
  Fail(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_FullerStack) {
  Step(I{O::Block, BlockType::Void});
  Step(I{O::I32Const, s32{}});
  Step(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_TypeMismatch) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_Depth1) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::Block, BlockType::Void});
  Step(I{O::I32Const, s32{}});
  Step(I{O::Br, Index{1}});
}

TEST_F(ValidateInstructionTest, Br_ForwardUnreachable) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::Block, BlockType::F32});
  Step(I{O::I32Const, s32{}});
  Step(I{O::Br, Index{1}});
  Step(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_Loop_Void) {
  Step(I{O::Loop, BlockType::Void});
  Step(I{O::Br, Index{0}});
  Step(I{O::End});

  Step(I{O::Loop, BlockType::I32});
  Step(I{O::Br, Index{0}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Br_Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::Loop, info.block_type});
    Step(I{O::Br, Index{0}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, BrIf_Void) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::Block, info.block_type});
    Step(info.instruction);
    Step(I{O::I32Const, s32{}});
    Step(I{O::BrIf, Index{0}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, BrIf_NoCondition) {
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_ConditionMismatch) {
  Step(I{O::F32Const, f32{}});
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_EmptyStack) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_TypeMismatch) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::F32Const, f32{}});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_PropagateValue) {
  Step(I{O::Block, BlockType::F32});
  Step(I{O::Block, BlockType::I32});
  Step(I{O::F32Const, f32{}});
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrIf, Index{1}});
  Fail(I{O::End});  // F32 is still on the stack.
}

TEST_F(ValidateInstructionTest, BrIf_Loop_Void) {
  Step(I{O::Loop, BlockType::Void});
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrIf, Index{0}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, BrIf_Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Step(I{O::Loop, info.block_type});
    Step(info.instruction);
    Step(I{O::I32Const, s32{}});
    Step(I{O::BrIf, Index{0}});
    Step(I{O::Unreachable});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, BrTable_Void) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrTable, BrTableImmediate{{0, 0, 0}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_MultiDepth_Void) {
  Step(I{O::Block, BlockType::Void});  // 3
  Step(I{O::Block, BlockType::Void});  // 2
  Step(I{O::Block, BlockType::Void});  // 1
  Step(I{O::Block, BlockType::Void});  // 0
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrTable, BrTableImmediate{{0, 1, 2, 3}, 4}});
}

TEST_F(ValidateInstructionTest, BrTable_MultiDepth_SingleResult) {
  Step(I{O::Block, BlockType::I32});   // 3
  Step(I{O::Block, BlockType::Void});  // 2
  Step(I{O::Block, BlockType::I32});   // 1
  Step(I{O::Block, BlockType::Void});  // 0
  Step(I{O::I32Const, s32{}});
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrTable, BrTableImmediate{{1, 1, 1, 3}, 3}});
}

TEST_F(ValidateInstructionTest, BrTable_Unreachable) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Step(I{O::BrTable, BrTableImmediate{{}, 1}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, BrTable_NoKey) {
  Fail(I{O::BrTable, BrTableImmediate{{}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_EmptyStack) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_ValueTypeMismatch) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::F32Const, f32{}});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{0}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_InconsistentLabelSignature) {
  Step(I{O::Block, BlockType::Void});
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{1}, 0}});
}

TEST_F(ValidateInstructionTest, Return) {
  Step(I{O::Return});
}

TEST_F(ValidateInstructionTest, Return_InsideBlocks) {
  Step(I{O::Block, BlockType::Void});
  Step(I{O::Block, BlockType::Void});
  Step(I{O::Block, BlockType::Void});
  Step(I{O::Return});
}

TEST_F(ValidateInstructionTest, Return_Unreachable) {
  Step(I{O::Block, BlockType::F64});
  Step(I{O::Return});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Return_SingleResult) {
  BeginFunction(FunctionType{{}, {ValueType::I32}});
  Step(I{O::I32Const, s32{}});
  Step(I{O::Return});
}

TEST_F(ValidateInstructionTest, Return_TypeMismatch) {
  BeginFunction(FunctionType{{}, {ValueType::I32}});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::Return});
}

TEST_F(ValidateInstructionTest, Call_Void_Void) {
  auto index = AddFunction(FunctionType{});
  Step(I{O::Call, Index{index}});
}

TEST_F(ValidateInstructionTest, Call_Params) {
  auto index = AddFunction(FunctionType{{ValueType::I32, ValueType::F32}, {}});
  Step(I{O::I32Const, s32{}});
  Step(I{O::F32Const, f32{}});
  Step(I{O::Call, Index{index}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Call_SingleResult) {
  auto index = AddFunction(FunctionType{{}, {ValueType::F64}});
  Step(I{O::Block, BlockType::F64});
  Step(I{O::Call, Index{index}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Call_EmptyStack) {
  auto index = AddFunction(FunctionType{{ValueType::I32}, {}});
  Fail(I{O::Call, Index{index}});
}

TEST_F(ValidateInstructionTest, Call_TypeMismatch) {
  auto index = AddFunction(FunctionType{{ValueType::I32}, {}});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::Call, Index{index}});
}

TEST_F(ValidateInstructionTest, Call_FunctionIndexOOB) {
  Fail(I{O::Call, Index{100}});
}

TEST_F(ValidateInstructionTest, Call_TypeIndexOOB) {
  context.functions.push_back(Function{100});
  Index index = context.functions.size() - 1;
  Fail(I{O::Call, Index{index}});
}

TEST_F(ValidateInstructionTest, CallIndirect) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  auto index = AddFunctionType(FunctionType{});
  Step(I{O::I32Const, s32{}});
  Step(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, CallIndirect_Params) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  auto index =
      AddFunctionType(FunctionType{{ValueType::F32, ValueType::I64}, {}});
  Step(I{O::F32Const, f32{}});
  Step(I{O::I64Const, s64{}});
  Step(I{O::I32Const, s32{}});
  Step(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, CallIndirect_SingleResult) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  auto index = AddFunctionType(FunctionType{{}, {ValueType::F64}});
  Step(I{O::Block, BlockType::F64});
  Step(I{O::I32Const, s32{}});
  Step(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, CallIndirect_TableIndexOOB) {
  auto index = AddFunctionType(FunctionType{});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, CallIndirect_EmptyStack) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  auto index = AddFunctionType(FunctionType{});
  Fail(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, CallIndirect_KeyTypeMismatch) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  auto index = AddFunctionType(FunctionType{});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, CallIndirect_TypeIndexOOB) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{100, 0}});
}

TEST_F(ValidateInstructionTest, Drop) {
  for (const auto& info : all_value_types) {
    Step(info.instruction);
    Step(I{O::Drop});
  }
}

TEST_F(ValidateInstructionTest, Drop_EmptyStack) {
  Fail(I{O::Drop});
}

TEST_F(ValidateInstructionTest, Select) {
  for (const auto& info : all_value_types) {
    Step(info.instruction);
    Step(info.instruction);
    Step(I{O::I32Const, s32{}});
    Step(I{O::Select});
  }
}

TEST_F(ValidateInstructionTest, Select_EmptyStack) {
  Fail(I{O::Select});
}

TEST_F(ValidateInstructionTest, Select_ConditionTypeMismatch) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::I32Const, s32{}});
  Step(I{O::F32Const, f32{}});
  Fail(I{O::Select});
}

TEST_F(ValidateInstructionTest, Select_InconsistentTypes) {
  Step(I{O::I32Const, s32{}});
  Step(I{O::F32Const, f32{}});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::Select});
}

TEST_F(ValidateInstructionTest, LocalGet) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    Step(I{O::Block, info.block_type});
    Step(I{O::LocalGet, Index{index}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, LocalGet_Param) {
  BeginFunction(FunctionType{{ValueType::I32, ValueType::F32}, {}});
  auto index = AddLocal(ValueType::I64);
  EXPECT_EQ(2, index);
  Step(I{O::LocalGet, Index{0}}); // 1st param.
  Step(I{O::LocalGet, Index{1}}); // 2nd param.
  Step(I{O::LocalGet, Index{2}}); // 1st local.
  Fail(I{O::LocalGet, Index{3}}); // Invalid.
}

TEST_F(ValidateInstructionTest, LocalGet_IndexOOB) {
  Fail(I{O::LocalGet, Index{100}});
}

TEST_F(ValidateInstructionTest, LocalSet) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    Step(I{O::Block, BlockType::Void});
    Step(info.instruction);
    Step(I{O::LocalSet, Index{index}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, LocalSet_Param) {
  BeginFunction(FunctionType{{ValueType::I32, ValueType::F32}, {}});
  auto index = AddLocal(ValueType::F64);
  EXPECT_EQ(2, index);
  Step(I{O::I32Const, s32{}});
  Step(I{O::LocalSet, Index{0}}); // 1st param.
  Step(I{O::F32Const, f32{}});
  Step(I{O::LocalSet, Index{1}}); // 2nd param.
  Step(I{O::F64Const, f64{}});
  Step(I{O::LocalSet, Index{2}}); // 1st local.
  Step(I{O::I32Const, s32{}});
  Fail(I{O::LocalSet, Index{3}}); // Invalid.
}

TEST_F(ValidateInstructionTest, LocalSet_Unreachable) {
  auto index = AddLocal(ValueType::I32);
  Step(I{O::Unreachable});
  Step(I{O::LocalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, LocalSet_IndexOOB) {
  Fail(I{O::LocalSet, Index{100}});
}

TEST_F(ValidateInstructionTest, LocalSet_EmptyStack) {
  auto index = AddLocal(ValueType::I32);
  Fail(I{O::LocalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, LocalSet_TypeMismatch) {
  auto index = AddLocal(ValueType::F32);
  Step(I{O::I32Const, s32{}});
  Fail(I{O::LocalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, LocalTee) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    Step(I{O::Block, info.block_type});
    Step(info.instruction);
    Step(I{O::LocalTee, Index{index}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, LocalTee_Unreachable) {
  auto index = AddLocal(ValueType::I32);
  Step(I{O::Unreachable});
  Step(I{O::LocalTee, Index{index}});
}

TEST_F(ValidateInstructionTest, LocalTee_IndexOOB) {
  Fail(I{O::LocalTee, Index{100}});
}

TEST_F(ValidateInstructionTest, LocalTee_EmptyStack) {
  auto index = AddLocal(ValueType::I32);
  Fail(I{O::LocalTee, Index{index}});
}

TEST_F(ValidateInstructionTest, LocalTee_TypeMismatch) {
  auto index = AddLocal(ValueType::F32);
  Step(I{O::I32Const, s32{}});
  Fail(I{O::LocalTee, Index{index}});
}

TEST_F(ValidateInstructionTest, GlobalGet) {
  for (auto mut : {Mutability::Var, Mutability::Const}) {
    for (const auto& info : all_value_types) {
      auto index = AddGlobal(GlobalType{info.value_type, mut});
      Step(I{O::Block, info.block_type});
      Step(I{O::GlobalGet, Index{index}});
      Step(I{O::End});
    }
  }
}

TEST_F(ValidateInstructionTest, GlobalGet_IndexOOB) {
  Fail(I{O::GlobalGet, Index{100}});
}

TEST_F(ValidateInstructionTest, GlobalSet) {
  for (const auto& info : all_value_types) {
    auto index = AddGlobal(GlobalType{info.value_type, Mutability::Var});
    Step(I{O::Block, BlockType::Void});
    Step(info.instruction);
    Step(I{O::GlobalSet, Index{index}});
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, GlobalSet_Unreachable) {
  auto index = AddGlobal(GlobalType{ValueType::I32, Mutability::Var});
  Step(I{O::Unreachable});
  Step(I{O::GlobalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, GlobalSet_IndexOOB) {
  Fail(I{O::GlobalSet, Index{100}});
}

TEST_F(ValidateInstructionTest, GlobalSet_EmptyStack) {
  auto index = AddGlobal(GlobalType{ValueType::I32, Mutability::Var});
  Fail(I{O::GlobalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, GlobalSet_TypeMismatch) {
  auto index = AddGlobal(GlobalType{ValueType::F32, Mutability::Var});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::GlobalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, GlobalSet_Immutable) {
  auto index = AddGlobal(GlobalType{ValueType::I32, Mutability::Const});
  Step(I{O::I32Const, s32{}});
  Fail(I{O::GlobalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, I32Load) {
  const Opcode opcodes[] = {O::I32Load, O::I32Load8S, O::I32Load8U,
                            O::I32Load16S, O::I32Load16U};
  AddMemory(MemoryType{Limits{0}});

  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});               // -> i32
  for (auto opcode : opcodes) {              //
    Step(I{opcode, MemArgImmediate{0, 0}});  // i32 -> i32
  }                                          //
  Step(I{O::End});                           // i32 ->
}

TEST_F(ValidateInstructionTest, I64Load) {
  const Opcode opcodes[] = {O::I64Load,    O::I64Load8S,  O::I64Load8U,
                            O::I64Load16S, O::I64Load16U, O::I64Load32S,
                            O::I64Load32U};
  AddMemory(MemoryType{Limits{0}});

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I64});
    Step(I{O::I32Const, s32{}});             // -> i32
    Step(I{opcode, MemArgImmediate{0, 0}});  // i32 -> i64
    Step(I{O::End});                         // i32 ->
  }
}

TEST_F(ValidateInstructionTest, F32Load) {
  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Block, BlockType::F32});
  Step(I{O::I32Const, s32{}});                 // -> i32
  Step(I{O::F32Load, MemArgImmediate{0, 0}});  // i32 -> f32
  Step(I{O::End});                             // f32 ->
}

TEST_F(ValidateInstructionTest, F64Load) {
  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Block, BlockType::F64});
  Step(I{O::I32Const, s32{}});                 // -> i32
  Step(I{O::F64Load, MemArgImmediate{0, 0}});  // i32 -> f64
  Step(I{O::End});                             // f64 ->
}

TEST_F(ValidateInstructionTest, Load_Alignment) {
  struct {
    Opcode opcode;
    uint32_t max_align;
  } const infos[] = {{O::I32Load, 2},    {O::I64Load, 3},    {O::F32Load, 2},
                     {O::F64Load, 3},    {O::I32Load8S, 0},  {O::I32Load8U, 0},
                     {O::I32Load16S, 1}, {O::I32Load16U, 1}, {O::I64Load8S, 0},
                     {O::I64Load8U, 0},  {O::I64Load16S, 1}, {O::I64Load16U, 1},
                     {O::I64Load32S, 2}, {O::I64Load32U, 2}};

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info : infos) {
    Step(I{O::I32Const, s32{}});
    Step(I{info.opcode, MemArgImmediate{info.max_align, 0}});

    Step(I{O::I32Const, s32{}});
    Fail(I{info.opcode, MemArgImmediate{info.max_align + 1, 0}});
  }
}

TEST_F(ValidateInstructionTest, Load_MemoryOOB) {
  const Opcode opcodes[] = {
      O::I32Load,    O::I64Load,    O::F32Load,    O::F64Load,   O::I32Load8S,
      O::I32Load8U,  O::I32Load16S, O::I32Load16U, O::I64Load8S, O::I64Load8U,
      O::I64Load16S, O::I64Load16U, O::I64Load32S, O::I64Load32U};

  for (const auto& opcode : opcodes) {
    Step(I{O::I32Const, s32{}});
    Fail(I{opcode, MemArgImmediate{0, 0}});
  }
}

TEST_F(ValidateInstructionTest, I32Store) {
  const Opcode opcodes[] = {O::I32Store, O::I32Store8, O::I32Store16};
  AddMemory(MemoryType{Limits{0}});

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::Void});
    Step(I{O::I32Const, s32{}});             // -> i32
    Step(I{O::I32Const, s32{}});             // i32 -> i32 i32
    Step(I{opcode, MemArgImmediate{0, 0}});  // i32 i32 ->
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, I64Store) {
  const Opcode opcodes[] = {O::I64Store, O::I64Store8, O::I64Store16,
                            O::I64Store32};
  AddMemory(MemoryType{Limits{0}});

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::Void});
    Step(I{O::I32Const, s32{}});             // -> i32
    Step(I{O::I64Const, s64{}});             // i32 -> i32 i64
    Step(I{opcode, MemArgImmediate{0, 0}});  // i32 i64 ->
    Step(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, F32Store) {
  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Block, BlockType::Void});
  Step(I{O::I32Const, s32{}});                  // -> i32
  Step(I{O::F32Const, f32{}});                  // i32 -> i32 f32
  Step(I{O::F32Store, MemArgImmediate{0, 0}});  // i32 f32 ->
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, F64Store) {
  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Block, BlockType::Void});
  Step(I{O::I32Const, s32{}});                  // -> i32
  Step(I{O::F64Const, f64{}});                  // i32 -> i32 f64
  Step(I{O::F64Store, MemArgImmediate{0, 0}});  // i32 f64 ->
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, Store_MemoryOOB) {
  const Opcode opcodes[] = {O::I32Store,  O::I64Store,   O::F32Store,
                            O::F64Store,  O::I32Store8,  O::I32Store16,
                            O::I64Store8, O::I64Store16, O::I64Store32};

  Step(I{O::Unreachable});
  for (const auto& opcode : opcodes) {
    Fail(I{opcode, MemArgImmediate{0, 0}});
  }
}

TEST_F(ValidateInstructionTest, Store_Alignment) {
  struct {
    Opcode opcode;
    uint32_t max_align;
  } const infos[] = {{O::I32Store, 2},  {O::I64Store, 3},   {O::F32Store, 2},
                     {O::F64Store, 3},  {O::I32Store8, 0},  {O::I32Store16, 1},
                     {O::I64Store8, 0}, {O::I64Store16, 1}, {O::I64Store32, 2}};

  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Unreachable});
  for (const auto& info : infos) {
    Step(I{info.opcode, MemArgImmediate{info.max_align, 0}});
    Fail(I{info.opcode, MemArgImmediate{info.max_align + 1, 0}});
  }
}

TEST_F(ValidateInstructionTest, MemorySize) {
  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Block, BlockType::I32});
  Step(I{O::MemorySize, u8{}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, MemorySize_MemoryIndexOOB) {
  Fail(I{O::MemorySize, u8{}});
}

TEST_F(ValidateInstructionTest, MemoryGrow) {
  AddMemory(MemoryType{Limits{0}});
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});
  Step(I{O::MemoryGrow, u8{}});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, MemoryGrow_MemoryIndexOOB) {
  Step(I{O::I32Const, s32{}});
  Fail(I{O::MemoryGrow, u8{}});
}

TEST_F(ValidateInstructionTest, I32Unary) {
  const Opcode opcodes[] = {O::I32Eqz, O::I32Clz, O::I32Ctz, O::I32Popcnt};

  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});   // -> i32
  for (auto opcode : opcodes) {  //
    Step(I{opcode});             // i32 -> i32
  }                              //
  Step(I{O::End});               // i32 ->
}

TEST_F(ValidateInstructionTest, I32Binary) {
  const Opcode opcodes[] = {
      O::I32Eq,   O::I32Ne,   O::I32LtS,  O::I32LtU,  O::I32GtS,
      O::I32GtU,  O::I32LeS,  O::I32LeU,  O::I32GeS,  O::I32GeU,
      O::I32Add,  O::I32Sub,  O::I32Mul,  O::I32DivS, O::I32DivU,
      O::I32RemS, O::I32RemU, O::I32And,  O::I32Or,   O::I32Xor,
      O::I32Shl,  O::I32ShrS, O::I32ShrU, O::I32Rotl, O::I32Rotr};

  Step(I{O::Block, BlockType::I32});
  Step(I{O::I32Const, s32{}});    // -> i32
  for (auto opcode : opcodes) {   //
    Step(I{O::I32Const, s32{}});  // i32 -> i32 i32
    Step(I{opcode});              // i32 i32 -> i32
  }                               //
  Step(I{O::End});                // i32 ->
}

TEST_F(ValidateInstructionTest, I64Eqz) {
  Step(I{O::Block, BlockType::I32});
  Step(I{O::I64Const, s64{}});
  Step(I{O::I64Eqz});
  Step(I{O::End});
}

TEST_F(ValidateInstructionTest, I64Unary) {
  const Opcode opcodes[] = {O::I64Clz, O::I64Ctz, O::I64Popcnt};

  Step(I{O::Block, BlockType::I64});
  Step(I{O::I64Const, s64{}});   // -> i64
  for (auto opcode : opcodes) {  //
    Step(I{opcode});             // i64 -> i64
  }                              //
  Step(I{O::End});               // i64 ->
}

TEST_F(ValidateInstructionTest, I64Compare) {
  const Opcode opcodes[] = {O::I64LtS, O::I64LtU, O::I64GtS, O::I64GtU,
                            O::I64LeS, O::I64LeU, O::I64GeS, O::I64GeU};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I32});
    Step(I{O::I64Const, s64{}});  // -> i64
    Step(I{O::I64Const, s64{}});  // i64 -> i64 i64
    Step(I{opcode});              // i64 i64 -> i32
    Step(I{O::End});              //
    Step(I{O::Drop});             // i32 ->
  }
}

TEST_F(ValidateInstructionTest, I64Binary) {
  const Opcode opcodes[] = {O::I64Add,  O::I64Sub,  O::I64Mul,  O::I64DivS,
                            O::I64DivU, O::I64RemS, O::I64RemU, O::I64And,
                            O::I64Or,   O::I64Xor,  O::I64Shl,  O::I64ShrS,
                            O::I64ShrU, O::I64Rotl, O::I64Rotr};

  Step(I{O::Block, BlockType::I64});
  Step(I{O::I64Const, s64{}});    // -> i64
  for (auto opcode : opcodes) {   //
    Step(I{O::I64Const, s64{}});  // i64 -> i64 i64
    Step(I{opcode});              // i64 i64 -> i64
  }                               //
  Step(I{O::End});                // i64 ->
}

TEST_F(ValidateInstructionTest, F32Unary) {
  const Opcode opcodes[] = {O::F32Abs,   O::F32Neg,     O::F32Ceil, O::F32Floor,
                            O::F32Trunc, O::F32Nearest, O::F32Sqrt};

  Step(I{O::Block, BlockType::F32});
  Step(I{O::F32Const, s64{}});   // -> f32
  for (auto opcode : opcodes) {  //
    Step(I{opcode});             // f32 -> f32
  }                              //
  Step(I{O::End});               // f32 ->
}

TEST_F(ValidateInstructionTest, F32Compare) {
  const Opcode opcodes[] = {O::F32Eq, O::F32Ne, O::F32Lt,
                            O::F32Gt, O::F32Le, O::F32Ge};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I32});
    Step(I{O::F32Const, f32{}});  // -> f32
    Step(I{O::F32Const, f32{}});  // f32 -> f32 f32
    Step(I{opcode});              // f32 f32 -> i32
    Step(I{O::End});              //
    Step(I{O::Drop});             // i32 ->
  }
}

TEST_F(ValidateInstructionTest, F32Binary) {
  const Opcode opcodes[] = {O::F32Add, O::F32Sub, O::F32Mul,     O::F32Div,
                            O::F32Min, O::F32Max, O::F32Copysign};

  Step(I{O::Block, BlockType::F32});
  Step(I{O::F32Const, f32{}});    // -> f32
  for (auto opcode : opcodes) {   //
    Step(I{O::F32Const, f32{}});  // f32 -> f32 f32
    Step(I{opcode});              // f32 f32 -> f32
  }                               //
  Step(I{O::End});                // f32 ->
}

TEST_F(ValidateInstructionTest, F64Unary) {
  const Opcode opcodes[] = {O::F64Abs,   O::F64Neg,     O::F64Ceil, O::F64Floor,
                            O::F64Trunc, O::F64Nearest, O::F64Sqrt};

  Step(I{O::Block, BlockType::F64});
  Step(I{O::F64Const, s64{}});   // -> f64
  for (auto opcode : opcodes) {  //
    Step(I{opcode});             // f64 -> f64
  }                              //
  Step(I{O::End});               // f64 ->
}

TEST_F(ValidateInstructionTest, F64Compare) {
  const Opcode opcodes[] = {O::F64Eq, O::F64Ne, O::F64Lt,
                            O::F64Gt, O::F64Le, O::F64Ge};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I32});
    Step(I{O::F64Const, f64{}});  // -> f64
    Step(I{O::F64Const, f64{}});  // f64 -> f64 f64
    Step(I{opcode});              // f64 f64 -> i32
    Step(I{O::End});              //
    Step(I{O::Drop});             // i32 ->
  }
}

TEST_F(ValidateInstructionTest, F64Binary) {
  const Opcode opcodes[] = {O::F64Add, O::F64Sub, O::F64Mul,     O::F64Div,
                            O::F64Min, O::F64Max, O::F64Copysign};

  Step(I{O::Block, BlockType::F64});
  Step(I{O::F64Const, f64{}});    // -> f64
  for (auto opcode : opcodes) {   //
    Step(I{O::F64Const, f64{}});  // f64 -> f64 f64
    Step(I{opcode});              // f64 f64 -> f64
  }                               //
  Step(I{O::End});                // f64 ->
}

TEST_F(ValidateInstructionTest, I32_F32) {
  const Opcode opcodes[] = {O::I32TruncF32S, O::I32TruncF32U,
                            O::I32ReinterpretF32};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I32});
    Step(I{O::F32Const, f32{}});  // -> f32
    Step(I{opcode});              // f32 -> i32
    Step(I{O::End});              //
    Step(I{O::Drop});             // i32 ->
  }
}

TEST_F(ValidateInstructionTest, I32_F64) {
  const Opcode opcodes[] = {O::I32TruncF64S, O::I32TruncF64U};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I32});
    Step(I{O::F64Const, f64{}});  // -> f64
    Step(I{opcode});              // f32 -> i32
    Step(I{O::End});              //
    Step(I{O::Drop});             // i32 ->
  }
}

TEST_F(ValidateInstructionTest, I64_I32) {
  const Opcode opcodes[] = {O::I64ExtendI32S, O::I64ExtendI32U};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I64});
    Step(I{O::I32Const, s32{}});  // -> i32
    Step(I{opcode});              // i32 -> i64
    Step(I{O::End});              //
    Step(I{O::Drop});             // i64 ->
  }
}

TEST_F(ValidateInstructionTest, I64_F32) {
  const Opcode opcodes[] = {O::I64TruncF32S, O::I64TruncF32U};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I64});
    Step(I{O::F32Const, f32{}});  // -> f32
    Step(I{opcode});              // f32 -> i64
    Step(I{O::End});              //
    Step(I{O::Drop});             // i64 ->
  }
}

TEST_F(ValidateInstructionTest, I64_F64) {
  const Opcode opcodes[] = {O::I64TruncF64S, O::I64TruncF64U,
                            O::I64ReinterpretF64};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::I64});
    Step(I{O::F64Const, f64{}});  // -> f64
    Step(I{opcode});              // f64 -> i64
    Step(I{O::End});              //
    Step(I{O::Drop});             // i64 ->
  }
}

TEST_F(ValidateInstructionTest, F32_I32) {
  const Opcode opcodes[] = {O::F32ConvertI32S, O::F32ConvertI32U,
                            O::F32ReinterpretI32};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::F32});
    Step(I{O::I32Const, s32{}});  // -> i32
    Step(I{opcode});              // i32 -> f32
    Step(I{O::End});              //
    Step(I{O::Drop});             // f32 ->
  }
}

TEST_F(ValidateInstructionTest, F32_I64) {
  const Opcode opcodes[] = {O::F32ConvertI64S, O::F32ConvertI64U};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::F32});
    Step(I{O::I64Const, s32{}});  // -> i64
    Step(I{opcode});              // i64 -> f32
    Step(I{O::End});              //
    Step(I{O::Drop});             // f32 ->
  }
}

TEST_F(ValidateInstructionTest, F32DemoteF64) {
  Step(I{O::Block, BlockType::F32});
  Step(I{O::F64Const, f64{}});  // -> f64
  Step(I{O::F32DemoteF64});     // f64 -> f32
  Step(I{O::End});              //
}

TEST_F(ValidateInstructionTest, F64_I32) {
  const Opcode opcodes[] = {O::F64ConvertI32S, O::F64ConvertI32U};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::F64});
    Step(I{O::I32Const, s32{}});  // -> i32
    Step(I{opcode});              // i32 -> f64
    Step(I{O::End});              //
    Step(I{O::Drop});             // f64 ->
  }
}

TEST_F(ValidateInstructionTest, F64_I64) {
  const Opcode opcodes[] = {O::F64ConvertI64S, O::F64ConvertI64U,
                            O::F64ReinterpretI64};

  for (auto opcode : opcodes) {
    Step(I{O::Block, BlockType::F64});
    Step(I{O::I64Const, s32{}});  // -> i64
    Step(I{opcode});              // i64 -> f64
    Step(I{O::End});              //
    Step(I{O::Drop});             // f64 ->
  }
}

TEST_F(ValidateInstructionTest, F64PromoteF32) {
  Step(I{O::Block, BlockType::F64});
  Step(I{O::F32Const, f32{}});  // -> f32
  Step(I{O::F64PromoteF32});    // f32 -> f64
  Step(I{O::End});              //
}
