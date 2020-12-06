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
#include "test/binary/constants.h"
#include "test/text/constants.h"
#include "wasp/binary/formatters.h"
#include "wasp/text/formatters.h"

using namespace ::wasp;
using namespace ::wasp::convert;
namespace tt = ::wasp::text::test;

namespace {

class ConvertToBinaryTest : public ::testing::Test {
 protected:
  template <typename B, typename T, typename... Args>
  void OK(const B& expected, const T& input, Args&&... args) {
    auto actual = ToBinary(context, input, std::forward<Args>(args)...);
    EXPECT_EQ(expected, actual);
  }

  template <typename F, typename B, typename T, typename... Args>
  void OKFunc(const F& func,
              const B& expected,
              const T& input,
              Args&&... args) {
    EXPECT_EQ(expected, func(context, input, std::forward<Args>(args)...));
  }

  Context context;
};

const SpanU8 loc1 = "A"_su8;
const SpanU8 loc2 = "B"_su8;
const SpanU8 loc3 = "C"_su8;
const SpanU8 loc4 = "D"_su8;
const SpanU8 loc5 = "E"_su8;
const SpanU8 loc6 = "F"_su8;
const SpanU8 loc7 = "G"_su8;
const SpanU8 loc8 = "H"_su8;

// Similar to the definitions in test/binary/constants, but using text
// locations (e.g. "i32").
const binary::HeapType BHT_Func{At{"func"_su8, HeapKind::Func}};
const binary::HeapType BHT_0{At{"0"_su8, Index{0}}};
const binary::ReferenceType BRT_Funcref{
    At{"funcref"_su8, ReferenceKind::Funcref}};
const binary::ValueType BVT_I32{At{"i32"_su8, NumericType::I32}};
const binary::ValueType BVT_F32{At{"f32"_su8, NumericType::F32}};
const binary::ValueType BVT_Funcref{At{"funcref"_su8, BRT_Funcref}};

}  // namespace

TEST_F(ConvertToBinaryTest, HeapType) {
  // HeapKind
  OK(BHT_Func, At{"func"_su8, tt::HT_Func});

  // Index
  OK(BHT_0, At{"0"_su8, tt::HT_0});
}

TEST_F(ConvertToBinaryTest, RefType) {
  OK(At{loc1, binary::RefType{BHT_Func, At{loc2, Null::No}}},
     At{loc1, text::RefType{tt::HT_Func, At{loc2, Null::No}}});
}

TEST_F(ConvertToBinaryTest, ReferenceType) {
  // ReferenceKind
  OK(At{loc1, binary::ReferenceType{At{loc2, ReferenceKind::Funcref}}},
     At{loc1, text::ReferenceType{At{loc2, ReferenceKind::Funcref}}});

  // RefType
  OK(At{loc1,
        binary::ReferenceType{binary::RefType{BHT_Func, At{loc2, Null::No}}}},
     At{loc1,
        text::ReferenceType{text::RefType{tt::HT_Func, At{loc2, Null::No}}}});
}

TEST_F(ConvertToBinaryTest, Rtt) {
  OK(At{loc1, binary::Rtt{At{loc2, Index{0}}, At{loc3, BHT_Func}}},
     At{loc1, text::Rtt{At{loc2, Index{0}}, At{loc3, tt::HT_Func}}});
}

TEST_F(ConvertToBinaryTest, ValueType) {
  // NumericKind
  OK(At{loc1, BVT_I32}, At{loc1, tt::VT_I32});

  // ReferenceType
  OK(At{loc1, BVT_Funcref}, At{loc1, tt::VT_Funcref});

  // Rtt
  OK(At{loc1, binary::ValueType{At{
                  loc2, binary::Rtt{At{loc3, Index{0}}, At{loc4, BHT_Func}}}}},
     At{loc1, text::ValueType{At{loc2, text::Rtt{At{loc3, Index{0}},
                                                 At{loc4, tt::HT_Func}}}}});
}

TEST_F(ConvertToBinaryTest, ValueTypeList) {
  OK(binary::ValueTypeList{binary::ValueType{
         At{loc2, binary::Rtt{At{loc3, Index{0}}, At{loc4, BHT_Func}}}}},
     text::ValueTypeList{text::ValueType{
         At{loc2, text::Rtt{At{loc3, Index{0}}, At{loc4, tt::HT_Func}}}}});
}

TEST_F(ConvertToBinaryTest, StorageType) {
  // ValueType
  OK(At{loc1, binary::StorageType{At{loc2, BVT_I32}}},
     At{loc1, text::StorageType{At{loc2, tt::VT_I32}}});

  // PackedType
  OK(At{loc1, binary::StorageType{At{loc2, PackedType::I8}}},
     At{loc1, text::StorageType{At{loc2, PackedType::I8}}});
}

TEST_F(ConvertToBinaryTest, VarList) {
  OK(
      binary::IndexList{
          At{loc1, Index{0}},
          At{loc2, Index{1}},
      },
      text::VarList{
          At{loc1, text::Var{Index{0}}},
          At{loc2, text::Var{Index{1}}},
      });
}

TEST_F(ConvertToBinaryTest, FunctionType) {
  OK(At{loc1,
        binary::FunctionType{
            binary::ValueTypeList{At{loc2, BVT_I32}},
            binary::ValueTypeList{At{loc3, BVT_F32}},
        }},
     At{loc1, text::FunctionType{
                  text::ValueTypeList{At{loc2, tt::VT_I32}},
                  text::ValueTypeList{At{loc3, tt::VT_F32}},
              }});
}

TEST_F(ConvertToBinaryTest, FieldType) {
  OK(At{loc1,
        binary::FieldType{At{loc2, binary::StorageType{At{loc3, BVT_I32}}},
                          At{loc4, Mutability::Const}}},
     At{loc1, text::FieldType{nullopt,
                              At{loc2, text::StorageType{At{loc3, tt::VT_I32}}},
                              At{loc4, Mutability::Const}}});
}

TEST_F(ConvertToBinaryTest, FieldTypeList) {
  OK(binary::FieldTypeList{At{
         loc1,
         binary::FieldType{At{loc2, binary::StorageType{At{loc3, BVT_I32}}},
                           At{loc4, Mutability::Const}}}},
     text::FieldTypeList{At{
         loc1, text::FieldType{
                   nullopt, At{loc2, text::StorageType{At{loc3, tt::VT_I32}}},
                   At{loc4, Mutability::Const}}}});
}

