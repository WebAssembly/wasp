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

#include "wasp/text/formatters.h"

#include "gtest/gtest.h"

#include "test/text/constants.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;

TEST(TextFormattersTest, TokenType) {
  EXPECT_EQ(R"(Binary)", format("{}", TokenType::Binary));
  EXPECT_EQ(R"(  End)", format("{:>5s}", TokenType::End));
}

TEST(TextFormattersTest, Sign) {
  EXPECT_EQ(R"(None)", format("{}", Sign::None));
  EXPECT_EQ(R"(  Plus)", format("{:>6s}", Sign::Plus));
}

TEST(TextFormattersTest, LiteralKind) {
  EXPECT_EQ(R"(Normal)", format("{}", LiteralKind::Normal));
  EXPECT_EQ(R"(  Nan)", format("{:>5s}", LiteralKind::Nan));
}

TEST(TextFormattersTest, Base) {
  EXPECT_EQ(R"(Decimal)", format("{}", Base::Decimal));
  EXPECT_EQ(R"(  Hex)", format("{:>5s}", Base::Hex));
}

TEST(TextFormattersTest, HasUnderscores) {
  EXPECT_EQ(R"(No)", format("{}", HasUnderscores::No));
  EXPECT_EQ(R"(  Yes)", format("{:>5s}", HasUnderscores::Yes));
}

TEST(TextFormattersTest, LiteralInfo) {
  EXPECT_EQ(R"({sign None, kind NanPayload, base Hex, has_underscores No})",
            format("{}", LiteralInfo{Sign::None, LiteralKind::NanPayload,
                                     Base::Hex, HasUnderscores::No}));
  EXPECT_EQ(R"(  {sign Plus, kind Normal, base Decimal, has_underscores Yes})",
            format("{:>61s}", LiteralInfo{Sign::Plus, LiteralKind::Normal,
                                          Base::Decimal, HasUnderscores::Yes}));
}

TEST(TextFormattersTest, Token) {
  EXPECT_EQ(R"({loc "\28", type Lpar, immediate empty})",
            format("{}", Token{"("_su8, TokenType::Lpar}));

  EXPECT_EQ(
      R"({loc "\69\33\32\2e\61\64\64", type BareInstr, immediate opcode_info {opcode i32.add, features none}})",
      format("{}", Token{"i32.add"_su8, TokenType::BareInstr,
                         OpcodeInfo{Opcode::I32Add, Features{0}}}));

  EXPECT_EQ(
      R"({loc "\69\33\32", type NumericType, immediate numeric_type i32})",
      format("{}", Token{"i32"_su8, TokenType::NumericType, NumericType::I32}));

  EXPECT_EQ(
      R"({loc "\66\75\6e\63\72\65\66", type ReferenceKind, immediate reference_kind funcref})",
      format("{}", Token{"funcref"_su8, TokenType::ReferenceKind,
                         ReferenceKind::Funcref}));

  EXPECT_EQ(
      R"({loc "\31\32\33", type Nat, immediate literal_info {sign None, kind Normal, base Decimal, has_underscores No}})",
      format("{}", Token{"123"_su8, TokenType::Nat,
                         LiteralInfo::Nat(HasUnderscores::No)}));
}

TEST(TextFormattersTest, Var) {
  EXPECT_EQ(R"(0)", format("{}", Var{Index{0}}));
  EXPECT_EQ(R"($a)", format("{}", Var{"$a"_sv}));
}

TEST(TextFormattersTest, VarList) {
  EXPECT_EQ(R"([0 1 2 $a])",
            format("{}", VarList{Var{Index{0}}, Var{Index{1}}, Var{Index{2}},
                                 Var{"$a"_sv}}));
}

TEST(TextFormattersTest, ValueTypeList) {
  EXPECT_EQ(R"([i32 f32])", format("{}", ValueTypeList{VT_I32, VT_F32}));
}

TEST(TextFormattersTest, FunctionType) {
  EXPECT_EQ(R"({params [i32], results [f32]})",
            format("{}", FunctionType{
                             ValueTypeList{VT_I32},
                             ValueTypeList{VT_F32},
                         }));
}

