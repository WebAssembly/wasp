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

#include "wasp/convert/to_binary.h"

#include "gtest/gtest.h"
#include "wasp/binary/formatters.h"
#include "wasp/text/formatters.h"

using namespace ::wasp;
using namespace ::wasp::convert;

namespace {

template <typename B, typename T, typename... Args>
void OK(const B& expected, const T& input, Args&&... args) {
  Context context;
  EXPECT_EQ(expected, ToBinary(context, input, std::forward<Args>(args)...));
}

template <typename F, typename B, typename T, typename... Args>
void OKFunc(const F& func, const B& expected, const T& input, Args&&... args) {
  Context context;
  EXPECT_EQ(expected, func(context, input, std::forward<Args>(args)...));
}

const SpanU8 loc1 = "A"_su8;
const SpanU8 loc2 = "B"_su8;
const SpanU8 loc3 = "C"_su8;
const SpanU8 loc4 = "D"_su8;
const SpanU8 loc5 = "E"_su8;
const SpanU8 loc6 = "F"_su8;
const SpanU8 loc7 = "G"_su8;

}  // namespace

TEST(ConvertToBinaryTest, VarList) {
  OK(
      binary::IndexList{
          MakeAt(loc1, Index{0}),
          MakeAt(loc2, Index{1}),
      },
      text::VarList{
          MakeAt(loc1, text::Var{Index{0}}),
          MakeAt(loc2, text::Var{Index{1}}),
      });
}

TEST(ConvertToBinaryTest, FunctionType) {
  OK(MakeAt(loc1,
            binary::FunctionType{
                ValueTypeList{MakeAt(loc2, ValueType::I32)},
                ValueTypeList{MakeAt(loc3, ValueType::F32)},
            }),
     MakeAt(loc1, text::FunctionType{
                      ValueTypeList{MakeAt(loc2, ValueType::I32)},
                      ValueTypeList{MakeAt(loc3, ValueType::F32)},
                  }));
}

TEST(ConvertToBinaryTest, TypeEntry) {
  OK(MakeAt(loc1, binary::TypeEntry{MakeAt(
                      loc2,
                      binary::FunctionType{
                          ValueTypeList{MakeAt(loc3, ValueType::I32)},
                          ValueTypeList{MakeAt(loc4, ValueType::F32)},
                      })}),
     MakeAt(
         loc1,
         text::TypeEntry{
             {},
             MakeAt(loc2, text::BoundFunctionType{
                              text::BoundValueTypeList{text::BoundValueType{
                                  nullopt, MakeAt(loc3, ValueType::I32)}},
                              ValueTypeList{MakeAt(loc4, ValueType::F32)}})}));
}