TEST_F(ConvertToBinaryTest, StructType) {
  OK(At{loc1,
        binary::StructType{binary::FieldTypeList{At{
            loc2,
            binary::FieldType{At{loc3, binary::StorageType{At{loc4, BVT_I32}}},
                              At{loc5, Mutability::Const}}}}}},
     At{loc1, text::StructType{text::FieldTypeList{At{
                  loc2, text::FieldType{
                            nullopt,
                            At{loc3, text::StorageType{At{loc4, tt::VT_I32}}},
                            At{loc5, Mutability::Const}}}}}});
}

TEST_F(ConvertToBinaryTest, ArrayType) {
  OK(At{loc1,
        binary::ArrayType{At{
            loc2,
            binary::FieldType{At{loc3, binary::StorageType{At{loc4, BVT_I32}}},
                              At{loc5, Mutability::Const}}}}},
     At{loc1, text::ArrayType{At{
                  loc2, text::FieldType{
                            nullopt,
                            At{loc3, text::StorageType{At{loc4, tt::VT_I32}}},
                            At{loc5, Mutability::Const}}}}});
}

TEST_F(ConvertToBinaryTest, DefinedType) {
  // FunctionType
  OK(At{loc1,
        binary::DefinedType{At{loc2,
                               binary::FunctionType{
                                   binary::ValueTypeList{At{loc3, BVT_I32}},
                                   binary::ValueTypeList{At{loc4, BVT_F32}},
                               }}}},
     At{loc1, text::DefinedType{
                  nullopt,
                  At{loc2, text::BoundFunctionType{
                               text::BoundValueTypeList{text::BoundValueType{
                                   nullopt, At{loc3, tt::VT_I32}}},
                               text::ValueTypeList{At{loc4, tt::VT_F32}}}}}});

  // StructType
  OK(At{loc1,
        binary::DefinedType{
            At{loc2,
               binary::StructType{binary::FieldTypeList{At{
                   loc3, binary::FieldType{At{loc4, binary::StorageType{At{
                                                        loc5, BVT_I32}}},
                                           At{loc6, Mutability::Const}}}}}}}},
     At{loc1,
        text::DefinedType{
            nullopt,
            At{loc2,
               text::StructType{text::FieldTypeList{At{
                   loc3, text::FieldType{
                             nullopt,
                             At{loc4, text::StorageType{At{loc5, tt::VT_I32}}},
                             At{loc6, Mutability::Const}}}}}}}});

  // ArrayType
  OK(At{loc1,
        binary::DefinedType{At{
            loc2, binary::ArrayType{At{
                      loc3, binary::FieldType{At{loc4, binary::StorageType{At{
                                                           loc5, BVT_I32}}},
                                              At{loc6, Mutability::Const}}}}}}},
     At{loc1,
        text::DefinedType{
            nullopt,
            At{loc2,
               text::ArrayType{At{
                   loc3, text::FieldType{
                             nullopt,
                             At{loc4, text::StorageType{At{loc5, tt::VT_I32}}},
                             At{loc6, Mutability::Const}}}}}}});
}

TEST_F(ConvertToBinaryTest, Import) {
  // Function
  OK(At{loc1,
        binary::Import{At{loc2, "m"_sv}, At{loc3, "n"_sv}, At{loc4, Index{0}}}},
     At{loc1,
        text::Import{
            At{loc2, text::Text{"\"m\"", 1}}, At{loc3, text::Text{"\"n\"", 1}},
            text::FunctionDesc{nullopt, At{loc4, text::Var{Index{0}}}, {}}}});

  // Table
  OK(At{loc1,
        binary::Import{At{loc2, "m"_sv}, At{loc3, "n"_sv},
                       At{loc4,
                          binary::TableType{
                              At{loc5, Limits{At{loc6, u32{1}}}},
                              At{loc7, BRT_Funcref},
                          }}}},
     At{loc1,
        text::Import{
            At{loc2, text::Text{"\"m\"", 1}}, At{loc3, text::Text{"\"n\"", 1}},
            text::TableDesc{nullopt,
                            At{loc4, text::TableType{
                                         At{loc5, Limits{At{loc6, u32{1}}}},
                                         At{loc7, tt::RT_Funcref},
                                     }}}}});

  // Memory
  OK(At{loc1, binary::Import{At{loc2, "m"_sv}, At{loc3, "n"_sv},
                             At{loc4,
                                MemoryType{
                                    At{loc5, Limits{At{loc6, u32{1}}}},
                                }}}},
     At{loc1,
        text::Import{
            At{loc2, text::Text{"\"m\"", 1}}, At{loc3, text::Text{"\"n\"", 1}},
            text::MemoryDesc{nullopt,
                             At{loc4, MemoryType{
                                          At{loc5, Limits{At{loc6, u32{1}}}},
                                      }}}}});

  // Global
  OK(At{loc1, binary::Import{At{loc2, "m"_sv}, At{loc3, "n"_sv},
                             At{loc4,
                                binary::GlobalType{
                                    At{loc5, BVT_I32},
                                    Mutability::Const,
                                }}}},
     At{loc1,
        text::Import{
            At{loc2, text::Text{"\"m\"", 1}}, At{loc3, text::Text{"\"n\"", 1}},
            text::GlobalDesc{nullopt, At{loc4, text::GlobalType{
                                                   At{loc5, tt::VT_I32},
                                                   Mutability::Const,
                                               }}}}});

  // Event
  OK(At{loc1, binary::Import{At{loc2, "m"_sv}, At{loc3, "n"_sv},
                             At{loc4,
                                binary::EventType{
                                    EventAttribute::Exception,
                                    At{loc5, Index{0}},
                                }}}},
     At{loc1,
        text::Import{
            At{loc2, text::Text{"\"m\"", 1}}, At{loc3, text::Text{"\"n\"", 1}},
            text::EventDesc{
                nullopt,
                At{loc4,
                   text::EventType{EventAttribute::Exception,
                                   text::FunctionTypeUse{
                                       At{loc5, text::Var{Index{0}}}, {}}}}}}});
}

