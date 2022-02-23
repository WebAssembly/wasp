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

#include "wasp/text/types.h"

#include "gtest/gtest.h"

#include "test/text/constants.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;

TEST(TextTypesTest, FunctionToImport) {
  auto module = At{"\"m\""_su8, Text{"\"m\"", 1}};
  auto name = At{"\"n\""_su8, Text{"\"n\"", 1}};
  auto desc = FunctionDesc{
      nullopt,
      At{"(type 0)"_su8, Var{Index{0}}},
      At{"(param $a i32) (result f32)"_su8,
         BoundFunctionType{
             {At{"$a i32"_su8, BoundValueType{"$a"_sv, At{"i32"_su8, VT_I32}}}},
             {At{"f32"_su8, VT_F32}}}},
  };

  EXPECT_EQ(
      (At{"(import \"m\" \"n\")"_su8, Import{module, name, desc}}),
      (Function{desc,
                At{"(import \"m\" \"n\")"_su8, InlineImport{module, name}},
                {}})
          .ToImport());
}

TEST(TextTypesTest, FunctionToExports) {
  auto name1 = At{"\"e1\""_su8, Text{"\"e1\"", 1}};
  auto name2 = At{"\"e2\""_su8, Text{"\"e2\"", 1}};
  auto desc = FunctionDesc{nullopt, At{"(type 0)"_su8, Var{Index{0}}}, {}};
  Index this_index = 13;

  EXPECT_EQ((ExportList{
                At{"(export \"e1\")"_su8,
                   Export{ExternalKind::Function, name1, Var{this_index}}},
                At{"(export \"e2\")"_su8,
                   Export{ExternalKind::Function, name2, Var{this_index}}},
            }),
            (Function{desc,
                      {},
                      {},
                      InlineExportList{
                          At{"(export \"e1\")"_su8, InlineExport{name1}},
                          At{"(export \"e2\")"_su8, InlineExport{name2}},
                      }})
                .ToExports(this_index));
}

TEST(TextTypesTest, TableToImport) {
  auto module = At{"\"m\""_su8, Text{"\"m\"", 1}};
  auto name = At{"\"n\""_su8, Text{"\"n\"", 1}};
  auto desc = TableDesc{
      nullopt,
      At{"1 funcref"_su8, TableType{At{"1"_su8, Limits{At{"1"_su8, u32{1}}}},
                                    At{"funcref"_su8, RT_Funcref}}}};

  EXPECT_EQ((At{"(import \"m\" \"n\")"_su8, Import{module, name, desc}}),
            (Table{desc,
                   At{"(import \"m\" \"n\")"_su8, InlineImport{module, name}},
                   {}})
                .ToImport());
}

TEST(TextTypesTest, TableToExports) {
  auto name1 = At{"\"e1\""_su8, Text{"\"e1\"", 1}};
  auto name2 = At{"\"e2\""_su8, Text{"\"e2\"", 1}};
  auto desc = TableDesc{
      nullopt,
      At{"1 funcref"_su8, TableType{At{"1"_su8, Limits{At{"1"_su8, u32{1}}}},
                                    At{"funcref"_su8, RT_Funcref}}}};
  Index this_index = 13;

  EXPECT_EQ((ExportList{
                At{"(export \"e1\")"_su8,
                   Export{ExternalKind::Table, name1, Var{this_index}}},
                At{"(export \"e2\")"_su8,
                   Export{ExternalKind::Table, name2, Var{this_index}}},
            }),
            (Table{desc,
                   InlineExportList{
                       At{"(export \"e1\")"_su8, InlineExport{name1}},
                       At{"(export \"e2\")"_su8, InlineExport{name2}},
                   }})
                .ToExports(this_index));
}

