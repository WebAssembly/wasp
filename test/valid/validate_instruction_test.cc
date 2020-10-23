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

#include <cassert>

#include "gtest/gtest.h"
#include "test/binary/constants.h"
#include "test/valid/test_utils.h"
#include "wasp/base/errors_nop.h"
#include "wasp/base/features.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"
#include "wasp/valid/formatters.h"
#include "wasp/valid/match.h"
#include "wasp/valid/validate.h"

#include "wasp/base/concat.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;

class ValidateInstructionTest : public ::testing::Test {
 protected:
  using I = Instruction;
  using O = Opcode;
  using VT = ValueType;
  using ST = StackType;
  using HT = HeapType;

  ValidateInstructionTest() : context{errors} {}

  virtual void SetUp() {
    BeginFunction(FunctionType{});
  }

  virtual void TearDown() {}

  void BeginFunction(const FunctionType& function_type) {
    context.Reset();
    AddFunction(function_type);
    EXPECT_TRUE(BeginCode(context, Location{}));
  }

  template <typename T>
  Index AddItem(std::vector<T>& vec, const T& type) {
    vec.push_back(type);
    return vec.size() - 1;
  }

  void IncrementDefinedTypeCount() {
    context.defined_type_count++;
    context.same_types.Reset(context.defined_type_count);
    context.match_types.Reset(context.defined_type_count);
  }

  Index AddFunctionType(const FunctionType& function_type) {
    IncrementDefinedTypeCount();
    return AddItem(context.types, DefinedType{function_type});
  }

  Index AddStructType(const StructType& struct_type) {
    IncrementDefinedTypeCount();
    return AddItem(context.types, DefinedType{struct_type});
  }

  Index AddArrayType(const ArrayType& array_type) {
    IncrementDefinedTypeCount();
    return AddItem(context.types, DefinedType{array_type});
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

  Index AddEvent(const EventType& event_type) {
    return AddItem(context.events, event_type);
  }

  Index AddElementSegment(ReferenceType elem_type) {
    return AddItem(context.element_segments, elem_type);
  }

  Index AddLocal(const ValueType& value_type) {
    bool ok = context.locals.Append(1, value_type);
    WASP_USE(ok);
    assert(ok);
    return context.locals.GetCount() - 1;
  }

  ValueType MakeValueTypeRef(Index index, Null null) {
    return ValueType{ReferenceType{RefType{HeapType{index}, null}}};
  }

  void Ok(const Instruction& instruction) {
    EXPECT_TRUE(Validate(context, instruction)) << instruction;
  }

  void Fail(const Instruction& instruction) {
    EXPECT_FALSE(Validate(context, instruction)) << instruction;
  }

  void OkWithTypeStack(const Instruction& instruction,
                       const StackTypeList& param_types,
                       const StackTypeList& result_types) {
    TestErrors errors;
    Context context_copy{context, errors};
    context_copy.type_stack = param_types;
    EXPECT_TRUE(Validate(context_copy, instruction))
        << concat(instruction, " with stack ", param_types);
    EXPECT_TRUE(IsSame(context, result_types, context_copy.type_stack))
        << instruction;
    ExpectNoErrors(errors);
  }

  void OkWithUnreachableStack(const Instruction& instruction,
                              const ValueTypeList& param_types,
                              const ValueTypeList& result_types) {
    TestErrors errors;
    Context context_copy{context, errors};
    context_copy.label_stack.back().unreachable = true;
    context_copy.type_stack = ToStackTypeList(param_types);
    EXPECT_TRUE(Validate(context_copy, instruction)) << instruction;
    EXPECT_TRUE(
        IsSame(context, ToStackTypeList(result_types), context_copy.type_stack))
        << instruction;
    ExpectNoErrors(errors);
  }

  void FailWithTypeStack(const Instruction& instruction,
                         const StackTypeList& param_types) {
    ErrorsNop errors_nop;
    Context context_copy{context, errors_nop};
    context_copy.type_stack = param_types;
    EXPECT_FALSE(Validate(context_copy, instruction))
        << concat(instruction, " with stack ", param_types);
  }

  void FailWithTypeStack(const Instruction& instruction,
                         const ValueTypeList& param_types) {
    FailWithTypeStack(instruction, ToStackTypeList(param_types));
  }


  void TestSignatureNoUnreachable(const Instruction& instruction,
                                  const ValueTypeList& param_types,
                                  const ValueTypeList& result_types) {
    const StackTypeList stack_param_types = ToStackTypeList(param_types);
    const StackTypeList stack_result_types = ToStackTypeList(result_types);

    // Test that it is only valid when the full list of parameters is on the
    // stack.
    for (size_t n = 0; n <= param_types.size(); ++n) {
      const StackTypeList stack_param_types_slice(stack_param_types.begin() + n,
                                                  stack_param_types.end());
      if (n == 0) {
        OkWithTypeStack(instruction, stack_param_types_slice,
                        stack_result_types);
      } else {
        FailWithTypeStack(instruction, stack_param_types_slice);
      }
    }

    if (!stack_param_types.empty()) {
      // Create a type stack of the right size, but with all mismatched types.
      auto mismatch_types = stack_param_types;
      for (auto& stack_type : mismatch_types) {
        stack_type =
            IsSame(context, stack_type, ST::I32()) ? ST::F64() : ST::I32();
      }
      FailWithTypeStack(instruction, mismatch_types);
    }
  }

  void TestSignature(const Instruction& instruction,
                     const ValueTypeList& param_types,
                     const ValueTypeList& result_types) {
    TestSignatureNoUnreachable(instruction, param_types, result_types);
    OkWithUnreachableStack(instruction, {}, result_types);
  }

  TestErrors errors;
  Context context;
};

struct ValueTypeInfo {
  ValueType value_type;
  BlockType block_type;
  Instruction instruction;
};
const ValueTypeInfo all_value_types[] = {
    {VT_I32, BT_I32, Instruction{Opcode::I32Const, s32{}}},
    {VT_I64, BT_I64, Instruction{Opcode::I64Const, s64{}}},
    {VT_F32, BT_F32, Instruction{Opcode::F32Const, f32{}}},
    {VT_F64, BT_F64, Instruction{Opcode::F64Const, f64{}}},
#if 0
    {VT_V128, BT_V128, Instruction{Opcode::V128Const, v128{}}},
    {VT_Externref), BT_Externref, Instruction{Opcode::RefNull}},
#endif
};

TEST_F(ValidateInstructionTest, Unreachable) {
  Ok(I{O::Unreachable});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Nop) {
  Ok(I{O::Nop});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Block_Void) {
  Ok(I{O::Block, BT_Void});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Block, info.block_type});
    Ok(info.instruction);
    Ok(I{O::End});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Block_MultiResult) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});
  Ok(I{O::Block, BlockType(index)});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, s32{}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Block_RefType) {
  auto index = AddFunctionType(FunctionType{{VT_Ref0}, {}});

  TestSignature(I{O::Block, BT_Ref0}, {}, {});
  TestSignature(I{O::Block, BlockType{index}}, {VT_Ref0}, {VT_Ref0});
}

TEST_F(ValidateInstructionTest, Block_RefType_IndexOOB) {
  Fail(I{O::Block, BlockType{VT_Ref1}});
}

TEST_F(ValidateInstructionTest, Block_Param) {
  auto index = AddFunctionType(FunctionType{{VT_I64}, {}});
  Ok(I{O::I64Const, s64{}});
  Ok(I{O::Block, BlockType(index)});
  Ok(I{O::Drop});
  Ok(I{O::End});
  Fail(I{O::Drop});  // Nothing left on the stack.
  ExpectError({"instruction", "Expected stack to contain 1 value, got 0"},
              errors);
}

TEST_F(ValidateInstructionTest, Loop_Void) {
  Ok(I{O::Loop, BT_Void});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Loop, info.block_type});
    Ok(info.instruction);
    Ok(I{O::End});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Loop_MultiResult) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});
  Ok(I{O::Loop, BlockType(index)});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, s32{}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Loop_RefType) {
  auto index = AddFunctionType(FunctionType{{VT_Ref0}, {}});

  TestSignature(I{O::Loop, BT_Ref0}, {}, {});
  TestSignature(I{O::Loop, BlockType{index}}, {VT_Ref0}, {VT_Ref0});
}

TEST_F(ValidateInstructionTest, Loop_RefType_IndexOOB) {
  Fail(I{O::Loop, BlockType{VT_Ref1}});
}

TEST_F(ValidateInstructionTest, Loop_Param) {
  auto index = AddFunctionType(FunctionType{{VT_I64}, {}});
  Ok(I{O::I64Const, s64{}});
  Ok(I{O::Loop, BlockType(index)});
  Ok(I{O::Drop});
  Ok(I{O::End});
  Fail(I{O::Drop});  // Nothing left on the stack.
  ExpectError({"instruction", "Expected stack to contain 1 value, got 0"},
              errors);
}

TEST_F(ValidateInstructionTest, If_End_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_Void});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_Else_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_Void});
  Ok(I{O::Else});
  Ok(I{O::End});
  ExpectNoErrors(errors);
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
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_Else_MultiResult) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType(index)});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::Else});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::End});

  Ok(I{O::I32TruncF32S});  // Convert f32 -> i32.
  Ok(I{O::I32Add});        // Should have [i32 i32] on the stack.
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_RefType) {
  auto index = AddFunctionType(FunctionType{{VT_Ref0}, {}});

  TestSignature(I{O::If, BT_Ref0}, {VT_I32}, {});
  TestSignature(I{O::If, BlockType{index}}, {VT_Ref0, VT_I32}, {VT_Ref0});
}

TEST_F(ValidateInstructionTest, If_RefType_IndexOOB) {
  Fail(I{O::If, BlockType{VT_Ref1}});
}

TEST_F(ValidateInstructionTest, If_Else_Param) {
  auto index = AddFunctionType(FunctionType{{VT_I32}, {}});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType(index)});
  Ok(I{O::Drop});
  Ok(I{O::Else});
  Ok(I{O::Drop});
  Ok(I{O::End});
  Fail(I{O::Drop});  // Nothing left on the stack.
  ExpectError({"instruction", "Expected stack to contain 1 value, got 0"},
              errors);
}

TEST_F(ValidateInstructionTest, If_Multi_PassThrough) {
  auto index = AddFunctionType(FunctionType{{VT_I32}, {VT_I32}});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType(index)});
  Ok(I{O::I32Eqz, s32{}});
  Ok(I{O::End});  // There is no else branch; if the condition is false, then
                  // the value is passed through.
  Ok(I{O::Drop});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_End_Void_Unreachable) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_Void});
  Ok(I{O::Unreachable});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_Else_Void_Unreachable) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_Void});
  Ok(I{O::Unreachable});
  Ok(I{O::Else});
  Ok(I{O::End});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_Void});
  Ok(I{O::Else});
  Ok(I{O::Unreachable});
  Ok(I{O::End});
  ExpectNoErrors(errors);
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
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_Else_MultiResult_Unreachable) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType(index)});
  Ok(I{O::Unreachable});
  Ok(I{O::Else});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, If_EmptyStack) {
  Fail(I{O::If, BT_Void});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, If_CondTypeMismatch) {
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::If, BT_Void});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, If_End_I32) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, If_End_I32_Unreachable) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_I32});
  Ok(I{O::Unreachable});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, If_Else_TypeMismatch) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Else});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BT_I32});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Else});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, If_Else_ArityMismatch) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::If, BlockType(index)});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::Else});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32 f32], got [i32]"},
              errors);
}

TEST_F(ValidateInstructionTest, Else_NoIf) {
  Fail(I{O::Else});
  ExpectError({"instruction", "Got else instruction without if"}, errors);

  Ok(I{O::Block, BT_Void});
  Fail(I{O::Else});
  ExpectError({"instruction", "Got else instruction without if"}, errors);
}

TEST_F(ValidateInstructionTest, End) {
  Ok(I{O::Block, BT_Void});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, End_Unreachable) {
  Ok(I{O::Block, BT_Void});
  Ok(I{O::Unreachable});
  Ok(I{O::End});

  Ok(I{O::Block, BT_I32});
  Ok(I{O::Unreachable});
  Ok(I{O::End});

  Ok(I{O::Block, BT_I32});
  Ok(I{O::Unreachable});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, End_Unreachable_TypeMismatch) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Unreachable});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got ...[f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, End_EmptyLabelStack) {
  Ok(I{O::End});  // This `end` ends the function.
  Fail(I{O::End});
  ExpectError({"instruction", "Unexpected instruction after function end"},
              errors);
}

TEST_F(ValidateInstructionTest, End_EmptyTypeStack) {
  Ok(I{O::Block, BT_I32});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, End_TypeMismatch) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, End_TooManyValues) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected empty stack, got [i32]"}, errors);
}

TEST_F(ValidateInstructionTest, End_Unreachable_TooManyValues) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Unreachable});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::End});
  ExpectError({"instruction", "Expected empty stack, got [i32]"}, errors);
}

TEST_F(ValidateInstructionTest, Try_Void) {
  Ok(I{O::Try, BT_Void});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Try_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Try, info.block_type});
    Ok(info.instruction);
    Ok(I{O::End});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Try_MultiResult) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});
  Ok(I{O::Try, BlockType(index)});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, s32{}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Try_Param) {
  auto index = AddFunctionType(FunctionType{{VT_I64}, {}});
  Ok(I{O::I64Const, s64{}});
  Ok(I{O::Try, BlockType(index)});
  Ok(I{O::Drop});
  Ok(I{O::End});
  Fail(I{O::Drop});  // Nothing left on the stack.
  ExpectError({"instruction", "Expected stack to contain 1 value, got 0"},
              errors);
}