TEST(TextFormattersTest, FunctionTypeUse) {
  EXPECT_EQ(R"({type_use none, type {params [], results []}})",
            format("{}", FunctionTypeUse{}));
  EXPECT_EQ(
      R"({type_use $a, type {params [i32], results [f32]}})",
      format("{}", FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                                     ValueTypeList{VT_I32},
                                                     ValueTypeList{VT_F32},
                                                 }}));
}

TEST(TextFormattersTest, BlockImmediate) {
  EXPECT_EQ(
      R"({label none, type {type_use none, type {params [], results []}}})",
      format("{}", BlockImmediate{}));
  EXPECT_EQ(
      R"({label $l, type {type_use $a, type {params [i32], results [f32]}}})",
      format("{}", BlockImmediate{
                       BindVar{"$l"_sv},
                       FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                                         ValueTypeList{VT_I32},
                                                         ValueTypeList{VT_F32},
                                                     }}}));
}

TEST(TextFormattersTest, BrOnExnImmediate) {
  EXPECT_EQ(R"({target $a, event $b})",
            format("{}", BrOnExnImmediate{Var{"$a"_sv}, Var{"$b"_sv}}));
}

TEST(TextFormattersTest, BrTableImmediate) {
  EXPECT_EQ(R"({targets [], default_target $b})",
            format("{}", BrTableImmediate{{}, Var{"$b"_sv}}));
  EXPECT_EQ(R"({targets [0 1 2 $a], default_target $b})",
            format("{}", BrTableImmediate{VarList{Var{Index{0}}, Var{Index{1}},
                                                  Var{Index{2}}, Var{"$a"_sv}},
                                          Var{"$b"_sv}}));
}

TEST(TextFormattersTest, CallIndirectImmediate) {
  EXPECT_EQ(
      R"({table none, type {type_use none, type {params [], results []}}})",
      format("{}", CallIndirectImmediate{}));
  EXPECT_EQ(
      R"({table $t, type {type_use $a, type {params [i32], results [f32]}}})",
      format("{}", CallIndirectImmediate{
                       Var{"$t"_sv},
                       FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                                         ValueTypeList{VT_I32},
                                                         ValueTypeList{VT_F32},
                                                     }}}));
}

TEST(TextFormattersTest, CopyImmediate) {
  EXPECT_EQ(R"({dst none, src none})", format("{}", CopyImmediate{}));
  EXPECT_EQ(R"({dst $a, src $b})",
            format("{}", CopyImmediate{Var{"$a"_sv}, Var{"$b"_sv}}));
}

TEST(TextFormattersTest, InitImmediate) {
  EXPECT_EQ(R"({segment $a, dst none})",
            format("{}", InitImmediate{Var{"$a"_sv}, nullopt}));
  EXPECT_EQ(R"({segment $a, dst $b})",
            format("{}", InitImmediate{Var{"$a"_sv}, Var{"$b"_sv}}));
}

TEST(TextFormattersTest, LetImmediate) {
  EXPECT_EQ(
      R"({block {label none, type {type_use none, type {params [], results []}}}, locals []})",
      format("{}", LetImmediate{}));
}

TEST(TextFormattersTest, MemArgImmediate) {
  EXPECT_EQ(R"({align none, offset none})", format("{}", MemArgImmediate{}));
  EXPECT_EQ(R"({align 4, offset 0})",
            format("{}", MemArgImmediate{u32{4}, u32{0}}));
}

