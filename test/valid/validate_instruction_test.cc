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
#include "wasp/valid/errors_nop.h"
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
  using VT = ValueType;

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

  Index AddElementSegment(const SegmentType& segment_type) {
    return AddItem(context.element_segments, segment_type);
  }

  Index AddLocal(const ValueType& value_type) {
    return AddItem(context.locals, value_type);
  }

  void Ok(const Instruction& instruction) {
    EXPECT_TRUE(Validate(instruction, context, features, errors))
        << format("{}", instruction);
  }

  void Fail(const Instruction& instruction) {
    EXPECT_FALSE(Validate(instruction, context, features, errors))
        << format("{}", instruction);
  }

  void TestSignature(const Instruction& instruction,
                     const ValueTypes& param_types,
                     const ValueTypes& result_types) {
    auto saved_context = context;
    ErrorsNop errors_nop;
    auto& type_stack = context.type_stack;

    // Test that it is only valid when the full list of parameters is on the
    // stack.
    for (size_t n = 0; n <= param_types.size(); ++n) {
      const ValueTypes param_types_slice(param_types.begin() + n,
                                         param_types.end());
      type_stack = param_types_slice;
      if (n == 0) {
        EXPECT_TRUE(Validate(instruction, context, features, errors))
            << format("{} with stack {}", instruction, param_types_slice);
        EXPECT_EQ(result_types, type_stack) << format("{}", instruction);
      } else {
        EXPECT_FALSE(Validate(instruction, context, features, errors_nop))
            << format("{} with stack {}", instruction, param_types_slice);
      }
    }

    if (!param_types.empty()) {
      // Create a type stack of the right size, but with all mismatched types.
      auto mismatch_types = param_types;
      for (auto& value_type : mismatch_types) {
        value_type = value_type == VT::I32 ? VT::F64 : VT::I32;
      }
      type_stack = mismatch_types;
      EXPECT_FALSE(Validate(instruction, context, features, errors_nop))
          << format("{} with stack", instruction, mismatch_types);
    }

    // Test that it is valid with an unreachable stack.
    context.label_stack.back().unreachable = true;
    type_stack.clear();
    EXPECT_TRUE(Validate(instruction, context, features, errors))
        << format("{}", instruction);

    context = saved_context;
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
  Ok(I{O::Unreachable});
}

TEST_F(ValidateInstructionTest, Nop) {
  Ok(I{O::Nop});
}

TEST_F(ValidateInstructionTest, Block_Void) {
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Block, info.block_type});
    Ok(info.instruction);
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, Loop_Void) {
  Ok(I{O::Loop, BlockType::Void});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Loop, info.block_type});
    Ok(info.instruction);
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, If_End_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::Void});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::Void});
  Ok(I{O::Else});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::If, info.block_type});
    Ok(info.instruction);
    Ok(I{O::Else});
    Ok(info.instruction);
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, If_End_Void_Unreachable) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::Void});
  Ok(I{O::Unreachable});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_Void_Unreachable) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::Void});
  Ok(I{O::Unreachable});
  Ok(I{O::Else});
  Ok(I{O::End});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::Void});
  Ok(I{O::Else});
  Ok(I{O::Unreachable});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_SingleResult_Unreachable) {
  for (const auto& info : all_value_types) {
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::If, info.block_type});
    Ok(I{O::Unreachable});
    Ok(I{O::Else});
    Ok(info.instruction);
    Ok(I{O::End});
  }

  for (const auto& info : all_value_types) {
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::If, info.block_type});
    Ok(info.instruction);
    Ok(I{O::Else});
    Ok(I{O::Unreachable});
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, If_EmptyStack) {
  Fail(I{O::If, BlockType::Void});
}

TEST_F(ValidateInstructionTest, If_CondTypeMismatch) {
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::If, BlockType::Void});
}

TEST_F(ValidateInstructionTest, If_End_I32) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, If_End_I32_Unreachable) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::I32});
  Ok(I{O::Unreachable});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, If_Else_TypeMismatch) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Else});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::End});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType::I32});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Else});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, Else_NoIf) {
  Fail(I{O::Else});

  Ok(I{O::Block, BlockType::Void});
  Fail(I{O::Else});
}

