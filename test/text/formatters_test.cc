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
#include "wasp/base/concat.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;

TEST(TextFormattersTest, TokenType) {
  EXPECT_EQ(R"(Binary)", concat(TokenType::Binary));
}

TEST(TextFormattersTest, Sign) {
  EXPECT_EQ(R"(None)", concat(Sign::None));
}

TEST(TextFormattersTest, LiteralKind) {
  EXPECT_EQ(R"(Normal)", concat(LiteralKind::Normal));
}

TEST(TextFormattersTest, Base) {
  EXPECT_EQ(R"(Decimal)", concat(Base::Decimal));
}

TEST(TextFormattersTest, HasUnderscores) {
  EXPECT_EQ(R"(No)", concat(HasUnderscores::No));
}

TEST(TextFormattersTest, LiteralInfo) {
  EXPECT_EQ(R"({sign None, kind NanPayload, base Hex, has_underscores No})",
            concat(LiteralInfo{Sign::None, LiteralKind::NanPayload, Base::Hex,
                               HasUnderscores::No}));
}

TEST(TextFormattersTest, Token) {
  EXPECT_EQ(R"({loc "\28", type Lpar, immediate empty})",
            concat(Token{"("_su8, TokenType::Lpar}));

  EXPECT_EQ(
      R"({loc "\69\33\32\2e\61\64\64", type BareInstr, immediate opcode_info {opcode i32.add, features none}})",
      concat(Token{"i32.add"_su8, TokenType::BareInstr,
                   OpcodeInfo{Opcode::I32Add, Features{0}}}));

  EXPECT_EQ(
      R"({loc "\69\33\32", type NumericType, immediate numeric_type i32})",
      concat(Token{"i32"_su8, TokenType::NumericType, NumericType::I32}));

  EXPECT_EQ(
      R"({loc "\66\75\6e\63\72\65\66", type ReferenceKind, immediate reference_kind funcref})",
      concat(Token{"funcref"_su8, TokenType::ReferenceKind,
                   ReferenceKind::Funcref}));

  EXPECT_EQ(
      R"({loc "\31\32\33", type Nat, immediate literal_info {sign None, kind Normal, base Decimal, has_underscores No}})",
      concat(Token{"123"_su8, TokenType::Nat,
                   LiteralInfo::Nat(HasUnderscores::No)}));
}

TEST(TextFormattersTest, Var) {
  EXPECT_EQ(R"(0)", concat(Var{Index{0}}));
  EXPECT_EQ(R"($a)", concat(Var{"$a"_sv}));
}

TEST(TextFormattersTest, VarList) {
  EXPECT_EQ(R"([0 1 2 $a])", concat(VarList{Var{Index{0}}, Var{Index{1}},
                                            Var{Index{2}}, Var{"$a"_sv}}));
}

TEST(TextFormattersTest, ValueTypeList) {
  EXPECT_EQ(R"([i32 f32])", concat(ValueTypeList{VT_I32, VT_F32}));
}

TEST(TextFormattersTest, FunctionType) {
  EXPECT_EQ(R"({params [i32], results [f32]})", concat(FunctionType{
                                                    ValueTypeList{VT_I32},
                                                    ValueTypeList{VT_F32},
                                                }));
}

TEST(TextFormattersTest, FunctionTypeUse) {
  EXPECT_EQ(R"({type_use none, type {params [], results []}})",
            concat(FunctionTypeUse{}));
  EXPECT_EQ(R"({type_use $a, type {params [i32], results [f32]}})",
            concat(FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                                     ValueTypeList{VT_I32},
                                                     ValueTypeList{VT_F32},
                                                 }}));
}

TEST(TextFormattersTest, BlockImmediate) {
  EXPECT_EQ(
      R"({label none, type {type_use none, type {params [], results []}}})",
      concat(BlockImmediate{}));
  EXPECT_EQ(
      R"({label $l, type {type_use $a, type {params [i32], results [f32]}}})",
      concat(BlockImmediate{
          BindVar{"$l"_sv},
          FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                            ValueTypeList{VT_I32},
                                            ValueTypeList{VT_F32},
                                        }}}));
}

TEST(TextFormattersTest, BrOnExnImmediate) {
  EXPECT_EQ(R"({target $a, event $b})",
            concat(BrOnExnImmediate{Var{"$a"_sv}, Var{"$b"_sv}}));
}