TEST_F(ValidateInstructionTest, Try_Catch_Void) {
  Ok(I{O::Try, BT_Void});
  Ok(I{O::Catch});
  Ok(I{O::Drop});  // Drop the exception.
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Try_Catch_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Try, info.block_type});
    Ok(info.instruction);
    Ok(I{O::Catch});
    Ok(I{O::Drop});  // Drop the exception.
    Ok(info.instruction);
    Ok(I{O::End});
    ExpectNoErrors(errors);
  }
}

TEST_F(ValidateInstructionTest, Try_Catch_MultiResult) {
  auto index = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});
  Ok(I{O::Try, BlockType(index)});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::Catch});
  Ok(I{O::Drop});  // Drop the exception.
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Try_Catch_Param) {
  auto index = AddFunctionType(FunctionType{{VT_I32}, {}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Try, BlockType(index)});
  Ok(I{O::Drop});
  Ok(I{O::Catch});
  Ok(I{O::Drop});  // Drop the exception.
  Ok(I{O::End});
  ExpectNoErrors(errors);
  Fail(I{O::Drop});  // Nothing left on the stack.
  ExpectError({"instruction", "Expected stack to contain 1 value, got 0"},
              errors);
}

TEST_F(ValidateInstructionTest, Catch_IsException) {
  auto global = AddGlobal(GlobalType{VT_Exnref, Mutability::Var});
  Ok(I{O::Try, BT_Void});
  Ok(I{O::Catch});
  Ok(I{O::GlobalSet, global});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Throw) {
  auto type_index = AddFunctionType(FunctionType{{VT_I32, VT_F32}, {}});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::Throw, Index{event_index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Throw_Unreachable) {
  auto type_index = AddFunctionType(FunctionType{{VT_I32}, {}});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::Block, BT_F32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Throw, Index{event_index}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Throw_IndexOOB) {
  Fail(I{O::Throw, Index{0}});
  ExpectError({"instruction", "Invalid event index 0, must be less than 0"},
              errors);
}

TEST_F(ValidateInstructionTest, Throw_TypeMismatch) {
  auto type_index = AddFunctionType(FunctionType{{VT_I32, VT_F32}, {}});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::I64Const, s32{}});
  Fail(I{O::Throw, Index{event_index}});
  ExpectError({"instruction", "Expected stack to contain [i32 f32], got [i64]"},
              errors);
}

TEST_F(ValidateInstructionTest, Rethrow) {
  context.type_stack = {ST::Exnref()};
  Ok(I{O::Rethrow});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Rethrow_Unreachable) {
  Ok(I{O::Try, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Catch});
  Ok(I{O::Rethrow});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Rethrow_Mismatch) {
  Fail(I{O::Rethrow});
  ExpectError({"instruction", "Expected stack to contain [exnref], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, BrOnExn_Void) {
  auto type_index = AddFunctionType(FunctionType{});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::Try, BT_Void});
  Ok(I{O::Catch});
  Ok(I{O::BrOnExn, BrOnExnImmediate{0, event_index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrOnExn_SingleResult) {
  auto type_index = AddFunctionType(FunctionType{{VT_I32}, {}});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Try, BT_Void});
  Ok(I{O::Catch});
  Ok(I{O::BrOnExn, BrOnExnImmediate{1, event_index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrOnExn_MultiResult) {
  auto if_v = AddFunctionType(FunctionType{{VT_I32, VT_F32}, {}});
  auto v_if = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, if_v});
  Ok(I{O::Block, BlockType(v_if)});
  Ok(I{O::Try, BT_Void});
  Ok(I{O::Catch});
  Ok(I{O::BrOnExn, BrOnExnImmediate{1, event_index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrOnExn_ForwardExn) {
  auto type_index = AddFunctionType(FunctionType{{VT_I32}, {}});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Try, BT_Void});
  Ok(I{O::Catch});
  Ok(I{O::BrOnExn, BrOnExnImmediate{1, event_index}});
  Ok(I{O::BrOnExn, BrOnExnImmediate{2, event_index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrOnExn_TypeMismatch) {
  auto type_index = AddFunctionType(FunctionType{});
  auto event_index = AddEvent(EventType{EventAttribute::Exception, type_index});
  Ok(I{O::Block, BT_Void});
  Fail(I{O::BrOnExn, BrOnExnImmediate{0, event_index}});
  ExpectError({"instruction", "Expected stack to contain [exnref], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, Br_Void) {
  Ok(I{O::Br, Index{0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Br_Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Block, info.block_type});
    Ok(info.instruction);
    Ok(I{O::Br, Index{0}});
    Ok(I{O::End});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Br_EmptyStack) {
  Ok(I{O::Block, BT_I32});
  Fail(I{O::Br, Index{0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, Br_FullerStack) {
  Ok(I{O::Block, BT_Void});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Br, Index{0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Br_TypeMismatch) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Br, Index{0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, Br_Depth1) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Block, BT_Void});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Br, Index{1}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Br_ForwardUnreachable) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::Block, BT_F32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Br, Index{1}});
  Ok(I{O::Br, Index{0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Br_Loop_Void) {
  Ok(I{O::Loop, BT_Void});
  Ok(I{O::Br, Index{0}});
  Ok(I{O::End});

  Ok(I{O::Loop, BT_I32});
  Ok(I{O::Br, Index{0}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Br_Loop_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Loop, info.block_type});
    Ok(I{O::Br, Index{0}});
    Ok(I{O::End});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrIf_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrIf, Index{0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrIf_Block_SingleResult) {
  for (const auto& info : all_value_types) {
    Ok(I{O::Block, info.block_type});
    Ok(info.instruction);
    Ok(I{O::I32Const, s32{}});
    Ok(I{O::BrIf, Index{0}});
    Ok(I{O::End});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrIf_NoCondition) {
  Fail(I{O::BrIf, Index{0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, BrIf_ConditionMismatch) {
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::BrIf, Index{0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, BrIf_EmptyStack) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrIf, Index{0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, BrIf_TypeMismatch) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrIf, Index{0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, BrIf_PropagateValue) {
  Ok(I{O::Block, BT_F32});
  Ok(I{O::Block, BT_I32});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrIf, Index{1}});
  Fail(I{O::End});  // F32 is still on the stack.
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, BrIf_Loop_Void) {
  Ok(I{O::Loop, BT_Void});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrIf, Index{0}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
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
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrTable_Void) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{0, 0, 0}, 0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrTable_MultiDepth_Void) {
  Ok(I{O::Block, BT_Void});  // 3
  Ok(I{O::Block, BT_Void});  // 2
  Ok(I{O::Block, BT_Void});  // 1
  Ok(I{O::Block, BT_Void});  // 0
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{0, 1, 2, 3}, 4}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrTable_MultiDepth_SingleResult) {
  Ok(I{O::Block, BT_I32});   // 3
  Ok(I{O::Block, BT_Void});  // 2
  Ok(I{O::Block, BT_I32});   // 1
  Ok(I{O::Block, BT_Void});  // 0
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{1, 1, 1, 3}, 3}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrTable_Unreachable) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{}, 1}});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrTable_NoKey) {
  Fail(I{O::BrTable, BrTableImmediate{{}, 0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, BrTable_EmptyStack) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{}, 0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, BrTable_ValueTypeMismatch) {
  Ok(I{O::Block, BT_I32});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{0}, 0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, BrTable_InconsistentLabelSignature) {
  Ok(I{O::Block, BT_Void});
  Ok(I{O::Block, BT_I32});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::BrTable, BrTableImmediate{{1}, 0}});
  ExpectError({"instruction",
               "br_table labels must have the same signature; expected "
               "[i32], got []"},
              errors);
}

TEST_F(ValidateInstructionTest, BrTable_References) {
  context.features.enable_reference_types();

  Ok(I{O::Block, BT_Externref});
  Ok(I{O::Block, BT_Externref});
  Ok(I{O::Block, BT_Externref});
  Ok(I{O::RefNull, HT_Extern});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::BrTable, BrTableImmediate{{0, 1}, 2}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, BrTable_FunctionReferences) {
  context.features.enable_function_references();

  Ok(I{O::Block, BT_RefNull0});
  Ok(I{O::Block, BT_RefNullFunc});
  Ok(I{O::Block, BT_Ref0});
  TestSignature(I{O::BrTable, BrTableImmediate{{0, 1}, 2}}, {VT_Ref0, VT_I32},
                {});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Return) {
  Ok(I{O::Return});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Return_InsideBlocks) {
  Ok(I{O::Block, BT_Void});
  Ok(I{O::Block, BT_Void});
  Ok(I{O::Block, BT_Void});
  Ok(I{O::Return});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Return_Unreachable) {
  Ok(I{O::Block, BT_F64});
  Ok(I{O::Return});
  Ok(I{O::End});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Return_SingleResult) {
  BeginFunction(FunctionType{{}, {VT_I32}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::Return});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Return_TypeMismatch) {
  BeginFunction(FunctionType{{}, {VT_I32}});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Return});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, Call_Void_Void) {
  auto index = AddFunction(FunctionType{});
  Ok(I{O::Call, Index{index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Call_Params) {
  ValueTypeList param_types{VT_I32, VT_F32};
  ValueTypeList result_types{VT_F64};
  auto index = AddFunction(FunctionType{param_types, result_types});
  TestSignature(I{O::Call, Index{index}}, param_types, result_types);
}

TEST_F(ValidateInstructionTest, Call_FunctionIndexOOB) {
  Fail(I{O::Call, Index{100}});
  ExpectError(
      {"instruction", "Invalid function index 100, must be less than 1"},
      errors);
}

TEST_F(ValidateInstructionTest, Call_TypeIndexOOB) {
  context.functions.push_back(Function{100});
  Index index = context.functions.size() - 1;
  Fail(I{O::Call, Index{index}});
  ExpectError({"instruction", "Invalid type index 100, must be less than 1"},
               errors);
}

TEST_F(ValidateInstructionTest, Call_MultiResult) {
  ValueTypeList param_types{VT_F32};
  ValueTypeList result_types{VT_I32, VT_I32};
  auto index = AddFunction(FunctionType{param_types, result_types});
  TestSignature(I{O::Call, Index{index}}, param_types, result_types);
}

TEST_F(ValidateInstructionTest, CallIndirect) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunctionType(FunctionType{});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, CallIndirect_Params) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunctionType(FunctionType{{VT_F32, VT_I64}, {}});
  TestSignature(I{O::CallIndirect, CallIndirectImmediate{index, 0}},
                {VT_F32, VT_I64, VT_I32}, {});
}

TEST_F(ValidateInstructionTest, CallIndirect_MultiResult) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunctionType(FunctionType{{}, {VT_I64, VT_F32}});
  TestSignature(I{O::CallIndirect, CallIndirectImmediate{index, 0}}, {VT_I32},
                {VT_I64, VT_F32});
}

TEST_F(ValidateInstructionTest, CallIndirect_TableIndexOOB) {
  auto index = AddFunctionType(FunctionType{});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{index, 0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, CallIndirect_TypeIndexOOB) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::CallIndirect, CallIndirectImmediate{100, 0}});
  ExpectError({"instruction", "Invalid type index 100, must be less than 1"},
               errors);
}

TEST_F(ValidateInstructionTest, CallIndirect_NonZeroTableIndex_ReferenceTypes) {
  auto index = AddFunctionType(FunctionType{});
  AddTable(TableType{Limits{0}, RT_Funcref});
  AddTable(TableType{Limits{0}, RT_Funcref});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::CallIndirect, CallIndirectImmediate{index, 1}});
}

TEST_F(ValidateInstructionTest, Drop) {
  for (const auto& info : all_value_types) {
    Ok(info.instruction);
    Ok(I{O::Drop});
  }
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, Drop_EmptyStack) {
  Fail(I{O::Drop});
  ExpectError({"instruction", "Expected stack to contain 1 value, got 0"},
               errors);
}

TEST_F(ValidateInstructionTest, Select) {
  for (const auto& info : all_value_types) {
    TestSignature(I{O::Select}, {info.value_type, info.value_type, VT_I32},
                  {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, Select_EmptyStack) {
  Fail(I{O::Select});
  ExpectErrors({{"instruction", "Expected stack to contain [i32], got []"},
                {"instruction", "Expected stack to have 1 value, got 0"},
                {"instruction", "Expected stack to contain [i32 i32], got []"}},
               errors);
}

TEST_F(ValidateInstructionTest, Select_ConditionTypeMismatch) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::Select});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
               errors);
}

TEST_F(ValidateInstructionTest, Select_InconsistentTypes) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::Select});
  ExpectError(
      {"instruction", "Expected stack to contain [f32 f32], got [i32 f32]"},
      errors);
}

TEST_F(ValidateInstructionTest, Select_ReferenceTypes) {
  // Select can only be used with {i,f}{32,64} value types.
  for (auto stack_type : {ST::Externref(), ST::Funcref()}) {
    context.type_stack = {stack_type, stack_type, ST::I32()};
    Fail(I{O::Select});
    ExpectError(
        {"instruction",
         concat("select instruction without expected type can only be used "
                "with i32, i64, f32, f64; got ",
                stack_type)},
        errors);
  }
}