TEST_F(ValidateInstructionTest, End) {
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, End_Unreachable) {
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::Unreachable});
  Ok(I{O::End});

  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::Unreachable});
  Ok(I{O::End});

  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::Unreachable});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, End_Unreachable_TypeMismatch) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::Unreachable});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_EmptyLabelStack) {
  Ok(I{O::End});  // This `end` ends the function.
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_EmptyTypeStack) {
  Ok(I{O::Block, BlockType::I32});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_TypeMismatch) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_TooManyValues) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, End_Unreachable_TooManyValues) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::Unreachable});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
}

TEST_F(ValidateInstructionTest, Br_Void) {
  Ok(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Block, info.block_type});
    Ok(info.instruction);
    Ok(I{O::Br, Index{0}});
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, Br_EmptyStack) {
  Ok(I{O::Block, BlockType::I32});
  Fail(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_FullerStack) {
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_TypeMismatch) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_Depth1) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Br, Index{1}});
}

TEST_F(ValidateInstructionTest, Br_ForwardUnreachable) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::Block, BlockType::F32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Br, Index{1}});
  Ok(I{O::Br, Index{0}});
}

TEST_F(ValidateInstructionTest, Br_Loop_Void) {
  Ok(I{O::Loop, BlockType::Void});
  Ok(I{O::Br, Index{0}});
  Ok(I{O::End});

  Ok(I{O::Loop, BlockType::I32});
  Ok(I{O::Br, Index{0}});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, Br_Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Loop, info.block_type});
    Ok(I{O::Br, Index{0}});
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, BrIf_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Block, info.block_type});
    Ok(info.instruction);
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::BrIf, Index{0}});
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, BrIf_NoCondition) {
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_ConditionMismatch) {
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_EmptyStack) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_TypeMismatch) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrIf, Index{0}});
}

TEST_F(ValidateInstructionTest, BrIf_PropagateValue) {
  Ok(I{O::Block, BlockType::F32});
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrIf, Index{1}});
  Fail(I{O::End});  // F32 is still on the stack.
}

TEST_F(ValidateInstructionTest, BrIf_Loop_Void) {
  Ok(I{O::Loop, BlockType::Void});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrIf, Index{0}});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, BrIf_Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Loop, info.block_type});
    Ok(info.instruction);
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::BrIf, Index{0}});
    Ok(I{O::Unreachable});
    Ok(I{O::End});
  }
}

TEST_F(ValidateInstructionTest, BrTable_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{0, 0, 0}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_MultiDepth_Void) {
  Ok(I{O::Block, BlockType::Void});  // 3
  Ok(I{O::Block, BlockType::Void});  // 2
  Ok(I{O::Block, BlockType::Void});  // 1
  Ok(I{O::Block, BlockType::Void});  // 0
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{0, 1, 2, 3}, 4}});
}

TEST_F(ValidateInstructionTest, BrTable_MultiDepth_SingleResult) {
  Ok(I{O::Block, BlockType::I32});   // 3
  Ok(I{O::Block, BlockType::Void});  // 2
  Ok(I{O::Block, BlockType::I32});   // 1
  Ok(I{O::Block, BlockType::Void});  // 0
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{1, 1, 1, 3}, 3}});
}

TEST_F(ValidateInstructionTest, BrTable_Unreachable) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{}, 1}});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, BrTable_NoKey) {
  Fail(I{O::BrTable, BrTableImmediate{{}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_EmptyStack) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_ValueTypeMismatch) {
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{0}, 0}});
}

TEST_F(ValidateInstructionTest, BrTable_InconsistentLabelSignature) {
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::Block, BlockType::I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{1}, 0}});
}

TEST_F(ValidateInstructionTest, Return) {
  Ok(I{O::Return});
}

TEST_F(ValidateInstructionTest, Return_InsideBlocks) {
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::Block, BlockType::Void});
  Ok(I{O::Return});
}

TEST_F(ValidateInstructionTest, Return_Unreachable) {
  Ok(I{O::Block, BlockType::F64});
  Ok(I{O::Return});
  Ok(I{O::End});
}

TEST_F(ValidateInstructionTest, Return_SingleResult) {
  BeginFunction(FunctionType{{}, {VT::I32}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Return});
}

TEST_F(ValidateInstructionTest, Return_TypeMismatch) {
  BeginFunction(FunctionType{{}, {VT::I32}});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Return});
}