TEST(TextFormattersTest, Instruction) {
  EXPECT_EQ(R"({opcode nop, immediate empty})",
            format("{}", Instruction{Opcode::Nop}));

  EXPECT_EQ(R"({opcode i32.const, immediate s32 0})",
            format("{}", Instruction{Opcode::I32Const, s32{}}));

  EXPECT_EQ(R"({opcode i64.const, immediate s64 0})",
            format("{}", Instruction{Opcode::I64Const, s64{}}));

  EXPECT_EQ(R"({opcode f32.const, immediate f32 0.0})",
            format("{}", Instruction{Opcode::F32Const, f32{}}));

  EXPECT_EQ(R"({opcode f64.const, immediate f64 0.0})",
            format("{}", Instruction{Opcode::F64Const, f64{}}));

  EXPECT_EQ(R"({opcode v128.const, immediate v128 0x0 0x0 0x0 0x0})",
            format("{}", Instruction{Opcode::V128Const, v128{}}));

  EXPECT_EQ(
      R"({opcode block, immediate block {label none, type {type_use none, type {params [], results []}}}})",
      format("{}", Instruction{Opcode::Block, BlockImmediate{}}));

  EXPECT_EQ(
      R"({opcode br_on_exn, immediate br_on_exn {target $a, event $b}})",
      format("{}", Instruction{Opcode::BrOnExn,
                               BrOnExnImmediate{Var{"$a"_sv}, Var{"$b"_sv}}}));

  EXPECT_EQ(
      R"({opcode br_table, immediate br_table {targets [], default_target $b}})",
      format("{}",
             Instruction{Opcode::BrTable, BrTableImmediate{{}, Var{"$b"_sv}}}));

  EXPECT_EQ(R"({opcode table.copy, immediate copy {dst none, src none}})",
            format("{}", Instruction{Opcode::TableCopy, CopyImmediate{}}));

  EXPECT_EQ(R"({opcode table.init, immediate init {segment $a, dst none}})",
            format("{}", Instruction{Opcode::TableInit,
                                     InitImmediate{Var{"$a"_sv}, nullopt}}));

  EXPECT_EQ(
      R"({opcode let, immediate let {block {label none, type {type_use none, type {params [], results []}}}, locals []}})",
      format("{}", Instruction{Opcode::Let, LetImmediate{}}));

  EXPECT_EQ(R"({opcode i32.load, immediate mem_arg {align none, offset none}})",
            format("{}", Instruction{Opcode::I32Load, MemArgImmediate{}}));

  EXPECT_EQ(R"({opcode select, immediate select []})",
            format("{}", Instruction{Opcode::Select, SelectImmediate{}}));

  EXPECT_EQ(
      R"({opcode v8x16.shuffle, immediate shuffle [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]})",
      format("{}", Instruction{Opcode::V8X16Shuffle, ShuffleImmediate{}}));

  EXPECT_EQ(R"({opcode local.get, immediate var 0})",
            format("{}", Instruction{Opcode::LocalGet, Var{Index{0}}}));
}

TEST(TextFormattersTest, InstructionList) {
  EXPECT_EQ(R"([{opcode nop, immediate empty} {opcode drop, immediate empty}])",
            format("{}", InstructionList{Instruction{Opcode::Nop},
                                         Instruction{Opcode::Drop}}));
}

TEST(TextFormattersTest, BoundValueType) {
  EXPECT_EQ(R"({name none, type i32})",
            format("{}", BoundValueType{nullopt, VT_I32}));
}

TEST(TextFormattersTest, BoundValueTypeList) {
  EXPECT_EQ(R"([{name none, type i32} {name $a, type f32}])",
            format("{}", BoundValueTypeList{
                             BoundValueType{nullopt, VT_I32},
                             BoundValueType{"$a"_sv, VT_F32},
                         }));
}

TEST(TextFormattersTest, BoundFunctionType) {
  EXPECT_EQ(R"({params [{name none, type i32}], results [f32]})",
            format("{}", BoundFunctionType{BoundValueTypeList{
                                               BoundValueType{nullopt, VT_I32}},
                                           ValueTypeList{VT_F32}}));
}

TEST(TextFormattersTest, TypeEntry) {
  EXPECT_EQ(R"({name $a, type {params [], results []}})",
            format("{}", TypeEntry{"$a"_sv, BoundFunctionType{}}));
}

TEST(TextFormattersTest, FunctionDesc) {
  EXPECT_EQ(R"({name none, type_use none, type {params [], results []}})",
            format("{}", FunctionDesc{}));
}