TEST(TextFormattersTest, BrTableImmediate) {
  EXPECT_EQ(R"({targets [], default_target $b})",
            concat(BrTableImmediate{{}, Var{"$b"_sv}}));
  EXPECT_EQ(R"({targets [0 1 2 $a], default_target $b})",
            concat(BrTableImmediate{VarList{Var{Index{0}}, Var{Index{1}},
                                            Var{Index{2}}, Var{"$a"_sv}},
                                    Var{"$b"_sv}}));
}

TEST(TextFormattersTest, CallIndirectImmediate) {
  EXPECT_EQ(
      R"({table none, type {type_use none, type {params [], results []}}})",
      concat(CallIndirectImmediate{}));
  EXPECT_EQ(
      R"({table $t, type {type_use $a, type {params [i32], results [f32]}}})",
      concat(CallIndirectImmediate{
          Var{"$t"_sv}, FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                                          ValueTypeList{VT_I32},
                                                          ValueTypeList{VT_F32},
                                                      }}}));
}

TEST(TextFormattersTest, CopyImmediate) {
  EXPECT_EQ(R"({dst none, src none})", concat(CopyImmediate{}));
  EXPECT_EQ(R"({dst $a, src $b})",
            concat(CopyImmediate{Var{"$a"_sv}, Var{"$b"_sv}}));
}

TEST(TextFormattersTest, InitImmediate) {
  EXPECT_EQ(R"({segment $a, dst none})",
            concat(InitImmediate{Var{"$a"_sv}, nullopt}));
  EXPECT_EQ(R"({segment $a, dst $b})",
            concat(InitImmediate{Var{"$a"_sv}, Var{"$b"_sv}}));
}

TEST(TextFormattersTest, LetImmediate) {
  EXPECT_EQ(
      R"({block {label none, type {type_use none, type {params [], results []}}}, locals []})",
      concat(LetImmediate{}));
}

TEST(TextFormattersTest, MemArgImmediate) {
  EXPECT_EQ(R"({align none, offset none})", concat(MemArgImmediate{}));
  EXPECT_EQ(R"({align 4, offset 0})", concat(MemArgImmediate{u32{4}, u32{0}}));
}

TEST(TextFormattersTest, Instruction) {
  EXPECT_EQ(R"({opcode nop, immediate empty})",
            concat(Instruction{Opcode::Nop}));

  EXPECT_EQ(R"({opcode i32.const, immediate s32 0})",
            concat(Instruction{Opcode::I32Const, s32{}}));

  EXPECT_EQ(R"({opcode i64.const, immediate s64 0})",
            concat(Instruction{Opcode::I64Const, s64{}}));

  EXPECT_EQ(R"({opcode f32.const, immediate f32 0})",
            concat(Instruction{Opcode::F32Const, f32{}}));

  EXPECT_EQ(R"({opcode f64.const, immediate f64 0})",
            concat(Instruction{Opcode::F64Const, f64{}}));

  EXPECT_EQ(R"({opcode v128.const, immediate v128 0x0 0x0 0x0 0x0})",
            concat(Instruction{Opcode::V128Const, v128{}}));

  EXPECT_EQ(
      R"({opcode block, immediate block {label none, type {type_use none, type {params [], results []}}}})",
      concat(Instruction{Opcode::Block, BlockImmediate{}}));

  EXPECT_EQ(R"({opcode br_on_exn, immediate br_on_exn {target $a, event $b}})",
            concat(Instruction{Opcode::BrOnExn,
                               BrOnExnImmediate{Var{"$a"_sv}, Var{"$b"_sv}}}));

  EXPECT_EQ(
      R"({opcode br_table, immediate br_table {targets [], default_target $b}})",
      concat(Instruction{Opcode::BrTable, BrTableImmediate{{}, Var{"$b"_sv}}}));

  EXPECT_EQ(R"({opcode table.copy, immediate copy {dst none, src none}})",
            concat(Instruction{Opcode::TableCopy, CopyImmediate{}}));

  EXPECT_EQ(R"({opcode table.init, immediate init {segment $a, dst none}})",
            concat(Instruction{Opcode::TableInit,
                               InitImmediate{Var{"$a"_sv}, nullopt}}));

  EXPECT_EQ(
      R"({opcode let, immediate let {block {label none, type {type_use none, type {params [], results []}}}, locals []}})",
      concat(Instruction{Opcode::Let, LetImmediate{}}));

  EXPECT_EQ(R"({opcode i32.load, immediate mem_arg {align none, offset none}})",
            concat(Instruction{Opcode::I32Load, MemArgImmediate{}}));

  EXPECT_EQ(R"({opcode select, immediate select []})",
            concat(Instruction{Opcode::Select, SelectImmediate{}}));

  EXPECT_EQ(
      R"({opcode v8x16.shuffle, immediate shuffle [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]})",
      concat(Instruction{Opcode::V8X16Shuffle, ShuffleImmediate{}}));

  EXPECT_EQ(R"({opcode local.get, immediate var 0})",
            concat(Instruction{Opcode::LocalGet, Var{Index{0}}}));
}

