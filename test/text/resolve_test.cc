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

#include "wasp/text/resolve.h"

#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "wasp/base/errors.h"
#include "wasp/text/formatters.h"
#include "wasp/text/read/context.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::test;

class TextResolveTest : public ::testing::Test {
 protected:
  using VT = ValueType;
  using BVT = BoundValueType;
  using I = Instruction;
  using O = Opcode;

  template <typename T>
  void OK(const T& after, const T& before) {
    T copy = before;
    Resolve(context, copy);
    EXPECT_EQ(after, copy);
    ExpectNoErrors(errors);
  }

  // TODO: Add fail tests after refactoring to remove ExpectedError.
#if 0
  void Fail(
            const ExpectedError& error,
            const T& value,
            SpanU8 orig_data) {
    T copy = value;
    Resolve(context);
    ExpectError(error, errors, orig_data);
    errors.Clear();
  }

  void Fail(const std::vector<ExpectedError>& expected_errors,
            const T& value,
            SpanU8 orig_data) {
    T copy = value;
    Resolve(context, copy);
    ExpectErrors(expected_errors, errors, orig_data);
    errors.Clear();
  }
#endif

  TestErrors errors;
  Context context{errors};
};

TEST_F(TextResolveTest, FunctionTypeUse) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  context.type_names.NewBound("$b"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::F32}}, {}});

  // Resolve the variable name to an index, and populate the function type.
  OK(FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}},
     FunctionTypeUse{Var{"$a"_sv}, {}});

  // Just populate the function type.
  OK(FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}},
     FunctionTypeUse{Var{u32{0}}, {}});

  // Populate the variable when not specified.
  OK(FunctionTypeUse{Var{u32{1}}, FunctionType{{VT::F32}, {}}},
     FunctionTypeUse{nullopt, FunctionType{{VT::F32}, {}}});
}