TEST_F(ValidateInstructionTest, SelectT) {
  for (const auto& vt :
       {VT_I32, VT_I64, VT_F32, VT_F64, VT_Externref, VT_Funcref}) {
    TestSignature(I{O::SelectT, SelectImmediate{vt}}, {vt, vt, VT_I32},
                  {vt});
  }
}

TEST_F(ValidateInstructionTest, SelectT_RefType) {
  TestSignature(I{O::SelectT, SelectImmediate{VT_Ref0}},
                {VT_Ref0, VT_Ref0, VT_I32}, {VT_Ref0});
}

TEST_F(ValidateInstructionTest, SelectT_RefType_IndexOOB) {
  // Use index 1, since index 0 is always defined (see AddFunction above).
  FailWithTypeStack(I{O::SelectT, SelectImmediate{VT_Ref1}},
                    ValueTypeList{VT_Ref1, VT_Ref1, VT_I32});
}

TEST_F(ValidateInstructionTest, SelectT_EmptyStack) {
  Fail(I{O::SelectT, SelectImmediate{VT_I64}});
  ExpectErrors({{"instruction", "Expected stack to contain [i32], got []"},
                {"instruction", "Expected stack to contain [i64 i64], got []"}},
               errors);
}

TEST_F(ValidateInstructionTest, SelectT_ConditionTypeMismatch) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::SelectT, SelectImmediate{VT_I32}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
              errors);
}

TEST_F(ValidateInstructionTest, LocalGet) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    TestSignature(I{O::LocalGet, Index{index}}, {}, {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, LocalGet_Param) {
  BeginFunction(FunctionType{{VT_I32, VT_F32}, {}});
  auto index = AddLocal(VT_I64);
  EXPECT_EQ(2, index);
  Ok(I{O::LocalGet, Index{0}}); // 1st param.
  Ok(I{O::LocalGet, Index{1}}); // 2nd param.
  Ok(I{O::LocalGet, Index{2}}); // 1st local.
  Fail(I{O::LocalGet, Index{3}}); // Invalid.
  ExpectError({"instruction", "Invalid local index 3, must be less than 3"},
               errors);
}

TEST_F(ValidateInstructionTest, LocalGet_IndexOOB) {
  Fail(I{O::LocalGet, Index{100}});
  ExpectError(
      {"instruction", "Invalid local index 100, must be less than 0"},
      errors);
}

TEST_F(ValidateInstructionTest, LocalSet) {
  for (const auto& info : all_value_types) {
    auto index = AddLocal(info.value_type);
    TestSignature(I{O::LocalSet, Index{index}}, {info.value_type}, {});
  }
}

TEST_F(ValidateInstructionTest, LocalSet_Param) {
  BeginFunction(FunctionType{{VT_I32, VT_F32}, {}});
  auto index = AddLocal(VT_F64);
  EXPECT_EQ(2, index);
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::LocalSet, Index{0}}); // 1st param.
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::LocalSet, Index{1}}); // 2nd param.
  Ok(I{O::F64Const, f64{}});
  Ok(I{O::LocalSet, Index{2}}); // 1st local.
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::LocalSet, Index{3}}); // Invalid.
  ExpectError({"instruction", "Invalid local index 3, must be less than 3"},
               errors);
}

TEST_F(ValidateInstructionTest, LocalSet_IndexOOB) {
  Fail(I{O::LocalSet, Index{100}});
  ExpectErrors({{"instruction", "Invalid local index 100, must be less than 0"},
                {"instruction", "Expected stack to contain [i32], got []"}},
               errors);
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
  ExpectErrors({{"instruction", "Invalid local index 100, must be less than 0"},
                {"instruction", "Expected stack to contain [i32], got []"}},
               errors);
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
  ExpectError(
      {"instruction", "Invalid global index 100, must be less than 0"},
      errors);
}

TEST_F(ValidateInstructionTest, GlobalSet) {
  for (const auto& info : all_value_types) {
    auto index = AddGlobal(GlobalType{info.value_type, Mutability::Var});
    TestSignature(I{O::GlobalSet, Index{index}}, {info.value_type}, {});
  }
}

TEST_F(ValidateInstructionTest, GlobalSet_IndexOOB) {
  Fail(I{O::GlobalSet, Index{100}});
  ExpectErrors(
      {{"instruction", "Invalid global index 100, must be less than 0"},
       {"instruction", "global.set is invalid on immutable global 100"},
       {"instruction", "Expected stack to contain [i32], got []"}},
      errors);
}

TEST_F(ValidateInstructionTest, GlobalSet_Immutable) {
  auto index = AddGlobal(GlobalType{VT_I32, Mutability::Const});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::GlobalSet, Index{index}});
  ExpectError({"instruction", "global.set is invalid on immutable global 0"},
               errors);
}

TEST_F(ValidateInstructionTest, TableGet_Funcref) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableGet, Index{0}}, {VT_I32}, {VT_Funcref});
}

TEST_F(ValidateInstructionTest, TableGet_Externref) {
  AddTable(TableType{Limits{0}, RT_Externref});
  TestSignature(I{O::TableGet, Index{0}}, {VT_I32}, {VT_Externref});
}

TEST_F(ValidateInstructionTest, TableGet_IndexOOB) {
  context.type_stack = {ST::I32()};
  Fail(I{O::TableGet, Index{0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, TableSet_Funcref) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableSet, Index{0}}, {VT_I32, VT_Funcref}, {});
}

TEST_F(ValidateInstructionTest, TableSet_Externref) {
  AddTable(TableType{Limits{0}, RT_Externref});
  TestSignature(I{O::TableSet, Index{0}}, {VT_I32, VT_Externref}, {});
}

TEST_F(ValidateInstructionTest, TableSet_IndexOOB) {
  context.type_stack = {ST::I32(), ST::Funcref()};
  Fail(I{O::TableSet, Index{0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
              errors);
}

TEST_F(ValidateInstructionTest, TableSet_InvalidType) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  context.type_stack = {ST::I32(), ST::Externref()};
  Fail(I{O::TableSet, Index{0}});
  ExpectError({"instruction",
               "Expected stack to contain [i32 funcref], got [i32 externref]"},
              errors);
}

TEST_F(ValidateInstructionTest, Load) {
  const struct {
    Opcode opcode;
    ValueType result;
  } infos[] = {
      {O::I32Load, VT_I32},    {O::I32Load8S, VT_I32},  {O::I32Load8U, VT_I32},
      {O::I32Load16S, VT_I32}, {O::I32Load16U, VT_I32}, {O::I64Load, VT_I64},
      {O::I64Load8S, VT_I64},  {O::I64Load8U, VT_I64},  {O::I64Load16S, VT_I64},
      {O::I64Load16U, VT_I64}, {O::I64Load32S, VT_I64}, {O::I64Load32U, VT_I64},
      {O::F32Load, VT_F32},    {O::F64Load, VT_F64}};

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{0, 0}}, {VT_I32},
                  {info.result});
  }
}

TEST_F(ValidateInstructionTest, Load_Alignment) {
  struct {
    Opcode opcode;
    u32 max_align;
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
    ExpectErrorSubstr({"instruction", "Invalid alignment"}, errors);
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
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                errors);
  }
}

TEST_F(ValidateInstructionTest, Store) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {
      {O::I32Store, VT_I32},   {O::I32Store8, VT_I32}, {O::I32Store16, VT_I32},
      {O::I64Store, VT_I64},   {O::I64Store8, VT_I64}, {O::I64Store16, VT_I64},
      {O::I64Store32, VT_I64}, {O::F32Store, VT_F32},  {O::F64Store, VT_F64}};

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{0, 0}},
                  {VT_I32, info.value_type}, {});
  }
}

TEST_F(ValidateInstructionTest, Store_MemoryOOB) {
  const Opcode opcodes[] = {O::I32Store,  O::I64Store,   O::F32Store,
                            O::F64Store,  O::I32Store8,  O::I32Store16,
                            O::I64Store8, O::I64Store16, O::I64Store32};

  Ok(I{O::Unreachable});
  for (const auto& opcode : opcodes) {
    Fail(I{opcode, MemArgImmediate{0, 0}});
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                errors);
  }
}

TEST_F(ValidateInstructionTest, Store_Alignment) {
  struct {
    Opcode opcode;
    u32 max_align;
  } const infos[] = {{O::I32Store, 2},  {O::I64Store, 3},   {O::F32Store, 2},
                     {O::F64Store, 3},  {O::I32Store8, 0},  {O::I32Store16, 1},
                     {O::I64Store8, 0}, {O::I64Store16, 1}, {O::I64Store32, 2}};

  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::Unreachable});
  for (const auto& info : infos) {
    Ok(I{info.opcode, MemArgImmediate{info.max_align, 0}});
    Fail(I{info.opcode, MemArgImmediate{info.max_align + 1, 0}});
    ExpectErrorSubstr({"instruction", "Invalid alignment"}, errors);
  }
}

TEST_F(ValidateInstructionTest, MemorySize) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemorySize, u8{}}, {}, {VT_I32});
}

TEST_F(ValidateInstructionTest, MemorySize_MemoryIndexOOB) {
  Fail(I{O::MemorySize, u8{}});
  ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, MemoryGrow) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryGrow, u8{}}, {VT_I32}, {VT_I32});
}

TEST_F(ValidateInstructionTest, MemoryGrow_MemoryIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryGrow, u8{}});
  ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, Unary) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {
      {O::I32Eqz, VT_I32},     {O::I32Clz, VT_I32},     {O::I32Ctz, VT_I32},
      {O::I32Popcnt, VT_I32},  {O::I64Clz, VT_I64},     {O::I64Ctz, VT_I64},
      {O::I64Popcnt, VT_I64},  {O::F32Abs, VT_F32},     {O::F32Neg, VT_F32},
      {O::F32Ceil, VT_F32},    {O::F32Floor, VT_F32},   {O::F32Trunc, VT_F32},
      {O::F32Nearest, VT_F32}, {O::F32Sqrt, VT_F32},    {O::F64Abs, VT_F64},
      {O::F64Neg, VT_F64},     {O::F64Ceil, VT_F64},    {O::F64Floor, VT_F64},
      {O::F64Trunc, VT_F64},   {O::F64Nearest, VT_F64}, {O::F64Sqrt, VT_F64},
  };

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type}, {info.value_type});
  }

  TestSignature(I{O::I64Eqz}, {VT_I64}, {VT_I32});
}

TEST_F(ValidateInstructionTest, Binary) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {
      {O::I32Add, VT_I32},      {O::I32Sub, VT_I32},     {O::I32Mul, VT_I32},
      {O::I32DivS, VT_I32},     {O::I32DivU, VT_I32},    {O::I32RemS, VT_I32},
      {O::I32RemU, VT_I32},     {O::I32And, VT_I32},     {O::I32Or, VT_I32},
      {O::I32Xor, VT_I32},      {O::I32Shl, VT_I32},     {O::I32ShrS, VT_I32},
      {O::I32ShrU, VT_I32},     {O::I32Rotl, VT_I32},    {O::I32Rotr, VT_I32},
      {O::I64Add, VT_I64},      {O::I64Sub, VT_I64},     {O::I64Mul, VT_I64},
      {O::I64DivS, VT_I64},     {O::I64DivU, VT_I64},    {O::I64RemS, VT_I64},
      {O::I64RemU, VT_I64},     {O::I64And, VT_I64},     {O::I64Or, VT_I64},
      {O::I64Xor, VT_I64},      {O::I64Shl, VT_I64},     {O::I64ShrS, VT_I64},
      {O::I64ShrU, VT_I64},     {O::I64Rotl, VT_I64},    {O::I64Rotr, VT_I64},
      {O::F32Add, VT_F32},      {O::F32Sub, VT_F32},     {O::F32Mul, VT_F32},
      {O::F32Div, VT_F32},      {O::F32Min, VT_F32},     {O::F32Max, VT_F32},
      {O::F32Copysign, VT_F32}, {O::F64Add, VT_F64},     {O::F64Sub, VT_F64},
      {O::F64Mul, VT_F64},      {O::F64Div, VT_F64},     {O::F64Min, VT_F64},
      {O::F64Max, VT_F64},      {O::F64Copysign, VT_F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type, info.value_type},
                  {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, Compare) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I32Eq, VT_I32},  {O::I32Ne, VT_I32},  {O::I32LtS, VT_I32},
               {O::I32LtU, VT_I32}, {O::I32GtS, VT_I32}, {O::I32GtU, VT_I32},
               {O::I32LeS, VT_I32}, {O::I32LeU, VT_I32}, {O::I32GeS, VT_I32},
               {O::I32GeU, VT_I32}, {O::I64Eq, VT_I64},  {O::I64Ne, VT_I64},
               {O::I64LtS, VT_I64}, {O::I64LtU, VT_I64}, {O::I64GtS, VT_I64},
               {O::I64GtU, VT_I64}, {O::I64LeS, VT_I64}, {O::I64LeU, VT_I64},
               {O::I64GeS, VT_I64}, {O::I64GeU, VT_I64}, {O::F32Eq, VT_F32},
               {O::F32Ne, VT_F32},  {O::F32Lt, VT_F32},  {O::F32Gt, VT_F32},
               {O::F32Le, VT_F32},  {O::F32Ge, VT_F32},  {O::F64Eq, VT_F64},
               {O::F64Ne, VT_F64},  {O::F64Lt, VT_F64},  {O::F64Gt, VT_F64},
               {O::F64Le, VT_F64},  {O::F64Ge, VT_F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type, info.value_type},
                  {VT_I32});
  }
}

