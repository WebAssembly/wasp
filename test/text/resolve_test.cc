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
#include "wasp/text/read/name_map.h"
#include "wasp/text/read/read_ctx.h"
#include "wasp/text/resolve_ctx.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;
using namespace ::wasp::test;

namespace {

const SpanU8 loc1 = "A"_su8;

// These constants are similar to their equivalents in text/constants.h, but
// they have a different location. For most tests below, the assumption is made
// that type $t maps to index 0.
const HeapType Resolved_HT_0{At{"$t"_su8, Var{Index{0}}}};
const RefType Resolved_RefType_0{At{"$t"_su8, Resolved_HT_0}, Null::No};
const ReferenceType Resolved_RT_Ref0{At{"$t"_su8, Resolved_RefType_0}};
const Rtt Resolved_RTT_0_0{At{"0"_su8, 0u}, At{"$t"_su8, Resolved_HT_0}};
const Rtt Resolved_RTT_1_0{At{"1"_su8, 1u}, At{"$t"_su8, Resolved_HT_0}};
const ValueType Resolved_VT_Ref0{At{"(ref $t)"_su8, Resolved_RT_Ref0}};
const ValueType Resolved_VT_RTT_0_0{At{"(rtt 0 $t)"_su8, Resolved_RTT_0_0}};

}  // namespace

class TextResolveTest : public ::testing::Test {
 protected:
  using BVT = BoundValueType;
  using I = Instruction;
  using O = Opcode;

  template <typename T>
  void OK(const T& expected, const T& before) {
    T actual = before;
    Resolve(ctx, actual);
    EXPECT_EQ(expected, actual);
    ExpectNoErrors(errors);
  }

  template <typename T, typename... Args>
  void Fail(const ErrorList& expected_error, const T& value, Args&&... args) {
    T copy = value;
    Resolve(ctx, copy, std::forward<Args>(args)...);
    ExpectError(expected_error, errors);
    errors.Clear();
  }

  template <typename T>
  void OKDefine(const T& value) {
    Define(ctx, value);
    ExpectNoErrors(errors);
  }

  template <typename T>
  void FailDefine(const ErrorList& expected_error, const T& value) {
    Define(ctx, value);
    ExpectError(expected_error, errors);
    errors.Clear();
  }

  template <typename T>
  void FailDefineTypes(const ErrorList& expected_error, const T& value) {
    DefineTypes(ctx, value);
    ExpectError(expected_error, errors);
    errors.Clear();
  }

  TestErrors errors;
  ResolveCtx ctx{errors};
};

TEST_F(TextResolveTest, Var_Undefined) {
  NameMap name_map;  // Empty name map.
  Fail({{loc1, "Undefined variable $a"}}, At{loc1, Var{"$a"_sv}}, name_map);
}

TEST_F(TextResolveTest, HeapType) {
  ctx.type_names.NewBound("$t"_sv);

  // HeapKind
  OK(HeapType{HeapKind::Func}, HeapType{HeapKind::Func});

  // Var
  OK(Resolved_HT_0, HT_T);
}

TEST_F(TextResolveTest, RefType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(Resolved_RefType_0, RefType_T);
}

TEST_F(TextResolveTest, ReferenceType) {
  ctx.type_names.NewBound("$t"_sv);

  // ReferenceKind
  OK(RT_Funcref, RT_Funcref);

  // RefType
  OK(Resolved_RT_Ref0, RT_RefT);
}

TEST_F(TextResolveTest, Rtt) {
  ctx.type_names.NewBound("$t"_sv);

  // RTTs that don't need resolving.
  OK(RTT_0_Func, RTT_0_Func);
  OK(RTT_0_Extern, RTT_0_Extern);
  OK(RTT_0_Eq, RTT_0_Eq);
  OK(RTT_0_I31, RTT_0_I31);
  OK(RTT_0_Any, RTT_0_Any);
  OK(RTT_0_0, RTT_0_0);

  // RTTs that need to be resolved.
  OK(Resolved_RTT_0_0, RTT_0_T);
  OK(Resolved_RTT_1_0, RTT_1_T);
}

TEST_F(TextResolveTest, ValueType) {
  ctx.type_names.NewBound("$t"_sv);

  // NumericType
  OK(VT_I32, VT_I32);

  // ReferenceType
  OK(Resolved_VT_Ref0, VT_RefT);

  // Rtt
  OK(Resolved_VT_RTT_0_0, VT_RTT_0_T);
}

TEST_F(TextResolveTest, ValueTypeList) {
  ctx.type_names.NewBound("$t"_sv);

  OK(
      // [i32, ref 0]
      ValueTypeList{VT_I32, Resolved_VT_Ref0},
      // [i32, ref $t]
      ValueTypeList{VT_I32, VT_RefT});
}

TEST_F(TextResolveTest, StorageType) {
  ctx.type_names.NewBound("$t"_sv);

  // ValueType
  OK(StorageType{VT_I32}, StorageType{VT_I32});
  OK(StorageType{Resolved_VT_Ref0}, StorageType{VT_RefT});

  // PackedType
  OK(StorageType{PackedType::I8}, StorageType{PackedType::I8});
  OK(StorageType{PackedType::I16}, StorageType{PackedType::I16});
}

TEST_F(TextResolveTest, FunctionType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(
      // (func (param i32) (result ref 0))
      FunctionType{{VT_I32}, {Resolved_VT_Ref0}},
      // (func (param i32) (result ref $t))
      FunctionType{{VT_I32}, {VT_RefT}});
}

TEST_F(TextResolveTest, FunctionTypeUse) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  ctx.type_names.NewBound("$b"_sv);
  ctx.function_type_map.Define(
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
  ctx.function_type_map.Define(bound_function_type);

  OK(FunctionDesc{nullopt, Var{Index{0}}, bound_function_type},
     FunctionDesc{nullopt, nullopt, bound_function_type});

  ASSERT_EQ(1u, ctx.function_type_map.Size());
}