TEST(TextFormattersTest, InstructionList) {
  EXPECT_EQ(R"([{opcode nop, immediate empty} {opcode drop, immediate empty}])",
            concat(InstructionList{Instruction{Opcode::Nop},
                                   Instruction{Opcode::Drop}}));
}

TEST(TextFormattersTest, BoundValueType) {
  EXPECT_EQ(R"({name none, type i32})",
            concat(BoundValueType{nullopt, VT_I32}));
}

TEST(TextFormattersTest, BoundValueTypeList) {
  EXPECT_EQ(R"([{name none, type i32} {name $a, type f32}])",
            concat(BoundValueTypeList{
                BoundValueType{nullopt, VT_I32},
                BoundValueType{"$a"_sv, VT_F32},
            }));
}

TEST(TextFormattersTest, BoundFunctionType) {
  EXPECT_EQ(R"({params [{name none, type i32}], results [f32]})",
            concat(BoundFunctionType{
                BoundValueTypeList{BoundValueType{nullopt, VT_I32}},
                ValueTypeList{VT_F32}}));
}

TEST(TextFormattersTest, DefinedType) {
  EXPECT_EQ(R"({name $a, type func {params [], results []}})",
            concat(DefinedType{"$a"_sv, BoundFunctionType{}}));
}

TEST(TextFormattersTest, FunctionDesc) {
  EXPECT_EQ(R"({name none, type_use none, type {params [], results []}})",
            concat(FunctionDesc{}));
}

TEST(TextFormattersTest, TableDesc) {
  EXPECT_EQ(R"({name none, type {min 1} funcref})",
            concat(TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}));
}

TEST(TextFormattersTest, MemoryDesc) {
  EXPECT_EQ(R"({name none, type {min 1}})",
            concat(MemoryDesc{nullopt, MemoryType{Limits{1}}}));
}

TEST(TextFormattersTest, GlobalDesc) {
  EXPECT_EQ(R"({name none, type const i32})",
            concat(GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}}));
}

TEST(TextFormattersTest, EventType) {
  EXPECT_EQ(
      R"({attribute exception, type {type_use none, type {params [], results []}}})",
      concat(EventType{EventAttribute::Exception, FunctionTypeUse{}}));
}

TEST(TextFormattersTest, EventDesc) {
  EXPECT_EQ(
      R"({name none, type {attribute exception, type {type_use none, type {params [], results []}}}})",
      concat(EventDesc{
          nullopt, EventType{EventAttribute::Exception, FunctionTypeUse{}}}));
}

TEST(TextFormattersTest, Import) {
  // Function
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc func {name none, type_use none, type {params [], results []}}})",
      concat(Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1}, FunctionDesc{}}));

  // Table
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc table {name none, type {min 1} funcref}})",
      concat(Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                    TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}}));

  // Memory
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc memory {name none, type {min 1}}})",
      concat(Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                    MemoryDesc{nullopt, MemoryType{Limits{1}}}}));

  // Global
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc global {name none, type const i32}})",
      concat(
          Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                 GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}}}));
  // Event
  EXPECT_EQ(
      R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc event {name none, type {attribute exception, type {type_use none, type {params [], results []}}}}})",
      concat(Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1},
                    EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                 FunctionTypeUse{}}}}));
}

TEST(TextFormattersTest, InlineImport) {
  EXPECT_EQ(R"({module {text $a, byte_size 1}, name {text $b, byte_size 1}})",
            concat(InlineImport{Text{"$a"_sv, 1}, Text{"$b"_sv, 1}}));
}