TEST_F(ValidateInstructionTest, Conversion) {
  const struct {
    Opcode opcode;
    ValueType to;
    ValueType from;
  } infos[] = {{O::I32WrapI64, VT_I32, VT_I64},
               {O::I32TruncF32S, VT_I32, VT_F32},
               {O::I32TruncF32U, VT_I32, VT_F32},
               {O::I32ReinterpretF32, VT_I32, VT_F32},
               {O::I32TruncF64S, VT_I32, VT_F64},
               {O::I32TruncF64U, VT_I32, VT_F64},
               {O::I64ExtendI32S, VT_I64, VT_I32},
               {O::I64ExtendI32U, VT_I64, VT_I32},
               {O::I64TruncF32S, VT_I64, VT_F32},
               {O::I64TruncF32U, VT_I64, VT_F32},
               {O::I64TruncF64S, VT_I64, VT_F64},
               {O::I64TruncF64U, VT_I64, VT_F64},
               {O::I64ReinterpretF64, VT_I64, VT_F64},
               {O::F32ConvertI32S, VT_F32, VT_I32},
               {O::F32ConvertI32U, VT_F32, VT_I32},
               {O::F32ReinterpretI32, VT_F32, VT_I32},
               {O::F32ConvertI64S, VT_F32, VT_I64},
               {O::F32ConvertI64U, VT_F32, VT_I64},
               {O::F32DemoteF64, VT_F32, VT_F64},
               {O::F64ConvertI32S, VT_F64, VT_I32},
               {O::F64ConvertI32U, VT_F64, VT_I32},
               {O::F64ConvertI64S, VT_F64, VT_I64},
               {O::F64ConvertI64U, VT_F64, VT_I64},
               {O::F64ReinterpretI64, VT_F64, VT_I64},
               {O::F64PromoteF32, VT_F64, VT_F32}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.from}, {info.to});
  }
}

TEST_F(ValidateInstructionTest, ReturnCall) {
  BeginFunction(FunctionType{{}, {VT_F32}});
  auto index = AddFunction(FunctionType{{VT_I32}, {VT_F32}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::ReturnCall, Index{index}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, ReturnCall_Unreachable) {
  BeginFunction(FunctionType{{}, {}});
  auto index = AddFunction(FunctionType{{VT_I32}, {}});

  Ok(I{O::Block, BT_I32});
  Ok(I{O::F64Const, f64{}});  // Extra value on stack is ok.

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::ReturnCall, Index{index}});

  Ok(I{O::End});  // Stack is polymorphic. F64 was dropped, so I32 result is OK.
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, ReturnCall_FunctionIndexOOB) {
  Index index = context.functions.size();
  Fail(I{O::ReturnCall, Index{index}});
  ExpectError(
      {"instruction", "Invalid function index 1, must be less than 1"},
      errors);
}

TEST_F(ValidateInstructionTest, ReturnCall_TypeIndexOOB) {
  context.functions.push_back(Function{100});
  Index index = context.functions.size() - 1;
  Fail(I{O::ReturnCall, Index{index}});
  ExpectError({"instruction", "Invalid type index 100, must be less than 1"},
               errors);
}

TEST_F(ValidateInstructionTest, ReturnCall_ParamTypeMismatch) {
  auto index = AddFunction(FunctionType{{VT_I32}, {}});
  Ok(I{O::F32Const, f32{}});
  Fail(I{O::ReturnCall, Index{index}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
               errors);
}

TEST_F(ValidateInstructionTest, ReturnCall_ResultTypeMismatch) {
  BeginFunction(FunctionType{{}, {VT_F32}});
  auto index = AddFunction(FunctionType{{}, {VT_I32}});
  Fail(I{O::ReturnCall, Index{index}});
  ExpectError(
      {"instruction",
      "Callee's result types [f32] must equal caller's result types [i32]"},
      errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect) {
  BeginFunction(FunctionType{{}, {VT_F32}});
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunctionType(FunctionType{{VT_I64}, {VT_F32}});
  Ok(I{O::I64Const, s64{}});  // Param.
  Ok(I{O::I32Const, s32{}});  // call_indirect key.
  Ok(I{O::ReturnCallIndirect, CallIndirectImmediate{index, 0}});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect_Unreachable) {
  BeginFunction(FunctionType{{}, {}});
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunctionType(FunctionType{{VT_I64}, {}});

  Ok(I{O::Block, BT_I32});
  Ok(I{O::F64Const, f64{}});  // Extra value on stack is ok.

  Ok(I{O::I64Const, s64{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::ReturnCallIndirect, CallIndirectImmediate{index, 0}});

  Ok(I{O::End});  // Stack is polymorphic. F64 was dropped, so I32 result is OK.
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect_TableIndexOOB) {
  auto index = AddFunction(FunctionType{{VT_I32}, {}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::ReturnCallIndirect, CallIndirectImmediate{index, 0}});
  ExpectErrors({{"instruction", "Invalid table index 0, must be less than 0"},
                {"instruction", "Expected stack to contain [i32], got []"}},
               errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect_TypeIndexOOB) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::ReturnCallIndirect, CallIndirectImmediate{100, 0}});
  ExpectError({"instruction", "Invalid type index 100, must be less than 1"},
               errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect_NoKey) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunction(FunctionType{{}, {}});
  Fail(I{O::ReturnCallIndirect, CallIndirectImmediate{index, 0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got []"},
               errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect_ParamTypeMismatch) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunction(FunctionType{{VT_I32}, {}});
  Ok(I{O::F32Const, f32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::ReturnCallIndirect, CallIndirectImmediate{index, 0}});
  ExpectError({"instruction", "Expected stack to contain [i32], got [f32]"},
               errors);
}

TEST_F(ValidateInstructionTest, ReturnCallIndirect_ResultTypeMismatch) {
  BeginFunction(FunctionType{{}, {VT_F32}});
  AddTable(TableType{Limits{0}, RT_Funcref});
  auto index = AddFunction(FunctionType{{}, {VT_I32}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::ReturnCallIndirect, CallIndirectImmediate{index, 0}});
  ExpectError(
      {"instruction",
      "Callee's result types [f32] must equal caller's result types [i32]"},
      errors);
}

TEST_F(ValidateInstructionTest, SignExtension) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I32Extend8S, VT_I32},
               {O::I32Extend16S, VT_I32},
               {O::I64Extend8S, VT_I64},
               {O::I64Extend16S, VT_I64},
               {O::I64Extend32S, VT_I64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type}, {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, ReferenceTypes) {
  auto RefNullExtern =
      VT{ReferenceType{RefType{HeapType{HeapKind::Extern}, Null::Yes}}};
  auto RefNullFunc =
      VT{ReferenceType{RefType{HeapType{HeapKind::Func}, Null::Yes}}};
  TestSignature(I{O::RefNull, HT_Extern}, {}, {RefNullExtern});
  TestSignature(I{O::RefNull, HT_Func}, {}, {RefNullFunc});
  TestSignature(I{O::RefIsNull}, {VT_Externref}, {VT_I32});
  TestSignature(I{O::RefIsNull}, {VT_Funcref}, {VT_I32});
}

TEST_F(ValidateInstructionTest, RefFunc) {
  context.declared_functions.insert(0);
  TestSignature(I{O::RefFunc, Index{0}}, {}, {VT_Ref0});
}

TEST_F(ValidateInstructionTest, RefFunc_UndeclaredFunctionReference) {
  Fail(I{O::RefFunc, Index{0}});
  ExpectError({"instruction", "Undeclared function reference 0"}, errors);
}

TEST_F(ValidateInstructionTest, SaturatingFloatToInt) {
  const struct {
    Opcode opcode;
    ValueType to;
    ValueType from;
  } infos[] = {{O::I32TruncSatF32S, VT_I32, VT_F32},
               {O::I32TruncSatF32U, VT_I32, VT_F32},
               {O::I32TruncSatF64S, VT_I32, VT_F64},
               {O::I32TruncSatF64U, VT_I32, VT_F64},
               {O::I64TruncSatF32S, VT_I64, VT_F32},
               {O::I64TruncSatF32U, VT_I64, VT_F32},
               {O::I64TruncSatF64S, VT_I64, VT_F64},
               {O::I64TruncSatF64U, VT_I64, VT_F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.from}, {info.to});
  }
}

TEST_F(ValidateInstructionTest, MemoryInit) {
  context.declared_data_count = 2;
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryInit, InitImmediate{1, 0}},
                {VT_I32, VT_I32, VT_I32}, {});
}

TEST_F(ValidateInstructionTest, MemoryInit_MemoryIndexOOB) {
  context.declared_data_count = 2;
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryInit, InitImmediate{1, 0}});
  ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, MemoryInit_SegmentIndexOOB) {
  context.declared_data_count = 2;
  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryInit, InitImmediate{2, 0}});
  ExpectError(
      {"instruction", "Invalid data segment index 2, must be less than 2"},
      errors);
}

TEST_F(ValidateInstructionTest, DataDrop) {
  context.declared_data_count = 2;
  TestSignature(I{O::DataDrop, Index{1}}, {}, {});
}

TEST_F(ValidateInstructionTest, DataDrop_SegmentIndexOOB) {
  context.declared_data_count = 2;
  Fail(I{O::DataDrop, Index{2}});
  ExpectError(
      {"instruction", "Invalid data segment index 2, must be less than 2"},
      errors);
}

TEST_F(ValidateInstructionTest, MemoryCopy) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryCopy, CopyImmediate{0, 0}}, {VT_I32, VT_I32, VT_I32},
                {});
}

TEST_F(ValidateInstructionTest, MemoryCopy_MemoryIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryCopy, CopyImmediate{0, 0}});
  ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, MemoryFill) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::MemoryFill, u8{0}}, {VT_I32, VT_I32, VT_I32}, {});
}

TEST_F(ValidateInstructionTest, MemoryFill_MemoryIndexOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::MemoryFill, u8{0}});
  ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, TableInit) {
  auto index = AddElementSegment(RT_Funcref);
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableInit, InitImmediate{index, 0}},
                {VT_I32, VT_I32, VT_I32}, {});
}

TEST_F(ValidateInstructionTest, TableInit_TypeMismatch) {
  auto index = AddElementSegment(RT_Externref);
  AddTable(TableType{Limits{0}, RT_Funcref});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableInit, InitImmediate{index, 0}});
  ExpectError({"instruction", "Expected reference type funcref, got externref"},
              errors);
}

TEST_F(ValidateInstructionTest, TableInit_TableIndexOOB) {
  auto index = AddElementSegment(RT_Funcref);
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableInit, InitImmediate{index, 0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, TableInit_SegmentIndexOOB) {
  AddTable(TableType{Limits{0}, RT_Externref});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableInit, InitImmediate{0, 0}});
  ExpectError(
      {"instruction", "Invalid element segment index 0, must be less than 0"},
      errors);
}

TEST_F(ValidateInstructionTest, ElemDrop) {
  auto index = AddElementSegment(RT_Funcref);
  TestSignature(I{O::ElemDrop, Index{index}}, {}, {});
}

TEST_F(ValidateInstructionTest, ElemDrop_SegmentIndexOOB) {
  Fail(I{O::ElemDrop, Index{0}});
  ExpectError(
      {"instruction", "Invalid element segment index 0, must be less than 0"},
      errors);
}

TEST_F(ValidateInstructionTest, TableCopy) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableCopy, CopyImmediate{0, 0}}, {VT_I32, VT_I32, VT_I32},
                {});
}

TEST_F(ValidateInstructionTest, TableCopy_TypeMismatch) {
  auto dst_index = AddTable(TableType{Limits{0}, RT_Funcref});
  auto src_index = AddTable(TableType{Limits{0}, RT_Externref});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableCopy, CopyImmediate{dst_index, src_index}});
  ExpectError({"instruction", "Expected reference type funcref, got externref"},
              errors);
}

TEST_F(ValidateInstructionTest, TableCopy_TableIndexOOB) {
  auto index = AddTable(TableType{Limits{0}, RT_Funcref});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::I32Const, s32{}});
  Fail(I{O::TableCopy, CopyImmediate{index, index + 1}});
  ExpectError({"instruction", "Invalid table index 1, must be less than 1"},
              errors);
}

TEST_F(ValidateInstructionTest, TableGrow) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableGrow, Index{0}}, {VT_Funcref, VT_I32}, {VT_I32});
}

TEST_F(ValidateInstructionTest, TableGrow_TableIndexOOB) {
  context.type_stack = StackTypeList{ST::Funcref(), ST::I32()};
  Fail(I{O::TableGrow, Index{0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, TableSize) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableSize, Index{0}}, {}, {VT_I32});
}