TEST(TextFormattersTest, TableDesc) {
  EXPECT_EQ(R"({name none, type {min 1} funcref})",
            format("{}", TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}));
}

TEST(TextFormattersTest, MemoryDesc) {
  EXPECT_EQ(R"({name none, type {min 1}})",
            format("{}", MemoryDesc{nullopt, MemoryType{Limits{1}}}));
}

TEST(TextFormattersTest, GlobalDesc) {
  EXPECT_EQ(R"({name none, type const i32})",
            format("{}", GlobalDesc{nullopt, GlobalType{VT_I32,
                                                        Mutability::Const}}));
}

TEST(TextFormattersTest, EventType) {
  EXPECT_EQ(
      R"({attribute exception, type {type_use none, type {params [], results []}}})",
      format("{}", EventType{EventAttribute::Exception, FunctionTypeUse{}}));
}

TEST(TextFormattersTest, EventDesc) {
  EXPECT_EQ(
      R"({name none, type {attribute exception, type {type_use none, type {params [], results []}}}})",
      format("{}", EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                FunctionTypeUse{}}}));
}

TEST(TextFormattersTest, Import) {
  // Function
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc func {name none, type_use none, type {params [], results []}}})",
      format("{}", Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1}, FunctionDesc{}}));

  // Table
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc table {name none, type {min 1} funcref}})",
      format("{}",
             Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                    TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}}));

  // Memory
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc memory {name none, type {min 1}}})",
      format("{}", Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                          MemoryDesc{nullopt, MemoryType{Limits{1}}}}));

  // Global
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc global {name none, type const i32}})",
      format("{}", Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                          GlobalDesc{nullopt,
                                     GlobalType{VT_I32, Mutability::Const}}}));
  // Event
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc event {name none, type {attribute exception, type {type_use none, type {params [], results []}}}}})",
      format("{}",
             Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                    EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                 FunctionTypeUse{}}}}));
}

TEST(TextFormattersTest, InlineImport) {
  EXPECT_EQ(R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}})",
            format("{}", InlineImport{Text{"$a"_sv, 1}, Text{"$b"_sv, 1}}));
}

TEST(TextFormattersTest, InlineExport) {
  EXPECT_EQ(R"({name {text $a, byte_size 1}})",
            format("{}", InlineExport{Text{"$a"_sv, 1}}));
}

TEST(TextFormattersTest, InlineExportList) {
  EXPECT_EQ(R"([{name {text $a, byte_size 1}} {name {text $b, byte_size 1}}])",
            format("{}", InlineExportList{
                             InlineExport{Text{"$a"_sv, 1}},
                             InlineExport{Text{"$b"_sv, 1}},
                         }));
}

TEST(TextFormattersTest, Function) {
  EXPECT_EQ(
      R"({desc {name none, type_use none, type {params [], results []}}, locals [], instructions [], import none, exports []})",
      format("{}", Function{FunctionDesc{}, {}, {}, {}}));
}

TEST(TextFormattersTest, ElementListWithExpressions) {
  EXPECT_EQ(R"({elemtype funcref, list []})",
            format("{}", ElementListWithExpressions{RT_Funcref, {}}));
}

TEST(TextFormattersTest, ElementListWithVars) {
  EXPECT_EQ(R"({kind func, list []})",
            format("{}", ElementListWithVars{ExternalKind::Function, {}}));
}

TEST(TextFormattersTest, ElementList) {
  EXPECT_EQ(
      R"(expression {elemtype funcref, list []})",
      format("{}", ElementList{ElementListWithExpressions{RT_Funcref, {}}}));

  EXPECT_EQ(R"(var {kind func, list []})",
            format("{}", ElementList{
                             ElementListWithVars{ExternalKind::Function, {}}}));
}

TEST(TextFormattersTest, Table) {
  EXPECT_EQ(
      R"({desc {name none, type {min 1} funcref}, import none, exports [], elements none})",
      format("{}",
             Table{TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}, {}}));
}