TEST(TextFormattersTest, InlineExport) {
  EXPECT_EQ(R"({name {text $a, byte_size 1}})",
            concat(InlineExport{Text{"$a"_sv, 1}}));
}

TEST(TextFormattersTest, InlineExportList) {
  EXPECT_EQ(R"([{name {text $a, byte_size 1}} {name {text $b, byte_size 1}}])",
            concat(InlineExportList{
                InlineExport{Text{"$a"_sv, 1}},
                InlineExport{Text{"$b"_sv, 1}},
            }));
}

TEST(TextFormattersTest, Function) {
  EXPECT_EQ(
      R"({desc {name none, type_use none, type {params [], results []}}, locals [], instructions [], import none, exports []})",
      concat(Function{FunctionDesc{}, {}, {}, {}}));
}

TEST(TextFormattersTest, ElementListWithExpressions) {
  EXPECT_EQ(R"({elemtype funcref, list []})",
            concat(ElementListWithExpressions{RT_Funcref, {}}));
}

TEST(TextFormattersTest, ElementListWithVars) {
  EXPECT_EQ(R"({kind func, list []})",
            concat(ElementListWithVars{ExternalKind::Function, {}}));
}

TEST(TextFormattersTest, ElementList) {
  EXPECT_EQ(R"(expression {elemtype funcref, list []})",
            concat(ElementList{ElementListWithExpressions{RT_Funcref, {}}}));

  EXPECT_EQ(
      R"(var {kind func, list []})",
      concat(ElementList{ElementListWithVars{ExternalKind::Function, {}}}));
}

TEST(TextFormattersTest, Table) {
  EXPECT_EQ(
      R"({desc {name none, type {min 1} funcref}, import none, exports [], elements none})",
      concat(Table{TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}, {}}));
}

TEST(TextFormattersTest, Memory) {
  EXPECT_EQ(
      R"({desc {name none, type {min 1}}, import none, exports [], data none})",
      concat(Memory{MemoryDesc{nullopt, MemoryType{Limits{1}}}, {}}));
}

TEST(TextFormattersTest, Global) {
  EXPECT_EQ(
      R"({desc {name none, type const i32}, init {instructions []}, import none, exports []})",
      concat(Global{GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
                    ConstantExpression{},
                    {}}));
}

TEST(TextFormattersTest, Export) {
  EXPECT_EQ(
      R"({kind func, name {text $a, byte_size 1}, var 0})",
      concat(Export{ExternalKind::Function, Text{"$a"_sv, 1}, Var{Index{0}}}));
}

TEST(TextFormattersTest, Start) {
  EXPECT_EQ(R"({var 0})", concat(Start{Var{Index{0}}}));
}

TEST(TextFormattersTest, ElementSegment) {
  EXPECT_EQ(
      R"({name none, type passive, table none, offset none, elements var {kind func, list []}})",
      concat(ElementSegment{nullopt, SegmentType::Passive, {}}));
}

TEST(TextFormattersTest, DataSegment) {
  EXPECT_EQ(R"({name none, type passive, memory none, offset none, data []})",
            concat(DataSegment{nullopt, {}}));
}

TEST(TextFormattersTest, Event) {
  EXPECT_EQ(
      R"({desc {name none, type {attribute exception, type {type_use none, type {params [], results []}}}}, import none, exports []})",
      concat(Event{EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                FunctionTypeUse{}}},
                   {}}));
}