TEST(ConvertToBinaryTest, Import) {
  // Function
  OK(MakeAt(loc1, binary::Import{MakeAt(loc2, "m"_sv), MakeAt(loc3, "n"_sv),
                                 MakeAt(loc4, Index{0})}),
     MakeAt(loc1,
            text::Import{MakeAt(loc2, text::Text{"\"m\"", 1}),
                         MakeAt(loc3, text::Text{"\"n\"", 1}),
                         text::FunctionDesc{
                             nullopt, MakeAt(loc4, text::Var{Index{0}}), {}}}));

  // Table
  OK(MakeAt(
         loc1,
         binary::Import{MakeAt(loc2, "m"_sv), MakeAt(loc3, "n"_sv),
                        MakeAt(loc4,
                               TableType{
                                   MakeAt(loc5, Limits{MakeAt(loc6, u32{1})}),
                                   MakeAt(loc7, ReferenceType::Funcref),
                               })}),
     MakeAt(loc1,
            text::Import{
                MakeAt(loc2, text::Text{"\"m\"", 1}),
                MakeAt(loc3, text::Text{"\"n\"", 1}),
                text::TableDesc{
                    nullopt,
                    MakeAt(loc4, TableType{
                                     MakeAt(loc5, Limits{MakeAt(loc6, u32{1})}),
                                     MakeAt(loc7, ReferenceType::Funcref),
                                 })}}));

  // Memory
  OK(MakeAt(
         loc1,
         binary::Import{MakeAt(loc2, "m"_sv), MakeAt(loc3, "n"_sv),
                        MakeAt(loc4,
                               MemoryType{
                                   MakeAt(loc5, Limits{MakeAt(loc6, u32{1})}),
                               })}),
     MakeAt(loc1,
            text::Import{
                MakeAt(loc2, text::Text{"\"m\"", 1}),
                MakeAt(loc3, text::Text{"\"n\"", 1}),
                text::MemoryDesc{
                    nullopt,
                    MakeAt(loc4, MemoryType{
                                     MakeAt(loc5, Limits{MakeAt(loc6, u32{1})}),
                                 })}}));

  // Global
  OK(MakeAt(loc1, binary::Import{MakeAt(loc2, "m"_sv), MakeAt(loc3, "n"_sv),
                                 MakeAt(loc4,
                                        GlobalType{
                                            MakeAt(loc5, ValueType::I32),
                                            Mutability::Const,
                                        })}),
     MakeAt(loc1,
            text::Import{
                MakeAt(loc2, text::Text{"\"m\"", 1}),
                MakeAt(loc3, text::Text{"\"n\"", 1}),
                text::GlobalDesc{nullopt,
                                 MakeAt(loc4, GlobalType{
                                                  MakeAt(loc5, ValueType::I32),
                                                  Mutability::Const,
                                              })}}));

  // Event
  OK(MakeAt(loc1, binary::Import{MakeAt(loc2, "m"_sv), MakeAt(loc3, "n"_sv),
                                 MakeAt(loc4,
                                        binary::EventType{
                                            EventAttribute::Exception,
                                            MakeAt(loc5, Index{0}),
                                        })}),
     MakeAt(loc1,
            text::Import{
                MakeAt(loc2, text::Text{"\"m\"", 1}),
                MakeAt(loc3, text::Text{"\"n\"", 1}),
                text::EventDesc{
                    nullopt,
                    MakeAt(loc4,
                           text::EventType{
                               EventAttribute::Exception,
                               text::FunctionTypeUse{
                                   MakeAt(loc5, text::Var{Index{0}}), {}}})}}));
}

TEST(ConvertToBinaryTest, Function) {
  OK(MakeAt(loc1, binary::Function{MakeAt(loc2, Index{13})}),
     MakeAt(loc1,
            text::Function{text::FunctionDesc{
                               nullopt, MakeAt(loc2, text::Var{Index{13}}), {}},
                           {},
                           {},
                           {}}));
}

TEST(ConvertToBinaryTest, Table) {
  auto table_type =
      MakeAt(loc1, TableType{Limits{MakeAt(loc2, Index{0})},
                             MakeAt(loc3, ReferenceType::Funcref)});

  OK(MakeAt(loc4, binary::Table{table_type}),
     MakeAt(loc4, text::Table{text::TableDesc{nullopt, table_type}, {}}));
}

TEST(ConvertToBinaryTest, Memory) {
  auto memory_type =
      MakeAt(loc1, MemoryType{Limits{MakeAt(loc2, Index{0})}});

  OK(MakeAt(loc3, binary::Memory{memory_type}),
     MakeAt(loc3, text::Memory{text::MemoryDesc{nullopt, memory_type}, {}}));
}

TEST(ConvertToBinaryTest, Global) {
  auto global_type = MakeAt(loc1, GlobalType{
                                      MakeAt(loc2, ValueType::I32),
                                      MakeAt(Mutability::Const),
                                  });

  OK(MakeAt(loc3,
            binary::Global{
                global_type,
                MakeAt(loc4,
                       binary::ConstantExpression{
                           MakeAt(loc5, binary::Instruction{MakeAt(
                                            loc6, Opcode::Nop)}),
                       }),
            }),
     MakeAt(loc3,
            text::Global{
                text::GlobalDesc{nullopt, global_type},
                MakeAt(loc4,
                       text::ConstantExpression{
                           MakeAt(loc5,
                                  text::Instruction{MakeAt(loc6, Opcode::Nop)}),
                       }),
                {}}));
}

TEST(ConvertToBinaryTest, Export) {
  OK(MakeAt(loc1,
            binary::Export{
                MakeAt(loc2, ExternalKind::Function),
                MakeAt(loc3, "hello"_sv),
                MakeAt(loc4, Index{13}),
            }),
     MakeAt(loc1, text::Export{
                      MakeAt(loc2, ExternalKind::Function),
                      MakeAt(loc3, text::Text{"\"hello\""_sv, 5}),
                      MakeAt(loc4, text::Var{Index{13}}),
                  }));
}