TEST_F(ValidateInstructionTest, Call_Void_Void) {
  auto index = AddFunction(FunctionType{});
  Ok(I{O::Call, Index{index}});
}

TEST_F(ValidateInstructionTest, Call_Params) {
  auto param_types = {VT::I32, VT::F32};
  auto result_types = {VT::F64};
  auto index = AddFunction(FunctionType{param_types, result_types});
  TestSignature(I{O::Call, Index{index}}, param_types, result_types);
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
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, CallIndirect_Params) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  auto index = AddFunctionType(FunctionType{{VT::F32, VT::I64}, {}});
  TestSignature(I{O::CallIndirect, CallIndirectImmediate{index, 0}},
                {VT::F32, VT::I64, VT::I32}, {});
}

TEST_F(ValidateInstructionTest, CallIndirect_TableIndexOOB) {
  auto index = AddFunctionType(FunctionType{});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, CallIndirect_TypeIndexOOB) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{100, 0}});
}

TEST_F(ValidateInstructionTest, Drop) {
  for (const auto& info : all_value_types) {
    Ok(info.instruction);
    Ok(I{O::Drop});
  }
}

TEST_F(ValidateInstructionTest, Drop_EmptyStack) {
  Fail(I{O::Drop});
}

TEST_F(ValidateInstructionTest, Select) {
  for (const auto& info : all_value_types) {
    Ok(info.instruction);
    Ok(info.instruction);
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::Select});
  }
}

TEST_F(ValidateInstructionTest, Select_EmptyStack) {
  Fail(I{O::Select});
}

TEST_F(ValidateInstructionTest, Select_ConditionTypeMismatch) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Select});
}

TEST_F(ValidateInstructionTest, Select_InconsistentTypes) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::Select});
}

TEST_F(ValidateInstructionTest, LocalGet) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    TestSignature(I{O::LocalGet, Index{index}}, {}, {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, LocalGet_Param) {
  BeginFunction(FunctionType{{VT::I32, VT::F32}, {}});
  auto index = AddLocal(VT::I64);
  EXPECT_EQ(2, index);
  Ok(I{O::LocalGet, Index{0}}); // 1st param.
  Ok(I{O::LocalGet, Index{1}}); // 2nd param.
  Ok(I{O::LocalGet, Index{2}}); // 1st local.
  Fail(I{O::LocalGet, Index{3}}); // Invalid.
}

TEST_F(ValidateInstructionTest, LocalGet_IndexOOB) {
  Fail(I{O::LocalGet, Index{100}});
}

TEST_F(ValidateInstructionTest, LocalSet) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    TestSignature(I{O::LocalSet, Index{index}}, {info.value_type}, {});
  }
}

TEST_F(ValidateInstructionTest, LocalSet_Param) {
  BeginFunction(FunctionType{{VT::I32, VT::F32}, {}});
  auto index = AddLocal(VT::F64);
  EXPECT_EQ(2, index);
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::LocalSet, Index{0}}); // 1st param.
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::LocalSet, Index{1}}); // 2nd param.
  Ok(I{O::F64Const, f64{}});
  Ok(I{O::LocalSet, Index{2}}); // 1st local.
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::LocalSet, Index{3}}); // Invalid.
}

TEST_F(ValidateInstructionTest, LocalSet_IndexOOB) {
  Fail(I{O::LocalSet, Index{100}});
}

TEST_F(ValidateInstructionTest, LocalTee) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    TestSignature(I{O::LocalTee, Index{index}}, {info.value_type},
                  {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, LocalTee_IndexOOB) {
  Fail(I{O::LocalTee, Index{100}});
}

TEST_F(ValidateInstructionTest, GlobalGet) {
  for (auto mut : {Mutability::Var, Mutability::Const}) {
    for (const auto& info : all_value_types) {
      auto index = AddGlobal(GlobalType{info.value_type, mut});
      TestSignature(I{O::GlobalGet, Index{index}}, {}, {info.value_type});
    }
  }
}

TEST_F(ValidateInstructionTest, GlobalGet_IndexOOB) {
  Fail(I{O::GlobalGet, Index{100}});
}

TEST_F(ValidateInstructionTest, GlobalSet) {
  for (const auto& info : all_value_types) {
    auto index = AddGlobal(GlobalType{info.value_type, Mutability::Var});
    TestSignature(I{O::GlobalSet, Index{index}}, {info.value_type}, {});
  }
}

TEST_F(ValidateInstructionTest, GlobalSet_IndexOOB) {
  Fail(I{O::GlobalSet, Index{100}});
}

TEST_F(ValidateInstructionTest, GlobalSet_Immutable) {
  auto index = AddGlobal(GlobalType{VT::I32, Mutability::Const});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::GlobalSet, Index{index}});
}

