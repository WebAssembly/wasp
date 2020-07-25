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
#include "test/text/constants.h"
#include "wasp/base/errors.h"
#include "wasp/text/formatters.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/name_map.h"
#include "wasp/text/resolve_context.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;
using namespace ::wasp::test;


namespace {

const SpanU8 loc1 = "A"_su8;

}  // namespace

class TextResolveTest : public ::testing::Test {
 protected:
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

  template <typename T, typename... Args>
  void Fail(const ErrorList& expected_error, const T& value, Args&&... args) {
    T copy = value;
    Resolve(context, copy, std::forward<Args>(args)...);
    ExpectError(expected_error, errors);
    errors.Clear();
  }

  template <typename T>
  void OKDefine(const T& value) {
    Define(context, value);
    ExpectNoErrors(errors);
  }

  template <typename T>
  void FailDefine(const ErrorList& expected_error, const T& value) {
    Define(context, value);
    ExpectError(expected_error, errors);
    errors.Clear();
  }

  TestErrors errors;
  ResolveContext context{errors};
};

TEST_F(TextResolveTest, Var_Undefined) {
  NameMap name_map;  // Empty name map.
  Fail({{loc1, "Undefined variable $a"}}, MakeAt(loc1, Var{"$a"_sv}), name_map);
}

TEST_F(TextResolveTest, FunctionTypeUse) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  context.type_names.NewBound("$b"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_F32}}, {}});

  // Resolve the variable name to an index, and populate the function type.
  OK(FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}},
     FunctionTypeUse{Var{"$a"_sv}, {}});

  // Just populate the function type.
  OK(FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}},
     FunctionTypeUse{Var{Index{0}}, {}});

  // Populate the variable when not specified.
  OK(FunctionTypeUse{Var{Index{1}}, FunctionType{{VT_F32}, {}}},
     FunctionTypeUse{nullopt, FunctionType{{VT_F32}, {}}});
}

TEST_F(TextResolveTest, FunctionTypeUse_ReuseType) {
  auto bound_function_type = BoundFunctionType{{BVT{nullopt, VT_I32}}, {}};
  context.function_type_map.Define(bound_function_type);

  OK(FunctionDesc{nullopt, Var{Index{0}}, bound_function_type},
     FunctionDesc{nullopt, nullopt, bound_function_type});

  ASSERT_EQ(1u, context.function_type_map.Size());
}

TEST_F(TextResolveTest, FunctionTypeUse_DeferType) {
  FunctionTypeMap& ftm = context.function_type_map;

  ftm.Define(BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});
  ftm.Define(BoundFunctionType{{BVT{nullopt, VT_I64}}, {}});

  OK(FunctionDesc{nullopt, Var{Index{2}},
                  BoundFunctionType{{BVT{nullopt, VT_F32}}, {}}},
     FunctionDesc{nullopt, nullopt,
                  BoundFunctionType{{BVT{nullopt, VT_F32}}, {}}});

  auto type_entries = ftm.EndModule();

  ASSERT_EQ(3u, ftm.Size());

  // Implicitly defined after other explicitly defined types.
  EXPECT_EQ((FunctionType{{VT_F32}, {}}), ftm.Get(2));

  // Generated type entry.
  ASSERT_EQ(1u, type_entries.size());
  EXPECT_EQ((TypeEntry{nullopt, BoundFunctionType{{BVT{nullopt, VT_F32}}, {}}}),
            type_entries[0]);
}

TEST_F(TextResolveTest, FunctionTypeUse_NoFunctionTypeInContext) {
  FunctionTypeUse type_use;
  Resolve(context, type_use);
  EXPECT_EQ((FunctionTypeUse{Var{Index{0}}, {}}), type_use);
}

TEST_F(TextResolveTest, BoundFunctionTypeUse_NoFunctionTypeInContext) {
  OptAt<Var> type_use;
  At<BoundFunctionType> type;
  Resolve(context, type_use, type);
  EXPECT_EQ(Var{Index{0}}, type_use);
  EXPECT_EQ(MakeAt(BoundFunctionType{}), type);
}

