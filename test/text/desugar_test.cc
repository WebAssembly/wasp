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

#include "wasp/text/desugar.h"

#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "test/text/constants.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;
using namespace ::wasp::test;

class TextDesugarTest : public ::testing::Test {
 protected:
  // Convenient pre-defined values to make the tests below easier to read.
  const Location loc1 = "A"_su8;
  const Location import_loc = "I"_su8;
  const Location export1_loc = "E1"_su8;
  const Location export2_loc = "E2"_su8;

  const At<Text> name1 = At{"T1"_su8, Text{"\"m\""_sv, 1}};
  const At<Text> name2 = At{"T2"_su8, Text{"\"n\""_sv, 1}};
  const At<Text> name3 = At{"T3"_su8, Text{"\"o\""_sv, 1}};
  const At<Text> name4 = At{"T4"_su8, Text{"\"p\""_sv, 1}};

  const At<InlineImport> import = At{import_loc, InlineImport{name1, name2}};
  const At<InlineExport> export1 = At{export1_loc, InlineExport{name3}};
  const At<InlineExport> export2 = At{export2_loc, InlineExport{name4}};

  const FunctionDesc func_desc{};
  const TableDesc table_desc{nullopt, TableType{Limits{0}, RT_Funcref}};
  const MemoryDesc memory_desc{nullopt, MemoryType{Limits{0}}};
  const GlobalDesc global_desc{nullopt, GlobalType{VT_I32, Mutability::Const}};
  const EventDesc event_desc{nullopt, EventType{EventAttribute::Exception, {}}};

  const ConstantExpression constant_expression{
      Instruction{Opcode::I32Const, s32{0}}};

  const ElementList element_list{
      ElementListWithVars{ExternalKind::Function, {Var{Index{0}}}}};

  void OK(const Module& after, const Module& before) {
    Module copy = before;
    Desugar(copy);
    EXPECT_EQ(after, copy);
  }
};

TEST_F(TextDesugarTest, Function_Defined) {
  OK(
      Module{
          ModuleItem{At{loc1, Function{}}},
      },
      Module{
          ModuleItem{At{loc1, Function{}}},
      });
}

TEST_F(TextDesugarTest, Function_DefinedExport) {
  OK(
      Module{
          ModuleItem{At{loc1, Function{}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Function, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Function, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{
              At{loc1, Function{FunctionDesc{}, {}, {}, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Function_Import) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, func_desc}}},
      },
      Module{ModuleItem{At{loc1, Function{func_desc, import, {}}}}});
}

TEST_F(TextDesugarTest, Function_ImportExport) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, func_desc}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Function, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Function, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Function{func_desc, import, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Table_Defined) {
  OK(
      Module{
          ModuleItem{At{loc1, Table{table_desc, {}}}},
      },
      Module{
          ModuleItem{At{loc1, Table{table_desc, {}}}},
      });
}

TEST_F(TextDesugarTest, Table_DefinedExport) {
  OK(
      Module{
          ModuleItem{At{loc1, Table{table_desc, {}}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Table, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Table, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Table{table_desc, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Table_DefinedSegment) {
  OK(
      Module{
          ModuleItem{At{loc1, Table{table_desc, {}}}},
          ModuleItem{ElementSegment{nullopt, Var{Index{0}}, constant_expression,
                                    element_list}},
      },
      Module{
          ModuleItem{At{loc1, Table{table_desc, {}, element_list}}},
      });
}

TEST_F(TextDesugarTest, Table_Import) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, table_desc}}},
      },
      Module{
          ModuleItem{At{loc1, Table{table_desc, import, {}}}},
      });
}

TEST_F(TextDesugarTest, Table_ImportExport) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, table_desc}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Table, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Table, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Table{table_desc, import, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Memory_Defined) {
  OK(
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, {}}}},
      },
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, {}}}},
      });
}

TEST_F(TextDesugarTest, Memory_DefinedExport) {
  OK(
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, {}}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Memory, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Memory, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Memory_DefinedSegment) {
  const DataItemList data_item_list{
      At{"T5"_su8, DataItem{Text{"\"hello\""_sv, 5}}},
  };

  OK(
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, {}}}},
          ModuleItem{DataSegment{nullopt, Var{Index{0}}, constant_expression,
                                 data_item_list}},
      },
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, {}, data_item_list}}},
      });
}

TEST_F(TextDesugarTest, Memory_Import) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, memory_desc}}},
      },
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, import, {}}}},
      });
}

TEST_F(TextDesugarTest, Memory_ImportExport) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, memory_desc}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Memory, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Memory, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Memory{memory_desc, import, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Global_Defined) {
  OK(
      Module{
          ModuleItem{At{loc1, Global{global_desc, constant_expression, {}}}},
      },
      Module{
          ModuleItem{At{loc1, Global{global_desc, constant_expression, {}}}},
      });
}

TEST_F(TextDesugarTest, Global_DefinedExport) {
  OK(
      Module{
          ModuleItem{At{loc1, Global{global_desc, constant_expression, {}}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Global, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Global, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{
              At{loc1,
                 Global{global_desc, constant_expression, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Global_Import) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, global_desc}}},
      },
      Module{
          ModuleItem{At{loc1, Global{global_desc, import, {}}}},
      });
}

TEST_F(TextDesugarTest, Global_ImportExport) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, global_desc}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Global, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Global, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Global{global_desc, import, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Event_Defined) {
  OK(
      Module{
          ModuleItem{At{loc1, Event{event_desc, {}}}},
      },
      Module{
          ModuleItem{At{loc1, Event{event_desc, {}}}},
      });
}

TEST_F(TextDesugarTest, Event_DefinedExport) {
  OK(
      Module{
          ModuleItem{At{loc1, Event{event_desc, {}}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Event, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Event, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Event{event_desc, {export1, export2}}}},
      });
}

TEST_F(TextDesugarTest, Event_Import) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, event_desc}}},
      },
      Module{
          ModuleItem{At{loc1, Event{event_desc, import, {}}}},
      });
}

TEST_F(TextDesugarTest, Event_ImportExport) {
  OK(
      Module{
          ModuleItem{At{import_loc, Import{name1, name2, event_desc}}},
          ModuleItem{At{export1_loc,
                        Export{ExternalKind::Event, name3, Var{Index{0}}}}},
          ModuleItem{At{export2_loc,
                        Export{ExternalKind::Event, name4, Var{Index{0}}}}},
      },
      Module{
          ModuleItem{At{loc1, Event{event_desc, import, {export1, export2}}}},
      });
}