TEST_F(ValidateInstructionTest, Load) {
  const struct {
    Opcode opcode;
    ValueType result;
  } infos[] = {{O::I32Load, VT::I32},    {O::I32Load8S, VT::I32},
               {O::I32Load8U, VT::I32},  {O::I32Load16S, VT::I32},
               {O::I32Load16U, VT::I32}, {O::I64Load, VT::I64},
               {O::I64Load8S, VT::I64},  {O::I64Load8U, VT::I64},
               {O::I64Load16S, VT::I64}, {O::I64Load16U, VT::I64},
               {O::I64Load32S, VT::I64}, {O::I64Load32U, VT::I64},
               {O::F32Load, VT::F32},    {O::F64Load, VT::F64}};

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{0, 0}}, {VT::I32},
                  {info.result});
  }
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
    Ok(I{O::I32Const, s32{}});
    Ok(I{info.opcode, MemArgImmediate{info.max_align, 0}});

    Ok(I{O::I32Const, s32{}});
    Fail(I{info.opcode, MemArgImmediate{info.max_align + 1, 0}});
  }
}

TEST_F(ValidateInstructionTest, Load_MemoryOOB) {
  const Opcode opcodes[] = {
      O::I32Load,    O::I64Load,    O::F32Load,    O::F64Load,   O::I32Load8S,
      O::I32Load8U,  O::I32Load16S, O::I32Load16U, O::I64Load8S, O::I64Load8U,
      O::I64Load16S, O::I64Load16U, O::I64Load32S, O::I64Load32U};

  for (const auto& opcode : opcodes) {
    Ok(I{O::I32Const, s32{}});
    Fail(I{opcode, MemArgImmediate{0, 0}});
  }
}

TEST_F(ValidateInstructionTest, Store) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I32Store, VT::I32},   {O::I32Store8, VT::I32},
               {O::I32Store16, VT::I32}, {O::I64Store, VT::I64},
               {O::I64Store8, VT::I64},  {O::I64Store16, VT::I64},
               {O::I64Store32, VT::I64}, {O::F32Store, VT::F32},
               {O::F64Store, VT::F64}};

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{0, 0}},
                  {VT::I32, info.value_type}, {});
  }
}

TEST_F(ValidateInstructionTest, Store_MemoryOOB) {
  const Opcode opcodes[] = {O::I32Store,  O::I64Store,   O::F32Store,
                            O::F64Store,  O::I32Store8,  O::I32Store16,
                            O::I64Store8, O::I64Store16, O::I64Store32};

  Ok(I{O::Unreachable});
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
  Ok(I{O::Unreachable});
  for (const auto& info : infos) {
    Ok(I{info.opcode, MemArgImmediate{info.max_align, 0}});
    Fail(I{info.opcode, MemArgImmediate{info.max_align + 1, 0}});
  }
}

TEST_F(ValidateInstructionTest, MemorySize) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemorySize, u8{}}, {}, {VT::I32});
}

TEST_F(ValidateInstructionTest, MemorySize_MemoryIndexOOB) {
  Fail(I{O::MemorySize, u8{}});
}

TEST_F(ValidateInstructionTest, MemoryGrow) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryGrow, u8{}}, {VT::I32}, {VT::I32});
}

TEST_F(ValidateInstructionTest, MemoryGrow_MemoryIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryGrow, u8{}});
}