TEST(TextFormattersTest, ModuleItem) {
  // DefinedType
  EXPECT_EQ(R"(type {name $a, type func {params [], results []}})",
            concat(ModuleItem{DefinedType{"$a"_sv, BoundFunctionType{}}}));

  // Import
  EXPECT_EQ(
      R"(import {module {text $a, byte_size 1}, name {text $b, byte_size 1}, desc func {name none, type_use none, type {params [], results []}}})",
      concat(ModuleItem{
          Import{Text{"$a"_sv, 1}, Text{"$b"_sv, 1}, FunctionDesc{}}}));

  // Function
  EXPECT_EQ(
      R"(func {desc {name none, type_use none, type {params [], results []}}, locals [], instructions [], import none, exports []})",
      concat(ModuleItem{Function{FunctionDesc{}, {}, {}, {}}}));

  // Table
  EXPECT_EQ(
      R"(table {desc {name none, type {min 1} funcref}, import none, exports [], elements none})",
      concat(ModuleItem{
          Table{TableDesc{nullopt, TableType{Limits{1}, RT_Funcref}}, {}}}));

  // Memory
  EXPECT_EQ(
      R"(memory {desc {name none, type {min 1}}, import none, exports [], data none})",
      concat(
          ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{1}}}, {}}}));

  // Global
  EXPECT_EQ(
      R"(global {desc {name none, type const i32}, init {instructions []}, import none, exports []})",
      concat(ModuleItem{
          Global{GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
                 ConstantExpression{},
                 {}}}));

  // Export
  EXPECT_EQ(R"(export {kind func, name {text $a, byte_size 1}, var 0})",
            concat(ModuleItem{Export{ExternalKind::Function, Text{"$a"_sv, 1},
                                     Var{Index{0}}}}));

  // Start
  EXPECT_EQ(R"(start {var 0})", concat(ModuleItem{Start{Var{Index{0}}}}));

  // ElementSegment
  EXPECT_EQ(
      R"(elem {name none, type passive, table none, offset none, elements var {kind func, list []}})",
      concat(ModuleItem{ElementSegment{nullopt, SegmentType::Passive, {}}}));

  // DataSegment
  EXPECT_EQ(
      R"(data {name none, type passive, memory none, offset none, data []})",
      concat(ModuleItem{DataSegment{nullopt, {}}}));

  // Event
  EXPECT_EQ(
      R"(event {desc {name none, type {attribute exception, type {type_use none, type {params [], results []}}}}, import none, exports []})",
      concat(ModuleItem{Event{
          EventDesc{nullopt,
                    EventType{EventAttribute::Exception, FunctionTypeUse{}}},
          {}}}));
}

TEST(TextFormattersTest, Module) {
  EXPECT_EQ(
      R"([type {name $a, type func {params [], results []}} start {var 0}])",
      concat(Module{
          ModuleItem{DefinedType{"$a"_sv, BoundFunctionType{}}},
          ModuleItem{Start{Var{Index{0}}}},
      }));
}

TEST(TextFormattersTest, ScriptModuleKind) {
  EXPECT_EQ(R"(binary)", concat(ScriptModuleKind::Binary));
  EXPECT_EQ(R"(text)", concat(ScriptModuleKind::Text));
  EXPECT_EQ(R"(quote)", concat(ScriptModuleKind::Quote));
}

TEST(TextFormattersTest, ScriptModule) {
  // Text Module.
  EXPECT_EQ(R"({name none, kind text, contents module []})",
            concat(ScriptModule{nullopt, ScriptModuleKind::Text, Module{}}));

  // Binary Module.
  EXPECT_EQ(
      R"({name none, kind binary, contents text_list []})",
      concat(ScriptModule{nullopt, ScriptModuleKind::Binary, TextList{}}));

  // Quote Module.
  EXPECT_EQ(R"({name none, kind quote, contents text_list []})",
            concat(ScriptModule{nullopt, ScriptModuleKind::Quote, TextList{}}));
}

TEST(TextFormattersTest, RefNullConst) {
  EXPECT_EQ(R"({})", concat(RefNullConst{HT_Func}));
}

TEST(TextFormattersTest, RefExternConst) {
  EXPECT_EQ(R"({var 0})", concat(RefExternConst{u32{}}));
}

TEST(TextFormattersTest, Const) {
  // u32
  EXPECT_EQ(R"(u32 0)", concat(Const{u32{}}));

  // u64
  EXPECT_EQ(R"(u64 0)", concat(Const{u64{}}));

  // f32
  EXPECT_EQ(R"(f32 0)", concat(Const{f32{}}));

  // f64
  EXPECT_EQ(R"(f64 0)", concat(Const{f64{}}));

  // v128
  EXPECT_EQ(R"(v128 0x0 0x0 0x0 0x0)", concat(Const{v128{}}));

  // RefNullConst
  EXPECT_EQ(R"(ref.null {})", concat(Const{RefNullConst{HT_Func}}));

  // RefExternConst
  EXPECT_EQ(R"(ref.extern {var 0})", concat(Const{RefExternConst{u32{}}}));
}