TEST_F(ValidateInstructionTest, TableSize_TableIndexOOB) {
  Fail(I{O::TableSize, Index{0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, TableFill) {
  AddTable(TableType{Limits{0}, RT_Funcref});
  TestSignature(I{O::TableFill, Index{0}}, {VT_I32, VT_Funcref, VT_I32}, {});
}

TEST_F(ValidateInstructionTest, TableFill_TableIndexOOB) {
  context.type_stack = StackTypeList{ST::I32(), ST::Funcref(), ST::I32()};
  Fail(I{O::TableFill, Index{0}});
  ExpectError({"instruction", "Invalid table index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, SimdLoad) {
  const Opcode opcodes[] = {
      O::V128Load,        O::V128Load8Splat,  O::V128Load16Splat,
      O::V128Load32Splat, O::V128Load64Splat, O::V128Load8X8S,
      O::V128Load8X8U,    O::V128Load16X4S,   O::V128Load16X4U,
      O::V128Load32X2S,   O::V128Load32X2U,   O::V128Load32Zero,
      O::V128Load64Zero,
  };

  AddMemory(MemoryType{Limits{0}});
  for (const auto& opcode : opcodes) {
    TestSignature(I{opcode, MemArgImmediate{0, 0}}, {VT_I32}, {VT_V128});
  }
}

TEST_F(ValidateInstructionTest, SimdLoad_Alignment) {
  struct {
    Opcode opcode;
    u32 max_align;
  } const infos[] = {
      {O::V128Load, 4},        {O::V128Load8Splat, 0},  {O::V128Load16Splat, 1},
      {O::V128Load32Splat, 2}, {O::V128Load64Splat, 3}, {O::V128Load8X8S, 3},
      {O::V128Load8X8U, 3},    {O::V128Load16X4S, 3},   {O::V128Load16X4U, 3},
      {O::V128Load32X2S, 3},   {O::V128Load32X2U, 3},   {O::V128Load32Zero, 2},
      {O::V128Load64Zero, 3},
  };

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info : infos) {
    Ok(I{O::I32Const, s32{}});
    Ok(I{info.opcode, MemArgImmediate{info.max_align, 0}});
    Ok(I{O::I32Const, s32{}});
    Fail(I{info.opcode, MemArgImmediate{info.max_align + 1, 0}});
    ExpectErrorSubstr({"instruction", "Invalid alignment"}, errors);
  }
}

TEST_F(ValidateInstructionTest, SimdLoad_MemoryOOB) {
  const Opcode opcodes[] = {
      O::V128Load,        O::V128Load8Splat,  O::V128Load16Splat,
      O::V128Load32Splat, O::V128Load64Splat, O::V128Load8X8S,
      O::V128Load8X8U,    O::V128Load16X4S,   O::V128Load16X4U,
      O::V128Load32X2S,   O::V128Load32X2U,   O::V128Load32Zero,
      O::V128Load64Zero,
  };

  for (const auto& opcode : opcodes) {
    Ok(I{O::I32Const, s32{}});
    Fail(I{opcode, MemArgImmediate{0, 0}});
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                 errors);
  }
}

TEST_F(ValidateInstructionTest, SimdStore) {
  AddMemory(MemoryType{Limits{0}});
  TestSignature(I{O::V128Store, MemArgImmediate{0, 0}}, {VT_I32, VT_V128}, {});
}

TEST_F(ValidateInstructionTest, SimdStore_Alignment) {
  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::Unreachable});
  Ok(I{O::V128Store, MemArgImmediate{4, 0}});
  Fail(I{O::V128Store, MemArgImmediate{5, 0}});
  ExpectError(
      {"instruction", "Invalid alignment v128.store {align 5, offset 0}"},
      errors);
}

TEST_F(ValidateInstructionTest, SimdStore_MemoryOOB) {
  Ok(I{O::I32Const, s32{}});
  Ok(I{O::V128Const, v128{}});
  Fail(I{O::V128Store, MemArgImmediate{0, 0}});
  ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
               errors);
}

TEST_F(ValidateInstructionTest, SimdConst) {
  TestSignature(I{O::V128Const}, {}, {VT_V128});
}

TEST_F(ValidateInstructionTest, SimdBitSelect) {
  TestSignature(I{O::V128BitSelect}, {VT_V128, VT_V128, VT_V128}, {VT_V128});
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
      O::F32X4Ceil,
      O::F32X4Floor,
      O::F32X4Trunc,
      O::F32X4Nearest,
      O::F64X2Abs,
      O::F64X2Neg,
      O::F64X2Sqrt,
      O::F64X2Ceil,
      O::F64X2Floor,
      O::F64X2Trunc,
      O::F64X2Nearest,
      O::I32X4TruncSatF32X4S,
      O::I32X4TruncSatF32X4U,
      O::F32X4ConvertI32X4S,
      O::F32X4ConvertI32X4U,
      O::I16X8WidenLowI8X16S,
      O::I16X8WidenHighI8X16S,
      O::I16X8WidenLowI8X16U,
      O::I16X8WidenHighI8X16U,
      O::I32X4WidenLowI16X8S,
      O::I32X4WidenHighI16X8S,
      O::I32X4WidenLowI16X8U,
      O::I32X4WidenHighI16X8U,
      O::I8X16Abs,
      O::I16X8Abs,
      O::I32X4Abs,
  };

  for (const auto& opcode: opcodes) {
    TestSignature(I{opcode}, {VT_V128}, {VT_V128});
  }
}

TEST_F(ValidateInstructionTest, SimdBinary) {
  const Opcode opcodes[] = {
      O::I8X16Eq,           O::I8X16Ne,
      O::I8X16LtS,          O::I8X16LtU,
      O::I8X16GtS,          O::I8X16GtU,
      O::I8X16LeS,          O::I8X16LeU,
      O::I8X16GeS,          O::I8X16GeU,
      O::I16X8Eq,           O::I16X8Ne,
      O::I16X8LtS,          O::I16X8LtU,
      O::I16X8GtS,          O::I16X8GtU,
      O::I16X8LeS,          O::I16X8LeU,
      O::I16X8GeS,          O::I16X8GeU,
      O::I32X4Eq,           O::I32X4Ne,
      O::I32X4LtS,          O::I32X4LtU,
      O::I32X4GtS,          O::I32X4GtU,
      O::I32X4LeS,          O::I32X4LeU,
      O::I32X4GeS,          O::I32X4GeU,
      O::F32X4Eq,           O::F32X4Ne,
      O::F32X4Lt,           O::F32X4Gt,
      O::F32X4Le,           O::F32X4Ge,
      O::F64X2Eq,           O::F64X2Ne,
      O::F64X2Lt,           O::F64X2Gt,
      O::F64X2Le,           O::F64X2Ge,
      O::V128And,           O::V128Or,
      O::V128Xor,           O::I8X16Add,
      O::I8X16AddSatS,      O::I8X16AddSatU,
      O::I8X16Sub,          O::I8X16SubSatS,
      O::I8X16SubSatU,      O::I8X16MinS,
      O::I8X16MinU,         O::I8X16MaxS,
      O::I8X16MaxU,         O::I16X8Add,
      O::I16X8AddSatS,      O::I16X8AddSatU,
      O::I16X8Sub,          O::I16X8SubSatS,
      O::I16X8SubSatU,      O::I16X8Mul,
      O::I16X8MinS,         O::I16X8MinU,
      O::I16X8MaxS,         O::I16X8MaxU,
      O::I32X4Add,          O::I32X4Sub,
      O::I32X4Mul,          O::I32X4MinS,
      O::I32X4MinU,         O::I32X4MaxS,
      O::I32X4MaxU,         O::I32X4DotI16X8S,
      O::I64X2Add,          O::I64X2Sub,
      O::I64X2Mul,          O::F32X4Add,
      O::F32X4Sub,          O::F32X4Mul,
      O::F32X4Div,          O::F32X4Min,
      O::F32X4Max,          O::F32X4Pmin,
      O::F32X4Pmax,         O::F64X2Add,
      O::F64X2Sub,          O::F64X2Mul,
      O::F64X2Div,          O::F64X2Min,
      O::F64X2Max,          O::F64X2Pmin,
      O::F64X2Pmax,         O::I8X16Swizzle,
      O::I8X16NarrowI16X8S, O::I8X16NarrowI16X8U,
      O::I16X8NarrowI32X4S, O::I16X8NarrowI32X4U,
      O::V128Andnot,        O::I8X16AvgrU,
      O::I16X8AvgrU,
  };

  for (const auto& opcode: opcodes) {
    TestSignature(I{opcode}, {VT_V128, VT_V128}, {VT_V128});
  }
}

TEST_F(ValidateInstructionTest, SimdShuffle) {
  TestSignature(I{O::I8X16Shuffle, ShuffleImmediate{}}, {VT_V128, VT_V128},
                {VT_V128});
}

TEST_F(ValidateInstructionTest, SimdShuffle_ValidLane) {
  // Test valid indexes.
  context.type_stack = StackTypeList{ST::V128(), ST::V128()};
  Ok(I{O::I8X16Shuffle, ShuffleImmediate{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15}}});

  // 16 through 31 is also allowed.
  context.type_stack = StackTypeList{ST::V128(), ST::V128()};
  Ok(I{O::I8X16Shuffle, ShuffleImmediate{{16, 17, 18, 19, 20, 21, 22, 23, 24,
                                          25, 26, 27, 28, 29, 30, 31}}});

  // >= 32 is not allowed.
  context.type_stack = StackTypeList{ST::V128(), ST::V128()};
  Fail(I{O::I8X16Shuffle,
         ShuffleImmediate{{32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}});
}

TEST_F(ValidateInstructionTest, SimdBoolean) {
  const Opcode opcodes[] = {
      O::I8X16AnyTrue, O::I8X16AllTrue, O::I8X16Bitmask,
      O::I16X8AnyTrue, O::I16X8AllTrue, O::I16X8Bitmask,
      O::I32X4AnyTrue, O::I32X4AllTrue, O::I32X4Bitmask,
  };

  for (const auto& opcode : opcodes) {
    TestSignature(I{opcode}, {VT_V128}, {VT_I32});
  }
}

TEST_F(ValidateInstructionTest, SimdSplats) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I8X16Splat, VT_I32}, {O::I16X8Splat, VT_I32},
               {O::I32X4Splat, VT_I32}, {O::I64X2Splat, VT_I64},
               {O::F32X4Splat, VT_F32}, {O::F64X2Splat, VT_F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode}, {info.value_type}, {VT_V128});
  }
}

TEST_F(ValidateInstructionTest, SimdExtractLanes) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I8X16ExtractLaneS, VT_I32}, {O::I8X16ExtractLaneU, VT_I32},
               {O::I16X8ExtractLaneS, VT_I32}, {O::I16X8ExtractLaneU, VT_I32},
               {O::I32X4ExtractLane, VT_I32},  {O::I64X2ExtractLane, VT_I64},
               {O::F32X4ExtractLane, VT_F32},  {O::F64X2ExtractLane, VT_F64}};

  for (const auto& info: infos) {
    TestSignature(I{info.opcode, SimdLaneImmediate{0}}, {VT_V128},
                  {info.value_type});
  }
}

TEST_F(ValidateInstructionTest, SimdExtractLanes_ValidLane) {
  const struct {
    Opcode opcode;
    SimdLaneImmediate max_valid_lane;
  } infos[] = {{O::I8X16ExtractLaneS, 15}, {O::I8X16ExtractLaneU, 15},
               {O::I16X8ExtractLaneS, 7}, {O::I16X8ExtractLaneU, 7},
               {O::I32X4ExtractLane, 3},  {O::I64X2ExtractLane, 1},
               {O::F32X4ExtractLane, 3},  {O::F64X2ExtractLane, 1}};

  for (const auto& info: infos) {
    // Test valid indexes.
    for (SimdLaneImmediate imm = 0; imm < info.max_valid_lane; ++imm) {
      context.type_stack = StackTypeList{ST::V128()};
      Ok(I{info.opcode, imm});
    }

    // Test invalid indexes.
    context.type_stack = StackTypeList{ST::V128()};
    Fail(I{info.opcode, SimdLaneImmediate(info.max_valid_lane + 1)});
  }
}

TEST_F(ValidateInstructionTest, SimdReplaceLanes) {
  const struct {
    Opcode opcode;
    ValueType value_type;
  } infos[] = {{O::I8X16ReplaceLane, VT_I32}, {O::I16X8ReplaceLane, VT_I32},
               {O::I32X4ReplaceLane, VT_I32}, {O::I64X2ReplaceLane, VT_I64},
               {O::F32X4ReplaceLane, VT_F32}, {O::F64X2ReplaceLane, VT_F64}};

  for (const auto& info : infos) {
    TestSignature(I{info.opcode, SimdLaneImmediate{0}},
                  {VT_V128, info.value_type}, {VT_V128});
  }
}

TEST_F(ValidateInstructionTest, SimdReplaceLanes_ValidLane) {
  const struct {
    Opcode opcode;
    StackType stack_type;
    SimdLaneImmediate max_valid_lane;
  } infos[] = {{O::I8X16ReplaceLane, ST::I32(), 15},
               {O::I16X8ReplaceLane, ST::I32(), 7},
               {O::I32X4ReplaceLane, ST::I32(), 3},
               {O::I64X2ReplaceLane, ST::I64(), 1},
               {O::F32X4ReplaceLane, ST::F32(), 3},
               {O::F64X2ReplaceLane, ST::F64(), 1}};

  for (const auto& info: infos) {
    // Test valid indexes.
    for (SimdLaneImmediate imm = 0; imm < info.max_valid_lane; ++imm) {
      context.type_stack = StackTypeList{ST::V128(), info.stack_type};
      Ok(I{info.opcode, imm});
    }

    // Test invalid indexes.
    context.type_stack = StackTypeList{ST::V128(), info.stack_type};
    Fail(I{info.opcode, SimdLaneImmediate(info.max_valid_lane + 1)});
  }
}

TEST_F(ValidateInstructionTest, SimdShifts) {
  const Opcode opcodes[] = {O::I8X16Shl, O::I8X16ShrS, O::I8X16ShrU,
                            O::I16X8Shl, O::I16X8ShrS, O::I16X8ShrU,
                            O::I32X4Shl, O::I32X4ShrS, O::I32X4ShrU,
                            O::I64X2Shl, O::I64X2ShrS, O::I64X2ShrU};

  for (const auto& opcode : opcodes) {
    TestSignature(I{opcode}, {VT_V128, VT_I32}, {VT_V128});
  }
}