TEST(ConvertToBinaryTest, Start) {
  OK(MakeAt(loc1, binary::Start{MakeAt(loc2, Index{13})}),
     MakeAt(loc1, text::Start{MakeAt(loc2, text::Var{Index{13}})}));
}

TEST(ConvertToBinaryTest, ElementExpression) {
  OK(MakeAt(loc1,
            binary::ElementExpression{{
                MakeAt(loc2,
                       binary::Instruction{MakeAt(loc3, Opcode::Unreachable)}),
                MakeAt(loc4, binary::Instruction{MakeAt(loc5, Opcode::Nop)}),
            }}),
     MakeAt(
         loc1,
         text::ElementExpression{{
             MakeAt(loc2, text::Instruction{MakeAt(loc3, Opcode::Unreachable)}),
             MakeAt(loc4, text::Instruction{MakeAt(loc5, Opcode::Nop)}),
         }}));
}

TEST(ConvertToBinaryTest, ElementExpressionList) {
  OK(MakeAt(
         loc1,
         binary::ElementExpressionList{
             MakeAt(loc2, binary::ElementExpression{MakeAt(
                              loc3, binary::Instruction{MakeAt(
                                        loc4, Opcode::Unreachable)})}),
             MakeAt(loc5,
                    binary::ElementExpression{MakeAt(
                        loc6, binary::Instruction{MakeAt(loc7, Opcode::Nop)})}),
         }),
     MakeAt(loc1, text::ElementExpressionList{
                      MakeAt(loc2, text::ElementExpression{MakeAt(
                                       loc3, text::Instruction{MakeAt(
                                                 loc4, Opcode::Unreachable)})}),
                      MakeAt(loc5, text::ElementExpression{MakeAt(
                                       loc6, text::Instruction{MakeAt(
                                                 loc7, Opcode::Nop)})}),
                  }));
}

TEST(ConvertToBinaryTest, ElementList) {
  // text::Var -> binary::Index
  OK(binary::ElementList{binary::ElementListWithIndexes{
         MakeAt(loc1, ExternalKind::Function),
         MakeAt(loc2,
                binary::IndexList{
                    MakeAt(loc3, Index{0}),
                    MakeAt(loc4, Index{1}),
                }),
     }},
     text::ElementList{text::ElementListWithVars{
         MakeAt(loc1, ExternalKind::Function),
         MakeAt(loc2,
                text::VarList{
                    MakeAt(loc3, text::Var{Index{0}}),
                    MakeAt(loc4, text::Var{Index{1}}),
                }),
     }});

  // ElementExpression.
  OK(binary::ElementList{binary::ElementListWithExpressions{
         MakeAt(loc1, ReferenceType::Funcref),
         MakeAt(loc2, binary::ElementExpressionList{MakeAt(
                          loc3, binary::ElementExpression{MakeAt(
                                    loc4, binary::Instruction{MakeAt(
                                              loc5, Opcode::Nop)})})}),
     }},
     text::ElementList{text::ElementListWithExpressions{
         MakeAt(loc1, ReferenceType::Funcref),
         MakeAt(loc2, text::ElementExpressionList{MakeAt(
                          loc3, text::ElementExpression{MakeAt(
                                    loc4, text::Instruction{MakeAt(
                                              loc5, Opcode::Nop)})})})}});
}

TEST(ConvertToBinaryTest, ElementSegment) {
  auto binary_list = binary::ElementList{
      binary::ElementListWithIndexes{MakeAt(loc1, ExternalKind::Function),
                                     binary::IndexList{MakeAt(loc2, Index{0})}},
  };

  auto text_list = text::ElementList{
      text::ElementListWithVars{
          MakeAt(loc1, ExternalKind::Function),
          text::VarList{MakeAt(loc2, text::Var{Index{0}})}},
  };

  // Active.
  OK(MakeAt(loc1,
            binary::ElementSegment{
                MakeAt(loc2, Index{0}),
                MakeAt(loc3,
                       binary::ConstantExpression{
                           MakeAt(loc4, binary::Instruction{MakeAt(
                                            loc5, Opcode::Nop)}),
                       }),
                binary_list}),
     MakeAt(loc1,
            text::ElementSegment{
                nullopt, MakeAt(loc2, text::Var{Index{0}}),
                MakeAt(loc3,
                       text::ConstantExpression{
                           MakeAt(loc4,
                                  text::Instruction{MakeAt(loc5, Opcode::Nop)}),
                       }),
                text_list}));

  // Passive.
  OK(MakeAt(loc1, binary::ElementSegment{SegmentType::Passive, binary_list}),
     MakeAt(loc1,
            text::ElementSegment{nullopt, SegmentType::Passive, text_list}));
}