TEST(TextTypesTest, TableToElementSegment) {
  auto elements = ElementList{ElementListWithVars{
      At{"func"_su8, ExternalKind::Function}, VarList{
                                                  At{"0"_su8, Var{Index{0}}},
                                                  At{"$a"_su8, Var{"$a"_sv}},
                                              }}};
  auto desc = TableDesc{
      nullopt, At{"funcref"_su8,
                  TableType{Limits{u32{2}}, At{"funcref"_su8, RT_Funcref}}}};
  Index this_index = 13;

  EXPECT_EQ((ElementSegment{nullopt, Var{this_index},
                            ConstantExpression{
                                Instruction{At{Opcode::I32Const}, At{s32{0}}}},
                            elements}),
            (Table{desc, {}, elements}).ToElementSegment(this_index));
}

TEST(TextTypesTest, MemoryToImport) {
  auto module = At{"\"m\""_su8, Text{"\"m\"", 1}};
  auto name = At{"\"n\""_su8, Text{"\"n\"", 1}};
  auto desc = MemoryDesc{
      nullopt,
      At{"1"_su8, MemoryType{At{"1"_su8, Limits{At{"1"_su8, u32{1}}}}}}};

  EXPECT_EQ((At{"(import \"m\" \"n\")"_su8, Import{module, name, desc}}),
            (Memory{desc,
                    At{"(import \"m\" \"n\")"_su8, InlineImport{module, name}},
                    {}})
                .ToImport());
}

TEST(TextTypesTest, MemoryToExports) {
  auto name1 = At{"\"e1\""_su8, Text{"\"e1\"", 1}};
  auto name2 = At{"\"e2\""_su8, Text{"\"e2\"", 1}};
  auto desc = MemoryDesc{
      nullopt,
      At{"1"_su8, MemoryType{At{"1"_su8, Limits{At{"1"_su8, u32{1}}}}}}};
  Index this_index = 13;

  EXPECT_EQ((ExportList{
                At{"(export \"e1\")"_su8,
                   Export{ExternalKind::Memory, name1, Var{this_index}}},
                At{"(export \"e2\")"_su8,
                   Export{ExternalKind::Memory, name2, Var{this_index}}},
            }),
            (Memory{desc,
                    InlineExportList{
                        At{"(export \"e1\")"_su8, InlineExport{name1}},
                        At{"(export \"e2\")"_su8, InlineExport{name2}},
                    }})
                .ToExports(this_index));
}

TEST(TextTypesTest, MemoryToDataSegment) {
  auto data = DataItemList{
      DataItem{Text{"\"hello\"", 5}},
      DataItem{Text{"\"world\"", 5}},
  };
  auto desc = MemoryDesc{nullopt, MemoryType{Limits{u32{1}}}};
  Index this_index = 13;

  EXPECT_EQ((DataSegment{nullopt, Var{this_index},
                         ConstantExpression{
                             Instruction{At{Opcode::I32Const}, At{s32{0}}}},
                         data}),
            (Memory{desc, {}, data}).ToDataSegment(this_index));
}

TEST(TextTypesTest, GlobalToImport) {
  auto module = At{"\"m\""_su8, Text{"\"m\"", 1}};
  auto name = At{"\"n\""_su8, Text{"\"n\"", 1}};
  auto desc = GlobalDesc{
      nullopt,
      At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32}, Mutability::Const}}};

  EXPECT_EQ((At{"(import \"m\" \"n\")"_su8, Import{module, name, desc}}),
            (Global{desc,
                    At{"(import \"m\" \"n\")"_su8, InlineImport{module, name}},
                    {}})
                .ToImport());
}

TEST(TextTypesTest, GlobalToExports) {
  auto name1 = At{"\"e1\""_su8, Text{"\"e1\"", 1}};
  auto name2 = At{"\"e2\""_su8, Text{"\"e2\"", 1}};
  auto desc = GlobalDesc{
      nullopt,
      At{"i32"_su8, GlobalType{At{"i32"_su8, VT_I32}, Mutability::Const}}};
  Index this_index = 13;

  EXPECT_EQ((ExportList{
                At{"(export \"e1\")"_su8,
                   Export{ExternalKind::Global, name1, Var{this_index}}},
                At{"(export \"e2\")"_su8,
                   Export{ExternalKind::Global, name2, Var{this_index}}},
            }),
            (Global{desc, ConstantExpression{},
                    InlineExportList{
                        At{"(export \"e1\")"_su8, InlineExport{name1}},
                        At{"(export \"e2\")"_su8, InlineExport{name2}},
                    }})
                .ToExports(this_index));
}