TEST(TextFormattersTest, ConstList) {
  EXPECT_EQ(R"([u32 0 u64 0 f32 0 f64 0])", concat(ConstList{
                                                Const{u32{}},
                                                Const{u64{}},
                                                Const{f32{}},
                                                Const{f64{}},
                                            }));
}

TEST(TextFormattersTest, InvokeAction) {
  EXPECT_EQ(R"({module none, name {text "a", byte_size 1}, consts []})",
            concat(InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}));
}

TEST(TextFormattersTest, GetAction) {
  EXPECT_EQ(R"({module none, name {text "a", byte_size 1}})",
            concat(GetAction{nullopt, Text{"\"a\""_sv, 1}}));
}

TEST(TextFormattersTest, Action) {
  // InvokeAction.
  EXPECT_EQ(R"(invoke {module none, name {text "a", byte_size 1}, consts []})",
            concat(Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}));

  // GetAction.
  EXPECT_EQ(R"(get {module none, name {text "a", byte_size 1}})",
            concat(Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}}));
}

TEST(TextFormattersTest, AssertionKind) {
  EXPECT_EQ(R"(malformed)", concat(AssertionKind::Malformed));
  EXPECT_EQ(R"(invalid)", concat(AssertionKind::Invalid));
  EXPECT_EQ(R"(unlinkable)", concat(AssertionKind::Unlinkable));
  EXPECT_EQ(R"(action_trap)", concat(AssertionKind::ActionTrap));
  EXPECT_EQ(R"(return)", concat(AssertionKind::Return));
  EXPECT_EQ(R"(module_trap)", concat(AssertionKind::ModuleTrap));
  EXPECT_EQ(R"(exhaustion)", concat(AssertionKind::Exhaustion));
}

TEST(TextFormattersTest, ModuleAssertion) {
  EXPECT_EQ(
      R"({module {name none, kind text, contents module []}, message {text "error", byte_size 5}})",
      concat(ModuleAssertion{
          ScriptModule{nullopt, ScriptModuleKind::Text, Module{}},
          Text{"\"error\""_sv, 5}}));
}

TEST(TextFormattersTest, ActionAssertion) {
  EXPECT_EQ(
      R"({action invoke {module none, name {text "a", byte_size 1}, consts []}, message {text "error", byte_size 5}})",
      concat(ActionAssertion{
          Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}},
          Text{"\"error\""_sv, 5}}));
}

TEST(TextFormattersTest, NanKind) {
  EXPECT_EQ(R"(arithmetic)", concat(NanKind::Arithmetic));
  EXPECT_EQ(R"(canonical)", concat(NanKind::Canonical));
}

TEST(TextFormattersTest, F32Result) {
  EXPECT_EQ(R"(f32 0)", concat(F32Result{f32{}}));
  EXPECT_EQ(R"(nan arithmetic)", concat(F32Result{NanKind::Arithmetic}));
  EXPECT_EQ(R"(nan canonical)", concat(F32Result{NanKind::Canonical}));
}

TEST(TextFormattersTest, F64Result) {
  EXPECT_EQ(R"(f64 0)", concat(F64Result{f64{}}));
  EXPECT_EQ(R"(nan arithmetic)", concat(F64Result{NanKind::Arithmetic}));
  EXPECT_EQ(R"(nan canonical)", concat(F64Result{NanKind::Canonical}));
}

TEST(TextFormattersTest, F32x4Result) {
  EXPECT_EQ(R"([f32 0 f32 0 f32 0 f32 0])", concat(F32x4Result{}));
  EXPECT_EQ(R"([f32 0 nan arithmetic f32 0 nan canonical])",
            concat(F32x4Result{f32{}, NanKind::Arithmetic, f32{},
                               NanKind::Canonical}));
}

TEST(TextFormattersTest, F64x2Result) {
  EXPECT_EQ(R"([f64 0 f64 0])", concat(F64x2Result{}));
  EXPECT_EQ(R"([f64 0 nan arithmetic])",
            concat(F64x2Result{f64{}, NanKind::Arithmetic}));
}

TEST(TextFormattersTest, RefExternResult) {
  EXPECT_EQ(R"({})", concat(RefExternResult{}));
}

TEST(TextFormattersTest, RefFuncResult) {
  EXPECT_EQ(R"({})", concat(RefFuncResult{}));
}