TEST_F(TextResolveTest, FunctionTypeUse_DeferType) {
  FunctionTypeMap& ftm = ctx.function_type_map;

  ftm.Define(BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});
  ftm.Define(BoundFunctionType{{BVT{nullopt, VT_I64}}, {}});

  OK(FunctionDesc{nullopt, Var{Index{2}},
                  BoundFunctionType{{BVT{nullopt, VT_F32}}, {}}},
     FunctionDesc{nullopt, nullopt,
                  BoundFunctionType{{BVT{nullopt, VT_F32}}, {}}});

  auto defined_types = ftm.EndModule();

  ASSERT_EQ(3u, ftm.Size());

  // Implicitly defined after other explicitly defined types.
  EXPECT_EQ((FunctionType{{VT_F32}, {}}), ftm.Get(2));

  // Generated defined type.
  ASSERT_EQ(1u, defined_types.size());
  EXPECT_EQ(
      (DefinedType{nullopt, BoundFunctionType{{BVT{nullopt, VT_F32}}, {}}}),
      defined_types[0]);
}

TEST_F(TextResolveTest, FunctionTypeUse_NoFunctionTypeInContext) {
  FunctionTypeUse type_use;
  Resolve(ctx, type_use);
  EXPECT_EQ((FunctionTypeUse{Var{Index{0}}, {}}), type_use);
}

TEST_F(TextResolveTest, FunctionTypeUse_IndexOOBWithExplicitParams) {
  FunctionTypeUse function_type_use;
  function_type_use.type_use = At{loc1, Var{Index{0}}};
  function_type_use.type = At{FunctionType{{VT_I32}, {}}};
  Resolve(ctx, function_type_use);
  ExpectError(ErrorList{{loc1, "Invalid type index 0"}}, errors);
}

TEST_F(TextResolveTest, BoundValueType) {
  ctx.type_names.NewBound("$t"_sv);
  OK(BVT{nullopt, Resolved_VT_Ref0}, BVT{nullopt, VT_RefT});
}

TEST_F(TextResolveTest, BoundValueTypeList) {
  ctx.type_names.NewBound("$t"_sv);
  OK(  // (param $a i32) (param ref 0)
      BoundValueTypeList{BVT{"$a"_sv, VT_I32}, BVT{nullopt, Resolved_VT_Ref0}},
      // (param $a i32) (param ref $t)
      BoundValueTypeList{BVT{"$a"_sv, VT_I32}, BVT{nullopt, VT_RefT}});
}

TEST_F(TextResolveTest, BoundFunctionType) {
  ctx.type_names.NewBound("$t"_sv);
  OK(  // (func (param $a i32) (result ref 0))
      BoundFunctionType{{BVT{"$a"_sv, VT_I32}}, {Resolved_VT_Ref0}},
      // (func (param $a i32) (result ref $t))
      BoundFunctionType{{BVT{"$a"_sv, VT_I32}}, {VT_RefT}});
}

TEST_F(TextResolveTest, BoundFunctionTypeUse_NoFunctionTypeInContext) {
  OptAt<Var> type_use;
  At<BoundFunctionType> type;
  Resolve(ctx, type_use, type);
  EXPECT_EQ(Var{Index{0}}, type_use);
  EXPECT_EQ(At{BoundFunctionType{}}, type);
}

TEST_F(TextResolveTest, BoundFunctionType_IndexOOBWithExplicitParams) {
  OptAt<Var> type_use = At{loc1, Var{Index{0}}};
  At<BoundFunctionType> type =
      At{BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}};
  Resolve(ctx, type_use, type);
  ExpectError(ErrorList{{loc1, "Invalid type index 0"}}, errors);
}