TEST_F(ConvertToBinaryTest, Function) {
  OK(At{loc1, binary::Function{At{loc2, Index{13}}}},
     At{loc1, text::Function{text::FunctionDesc{
                                 nullopt, At{loc2, text::Var{Index{13}}}, {}},
                             {},
                             {},
                             {}}});
}

TEST_F(ConvertToBinaryTest, Table) {
  auto binary_table_type =
      At{loc1,
         binary::TableType{Limits{At{loc2, Index{0}}}, At{loc3, BRT_Funcref}}};
  auto text_table_type = At{loc1, text::TableType{Limits{At{loc2, Index{0}}},
                                                  At{loc3, tt::RT_Funcref}}};

  OK(At{loc4, binary::Table{binary_table_type}},
     At{loc4, text::Table{text::TableDesc{nullopt, text_table_type}, {}}});
}

TEST_F(ConvertToBinaryTest, Memory) {
  auto memory_type = At{loc1, MemoryType{Limits{At{loc2, Index{0}}}}};

  OK(At{loc3, binary::Memory{memory_type}},
     At{loc3, text::Memory{text::MemoryDesc{nullopt, memory_type}, {}}});
}

TEST_F(ConvertToBinaryTest, Global) {
  auto binary_global_type = At{loc1, binary::GlobalType{
                                         At{loc2, BVT_I32},
                                         At{Mutability::Const},
                                     }};

  auto text_global_type = At{loc1, text::GlobalType{
                                       At{loc2, tt::VT_I32},
                                       At{Mutability::Const},
                                   }};

  OK(At{loc3,
        binary::Global{
            binary_global_type,
            At{loc4,
               binary::ConstantExpression{
                   At{loc5, binary::Instruction{At{loc6, Opcode::Nop}}},
               }},
        }},
     At{loc3,
        text::Global{text::GlobalDesc{nullopt, text_global_type},
                     At{loc4,
                        text::ConstantExpression{
                            At{loc5, text::Instruction{At{loc6, Opcode::Nop}}},
                        }},
                     {}}});
}

TEST_F(ConvertToBinaryTest, Export) {
  OK(At{loc1,
        binary::Export{
            At{loc2, ExternalKind::Function},
            At{loc3, "hello"_sv},
            At{loc4, Index{13}},
        }},
     At{loc1, text::Export{
                  At{loc2, ExternalKind::Function},
                  At{loc3, text::Text{"\"hello\""_sv, 5}},
                  At{loc4, text::Var{Index{13}}},
              }});
}

TEST_F(ConvertToBinaryTest, Start) {
  OK(At{loc1, binary::Start{At{loc2, Index{13}}}},
     At{loc1, text::Start{At{loc2, text::Var{Index{13}}}}});
}

TEST_F(ConvertToBinaryTest, ElementExpression) {
  OK(At{loc1, binary::ElementExpression{{
                  At{loc2, binary::Instruction{At{loc3, Opcode::Unreachable}}},
                  At{loc4, binary::Instruction{At{loc5, Opcode::Nop}}},
              }}},
     At{loc1, text::ElementExpression{{
                  At{loc2, text::Instruction{At{loc3, Opcode::Unreachable}}},
                  At{loc4, text::Instruction{At{loc5, Opcode::Nop}}},
              }}});
}

TEST_F(ConvertToBinaryTest, ElementExpressionList) {
  OK(At{loc1,
        binary::ElementExpressionList{
            At{loc2,
               binary::ElementExpression{At{
                   loc3, binary::Instruction{At{loc4, Opcode::Unreachable}}}}},
            At{loc5, binary::ElementExpression{At{
                         loc6, binary::Instruction{At{loc7, Opcode::Nop}}}}},
        }},
     At{loc1,
        text::ElementExpressionList{
            At{loc2,
               text::ElementExpression{
                   At{loc3, text::Instruction{At{loc4, Opcode::Unreachable}}}}},
            At{loc5, text::ElementExpression{At{
                         loc6, text::Instruction{At{loc7, Opcode::Nop}}}}},
        }});
}

TEST_F(ConvertToBinaryTest, ElementList) {
  // text::Var -> binary::Index
  OK(binary::ElementList{binary::ElementListWithIndexes{
         At{loc1, ExternalKind::Function},
         At{loc2,
            binary::IndexList{
                At{loc3, Index{0}},
                At{loc4, Index{1}},
            }},
     }},
     text::ElementList{text::ElementListWithVars{
         At{loc1, ExternalKind::Function},
         At{loc2,
            text::VarList{
                At{loc3, text::Var{Index{0}}},
                At{loc4, text::Var{Index{1}}},
            }},
     }});

  // ElementExpression.
  OK(binary::ElementList{binary::ElementListWithExpressions{
         At{loc1, BRT_Funcref},
         At{loc2,
            binary::ElementExpressionList{At{
                loc3, binary::ElementExpression{At{
                          loc4, binary::Instruction{At{loc5, Opcode::Nop}}}}}}},
     }},
     text::ElementList{text::ElementListWithExpressions{
         At{loc1, tt::RT_Funcref},
         At{loc2, text::ElementExpressionList{
                      At{loc3, text::ElementExpression{
                                   At{loc4, text::Instruction{
                                                At{loc5, Opcode::Nop}}}}}}}}});
}

TEST_F(ConvertToBinaryTest, ElementSegment) {
  auto binary_list = binary::ElementList{
      binary::ElementListWithIndexes{At{loc1, ExternalKind::Function},
                                     binary::IndexList{At{loc2, Index{0}}}},
  };

  auto text_list = text::ElementList{
      text::ElementListWithVars{At{loc1, ExternalKind::Function},
                                text::VarList{At{loc2, text::Var{Index{0}}}}},
  };

  // Active.
  OK(At{loc1,
        binary::ElementSegment{
            At{loc2, Index{0}},
            At{loc3,
               binary::ConstantExpression{
                   At{loc4, binary::Instruction{At{loc5, Opcode::Nop}}},
               }},
            binary_list}},
     At{loc1,
        text::ElementSegment{
            nullopt, At{loc2, text::Var{Index{0}}},
            At{loc3,
               text::ConstantExpression{
                   At{loc4, text::Instruction{At{loc5, Opcode::Nop}}},
               }},
            text_list}});

  // Passive.
  OK(At{loc1, binary::ElementSegment{SegmentType::Passive, binary_list}},
     At{loc1, text::ElementSegment{nullopt, SegmentType::Passive, text_list}});
}