TEST(TextFormattersTest, Memory) {
  EXPECT_EQ(
      R"({desc {name none, type {min 1}}, import none, exports [], data none})",
      format("{}", Memory{MemoryDesc{nullopt, MemoryType{Limits{1}}}, {}}));
}

TEST(TextFormattersTest, Global) {
  EXPECT_EQ(
      R"({desc {name none, type const i32}, init {instructions []}, import none, exports []})",
      format("{}",
             Global{GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
                    ConstantExpression{},
                    {}}));
}

TEST(TextFormattersTest, Export) {
  EXPECT_EQ(R"({kind func, name {text $a, byte_size 1}, var 0})",
            format("{}", Export{ExternalKind::Function, Text{"$a"_sv, 1},
                                Var{Index{0}}}));
}

TEST(TextFormattersTest, Start) {
  EXPECT_EQ(R"({var 0})", format("{}", Start{Var{Index{0}}}));
}

TEST(TextFormattersTest, ElementSegment) {
  EXPECT_EQ(
      R"({name none, type passive, table none, offset none, elements var {kind func, list []}})",
      format("{}", ElementSegment{nullopt, SegmentType::Passive, {}}));
}

TEST(TextFormattersTest, DataSegment) {
  EXPECT_EQ(R"({name none, type passive, memory none, offset none, data []})",
            format("{}", DataSegment{nullopt, {}}));
}

TEST(TextFormattersTest, Event) {
  EXPECT_EQ(
      R"({desc {name none, type {attribute exception, type {type_use none, type {params [], results []}}}}, import none, exports []})",
      format("{}", Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                      FunctionTypeUse{}}},
                         {}}));
}

TEST(TextFormattersTest, ModuleItem) {
  // TypeEntry
  EXPECT_EQ(R"(type {name $a, type {params [], results []}})",
            format("{}", ModuleItem{TypeEntry{"$a"_sv, BoundFunctionType{}}}));

  // Import
  EXPECT_EQ(
      R"(import {module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc func {name none, type_use none, type {params [], results []}}})",
      format("{}", ModuleItem{Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                                     FunctionDesc{}}}));

  // Function
  EXPECT_EQ(
      R"(func {desc {name none, type_use none, type {params [], results []}}, locals [], instructions [], import none, exports []})",
      format("{}", ModuleItem{Function{FunctionDesc{}, {}, {}, {}}}));

  // Table
  EXPECT_EQ(
      R"(table {desc {name none, type {min 1} funcref}, import none, exports [], elements none})",
      format("{}",
             ModuleItem{Table{
                 TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}, {}}}));

  // Memory
  EXPECT_EQ(
      R"(memory {desc {name none, type {min 1}}, import none, exports [], data none})",
      format("{}", ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{1}}},
                                     {}}}));

  // Global
  EXPECT_EQ(
      R"(global {desc {name none, type const i32}, init {instructions []}, import none, exports []})",
      format("{}",
             ModuleItem{Global{
                 GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
                 ConstantExpression{},
                 {}}}));

  // Export
  EXPECT_EQ(R"(export {kind func, name {text $a, byte_size 1}, var 0})",
            format("{}", ModuleItem{Export{ExternalKind::Function,
                                           Text{"$a"_sv, 1}, Var{Index{0}}}}));

  // Start
  EXPECT_EQ(R"(start {var 0})", format("{}", ModuleItem{Start{Var{Index{0}}}}));

  // ElementSegment
  EXPECT_EQ(
      R"(elem {name none, type passive, table none, offset none, elements var {kind func, list []}})",
      format("{}",
             ModuleItem{ElementSegment{nullopt, SegmentType::Passive, {}}}));

  // DataSegment
  EXPECT_EQ(
      R"(data {name none, type passive, memory none, offset none, data []})",
      format("{}", ModuleItem{DataSegment{nullopt, {}}}));

  // Event
  EXPECT_EQ(
      R"(event {desc {name none, type {attribute exception, type {type_use none, type {params [], results []}}}}, import none, exports []})",
      format("{}", ModuleItem{Event{
                       EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                    FunctionTypeUse{}}},
                       {}}}));
}

