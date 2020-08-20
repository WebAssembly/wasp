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
  auto data = TextList{
      Text{"\"hello\"", 5},
      Text{"\"world\"", 5},
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

TEST(TextTypesTest, EventToImport) {
  auto module = At{"\"m\""_su8, Text{"\"m\"", 1}};
  auto name = At{"\"n\""_su8, Text{"\"n\"", 1}};
  auto desc = EventDesc{
      nullopt,
      At{"(type 0)"_su8,
         EventType{
             EventAttribute::Exception,
             At{"(type 0)"_su8,
                FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
         }}};

  EXPECT_EQ((At{"(import \"m\" \"n\")"_su8, Import{module, name, desc}}),
            (Event{desc,
                   At{"(import \"m\" \"n\")"_su8, InlineImport{module, name}},
                   {}})
                .ToImport());
}

TEST(TextTypesTest, EventToExports) {
  auto name1 = At{"\"e1\""_su8, Text{"\"e1\"", 1}};
  auto name2 = At{"\"e2\""_su8, Text{"\"e2\"", 1}};
  auto desc = EventDesc{
      nullopt,
      At{"(type 0)"_su8,
         EventType{
             EventAttribute::Exception,
             At{"(type 0)"_su8,
                FunctionTypeUse{At{"(type 0)"_su8, Var{Index{0}}}, {}}},
         }}};
  Index this_index = 13;

  EXPECT_EQ((ExportList{
                At{"(export \"e1\")"_su8,
                   Export{ExternalKind::Event, name1, Var{this_index}}},
                At{"(export \"e2\")"_su8,
                   Export{ExternalKind::Event, name2, Var{this_index}}},
            }),
            (Event{desc,
                   InlineExportList{
                       At{"(export \"e1\")"_su8, InlineExport{name1}},
                       At{"(export \"e2\")"_su8, InlineExport{name2}},
                   }})
                .ToExports(this_index));
}