TEST(TextFormattersTest, ReturnResult) {
  // u32
  EXPECT_EQ(R"(u32 0)", concat(ReturnResult{u32{}}));

  // u64
  EXPECT_EQ(R"(u64 0)", concat(ReturnResult{u64{}}));

  // v128
  EXPECT_EQ(R"(v128 0x0 0x0 0x0 0x0)", concat(ReturnResult{v128{}}));

  // F32Result
  EXPECT_EQ(R"(f32 f32 0)", concat(ReturnResult{F32Result{}}));

  // F64Result
  EXPECT_EQ(R"(f64 f64 0)", concat(ReturnResult{F64Result{}}));

  // F32x4Result
  EXPECT_EQ(R"(f32x4 [f32 0 f32 0 f32 0 f32 0])",
            concat(ReturnResult{F32x4Result{}}));

  // F64x2Result
  EXPECT_EQ(R"(f64x2 [f64 0 f64 0])", concat(ReturnResult{F64x2Result{}}));

  // RefExternResult
  EXPECT_EQ(R"(ref.extern {})", concat(ReturnResult{RefExternResult{}}));

  // RefFuncResult
  EXPECT_EQ(R"(ref.func {})", concat(ReturnResult{RefFuncResult{}}));
}

TEST(TextFormattersTest, ReturnResultList) {
  EXPECT_EQ(R"([u32 0 u64 0])", concat(ReturnResultList{
                                    ReturnResult{u32{}},
                                    ReturnResult{u64{}},
                                }));
}

TEST(TextFormattersTest, ReturnAssertion) {
  EXPECT_EQ(
      R"({action invoke {module none, name {text "a", byte_size 1}, consts []}, results []})",
      concat(ReturnAssertion{
          Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}, {}}));
}

TEST(TextFormattersTest, Assertion) {
  // ModuleAssertion.
  EXPECT_EQ(
      R"({kind invalid, desc module {module {name none, kind text, contents module []}, message {text "error", byte_size 5}}})",
      concat(
          Assertion{AssertionKind::Invalid,
                    ModuleAssertion{
                        ScriptModule{nullopt, ScriptModuleKind::Text, Module{}},
                        Text{"\"error\""_sv, 5}}}));

  // ActionAssertion.
  EXPECT_EQ(
      R"({kind action_trap, desc action {action invoke {module none, name {text "a", byte_size 1}, consts []}, message {text "error", byte_size 5}}})",
      concat(Assertion{AssertionKind::ActionTrap,
                       ActionAssertion{Action{InvokeAction{
                                           nullopt, Text{"\"a\""_sv, 1}, {}}},
                                       Text{"\"error\""_sv, 5}}}));

  // ReturnAssertion.
  EXPECT_EQ(
      R"({kind return, desc return {action invoke {module none, name {text "a", byte_size 1}, consts []}, results []}})",
      concat(Assertion{
          AssertionKind::Return,
          ReturnAssertion{
              Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}, {}}}));
}

TEST(TextFormattersTest, Register) {
  EXPECT_EQ(R"({name {text "hi", byte_size 2}, module none})",
            concat(Register{Text{"\"hi\"", 2}, nullopt}));
}

TEST(TextFormattersTest, Command) {
  // ScriptModule.
  EXPECT_EQ(
      R"(module {name none, kind text, contents module []})",
      concat(Command{ScriptModule{nullopt, ScriptModuleKind::Text, Module{}}}));

  // Register.
  EXPECT_EQ(R"(register {name {text "hi", byte_size 2}, module none})",
            concat(Command{Register{Text{"\"hi\"", 2}, nullopt}}));

  // Action.
  EXPECT_EQ(R"(action get {module none, name {text "a", byte_size 1}})",
            concat(Command{Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}}}));

  // Assertion.
  EXPECT_EQ(
      R"(assertion {kind return, desc return {action invoke {module none, name {text "a", byte_size 1}, consts []}, results []}})",
      concat(Command{Assertion{
          AssertionKind::Return,
          ReturnAssertion{
              Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}, {}}}}));
}

TEST(TextFormattersTest, Script) {
  // Register.
  EXPECT_EQ(
      R"([register {name {text "hi", byte_size 2}, module none} action get {module none, name {text "a", byte_size 1}}])",
      concat(Script{Command{Register{Text{"\"hi\"", 2}, nullopt}},
                    Command{Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}}}}));
}