TEST_F(ValidateInstructionTest, Unary) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {
      {O::I32Eqz, VT::I32},     {O::I32Clz, VT::I32},
      {O::I32Ctz, VT::I32},     {O::I32Popcnt, VT::I32},
      {O::I64Clz, VT::I64},     {O::I64Ctz, VT::I64},
      {O::I64Popcnt, VT::I64},  {O::F32Abs, VT::F32},
      {O::F32Neg, VT::F32},     {O::F32Ceil, VT::F32},
      {O::F32Floor, VT::F32},   {O::F32Trunc, VT::F32},
      {O::F32Nearest, VT::F32}, {O::F32Sqrt, VT::F32},
      {O::F64Abs, VT::F64},     {O::F64Neg, VT::F64},
      {O::F64Ceil, VT::F64},    {O::F64Floor, VT::F64},
      {O::F64Trunc, VT::F64},   {O::F64Nearest, VT::F64},
      {O::F64Sqrt, VT::F64},
  };

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type}, {info.value_type});
  }

  TestSignature(I{O::I64Eqz}, {VT::I64}, {VT::I32});
}

TEST_F(ValidateInstructionTest, Binary) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I32Add, VT::I32},      {O::I32Sub, VT::I32},
               {O::I32Mul, VT::I32},      {O::I32DivS, VT::I32},
               {O::I32DivU, VT::I32},     {O::I32RemS, VT::I32},
               {O::I32RemU, VT::I32},     {O::I32And, VT::I32},
               {O::I32Or, VT::I32},       {O::I32Xor, VT::I32},
               {O::I32Shl, VT::I32},      {O::I32ShrS, VT::I32},
               {O::I32ShrU, VT::I32},     {O::I32Rotl, VT::I32},
               {O::I32Rotr, VT::I32},     {O::I64Add, VT::I64},
               {O::I64Sub, VT::I64},      {O::I64Mul, VT::I64},
               {O::I64DivS, VT::I64},     {O::I64DivU, VT::I64},
               {O::I64RemS, VT::I64},     {O::I64RemU, VT::I64},
               {O::I64And, VT::I64},      {O::I64Or, VT::I64},
               {O::I64Xor, VT::I64},      {O::I64Shl, VT::I64},
               {O::I64ShrS, VT::I64},     {O::I64ShrU, VT::I64},
               {O::I64Rotl, VT::I64},     {O::I64Rotr, VT::I64},
               {O::F32Add, VT::F32},      {O::F32Sub, VT::F32},
               {O::F32Mul, VT::F32},      {O::F32Div, VT::F32},
               {O::F32Min, VT::F32},      {O::F32Max, VT::F32},
               {O::F32Copysign, VT::F32}, {O::F64Add, VT::F64},
               {O::F64Sub, VT::F64},      {O::F64Mul, VT::F64},
               {O::F64Div, VT::F64},      {O::F64Min, VT::F64},
               {O::F64Max, VT::F64},      {O::F64Copysign, VT::F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type, info.value_type},
                  {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, Compare) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I32Eq, VT::I32},  {O::I32Ne, VT::I32},  {O::I32LtS, VT::I32},
               {O::I32LtU, VT::I32}, {O::I32GtS, VT::I32}, {O::I32GtU, VT::I32},
               {O::I32LeS, VT::I32}, {O::I32LeU, VT::I32}, {O::I32GeS, VT::I32},
               {O::I32GeU, VT::I32}, {O::I64Eq, VT::I64},  {O::I64Ne, VT::I64},
               {O::I64LtS, VT::I64}, {O::I64LtU, VT::I64}, {O::I64GtS, VT::I64},
               {O::I64GtU, VT::I64}, {O::I64LeS, VT::I64}, {O::I64LeU, VT::I64},
               {O::I64GeS, VT::I64}, {O::I64GeU, VT::I64}, {O::F32Eq, VT::F32},
               {O::F32Ne, VT::F32},  {O::F32Lt, VT::F32},  {O::F32Gt, VT::F32},
               {O::F32Le, VT::F32},  {O::F32Ge, VT::F32},  {O::F64Eq, VT::F64},
               {O::F64Ne, VT::F64},  {O::F64Lt, VT::F64},  {O::F64Gt, VT::F64},
               {O::F64Le, VT::F64},  {O::F64Ge, VT::F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type, info.value_type},
                  {VT::I32});
  }
}