TEST_F(ConvertToBinaryTest, BlockImmediate) {
  // Void inline type.
  OK(At{loc1, binary::BlockType{binary::VoidType{}}},
     At{loc1, text::BlockImmediate{nullopt, text::FunctionTypeUse{}}});

  // Single inline type.
  OK(At{loc1, binary::BlockType{BVT_I32}},
     At{loc1,
        text::BlockImmediate{
            nullopt, text::FunctionTypeUse{
                         nullopt, text::FunctionType{{}, {tt::VT_I32}}}}});

  // Generic type (via multi-value proposal).
  OK(At{loc1, binary::BlockType(13)},
     At{loc1, text::BlockImmediate{nullopt,
                                   text::FunctionTypeUse{
                                       text::Var{Index{13}},
                                       text::FunctionType{{tt::VT_I32}, {}}}}});
}

#if 0
TEST_F(ConvertToBinaryTest, BrOnCastImmediate) {
  OK(At{loc1,
        binary::BrOnCastImmediate{
            At{loc2, Index{13}},
            At{loc3, binary::HeapType2Immediate{At{loc4, BHT_Func},
                                                At{loc5, BHT_Func}}}}},
     At{loc1, text::BrOnCastImmediate{
                  At{loc2, text::Var{Index{13}}},
                  At{loc3, text::HeapType2Immediate{At{loc4, tt::HT_Func},
                                                    At{loc5, tt::HT_Func}}}}});
}
#endif

TEST_F(ConvertToBinaryTest, BrOnExnImmediate) {
  OK(At{loc1,
        binary::BrOnExnImmediate{At{loc2, Index{13}}, At{loc3, Index{14}}}},
     At{loc1, text::BrOnExnImmediate{At{loc2, text::Var{Index{13}}},
                                     At{loc3, text::Var{Index{14}}}}});
}

TEST_F(ConvertToBinaryTest, BrTableImmediate) {
  OK(At{loc1,
        binary::BrTableImmediate{{At{loc2, Index{13}}}, At{loc3, Index{14}}}},
     At{loc1, text::BrTableImmediate{{At{loc2, text::Var{Index{13}}}},
                                     At{loc3, text::Var{Index{14}}}}});
}

TEST_F(ConvertToBinaryTest, CallIndirectImmediate) {
  // Table defined.
  OK(At{loc1, binary::CallIndirectImmediate{At{loc3, Index{13}},
                                            At{loc2, Index{14}}}},
     At{loc1, text::CallIndirectImmediate{
                  At{loc2, text::Var{Index{14}}},
                  text::FunctionTypeUse{At{loc3, text::Var{Index{13}}}, {}}}});

  // Table is nullopt.
  OK(At{loc1, binary::CallIndirectImmediate{At{loc2, Index{13}}, Index{0}}},
     At{loc1, text::CallIndirectImmediate{
                  nullopt,
                  text::FunctionTypeUse{At{loc2, text::Var{Index{13}}}, {}}}});
}

TEST_F(ConvertToBinaryTest, CopyImmediate) {
  // dst and src defined.
  OK(At{loc1, binary::CopyImmediate{At{loc2, Index{13}}, At{loc3, Index{14}}}},
     At{loc1, text::CopyImmediate{At{loc2, text::Var{Index{13}}},
                                  At{loc3, text::Var{Index{14}}}}});

  // dst and src are nullopt.
  OK(At{loc1, binary::CopyImmediate{Index{0}, Index{0}}},
     At{loc1, text::CopyImmediate{}});
}

TEST_F(ConvertToBinaryTest, FuncBindImmediate) {
  OK(At{loc1, binary::FuncBindImmediate{At{loc2, Index{13}}}},
     At{loc1, text::FuncBindImmediate{At{loc2, text::Var{Index{13}}}, {}}});
}

TEST_F(ConvertToBinaryTest, HeapType2Immediate) {
  OK(At{loc1,
        binary::HeapType2Immediate{At{loc2, BHT_Func}, At{loc3, BHT_Func}}},
     At{loc1, text::HeapType2Immediate{At{loc2, tt::HT_Func},
                                       At{loc3, tt::HT_Func}}});
}

TEST_F(ConvertToBinaryTest, InitImmediate) {
  // dst defined.
  OK(At{loc1, binary::InitImmediate{At{loc2, Index{13}}, At{loc3, Index{14}}}},
     At{loc1, text::InitImmediate{At{loc2, text::Var{Index{13}}},
                                  At{loc3, text::Var{Index{14}}}}});

  // dst is nullopt.
  OK(At{loc1, binary::InitImmediate{At{loc2, Index{13}}, Index{0}}},
     At{loc1, text::InitImmediate{At{loc2, text::Var{Index{13}}}, nullopt}});
}

TEST_F(ConvertToBinaryTest, LetImmediate) {
  // Empty let immediate.
  OK(At{loc1, binary::LetImmediate{binary::BlockType{binary::VoidType{}}, {}}},
     At{loc1, text::LetImmediate{}});

  // Let immediate with locals.
  OK(At{loc1, binary::LetImmediate{binary::BlockType{binary::VoidType{}},
                                   At{loc2, binary::LocalsList{binary::Locals{
                                                2, At{loc3, BVT_I32}}}}}},
     At{loc1,
        text::LetImmediate{
            text::BlockImmediate{},
            {At{loc2, text::BoundValueTypeList{
                          text::BoundValueType{nullopt, At{loc3, tt::VT_I32}},
                          text::BoundValueType{nullopt, tt::VT_I32},
                      }}}}});
}