TEST_F(ValidateInstructionTest, AtomicNotifyAndWait) {
  AddMemory(MemoryType{Limits{0, 0, Shared::Yes}});
  TestSignature(I{O::MemoryAtomicNotify, MemArgImmediate{2, 0}},
                {VT_I32, VT_I32}, {VT_I32});
  TestSignature(I{O::MemoryAtomicWait32, MemArgImmediate{2, 0}},
                {VT_I32, VT_I32, VT_I64}, {VT_I32});
  TestSignature(I{O::MemoryAtomicWait64, MemArgImmediate{3, 0}},
                {VT_I32, VT_I64, VT_I64}, {VT_I32});
}

TEST_F(ValidateInstructionTest, AtomicNotifyAndWait_Alignment) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {{O::MemoryAtomicNotify, 2},
               {O::MemoryAtomicWait32, 2},
               {O::MemoryAtomicWait64, 3}};

  AddMemory(MemoryType{Limits{0, 0, Shared::Yes}});
  Ok(I{O::Unreachable});

  for (const auto& info: infos) {
    // Only natural alignment is valid.
    for (u32 align = 0; align <= info.align + 1; ++align) {
      if (align != info.align) {
        Fail(I{info.opcode, MemArgImmediate{align, 0}});
        Ok(I{O::Drop});
        ExpectErrorSubstr({"instruction", "Invalid atomic alignment"}, errors);
      }
    }
  }
}


TEST_F(ValidateInstructionTest, AtomicLoad) {
  const struct {
    Opcode opcode;
    ValueType result;
    u32 align;
  } infos[] = {
      {O::I32AtomicLoad, VT_I32, 2},    {O::I32AtomicLoad8U, VT_I32, 0},
      {O::I32AtomicLoad16U, VT_I32, 1}, {O::I64AtomicLoad, VT_I64, 3},
      {O::I64AtomicLoad8U, VT_I64, 0},  {O::I64AtomicLoad16U, VT_I64, 1},
      {O::I64AtomicLoad32U, VT_I64, 2}};

  AddMemory(MemoryType{Limits{0, 0, Shared::Yes}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{info.align, 0}}, {VT_I32},
                  {info.result});
  }

  for (const auto& info: infos) {
    // Only natural alignment is valid.
    for (u32 align = 0; align <= info.align + 1; ++align) {
      if (align != info.align) {
        Ok(I{O::I32Const, s32{}});
        Fail(I{info.opcode, MemArgImmediate{align, 0}});
        ExpectErrorSubstr({"instruction", "Invalid atomic alignment"}, errors);
      }
    }
  }
}

TEST_F(ValidateInstructionTest, AtomicLoad_MemoryOOB) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {{O::I32AtomicLoad, 2},    {O::I32AtomicLoad8U, 0},
               {O::I32AtomicLoad16U, 1}, {O::I64AtomicLoad, 3},
               {O::I64AtomicLoad8U, 0},  {O::I64AtomicLoad16U, 1},
               {O::I64AtomicLoad32U, 2}};

  for (const auto& info: infos) {
    Ok(I{O::I32Const, s32{}});
    Fail(I{info.opcode, MemArgImmediate{info.align, 0}});
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                errors);
  }
}

TEST_F(ValidateInstructionTest, AtomicLoad_MemoryNonShared) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {{O::I32AtomicLoad, 2},    {O::I32AtomicLoad8U, 0},
               {O::I32AtomicLoad16U, 1}, {O::I64AtomicLoad, 3},
               {O::I64AtomicLoad8U, 0},  {O::I64AtomicLoad16U, 1},
               {O::I64AtomicLoad32U, 2}};

  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    Ok(I{O::I32Const, s32{}});
    Ok(I{info.opcode, MemArgImmediate{info.align, 0}});
  }
}

TEST_F(ValidateInstructionTest, AtomicStore) {
  const struct {
    Opcode opcode;
    ValueType value_type;
    u32 align;
  } infos[] = {
      {O::I32AtomicStore, VT_I32, 2},   {O::I32AtomicStore8, VT_I32, 0},
      {O::I32AtomicStore16, VT_I32, 1}, {O::I64AtomicStore, VT_I64, 3},
      {O::I64AtomicStore8, VT_I64, 0},  {O::I64AtomicStore16, VT_I64, 1},
      {O::I64AtomicStore32, VT_I64, 2}};

  AddMemory(MemoryType{Limits{0, 0, Shared::Yes}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{info.align, 0}},
                  {VT_I32, info.value_type}, {});
  }

  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    // Only natural alignment is valid.
    for (u32 align = 0; align <= info.align + 1; ++align) {
      if (align != info.align) {
        Fail(I{info.opcode, MemArgImmediate{align, 0}});
        ExpectErrorSubstr({"instruction", "Invalid atomic alignment"}, errors);
      }
    }
  }
}

TEST_F(ValidateInstructionTest, AtomicStore_MemoryOOB) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {{O::I32AtomicStore, 2},   {O::I32AtomicStore8, 0},
               {O::I32AtomicStore16, 1}, {O::I64AtomicStore, 3},
               {O::I64AtomicStore8, 0},  {O::I64AtomicStore16, 1},
               {O::I64AtomicStore32, 2}};

  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    Fail(I{info.opcode, MemArgImmediate{info.align, 0}});
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                errors);
  }
}

TEST_F(ValidateInstructionTest, AtomicStore_MemoryNonShared) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {{O::I32AtomicStore, 2},   {O::I32AtomicStore8, 0},
               {O::I32AtomicStore16, 1}, {O::I64AtomicStore, 3},
               {O::I64AtomicStore8, 0},  {O::I64AtomicStore16, 1},
               {O::I64AtomicStore32, 2}};

  AddMemory(MemoryType{Limits{0}});
  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    Ok(I{info.opcode, MemArgImmediate{info.align, 0}});
  }
}

TEST_F(ValidateInstructionTest, AtomicRmw) {
  const struct {
    Opcode opcode;
    ValueType value_type;
    u32 align;
  } infos[] = {
      {O::I32AtomicRmwAdd, VT_I32, 2},    {O::I32AtomicRmwSub, VT_I32, 2},
      {O::I32AtomicRmwAnd, VT_I32, 2},    {O::I32AtomicRmwOr, VT_I32, 2},
      {O::I32AtomicRmwXor, VT_I32, 2},    {O::I32AtomicRmwXchg, VT_I32, 2},
      {O::I32AtomicRmw16AddU, VT_I32, 1}, {O::I32AtomicRmw16SubU, VT_I32, 1},
      {O::I32AtomicRmw16AndU, VT_I32, 1}, {O::I32AtomicRmw16OrU, VT_I32, 1},
      {O::I32AtomicRmw16XorU, VT_I32, 1}, {O::I32AtomicRmw16XchgU, VT_I32, 1},
      {O::I32AtomicRmw8AddU, VT_I32, 0},  {O::I32AtomicRmw8SubU, VT_I32, 0},
      {O::I32AtomicRmw8AndU, VT_I32, 0},  {O::I32AtomicRmw8OrU, VT_I32, 0},
      {O::I32AtomicRmw8XorU, VT_I32, 0},  {O::I32AtomicRmw8XchgU, VT_I32, 0},
      {O::I64AtomicRmwAdd, VT_I64, 3},    {O::I64AtomicRmwSub, VT_I64, 3},
      {O::I64AtomicRmwAnd, VT_I64, 3},    {O::I64AtomicRmwOr, VT_I64, 3},
      {O::I64AtomicRmwXor, VT_I64, 3},    {O::I64AtomicRmwXchg, VT_I64, 3},
      {O::I64AtomicRmw32AddU, VT_I64, 2}, {O::I64AtomicRmw32SubU, VT_I64, 2},
      {O::I64AtomicRmw32AndU, VT_I64, 2}, {O::I64AtomicRmw32OrU, VT_I64, 2},
      {O::I64AtomicRmw32XorU, VT_I64, 2}, {O::I64AtomicRmw32XchgU, VT_I64, 2},
      {O::I64AtomicRmw16AddU, VT_I64, 1}, {O::I64AtomicRmw16SubU, VT_I64, 1},
      {O::I64AtomicRmw16AndU, VT_I64, 1}, {O::I64AtomicRmw16OrU, VT_I64, 1},
      {O::I64AtomicRmw16XorU, VT_I64, 1}, {O::I64AtomicRmw16XchgU, VT_I64, 1},
      {O::I64AtomicRmw8AddU, VT_I64, 0},  {O::I64AtomicRmw8SubU, VT_I64, 0},
      {O::I64AtomicRmw8AndU, VT_I64, 0},  {O::I64AtomicRmw8OrU, VT_I64, 0},
      {O::I64AtomicRmw8XorU, VT_I64, 0},  {O::I64AtomicRmw8XchgU, VT_I64, 0},
  };

  AddMemory(MemoryType{Limits{0, 0, Shared::Yes}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{info.align, 0}},
                  {VT_I32, info.value_type}, {info.value_type});
  }

  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    // Only natural alignment is valid.
    for (u32 align = 0; align <= info.align + 1; ++align) {
      if (align != info.align) {
        Fail(I{info.opcode, MemArgImmediate{align, 0}});
        ExpectErrorSubstr({"instruction", "Invalid atomic alignment"}, errors);
        Ok(I{O::Drop});
      }
    }
  }
}

TEST_F(ValidateInstructionTest, AtomicRmw_MemoryOOB) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {
      {O::I32AtomicRmwAdd, 2},    {O::I32AtomicRmwSub, 2},
      {O::I32AtomicRmwAnd, 2},    {O::I32AtomicRmwOr, 2},
      {O::I32AtomicRmwXor, 2},    {O::I32AtomicRmwXchg, 2},
      {O::I32AtomicRmw16AddU, 1}, {O::I32AtomicRmw16SubU, 1},
      {O::I32AtomicRmw16AndU, 1}, {O::I32AtomicRmw16OrU, 1},
      {O::I32AtomicRmw16XorU, 1}, {O::I32AtomicRmw16XchgU, 1},
      {O::I32AtomicRmw8AddU, 0},  {O::I32AtomicRmw8SubU, 0},
      {O::I32AtomicRmw8AndU, 0},  {O::I32AtomicRmw8OrU, 0},
      {O::I32AtomicRmw8XorU, 0},  {O::I32AtomicRmw8XchgU, 0},
      {O::I64AtomicRmwAdd, 3},    {O::I64AtomicRmwSub, 3},
      {O::I64AtomicRmwAnd, 3},    {O::I64AtomicRmwOr, 3},
      {O::I64AtomicRmwXor, 3},    {O::I64AtomicRmwXchg, 3},
      {O::I64AtomicRmw32AddU, 2}, {O::I64AtomicRmw32SubU, 2},
      {O::I64AtomicRmw32AndU, 2}, {O::I64AtomicRmw32OrU, 2},
      {O::I64AtomicRmw32XorU, 2}, {O::I64AtomicRmw32XchgU, 2},
      {O::I64AtomicRmw16AddU, 1}, {O::I64AtomicRmw16SubU, 1},
      {O::I64AtomicRmw16AndU, 1}, {O::I64AtomicRmw16OrU, 1},
      {O::I64AtomicRmw16XorU, 1}, {O::I64AtomicRmw16XchgU, 1},
      {O::I64AtomicRmw8AddU, 0},  {O::I64AtomicRmw8SubU, 0},
      {O::I64AtomicRmw8AndU, 0},  {O::I64AtomicRmw8OrU, 0},
      {O::I64AtomicRmw8XorU, 0},  {O::I64AtomicRmw8XchgU, 0},
  };

  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    Fail(I{info.opcode, MemArgImmediate{info.align, 0}});
    Ok(I{O::Drop});
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                errors);
  }
}

TEST_F(ValidateInstructionTest, AtomicRmw_MemoryNonShared) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {
      {O::I32AtomicRmwAdd, 2},    {O::I32AtomicRmwSub, 2},
      {O::I32AtomicRmwAnd, 2},    {O::I32AtomicRmwOr, 2},
      {O::I32AtomicRmwXor, 2},    {O::I32AtomicRmwXchg, 2},
      {O::I32AtomicRmw16AddU, 1}, {O::I32AtomicRmw16SubU, 1},
      {O::I32AtomicRmw16AndU, 1}, {O::I32AtomicRmw16OrU, 1},
      {O::I32AtomicRmw16XorU, 1}, {O::I32AtomicRmw16XchgU, 1},
      {O::I32AtomicRmw8AddU, 0},  {O::I32AtomicRmw8SubU, 0},
      {O::I32AtomicRmw8AndU, 0},  {O::I32AtomicRmw8OrU, 0},
      {O::I32AtomicRmw8XorU, 0},  {O::I32AtomicRmw8XchgU, 0},
      {O::I64AtomicRmwAdd, 3},    {O::I64AtomicRmwSub, 3},
      {O::I64AtomicRmwAnd, 3},    {O::I64AtomicRmwOr, 3},
      {O::I64AtomicRmwXor, 3},    {O::I64AtomicRmwXchg, 3},
      {O::I64AtomicRmw32AddU, 2}, {O::I64AtomicRmw32SubU, 2},
      {O::I64AtomicRmw32AndU, 2}, {O::I64AtomicRmw32OrU, 2},
      {O::I64AtomicRmw32XorU, 2}, {O::I64AtomicRmw32XchgU, 2},
      {O::I64AtomicRmw16AddU, 1}, {O::I64AtomicRmw16SubU, 1},
      {O::I64AtomicRmw16AndU, 1}, {O::I64AtomicRmw16OrU, 1},
      {O::I64AtomicRmw16XorU, 1}, {O::I64AtomicRmw16XchgU, 1},
      {O::I64AtomicRmw8AddU, 0},  {O::I64AtomicRmw8SubU, 0},
      {O::I64AtomicRmw8AndU, 0},  {O::I64AtomicRmw8OrU, 0},
      {O::I64AtomicRmw8XorU, 0},  {O::I64AtomicRmw8XchgU, 0},
  };

  Ok(I{O::Unreachable});
  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    Ok(I{info.opcode, MemArgImmediate{info.align, 0}});
    Ok(I{O::Drop});
  }
}