TEST_F(ValidateInstructionTest, Conversion) {
  const struct {
    Opcode opcode;
    ValueType to;
    ValueType from;
  } infos[] = {{O::I32TruncF32S, VT::I32, VT::F32},
               {O::I32TruncF32U, VT::I32, VT::F32},
               {O::I32ReinterpretF32, VT::I32, VT::F32},
               {O::I32TruncF64S, VT::I32, VT::F64},
               {O::I32TruncF64U, VT::I32, VT::F64},
               {O::I64ExtendI32S, VT::I64, VT::I32},
               {O::I64ExtendI32U, VT::I64, VT::I32},
               {O::I64TruncF32S, VT::I64, VT::F32},
               {O::I64TruncF32U, VT::I64, VT::F32},
               {O::I64TruncF64S, VT::I64, VT::F64},
               {O::I64TruncF64U, VT::I64, VT::F64},
               {O::I64ReinterpretF64, VT::I64, VT::F64},
               {O::F32ConvertI32S, VT::F32, VT::I32},
               {O::F32ConvertI32U, VT::F32, VT::I32},
               {O::F32ReinterpretI32, VT::F32, VT::I32},
               {O::F32ConvertI64S, VT::F32, VT::I64},
               {O::F32ConvertI64U, VT::F32, VT::I64},
               {O::F32DemoteF64, VT::F32, VT::F64},
               {O::F64ConvertI32S, VT::F64, VT::I32},
               {O::F64ConvertI32U, VT::F64, VT::I32},
               {O::F64ConvertI64S, VT::F64, VT::I64},
               {O::F64ConvertI64U, VT::F64, VT::I64},
               {O::F64ReinterpretI64, VT::F64, VT::I64},
               {O::F64PromoteF32, VT::F64, VT::F32}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.from}, {info.to});
  }
}

TEST_F(ValidateInstructionTest, SignExtension) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I32Extend8S, VT::I32},
               {O::I32Extend16S, VT::I32},
               {O::I64Extend8S, VT::I64},
               {O::I64Extend16S, VT::I64},
               {O::I64Extend32S, VT::I64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type}, {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, SaturatingFloatToInt) {
  const struct {
    Opcode opcode;
    ValueType to;
    ValueType from;
  } infos[] = {{O::I32TruncSatF32S, VT::I32, VT::F32},
               {O::I32TruncSatF32U, VT::I32, VT::F32},
               {O::I32TruncSatF64S, VT::I32, VT::F64},
               {O::I32TruncSatF64U, VT::I32, VT::F64},
               {O::I64TruncSatF32S, VT::I64, VT::F32},
               {O::I64TruncSatF32U, VT::I64, VT::F32},
               {O::I64TruncSatF64S, VT::I64, VT::F64},
               {O::I64TruncSatF64U, VT::I64, VT::F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.from}, {info.to});
  }
}

TEST_F(ValidateInstructionTest, MemoryInit) {
  context.data_segment_count = 2;
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryInit, InitImmediate{1, 0}},
                {VT::I32, VT::I32, VT::I32}, {});
}

TEST_F(ValidateInstructionTest, MemoryInit_MemoryIndexOOB) {
  context.data_segment_count = 2;
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryInit, InitImmediate{1, 0}});
}

TEST_F(ValidateInstructionTest, MemoryInit_SegmentIndexOOB) {
  context.data_segment_count = 2;
  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryInit, InitImmediate{2, 0}});
}

TEST_F(ValidateInstructionTest, DataDrop) {
  context.data_segment_count = 2;
  TestSignature(I{O::DataDrop, Index{1}}, {}, {});
}

TEST_F(ValidateInstructionTest, DataDrop_SegmentIndexOOB) {
  context.data_segment_count = 2;
  Fail(I{O::DataDrop, Index{2}});
}

TEST_F(ValidateInstructionTest, MemoryCopy) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryCopy, CopyImmediate{0, 0}},
                {VT::I32, VT::I32, VT::I32}, {});
}

TEST_F(ValidateInstructionTest, MemoryCopy_MemoryIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryCopy, CopyImmediate{0, 0}});
}

TEST_F(ValidateInstructionTest, MemoryFill) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryFill, u8{0}}, {VT::I32, VT::I32, VT::I32}, {});
}

TEST_F(ValidateInstructionTest, MemoryFill_MemoryIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryFill, u8{0}});
}

TEST_F(ValidateInstructionTest, TableInit) {
  auto index = AddElementSegment(SegmentType::Passive);
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  TestSignature(I{O::TableInit, InitImmediate{index, 0}},
                {VT::I32, VT::I32, VT::I32}, {});
}