TEST_F(TextResolveTest, BlockImmediate) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(BlockImmediate{nullopt,
                    FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}},
     BlockImmediate{nullopt, FunctionTypeUse{Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, BlockImmediate_InlineType) {
  // An inline type can only be void, or a single result type.
  OK(BlockImmediate{nullopt, {}}, BlockImmediate{nullopt, {}});

  for (auto value_type : {VT_I32, VT_I64, VT_F32, VT_F64, VT_V128, VT_Funcref,
                          VT_Externref, VT_Exnref}) {
    OK(BlockImmediate{nullopt,
                      FunctionTypeUse{nullopt, FunctionType{{}, {value_type}}}},
       BlockImmediate{
           nullopt, FunctionTypeUse{nullopt, FunctionType{{}, {value_type}}}});
  }

  // None of the inline block types should extend the context's function type
  // map.
  auto type_entries = context.function_type_map.EndModule();
  EXPECT_EQ(0u, context.function_type_map.Size());
  EXPECT_EQ(0u, type_entries.size());
}

TEST_F(TextResolveTest, BrOnExnImmediate) {
  context.label_names.Push();
  context.label_names.NewBound("$l");
  context.event_names.NewBound("$e");

  OK(BrOnExnImmediate{Var{Index{0}}, Var{Index{0}}},
     BrOnExnImmediate{Var{"$l"_sv}, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, BrTableImmediate) {
  context.label_names.Push();
  context.label_names.NewBound("$l0");
  context.label_names.Push();
  context.label_names.NewBound("$l1");
  context.label_names.Push();
  context.label_names.NewUnbound();
  context.label_names.Push();
  context.label_names.NewBound("$l3");

  OK(BrTableImmediate{{
                          Var{Index{3}},
                          Var{Index{2}},
                          Var{Index{1}},
                      },
                      Var{Index{0}}},
     BrTableImmediate{{
                          Var{"$l0"_sv},
                          Var{"$l1"_sv},
                          Var{Index{1}},
                      },
                      Var{"$l3"_sv}});
}

TEST_F(TextResolveTest, CallIndirectImmediate) {
  context.table_names.NewBound("$t");
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(CallIndirectImmediate{Var{Index{0}},
                           FunctionTypeUse{Var{Index{0}},
                                           FunctionType{{VT_I32}, {}}}},
     CallIndirectImmediate{Var{"$t"_sv}, FunctionTypeUse{Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, Instruction_NoOp) {
  // Bare.
  OK(I{O::Nop}, I{O::Nop});

  // s32 Immediate.
  OK(I{O::I32Const, s32{}}, I{O::I32Const, s32{}});

  // s64 Immediate.
  OK(I{O::I64Const, s64{}}, I{O::I64Const, s64{}});

  // f32 Immediate.
  OK(I{O::F32Const, f32{}}, I{O::F32Const, f32{}});

  // f64 Immediate.
  OK(I{O::F64Const, f64{}}, I{O::F64Const, f64{}});

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
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(I{O::Block,
       BlockImmediate{nullopt, FunctionTypeUse{Var{Index{0}},
                                               FunctionType{{VT_I32}, {}}}}},
     I{O::Block, BlockImmediate{nullopt, FunctionTypeUse{Var{"$a"_sv}, {}}}});

  // Populate the type use.
  OK(I{O::Block,
       BlockImmediate{nullopt, FunctionTypeUse{Var{Index{0}},
                                               FunctionType{{VT_I32}, {}}}}},
     I{O::Block,
       BlockImmediate{nullopt,
                      FunctionTypeUse{nullopt, FunctionType{{VT_I32}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_BrOnExnImmediate) {
  context.label_names.Push();
  context.label_names.NewBound("$l");
  context.event_names.NewBound("$e");

  OK(I{O::BrOnExn, BrOnExnImmediate{Var{Index{0}}, Var{Index{0}}}},
     I{O::BrOnExn, BrOnExnImmediate{Var{"$l"_sv}, Var{"$e"_sv}}});
}

TEST_F(TextResolveTest, Instruction_BrTableImmediate) {
  context.label_names.Push();
  context.label_names.NewBound("$l0");
  context.label_names.Push();
  context.label_names.NewBound("$l1");

  OK(I{O::BrTable, BrTableImmediate{{Var{Index{1}}}, Var{Index{0}}}},
     I{O::BrTable, BrTableImmediate{{Var{"$l0"_sv}}, Var{"$l1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_CallIndirectImmediate) {
  context.table_names.NewBound("$t");
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(I{O::CallIndirect,
       CallIndirectImmediate{
           Var{Index{0}},
           FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}}},
     I{O::CallIndirect,
       CallIndirectImmediate{Var{"$t"_sv}, FunctionTypeUse{Var{"$a"_sv}, {}}}});

  // Populate the type use.
  OK(I{O::CallIndirect,
       CallIndirectImmediate{
           Var{Index{0}},
           FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}}},
     I{O::CallIndirect,
       CallIndirectImmediate{
           Var{"$t"_sv},
           FunctionTypeUse{nullopt, FunctionType{{VT_I32}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_CopyImmediate_Table) {
  context.table_names.NewBound("$t0");
  context.table_names.NewBound("$t1");

  OK(I{O::TableCopy, CopyImmediate{Var{Index{0}}, Var{Index{1}}}},
     I{O::TableCopy, CopyImmediate{Var{"$t0"_sv}, Var{"$t1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_CopyImmediate_Memory) {
  context.memory_names.NewBound("$m0");
  context.memory_names.NewBound("$m1");

  OK(I{O::MemoryCopy, CopyImmediate{Var{Index{0}}, Var{Index{1}}}},
     I{O::MemoryCopy, CopyImmediate{Var{"$m0"_sv}, Var{"$m1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_InitImmediate_Table) {
  context.element_segment_names.NewBound("$e");
  context.table_names.NewBound("$t");

  OK(I{O::TableInit, InitImmediate{Var{Index{0}}, Var{Index{0}}}},
     I{O::TableInit, InitImmediate{Var{"$e"_sv}, Var{"$t"_sv}}});
}

TEST_F(TextResolveTest, Instruction_InitImmediate_Memory) {
  context.data_segment_names.NewBound("$d");
  context.memory_names.NewBound("$m");

  OK(I{O::MemoryInit, InitImmediate{Var{Index{0}}, Var{Index{0}}}},
     I{O::MemoryInit, InitImmediate{Var{"$d"_sv}, Var{"$m"_sv}}});
}

TEST_F(TextResolveTest, Instruction_Var_Function) {
  context.function_names.NewBound("$f");

  OK(I{O::Call, Var{Index{0}}}, I{O::Call, Var{"$f"_sv}});
  OK(I{O::ReturnCall, Var{Index{0}}}, I{O::ReturnCall, Var{"$f"_sv}});
  OK(I{O::RefFunc, Var{Index{0}}}, I{O::RefFunc, Var{"$f"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Table) {
  context.table_names.NewBound("$t");

  OK(I{O::TableFill, Var{Index{0}}}, I{O::TableFill, Var{"$t"_sv}});
  OK(I{O::TableGet, Var{Index{0}}}, I{O::TableGet, Var{"$t"_sv}});
  OK(I{O::TableGrow, Var{Index{0}}}, I{O::TableGrow, Var{"$t"_sv}});
  OK(I{O::TableSet, Var{Index{0}}}, I{O::TableSet, Var{"$t"_sv}});
  OK(I{O::TableSize, Var{Index{0}}}, I{O::TableSize, Var{"$t"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Global) {
  context.global_names.NewBound("$t");

  OK(I{O::GlobalGet, Var{Index{0}}}, I{O::GlobalGet, Var{"$t"_sv}});
  OK(I{O::GlobalSet, Var{Index{0}}}, I{O::GlobalSet, Var{"$t"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Event) {
  context.event_names.NewBound("$e");

  OK(I{O::Throw, Var{Index{0}}}, I{O::Throw, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Element) {
  context.element_segment_names.NewBound("$e");

  OK(I{O::ElemDrop, Var{Index{0}}}, I{O::ElemDrop, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Data) {
  context.data_segment_names.NewBound("$d");

  OK(I{O::DataDrop, Var{Index{0}}}, I{O::DataDrop, Var{"$d"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Label) {
  context.label_names.Push();
  context.label_names.NewBound("$l");

  OK(I{O::BrIf, Var{Index{0}}}, I{O::BrIf, Var{"$l"_sv}});
  OK(I{O::Br, Var{Index{0}}}, I{O::Br, Var{"$l"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Local) {
  context.local_names.NewBound("$l");

  OK(I{O::LocalGet, Var{Index{0}}}, I{O::LocalGet, Var{"$l"_sv}});
  OK(I{O::LocalSet, Var{Index{0}}}, I{O::LocalSet, Var{"$l"_sv}});
  OK(I{O::LocalTee, Var{Index{0}}}, I{O::LocalTee, Var{"$l"_sv}});
}

TEST_F(TextResolveTest, InstructionList) {
  context.function_names.NewBound("$f");
  context.local_names.NewBound("$l");

  OK(
      InstructionList{
          I{O::LocalGet, Var{Index{0}}},
          I{O::LocalSet, Var{Index{0}}},
          I{O::Call, Var{Index{0}}},
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
          I{O::Br, Var{Index{0}}},
          I{O::Br, Var{Index{1}}},
          I{O::End},
          I{O::Block, BlockImmediate{"$l0"_sv, {}}},
          I{O::Br, Var{Index{0}}},
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

TEST_F(TextResolveTest, InstructionList_LabelDuplicate) {
  context.function_type_map.Define(BoundFunctionType{});

  OK(
      InstructionList{
          I{O::Block, BlockImmediate{"$l"_sv, {}}},
          I{O::Block, BlockImmediate{"$l"_sv, {}}},
          I{O::Br, Var{Index{0}}},
          I{O::End},
          I{O::Br, Var{Index{0}}},
          I{O::End},
      },
      InstructionList{
          I{O::Block, BlockImmediate{"$l"_sv, {}}},
          I{O::Block, BlockImmediate{"$l"_sv, {}}},
          I{O::Br, Var{"$l"_sv}},
          I{O::End},
          I{O::Br, Var{"$l"_sv}},
          I{O::End},
      });
}

TEST_F(TextResolveTest, InstructionList_EndBlock) {
  context.function_type_map.Define(BoundFunctionType{});

  OK(
      InstructionList{
          I{O::Block, BlockImmediate{"$outer"_sv, {}}},
          I{O::Block, BlockImmediate{"$inner"_sv, {}}},
          I{O::Br, Var{Index{0}}},
          I{O::Br, Var{Index{1}}},
          I{O::End},
          I{O::Br, Var{Index{0}}},
          I{O::End},
      },
      InstructionList{
          I{O::Block, BlockImmediate{"$outer"_sv, {}}},
          I{O::Block, BlockImmediate{"$inner"_sv, {}}},
          I{O::Br, Var{"$inner"_sv}},
          I{O::Br, Var{"$outer"_sv}},
          I{O::End},
          I{O::Br, Var{"$outer"_sv}},
          I{O::End},
      });
}

TEST_F(TextResolveTest, TypeEntry_DuplicateName) {
  context.type_names.NewBound("$t"_sv);

  FailDefine({{loc1, "Variable $t is already bound to index 0"}},
             TypeEntry{MakeAt(loc1, "$t"_sv), BoundFunctionType{}});
}

TEST_F(TextResolveTest, TypeEntry_DistinctTypes) {
  OKDefine(TypeEntry{"$a"_sv, BoundFunctionType{}});
  OKDefine(TypeEntry{"$b"_sv, BoundFunctionType{}});

  ASSERT_EQ(2u, context.function_type_map.Size());
}

TEST_F(TextResolveTest, FunctionDesc) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // $p param name is not copied.
  OK(FunctionDesc{nullopt, Var{Index{0}},
                  BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}},
     FunctionDesc{nullopt, Var{"$a"_sv}, {}});

  // Populate the type use.
  OK(FunctionDesc{nullopt, Var{Index{0}},
                  BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}},
     FunctionDesc{nullopt, nullopt,
                  BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}});
}

TEST_F(TextResolveTest, FunctionDesc_DuplicateName) {
  context.function_names.NewBound("$f"_sv);

  FailDefine({{loc1, "Variable $f is already bound to index 0"}},
             FunctionDesc{MakeAt(loc1, "$f"_sv), nullopt, {}});
}

TEST_F(TextResolveTest, FunctionDesc_DuplicateParamName) {
  Fail({{loc1, "Variable $foo is already bound to index 0"}},
       FunctionDesc{nullopt, nullopt,
                    BoundFunctionType{{BVT{"$foo"_sv, VT_I32},
                                       BVT{MakeAt(loc1, "$foo"_sv), VT_I64}},
                                      {}}});
}

TEST_F(TextResolveTest, EventType) {
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(EventType{EventAttribute::Exception,
               FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}},
     EventType{EventAttribute::Exception, FunctionTypeUse{Var{"$a"_sv}, {}}});

  // Populate the type use.
  OK(EventType{EventAttribute::Exception,
               FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}},
     EventType{EventAttribute::Exception,
               FunctionTypeUse{nullopt, FunctionType{{VT_I32}, {}}}});
}

TEST_F(TextResolveTest, EventDesc) {
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(EventDesc{nullopt, EventType{EventAttribute::Exception,
                                  FunctionTypeUse{Var{Index{0}},
                                                  FunctionType{{VT_I32}, {}}}}},
     EventDesc{nullopt, EventType{EventAttribute::Exception,
                                  FunctionTypeUse{Var{"$a"_sv}, {}}}});
}

TEST_F(TextResolveTest, Import_Function) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // $p param name is not copied.
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            FunctionDesc{nullopt, Var{Index{0}},
                         BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            FunctionDesc{nullopt, Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, Import_Table) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}}});
}

TEST_F(TextResolveTest, Import_Memory) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            MemoryDesc{nullopt, MemoryType{Limits{0}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            MemoryDesc{nullopt, MemoryType{Limits{0}}}});
}

TEST_F(TextResolveTest, Import_Global) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}}});
}

TEST_F(TextResolveTest, Import_Event) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // $p param name is not copied.
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            EventDesc{nullopt,
                      EventType{EventAttribute::Exception,
                                FunctionTypeUse{Var{Index{0}},
                                                FunctionType{{VT_I32}, {}}}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            EventDesc{nullopt, EventType{EventAttribute::Exception,
                                         FunctionTypeUse{Var{"$a"_sv}, {}}}}});
}

TEST_F(TextResolveTest, Function) {
  context.type_names.NewBound("$a"_sv);
  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  OK(Function{FunctionDesc{nullopt, Var{Index{0}},
                           BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}},
              {BVT{"$l"_sv, VT_I32}},
              InstructionList{I{O::LocalGet, Var{Index{1}}}},
              {}},
     Function{FunctionDesc{nullopt, Var{"$a"_sv}, {}},
              {BVT{"$l"_sv, VT_I32}},
              InstructionList{I{O::LocalGet, Var{"$l"_sv}}},
              {}});
}

TEST_F(TextResolveTest, Function_DuplicateLocalName) {
  Fail({{loc1, "Variable $foo is already bound to index 0"}},
       Function{FunctionDesc{},
                {BVT{"$foo"_sv, VT_I32}, BVT{MakeAt(loc1, "$foo"_sv), VT_I64}},
                InstructionList{},
                {}});
}

TEST_F(TextResolveTest, Function_DuplicateParamLocalNames) {
  Fail({{loc1, "Variable $foo is already bound to index 0"}},
       Function{FunctionDesc{nullopt, nullopt,
                             BoundFunctionType{{BVT{"$foo"_sv, VT_I32}}, {}}},
                {BVT{MakeAt(loc1, "$foo"_sv), VT_I64}},
                InstructionList{},
                {}});
}

TEST_F(TextResolveTest, ElementExpressionList) {
  context.function_names.NewBound("$f"_sv);

  OK(
      ElementExpressionList{
          ElementExpression{I{O::RefNull}},
          ElementExpression{I{O::RefFunc, Var{Index{0}}}},
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
          RT_Funcref,
          ElementExpressionList{
              ElementExpression{I{O::RefNull}},
              ElementExpression{I{O::RefFunc, Var{Index{0}}}},
          }},
      ElementListWithExpressions{
          RT_Funcref,
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
                             Var{Index{0}},
                             Var{Index{1}},
                         }},
     ElementListWithVars{ExternalKind::Function, VarList{
                                                     Var{"$f"_sv},
                                                     Var{Index{1}},
                                                 }});
}

TEST_F(TextResolveTest, ElementList) {
  context.function_names.NewBound("$f"_sv);
  context.function_names.NewUnbound();

  // Expressions.
  OK(ElementList{ElementListWithExpressions{
         RT_Funcref,
         ElementExpressionList{
             ElementExpression{I{O::RefNull}},
             ElementExpression{I{O::RefFunc, Var{Index{0}}}},
         }}},
     ElementList{ElementListWithExpressions{
         RT_Funcref,
         ElementExpressionList{
             ElementExpression{I{O::RefNull}},
             ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
         }}});

  // Vars.
  OK(ElementList{ElementListWithVars{ExternalKind::Function,
                                     VarList{
                                         Var{Index{0}},
                                         Var{Index{1}},
                                     }}},
     ElementList{ElementListWithVars{ExternalKind::Function, VarList{
                                                                 Var{"$f"_sv},
                                                                 Var{Index{1}},
                                                             }}});
}

TEST_F(TextResolveTest, Table) {
  context.function_names.NewBound("$f"_sv);

  OK(Table{TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}},
           {},
           ElementList{ElementListWithExpressions{
               RT_Funcref,
               ElementExpressionList{
                   ElementExpression{I{O::RefFunc, Var{Index{0}}}},
               }}}},
     Table{TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}},
           {},
           ElementList{ElementListWithExpressions{
               RT_Funcref, ElementExpressionList{
                               ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
                           }}}});
}

TEST_F(TextResolveTest, Table_DuplicateName) {
  context.table_names.NewBound("$t"_sv);

  FailDefine(
      {{loc1, "Variable $t is already bound to index 0"}},
      TableDesc{MakeAt(loc1, "$t"_sv), TableType{Limits{0}, RT_Funcref}});
}

TEST_F(TextResolveTest, Memory_DuplicateName) {
  context.memory_names.NewBound("$m"_sv);

  FailDefine({{loc1, "Variable $m is already bound to index 0"}},
             MemoryDesc{MakeAt(loc1, "$m"_sv), MemoryType{Limits{0}}});
}

TEST_F(TextResolveTest, Global) {
  context.global_names.NewBound("$g"_sv);

  OK(
      Global{
          GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
          ConstantExpression{I{O::GlobalGet, Var{Index{0}}}},
          {},
      },
      Global{
          GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
          ConstantExpression{I{O::GlobalGet, Var{"$g"_sv}}},
          {},
      });
}

TEST_F(TextResolveTest, Global_DuplicateName) {
  context.global_names.NewBound("$g"_sv);

  FailDefine(
      {{loc1, "Variable $g is already bound to index 0"}},
      GlobalDesc{MakeAt(loc1, "$g"_sv), GlobalType{VT_I32, Mutability::Const}});
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

  OK(Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{Index{0}}},
     Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{"$f"_sv}});

  OK(Export{ExternalKind::Table, Text{"\"t\"", 1}, Var{Index{1}}},
     Export{ExternalKind::Table, Text{"\"t\"", 1}, Var{"$t"_sv}});

  OK(Export{ExternalKind::Memory, Text{"\"m\"", 1}, Var{Index{2}}},
     Export{ExternalKind::Memory, Text{"\"m\"", 1}, Var{"$m"_sv}});

  OK(Export{ExternalKind::Global, Text{"\"g\"", 1}, Var{Index{3}}},
     Export{ExternalKind::Global, Text{"\"g\"", 1}, Var{"$g"_sv}});

  OK(Export{ExternalKind::Event, Text{"\"e\"", 1}, Var{Index{4}}},
     Export{ExternalKind::Event, Text{"\"e\"", 1}, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Start) {
  context.function_names.NewBound("$f"_sv);

  OK(Start{Var{Index{0}}}, Start{Var{"$f"_sv}});
}

TEST_F(TextResolveTest, ElementSegment) {
  context.function_names.NewBound("$f"_sv);
  context.table_names.NewBound("$t"_sv);
  context.global_names.NewBound("$g"_sv);

  OK(ElementSegment{nullopt, Var{Index{0}},
                    ConstantExpression{I{O::GlobalGet, Var{Index{0}}}},
                    ElementList{ElementListWithVars{ExternalKind::Function,
                                                    VarList{Var{Index{0}}}}}},
     ElementSegment{nullopt, Var{"$t"_sv},
                    ConstantExpression{I{O::GlobalGet, Var{"$g"_sv}}},
                    ElementList{ElementListWithVars{ExternalKind::Function,
                                                    VarList{Var{"$f"_sv}}}}});
}

TEST_F(TextResolveTest, ElementSegment_DuplicateName) {
  context.element_segment_names.NewBound("$e"_sv);

  FailDefine({{loc1, "Variable $e is already bound to index 0"}},
             ElementSegment{MakeAt(loc1, "$e"_sv), nullopt,
                            ConstantExpression{}, ElementList{}});
}

TEST_F(TextResolveTest, DataSegment) {
  context.memory_names.NewBound("$m"_sv);
  context.global_names.NewBound("$g"_sv);

  OK(DataSegment{nullopt,
                 Var{Index{0}},
                 ConstantExpression{I{O::GlobalGet, Var{Index{0}}}},
                 {}},
     DataSegment{nullopt,
                 Var{"$m"_sv},
                 ConstantExpression{I{O::GlobalGet, Var{"$g"_sv}}},
                 {}});
}

TEST_F(TextResolveTest, DataSegment_DuplicateName) {
  context.data_segment_names.NewBound("$d"_sv);

  FailDefine(
      {{loc1, "Variable $d is already bound to index 0"}},
      DataSegment{MakeAt(loc1, "$d"_sv), nullopt, ConstantExpression{}, {}});
}

TEST_F(TextResolveTest, Event) {
  context.type_names.NewBound("$a");
  context.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(Event{EventDesc{nullopt,
                     EventType{EventAttribute::Exception,
                               FunctionTypeUse{Var{Index{0}},
                                               FunctionType{{VT_I32}, {}}}}},
           {}},
     Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                        FunctionTypeUse{Var{"$a"_sv}, {}}}},
           {}});
}

TEST_F(TextResolveTest, Event_DuplicateName) {
  context.event_names.NewBound("$e"_sv);

  FailDefine(
      {{loc1, "Variable $e is already bound to index 0"}},
      EventDesc{MakeAt(loc1, "$e"_sv),
                EventType{EventAttribute::Exception, FunctionTypeUse{}}});
}

TEST_F(TextResolveTest, ModuleItem) {
  context.type_names.NewBound("$a");
  context.function_names.NewBound("$f");
  context.table_names.NewBound("$t");
  context.memory_names.NewBound("$m");
  context.global_names.NewBound("$g");

  context.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // TypeEntry.
  OK(ModuleItem{TypeEntry{}}, ModuleItem{TypeEntry{}});

  // Import.
  OK(ModuleItem{Import{
         Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
         FunctionDesc{nullopt, Var{Index{0}},
                      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}}}},
     ModuleItem{Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
                       FunctionDesc{nullopt, Var{"$a"_sv}, {}}}});

  // Function.
  OK(ModuleItem{Function{
         FunctionDesc{nullopt, Var{Index{0}},
                      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}},
         {BVT{"$l"_sv, VT_I32}},
         InstructionList{I{O::LocalGet, Var{Index{1}}}},
         {}}},
     ModuleItem{Function{FunctionDesc{nullopt, Var{"$a"_sv}, {}},
                         {BVT{"$l"_sv, VT_I32}},
                         InstructionList{I{O::LocalGet, Var{"$l"_sv}}},
                         {}}});

  // Table.
  OK(ModuleItem{Table{TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}},
                      {},
                      ElementList{ElementListWithExpressions{
                          RT_Funcref,
                          ElementExpressionList{
                              ElementExpression{I{O::RefFunc, Var{Index{0}}}},
                          }}}}},
     ModuleItem{Table{
         TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}},
         {},
         ElementList{ElementListWithExpressions{
             RT_Funcref, ElementExpressionList{
                             ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
                         }}}}});

  // Memory.
  OK(ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{0}}}, {}}},
     ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{0}}}, {}}});

  // Global.
  OK(ModuleItem{Global{
         GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
         ConstantExpression{I{O::GlobalGet, Var{Index{0}}}},
         {},
     }},
     ModuleItem{Global{
         GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
         ConstantExpression{I{O::GlobalGet, Var{"$g"_sv}}},
         {},
     }});

  // Export.
  OK(ModuleItem{Export{ExternalKind::Function, Text{"\"f\"", 1},
                       Var{Index{0}}}},
     ModuleItem{
         Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{"$f"_sv}}});

  // Start.
  OK(ModuleItem{Start{Var{Index{0}}}}, ModuleItem{Start{Var{"$f"_sv}}});

  // ElementSegment.
  OK(ModuleItem{ElementSegment{
         nullopt, Var{Index{0}},
         ConstantExpression{I{O::GlobalGet, Var{Index{0}}}},
         ElementList{ElementListWithVars{ExternalKind::Function,
                                         VarList{Var{Index{0}}}}}}},
     ModuleItem{
         ElementSegment{nullopt, Var{"$t"_sv},
                        ConstantExpression{I{O::GlobalGet, Var{"$g"_sv}}},
                        ElementList{ElementListWithVars{
                            ExternalKind::Function, VarList{Var{"$f"_sv}}}}}});

  // DataSegment.
  OK(ModuleItem{DataSegment{nullopt,
                            Var{Index{0}},
                            ConstantExpression{I{O::GlobalGet, Var{Index{0}}}},
                            {}}},
     ModuleItem{DataSegment{nullopt,
                            Var{"$m"_sv},
                            ConstantExpression{I{O::GlobalGet, Var{"$g"_sv}}},
                            {}}});

  // Event.
  OK(ModuleItem{Event{
         EventDesc{nullopt,
                   EventType{EventAttribute::Exception,
                             FunctionTypeUse{Var{Index{0}},
                                             FunctionType{{VT_I32}, {}}}}},
         {}}},
     ModuleItem{
         Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                            FunctionTypeUse{Var{"$a"_sv}, {}}}},
               {}}});
}

TEST_F(TextResolveTest, ModuleWithDeferredTypes) {
  OK(
      Module{
          // (func (type 0))
          ModuleItem{
              Function{FunctionDesc{nullopt, Var{Index{0}}, {}}, {}, {}, {}}},
          // (func (type 1) (param i32))
          ModuleItem{Function{FunctionDesc{
                                  nullopt,
                                  Var{Index{1}},
                                  BoundFunctionType{{BVT{nullopt, VT_I32}}, {}},
                              },
                              {},
                              {},
                              {}}},

          // The deferred type entries.
          // (type (func))
          ModuleItem{TypeEntry{nullopt, BoundFunctionType{}}},
          // (type (func (param i32))
          ModuleItem{TypeEntry{
              nullopt,
              BoundFunctionType{BoundValueTypeList{BVT{nullopt, VT_I32}}, {}}}},
      },
      Module{
          // (func)
          ModuleItem{Function{}},
          // (func (param i32))
          ModuleItem{Function{FunctionDesc{
                                  nullopt,
                                  nullopt,
                                  BoundFunctionType{{BVT{nullopt, VT_I32}}, {}},
                              },
                              {},
                              {},
                              {}}},
      });
}