TEST_F(ValidateInstructionTest, AtomicCmpxchg) {
  const struct {
    Opcode opcode;
    ValueType value_type;
    u32 align;
  } infos[] = {
      {O::I32AtomicRmwCmpxchg, VT_I32, 2},
      {O::I32AtomicRmw16CmpxchgU, VT_I32, 1},
      {O::I32AtomicRmw8CmpxchgU, VT_I32, 0},
      {O::I64AtomicRmwCmpxchg, VT_I64, 3},
      {O::I64AtomicRmw32CmpxchgU, VT_I64, 2},
      {O::I64AtomicRmw16CmpxchgU, VT_I64, 1},
      {O::I64AtomicRmw8CmpxchgU, VT_I64, 0},
  };

  AddMemory(MemoryType{Limits{0, 0, Shared::Yes}});
  for (const auto& info: infos) {
    TestSignature(I{info.opcode, MemArgImmediate{info.align, 0}},
                  {VT_I32, info.value_type, info.value_type},
                  {info.value_type});
  }

  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    // Only natural alignment is valid.
    for (u32 align = 0; align <= info.align + 1; ++align) {
      if (align != info.align) {
        Fail(I{info.opcode, MemArgImmediate{align, 0}});
        ExpectErrorSubstr({"instruction", "Invalid atomic alignment"}, errors);
        Ok(I{O::Drop});
      }
    }
  }
}

TEST_F(ValidateInstructionTest, AtomicCmpxchg_MemoryOOB) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {
      {O::I32AtomicRmwCmpxchg, 2},    {O::I32AtomicRmw16CmpxchgU, 1},
      {O::I32AtomicRmw8CmpxchgU, 0},  {O::I64AtomicRmwCmpxchg, 3},
      {O::I64AtomicRmw32CmpxchgU, 2}, {O::I64AtomicRmw16CmpxchgU, 1},
      {O::I64AtomicRmw8CmpxchgU, 0},
  };

  Ok(I{O::Unreachable});
  for (const auto& info: infos) {
    Fail(I{info.opcode, MemArgImmediate{info.align, 0}});
    Ok(I{O::Drop});
    ExpectError({"instruction", "Invalid memory index 0, must be less than 0"},
                errors);
  }
}

TEST_F(ValidateInstructionTest, AtomicCmpxchg_MemoryNonShared) {
  const struct {
    Opcode opcode;
    u32 align;
  } infos[] = {
      {O::I32AtomicRmwCmpxchg, 2},    {O::I32AtomicRmw16CmpxchgU, 1},
      {O::I32AtomicRmw8CmpxchgU, 0},  {O::I64AtomicRmwCmpxchg, 3},
      {O::I64AtomicRmw32CmpxchgU, 2}, {O::I64AtomicRmw16CmpxchgU, 1},
      {O::I64AtomicRmw8CmpxchgU, 0},
  };

  Ok(I{O::Unreachable});
  AddMemory(MemoryType{Limits{0}});
  for (const auto& info: infos) {
    Ok(I{info.opcode, MemArgImmediate{info.align, 0}});
    Ok(I{O::Drop});
  }
}

TEST_F(ValidateInstructionTest, BrOnNull_Void) {
  Ok(I{O::Block, BT_Void});

  // Should be valid with nullable and non-nullable types.
  TestSignature(I{O::BrOnNull, Index{0}}, {VT_RefNullFunc}, {VT_RefFunc});
  TestSignature(I{O::BrOnNull, Index{0}}, {VT_RefFunc}, {VT_RefFunc});
}

TEST_F(ValidateInstructionTest, BrOnNull_SingleResult) {
  Ok(I{O::Block, BT_I32});

  // Should be valid with nullable and non-nullable types.
  TestSignature(I{O::BrOnNull, Index{0}}, {VT_I32, VT_RefNullFunc},
                {VT_I32, VT_RefFunc});
  TestSignature(I{O::BrOnNull, Index{0}}, {VT_I32, VT_RefFunc},
                {VT_I32, VT_RefFunc});
}

TEST_F(ValidateInstructionTest, BrOnNull_MultiResult) {
  auto v_if = AddFunctionType(FunctionType{{}, {VT_I32, VT_F32}});
  Ok(I{O::Block, BlockType(v_if)});

  // Should be valid with nullable and non-nullable types.
  TestSignature(I{O::BrOnNull, Index{0}}, {VT_I32, VT_F32, VT_RefNullFunc},
                {VT_I32, VT_F32, VT_RefFunc});
  TestSignature(I{O::BrOnNull, Index{0}}, {VT_I32, VT_F32, VT_RefFunc},
                {VT_I32, VT_F32, VT_RefFunc});
}

TEST_F(ValidateInstructionTest, RefAsNonNull) {
  TestSignature(I{O::RefAsNonNull}, {VT_RefNullFunc}, {VT_RefFunc});
  TestSignature(I{O::RefAsNonNull}, {VT_RefFunc}, {VT_RefFunc});
}

TEST_F(ValidateInstructionTest, CallRef) {
  auto index = AddFunctionType(FunctionType{{VT_I32, VT_F32}, {VT_F64}});

  // Can be called with ref type.
  ValueType ref_type = MakeValueTypeRef(index, Null::No);
  TestSignatureNoUnreachable(I{O::CallRef}, {VT_I32, VT_F32, ref_type},
                             {VT_F64});

  // Can be called with ref null type.
  ValueType ref_null_type = MakeValueTypeRef(index, Null::Yes);
  TestSignatureNoUnreachable(I{O::CallRef}, {VT_I32, VT_F32, ref_null_type},
                             {VT_F64});

  // It's impossible to know the result types of call_ref with an unreachable
  // stack, since the signature depends on the typed function reference. So
  // it's fine to keep the type stack empty.
  OkWithUnreachableStack(I{O::CallRef}, {}, {});
}

TEST_F(ValidateInstructionTest, CallRef_ParamTypeMismatch) {
  auto index = AddFunction(FunctionType{{VT_F32}, {}});
  context.declared_functions.insert(index);

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::RefFunc, index});
  Fail(I{O::CallRef});
  ExpectError({"instruction", "Expected stack to contain [f32], got [i32]"},
              errors);
}

TEST_F(ValidateInstructionTest, ReturnCallRef) {
  BeginFunction(FunctionType{{}, {VT_F64}});
  auto index = AddFunctionType(FunctionType{{VT_I32, VT_F32}, {VT_F64}});

  // Can be called with ref type.
  ValueType ref_type = MakeValueTypeRef(index, Null::No);
  TestSignature(I{O::ReturnCallRef}, {VT_I32, VT_F32, ref_type}, {});

  // Can be called with ref null type.
  ValueType ref_null_type = MakeValueTypeRef(index, Null::Yes);
  TestSignature(I{O::ReturnCallRef}, {VT_I32, VT_F32, ref_null_type}, {});
}

TEST_F(ValidateInstructionTest, ReturnCallRef_ResultType_Subtyping) {
  BeginFunction(FunctionType{{}, {VT_Funcref}});
  auto index = AddFunction(FunctionType{{}, {VT_Ref0}});
  context.declared_functions.insert(index);

  Ok(I{O::RefFunc, index});
  Ok(I{O::ReturnCallRef});
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, ReturnCallRef_Unreachable) {
  BeginFunction(FunctionType{});
  auto index = AddFunction(FunctionType{});
  context.declared_functions.insert(index);

  Ok(I{O::RefFunc, index});
  Ok(I{O::ReturnCallRef});

  EXPECT_TRUE(context.IsStackPolymorphic());
  ExpectNoErrors(errors);
}

TEST_F(ValidateInstructionTest, ReturnCallRef_ParamTypeMismatch) {
  BeginFunction(FunctionType{});
  auto index = AddFunction(FunctionType{{VT_F32}, {}});
  context.declared_functions.insert(index);

  Ok(I{O::I32Const, s32{}});
  Ok(I{O::RefFunc, index});
  Fail(I{O::ReturnCallRef});
  ExpectError({"instruction", "Expected stack to contain [f32], got [i32]"},
              errors);
}

TEST_F(ValidateInstructionTest, ReturnCallRef_ResultTypeMismatch) {
  BeginFunction(FunctionType{{}, {VT_F32}});
  auto index = AddFunction(FunctionType{{}, {VT_I32}});
  context.declared_functions.insert(index);

  Ok(I{O::RefFunc, index});
  Fail(I{O::ReturnCallRef});
  ExpectError(
      {"instruction",
       "Callee's result types [f32] must equal caller's result types [i32]"},
      errors);
}

TEST_F(ValidateInstructionTest, FuncBind) {
  const struct {
    FunctionType old_ft;
    FunctionType new_ft;
    ValueTypeList params;
  } infos[] = {
      // Can bind identical function type.
      {{{}, {}}, {{}, {}}, {}},
      // Result types must match.
      {{{}, {VT_I32}}, {{}, {VT_I32}}, {}},
      // Can bind more than one param.
      {{{VT_I32, VT_F32}, {}}, {{}, {}}, {VT_I32, VT_F32}},
      // Can leave unbound types.
      {{{VT_I32, VT_F32}, {}}, {{VT_F32}, {}}, {VT_I32}},
      // Function params are covariant.
      {{{VT_RefFunc}, {}}, {{VT_RefNullFunc}, {}}, {}},
      // Function results are contravariant.
      {{{}, {VT_RefNullFunc}}, {{}, {VT_RefFunc}}, {}},
  };

  for (const auto& info: infos) {
    context.Reset();
    BeginFunction(FunctionType{});
    auto old_index = AddFunctionType(info.old_ft);
    auto new_index = AddFunctionType(info.new_ft);

    ValueType VT_RefOld = MakeValueTypeRef(old_index, Null::No);
    ValueType VT_RefNew = MakeValueTypeRef(new_index, Null::No);
    ValueTypeList params = info.params;
    params.push_back(VT_RefOld);
    TestSignature(I{O::FuncBind, new_index}, params, {VT_RefNew});
  }
}

TEST_F(ValidateInstructionTest, FuncBind_TypeMismatch) {
  const struct {
    FunctionType old_ft;
    FunctionType new_ft;
    ValueTypeList params;
  } infos[] = {
    // Binding can't add new params.
    {{{VT_I32}, {}}, {{VT_I32, VT_I32}, {}}, {}},
    // Params must match.
    {{{VT_F32}, {}}, {{VT_I32}, {}}, {}},
    // Param types must be covariant.
    {{{VT_RefNullFunc}, {}}, {{VT_RefFunc}, {}}, {}},
    // Result types must match.
    {{{}, {VT_I32}}, {{}, {VT_I32, VT_I32}}, {}},
    // Result types must be contravariant.
    {{{}, {VT_RefFunc}}, {{VT_RefNullFunc}, {}}, {}},
    // Bound param types must be on operand stack.
    {{{VT_I32, VT_F32}, {}}, {{VT_F32}, {}}, {VT_F32}},
  };

  for (const auto& info: infos) {
    context.Reset();
    BeginFunction(FunctionType{});
    auto old_index = AddFunctionType(info.old_ft);
    auto new_index = AddFunctionType(info.new_ft);

    ValueType VT_RefOld = MakeValueTypeRef(old_index, Null::No);
    ValueTypeList params = info.params;
    params.push_back(VT_RefOld);
    FailWithTypeStack(I{O::FuncBind, new_index}, params);
    ClearErrors(errors);
  }
}

TEST_F(ValidateInstructionTest, Let) {
  // Empty let.
  TestSignature(I{O::Let, LetImmediate{BT_Void, {}}}, {}, {});

  // A local.
  TestSignature(I{O::Let, LetImmediate{BT_Void, {Locals{1, VT_I32}}}}, {VT_I32},
                {});

  // Multiple locals.
  TestSignature(I{O::Let, LetImmediate{BT_Void, {Locals{2, VT_I32}}}},
                {VT_I32, VT_I32}, {});

  // Multiple local blocks.
  TestSignature(
      I{O::Let, LetImmediate{BT_Void, {Locals{1, VT_I32}, Locals{1, VT_F32}}}},
      {VT_I32, VT_F32}, {});

  // Params and locals.
  auto index = AddFunctionType(FunctionType{{VT_F32}, {}});
  TestSignature(I{O::Let, LetImmediate{BlockType{index}, {Locals{1, VT_I32}}}},
                {VT_F32, VT_I32}, {VT_F32});
}

TEST_F(ValidateInstructionTest, Let_Defaultable) {
  // Let expressions can bind non-defaultable types (unlike function locals).
  TestSignature(I{O::Let, LetImmediate{BT_Void, {Locals{1, VT_Ref0}}}},
                {VT_Ref0}, {});
}

TEST_F(ValidateInstructionTest, RefEq) {
  TestSignature(I{O::RefEq}, {VT_RefEq, VT_RefEq}, {VT_I32});
}