TEST_F(ConvertToBinaryTest, MemArgImmediate) {
  u32 natural_align = 16;
  u32 natural_align_log2 = 4;
  u32 align = 8;
  u32 align_log2 = 3;
  u32 offset = 5;

  // align and offset defined.
  OK(At{loc1, binary::MemArgImmediate{At{loc2, align_log2}, At{loc3, offset}}},
     At{loc1, text::MemArgImmediate{At{loc2, align}, At{loc3, offset}}},
     natural_align);

  // offset nullopt.
  OK(At{loc1, binary::MemArgImmediate{At{loc2, align_log2}, u32{0}}},
     At{loc1, text::MemArgImmediate{At{loc2, align}, nullopt}}, natural_align);

  // align and offset are nullopt.
  OK(At{loc1, binary::MemArgImmediate{natural_align_log2, u32{0}}},
     At{loc1, text::MemArgImmediate{nullopt, nullopt}}, natural_align);
}

#if 0
TEST_F(ConvertToBinaryTest, RttSubImmediate) {
  OK(At{loc1,
        binary::RttSubImmediate{
            At{loc2, Index{13}},
            At{loc3, binary::HeapType2Immediate{At{loc4, BHT_Func},
                                                At{loc5, BHT_Func}}}}},
     At{loc1, text::RttSubImmediate{
                  At{loc2, Index{13}},
                  At{loc3, text::HeapType2Immediate{At{loc4, tt::HT_Func},
                                                    At{loc5, tt::HT_Func}}}}});
}
#endif

TEST_F(ConvertToBinaryTest, StructFieldImmediate) {
  OK(At{loc1,
        binary::StructFieldImmediate{At{loc2, Index{13}}, At{loc3, Index{14}}}},
     At{loc1, text::StructFieldImmediate{At{loc2, text::Var{Index{13}}},
                                         At{loc3, text::Var{Index{14}}}}});
}

TEST_F(ConvertToBinaryTest, Instruction) {
  // Bare.
  OK(At{loc1, binary::Instruction{At{loc2, Opcode::Nop}}},
     At{loc1, text::Instruction{At{loc2, Opcode::Nop}}});

  // s32.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::I32Const}, At{loc3, s32{0}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::I32Const}, At{loc3, s32{0}}}});

  // s64.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::I64Const}, At{loc3, s64{0}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::I64Const}, At{loc3, s64{0}}}});

  // f32.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::F32Const}, At{loc3, f32{0}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::F32Const}, At{loc3, f32{0}}}});

  // f64.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::F64Const}, At{loc3, f64{0}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::F64Const}, At{loc3, f64{0}}}});

  // v128.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::V128Const}, At{loc3, v128{}}}},
     At{loc1,
        text::Instruction{At{loc2, Opcode::V128Const}, At{loc3, v128{}}}});

  // Var.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::LocalGet}, At{loc3, Index{13}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::LocalGet},
                                At{loc3, text::Var{Index{13}}}}});

  // BlockImmediate.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::Block}, binary::BlockType{13}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::Block},
                  text::BlockImmediate{
                      nullopt, text::FunctionTypeUse{
                                   text::Var{Index{13}},
                                   text::FunctionType{{tt::VT_I32}, {}}}}}});

  // BrOnExnImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::BrOnExn},
            At{loc3, binary::BrOnExnImmediate{At{loc4, Index{13}},
                                              At{loc5, Index{14}}}}}},
     At{loc1,
        text::Instruction{
            At{loc2, Opcode::BrOnExn},
            At{loc3, text::BrOnExnImmediate{At{loc4, text::Var{Index{13}}},
                                            At{loc5, text::Var{Index{14}}}}}}});

  // BrTableImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::BrTable},
            At{loc3, binary::BrTableImmediate{{At{loc4, Index{13}}},
                                              At{loc5, Index{14}}}}}},
     At{loc1,
        text::Instruction{
            At{loc2, Opcode::BrTable},
            At{loc3, text::BrTableImmediate{{At{loc4, text::Var{Index{13}}}},
                                            At{loc5, text::Var{Index{14}}}}}}});

  // CallIndirectImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::CallIndirect},
            At{loc3, binary::CallIndirectImmediate{At{loc5, Index{13}},
                                                   At{loc4, Index{14}}}}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::CallIndirect},
                  At{loc3, text::CallIndirectImmediate{
                               At{loc4, text::Var{Index{14}}},
                               text::FunctionTypeUse{
                                   At{loc5, text::Var{Index{13}}}, {}}}}}});

  // CopyImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::MemoryCopy},
            At{loc3, binary::CopyImmediate{At{loc4, Index{13}},
                                           At{loc5, Index{14}}}}}},
     At{loc1,
        text::Instruction{
            At{loc2, Opcode::MemoryCopy},
            At{loc3, text::CopyImmediate{At{loc4, text::Var{Index{13}}},
                                         At{loc5, text::Var{Index{14}}}}}}});

  // FuncBindImmediate
  OK(At{loc1, binary::Instruction{At{loc2, Opcode::FuncBind},
                                  At{loc3, binary::FuncBindImmediate{At{
                                               loc4, Index{13}}}}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::FuncBind},
                  At{loc3, text::FuncBindImmediate{text::FunctionTypeUse{
                               At{loc4, text::Var{Index{13}}}, {}}}}}});

  // InitImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::TableInit},
            At{loc3, binary::InitImmediate{At{loc4, Index{13}},
                                           At{loc5, Index{14}}}}}},
     At{loc1,
        text::Instruction{
            At{loc2, Opcode::TableInit},
            At{loc3, text::InitImmediate{At{loc4, text::Var{Index{13}}},
                                         At{loc5, text::Var{Index{14}}}}}}});

  // LetImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::Let},
            At{loc3,
               binary::LetImmediate{binary::BlockType{15},
                                    At{loc4, binary::LocalsList{binary::Locals{
                                                 2, At{loc6, BVT_I32}}}}}}}},
     At{loc1,
        text::Instruction{
            At{loc2, Opcode::Let},
            At{loc3,
               text::LetImmediate{
                   text::BlockImmediate{
                       nullopt,
                       text::FunctionTypeUse{text::Var{Index{15}}, {}}},
                   {At{loc4,
                       text::BoundValueTypeList{
                           text::BoundValueType{nullopt, At{loc6, tt::VT_I32}},
                           text::BoundValueType{nullopt, tt::VT_I32},
                       }}}}}}});

  // MemArgImmediate.
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::I32Load},
            At{loc3,
               binary::MemArgImmediate{At{loc4, u32{2}}, At{loc5, u32{13}}}}}},
     At{loc1,
        text::Instruction{At{loc2, Opcode::I32Load},
                          At{loc3, text::MemArgImmediate{At{loc4, u32{4}},
                                                         At{loc5, u32{13}}}}}});

  // HeapType.
  OK(At{loc1,
        binary::Instruction{At{loc2, Opcode::RefNull}, At{loc3, BHT_Func}}},
     At{loc1,
        text::Instruction{At{loc2, Opcode::RefNull}, At{loc3, tt::HT_Func}}});

  // SelectImmediate.
  OK(At{loc1, binary::Instruction{At{loc2, Opcode::SelectT},
                                  At{loc3, binary::SelectImmediate{At{
                                               loc4, BVT_I32}}}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::SelectT},
                  At{loc3, text::SelectImmediate{At{loc4, tt::VT_I32}}}}});

  // ShuffleImmediate.
  OK(At{loc1, binary::Instruction{At{loc2, Opcode::I8X16Shuffle},
                                  At{loc3, ShuffleImmediate{}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::I8X16Shuffle},
                                At{loc3, ShuffleImmediate{}}}});

  // SimdLaneImmediate.
  OK(At{loc1, binary::Instruction{At{loc2, Opcode::I8X16ExtractLaneS},
                                  At{loc3, binary::SimdLaneImmediate{13}}}},
     At{loc1, text::Instruction{At{loc2, Opcode::I8X16ExtractLaneS},
                                At{loc3, text::SimdLaneImmediate{13}}}});