TEST(TextFormattersTest, Module) {
  EXPECT_EQ(
      R"([type {name $a, type {params [], results []}} start {var 0}])",
      format("{}", Module{
                       ModuleItem{TypeEntry{"$a"_sv, BoundFunctionType{}}},
                       ModuleItem{Start{Var{Index{0}}}},
                   }));
}

TEST(TextFormattersTest, ScriptModuleKind) {
  EXPECT_EQ(R"(binary)", format("{}", ScriptModuleKind::Binary));
  EXPECT_EQ(R"(text)", format("{}", ScriptModuleKind::Text));
  EXPECT_EQ(R"(quote)", format("{}", ScriptModuleKind::Quote));
}

TEST(TextFormattersTest, ScriptModule) {
  // Text Module.
  EXPECT_EQ(
      R"({name none, kind text, contents module []})",
      format("{}", ScriptModule{nullopt, ScriptModuleKind::Text, Module{}}));

  // Binary Module.
  EXPECT_EQ(R"({name none, kind binary, contents text_list []})",
            format("{}", ScriptModule{nullopt, ScriptModuleKind::Binary,
                                      TextList{}}));

  // Quote Module.
  EXPECT_EQ(
      R"({name none, kind quote, contents text_list []})",
      format("{}", ScriptModule{nullopt, ScriptModuleKind::Quote, TextList{}}));
}

TEST(TextFormattersTest, RefNullConst) {
  EXPECT_EQ(R"({})", format("{}", RefNullConst{HT_Func}));
}

TEST(TextFormattersTest, RefExternConst) {
  EXPECT_EQ(R"({var 0})", format("{}", RefExternConst{u32{}}));
}

TEST(TextFormattersTest, Const) {
  // u32
  EXPECT_EQ(R"(u32 0)", format("{}", Const{u32{}}));

  // u64
  EXPECT_EQ(R"(u64 0)", format("{}", Const{u64{}}));

  // f32
  EXPECT_EQ(R"(f32 0.0)", format("{}", Const{f32{}}));

  // f64
  EXPECT_EQ(R"(f64 0.0)", format("{}", Const{f64{}}));

  // v128
  EXPECT_EQ(R"(v128 0x0 0x0 0x0 0x0)", format("{}", Const{v128{}}));

  // RefNullConst
  EXPECT_EQ(R"(ref.null {})", format("{}", Const{RefNullConst{HT_Func}}));

  // RefExternConst
  EXPECT_EQ(R"(ref.extern {var 0})",
            format("{}", Const{RefExternConst{u32{}}}));
}

TEST(TextFormattersTest, ConstList) {
  EXPECT_EQ(R"([u32 0 u64 0 f32 0.0 f64 0.0])", format("{}", ConstList{
                                                                 Const{u32{}},
                                                                 Const{u64{}},
                                                                 Const{f32{}},
                                                                 Const{f64{}},
                                                             }));
}

TEST(TextFormattersTest, InvokeAction) {
  EXPECT_EQ(R"({module none, name {text "a", byte_size 1}, consts []})",
            format("{}", InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}));
}

TEST(TextFormattersTest, GetAction) {
  EXPECT_EQ(R"({module none, name {text "a", byte_size 1}})",
            format("{}", GetAction{nullopt, Text{"\"a\""_sv, 1}}));
}

TEST(TextFormattersTest, Action) {
  // InvokeAction.
  EXPECT_EQ(
      R"(invoke {module none, name {text "a", byte_size 1}, consts []})",
      format("{}", Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}));

  // GetAction.
  EXPECT_EQ(R"(get {module none, name {text "a", byte_size 1}})",
            format("{}", Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}}));
}