TEST_F(ValidateInstructionTest, I31) {
  TestSignature(I{O::I31New}, {VT_I32}, {VT_RefI31});
  TestSignature(I{O::I31GetU}, {VT_RefI31}, {VT_I32});
  TestSignature(I{O::I31GetS}, {VT_RefI31}, {VT_I32});
}

TEST_F(ValidateInstructionTest, RttCanon) {
  TestSignature(I{O::RttCanon, HT_Any}, {}, {VT_RTT_0_Any});
  TestSignature(I{O::RttCanon, HT_Func}, {}, {VT_RTT_1_Func});
  TestSignature(I{O::RttCanon, HT_Extern}, {}, {VT_RTT_1_Extern});
  TestSignature(I{O::RttCanon, HT_Eq}, {}, {VT_RTT_1_Eq});
  TestSignature(I{O::RttCanon, HT_I31}, {}, {VT_RTT_1_I31});
  TestSignature(I{O::RttCanon, HT_Exn}, {}, {VT_RTT_1_Exn});
}

TEST_F(ValidateInstructionTest, RttSub) {
  auto parent = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const}}});

  auto child = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const},
                    FieldType{StorageType{VT_I64}, Mutability::Const}}});

  ValueType VT_RTT_1_Parent{Rtt{1u, HeapType{parent}}};
  ValueType VT_RTT_2_Child{Rtt{2u, HeapType{child}}};

  TestSignatureNoUnreachable(I{O::RttSub, HeapType{parent}}, {VT_RTT_0_Any},
                             {VT_RTT_1_Parent});

  TestSignatureNoUnreachable(I{O::RttSub, HeapType{child}}, {VT_RTT_1_Parent},
                             {VT_RTT_2_Child});

  // It's impossible to know the result types of rtt.sub with an unreachable
  // stack, since the signature depends on the rtt on the stack. So it's fine
  // to keep the type stack empty.
  OkWithUnreachableStack(I{O::RttSub, HT_Func}, {}, {});
}

TEST_F(ValidateInstructionTest, RefTest) {
  auto index = AddFunctionType({});
  HeapType HT_index{index};

  // ref.test any func: [(anyref) (rtt 1 func)] -> [i32]
  TestSignature(I{O::RefTest, HeapType2Immediate{HT_Any, HT_Func}},
                {VT_Anyref, VT_RTT_1_Func}, {VT_I32});

  // ref.test func i: [(funcref) (rtt 2 i)] -> [i32]
  TestSignature(I{O::RefTest, HeapType2Immediate{HT_Func, HT_index}},
                {VT_Funcref, ValueType{Rtt{2, HT_index}}}, {VT_I32});
}

TEST_F(ValidateInstructionTest, RefTest_NotSubtype) {
  // extern is not a subtype of func
  FailWithTypeStack(I{O::RefTest, HeapType2Immediate{HT_Func, HT_Extern}},
                    {VT_Funcref, VT_RTT_1_Extern});
}

TEST_F(ValidateInstructionTest, RefCast) {
  auto index = AddFunctionType({});
  HeapType HT_index{index};

  // ref.cast any func: [(anyref) (rtt 1 func)] -> [funcref]
  TestSignature(I{O::RefCast, HeapType2Immediate{HT_Any, HT_Func}},
                {VT_Anyref, VT_RTT_1_Func}, {VT_RefFunc});

  // ref.cast func i: [(funcref) (rtt 2 i)] -> [ref i]
  TestSignature(I{O::RefCast, HeapType2Immediate{HT_Func, HT_index}},
                {VT_Funcref, ValueType{Rtt{2, HT_index}}},
                {ValueType{ReferenceType{RefType{HT_index, Null::No}}}});
}

TEST_F(ValidateInstructionTest, RefCast_NotSubtype) {
  // extern is not a subtype of func
  FailWithTypeStack(I{O::RefCast, HeapType2Immediate{HT_Func, HT_Extern}},
                    {VT_Funcref, VT_RTT_1_Extern});
}

TEST_F(ValidateInstructionTest, BrOnCast) {
  Ok(I{O::Block, BT_RefFunc});

  // Should be valid with nullable and non-nullable types. The result value is
  // the first parameter passed through.
  TestSignatureNoUnreachable(I{O::BrOnCast, Index{0}},
                             {VT_RefNullAny, VT_RTT_1_Func}, {VT_RefNullAny});
  TestSignatureNoUnreachable(I{O::BrOnCast, Index{0}},
                             {VT_RefAny, VT_RTT_1_Func}, {VT_RefAny});

  // It's impossible to know the result types of br_on_cast with an unreachable
  // stack, since the signature depends on the rtt on the stack. So it's fine
  // to keep the type stack empty.
  OkWithUnreachableStack(I{O::BrOnCast, Index{0}}, {}, {});
  OkWithUnreachableStack(I{O::BrOnCast, Index{0}}, {VT_RTT_1_Func}, {});
}

TEST_F(ValidateInstructionTest, BrOnCast_NotSubtype) {
  // extern is not a subtype of func
  FailWithTypeStack(I{O::BrOnCast, Index{0}}, {VT_Funcref, VT_RTT_1_Extern});
}

TEST_F(ValidateInstructionTest, StructNewWithRtt) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const},
                    FieldType{StorageType{VT_I64}, Mutability::Const}}});

  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  ValueType VT_Ref_index{ReferenceType{RefType{HeapType{index}, Null::No}}};

  TestSignature(I{O::StructNewWithRtt, index}, {VT_I32, VT_I64, VT_RTT_1_index},
                {VT_Ref_index});
}

TEST_F(ValidateInstructionTest, StructNewWithRtt_RttMismatch) {
  auto index = AddStructType({});
  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  FailWithTypeStack(I{O::StructNewWithRtt, Index{999}}, {VT_RTT_1_index});
}

TEST_F(ValidateInstructionTest, StructNewDefaultWithRtt) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const},
                    FieldType{StorageType{VT_I64}, Mutability::Const},
                    // Nullable reference field
                    FieldType{StorageType{VT_RefNull0}, Mutability::Const}}});

  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  ValueType VT_Ref_index{ReferenceType{RefType{HeapType{index}, Null::No}}};

  TestSignature(I{O::StructNewDefaultWithRtt, index}, {VT_RTT_1_index},
                {VT_Ref_index});
}

TEST_F(ValidateInstructionTest, StructNewDefaultWithRtt_RttMismatch) {
  auto index = AddStructType({});
  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  FailWithTypeStack(I{O::StructNewDefaultWithRtt, Index{999}},
                    {VT_RTT_1_index});
}

TEST_F(ValidateInstructionTest, StructNewDefaultWithRtt_NonDefaultable) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_Ref0}, Mutability::Const}}});

  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  FailWithTypeStack(I{O::StructNewDefaultWithRtt, Index{1}},
                    {VT_RTT_1_index});
}

TEST_F(ValidateInstructionTest, StructGet) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_F32}, Mutability::Const}}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  TestSignature(I{O::StructGet, StructFieldImmediate{index, Index{0}}},
                {VT_RefNull_index}, {VT_F32});
  TestSignature(I{O::StructGet, StructFieldImmediate{index, Index{0}}},
                {VT_Ref_index}, {VT_F32});
}

TEST_F(ValidateInstructionTest, StructGet_FailPacked) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{PackedType::I8}, Mutability::Const}}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);

  FailWithTypeStack(I{O::StructGet, StructFieldImmediate{index, Index{0}}},
                    {VT_RefNull_index});
}

TEST_F(ValidateInstructionTest, StructGetPacked) {
  auto index = AddStructType(StructType{FieldTypeList{
      FieldType{StorageType{PackedType::I8}, Mutability::Const},
      FieldType{StorageType{PackedType::I16}, Mutability::Const}}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  // StructGetS
  TestSignature(I{O::StructGetS, StructFieldImmediate{index, Index{0}}},
                {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::StructGetS, StructFieldImmediate{index, Index{0}}},
                {VT_Ref_index}, {VT_I32});
  TestSignature(I{O::StructGetS, StructFieldImmediate{index, Index{1}}},
                {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::StructGetS, StructFieldImmediate{index, Index{1}}},
                {VT_Ref_index}, {VT_I32});

  // StructGetU
  TestSignature(I{O::StructGetU, StructFieldImmediate{index, Index{0}}},
                {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::StructGetU, StructFieldImmediate{index, Index{0}}},
                {VT_Ref_index}, {VT_I32});
  TestSignature(I{O::StructGetU, StructFieldImmediate{index, Index{1}}},
                {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::StructGetU, StructFieldImmediate{index, Index{1}}},
                {VT_Ref_index}, {VT_I32});
}

TEST_F(ValidateInstructionTest, StructGetPacked_FailUnpacked) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_F32}, Mutability::Const}}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);

  FailWithTypeStack(I{O::StructGetS, StructFieldImmediate{index, Index{0}}},
                    {VT_RefNull_index});
  FailWithTypeStack(I{O::StructGetU, StructFieldImmediate{index, Index{0}}},
                    {VT_RefNull_index});
}

TEST_F(ValidateInstructionTest, StructSet) {
  auto index = AddStructType(StructType{
      FieldTypeList{FieldType{StorageType{VT_F32}, Mutability::Var}}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  TestSignature(I{O::StructSet, StructFieldImmediate{index, Index{0}}},
                {VT_RefNull_index, VT_F32}, {});
  TestSignature(I{O::StructSet, StructFieldImmediate{index, Index{0}}},
                {VT_Ref_index, VT_F32}, {});
}

TEST_F(ValidateInstructionTest, ArrayNewWithRtt) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_I64}, Mutability::Const}});

  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  ValueType VT_Ref_index{ReferenceType{RefType{HeapType{index}, Null::No}}};

  TestSignature(I{O::ArrayNewWithRtt, index}, {VT_I64, VT_I32, VT_RTT_1_index},
                {VT_Ref_index});
}

TEST_F(ValidateInstructionTest, ArrayNewWithRtt_RttMismatch) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_I64}, Mutability::Const}});
  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  FailWithTypeStack(I{O::ArrayNewWithRtt, Index{999}}, {VT_RTT_1_index});
}

TEST_F(ValidateInstructionTest, ArrayNewDefaultWithRtt) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_I64}, Mutability::Const}});

  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  ValueType VT_Ref_index{ReferenceType{RefType{HeapType{index}, Null::No}}};

  TestSignature(I{O::ArrayNewDefaultWithRtt, index}, {VT_I32, VT_RTT_1_index},
                {VT_Ref_index});
}

TEST_F(ValidateInstructionTest, ArrayNewDefaultWithRtt_RttMismatch) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_I64}, Mutability::Const}});
  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  FailWithTypeStack(I{O::ArrayNewDefaultWithRtt, Index{999}},
                    {VT_I32, VT_RTT_1_index});
}

TEST_F(ValidateInstructionTest, ArrayNewDefaultWithRtt_NonDefaultable) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_Ref0}, Mutability::Const}});

  ValueType VT_RTT_1_index{Rtt{1u, HeapType{index}}};
  FailWithTypeStack(I{O::ArrayNewDefaultWithRtt, Index{1}},
                    {VT_I32, VT_RTT_1_index});
}

TEST_F(ValidateInstructionTest, ArrayGet) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_F32}, Mutability::Const}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  TestSignature(I{O::ArrayGet, index}, {VT_RefNull_index}, {VT_F32});
  TestSignature(I{O::ArrayGet, index}, {VT_Ref_index}, {VT_F32});
}

TEST_F(ValidateInstructionTest, ArrayGet_FailPacked) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{PackedType::I8}, Mutability::Const}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);

  FailWithTypeStack(I{O::ArrayGet, index}, {VT_RefNull_index});
}

TEST_F(ValidateInstructionTest, ArrayGetPacked) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{PackedType::I8}, Mutability::Const}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  // ArrayGetS
  TestSignature(I{O::ArrayGetS, index}, {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::ArrayGetS, index}, {VT_Ref_index}, {VT_I32});

  // ArrayGetU
  TestSignature(I{O::ArrayGetU, index}, {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::ArrayGetU, index}, {VT_Ref_index}, {VT_I32});
}

TEST_F(ValidateInstructionTest, ArrayGetPacked_FailUnpacked) {
  auto index = AddArrayType(
      ArrayType{FieldType{StorageType{VT_F32}, Mutability::Const}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);

  FailWithTypeStack(I{O::ArrayGetS, index}, {VT_RefNull_index});
  FailWithTypeStack(I{O::ArrayGetU, index}, {VT_RefNull_index});
}

TEST_F(ValidateInstructionTest, ArraySet) {
  auto index =
      AddArrayType(ArrayType{FieldType{StorageType{VT_F32}, Mutability::Var}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  TestSignature(I{O::ArraySet, index}, {VT_RefNull_index, VT_F32}, {});
  TestSignature(I{O::ArraySet, index}, {VT_Ref_index, VT_F32}, {});
}

TEST_F(ValidateInstructionTest, ArrayLen) {
  auto index =
      AddArrayType(ArrayType{FieldType{StorageType{VT_F32}, Mutability::Var}});
  ValueType VT_RefNull_index = MakeValueTypeRef(index, Null::Yes);
  ValueType VT_Ref_index = MakeValueTypeRef(index, Null::No);

  TestSignature(I{O::ArrayLen, index}, {VT_RefNull_index}, {VT_I32});
  TestSignature(I{O::ArrayLen, index}, {VT_Ref_index}, {VT_I32});
}