#if 0
  // BrOnCastImmediate
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::BrOnCast},
            At{loc3,
               binary::BrOnCastImmediate{
                   At{loc4, Index{13}},
                   At{loc5, binary::HeapType2Immediate{At{loc6, BHT_Func},
                                                       At{loc7, BHT_Func}}}}}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::BrOnCast},
                  At{loc3, text::BrOnCastImmediate{
                               At{loc4, text::Var{Index{13}}},
                               At{loc5, text::HeapType2Immediate{
                                            At{loc6, tt::HT_Func},
                                            At{loc7, tt::HT_Func}}}}}}});
#endif

  // HeapType2Immediate
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::RefTest},
            At{loc3, binary::HeapType2Immediate{At{loc4, BHT_Func},
                                                At{loc5, BHT_Func}}}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::RefTest},
                  At{loc3, text::HeapType2Immediate{At{loc4, tt::HT_Func},
                                                    At{loc5, tt::HT_Func}}}}});

#if 0
  // RttSubImmediate
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::RttSub},
            At{loc3,
               binary::RttSubImmediate{
                   At{loc4, Index{13}},
                   At{loc5, binary::HeapType2Immediate{At{loc6, BHT_Func},
                                                       At{loc7, BHT_Func}}}}}}},
     At{loc1, text::Instruction{
                  At{loc2, Opcode::RttSub},
                  At{loc3, text::RttSubImmediate{
                               At{loc4, Index{13}},
                               At{loc5, text::HeapType2Immediate{
                                            At{loc6, tt::HT_Func},
                                            At{loc7, tt::HT_Func}}}}}}});
#endif

  // StructFieldImmediate
  OK(At{loc1,
        binary::Instruction{
            At{loc2, Opcode::StructGet},
            At{loc3, binary::StructFieldImmediate{At{loc4, Index{13}},
                                                  At{loc5, Index{14}}}}}},
     At{loc1,
        text::Instruction{At{loc2, Opcode::StructGet},
                          At{loc3, text::StructFieldImmediate{
                                       At{loc4, text::Var{Index{13}}},
                                       At{loc5, text::Var{Index{14}}}}}}});
}