TEST(ConvertToBinaryTest, BlockImmediate) {
  // Void inline type.
  OK(MakeAt(loc1, binary::BlockType::Void),
     MakeAt(loc1, text::BlockImmediate{nullopt, text::FunctionTypeUse{}}));

  // Single inline type.
  OK(MakeAt(loc1, binary::BlockType::I32),
     MakeAt(loc1, text::BlockImmediate{
                      nullopt,
                      text::FunctionTypeUse{
                          nullopt, text::FunctionType{{}, {ValueType::I32}}}}));

  // Generic type (via multi-value proposal).
  OK(MakeAt(loc1, binary::BlockType(13)),
     MakeAt(loc1, text::BlockImmediate{
                      nullopt, text::FunctionTypeUse{
                                   text::Var{Index{13}},
                                   text::FunctionType{{ValueType::I32}, {}}}}));
}

TEST(ConvertToBinaryTest, BrOnExnImmediate) {
  OK(MakeAt(loc1, binary::BrOnExnImmediate{MakeAt(loc2, Index{13}),
                                           MakeAt(loc3, Index{14})}),
     MakeAt(loc1, text::BrOnExnImmediate{MakeAt(loc2, text::Var{Index{13}}),
                                         MakeAt(loc3, text::Var{Index{14}})}));
}

TEST(ConvertToBinaryTest, BrTableImmediate) {
  OK(MakeAt(loc1, binary::BrTableImmediate{{MakeAt(loc2, Index{13})},
                                           MakeAt(loc3, Index{14})}),
     MakeAt(loc1, text::BrTableImmediate{{MakeAt(loc2, text::Var{Index{13}})},
                                         MakeAt(loc3, text::Var{Index{14}})}));
}

TEST(ConvertToBinaryTest, CallIndirectImmediate) {
  // Table defined.
  OK(MakeAt(loc1, binary::CallIndirectImmediate{MakeAt(loc3, Index{13}),
                                                MakeAt(loc2, Index{14})}),
     MakeAt(loc1, text::CallIndirectImmediate{
                      MakeAt(loc2, text::Var{Index{14}}),
                      text::FunctionTypeUse{MakeAt(loc3, text::Var{Index{13}}),
                                            {}}}));

  // Table is nullopt.
  OK(MakeAt(loc1,
            binary::CallIndirectImmediate{MakeAt(loc2, Index{13}), Index{0}}),
     MakeAt(loc1, text::CallIndirectImmediate{
                      nullopt, text::FunctionTypeUse{
                                   MakeAt(loc2, text::Var{Index{13}}), {}}}));
}

TEST(ConvertToBinaryTest, CopyImmediate) {
  // dst and src defined.
  OK(MakeAt(loc1, binary::CopyImmediate{MakeAt(loc2, Index{13}),
                                        MakeAt(loc3, Index{14})}),
     MakeAt(loc1, text::CopyImmediate{MakeAt(loc2, text::Var{Index{13}}),
                                      MakeAt(loc3, text::Var{Index{14}})}));

  // dst and src are nullopt.
  OK(MakeAt(loc1, binary::CopyImmediate{Index{0}, Index{0}}),
     MakeAt(loc1, text::CopyImmediate{}));
}

TEST(ConvertToBinaryTest, InitImmediate) {
  // dst defined.
  OK(MakeAt(loc1, binary::InitImmediate{MakeAt(loc2, Index{13}),
                                        MakeAt(loc3, Index{14})}),
     MakeAt(loc1, text::InitImmediate{MakeAt(loc2, text::Var{Index{13}}),
                                      MakeAt(loc3, text::Var{Index{14}})}));

  // dst is nullopt.
  OK(MakeAt(loc1, binary::InitImmediate{MakeAt(loc2, Index{13}), Index{0}}),
     MakeAt(loc1,
            text::InitImmediate{MakeAt(loc2, text::Var{Index{13}}), nullopt}));
}