TEST_F(ValidateInstructionTest, TableInit_TableIndexOOB) {
  auto index = AddElementSegment(SegmentType::Passive);
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableInit, InitImmediate{index, 0}});
}

TEST_F(ValidateInstructionTest, TableInit_SegmentIndexOOB) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableInit, InitImmediate{0, 0}});
}

TEST_F(ValidateInstructionTest, ElemDrop) {
  auto index = AddElementSegment(SegmentType::Passive);
  TestSignature(I{O::ElemDrop, Index{index}}, {}, {});
}

TEST_F(ValidateInstructionTest, ElemDrop_SegmentIndexOOB) {
  Fail(I{O::ElemDrop, Index{0}});
}

TEST_F(ValidateInstructionTest, TableCopy) {
  AddTable(TableType{Limits{0}, ElementType::Funcref});
  TestSignature(I{O::TableCopy, CopyImmediate{0, 0}},
                {VT::I32, VT::I32, VT::I32}, {});
}

TEST_F(ValidateInstructionTest, TableCopy_TableIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableCopy, CopyImmediate{0, 0}});
}

TEST_F(ValidateInstructionTest, SimdLoad) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::V128Load, MemArgImmediate{0, 0}}, {VT::I32}, {VT::V128});
}

TEST_F(ValidateInstructionTest, SimdLoad_Alignment) {
  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::V128Load, MemArgImmediate{4, 0}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::V128Load, MemArgImmediate{5, 0}});
}

TEST_F(ValidateInstructionTest, SimdLoad_MemoryOOB) {
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::V128Load, MemArgImmediate{0, 0}});
}

TEST_F(ValidateInstructionTest, SimdStore) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::V128Store, MemArgImmediate{0, 0}}, {VT::I32, VT::V128},
                {});
}

TEST_F(ValidateInstructionTest, SimdStore_Alignment) {
  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::Unreachable});
  Ok(I{O::V128Store, MemArgImmediate{4, 0}});
  Fail(I{O::V128Store, MemArgImmediate{5, 0}});
}

TEST_F(ValidateInstructionTest, SimdStore_MemoryOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::V128Const, v128{}});
  Fail(I{O::V128Store, MemArgImmediate{0, 0}});
}

TEST_F(ValidateInstructionTest, SimdConst) {
  TestSignature(I{O::V128Const}, {}, {VT::V128});
}

TEST_F(ValidateInstructionTest, SimdBitSelect) {
  TestSignature(I{O::V128BitSelect}, {VT::V128, VT::V128, VT::V128},
                {VT::V128});
}

TEST_F(ValidateInstructionTest, SimdUnary) {
  const Opcode opcodes[] = {
    O::V128Not,
    O::I8X16Neg,
    O::I16X8Neg,
    O::I32X4Neg,
    O::I64X2Neg,
    O::F32X4Abs,
    O::F32X4Neg,
    O::F32X4Sqrt,
    O::F64X2Abs,
    O::F64X2Neg,
    O::F64X2Sqrt,
    O::I32X4TruncSatF32X4S,
    O::I32X4TruncSatF32X4U,
    O::I64X2TruncSatF64X2S,
    O::I64X2TruncSatF64X2U,
    O::F32X4ConvertI32X4S,
    O::F32X4ConvertI32X4U,
    O::F64X2ConvertI64X2S,
    O::F64X2ConvertI64X2U,
  };

  for (const auto& opcode: opcodes) {
    TestSignature(I{opcode}, {VT::V128}, {VT::V128});
  }
}