TEST(TextFormattersTest, AssertionKind) {
  EXPECT_EQ(R"(malformed)", format("{}", AssertionKind::Malformed));
  EXPECT_EQ(R"(invalid)", format("{}", AssertionKind::Invalid));
  EXPECT_EQ(R"(unlinkable)", format("{}", AssertionKind::Unlinkable));
  EXPECT_EQ(R"(action_trap)", format("{}", AssertionKind::ActionTrap));
  EXPECT_EQ(R"(return)", format("{}", AssertionKind::Return));
  EXPECT_EQ(R"(module_trap)", format("{}", AssertionKind::ModuleTrap));
  EXPECT_EQ(R"(exhaustion)", format("{}", AssertionKind::Exhaustion));
}

TEST(TextFormattersTest, ModuleAssertion) {
  EXPECT_EQ(
      R"({module {name none, kind text, contents module []}, message {text "error", byte_size 5}})",
      format("{}", ModuleAssertion{
                       ScriptModule{nullopt, ScriptModuleKind::Text, Module{}},
                       Text{"\"error\""_sv, 5}}));
}

TEST(TextFormattersTest, ActionAssertion) {
  EXPECT_EQ(
      R"({action invoke {module none, name {text "a", byte_size 1}, consts []}, message {text "error", byte_size 5}})",
      format("{}", ActionAssertion{
                       Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}},
                       Text{"\"error\""_sv, 5}}));
}

TEST(TextFormattersTest, NanKind) {
  EXPECT_EQ(R"(arithmetic)", format("{}", NanKind::Arithmetic));
  EXPECT_EQ(R"(canonical)", format("{}", NanKind::Canonical));
}

TEST(TextFormattersTest, F32Result) {
  EXPECT_EQ(R"(f32 0.0)", format("{}", F32Result{f32{}}));
  EXPECT_EQ(R"(nan arithmetic)", format("{}", F32Result{NanKind::Arithmetic}));
  EXPECT_EQ(R"(nan canonical)", format("{}", F32Result{NanKind::Canonical}));
}

TEST(TextFormattersTest, F64Result) {
  EXPECT_EQ(R"(f64 0.0)", format("{}", F64Result{f64{}}));
  EXPECT_EQ(R"(nan arithmetic)", format("{}", F64Result{NanKind::Arithmetic}));
  EXPECT_EQ(R"(nan canonical)", format("{}", F64Result{NanKind::Canonical}));
}

TEST(TextFormattersTest, F32x4Result) {
  EXPECT_EQ(R"([f32 0.0 f32 0.0 f32 0.0 f32 0.0])", format("{}", F32x4Result{}));
  EXPECT_EQ(R"([f32 0.0 nan arithmetic f32 0.0 nan canonical])",
            format("{}", F32x4Result{f32{}, NanKind::Arithmetic, f32{},
                                     NanKind::Canonical}));
}

TEST(TextFormattersTest, F64x2Result) {
  EXPECT_EQ(R"([f64 0.0 f64 0.0])", format("{}", F64x2Result{}));
  EXPECT_EQ(R"([f64 0.0 nan arithmetic])",
            format("{}", F64x2Result{f64{}, NanKind::Arithmetic}));
}

TEST(TextFormattersTest, RefExternResult) {
  EXPECT_EQ(R"({})", format("{}", RefExternResult{}));
}

TEST(TextFormattersTest, RefFuncResult) {
  EXPECT_EQ(R"({})", format("{}", RefFuncResult{}));
}

TEST(TextFormattersTest, ReturnResult) {
  // u32
  EXPECT_EQ(R"(u32 0)", format("{}", ReturnResult{u32{}}));

  // u64
  EXPECT_EQ(R"(u64 0)", format("{}", ReturnResult{u64{}}));

  // v128
  EXPECT_EQ(R"(v128 0x0 0x0 0x0 0x0)", format("{}", ReturnResult{v128{}}));

  // F32Result
  EXPECT_EQ(R"(f32 f32 0.0)", format("{}", ReturnResult{F32Result{}}));

  // F64Result
  EXPECT_EQ(R"(f64 f64 0.0)", format("{}", ReturnResult{F64Result{}}));

  // F32x4Result
  EXPECT_EQ(R"(f32x4 [f32 0.0 f32 0.0 f32 0.0 f32 0.0])",
            format("{}", ReturnResult{F32x4Result{}}));

  // F64x2Result
  EXPECT_EQ(R"(f64x2 [f64 0.0 f64 0.0])",
            format("{}", ReturnResult{F64x2Result{}}));

  // RefExternResult
  EXPECT_EQ(R"(ref.extern {})", format("{}", ReturnResult{RefExternResult{}}));

  // RefFuncResult
  EXPECT_EQ(R"(ref.func {})", format("{}", ReturnResult{RefFuncResult{}}));
}