TEST(ConvertToBinaryTest, MemArgImmediate) {
  u32 natural_align = 16;
  u32 natural_align_log2 = 4;
  u32 align = 8;
  u32 align_log2 = 3;
  u32 offset = 5;

  // align and offset defined.
  OK(MakeAt(loc1, binary::MemArgImmediate{MakeAt(loc2, align_log2),
                                          MakeAt(loc3, offset)}),
     MakeAt(loc1,
            text::MemArgImmediate{MakeAt(loc2, align), MakeAt(loc3, offset)}),
     natural_align);

  // offset nullopt.
  OK(MakeAt(loc1, binary::MemArgImmediate{MakeAt(loc2, align_log2), u32{0}}),
     MakeAt(loc1, text::MemArgImmediate{MakeAt(loc2, align), nullopt}),
     natural_align);

  // align and offset are nullopt.
  OK(MakeAt(loc1, binary::MemArgImmediate{natural_align_log2, u32{0}}),
     MakeAt(loc1, text::MemArgImmediate{nullopt, nullopt}), natural_align);
}

TEST(ConvertToBinaryTest, Instruction) {
  // Bare.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::Nop)}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::Nop)}));

  // s32.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::I32Const),
                                      MakeAt(loc3, s32{0})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::I32Const),
                                    MakeAt(loc3, s32{0})}));

  // s64.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::I64Const),
                                      MakeAt(loc3, s64{0})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::I64Const),
                                    MakeAt(loc3, s64{0})}));

  // f32.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::F32Const),
                                      MakeAt(loc3, f32{0})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::F32Const),
                                    MakeAt(loc3, f32{0})}));

  // f64.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::F64Const),
                                      MakeAt(loc3, f64{0})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::F64Const),
                                    MakeAt(loc3, f64{0})}));

  // v128.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::V128Const),
                                      MakeAt(loc3, v128{})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::V128Const),
                                    MakeAt(loc3, v128{})}));

  // Var.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::LocalGet),
                                      MakeAt(loc3, Index{13})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::LocalGet),
                                    MakeAt(loc3, text::Var{Index{13}})}));

  // BlockImmediate.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::Block),
                                      binary::BlockType{13}}),
     MakeAt(loc1,
            text::Instruction{
                MakeAt(loc2, Opcode::Block),
                text::BlockImmediate{
                    nullopt, text::FunctionTypeUse{
                                 text::Var{Index{13}},
                                 text::FunctionType{{ValueType::I32}, {}}}}}));

  // BrOnExnImmediate.
  OK(MakeAt(
         loc1,
         binary::Instruction{
             MakeAt(loc2, Opcode::BrOnExn),
             MakeAt(loc3, binary::BrOnExnImmediate{MakeAt(loc4, Index{13}),
                                                   MakeAt(loc5, Index{14})})}),
     MakeAt(loc1, text::Instruction{
                      MakeAt(loc2, Opcode::BrOnExn),
                      MakeAt(loc3, text::BrOnExnImmediate{
                                       MakeAt(loc4, text::Var{Index{13}}),
                                       MakeAt(loc5, text::Var{Index{14}})})}));

  // BrTableImmediate.
  OK(MakeAt(
         loc1,
         binary::Instruction{
             MakeAt(loc2, Opcode::BrTable),
             MakeAt(loc3, binary::BrTableImmediate{{MakeAt(loc4, Index{13})},
                                                   MakeAt(loc5, Index{14})})}),
     MakeAt(loc1, text::Instruction{
                      MakeAt(loc2, Opcode::BrTable),
                      MakeAt(loc3, text::BrTableImmediate{
                                       {MakeAt(loc4, text::Var{Index{13}})},
                                       MakeAt(loc5, text::Var{Index{14}})})}));

  // CallIndirectImmediate.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::CallIndirect),
                                      MakeAt(loc3,
                                             binary::CallIndirectImmediate{
                                                 MakeAt(loc5, Index{13}),
                                                 MakeAt(loc4, Index{14})})}),
     MakeAt(
         loc1,
         text::Instruction{
             MakeAt(loc2, Opcode::CallIndirect),
             MakeAt(loc3, text::CallIndirectImmediate{
                              MakeAt(loc4, text::Var{Index{14}}),
                              text::FunctionTypeUse{
                                  MakeAt(loc5, text::Var{Index{13}}), {}}})}));

  // CopyImmediate.
  OK(MakeAt(loc1,
            binary::Instruction{
                MakeAt(loc2, Opcode::MemoryCopy),
                MakeAt(loc3, binary::CopyImmediate{MakeAt(loc4, Index{13}),
                                                   MakeAt(loc5, Index{14})})}),
     MakeAt(loc1, text::Instruction{
                      MakeAt(loc2, Opcode::MemoryCopy),
                      MakeAt(loc3, text::CopyImmediate{
                                       MakeAt(loc4, text::Var{Index{13}}),
                                       MakeAt(loc5, text::Var{Index{14}})})}));

  // InitImmediate.
  OK(MakeAt(loc1,
            binary::Instruction{
                MakeAt(loc2, Opcode::TableInit),
                MakeAt(loc3, binary::InitImmediate{MakeAt(loc4, Index{13}),
                                                   MakeAt(loc5, Index{14})})}),
     MakeAt(loc1, text::Instruction{
                      MakeAt(loc2, Opcode::TableInit),
                      MakeAt(loc3, text::InitImmediate{
                                       MakeAt(loc4, text::Var{Index{13}}),
                                       MakeAt(loc5, text::Var{Index{14}})})}));

  // MemArgImmediate.
  OK(MakeAt(loc1,
            binary::Instruction{
                MakeAt(loc2, Opcode::I32Load),
                MakeAt(loc3, binary::MemArgImmediate{MakeAt(loc4, u32{2}),
                                                     MakeAt(loc5, u32{13})})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::I32Load),
                                    MakeAt(loc3, text::MemArgImmediate{
                                                     MakeAt(loc4, u32{4}),
                                                     MakeAt(loc5, u32{13})})}));

  // ReferenceType.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::RefNull),
                                      MakeAt(loc3, ReferenceType::Funcref)}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::RefNull),
                                    MakeAt(loc3, ReferenceType::Funcref)}));

  // SelectImmediate.
  OK(MakeAt(loc1,
            binary::Instruction{MakeAt(loc2, Opcode::SelectT),
                                MakeAt(loc3, binary::SelectImmediate{MakeAt(
                                                 loc4, ValueType::I32)})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::SelectT),
                                    MakeAt(loc3, text::SelectImmediate{MakeAt(
                                                     loc4, ValueType::I32)})}));

  // ShuffleImmediate.
  OK(MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::V8X16Shuffle),
                                      MakeAt(loc3, ShuffleImmediate{})}),
     MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::V8X16Shuffle),
                                    MakeAt(loc3, ShuffleImmediate{})}));

  // SimdLaneImmediate.
  OK(MakeAt(loc1,
            binary::Instruction{MakeAt(loc2, Opcode::I8X16ExtractLaneS),
                                MakeAt(loc3, binary::SimdLaneImmediate{13})}),
     MakeAt(loc1,
            text::Instruction{MakeAt(loc2, Opcode::I8X16ExtractLaneS),
                              MakeAt(loc3, text::SimdLaneImmediate{13})}));
}