TEST(TextTypesTest, TagToImport) {
  auto module = At{"\"m\""_su8, Text{"\"m\"", 1}};
  auto name = At{"\"n\""_su8, Text{"\"n\"", 1}};
  auto desc =
      TagDesc{nullopt,
              At{"(type 0)"_su8,
                 TagType{
                     TagAttribute::Exception,
                     At{"(type 0)"_su8,
                        FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
                 }}};

  EXPECT_EQ((At{"(import \"m\" \"n\")"_su8, Import{module, name, desc}}),
            (Tag{desc,
                 At{"(import \"m\" \"n\")"_su8, InlineImport{module, name}},
                 {}})
                .ToImport());
}

TEST(TextTypesTest, TagToExports) {
  auto name1 = At{"\"e1\""_su8, Text{"\"e1\"", 1}};
  auto name2 = At{"\"e2\""_su8, Text{"\"e2\"", 1}};
  auto desc =
      TagDesc{nullopt,
              At{"(type 0)"_su8,
                 TagType{
                     TagAttribute::Exception,
                     At{"(type 0)"_su8,
                        FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
                 }}};
  Index this_index = 13;

  EXPECT_EQ((ExportList{
                At{"(export \"e1\")"_su8,
                   Export{ExternalKind::Tag, name1, Var{this_index}}},
                At{"(export \"e2\")"_su8,
                   Export{ExternalKind::Tag, name2, Var{this_index}}},
            }),
            (Tag{desc,
                 InlineExportList{
                     At{"(export \"e1\")"_su8, InlineExport{name1}},
                     At{"(export \"e2\")"_su8, InlineExport{name2}},
                 }})
                .ToExports(this_index));
}

TEST(TextTypesTest, NumericData) {
  Buffer buffer = ToBuffer(
      "\x00\x01\x02\x03\x04\x05\x06\x07"
      "\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
      "\x10\x11\x12\x13\x14\x15\x16\x17"
      "\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
      "\x20\x21\x22\x23\x24\x25\x26\x27"
      "\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
      "\x00\x00\x80\x3f\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\xf0\x3f"_su8);

  struct {
    NumericData data;
    u32 data_type_size;
    Index count;
    u32 byte_size;
  } tests[] = {
      {{NumericDataType::I8, buffer}, 1, 64, 64},
      {{NumericDataType::I16, buffer}, 2, 32, 64},
      {{NumericDataType::I32, buffer}, 4, 16, 64},
      {{NumericDataType::I64, buffer}, 8, 8, 64},
      {{NumericDataType::F32, buffer}, 4, 16, 64},
      {{NumericDataType::F64, buffer}, 8, 8, 64},
      {{NumericDataType::V128, buffer}, 16, 4, 64},
  };

  for (auto&& test : tests) {
    EXPECT_EQ(test.data_type_size, test.data.data_type_size());
    EXPECT_EQ(test.count, test.data.count());
    EXPECT_EQ(test.byte_size, test.data.byte_size());
  }

  EXPECT_EQ(0x02u, tests[0].data.value<u8>(2));                   // u8
  EXPECT_EQ(0x0504u, tests[1].data.value<u16>(2));                // u16
  EXPECT_EQ(0x0b0a0908u, tests[2].data.value<u32>(2));            // u32
  EXPECT_EQ(0x1716151413121110ull, tests[3].data.value<u64>(2));  // u64
  EXPECT_EQ(1.0f, tests[4].data.value<f32>(12));                  // f32
  EXPECT_EQ(1.0, tests[5].data.value<f64>(7));                    // f64
  EXPECT_EQ((v128{u8x16{{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
                         0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f}}}),
            tests[6].data.value<v128>(2));  // f64
}