TEST_F(ConvertToBinaryTest, OpcodeAlignment) {
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
      {Opcode::V128Load8Splat, 0},

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
      {Opcode::V128Load16Splat, 1},

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
      {Opcode::V128Load32Splat, 2},
      {Opcode::V128Load32Zero, 2},

      {Opcode::F64Load, 3},
      {Opcode::F64Store, 3},
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
      {Opcode::MemoryAtomicWait64, 3},
      {Opcode::V128Load16X4S, 3},
      {Opcode::V128Load16X4U, 3},
      {Opcode::V128Load32X2S, 3},
      {Opcode::V128Load32X2U, 3},
      {Opcode::V128Load64Splat, 3},
      {Opcode::V128Load64Zero, 3},
      {Opcode::V128Load8X8S, 3},
      {Opcode::V128Load8X8U, 3},

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

TEST_F(ConvertToBinaryTest, InstructionList) {
  OK(binary::InstructionList{At{loc1,
                                binary::Instruction{At{loc2, Opcode::Nop}}},
                             At{loc3,
                                binary::Instruction{At{loc4, Opcode::Nop}}}},
     text::InstructionList{At{loc1, text::Instruction{At{loc2, Opcode::Nop}}},
                           At{loc3, text::Instruction{At{loc4, Opcode::Nop}}}});
}

TEST_F(ConvertToBinaryTest, Expression) {
  OKFunc(ToBinaryUnpackedExpression,
         At{loc1,
            binary::UnpackedExpression{At{
                loc1,
                binary::InstructionList{
                    At{loc2, binary::Instruction{At{loc3, Opcode::I32Const},
                                                 At{loc4, s32{0}}}},
                    At{loc5, binary::Instruction{At{loc6, Opcode::Drop}}},
                    At{loc7, binary::Instruction{At{loc8, Opcode::End}}},
                }}}},
         At{loc1, text::InstructionList{
                      At{loc2, text::Instruction{At{loc3, Opcode::I32Const},
                                                 At{loc4, s32{0}}}},
                      At{loc5, text::Instruction{At{loc6, Opcode::Drop}}},
                      At{loc7, text::Instruction{At{loc8, Opcode::End}}},
                  }});
}

TEST_F(ConvertToBinaryTest, LocalsList) {
  OKFunc(ToBinaryLocalsList,
         At{loc1, binary::LocalsList{binary::Locals{2, At{loc2, BVT_I32}},
                                     binary::Locals{1, At{loc4, BVT_F32}}}},
         At{loc1, text::BoundValueTypeList{
                      text::BoundValueType{nullopt, At{loc2, tt::VT_I32}},
                      text::BoundValueType{nullopt, At{loc3, tt::VT_I32}},
                      text::BoundValueType{nullopt, At{loc4, tt::VT_F32}},
                  }});
}

TEST_F(ConvertToBinaryTest, Code) {
  OKFunc(
      ToBinaryCode,
      At{loc1,
         binary::UnpackedCode{
             At{loc2, binary::LocalsList{binary::Locals{1, At{loc3, BVT_I32}}}},
             At{loc4,
                binary::UnpackedExpression{
                    At{loc4,
                       binary::InstructionList{
                           At{loc5, binary::Instruction{At{loc6, Opcode::Nop}}},
                           At{loc6, binary::Instruction{At{loc8, Opcode::End}}},
                       }}}}}},
      At{loc1,
         text::Function{
             {},
             At{loc2, text::BoundValueTypeList{text::BoundValueType{
                          nullopt, At{loc3, tt::VT_I32}}}},
             At{loc4,
                text::InstructionList{
                    At{loc5, text::Instruction{At{loc6, Opcode::Nop}}},
                    At{loc6, text::Instruction{At{loc8, Opcode::End}}},
                }},
             {}}});
}

TEST_F(ConvertToBinaryTest, DataSegment) {
  // Active.
  OK(At{loc1,
        binary::DataSegment{
            At{loc2, Index{13}},
            At{loc3, binary::ConstantExpression{At{
                         loc4, binary::Instruction{At{loc5, Opcode::Nop}}}}},
            "hello\x00"_su8}},
     At{loc1,
        text::DataSegment{
            nullopt, At{loc2, text::Var{Index{13}}},
            At{loc3, text::ConstantExpression{At{
                         loc4, text::Instruction{At{loc5, Opcode::Nop}}}}},
            text::DataItemList{text::DataItem{text::Text{"\"hello\"", 5}},
                               text::DataItem{text::Text{"\"\\00\"", 1}}}}});
}

TEST_F(ConvertToBinaryTest, DataSegment_numeric_values) {
  OK(At{loc1,
        binary::DataSegment{
            At{loc2, Index{13}},
            At{loc3, binary::ConstantExpression{At{
                         loc4, binary::Instruction{At{loc5, Opcode::Nop}}}}},
            "hello\x00"_su8}},
     At{loc1,
        text::DataSegment{
            nullopt, At{loc2, text::Var{Index{13}}},
            At{loc3, text::ConstantExpression{At{
                         loc4, text::Instruction{At{loc5, Opcode::Nop}}}}},
            text::DataItemList{text::DataItem{text::NumericData{
                text::NumericDataType::I8,
                ToBuffer("\x68\x65\x6c\x6c\x6f\x00"_su8)}}}}});
}

TEST_F(ConvertToBinaryTest, EventType) {
  OK(At{loc1,
        binary::EventType{
            EventAttribute::Exception,
            At{loc2, Index{0}},
        }},
     At{loc1, text::EventType{
                  EventAttribute::Exception,
                  text::FunctionTypeUse{
                      At{loc2, text::Var{Index{0}}},
                      {},
                  },
              }});
}

TEST_F(ConvertToBinaryTest, Event) {
  OK(At{loc1, binary::Event{At{loc2,
                               binary::EventType{
                                   EventAttribute::Exception,
                                   At{loc3, Index{0}},
                               }}}},
     At{loc1,
        text::Event{text::EventDesc{
                        nullopt, At{loc2,
                                    text::EventType{
                                        EventAttribute::Exception,
                                        text::FunctionTypeUse{
                                            At{loc3, text::Var{Index{0}}},
                                            {},
                                        },
                                    }}},
                    {}}});
}

TEST_F(ConvertToBinaryTest, Module) {
  // Additional locations only needed for Module. :-)
  const SpanU8 loc9 = "I"_su8;
  const SpanU8 loc10 = "J"_su8;
  const SpanU8 loc11 = "K"_su8;
  const SpanU8 loc12 = "L"_su8;
  const SpanU8 loc13 = "M"_su8;
  const SpanU8 loc14 = "N"_su8;
  const SpanU8 loc15 = "O"_su8;
  const SpanU8 loc16 = "P"_su8;
  const SpanU8 loc17 = "Q"_su8;
  const SpanU8 loc18 = "R"_su8;
  const SpanU8 loc19 = "S"_su8;
  const SpanU8 loc20 = "T"_su8;
  const SpanU8 loc21 = "U"_su8;
  const SpanU8 loc22 = "V"_su8;
  const SpanU8 loc23 = "W"_su8;
  const SpanU8 loc24 = "X"_su8;
  const SpanU8 loc25 = "Y"_su8;
  const SpanU8 loc26 = "Z"_su8;
  const SpanU8 loc27 = "AA"_su8;
  const SpanU8 loc28 = "BB"_su8;

  auto binary_table_type =
      At{"T0"_su8, binary::TableType{At{"T1"_su8, Limits{At{"T2"_su8, u32{0}}}},
                                     At{"T3"_su8, BRT_Funcref}}};

  auto text_table_type =
      At{"T0"_su8, text::TableType{At{"T1"_su8, Limits{At{"T2"_su8, u32{0}}}},
                                   At{"T3"_su8, tt::RT_Funcref}}};

  auto memory_type =
      At{"M0"_su8, MemoryType{At{"M1"_su8, Limits{At{"M2"_su8, u32{0}}}}}};

  auto binary_global_type =
      At{"G0"_su8, binary::GlobalType{At{"G1"_su8, BVT_I32},
                                      At{"G2"_su8, Mutability::Const}}};

  auto text_global_type =
      At{"G0"_su8, text::GlobalType{At{"G1"_su8, tt::VT_I32},
                                    At{"G2"_su8, Mutability::Const}}};

  auto external_kind = At{"EK"_su8, ExternalKind::Function};

  // These will be shared between global, data, and element segments. This
  // would never actually happen, but it simplifies the test below.
  auto binary_constant_expression =
      At{"CE0"_su8,
         binary::ConstantExpression{
             At{"CE1"_su8, binary::Instruction{At{"CE2"_su8, Opcode::I32Const},
                                               At{"CE3"_su8, s32{0}}}}}};

  auto text_constant_expression =
      At{"CE0"_su8,
         text::ConstantExpression{
             At{"CE1"_su8, text::Instruction{At{"CE2"_su8, Opcode::I32Const},
                                             At{"CE3"_su8, s32{0}}}}}};

  OK(At{loc1,
        binary::Module{
            // types
            {At{loc2, binary::DefinedType{binary::FunctionType{}}}},
            // imports
            {At{loc3, binary::Import{At{loc4, "m"_sv}, At{loc5, "n"_sv},
                                     At{loc6, Index{0}}}}},
            // functions
            {At{loc7, binary::Function{At{loc8, Index{0}}}}},
            // tables
            {At{loc9, binary::Table{binary_table_type}}},
            // memories
            {At{loc10, binary::Memory{memory_type}}},
            // globals
            {At{loc11, binary::Global{binary_global_type,
                                      binary_constant_expression}}},
            // events
            {At{loc12, binary::Event{At{
                           loc13, binary::EventType{EventAttribute::Exception,
                                                    At{loc14, Index{0}}}}}}},
            // exports
            {At{loc15, binary::Export{external_kind, At{loc16, "e"_sv},
                                      At{loc17, Index{0}}}}},
            // starts
            {At{loc18, binary::Start{At{loc19, Index{0}}}}},
            // element_segments
            {At{loc20,
                binary::ElementSegment{
                    At{loc21, Index{0}}, binary_constant_expression,
                    binary::ElementList{binary::ElementListWithIndexes{
                        external_kind, {At{loc22, Index{0}}}}}}}},
            // data_count omitted because bulk memory is not enabled.
            nullopt,
            // codes
            {At{loc7,
                binary::UnpackedCode{
                    binary::LocalsList{},
                    binary::UnpackedExpression{binary::InstructionList{
                        At{loc25, binary::Instruction{At{loc26, Opcode::Nop}}},
                        At{loc27, binary::Instruction{At{loc28, Opcode::End}}},
                    }}}}},
            // data_segments
            {At{loc23,
                binary::DataSegment{At{loc24, Index{0}},
                                    binary_constant_expression, "hello"_su8}}},
        }},
     At{loc1,
        text::Module{
            // (type (func))
            text::ModuleItem{At{
                loc2, text::DefinedType{nullopt, text::BoundFunctionType{}}}},
            // (import "m" "n" (func (type 0)))
            text::ModuleItem{At{
                loc3,
                text::Import{At{loc4, text::Text{"\"m\"", 1}},
                             At{loc5, text::Text{"\"n\"", 1}},
                             text::FunctionDesc{
                                 nullopt, At{loc6, text::Var{Index{0}}}, {}}}}},
            // (event)
            text::ModuleItem{
                At{loc12,
                   text::Event{
                       text::EventDesc{
                           nullopt,
                           At{loc13,
                              text::EventType{
                                  EventAttribute::Exception,
                                  text::FunctionTypeUse{
                                      At{loc14, text::Var{Index{0}}}, {}}}}},
                       {}}}},
            // (global i32 i32.const 0)
            text::ModuleItem{At{
                loc11, text::Global{text::GlobalDesc{nullopt, text_global_type},
                                    text_constant_expression,
                                    {}}}},
            // (memory 0)
            text::ModuleItem{
                At{loc10,
                   text::Memory{text::MemoryDesc{nullopt, memory_type}, {}}}},
            // (table 0 funcref)
            text::ModuleItem{
                At{loc9,
                   text::Table{text::TableDesc{nullopt, text_table_type}, {}}}},
            // (start 0)
            text::ModuleItem{
                At{loc18, text::Start{At{loc19, text::Var{Index{0}}}}}},
            // (func (type 0) nop)
            text::ModuleItem{
                At{loc7,
                   text::Function{
                       text::FunctionDesc{
                           nullopt, At{loc8, text::Var{Index{0}}}, {}},
                       {},
                       {At{loc25, text::Instruction{At{loc26, Opcode::Nop}}},
                        At{loc27, text::Instruction{At{loc28, Opcode::End}}}},
                       {}}}},
            // (elem (i32.const 0) func 0)
            text::ModuleItem{
                At{loc20,
                   text::ElementSegment{
                       nullopt, At{loc21, text::Var{Index{0}}},
                       text_constant_expression,
                       text::ElementList{text::ElementListWithVars{
                           external_kind, {At{loc22, text::Var{Index{0}}}}}}}}},
            // (export "e" (func 0))
            text::ModuleItem{
                At{loc15, text::Export{external_kind,
                                       At{loc16, text::Text{"\"e\""_sv, 1}},
                                       At{loc17, text::Var{Index{0}}}}}},
            // (data (i32.const 0) "hello")
            text::ModuleItem{
                At{loc23,
                   text::DataSegment{nullopt, At{loc24, text::Var{Index{0}}},
                                     text_constant_expression,
                                     text::DataItemList{text::DataItem{
                                         text::Text{"\"hello\""_sv, 5}}}}}},
        }});
}

TEST_F(ConvertToBinaryTest, DataCount_BulkMemory) {
  context.features.enable_bulk_memory();

  OK(At{loc1,
        binary::Module{
            // types
            {},
            // imports
            {},
            // functions
            {},
            // tables
            {},
            // memories
            {},
            // globals
            {},
            // events
            {},
            // exports
            {},
            // starts
            {},
            // element_segments
            {},
            // data_count
            {binary::DataCount{Index{1}}},
            // codes
            {},
            // data_segments
            {At{loc2,
                binary::DataSegment{
                    At{loc3, Index{0}},
                    At{loc4, binary::ConstantExpression{At{
                                 loc5, binary::Instruction{At{loc6,
                                                              Opcode::I32Const},
                                                           At{loc7, s32{0}}}}}},
                    "hello"_su8}}},
        }},
     At{loc1,
        text::Module{
            // (data (i32.const 0) "hello")
            text::ModuleItem{At{
                loc2,
                text::DataSegment{
                    nullopt, At{loc3, text::Var{Index{0}}},
                    At{loc4,
                       text::ConstantExpression{At{
                           loc5, text::Instruction{At{loc6, Opcode::I32Const},
                                                   At{loc7, s32{0}}}}}},
                    text::DataItemList{
                        text::DataItem{text::Text{"\"hello\""_sv, 5}}}}}},
        }});
}