TEST(ConvertToBinaryTest, OpcodeAlignment) {
  struct {
    Opcode opcode;
    u32 expected_align_log2;
  } tests[] = {
      {Opcode::I32AtomicLoad8U, 0},
      {Opcode::I32AtomicRmw8AddU, 0},
      {Opcode::I32AtomicRmw8AndU, 0},
      {Opcode::I32AtomicRmw8CmpxchgU, 0},
      {Opcode::I32AtomicRmw8OrU, 0},
      {Opcode::I32AtomicRmw8SubU, 0},
      {Opcode::I32AtomicRmw8XchgU, 0},
      {Opcode::I32AtomicRmw8XorU, 0},
      {Opcode::I32AtomicStore8, 0},
      {Opcode::I32Load8S, 0},
      {Opcode::I32Load8U, 0},
      {Opcode::I32Store8, 0},
      {Opcode::I64AtomicLoad8U, 0},
      {Opcode::I64AtomicRmw8AddU, 0},
      {Opcode::I64AtomicRmw8AndU, 0},
      {Opcode::I64AtomicRmw8CmpxchgU, 0},
      {Opcode::I64AtomicRmw8OrU, 0},
      {Opcode::I64AtomicRmw8SubU, 0},
      {Opcode::I64AtomicRmw8XchgU, 0},
      {Opcode::I64AtomicRmw8XorU, 0},
      {Opcode::I64AtomicStore8, 0},
      {Opcode::I64Load8S, 0},
      {Opcode::I64Load8U, 0},
      {Opcode::I64Store8, 0},
      {Opcode::V8X16LoadSplat, 0},

      {Opcode::I32AtomicLoad16U, 1},
      {Opcode::I32AtomicRmw16AddU, 1},
      {Opcode::I32AtomicRmw16AndU, 1},
      {Opcode::I32AtomicRmw16CmpxchgU, 1},
      {Opcode::I32AtomicRmw16OrU, 1},
      {Opcode::I32AtomicRmw16SubU, 1},
      {Opcode::I32AtomicRmw16XchgU, 1},
      {Opcode::I32AtomicRmw16XorU, 1},
      {Opcode::I32AtomicStore16, 1},
      {Opcode::I32Load16S, 1},
      {Opcode::I32Load16U, 1},
      {Opcode::I32Store16, 1},
      {Opcode::I64AtomicLoad16U, 1},
      {Opcode::I64AtomicRmw16AddU, 1},
      {Opcode::I64AtomicRmw16AndU, 1},
      {Opcode::I64AtomicRmw16CmpxchgU, 1},
      {Opcode::I64AtomicRmw16OrU, 1},
      {Opcode::I64AtomicRmw16SubU, 1},
      {Opcode::I64AtomicRmw16XchgU, 1},
      {Opcode::I64AtomicRmw16XorU, 1},
      {Opcode::I64AtomicStore16, 1},
      {Opcode::I64Load16S, 1},
      {Opcode::I64Load16U, 1},
      {Opcode::I64Store16, 1},
      {Opcode::V16X8LoadSplat, 1},

      {Opcode::F32Load, 2},
      {Opcode::F32Store, 2},
      {Opcode::I32AtomicLoad, 2},
      {Opcode::I32AtomicRmwAdd, 2},
      {Opcode::I32AtomicRmwAnd, 2},
      {Opcode::I32AtomicRmwCmpxchg, 2},
      {Opcode::I32AtomicRmwOr, 2},
      {Opcode::I32AtomicRmwSub, 2},
      {Opcode::I32AtomicRmwXchg, 2},
      {Opcode::I32AtomicRmwXor, 2},
      {Opcode::I32AtomicStore, 2},
      {Opcode::I32Load, 2},
      {Opcode::I32Store, 2},
      {Opcode::I64AtomicLoad32U, 2},
      {Opcode::I64AtomicRmw32AddU, 2},
      {Opcode::I64AtomicRmw32AndU, 2},
      {Opcode::I64AtomicRmw32CmpxchgU, 2},
      {Opcode::I64AtomicRmw32OrU, 2},
      {Opcode::I64AtomicRmw32SubU, 2},
      {Opcode::I64AtomicRmw32XchgU, 2},
      {Opcode::I64AtomicRmw32XorU, 2},
      {Opcode::I64AtomicStore32, 2},
      {Opcode::I64Load32S, 2},
      {Opcode::I64Load32U, 2},
      {Opcode::I64Store32, 2},
      {Opcode::MemoryAtomicNotify, 2},
      {Opcode::MemoryAtomicWait32, 2},
      {Opcode::V32X4LoadSplat, 2},

      {Opcode::F64Load, 3},
      {Opcode::F64Store, 3},
      {Opcode::I16X8Load8X8S, 3},
      {Opcode::I16X8Load8X8U, 3},
      {Opcode::I32X4Load16X4S, 3},
      {Opcode::I32X4Load16X4U, 3},
      {Opcode::I64AtomicLoad, 3},
      {Opcode::I64AtomicRmwAdd, 3},
      {Opcode::I64AtomicRmwAnd, 3},
      {Opcode::I64AtomicRmwCmpxchg, 3},
      {Opcode::I64AtomicRmwOr, 3},
      {Opcode::I64AtomicRmwSub, 3},
      {Opcode::I64AtomicRmwXchg, 3},
      {Opcode::I64AtomicRmwXor, 3},
      {Opcode::I64AtomicStore, 3},
      {Opcode::I64Load, 3},
      {Opcode::I64Store, 3},
      {Opcode::I64X2Load32X2S, 3},
      {Opcode::I64X2Load32X2U, 3},
      {Opcode::MemoryAtomicWait64, 3},
      {Opcode::V64X2LoadSplat, 3},

      {Opcode::V128Load, 4},
      {Opcode::V128Store, 4},
  };
  for (auto& test : tests) {
    Context context;
    auto result = ToBinary(
        context, text::Instruction{test.opcode, text::MemArgImmediate{}});
    EXPECT_EQ(test.expected_align_log2,
              result->mem_arg_immediate()->align_log2);
  }
}