TEST_F(TextResolveTest, BlockImmediate) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(BlockImmediate{nullopt,
                    FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}},
     BlockImmediate{nullopt, FunctionTypeUse{Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, BlockImmediate_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(
      // (type 1) (param (ref 0))
      BlockImmediate{
          nullopt,
          FunctionTypeUse{Var{Index{1}}, FunctionType{{Resolved_VT_Ref0}, {}}}},
      // (param (ref $t))
      BlockImmediate{nullopt,
                     FunctionTypeUse{nullopt, FunctionType{{VT_RefT}, {}}}});
}

TEST_F(TextResolveTest, BlockImmediate_InlineType) {
  // An inline type can only be void, or a single result type.
  OK(BlockImmediate{nullopt, {}}, BlockImmediate{nullopt, {}});

  for (auto value_type :
       {VT_I32, VT_I64, VT_F32, VT_F64, VT_V128, VT_Funcref, VT_Externref}) {
    OK(BlockImmediate{nullopt,
                      FunctionTypeUse{nullopt, FunctionType{{}, {value_type}}}},
       BlockImmediate{
           nullopt, FunctionTypeUse{nullopt, FunctionType{{}, {value_type}}}});
  }

  // None of the inline block types should extend the ctx's function type
  // map.
  auto defined_types = ctx.function_type_map.EndModule();
  EXPECT_EQ(0u, ctx.function_type_map.Size());
  EXPECT_EQ(0u, defined_types.size());
}

TEST_F(TextResolveTest, BlockImmediate_InlineRefType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(
      // (func (result (ref 0)))
      BlockImmediate{
          nullopt,
          FunctionTypeUse{nullopt, FunctionType{{}, {Resolved_VT_Ref0}}}},
      // (func (result (ref $t)))
      BlockImmediate{nullopt,
                     FunctionTypeUse{nullopt, FunctionType{{}, {VT_RefT}}}});
}

TEST_F(TextResolveTest, BrOnCastImmediate) {
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l");
  ctx.type_names.NewBound("$t");

  OK(BrOnCastImmediate{Var{Index{0}},
                       HeapType2Immediate{Resolved_HT_0, Resolved_HT_0}},
     BrOnCastImmediate{Var{"$l"_sv}, HeapType2Immediate{HT_T, HT_T}});
}

TEST_F(TextResolveTest, BrTableImmediate) {
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l0");
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l1");
  ctx.label_names.Push();
  ctx.label_names.NewUnbound();
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l3");

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
  ctx.table_names.NewBound("$t");
  ctx.type_names.NewBound("$a");
  ctx.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(CallIndirectImmediate{Var{Index{0}},
                           FunctionTypeUse{Var{Index{0}},
                                           FunctionType{{VT_I32}, {}}}},
     CallIndirectImmediate{Var{"$t"_sv}, FunctionTypeUse{Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, CallIndirectImmediate_RefType) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(
      // call_indirect (type 0) (param (ref 0))
      CallIndirectImmediate{
          nullopt,
          FunctionTypeUse{Var{Index{1}}, FunctionType{{Resolved_VT_Ref0}, {}}}},
      // call_indirect (param (ref $t))
      CallIndirectImmediate{
          nullopt, FunctionTypeUse{nullopt, FunctionType{{VT_RefT}, {}}}});
}

TEST_F(TextResolveTest, HeapType2Immediate) {
  ctx.type_names.NewBound("$t");

  OK(HeapType2Immediate{Resolved_HT_0, Resolved_HT_0},
     HeapType2Immediate{HT_T, HT_T});
}

TEST_F(TextResolveTest, RttSubImmediate) {
  ctx.type_names.NewBound("$t");

  OK(RttSubImmediate{Index{1},
                     HeapType2Immediate{Resolved_HT_0, Resolved_HT_0}},
     RttSubImmediate{Index{1}, HeapType2Immediate{HT_T, HT_T}});
}

TEST_F(TextResolveTest, StructFieldImmediate) {
  ctx.type_names.NewBound("$S1");
  {
    auto& name_map = ctx.NewFieldNameMap(0);
    name_map.NewBound("$F1");
    name_map.NewBound("$F2");
  }

  {
    ctx.type_names.NewBound("$S2");
    auto& name_map = ctx.NewFieldNameMap(1);
    name_map.NewBound("$F3");
  }

  OK(StructFieldImmediate{Var{Index{0}}, Var{Index{0}}},
     StructFieldImmediate{Var{"$S1"_sv}, Var{"$F1"_sv}});

  OK(StructFieldImmediate{Var{Index{0}}, Var{Index{1}}},
     StructFieldImmediate{Var{"$S1"_sv}, Var{"$F2"_sv}});

  OK(StructFieldImmediate{Var{Index{1}}, Var{Index{0}}},
     StructFieldImmediate{Var{"$S2"_sv}, Var{"$F3"_sv}});

  // Undefined struct name.
  Fail({{loc1, "Undefined variable $S"}},
       StructFieldImmediate{At{loc1, Var{"$S"_sv}}, Var{"$F"_sv}});

  // Struct var is index.
  OK(StructFieldImmediate{Var{Index{0}}, Var{Index{0}}},
     StructFieldImmediate{Var{Index{0}}, Var{"$F1"_sv}});
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

  // Select Immediate.
  OK(I{O::Select, SelectImmediate{}}, I{O::Select, SelectImmediate{}});

  // SimdShuffle Immediate.
  OK(I{O::I8X16Shuffle, ShuffleImmediate{}},
     I{O::I8X16Shuffle, ShuffleImmediate{}});
}

TEST_F(TextResolveTest, Instruction_BlockImmediate) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(
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

TEST_F(TextResolveTest, Instruction_BlockImmediate_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(
      // block (type 1) (param (ref 0))
      I{O::Block,
        BlockImmediate{nullopt,
                       FunctionTypeUse{Var{Index{1}},
                                       FunctionType{{Resolved_VT_Ref0}, {}}}}},
      // block (param (ref $t))
      I{O::Block,
        BlockImmediate{nullopt,
                       FunctionTypeUse{nullopt, FunctionType{{VT_RefT}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_BrOnCastImmediate) {
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l");
  ctx.type_names.NewBound("$t");

  OK(I{O::BrOnCast,
       BrOnCastImmediate{Var{Index{0}},
                         HeapType2Immediate{Resolved_HT_0, Resolved_HT_0}}},
     I{O::BrOnCast,
       BrOnCastImmediate{Var{"$l"_sv}, HeapType2Immediate{HT_T, HT_T}}});
}

TEST_F(TextResolveTest, Instruction_BrTableImmediate) {
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l0");
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l1");

  OK(I{O::BrTable, BrTableImmediate{{Var{Index{1}}}, Var{Index{0}}}},
     I{O::BrTable, BrTableImmediate{{Var{"$l0"_sv}}, Var{"$l1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_CallIndirectImmediate) {
  ctx.table_names.NewBound("$t");
  ctx.type_names.NewBound("$a");
  ctx.function_type_map.Define(
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

TEST_F(TextResolveTest, Instruction_CallIndirectImmediate_RefType) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(
      // call_indirect (type 1) (param (ref 0))
      I{O::CallIndirect,
        CallIndirectImmediate{
            nullopt, FunctionTypeUse{Var{Index{1}},
                                     FunctionType{{Resolved_VT_Ref0}, {}}}}},
      // call_indirect (param (ref $t))
      I{O::CallIndirect,
        CallIndirectImmediate{
            nullopt, FunctionTypeUse{nullopt, FunctionType{{VT_RefT}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_CopyImmediate_Table) {
  ctx.table_names.NewBound("$t0");
  ctx.table_names.NewBound("$t1");

  OK(I{O::TableCopy, CopyImmediate{Var{Index{0}}, Var{Index{1}}}},
     I{O::TableCopy, CopyImmediate{Var{"$t0"_sv}, Var{"$t1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_CopyImmediate_Memory) {
  ctx.memory_names.NewBound("$m0");
  ctx.memory_names.NewBound("$m1");

  OK(I{O::MemoryCopy, CopyImmediate{Var{Index{0}}, Var{Index{1}}}},
     I{O::MemoryCopy, CopyImmediate{Var{"$m0"_sv}, Var{"$m1"_sv}}});
}

TEST_F(TextResolveTest, Instruction_InitImmediate_Table) {
  ctx.element_segment_names.NewBound("$e");
  ctx.table_names.NewBound("$t");

  OK(I{O::TableInit, InitImmediate{Var{Index{0}}, Var{Index{0}}}},
     I{O::TableInit, InitImmediate{Var{"$e"_sv}, Var{"$t"_sv}}});
}

TEST_F(TextResolveTest, Instruction_FuncBindImmediate) {
  ctx.type_names.NewBound("$a");
  ctx.function_type_map.Define(
      BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(I{O::FuncBind, FuncBindImmediate{FunctionTypeUse{
                        Var{Index{0}}, FunctionType{{VT_I32}, {}}}}},
     I{O::FuncBind, FuncBindImmediate{FunctionTypeUse{Var{"$a"_sv}, {}}}});

  // Populate the type use.
  OK(I{O::FuncBind, FuncBindImmediate{FunctionTypeUse{
                        Var{Index{0}}, FunctionType{{VT_I32}, {}}}}},
     I{O::FuncBind, FuncBindImmediate{
                        FunctionTypeUse{nullopt, FunctionType{{VT_I32}, {}}}}});
}

TEST_F(TextResolveTest, Instruction_HeapType2Immediate) {
  ctx.type_names.NewBound("$t");

  OK(I{O::RefTest, HeapType2Immediate{Resolved_HT_0, Resolved_HT_0}},
     I{O::RefTest, HeapType2Immediate{HT_T, HT_T}});

  OK(I{O::RefCast, HeapType2Immediate{Resolved_HT_0, Resolved_HT_0}},
     I{O::RefCast, HeapType2Immediate{HT_T, HT_T}});
}

TEST_F(TextResolveTest, Instruction_InitImmediate_Memory) {
  ctx.data_segment_names.NewBound("$d");
  ctx.memory_names.NewBound("$m");

  OK(I{O::MemoryInit, InitImmediate{Var{Index{0}}, Var{Index{0}}}},
     I{O::MemoryInit, InitImmediate{Var{"$d"_sv}, Var{"$m"_sv}}});
}

TEST_F(TextResolveTest, Instruction_Var_Function) {
  ctx.function_names.NewBound("$f");

  OK(I{O::Call, Var{Index{0}}}, I{O::Call, Var{"$f"_sv}});
  OK(I{O::ReturnCall, Var{Index{0}}}, I{O::ReturnCall, Var{"$f"_sv}});
  OK(I{O::RefFunc, Var{Index{0}}}, I{O::RefFunc, Var{"$f"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Table) {
  ctx.table_names.NewBound("$t");

  OK(I{O::TableFill, Var{Index{0}}}, I{O::TableFill, Var{"$t"_sv}});
  OK(I{O::TableGet, Var{Index{0}}}, I{O::TableGet, Var{"$t"_sv}});
  OK(I{O::TableGrow, Var{Index{0}}}, I{O::TableGrow, Var{"$t"_sv}});
  OK(I{O::TableSet, Var{Index{0}}}, I{O::TableSet, Var{"$t"_sv}});
  OK(I{O::TableSize, Var{Index{0}}}, I{O::TableSize, Var{"$t"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Global) {
  ctx.global_names.NewBound("$t");

  OK(I{O::GlobalGet, Var{Index{0}}}, I{O::GlobalGet, Var{"$t"_sv}});
  OK(I{O::GlobalSet, Var{Index{0}}}, I{O::GlobalSet, Var{"$t"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Tag) {
  ctx.tag_names.NewBound("$e");

  OK(I{O::Throw, Var{Index{0}}}, I{O::Throw, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Element) {
  ctx.element_segment_names.NewBound("$e");

  OK(I{O::ElemDrop, Var{Index{0}}}, I{O::ElemDrop, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Data) {
  ctx.data_segment_names.NewBound("$d");

  OK(I{O::DataDrop, Var{Index{0}}}, I{O::DataDrop, Var{"$d"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Label) {
  ctx.label_names.Push();
  ctx.label_names.NewBound("$l");

  OK(I{O::BrIf, Var{Index{0}}}, I{O::BrIf, Var{"$l"_sv}});
  OK(I{O::Br, Var{Index{0}}}, I{O::Br, Var{"$l"_sv}});
}

TEST_F(TextResolveTest, Instruction_Var_Local) {
  ctx.local_names.NewBound("$l");

  OK(I{O::LocalGet, Var{Index{0}}}, I{O::LocalGet, Var{"$l"_sv}});
  OK(I{O::LocalSet, Var{Index{0}}}, I{O::LocalSet, Var{"$l"_sv}});
  OK(I{O::LocalTee, Var{Index{0}}}, I{O::LocalTee, Var{"$l"_sv}});
}

TEST_F(TextResolveTest, Instruction_HeapType) {
  ctx.type_names.NewBound("$t");

  OK(I{O::RefNull, Resolved_HT_0}, I{O::RefNull, HT_T});
}

TEST_F(TextResolveTest, Instruction_LetImmediate) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(I{O::Let,
       LetImmediate{
           BlockImmediate{
               nullopt, FunctionTypeUse{Var{Index{1}},
                                        FunctionType{{Resolved_VT_Ref0}, {}}}},
           {BVT{nullopt, Resolved_VT_Ref0}}}},
     I{O::Let,
       LetImmediate{
           BlockImmediate{
               nullopt, FunctionTypeUse{nullopt, FunctionType{{VT_RefT}, {}}}},
           {BVT{nullopt, VT_RefT}}}});
}

TEST_F(TextResolveTest, Instruction_MemArgImmediate) {
  ctx.memory_names.NewBound("$m"_sv);

  OK(I{O::I32Load, MemArgImmediate{nullopt, nullopt, Var{Index{0}}}},
     I{O::I32Load, MemArgImmediate{nullopt, nullopt, Var{"$m"_sv}}});
}

TEST_F(TextResolveTest, Instruction_MemOptImmediate) {
  ctx.memory_names.NewBound("$m"_sv);

  OK(I{O::MemorySize, MemOptImmediate{Var{Index{0}}}},
     I{O::MemorySize, MemOptImmediate{Var{"$m"_sv}}});
}

TEST_F(TextResolveTest, Instruction_RttSubImmediate) {
  ctx.type_names.NewBound("$t");

  OK(I{O::RttSub, RttSubImmediate{Index{1}, HeapType2Immediate{Resolved_HT_0,
                                                               Resolved_HT_0}}},
     I{O::RttSub, RttSubImmediate{Index{1}, HeapType2Immediate{HT_T, HT_T}}});
}

TEST_F(TextResolveTest, Instruction_SelectImmediate) {
  ctx.type_names.NewBound("$t");

  OK(I{O::SelectT, SelectImmediate{VT_I32, Resolved_VT_Ref0}},
     I{O::SelectT, SelectImmediate{VT_I32, VT_RefT}});
}

TEST_F(TextResolveTest, Instruction_SimdMemoryLaneImmediate) {
  ctx.memory_names.NewBound("$m"_sv);

  OK(I{O::V128Load8Lane,
       SimdMemoryLaneImmediate{MemArgImmediate{nullopt, nullopt, Var{Index{0}}},
                               0}},
     I{O::V128Load8Lane,
       SimdMemoryLaneImmediate{MemArgImmediate{nullopt, nullopt, Var{"$m"_sv}},
                               0}});
}

TEST_F(TextResolveTest, Instruction_StructFieldImmediate) {
  ctx.type_names.NewBound("$S");
  auto& name_map = ctx.NewFieldNameMap(0);
  name_map.NewBound("$F");

  OK(I{O::StructGet, StructFieldImmediate{Var{Index{0}}, Var{Index{0}}}},
     I{O::StructGet, StructFieldImmediate{Var{"$S"_sv}, Var{"$F"_sv}}});
}

TEST_F(TextResolveTest, InstructionList) {
  ctx.function_names.NewBound("$f");
  ctx.local_names.NewBound("$l");

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
  ctx.function_type_map.Define(BoundFunctionType{});

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
  ctx.function_type_map.Define(BoundFunctionType{});

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
  ctx.function_type_map.Define(BoundFunctionType{});

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

TEST_F(TextResolveTest, FieldType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(FieldType{nullopt, StorageType{Resolved_VT_Ref0}, Mutability::Const},
     FieldType{nullopt, StorageType{VT_RefT}, Mutability::Const});
}

TEST_F(TextResolveTest, FieldTypeList) {
  ctx.type_names.NewBound("$t"_sv);

  OK(FieldTypeList{FieldType{nullopt, StorageType{VT_Funcref}, Mutability::Var},
                   FieldType{nullopt, StorageType{Resolved_VT_Ref0},
                             Mutability::Const}},
     FieldTypeList{
         // No resolving required.
         FieldType{nullopt, StorageType{VT_Funcref}, Mutability::Var},
         // Resolve field type.
         FieldType{nullopt, StorageType{VT_RefT}, Mutability::Const}});
}

TEST_F(TextResolveTest, StructType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(StructType{FieldTypeList{
         FieldType{nullopt, StorageType{Resolved_VT_Ref0}, Mutability::Const}}},
     StructType{FieldTypeList{
         FieldType{nullopt, StorageType{VT_RefT}, Mutability::Const}}});
}

TEST_F(TextResolveTest, ArrayType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(ArrayType{FieldType{nullopt, StorageType{Resolved_VT_Ref0},
                         Mutability::Const}},
     ArrayType{FieldType{nullopt, StorageType{VT_RefT}, Mutability::Const}});
}

TEST_F(TextResolveTest, DefinedType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(
      // type (param (ref 0)) (result i32)
      DefinedType{
          nullopt,
          BoundFunctionType{{BVT{nullopt, Resolved_VT_Ref0}}, {VT_I32}},
      },
      // type (param (ref $t)) (result i32)
      DefinedType{
          nullopt,
          BoundFunctionType{{BVT{nullopt, VT_RefT}}, {VT_I32}},
      });
}

TEST_F(TextResolveTest, DefinedType_gc) {
  ctx.type_names.NewBound("$t"_sv);

  // StructType
  OK(
      // type (struct (field (ref 0)))
      DefinedType{nullopt, StructType{FieldTypeList{
                               FieldType{nullopt, StorageType{Resolved_VT_Ref0},
                                         Mutability::Const}}}},
      // type (struct (field (ref $t)))
      DefinedType{nullopt,
                  StructType{FieldTypeList{FieldType{
                      nullopt, StorageType{VT_RefT}, Mutability::Const}}}});

  // ArrayType
  OK(
      // type (array (field (ref 0)))
      DefinedType{nullopt,
                  ArrayType{FieldType{nullopt, StorageType{Resolved_VT_Ref0},
                                      Mutability::Const}}},
      // type (array (field (ref $t)))
      DefinedType{nullopt, ArrayType{FieldType{nullopt, StorageType{VT_RefT},
                                               Mutability::Const}}});
}

TEST_F(TextResolveTest, DefinedType_DuplicateName) {
  ctx.type_names.NewBound("$t"_sv);

  FailDefineTypes({{loc1, "Variable $t is already bound to index 0"}},
                  DefinedType{At{loc1, "$t"_sv}, BoundFunctionType{}});
}

TEST_F(TextResolveTest, DefinedType_DistinctTypes) {
  OKDefine(DefinedType{"$a"_sv, BoundFunctionType{}});
  OKDefine(DefinedType{"$b"_sv, BoundFunctionType{}});

  ASSERT_EQ(2u, ctx.function_type_map.Size());
}

TEST_F(TextResolveTest, FunctionDesc) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(
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

TEST_F(TextResolveTest, FunctionDesc_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // type (type 1) (param (ref 0))
      FunctionDesc{nullopt, Var{Index{1}},
                   BoundFunctionType{{BVT{nullopt, Resolved_VT_Ref0}}, {}}},
      // type (param (ref $t))
      FunctionDesc{nullopt, nullopt,
                   BoundFunctionType{{BVT{nullopt, VT_RefT}}, {}}});
}

TEST_F(TextResolveTest, FunctionDesc_DuplicateName) {
  ctx.function_names.NewBound("$f"_sv);

  FailDefine({{loc1, "Variable $f is already bound to index 0"}},
             FunctionDesc{At{loc1, "$f"_sv}, nullopt, {}});
}

TEST_F(TextResolveTest, FunctionDesc_DuplicateParamName) {
  Fail({{loc1, "Variable $foo is already bound to index 0"}},
       FunctionDesc{nullopt, nullopt,
                    BoundFunctionType{{BVT{"$foo"_sv, VT_I32},
                                       BVT{At{loc1, "$foo"_sv}, VT_I64}},
                                      {}}});
}

TEST_F(TextResolveTest, TableType) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(TableType{Limits{0}, Resolved_RT_Ref0}, TableType{Limits{0}, RT_RefT});
}

TEST_F(TextResolveTest, TableDesc) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(TableDesc{nullopt, TableType{Limits{0}, Resolved_RT_Ref0}},
     TableDesc{nullopt, TableType{Limits{0}, RT_RefT}});
}

TEST_F(TextResolveTest, GlobalType) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(GlobalType{Resolved_VT_Ref0, Mutability::Const},
     GlobalType{VT_RefT, Mutability::Const});
}

TEST_F(TextResolveTest, GlobalDesc) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(GlobalDesc{nullopt, GlobalType{Resolved_VT_Ref0, Mutability::Const}},
     GlobalDesc{nullopt, GlobalType{VT_RefT, Mutability::Const}});
}

TEST_F(TextResolveTest, TagType) {
  ctx.type_names.NewBound("$a");
  ctx.function_type_map.Define(BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(TagType{TagAttribute::Exception,
             FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}},
     TagType{TagAttribute::Exception, FunctionTypeUse{Var{"$a"_sv}, {}}});

  // Populate the type use.
  OK(TagType{TagAttribute::Exception,
             FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}},
     TagType{TagAttribute::Exception,
             FunctionTypeUse{nullopt, FunctionType{{VT_I32}, {}}}});
}

TEST_F(TextResolveTest, TagType_RefType) {
  ctx.type_names.NewBound("$t");
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // type (type 1) (param (ref 0))
      TagType{
          TagAttribute::Exception,
          FunctionTypeUse{Var{Index{1}}, FunctionType{{Resolved_VT_Ref0}, {}}}},
      // type (param (ref $t))
      TagType{TagAttribute::Exception,
              FunctionTypeUse{nullopt, FunctionType{{VT_RefT}, {}}}});
}

TEST_F(TextResolveTest, TagDesc) {
  ctx.type_names.NewBound("$a");
  ctx.function_type_map.Define(BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(TagDesc{nullopt, TagType{TagAttribute::Exception,
                              FunctionTypeUse{Var{Index{0}},
                                              FunctionType{{VT_I32}, {}}}}},
     TagDesc{nullopt, TagType{TagAttribute::Exception,
                              FunctionTypeUse{Var{"$a"_sv}, {}}}});
}

TEST_F(TextResolveTest, Import_Function) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // $p param name is not copied.
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            FunctionDesc{nullopt, Var{Index{0}},
                         BoundFunctionType{{BVT{nullopt, VT_I32}}, {}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            FunctionDesc{nullopt, Var{"$a"_sv}, {}}});
}

TEST_F(TextResolveTest, Import_Function_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // (import "m" "n" (func (param (ref 0))))
      Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
             FunctionDesc{
                 nullopt, Var{Index{1}},
                 BoundFunctionType{{BVT{nullopt, Resolved_VT_Ref0}}, {}}}},
      // (import "m" "n" (func (param (ref $t))))
      Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
             FunctionDesc{nullopt, nullopt,
                          BoundFunctionType{{BVT{nullopt, VT_RefT}}, {}}}});
}

TEST_F(TextResolveTest, Import_Table) {
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}}});
}

TEST_F(TextResolveTest, Import_Table_RefType) {
  ctx.type_names.NewBound("$t"_sv);

  OK(  // (import "m" "n" (table 0 (ref 0)))
      Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
             TableDesc{nullopt, TableType{Limits{0}, Resolved_RT_Ref0}}},
      // (import "m" "n" (table 0 (ref $t)))
      Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
             TableDesc{nullopt, TableType{Limits{0}, RT_RefT}}});
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

TEST_F(TextResolveTest, Import_Global_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            GlobalDesc{nullopt,
                       GlobalType{Resolved_VT_Ref0, Mutability::Const}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            GlobalDesc{nullopt, GlobalType{VT_RefT, Mutability::Const}}});
}

TEST_F(TextResolveTest, Import_Tag) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // $p param name is not copied.
  OK(Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TagDesc{nullopt,
                    TagType{TagAttribute::Exception,
                            FunctionTypeUse{Var{Index{0}},
                                            FunctionType{{VT_I32}, {}}}}}},
     Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
            TagDesc{nullopt, TagType{TagAttribute::Exception,
                                     FunctionTypeUse{Var{"$a"_sv}, {}}}}});
}

TEST_F(TextResolveTest, Import_Tag_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // import "m" "n" (tag (type 1) (param (ref 0)))
      Import{
          Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
          TagDesc{nullopt, TagType{TagAttribute::Exception,
                                   FunctionTypeUse{
                                       Var{Index{1}},
                                       FunctionType{{Resolved_VT_Ref0}, {}}}}}},
      // import "m" "n" (tag (func (param (ref $t)))
      Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
             TagDesc{nullopt,
                     TagType{TagAttribute::Exception,
                             FunctionTypeUse{nullopt,
                                             FunctionType{{VT_RefT}, {}}}}}});
}

TEST_F(TextResolveTest, Function) {
  ctx.type_names.NewBound("$a"_sv);
  ctx.function_type_map.Define(
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

TEST_F(TextResolveTest, Function_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // (func (type 1) (param (ref 0)) (local (ref 0)))
      Function{
          FunctionDesc{nullopt, Var{Index{1}},
                       BoundFunctionType{{BVT{nullopt, Resolved_VT_Ref0}}, {}}},
          {BVT{nullopt, Resolved_VT_Ref0}},
          {},
          {}},
      // (func (param (ref $t)) (local (ref $t)))
      Function{FunctionDesc{nullopt, nullopt,
                            BoundFunctionType{{BVT{nullopt, VT_RefT}}, {}}},
               {BVT{nullopt, VT_RefT}},
               {},
               {}});
}

TEST_F(TextResolveTest, Function_DuplicateLocalName) {
  Fail({{loc1, "Variable $foo is already bound to index 0"}},
       Function{FunctionDesc{},
                {BVT{"$foo"_sv, VT_I32}, BVT{At{loc1, "$foo"_sv}, VT_I64}},
                InstructionList{},
                {}});
}

TEST_F(TextResolveTest, Function_DuplicateParamLocalNames) {
  Fail({{loc1, "Variable $foo is already bound to index 0"}},
       Function{FunctionDesc{nullopt, nullopt,
                             BoundFunctionType{{BVT{"$foo"_sv, VT_I32}}, {}}},
                {BVT{At{loc1, "$foo"_sv}, VT_I64}},
                InstructionList{},
                {}});
}

TEST_F(TextResolveTest, ElementExpressionList) {
  ctx.function_names.NewBound("$f"_sv);

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
  ctx.function_names.NewBound("$f"_sv);

  OK(
      ElementListWithExpressions{
          RT_Funcref,
          ElementExpressionList{
              ElementExpression{I{O::RefNull}},
              ElementExpression{I{O::RefFunc, Var{Index{0}}}},
          }},
      ElementListWithExpressions{
          RT_Funcref, ElementExpressionList{
                          ElementExpression{I{O::RefNull}},
                          ElementExpression{I{O::RefFunc, Var{"$f"_sv}}},
                      }});
}

TEST_F(TextResolveTest, ElementListWithExpressions_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(ElementListWithExpressions{Resolved_RT_Ref0, {}},
     ElementListWithExpressions{RT_RefT, {}});
}

TEST_F(TextResolveTest, ElementListWithVars) {
  ctx.function_names.NewBound("$f"_sv);
  ctx.function_names.NewUnbound();

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
  ctx.function_names.NewBound("$f"_sv);
  ctx.function_names.NewUnbound();

  // Expressions.
  OK(ElementList{ElementListWithExpressions{
         RT_Funcref,
         ElementExpressionList{
             ElementExpression{I{O::RefNull}},
             ElementExpression{I{O::RefFunc, Var{Index{0}}}},
         }}},
     ElementList{ElementListWithExpressions{
         RT_Funcref, ElementExpressionList{
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
  ctx.function_names.NewBound("$f"_sv);

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

TEST_F(TextResolveTest, Table_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // (table 0 (ref 0))
      Table{
          TableDesc{nullopt, TableType{Limits{0}, Resolved_RT_Ref0}},
          {},
          ElementList{},
      },
      // (table 0 (ref $t))
      Table{
          TableDesc{nullopt, TableType{Limits{0}, RT_RefT}},
          {},
          ElementList{},
      });
}

TEST_F(TextResolveTest, Table_DuplicateName) {
  ctx.table_names.NewBound("$t"_sv);

  FailDefine({{loc1, "Variable $t is already bound to index 0"}},
             TableDesc{At{loc1, "$t"_sv}, TableType{Limits{0}, RT_Funcref}});
}

TEST_F(TextResolveTest, Memory_DuplicateName) {
  ctx.memory_names.NewBound("$m"_sv);

  FailDefine({{loc1, "Variable $m is already bound to index 0"}},
             MemoryDesc{At{loc1, "$m"_sv}, MemoryType{Limits{0}}});
}

TEST_F(TextResolveTest, Global) {
  ctx.global_names.NewBound("$g"_sv);

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

TEST_F(TextResolveTest, Global_RefType) {
  ctx.type_names.NewBound("$t"_sv);
  ctx.function_type_map.Define(BoundFunctionType{});

  OK(  // (global (ref 0))
      Global{
          GlobalDesc{nullopt, GlobalType{Resolved_VT_Ref0, Mutability::Const}},
          ConstantExpression{},
          {}},
      // (global (ref $t))
      Global{GlobalDesc{nullopt, GlobalType{VT_RefT, Mutability::Const}},
             ConstantExpression{},
             {}});
}

TEST_F(TextResolveTest, Global_DuplicateName) {
  ctx.global_names.NewBound("$g"_sv);

  FailDefine(
      {{loc1, "Variable $g is already bound to index 0"}},
      GlobalDesc{At{loc1, "$g"_sv}, GlobalType{VT_I32, Mutability::Const}});
}

TEST_F(TextResolveTest, Export) {
  ctx.function_names.NewBound("$f"_sv);  // 0
  ctx.table_names.NewUnbound();
  ctx.table_names.NewBound("$t"_sv);  // 1
  ctx.memory_names.NewUnbound();
  ctx.memory_names.NewUnbound();
  ctx.memory_names.NewBound("$m"_sv);  // 2
  ctx.global_names.NewUnbound();
  ctx.global_names.NewUnbound();
  ctx.global_names.NewUnbound();
  ctx.global_names.NewBound("$g"_sv);  // 3
  ctx.tag_names.NewUnbound();
  ctx.tag_names.NewUnbound();
  ctx.tag_names.NewUnbound();
  ctx.tag_names.NewUnbound();
  ctx.tag_names.NewBound("$e"_sv);  // 4

  OK(Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{Index{0}}},
     Export{ExternalKind::Function, Text{"\"f\"", 1}, Var{"$f"_sv}});

  OK(Export{ExternalKind::Table, Text{"\"t\"", 1}, Var{Index{1}}},
     Export{ExternalKind::Table, Text{"\"t\"", 1}, Var{"$t"_sv}});

  OK(Export{ExternalKind::Memory, Text{"\"m\"", 1}, Var{Index{2}}},
     Export{ExternalKind::Memory, Text{"\"m\"", 1}, Var{"$m"_sv}});

  OK(Export{ExternalKind::Global, Text{"\"g\"", 1}, Var{Index{3}}},
     Export{ExternalKind::Global, Text{"\"g\"", 1}, Var{"$g"_sv}});

  OK(Export{ExternalKind::Tag, Text{"\"e\"", 1}, Var{Index{4}}},
     Export{ExternalKind::Tag, Text{"\"e\"", 1}, Var{"$e"_sv}});
}

TEST_F(TextResolveTest, Start) {
  ctx.function_names.NewBound("$f"_sv);

  OK(Start{Var{Index{0}}}, Start{Var{"$f"_sv}});
}

TEST_F(TextResolveTest, ElementSegment) {
  ctx.function_names.NewBound("$f"_sv);
  ctx.table_names.NewBound("$t"_sv);
  ctx.global_names.NewBound("$g"_sv);

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
  ctx.element_segment_names.NewBound("$e"_sv);

  FailDefine({{loc1, "Variable $e is already bound to index 0"}},
             ElementSegment{At{loc1, "$e"_sv}, nullopt, ConstantExpression{},
                            ElementList{}});
}

TEST_F(TextResolveTest, DataSegment) {
  ctx.memory_names.NewBound("$m"_sv);
  ctx.global_names.NewBound("$g"_sv);

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
  ctx.data_segment_names.NewBound("$d"_sv);

  FailDefine({{loc1, "Variable $d is already bound to index 0"}},
             DataSegment{At{loc1, "$d"_sv}, nullopt, ConstantExpression{}, {}});
}

TEST_F(TextResolveTest, Tag) {
  ctx.type_names.NewBound("$a");
  ctx.function_type_map.Define(BoundFunctionType{{BVT{nullopt, VT_I32}}, {}});

  OK(Tag{TagDesc{nullopt, TagType{TagAttribute::Exception,
                                  FunctionTypeUse{Var{Index{0}},
                                                  FunctionType{{VT_I32}, {}}}}},
         {}},
     Tag{TagDesc{nullopt, TagType{TagAttribute::Exception,
                                  FunctionTypeUse{Var{"$a"_sv}, {}}}},
         {}});
}

TEST_F(TextResolveTest, Tag_DuplicateName) {
  ctx.tag_names.NewBound("$e"_sv);

  FailDefine({{loc1, "Variable $e is already bound to index 0"}},
             TagDesc{At{loc1, "$e"_sv},
                     TagType{TagAttribute::Exception, FunctionTypeUse{}}});
}

TEST_F(TextResolveTest, ModuleItem) {
  ctx.type_names.NewBound("$a");
  ctx.function_names.NewBound("$f");
  ctx.table_names.NewBound("$t");
  ctx.memory_names.NewBound("$m");
  ctx.global_names.NewBound("$g");

  ctx.function_type_map.Define(
      BoundFunctionType{{BVT{"$p"_sv, VT_I32}}, {}});

  // DefinedType.
  OK(ModuleItem{DefinedType{nullopt, BoundFunctionType{}}},
     ModuleItem{DefinedType{nullopt, BoundFunctionType{}}});

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

  // Tag.
  OK(ModuleItem{Tag{
         TagDesc{nullopt, TagType{TagAttribute::Exception,
                                  FunctionTypeUse{Var{Index{0}},
                                                  FunctionType{{VT_I32}, {}}}}},
         {}}},
     ModuleItem{
         Tag{TagDesc{nullopt, TagType{TagAttribute::Exception,
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

          // The deferred defined types.
          // (type (func))
          ModuleItem{DefinedType{nullopt, BoundFunctionType{}}},
          // (type (func (param i32))
          ModuleItem{DefinedType{
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

TEST_F(TextResolveTest, ModuleWithDeferredTypes_AndStruct) {
  OK(
      Module{
          // (type (struct))
          ModuleItem{DefinedType{nullopt, StructType{}}},
          // (func (type 1) (param i32))
          ModuleItem{Function{FunctionDesc{
                                  nullopt,
                                  Var{Index{1}},
                                  BoundFunctionType{{BVT{nullopt, VT_I32}}, {}},
                              },
                              {},
                              {},
                              {}}},

          // The deferred defined types.
          // (type (func (param i32))
          ModuleItem{DefinedType{
              nullopt,
              BoundFunctionType{BoundValueTypeList{BVT{nullopt, VT_I32}}, {}}}},
      },
      Module{
          // (type (struct))
          ModuleItem{DefinedType{nullopt, StructType{}}},
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