TEST_F(TextResolveTest, BlockImmediate) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(BlockImmediate{nullopt,
                    FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}},
     BlockImmediate{nullopt, FunctionTypeUse{Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, BlockImmediate_InlineType) {
  // An inline type can only be void, or a single result type.
  OK(BlockImmediate{nullopt, {}}, BlockImmediate{nullopt, {}});

  for (auto value_type : {VT::I32, VT::I64, VT::F32, VT::F64, VT::V128,
                          VT::Funcref, VT::Externref, VT::Exnref}) {
    OK(BlockImmediate{nullopt,
                      FunctionTypeUse{nullopt, FunctionType{{}, {value_type}}}},
       BlockImmediate{
           nullopt, FunctionTypeUse{nullopt, FunctionType{{}, {value_type}}}});
  }
}

TEST_F(TextResolveTest, BrOnExnImmediate) {
  context.label_names.NewBound("$l");
  context.event_names.NewBound("$e");

  OK(BrOnExnImmediate{Var{u32{0}}, Var{u32{0}}},
     BrOnExnImmediate{Var{"$l"_sv}, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, BrTableImmediate) {
  context.label_names.NewBound("$l0");
  context.label_names.NewBound("$l1");
  context.label_names.NewUnbound();
  context.label_names.NewBound("$l3");

  OK(BrTableImmediate{{
                          Var{u32{3}},
                          Var{u32{2}},
                          Var{u32{1}},
                      },
                      Var{u32{0}}},
     BrTableImmediate{{
                          Var{"$l0"_sv},
                          Var{"$l1"_sv},
                          Var{u32{1}},
                      },
                      Var{"$l3"_sv}});
}

TEST_F(TextResolveTest, CallIndirectImmediate) {
  context.table_names.NewBound("$t");
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(CallIndirectImmediate{Var{u32{0}},
                           FunctionTypeUse{Var{u32{0}},
                                           FunctionType{{VT::I32}, {}}}},
     CallIndirectImmediate{Var{"$t"_sv}, FunctionTypeUse{Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, Instruction_NoOp) {
  // Bare.
  OK(I{O::Nop}, I{O::Nop});

  // u32 Immediate.
  OK(I{O::I32Const, MakeAt(""_su8, u32{})}, I{O::I32Const, MakeAt(""_su8, u32{})});

  // u64 Immediate.
  OK(I{O::I64Const, MakeAt(""_su8, u64{})}, I{O::I64Const, MakeAt(""_su8, u64{})});

  // f32 Immediate.
  OK(I{O::F32Const, MakeAt(""_su8, f32{})}, I{O::F32Const, MakeAt(""_su8, f32{})});

  // f64 Immediate.
  OK(I{O::F64Const, MakeAt(""_su8, f64{})}, I{O::F64Const, MakeAt(""_su8, f64{})});

  // v128 Immediate.
  OK(I{O::V128Const, v128{}}, I{O::V128Const, v128{}});

  // MemArg Immediate.
  OK(I{O::I32Load, MemArgImmediate{}}, I{O::I32Load, MemArgImmediate{}});

  // Select Immediate.
  OK(I{O::Select, SelectImmediate{}}, I{O::Select, SelectImmediate{}});

  // SimdShuffle Immediate.
  OK(I{O::V8X16Shuffle, ShuffleImmediate{}}, I{O::V8X16Shuffle, ShuffleImmediate{}});
}

TEST_F(TextResolveTest, Instruction_BlockImmediate) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(I{O::Block,
       BlockImmediate{
           nullopt, FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}}},
     I{O::Block, BlockImmediate{nullopt, FunctionTypeUse{Var{"$a"_sv}, {}}}});

  // Populate the type use.
  OK(I{O::Block,
       BlockImmediate{
           nullopt, FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}}},
     I{O::Block,
       BlockImmediate{nullopt,
                      FunctionTypeUse{nullopt, FunctionType{{VT::I32}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_BrOnExnImmediate) {
  context.label_names.NewBound("$l");
  context.event_names.NewBound("$e");

  OK(I{O::BrOnExn, BrOnExnImmediate{Var{u32{0}}, Var{u32{0}}}},
     I{O::BrOnExn, BrOnExnImmediate{Var{"$l"_sv}, Var{"$e"_sv}}});
}

TEST_F(TextResolveTest, Instruction_BrTableImmediate) {
  context.label_names.NewBound("$l0");
  context.label_names.NewBound("$l1");

  OK(I{O::BrTable, BrTableImmediate{{Var{u32{1}}}, Var{u32{0}}}},
     I{O::BrTable, BrTableImmediate{{Var{"$l0"_sv}}, Var{"$l1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_CallIndirectImmediate) {
  context.table_names.NewBound("$t");
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(I{O::CallIndirect,
       CallIndirectImmediate{
           Var{u32{0}},
           FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}}},
     I{O::CallIndirect,
       CallIndirectImmediate{Var{"$t"_sv}, FunctionTypeUse{Var{"$a"_sv}, {}}}});

  // Populate the type use.
  OK(I{O::CallIndirect,
       CallIndirectImmediate{
           Var{u32{0}},
           FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}}},
     I{O::CallIndirect,
       CallIndirectImmediate{
           Var{"$t"_sv},
           FunctionTypeUse{nullopt, FunctionType{{VT::I32}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_CopyImmediate_Table) {
  context.table_names.NewBound("$t0");
  context.table_names.NewBound("$t1");

  OK(I{O::TableCopy, CopyImmediate{Var{u32{0}}, Var{u32{1}}}},
     I{O::TableCopy, CopyImmediate{Var{"$t0"_sv}, Var{"$t1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_CopyImmediate_Memory) {
  context.memory_names.NewBound("$m0");
  context.memory_names.NewBound("$m1");

  OK(I{O::MemoryCopy, CopyImmediate{Var{u32{0}}, Var{u32{1}}}},
     I{O::MemoryCopy, CopyImmediate{Var{"$m0"_sv}, Var{"$m1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_InitImmediate_Table) {
  context.element_segment_names.NewBound("$e");
  context.table_names.NewBound("$t");

  OK(I{O::TableInit, InitImmediate{Var{u32{0}}, Var{u32{0}}}},
     I{O::TableInit, InitImmediate{Var{"$e"_sv}, Var{"$t"_sv}}});
}

TEST_F(TextResolveTest, Instruction_Var_Function) {
  context.function_names.NewBound("$f");

  OK(I{O::Call, Var{u32{0}}}, I{O::Call, Var{"$f"_sv}});
  OK(I{O::ReturnCall, Var{u32{0}}}, I{O::ReturnCall, Var{"$f"_sv}});
  OK(I{O::RefFunc, Var{u32{0}}}, I{O::RefFunc, Var{"$f"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Table) {
  context.table_names.NewBound("$t");

  OK(I{O::TableFill, Var{u32{0}}}, I{O::TableFill, Var{"$t"_sv}});
  OK(I{O::TableGet, Var{u32{0}}}, I{O::TableGet, Var{"$t"_sv}});
  OK(I{O::TableGrow, Var{u32{0}}}, I{O::TableGrow, Var{"$t"_sv}});
  OK(I{O::TableSet, Var{u32{0}}}, I{O::TableSet, Var{"$t"_sv}});
  OK(I{O::TableSize, Var{u32{0}}}, I{O::TableSize, Var{"$t"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Global) {
  context.global_names.NewBound("$t");

  OK(I{O::GlobalGet, Var{u32{0}}}, I{O::GlobalGet, Var{"$t"_sv}});
  OK(I{O::GlobalSet, Var{u32{0}}}, I{O::GlobalSet, Var{"$t"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Event) {
  context.event_names.NewBound("$e");

  OK(I{O::Throw, Var{u32{0}}}, I{O::Throw, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Element) {
  context.element_segment_names.NewBound("$e");

  OK(I{O::ElemDrop, Var{u32{0}}}, I{O::ElemDrop, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Data) {
  context.data_segment_names.NewBound("$d");

  OK(I{O::MemoryInit, Var{u32{0}}}, I{O::MemoryInit, Var{"$d"_sv}});
  OK(I{O::DataDrop, Var{u32{0}}}, I{O::DataDrop, Var{"$d"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Label) {
  context.label_names.NewBound("$l");

  OK(I{O::BrIf, Var{u32{0}}}, I{O::BrIf, Var{"$l"_sv}});
  OK(I{O::Br, Var{u32{0}}}, I{O::Br, Var{"$l"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Local) {
  context.local_names.NewBound("$l");

  OK(I{O::LocalGet, Var{u32{0}}}, I{O::LocalGet, Var{"$l"_sv}});
  OK(I{O::LocalSet, Var{u32{0}}}, I{O::LocalSet, Var{"$l"_sv}});
  OK(I{O::LocalTee, Var{u32{0}}}, I{O::LocalTee, Var{"$l"_sv}});
}

TEST_F(TextResolveTest, InstructionList) {
  context.function_names.NewBound("$f");
  context.local_names.NewBound("$l");

  OK(
      InstructionList{
          I{O::LocalGet, Var{u32{0}}},
          I{O::LocalSet, Var{u32{0}}},
          I{O::Call, Var{u32{0}}},
      },
      InstructionList{
          I{O::LocalGet, Var{"$l"_sv}},
          I{O::LocalSet, Var{"$l"_sv}},
          I{O::Call, Var{"$f"_sv}},
      });
}

TEST_F(TextResolveTest, InstructionList_LabelReuse) {
  context.function_type_map.Define(BoundFunctionType{});

  OK(
      InstructionList{
          I{O::Block, BlockImmediate{"$l1"_sv, {}}},
          I{O::Block, BlockImmediate{"$l0"_sv, {}}},
          I{O::Br, Var{u32{0}}},
          I{O::Br, Var{u32{1}}},
          I{O::End},
          I{O::Block, BlockImmediate{"$l0"_sv, {}}},
          I{O::Br, Var{u32{0}}},
          I{O::End},
          I{O::End},
      },
      InstructionList{
          I{O::Block, BlockImmediate{"$l1"_sv, {}}},
          I{O::Block, BlockImmediate{"$l0"_sv, {}}},
          I{O::Br, Var{"$l0"_sv}},
          I{O::Br, Var{"$l1"_sv}},
          I{O::End},
          I{O::Block, BlockImmediate{"$l0"_sv, {}}},
          I{O::Br, Var{"$l0"_sv}},
          I{O::End},
          I{O::End},
      });
}

TEST_F(TextResolveTest, FunctionDesc) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT::I32}}, {}});

  // $p param name is not copied.
  OK(FunctionDesc{nullopt, Var{u32{0}},
                  BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}},
     FunctionDesc{nullopt, Var{"$a"_sv}, {}});

  // Populate the type use.
  OK(FunctionDesc{nullopt, Var{u32{0}},
                  BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}},
     FunctionDesc{nullopt, nullopt,
                  BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}});
}

TEST_F(TextResolveTest, EventType) {
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(EventType{EventAttribute::Exception,
               FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}},
     EventType{EventAttribute::Exception, FunctionTypeUse{Var{"$a"_sv}, {}}});

  // Populate the type use.
  OK(EventType{EventAttribute::Exception,
               FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}},
     EventType{EventAttribute::Exception,
               FunctionTypeUse{nullopt, FunctionType{{VT::I32}, {}}}});
}

TEST_F(TextResolveTest, EventDesc) {
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(EventDesc{nullopt,
               EventType{
                   EventAttribute::Exception,
                   FunctionTypeUse{Var{u32{0}}, FunctionType{{VT::I32}, {}}}}},
     EventDesc{nullopt, EventType{EventAttribute::Exception,
                                  FunctionTypeUse{Var{"$a"_sv}, {}}}});
}

TEST_F(TextResolveTest, Import_Function) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT::I32}}, {}});

  // $p param name is not copied.
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            FunctionDesc{nullopt, Var{u32{0}},
                         BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            FunctionDesc{nullopt, Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, Import_Table) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TableDesc{nullopt, TableType{Limits{0}, ReferenceType::Funcref}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TableDesc{nullopt, TableType{Limits{0}, ReferenceType::Funcref}}});
}

TEST_F(TextResolveTest, Import_Memory) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            MemoryDesc{nullopt, MemoryType{Limits{0}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            MemoryDesc{nullopt, MemoryType{Limits{0}}}});
}

TEST_F(TextResolveTest, Import_Global) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            GlobalDesc{nullopt, GlobalType{ValueType::I32, Mutability::Const}}},
     Import{
         Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
         GlobalDesc{nullopt, GlobalType{ValueType::I32, Mutability::Const}}});
}

TEST_F(TextResolveTest, Import_Event) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT::I32}}, {}});

  // $p param name is not copied.
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            EventDesc{nullopt,
                      EventType{EventAttribute::Exception,
                                FunctionTypeUse{Var{u32{0}},
                                                FunctionType{{VT::I32}, {}}}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            EventDesc{nullopt, EventType{EventAttribute::Exception,
                                         FunctionTypeUse{Var{"$a"_sv}, {}}}}});
}

TEST_F(TextResolveTest, Function) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT::I32}}, {}});

  OK(Function{FunctionDesc{nullopt, Var{u32{0}},
                           BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}},
              {BVT{"$l"_sv, VT::I32}},
              InstructionList{I{O::LocalGet, Var{u32{1}}}},
              {}},
     Function{FunctionDesc{nullopt, Var{"$a"_sv}, {}},
              {BVT{"$l"_sv, VT::I32}},
              InstructionList{I{O::LocalGet, Var{"$l"_sv}}},
              {}});
}

TEST_F(TextResolveTest, ElementExpressionList) {
  context.function_names.NewBound("$f"_sv);

  OK(
      ElementExpressionList{
          ElementExpression{I{O::RefNull}},
          ElementExpression{I{O::RefFunc, Var{u32{0}}}},
      },
      ElementExpressionList{
          ElementExpression{I{O::RefNull}},
          ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
      });
}

TEST_F(TextResolveTest, ElementListWithExpressions) {
  context.function_names.NewBound("$f"_sv);

  OK(
      ElementListWithExpressions{
          ReferenceType::Funcref,
          ElementExpressionList{
              ElementExpression{I{O::RefNull}},
              ElementExpression{I{O::RefFunc, Var{u32{0}}}},
          }},
      ElementListWithExpressions{
          ReferenceType::Funcref,
          ElementExpressionList{
              ElementExpression{I{O::RefNull}},
              ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
          }});
}

TEST_F(TextResolveTest, ElementListWithVars) {
  context.function_names.NewBound("$f"_sv);
  context.function_names.NewUnbound();

  OK(ElementListWithVars{ExternalKind::Function,
                         VarList{
                             Var{u32{0}},
                             Var{u32{1}},
                         }},
     ElementListWithVars{ExternalKind::Function, VarList{
                                                     Var{"$f"_sv},
                                                     Var{u32{1}},
                                                 }});
}

TEST_F(TextResolveTest, ElementList) {
  context.function_names.NewBound("$f"_sv);
  context.function_names.NewUnbound();

  // Expressions.
  OK(ElementList{ElementListWithExpressions{
         ReferenceType::Funcref,
         ElementExpressionList{
             ElementExpression{I{O::RefNull}},
             ElementExpression{I{O::RefFunc, Var{u32{0}}}},
         }}},
     ElementList{ElementListWithExpressions{
         ReferenceType::Funcref,
         ElementExpressionList{
             ElementExpression{I{O::RefNull}},
             ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
         }}});

  // Vars.
  OK(ElementList{ElementListWithVars{ExternalKind::Function,
                                     VarList{
                                         Var{u32{0}},
                                         Var{u32{1}},
                                     }}},
     ElementList{ElementListWithVars{ExternalKind::Function, VarList{
                                                                 Var{"$f"_sv},
                                                                 Var{u32{1}},
                                                             }}});
}

TEST_F(TextResolveTest, Table) {
  context.function_names.NewBound("$f"_sv);

  OK(Table{TableDesc{nullopt, TableType{Limits{0}, ReferenceType::Funcref}},
           {},
           ElementList{ElementListWithExpressions{
               ReferenceType::Funcref,
               ElementExpressionList{
                   ElementExpression{I{O::RefFunc, Var{u32{0}}}},
               }}}},
     Table{TableDesc{nullopt, TableType{Limits{0}, ReferenceType::Funcref}},
           {},
           ElementList{ElementListWithExpressions{
               ReferenceType::Funcref,
               ElementExpressionList{
                   ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
               }}}});
}

TEST_F(TextResolveTest, Global) {
  context.global_names.NewBound("$g"_sv);

  OK(
      Global{
          GlobalDesc{nullopt, GlobalType{ValueType::I32, Mutability::Const}},
          InstructionList{I{O::GlobalGet, Var{u32{0}}}},
          {},
      },
      Global{
          GlobalDesc{nullopt, GlobalType{ValueType::I32, Mutability::Const}},
          InstructionList{I{O::GlobalGet, Var{"$g"_sv}}},
          {},
      });
}

TEST_F(TextResolveTest, Export) {
  context.function_names.NewBound("$f"_sv);  // 0
  context.table_names.NewUnbound();
  context.table_names.NewBound("$t"_sv);  // 1
  context.memory_names.NewUnbound();
  context.memory_names.NewUnbound();
  context.memory_names.NewBound("$m"_sv);  // 2
  context.global_names.NewUnbound();
  context.global_names.NewUnbound();
  context.global_names.NewUnbound();
  context.global_names.NewBound("$g"_sv);  // 3
  context.event_names.NewUnbound();
  context.event_names.NewUnbound();
  context.event_names.NewUnbound();
  context.event_names.NewUnbound();
  context.event_names.NewBound("$e"_sv);  // 4

  OK(Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{u32{0}}},
     Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{"$f"_sv}});

  OK(Export{ExternalKind::Table, Text{"\"t\"", 1}, Var{u32{1}}},
     Export{ExternalKind::Table, Text{"\"t\"", 1}, Var{"$t"_sv}});

  OK(Export{ExternalKind::Memory, Text{"\"m\"", 1}, Var{u32{2}}},
     Export{ExternalKind::Memory, Text{"\"m\"", 1}, Var{"$m"_sv}});

  OK(Export{ExternalKind::Global, Text{"\"g\"", 1}, Var{u32{3}}},
     Export{ExternalKind::Global, Text{"\"g\"", 1}, Var{"$g"_sv}});

  OK(Export{ExternalKind::Event, Text{"\"e\"", 1}, Var{u32{4}}},
     Export{ExternalKind::Event, Text{"\"e\"", 1}, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Start) {
  context.function_names.NewBound("$f"_sv);

  OK(Start{Var{u32{0}}}, Start{Var{"$f"_sv}});
}

TEST_F(TextResolveTest, ElementSegment) {
  context.function_names.NewBound("$f"_sv);
  context.table_names.NewBound("$t"_sv);
  context.global_names.NewBound("$g"_sv);

  OK(ElementSegment{nullopt, Var{u32{0}},
                    InstructionList{I{O::GlobalGet, Var{u32{0}}}},
                    ElementList{ElementListWithVars{ExternalKind::Function,
                                                    VarList{Var{u32{0}}}}}},
     ElementSegment{nullopt, Var{"$t"_sv},
                    InstructionList{I{O::GlobalGet, Var{"$g"_sv}}},
                    ElementList{ElementListWithVars{ExternalKind::Function,
                                                    VarList{Var{"$f"_sv}}}}});
}

TEST_F(TextResolveTest, DataSegment) {
  context.memory_names.NewBound("$m"_sv);
  context.global_names.NewBound("$g"_sv);

  OK(DataSegment{nullopt,
                 Var{u32{0}},
                 InstructionList{I{O::GlobalGet, Var{u32{0}}}},
                 {}},
     DataSegment{nullopt,
                 Var{"$m"_sv},
                 InstructionList{I{O::GlobalGet, Var{"$g"_sv}}},
                 {}});
}

TEST_F(TextResolveTest, Event) {
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}});

  OK(Event{EventDesc{nullopt,
                     EventType{EventAttribute::Exception,
                               FunctionTypeUse{Var{u32{0}},
                                               FunctionType{{VT::I32}, {}}}}},
           {}},
     Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                        FunctionTypeUse{Var{"$a"_sv}, {}}}},
           {}});
}

TEST_F(TextResolveTest, ModuleItem) {
  context.type_names.NewBound("$a");
  context.function_names.NewBound("$f");
  context.table_names.NewBound("$t");
  context.memory_names.NewBound("$m");
  context.global_names.NewBound("$g");

  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT::I32}}, {}});

  // TypeEntry.
  OK(ModuleItem{TypeEntry{}}, ModuleItem{TypeEntry{}});

  // Import.
  OK(ModuleItem{Import{
         Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
         FunctionDesc{nullopt, Var{u32{0}},
                      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}}}},
     ModuleItem{Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
                       FunctionDesc{nullopt, Var{"$a"_sv}, {}}}});

  // Function.
  OK(ModuleItem{Function{
         FunctionDesc{nullopt, Var{u32{0}},
                      BoundFunctionType{{BVT{nullopt, VT::I32}}, {}}},
         {BVT{"$l"_sv, VT::I32}},
         InstructionList{I{O::LocalGet, Var{u32{1}}}},
         {}}},
     ModuleItem{Function{FunctionDesc{nullopt, Var{"$a"_sv}, {}},
                         {BVT{"$l"_sv, VT::I32}},
                         InstructionList{I{O::LocalGet, Var{"$l"_sv}}},
                         {}}});

  // Table.
  OK(ModuleItem{Table{
         TableDesc{nullopt, TableType{Limits{0}, ReferenceType::Funcref}},
         {},
         ElementList{ElementListWithExpressions{
             ReferenceType::Funcref,
             ElementExpressionList{
                 ElementExpression{I{O::RefFunc, Var{u32{0}}}},
             }}}}},
     ModuleItem{
         Table{TableDesc{nullopt, TableType{Limits{0}, ReferenceType::Funcref}},
               {},
               ElementList{ElementListWithExpressions{
                   ReferenceType::Funcref,
                   ElementExpressionList{
                       ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
                   }}}}});

  // Memory.
  OK(ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{0}}}, {}}},
     ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{0}}}, {}}});

  // Global.
  OK(ModuleItem{Global{
         GlobalDesc{nullopt, GlobalType{ValueType::I32, Mutability::Const}},
         InstructionList{I{O::GlobalGet, Var{u32{0}}}},
         {},
     }},
     ModuleItem{Global{
         GlobalDesc{nullopt, GlobalType{ValueType::I32, Mutability::Const}},
         InstructionList{I{O::GlobalGet, Var{"$g"_sv}}},
         {},
     }});

  // Export.
  OK(ModuleItem{Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{u32{0}}}},
     ModuleItem{
         Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{"$f"_sv}}});

  // Start.
  OK(ModuleItem{Start{Var{u32{0}}}}, ModuleItem{Start{Var{"$f"_sv}}});

  // ElementSegment.
  OK(ModuleItem{ElementSegment{
         nullopt, Var{u32{0}}, InstructionList{I{O::GlobalGet, Var{u32{0}}}},
         ElementList{ElementListWithVars{ExternalKind::Function,
                                         VarList{Var{u32{0}}}}}}},
     ModuleItem{ElementSegment{
         nullopt, Var{"$t"_sv}, InstructionList{I{O::GlobalGet, Var{"$g"_sv}}},
         ElementList{ElementListWithVars{ExternalKind::Function,
                                         VarList{Var{"$f"_sv}}}}}});

  // DataSegment.
  OK(ModuleItem{DataSegment{nullopt,
                            Var{u32{0}},
                            InstructionList{I{O::GlobalGet, Var{u32{0}}}},
                            {}}},
     ModuleItem{DataSegment{nullopt,
                            Var{"$m"_sv},
                            InstructionList{I{O::GlobalGet, Var{"$g"_sv}}},
                            {}}});

  // Event.
  OK(ModuleItem{Event{
         EventDesc{nullopt,
                   EventType{EventAttribute::Exception,
                             FunctionTypeUse{Var{u32{0}},
                                             FunctionType{{VT::I32}, {}}}}},
         {}}},
     ModuleItem{
         Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                            FunctionTypeUse{Var{"$a"_sv}, {}}}},
               {}}});
}