TEST(ConvertToBinaryTest, InstructionList) {
  OK(
      binary::InstructionList{
          MakeAt(loc1, binary::Instruction{MakeAt(loc2, Opcode::Nop)}),
          MakeAt(loc3, binary::Instruction{MakeAt(loc4, Opcode::Nop)})},
      text::InstructionList{
          MakeAt(loc1, text::Instruction{MakeAt(loc2, Opcode::Nop)}),
          MakeAt(loc3, text::Instruction{MakeAt(loc4, Opcode::Nop)})});
}

TEST(ConvertToBinaryTest, Expression) {
  OKFunc(ToBinaryExpression,
         MakeAt(loc1,
                binary::Expression{
                    "\x01"      // nop
                    "\x41\x00"  // i32.const 0
                    "\x1a"_su8  // drop
                }),
         MakeAt(loc1, text::InstructionList{
                          text::Instruction{Opcode::Nop},
                          text::Instruction{Opcode::I32Const, s32{0}},
                          text::Instruction{Opcode::Drop},
                      }));
}

TEST(ConvertToBinaryTest, LocalsList) {
  OKFunc(ToBinaryLocalsList,
         MakeAt(loc1,
                binary::LocalsList{
                    binary::Locals{2, MakeAt(loc2, ValueType::I32)},
                    binary::Locals{1, MakeAt(loc4, ValueType::F32)}}),
         MakeAt(loc1,
                text::BoundValueTypeList{
                    text::BoundValueType{nullopt, MakeAt(loc2, ValueType::I32)},
                    text::BoundValueType{nullopt, MakeAt(loc3, ValueType::I32)},
                    text::BoundValueType{nullopt, MakeAt(loc4, ValueType::F32)},
                }));
}