TEST(TextFormattersTest, ReturnResultList) {
  EXPECT_EQ(R"([u32 0 u64 0])", format("{}", ReturnResultList{
                                                 ReturnResult{u32{}},
                                                 ReturnResult{u64{}},
                                             }));
}

TEST(TextFormattersTest, ReturnAssertion) {
  EXPECT_EQ(
      R"({action invoke {module none, name {text "a", byte_size 1}, consts []}, results []})",
      format("{}",
             ReturnAssertion{
                 Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}, {}}));
}

TEST(TextFormattersTest, Assertion) {
  // ModuleAssertion.
  EXPECT_EQ(
      R"({kind invalid, desc module {module {name none, kind text, contents module []}, message {text "error", byte_size 5}}})",
      format("{}", Assertion{AssertionKind::Invalid,
                             ModuleAssertion{
                                 ScriptModule{nullopt, ScriptModuleKind::Text,
                                              Module{}},
                                 Text{"\"error\""_sv, 5}}}));

  // ActionAssertion.
  EXPECT_EQ(
      R"({kind action_trap, desc action {action invoke {module none, name {text "a", byte_size 1}, consts []}, message {text "error", byte_size 5}}})",
      format("{}",
             Assertion{AssertionKind::ActionTrap,
                       ActionAssertion{Action{InvokeAction{
                                           nullopt, Text{"\"a\""_sv, 1}, {}}},
                                       Text{"\"error\""_sv, 5}}}));

  // ReturnAssertion.
  EXPECT_EQ(
      R"({kind return, desc return {action invoke {module none, name {text "a", byte_size 1}, consts []}, results []}})",
      format("{}",
             Assertion{AssertionKind::Return,
                       ReturnAssertion{Action{InvokeAction{
                                           nullopt, Text{"\"a\""_sv, 1}, {}}},
                                       {}}}));
}

TEST(TextFormattersTest, Register) {
  EXPECT_EQ(R"({name {text "hi", byte_size 2}, module none})",
            format("{}", Register{Text{"\"hi\"", 2}, nullopt}));
}

TEST(TextFormattersTest, Command) {
  // ScriptModule.
  EXPECT_EQ(R"(module {name none, kind text, contents module []})",
            format("{}", Command{ScriptModule{nullopt, ScriptModuleKind::Text,
                                              Module{}}}));

  // Register.
  EXPECT_EQ(R"(register {name {text "hi", byte_size 2}, module none})",
            format("{}", Command{Register{Text{"\"hi\"", 2}, nullopt}}));

  // Action.
  EXPECT_EQ(
      R"(action get {module none, name {text "a", byte_size 1}})",
      format("{}", Command{Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}}}));

  // Assertion.
  EXPECT_EQ(
      R"(assertion {kind return, desc return {action invoke {module none, name {text "a", byte_size 1}, consts []}, results []}})",
      format("{}", Command{Assertion{
                       AssertionKind::Return,
                       ReturnAssertion{Action{InvokeAction{
                                           nullopt, Text{"\"a\""_sv, 1}, {}}},
                                       {}}}}));
}

TEST(TextFormattersTest, Script) {
  // Register.
  EXPECT_EQ(
      R"([register {name {text "hi", byte_size 2}, module none} action get {module none, name {text "a", byte_size 1}}])",
      format("{}",
             Script{Command{Register{Text{"\"hi\"", 2}, nullopt}},
                    Command{Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}}}}));
}