TEST_F(ValidateInstructionTest, SimdBinary) {
  const Opcode opcodes[] = {
      O::V8X16Shuffle,      O::I8X16Eq,           O::I8X16Ne,
      O::I8X16LtS,          O::I8X16LtU,          O::I8X16GtS,
      O::I8X16GtU,          O::I8X16LeS,          O::I8X16LeU,
      O::I8X16GeS,          O::I8X16GeU,          O::I16X8Eq,
      O::I16X8Ne,           O::I16X8LtS,          O::I16X8LtU,
      O::I16X8GtS,          O::I16X8GtU,          O::I16X8LeS,
      O::I16X8LeU,          O::I16X8GeS,          O::I16X8GeU,
      O::I32X4Eq,           O::I32X4Ne,           O::I32X4LtS,
      O::I32X4LtU,          O::I32X4GtS,          O::I32X4GtU,
      O::I32X4LeS,          O::I32X4LeU,          O::I32X4GeS,
      O::I32X4GeU,          O::F32X4Eq,           O::F32X4Ne,
      O::F32X4Lt,           O::F32X4Gt,           O::F32X4Le,
      O::F32X4Ge,           O::F64X2Eq,           O::F64X2Ne,
      O::F64X2Lt,           O::F64X2Gt,           O::F64X2Le,
      O::F64X2Ge,           O::V128And,           O::V128Or,
      O::V128Xor,           O::I8X16Add,          O::I8X16AddSaturateS,
      O::I8X16AddSaturateU, O::I8X16Sub,          O::I8X16SubSaturateS,
      O::I8X16SubSaturateU, O::I8X16Mul,          O::I16X8Add,
      O::I16X8AddSaturateS, O::I16X8AddSaturateU, O::I16X8Sub,
      O::I16X8SubSaturateS, O::I16X8SubSaturateU, O::I16X8Mul,
      O::I32X4Add,          O::I32X4Sub,          O::I32X4Mul,
      O::I64X2Add,          O::I64X2Sub,          O::F32X4Add,
      O::F32X4Sub,          O::F32X4Mul,          O::F32X4Div,
      O::F32X4Min,          O::F32X4Max,          O::F64X2Add,
      O::F64X2Sub,          O::F64X2Mul,          O::F64X2Div,
      O::F64X2Min,          O::F64X2Max};

  for (const auto& opcode: opcodes) {
    TestSignature(I{opcode}, {VT::V128, VT::V128}, {VT::V128});
  }
}

TEST_F(ValidateInstructionTest, SimdAnyTrueAllTrue) {
  const Opcode opcodes[] = {O::I8X16AnyTrue, O::I8X16AllTrue, O::I16X8AnyTrue,
                            O::I16X8AllTrue, O::I32X4AnyTrue, O::I32X4AllTrue,
                            O::I64X2AnyTrue, O::I64X2AllTrue};

  for (const auto& opcode : opcodes) {
    TestSignature(I{opcode}, {VT::V128}, {VT::I32});
  }
}

TEST_F(ValidateInstructionTest, SimdSplats) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I8X16Splat, VT::I32}, {O::I16X8Splat, VT::I32},
               {O::I32X4Splat, VT::I32}, {O::I64X2Splat, VT::I64},
               {O::F32X4Splat, VT::F32}, {O::F64X2Splat, VT::F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type}, {VT::V128});
  }
}

TEST_F(ValidateInstructionTest, SimdExtractLanes) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I8X16ExtractLaneS, VT::I32}, {O::I8X16ExtractLaneU, VT::I32},
               {O::I16X8ExtractLaneS, VT::I32}, {O::I16X8ExtractLaneU, VT::I32},
               {O::I32X4ExtractLane, VT::I32},  {O::I64X2ExtractLane, VT::I64},
               {O::F32X4ExtractLane, VT::F32},  {O::F64X2ExtractLane, VT::F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {VT::V128}, {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, SimdReplaceLanes) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I8X16ReplaceLane, VT::I32}, {O::I16X8ReplaceLane, VT::I32},
               {O::I32X4ReplaceLane, VT::I32}, {O::I64X2ReplaceLane, VT::I64},
               {O::F32X4ReplaceLane, VT::F32}, {O::F64X2ReplaceLane, VT::F64}};

  for (const auto& info : infos) {
    TestSignature(I{info.opcode}, {VT::V128, info.value_type}, {VT::V128});
  }
}

TEST_F(ValidateInstructionTest, SimdShifts) {
  const Opcode opcodes[] = {O::I8X16Shl, O::I8X16ShrS, O::I8X16ShrU,
                            O::I16X8Shl, O::I16X8ShrS, O::I16X8ShrU,
                            O::I32X4Shl, O::I32X4ShrS, O::I32X4ShrU,
                            O::I64X2Shl, O::I64X2ShrS, O::I64X2ShrU};

  for (const auto& opcode : opcodes) {
    TestSignature(I{opcode}, {VT::V128, VT::I32}, {VT::V128});
  }
}