TEST(ConvertToBinaryTest, Code) {
  OKFunc(
      ToBinaryCode,
      MakeAt(loc1,
             binary::Code{MakeAt(loc2, binary::LocalsList{binary::Locals{
                                           1, MakeAt(loc3, ValueType::I32)}}),
                          binary::Expression{"\x01"_su8}}),
      MakeAt(loc1,
             text::Function{
                 {},
                 MakeAt(loc2, text::BoundValueTypeList{text::BoundValueType{
                                  nullopt, MakeAt(loc3, ValueType::I32)}}),
                 {text::Instruction{Opcode::Nop}},
                 {}}));
}

TEST(ConvertToBinaryTest, DataSegment) {
  // Active.
  OK(MakeAt(
         loc1,
         binary::DataSegment{MakeAt(loc2, Index{13}),
                             MakeAt(loc3, binary::ConstantExpression{MakeAt(
                                              loc4, binary::Instruction{MakeAt(
                                                        loc5, Opcode::Nop)})}),
                             "hello\x00"_su8}),
     MakeAt(loc1,
            text::DataSegment{nullopt, MakeAt(loc2, text::Var{Index{13}}),
                              MakeAt(loc3, text::ConstantExpression{MakeAt(
                                               loc4, text::Instruction{MakeAt(
                                                         loc5, Opcode::Nop)})}),
                              text::TextList{text::Text{"\"hello\"", 5},
                                             text::Text{"\"\\00\"", 1}}}));
}

TEST(ConvertToBinaryTest, EventType) {
  OK(MakeAt(loc1,
            binary::EventType{
                EventAttribute::Exception,
                MakeAt(loc2, Index{0}),
            }),
     MakeAt(loc1, text::EventType{
                      EventAttribute::Exception,
                      text::FunctionTypeUse{
                          MakeAt(loc2, text::Var{Index{0}}),
                          {},
                      },
                  }));
}

TEST(ConvertToBinaryTest, Event) {
  OK(MakeAt(loc1, binary::Event{MakeAt(loc2,
                                       binary::EventType{
                                           EventAttribute::Exception,
                                           MakeAt(loc3, Index{0}),
                                       })}),
     MakeAt(loc1,
            text::Event{text::EventDesc{
                            nullopt,
                            MakeAt(loc2,
                                   text::EventType{
                                       EventAttribute::Exception,
                                       text::FunctionTypeUse{
                                           MakeAt(loc3, text::Var{Index{0}}),
                                           {},
                                       },
                                   })},
                        {}}));
}
