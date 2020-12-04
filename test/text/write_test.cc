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

#include <vector>

#include "gtest/gtest.h"

#include "test/text/constants.h"
#include "test/write_test_utils.h"
#include "wasp/base/errors.h"
#include "wasp/text/formatters.h"
#include "wasp/text/write.h"

using namespace ::wasp;
using namespace ::wasp::text;
using namespace ::wasp::text::test;
using namespace ::wasp::test;

using I = Instruction;
using O = Opcode;

namespace {

template <typename T, typename... Args>
void ExpectWrite(string_view expected, const T& value, Args&&... args) {
  WriteContext context;
  std::string result(expected.size(), 'X');
  auto iter =
      wasp::text::Write(context, value, std::forward<Args>(args)...,
                        MakeClampedIterator(result.begin(), result.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, result);
}

}  // namespace

TEST(TextWriteTest, Var) {
  ExpectWrite("0"_sv, Var{Index{}});
  ExpectWrite("$a"_sv, Var{"$a"_sv});
}

TEST(TextWriteTest, VarList) {
  ExpectWrite("0 $a 1 $b"_sv, VarList{
                                  Var{Index{}},
                                  Var{"$a"_sv},
                                  Var{Index{1}},
                                  Var{"$b"_sv},
                              });
}

TEST(TextWriteTest, Text) {
  ExpectWrite("\"hi\""_sv, Text{"\"hi\"", 2});
}

TEST(TextWriteTest, TextList) {
  ExpectWrite("\"hi\" \"bye\""_sv, TextList{
                                       Text{"\"hi\"", 2},
                                       Text{"\"bye\"", 3},
                                   });
}

TEST(TextWriteTest, ReferenceType) {
  ExpectWrite("externref"_sv, RT_Externref);
  ExpectWrite("funcref"_sv, RT_Funcref);
  ExpectWrite("exnref"_sv, RT_Exnref);
  ExpectWrite("eqref"_sv, RT_Eqref);
  ExpectWrite("i31ref"_sv, RT_I31ref);
  ExpectWrite("anyref"_sv, RT_Anyref);
  ExpectWrite("(ref func)"_sv, RT_RefFunc);
  ExpectWrite("(ref null func)"_sv, RT_RefNullFunc);
  ExpectWrite("(ref extern)"_sv, RT_RefExtern);
  ExpectWrite("(ref null extern)"_sv, RT_RefNullExtern);
  ExpectWrite("(ref exn)"_sv, RT_RefExn);
  ExpectWrite("(ref null exn)"_sv, RT_RefNullExn);
  ExpectWrite("(ref eq)"_sv, RT_RefEq);
  ExpectWrite("(ref null eq)"_sv, RT_RefNullEq);
  ExpectWrite("(ref i31)"_sv, RT_RefI31);
  ExpectWrite("(ref null i31)"_sv, RT_RefNullI31);
  ExpectWrite("(ref any)"_sv, RT_RefAny);
  ExpectWrite("(ref null any)"_sv, RT_RefNullAny);
  ExpectWrite("(ref 0)"_sv, RT_Ref0);
  ExpectWrite("(ref null 0)"_sv, RT_RefNull0);
  ExpectWrite("(ref $t)"_sv, RT_RefT);
  ExpectWrite("(ref null $t)"_sv, RT_RefNullT);
}

TEST(TextWriteTest, Rtt) {
  ExpectWrite("(rtt 0 func)"_sv, RTT_0_Func);
  ExpectWrite("(rtt 0 extern)"_sv, RTT_0_Extern);
  ExpectWrite("(rtt 0 exn)"_sv, RTT_0_Exn);
  ExpectWrite("(rtt 0 eq)"_sv, RTT_0_Eq);
  ExpectWrite("(rtt 0 i31)"_sv, RTT_0_I31);
  ExpectWrite("(rtt 0 any)"_sv, RTT_0_Any);
}

TEST(TextWriteTest, ValueType) {
  // NumericType
  ExpectWrite("i32"_sv, VT_I32);
  ExpectWrite("i64"_sv, VT_I64);
  ExpectWrite("f32"_sv, VT_F32);
  ExpectWrite("f64"_sv, VT_F64);
  ExpectWrite("v128"_sv, VT_V128);

  // ReferenceType
  ExpectWrite("externref"_sv, VT_Externref);
  ExpectWrite("funcref"_sv, VT_Funcref);
  ExpectWrite("exnref"_sv, VT_Exnref);
  ExpectWrite("eqref"_sv, VT_Eqref);
  ExpectWrite("i31ref"_sv, VT_I31ref);
  ExpectWrite("anyref"_sv, VT_Anyref);
  ExpectWrite("(ref func)"_sv, VT_RefFunc);
  ExpectWrite("(ref null func)"_sv, VT_RefNullFunc);
  ExpectWrite("(ref extern)"_sv, VT_RefExtern);
  ExpectWrite("(ref null extern)"_sv, VT_RefNullExtern);
  ExpectWrite("(ref exn)"_sv, VT_RefExn);
  ExpectWrite("(ref null exn)"_sv, VT_RefNullExn);
  ExpectWrite("(ref eq)"_sv, VT_RefEq);
  ExpectWrite("(ref null eq)"_sv, VT_RefNullEq);
  ExpectWrite("(ref i31)"_sv, VT_RefI31);
  ExpectWrite("(ref null i31)"_sv, VT_RefNullI31);
  ExpectWrite("(ref any)"_sv, VT_RefAny);
  ExpectWrite("(ref null any)"_sv, VT_RefNullAny);
  ExpectWrite("(ref 0)"_sv, VT_Ref0);
  ExpectWrite("(ref null 0)"_sv, VT_RefNull0);
  ExpectWrite("(ref $t)"_sv, VT_RefT);
  ExpectWrite("(ref null $t)"_sv, VT_RefNullT);

  // Rtt
  ExpectWrite("(rtt 0 func)"_sv, VT_RTT_0_Func);
  ExpectWrite("(rtt 0 extern)"_sv, VT_RTT_0_Extern);
  ExpectWrite("(rtt 0 exn)"_sv, VT_RTT_0_Exn);
  ExpectWrite("(rtt 0 eq)"_sv, VT_RTT_0_Eq);
  ExpectWrite("(rtt 0 i31)"_sv, VT_RTT_0_I31);
  ExpectWrite("(rtt 0 any)"_sv, VT_RTT_0_Any);
}

TEST(TextWriteTest, ValueTypeList) {
  ExpectWrite("i32 i64"_sv, ValueTypeList{VT_I32, VT_I64});
}

TEST(TextWriteTest, StorageType) {
  ExpectWrite("i32"_sv, StorageType{VT_I32});
  ExpectWrite("i8"_sv, StorageType{PackedType::I8});
  ExpectWrite("i16"_sv, StorageType{PackedType::I16});
}

TEST(TextWriteTest, FieldType) {
  // No name, immutable
  ExpectWrite("(field i32)"_sv,
              FieldType{nullopt, StorageType{VT_I32}, Mutability::Const});

  // No name, mutable
  ExpectWrite("(field (mut i32))"_sv,
              FieldType{nullopt, StorageType{VT_I32}, Mutability::Var});

  // Named
  ExpectWrite("(field $a i32)"_sv,
              FieldType{"$a"_sv, StorageType{VT_I32}, Mutability::Const});
}

TEST(TextWriteTest, FieldTypeList) {
  // Combine unbound fields.
  ExpectWrite("(field i32 (mut i64) f32)"_sv,
              FieldTypeList{
                  FieldType{nullopt, StorageType{VT_I32}, Mutability::Const},
                  FieldType{nullopt, StorageType{VT_I64}, Mutability::Var},
                  FieldType{nullopt, StorageType{VT_F32}, Mutability::Const},
              });

  // Bound fields must be separated.
  ExpectWrite("(field i32) (field $a (mut i64)) (field f32)"_sv,
              FieldTypeList{
                  FieldType{nullopt, StorageType{VT_I32}, Mutability::Const},
                  FieldType{"$a"_sv, StorageType{VT_I64}, Mutability::Var},
                  FieldType{nullopt, StorageType{VT_F32}, Mutability::Const},
              });
}

TEST(TextWriteTest, StructType) {
  ExpectWrite(
      "(struct (field i32 i8))",
      StructType{FieldTypeList{
          FieldType{nullopt, StorageType{VT_I32}, Mutability::Const},
          FieldType{nullopt, StorageType{PackedType::I8}, Mutability::Const},
      }});
}

TEST(TextWriteTest, ArrayType) {
  ExpectWrite(
      "(array (field i32))",
      ArrayType{FieldType{nullopt, StorageType{VT_I32}, Mutability::Const}});
}

TEST(TextWriteTest, FunctionType) {
  ExpectWrite("(param i32 i64) (result f32)"_sv,
              FunctionType{
                  ValueTypeList{VT_I32, VT_I64},
                  ValueTypeList{VT_F32},
              });
}

TEST(TextWriteTest, FunctionTypeUse) {
  ExpectWrite(""_sv, FunctionTypeUse{});
  ExpectWrite("(type 0)"_sv, FunctionTypeUse{Var{Index{0}}, {}});
  ExpectWrite("(result i32)"_sv,
              FunctionTypeUse{nullopt, FunctionType{
                                           ValueTypeList{},
                                           ValueTypeList{VT_I32},
                                       }});
  ExpectWrite("(type $a) (param i32) (result i32)"_sv,
              FunctionTypeUse{Var{"$a"_sv}, FunctionType{
                                                ValueTypeList{VT_I32},
                                                ValueTypeList{VT_I32},
                                            }});
}

TEST(TextWriteTest, BlockImmediate) {
  ExpectWrite(""_sv, BlockImmediate{});
  ExpectWrite("$l"_sv, BlockImmediate{"$l"_sv, {}});
  ExpectWrite("$l (type 0)"_sv,
              BlockImmediate{"$l"_sv, FunctionTypeUse{Var{Index{0}}, {}}});
}

TEST(TextWriteTest, BrOnCastImmediate) {
  ExpectWrite(
      "$l func 0"_sv,
      BrOnCastImmediate{Var{"$l"_sv}, HeapType2Immediate{HT_Func, HT_0}});
}

TEST(TextWriteTest, BrOnExnImmediate) {
  ExpectWrite("$l $e"_sv, BrOnExnImmediate{
                              Var{"$l"_sv},
                              Var{"$e"_sv},
                          });
}

TEST(TextWriteTest, BrTableImmediate) {
  ExpectWrite("0"_sv, BrTableImmediate{{}, Var{Index{0}}});
  ExpectWrite("0 1 2 $def"_sv, BrTableImmediate{VarList{
                                                    Var{Index{0}},
                                                    Var{Index{1}},
                                                    Var{Index{2}},
                                                },
                                                Var{"$def"_sv}});
}

TEST(TextWriteTest, CallIndirectImmediate) {
  ExpectWrite("$t"_sv, CallIndirectImmediate{Var{"$t"_sv}, {}});
  ExpectWrite(
      "$t (type 0)"_sv,
      CallIndirectImmediate{Var{"$t"_sv}, FunctionTypeUse{Var{Index{0}}, {}}});
}

TEST(TextWriteTest, CopyImmediate) {
  ExpectWrite("$d"_sv, CopyImmediate{Var{"$d"_sv}, nullopt});
  ExpectWrite("$s"_sv, CopyImmediate{nullopt, Var{"$s"_sv}});
  ExpectWrite("$d $s"_sv, CopyImmediate{Var{"$d"_sv}, Var{"$s"_sv}});
}

TEST(TextWriteTest, HeapType2Immediate) {
  ExpectWrite("func 0"_sv, HeapType2Immediate{HT_Func, HT_0});
}

TEST(TextWriteTest, InitImmediate) {
  ExpectWrite("$seg"_sv, InitImmediate{Var{"$seg"_sv}, nullopt});
  ExpectWrite("$dst $seg"_sv, InitImmediate{Var{"$seg"_sv}, Var{"$dst"_sv}});
}

TEST(TextWriteTest, LetImmediate) {
  ExpectWrite(""_sv, LetImmediate{});
  ExpectWrite("$l"_sv, LetImmediate{BlockImmediate{BindVar{"$l"_sv}, {}}, {}});
  ExpectWrite(
      "(type 0) (local i32)"_sv,
      LetImmediate{BlockImmediate{nullopt, FunctionTypeUse{Var{Index{0}}, {}}},
                   BoundValueTypeList{BoundValueType{nullopt, VT_I32}}});
  ExpectWrite(
      "(param i32) (local f32 f64)"_sv,
      LetImmediate{
          BlockImmediate{nullopt,
                         FunctionTypeUse{
                             nullopt, FunctionType{ValueTypeList{VT_I32}, {}}}},
          BoundValueTypeList{BoundValueType{nullopt, VT_F32},
                             BoundValueType{nullopt, VT_F64}}});
}

TEST(TextWriteTest, MemArgImmediate) {
  ExpectWrite(""_sv, MemArgImmediate{});
  ExpectWrite("align=4"_sv, MemArgImmediate{u32{4}, nullopt});
  ExpectWrite("offset=10"_sv, MemArgImmediate{nullopt, u32{10}});
  ExpectWrite("offset=10 align=4"_sv, MemArgImmediate{u32{4}, u32{10}});
}

TEST(TextWriteTest, RttSubImmediate) {
  ExpectWrite("1 func 0"_sv,
              RttSubImmediate{1u, HeapType2Immediate{HT_Func, HT_0}});
}

TEST(TextWriteTest, ShuffleImmediate) {
  ExpectWrite("0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_sv, ShuffleImmediate{});
  ExpectWrite("0 1 2 3 4 5 6 7 8 7 6 5 4 3 2 1"_sv,
              ShuffleImmediate{0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1});
}

TEST(TextWriteTest, StructFieldImmediate) {
  ExpectWrite("$s $f"_sv, StructFieldImmediate{Var{"$s"_sv}, Var{"$f"_sv}});
}

TEST(TextWriteTest, Opcode) {
  ExpectWrite("nop"_sv, Opcode::Nop);
  ExpectWrite("i32.add"_sv, Opcode::I32Add);
}

TEST(TextWriteTest, Instruction) {
  // Bare
  ExpectWrite("nop"_sv, I{O::Nop});

  // s32
  ExpectWrite("i32.const 0"_sv, I{O::I32Const, s32{}});

  // s64
  ExpectWrite("i64.const 0"_sv, I{O::I64Const, s64{}});

  // f32
  ExpectWrite("f32.const 0"_sv, I{O::F32Const, f32{}});

  // f64
  ExpectWrite("f64.const 0"_sv, I{O::F64Const, f64{}});

  // v128
  ExpectWrite("v128.const i32x4 0 0 0 0"_sv, I{O::V128Const, v128{}});

  // BlockImmediate
  ExpectWrite(
      "block $l (type 0) (param i32)"_sv,
      I{O::Block,
        BlockImmediate{"$l"_sv, FunctionTypeUse{Var{Index{0}},
                                                FunctionType{{VT_I32}, {}}}}});

#if 0
  // BrOnCastImmediate
  ExpectWrite(
      "br_on_cast $l func 0"_sv,
      I{O::BrOnCast,
        BrOnCastImmediate{Var{"$l"_sv}, HeapType2Immediate{HT_Func, HT_0}}});
#else
  // BrOnCastImmediate
  ExpectWrite("br_on_cast $l"_sv, I{O::BrOnCast, Var{"$l"_sv}});
#endif

  // BrOnExnImmediate
  ExpectWrite("br_on_exn $l $e"_sv,
              I{O::BrOnExn, BrOnExnImmediate{Var{"$l"_sv}, Var{"$e"_sv}}});

  // BrTableImmediate
  ExpectWrite(
      "br_table 0 1 $d"_sv,
      I{O::BrTable,
        BrTableImmediate{VarList{Var{Index{0}}, Var{Index{1}}}, Var{"$d"_sv}}});

  // CallIndirectImmediate
  ExpectWrite(
      "call_indirect $t (type 0) (param i32)"_sv,
      I{O::CallIndirect,
        CallIndirectImmediate{
            Var{"$t"_sv},
            FunctionTypeUse{Var{Index{0}}, FunctionType{{VT_I32}, {}}}}});

  // CopyImmediate
  ExpectWrite("table.copy $d $s"_sv,
              I{O::TableCopy, CopyImmediate{Var{"$d"_sv}, Var{"$s"_sv}}});

  // FuncBindImmediate
  ExpectWrite("func.bind (type 0)"_sv,
              I{O::FuncBind, FuncBindImmediate{Var{Index{0}}, {}}});

  // HeapType2Immediate
  ExpectWrite("ref.test func 0"_sv,
              I{O::RefTest, HeapType2Immediate{HT_Func, HT_0}});

  // InitImmediate
  ExpectWrite("table.init $table $seg"_sv,
              I{O::TableInit, InitImmediate{Var{"$seg"_sv}, Var{"$table"_sv}}});

  // MemArgImmediate
  ExpectWrite("i32.load offset=10 align=4"_sv,
              I{O::I32Load, MemArgImmediate{u32{4}, u32{10}}});

#if 0
  // RttSubImmediate
  ExpectWrite(
      "rtt.sub 1 func 0"_sv,
      I{O::RttSub, RttSubImmediate{1u, HeapType2Immediate{HT_Func, HT_0}}});
#else
  // RttSubImmediate
  ExpectWrite("rtt.sub 0"_sv, I{O::RttSub, HT_0});
#endif

  // SelectImmediate
  ExpectWrite("select i32"_sv, I{O::Select, SelectImmediate{VT_I32}});

  // ShuffleImmediate
  ExpectWrite("i8x16.shuffle 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"_sv,
              I{O::I8X16Shuffle, ShuffleImmediate{}});

  // StructFieldImmediate
  ExpectWrite(
      "struct.get $s $f"_sv,
      I{O::StructGet, StructFieldImmediate{Var{"$s"_sv}, Var{"$f"_sv}}});

  // Var
  ExpectWrite("local.get $a"_sv, I{O::LocalGet, Var{"$a"_sv}});

  // LetImmediate
  ExpectWrite(
      "let $l (local i32)"_sv,
      I{O::Let,
        LetImmediate{BlockImmediate{"$l"_sv, FunctionTypeUse{}},
                     BoundValueTypeList{BoundValueType{nullopt, VT_I32}}}});
}

TEST(TextWriteTest, InstructionList) {
  ExpectWrite("block nop end nop"_sv, InstructionList{
                                          I{O::Block, BlockImmediate{}},
                                          I{O::Nop},
                                          I{O::End},
                                          I{O::Nop},
                                      });
}

TEST(TextWriteTest, BoundValueType) {
  ExpectWrite("i32"_sv, BoundValueType{nullopt, VT_I32});
  ExpectWrite("$a i32"_sv, BoundValueType{"$a"_sv, VT_I32});
}

TEST(TextWriteTest, BoundValueTypeList_Param) {
  ExpectWrite(""_sv, BoundValueTypeList{}, "param");

  ExpectWrite("(param $a i32)"_sv,
              BoundValueTypeList{
                  BoundValueType{"$a"_sv, VT_I32},
              },
              "param");

  ExpectWrite("(param i32 i32)"_sv,
              BoundValueTypeList{
                  BoundValueType{nullopt, VT_I32},
                  BoundValueType{nullopt, VT_I32},
              },
              "param");

  ExpectWrite("(param i32 f32) (param $a i32) (param i32)"_sv,
              BoundValueTypeList{
                  BoundValueType{nullopt, VT_I32},
                  BoundValueType{nullopt, VT_F32},
                  BoundValueType{"$a"_sv, VT_I32},
                  BoundValueType{nullopt, VT_I32},
              },
              "param");
}

TEST(TextWriteTest, BoundFunctionType) {
  ExpectWrite(""_sv, BoundFunctionType{});

  ExpectWrite("(param i32)"_sv,
              BoundFunctionType{
                  BoundValueTypeList{BoundValueType{nullopt, VT_I32}}, {}});

  ExpectWrite("(result i32)"_sv, BoundFunctionType{{}, ValueTypeList{VT_I32}});

  ExpectWrite(
      "(param $a i32) (result i32)"_sv,
      BoundFunctionType{BoundValueTypeList{BoundValueType{"$a"_sv, VT_I32}},
                        ValueTypeList{VT_I32}});
}

TEST(TextWriteTest, DefinedType) {
  ExpectWrite("(type (func))"_sv, DefinedType{nullopt, BoundFunctionType{}});

  ExpectWrite(
      "(type $F (func (param $a i32) (result i32)))"_sv,
      DefinedType{"$F"_sv,
                  BoundFunctionType{
                      BoundValueTypeList{BoundValueType{"$a"_sv, VT_I32}},
                      ValueTypeList{VT_I32},
                  }});

  ExpectWrite(
      "(type $A (array (field i32)))"_sv,
      DefinedType{"$A"_sv, ArrayType{FieldType{nullopt, StorageType{VT_I32},
                                               Mutability::Const}}});

  ExpectWrite(
      "(type $S (struct (field i32 i8)))"_sv,
      DefinedType{"$S"_sv, StructType{FieldTypeList{
                               FieldType{nullopt, StorageType{VT_I32},
                                         Mutability::Const},
                               FieldType{nullopt, StorageType{PackedType::I8},
                                         Mutability::Const},
                           }}});
}

TEST(TextWriteTest, FunctionDesc) {
  ExpectWrite("func"_sv, FunctionDesc{});

  ExpectWrite("func $f"_sv, FunctionDesc{"$f"_sv, nullopt, {}});

  ExpectWrite("func (type 0)"_sv, FunctionDesc{nullopt, Var{Index{0}}, {}});

  ExpectWrite(
      "func (param i32)"_sv,
      FunctionDesc{
          nullopt, nullopt,
          BoundFunctionType{BoundValueTypeList{BoundValueType{nullopt, VT_I32}},
                            {}}});

  ExpectWrite(
      "func $f (type 0) (param i32)"_sv,
      FunctionDesc{
          "$f"_sv, Var{Index{0}},
          BoundFunctionType{BoundValueTypeList{BoundValueType{nullopt, VT_I32}},
                            {}}});
}

TEST(TextWriteTest, Limits) {
  ExpectWrite("0"_sv, Limits{0});
  ExpectWrite("0 0"_sv, Limits{0, 0});
  ExpectWrite("0 0 shared"_sv, Limits{0, 0, Shared::Yes});
  ExpectWrite("i64 0 0"_sv, Limits{0, 0, Shared::No, IndexType::I64});
}

TEST(TextWriteTest, TableType) {
  ExpectWrite("0 funcref"_sv, TableType{Limits{0}, RT_Funcref});
}

TEST(TextWriteTest, TableDesc) {
  ExpectWrite("table 0 funcref"_sv,
              TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}});

  ExpectWrite("table $t 1 funcref"_sv,
              TableDesc{"$t"_sv, TableType{Limits{1}, RT_Funcref}});
}

TEST(TextWriteTest, MemoryType) {
  ExpectWrite("0"_sv, MemoryType{Limits{0}});
}

TEST(TextWriteTest, MemoryDesc) {
  ExpectWrite("memory 1 2"_sv, MemoryDesc{nullopt, MemoryType{Limits{1, 2}}});

  ExpectWrite("memory $m 1"_sv, MemoryDesc{"$m"_sv, MemoryType{Limits{1}}});
}

TEST(TextWriteTest, GlobalType) {
  ExpectWrite("i32"_sv, GlobalType{VT_I32, Mutability::Const});
  ExpectWrite("(mut f32)"_sv, GlobalType{VT_F32, Mutability::Var});
}

TEST(TextWriteTest, GlobalDesc) {
  ExpectWrite("global i32"_sv,
              GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}});

  ExpectWrite("global $g (mut f32)"_sv,
              GlobalDesc{"$g"_sv, GlobalType{VT_F32, Mutability::Var}});
}

TEST(TextWriteTest, EventType) {
  ExpectWrite(""_sv, EventType{});
  ExpectWrite("(type 0)"_sv, EventType{EventAttribute::Exception,
                                       FunctionTypeUse{Var{Index{0}}, {}}});
}

TEST(TextWriteTest, EventDesc) {
  ExpectWrite("event"_sv, EventDesc{nullopt, EventType{}});
  ExpectWrite(
      "event $e (type 0)"_sv,
      EventDesc{"$e"_sv, EventType{EventAttribute::Exception,
                                   FunctionTypeUse{Var{Index{0}}, {}}}});
}

TEST(TextWriteTest, Import) {
  // Function
  ExpectWrite("(import \"a\" \"b\" (func))"_sv,
              Import{Text{"\"a\"", 1}, Text{"\"b\"", 1}, FunctionDesc{}});

  // Table
  ExpectWrite("(import \"a\" \"b\" (table 0 funcref))"_sv,
              Import{Text{"\"a\"", 1}, Text{"\"b\"", 1},
                     TableDesc{nullopt, TableType{Limits{0}, RT_Funcref}}});

  // Memory
  ExpectWrite("(import \"a\" \"b\" (memory 0))"_sv,
              Import{Text{"\"a\"", 1}, Text{"\"b\"", 1},
                     MemoryDesc{nullopt, MemoryType{Limits{0}}}});

  // Global
  ExpectWrite(
      "(import \"a\" \"b\" (global i32))"_sv,
      Import{Text{"\"a\"", 1}, Text{"\"b\"", 1},
             GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}}});

  // Event
  ExpectWrite("(import \"a\" \"b\" (event))"_sv,
              Import{Text{"\"a\"", 1}, Text{"\"b\"", 1},
                     EventDesc{nullopt, EventType{EventAttribute::Exception,
                                                  FunctionTypeUse{}}}});
}

TEST(TextWriteTest, InlineImport) {
  ExpectWrite("(import \"a\" \"b\")"_sv,
              InlineImport{Text{"\"a\"", 1}, Text{"\"b\"", 1}});
}

TEST(TextWriteTest, InlineExport) {
  ExpectWrite("(export \"a\")"_sv, InlineExport{Text{"\"a\"", 1}});
}

TEST(TextWriteTest, InlineExportList) {
  ExpectWrite(""_sv, InlineExportList{});

  ExpectWrite("(export \"a\") (export \"b\")"_sv,
              InlineExportList{
                  InlineExport{Text{"\"a\"", 1}},
                  InlineExport{Text{"\"b\"", 1}},
              });
}

TEST(TextWriteTest, Function) {
  // Empty func.
  ExpectWrite("(func)"_sv, Function{});

  // Name.
  ExpectWrite("(func $f)"_sv,
              Function{FunctionDesc{"$f"_sv, nullopt, {}}, {}, {}, {}});

  // Inline export.
  ExpectWrite("(func (export \"e\"))"_sv,
              Function{FunctionDesc{{}, nullopt, {}},
                       {},
                       {},
                       InlineExportList{
                           InlineExport{Text{"\"e\""_sv, 1}},
                       }});

  // Locals.
  ExpectWrite("(func\n  (local i32 i64))"_sv,
              Function{{},
                       BoundValueTypeList{
                           BoundValueType{nullopt, VT_I32},
                           BoundValueType{nullopt, VT_I64},
                       },
                       {},
                       {}});

  // Instruction.
  ExpectWrite(
      "(func\n  nop\n  nop\n  nop)"_sv,
      Function{{}, {}, InstructionList{I{O::Nop}, I{O::Nop}, I{O::Nop}}, {}});

  // Everything for defined Function.
  ExpectWrite("(func $f (export \"m\") (type 0)\n  (local i32)\n  nop)"_sv,
              Function{FunctionDesc{"$f"_sv, Var{Index{0}}, {}},
                       BoundValueTypeList{
                           BoundValueType{nullopt, VT_I32},
                       },
                       InstructionList{I{O::Nop}},
                       InlineExportList{
                           InlineExport{Text{"\"m\""_sv, 1}},
                       }});
}

TEST(TextWriteTest, FunctionInlineImport) {
  // Import.
  ExpectWrite(
      "(func (import \"m\" \"n\"))"_sv,
      Function{{}, InlineImport{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1}}, {}});

  // Everything for imported Function.
  ExpectWrite("(func $f (export \"m\") (import \"a\" \"b\") (param i32))"_sv,
              Function{FunctionDesc{"$f"_sv, nullopt,
                                    BoundFunctionType{
                                        {BoundValueType{nullopt, VT_I32}}, {}}},
                       InlineImport{Text{"\"a\""_sv, 1}, Text{"\"b\""_sv, 1}},
                       InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});
}

TEST(TextWriteTest, Function_OmitFinalEnd) {
  ExpectWrite(
      "(func\n  nop\n  nop)"_sv,
      Function{{}, {}, InstructionList{I{O::Nop}, I{O::Nop}, I{O::End}}, {}});
}


TEST(TextWriteTest, Function_DontOverDedent) {
  // Multiple ends like this is syntatically malformed, but still should be
  // writable.
  ExpectWrite(
      "(func\n  end\n  end)"_sv,
      Function{{}, {}, InstructionList{I{O::End}, I{O::End}, I{O::End}}, {}});
}

TEST(TextWriteTest, ElementExpressionList) {
  ExpectWrite("(ref.null) (ref.func 0)"_sv,
              ElementExpressionList{
                  ElementExpression{
                      I{O::RefNull},
                  },
                  ElementExpression{
                      I{O::RefFunc, Var{Index{0}}},
                  },
              });
}

TEST(TextWriteTest, ElementListWithExpressions) {
  ExpectWrite("funcref"_sv, ElementListWithExpressions{RT_Funcref, {}});

  ExpectWrite("funcref (ref.null)"_sv,
              ElementListWithExpressions{RT_Funcref,
                                         ElementExpressionList{
                                             ElementExpression{I{O::RefNull}},
                                         }});
}

TEST(TextWriteTest, ElementListWithVars) {
  ExpectWrite("func"_sv, ElementListWithVars{ExternalKind::Function, {}});

  ExpectWrite("func 0 1"_sv,
              ElementListWithVars{ExternalKind::Function, VarList{
                                                              Var{Index{0}},
                                                              Var{Index{1}},
                                                          }});
}

TEST(TextWriteTest, ElementList) {
  ExpectWrite("funcref"_sv,
              ElementList{ElementListWithExpressions{RT_Funcref, {}}});

  ExpectWrite("func 0"_sv,
              ElementList{ElementListWithVars{ExternalKind::Function,
                                              VarList{Var{Index{0}}}}});
}

TEST(TextWriteTest, Table) {
  // Simplest table.
  ExpectWrite("(table 0 funcref)"_sv,
              Table{TableDesc{{}, TableType{Limits{u32{0}}, RT_Funcref}}, {}});

  // Name.
  ExpectWrite(
      "(table $t 0 funcref)"_sv,
      Table{TableDesc{"$t"_sv, TableType{Limits{u32{0}}, RT_Funcref}}, {}});

  // Inline export.
  ExpectWrite("(table (export \"m\") 0 funcref)"_sv,
              Table{TableDesc{{}, TableType{Limits{u32{0}}, RT_Funcref}},
                    InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});

  // Name and inline export.
  ExpectWrite("(table $t2 (export \"m\") 0 funcref)"_sv,
              Table{TableDesc{"$t2"_sv, TableType{Limits{u32{0}}, RT_Funcref}},
                    InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});

  // Inline element var list.
  ExpectWrite(
      "(table funcref (elem 0 1 2))"_sv,
      Table{TableDesc{{}, TableType{Limits{u32{3}, u32{3}}, RT_Funcref}},
            {},
            ElementListWithVars{ExternalKind::Function, VarList{
                                                            Var{Index{0}},
                                                            Var{Index{1}},
                                                            Var{Index{2}},
                                                        }}});

  // Inline element var list.
  ExpectWrite(
      "(table funcref (elem (nop) (nop)))"_sv,
      Table{TableDesc{{}, TableType{Limits{u32{2}, u32{2}}, RT_Funcref}},
            {},
            ElementListWithExpressions{RT_Funcref,
                                       ElementExpressionList{
                                           ElementExpression{I{O::Nop}},
                                           ElementExpression{I{O::Nop}},
                                       }}});
}

TEST(TextWriteTest, TableInlineImport) {
  // Inline import.
  ExpectWrite("(table (import \"m\" \"n\") 0 funcref)"_sv,
              Table{TableDesc{{}, TableType{Limits{u32{0}}, RT_Funcref}},
                    InlineImport{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1}},
                    {}});

  // Everything for Table import.
  ExpectWrite("(table $t (export \"m\") (import \"a\" \"b\") 0 funcref)"_sv,
              Table{TableDesc{"$t"_sv, TableType{Limits{u32{0}}, RT_Funcref}},
                    InlineImport{Text{"\"a\""_sv, 1}, Text{"\"b\""_sv, 1}},
                    InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});
}

TEST(TextWriteTest, NumericData) {
  Buffer buffer = ToBuffer(
      "\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00"_su8);

  ExpectWrite("(i8 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)"_sv,
              NumericData{NumericDataType::I8, buffer});
  ExpectWrite("(i16 0 0 0 0 0 0 0 0)"_sv,
              NumericData{NumericDataType::I16, buffer});
  ExpectWrite("(i32 0 0 0 0)"_sv, NumericData{NumericDataType::I32, buffer});
  ExpectWrite("(i64 0 0)"_sv, NumericData{NumericDataType::I64, buffer});
  ExpectWrite("(f32 0 0 0 0)"_sv, NumericData{NumericDataType::F32, buffer});
  ExpectWrite("(f64 0 0)"_sv, NumericData{NumericDataType::F64, buffer});
  ExpectWrite("(v128 i32x4 0 0 0 0)"_sv,
              NumericData{NumericDataType::V128, buffer});
}

TEST(TextWriteTest, DataItem) {
  // Text
  ExpectWrite("\"m\""_sv, DataItem{Text{"\"m\""_sv, 1}});

  // NumericData
  ExpectWrite("(i32 0)"_sv,
              DataItem{NumericData{NumericDataType::I32,
                                   ToBuffer("\x00\x00\x00\x00"_su8)}});
}

TEST(TextWriteTest, DataItemList) {
  ExpectWrite(
      "\"m\" (i32 0)"_sv,
      DataItemList{DataItem{Text{"\"m\""_sv, 1}},
                   DataItem{NumericData{NumericDataType::I32,
                                        ToBuffer("\x00\x00\x00\x00"_su8)}}});
}

TEST(TextWriteTest, Memory) {
  // Simplest memory.
  ExpectWrite("(memory 0)"_sv,
              Memory{MemoryDesc{{}, MemoryType{Limits{u32{0}}}}, {}});

  // Name.
  ExpectWrite("(memory $m 0)"_sv,
              Memory{MemoryDesc{"$m"_sv, MemoryType{Limits{u32{0}}}}, {}});

  // Inline export.
  ExpectWrite("(memory (export \"m\") 0)"_sv,
              Memory{MemoryDesc{{}, MemoryType{Limits{u32{0}}}},
                     InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});

  // Name and inline export.
  ExpectWrite("(memory $t (export \"m\") 0)"_sv,
              Memory{MemoryDesc{"$t"_sv, MemoryType{Limits{u32{0}}}},
                     InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});

  // Inline data segment.
  ExpectWrite("(memory (data \"hello\" \"world\"))"_sv,
              Memory{MemoryDesc{{}, MemoryType{Limits{u32{10}, u32{10}}}},
                     {},
                     DataItemList{
                         DataItem{Text{"\"hello\""_sv, 5}},
                         DataItem{Text{"\"world\""_sv, 5}},
                     }});
}

TEST(TextWriteTest, MemoryInlineImport) {
  // Inline import.
  ExpectWrite("(memory (import \"m\" \"n\") 0)"_sv,
              Memory{MemoryDesc{{}, MemoryType{Limits{u32{0}}}},
                     InlineImport{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1}},
                     {}});

  // Everything for Memory import.
  ExpectWrite("(memory $t (export \"m\") (import \"a\" \"b\") 0)"_sv,
              Memory{MemoryDesc{"$t"_sv, MemoryType{Limits{u32{0}}}},
                     InlineImport{Text{"\"a\""_sv, 1}, Text{"\"b\""_sv, 1}},
                     InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});
}

TEST(TextWriteTest, Global) {
  // Simplest global.
  ExpectWrite("(global i32 nop)"_sv,
              Global{GlobalDesc{{}, GlobalType{VT_I32, Mutability::Const}},
                     ConstantExpression{I{O::Nop}},
                     {}});

  // Name.
  ExpectWrite("(global $g i32 nop)"_sv,
              Global{GlobalDesc{"$g"_sv, GlobalType{VT_I32, Mutability::Const}},
                     ConstantExpression{I{O::Nop}},
                     {}});

  // Inline export.
  ExpectWrite("(global (export \"m\") i32 nop)"_sv,
              Global{GlobalDesc{{}, GlobalType{VT_I32, Mutability::Const}},
                     ConstantExpression{I{O::Nop}},
                     InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});

  // Name and inline export.
  ExpectWrite(
      "(global $g2 (export \"m\") i32 nop)"_sv,
      Global{GlobalDesc{"$g2"_sv, GlobalType{VT_I32, Mutability::Const}},
             ConstantExpression{I{O::Nop}},
             InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});
}

TEST(TextWriteTest, GlobalInlineImport) {
  // Inline import.
  ExpectWrite("(global (import \"m\" \"n\") i32)"_sv,
              Global{GlobalDesc{{}, GlobalType{VT_I32, Mutability::Const}},
                     InlineImport{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1}},
                     {}});

  // Everything for Global import.
  ExpectWrite("(global $g (export \"m\") (import \"a\" \"b\") i32)"_sv,
              Global{GlobalDesc{"$g"_sv, GlobalType{VT_I32, Mutability::Const}},
                     InlineImport{Text{"\"a\""_sv, 1}, Text{"\"b\""_sv, 1}},
                     InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});
}

TEST(TextWriteTest, Export) {
  // Function.
  ExpectWrite(
      "(export \"m\" (func 0))"_sv,
      Export{ExternalKind::Function, Text{"\"m\""_sv, 1}, Var{Index{0}}});

  // Table.
  ExpectWrite("(export \"m\" (table 0))"_sv,
              Export{ExternalKind::Table, Text{"\"m\""_sv, 1}, Var{Index{0}}});

  // Memory.
  ExpectWrite("(export \"m\" (memory 0))"_sv,
              Export{ExternalKind::Memory, Text{"\"m\""_sv, 1}, Var{Index{0}}});

  // Global.
  ExpectWrite("(export \"m\" (global 0))"_sv,
              Export{ExternalKind::Global, Text{"\"m\""_sv, 1}, Var{Index{0}}});

  // Event.
  ExpectWrite("(export \"m\" (event 0))"_sv,
              Export{ExternalKind::Event, Text{"\"m\""_sv, 1}, Var{Index{0}}});
}

TEST(TextWriteTest, Start) {
  ExpectWrite("(start 0)"_sv, Start{Var{Index{0}}});
}

TEST(TextWriteTest, ElementSegment) {
  // No table var, empty var list.
  ExpectWrite("(elem (offset nop))"_sv,
              ElementSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}},
                             ElementListWithVars{ExternalKind::Function, {}}});

  // No table var, var list.
  ExpectWrite(
      "(elem (offset nop) 0 1 2)"_sv,
      ElementSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}},
                     ElementListWithVars{ExternalKind::Function,
                                         VarList{Var{Index{0}}, Var{Index{1}},
                                                 Var{Index{2}}}}});

  // Table var.
  ExpectWrite(
      "(elem (table 0) (offset nop) func)"_sv,
      ElementSegment{nullopt, Var{Index{0}}, ConstantExpression{I{O::Nop}},
                     ElementListWithVars{ExternalKind::Function, {}}});

  // Table var as Id.
  ExpectWrite(
      "(elem (table $t) (offset nop) func)"_sv,
      ElementSegment{nullopt, Var{"$t"_sv}, ConstantExpression{I{O::Nop}},
                     ElementListWithVars{ExternalKind::Function, {}}});

  // Passive, w/ expression list.
  ExpectWrite("(elem funcref (nop) (nop))"_sv,
              ElementSegment{nullopt, SegmentType::Passive,
                             ElementListWithExpressions{
                                 RT_Funcref, ElementExpressionList{
                                                 ElementExpression{I{O::Nop}},
                                                 ElementExpression{I{O::Nop}},
                                             }}});

  // Passive, w/ var list.
  ExpectWrite("(elem func 0 $e)"_sv,
              ElementSegment{
                  nullopt, SegmentType::Passive,
                  ElementListWithVars{ExternalKind::Function, VarList{
                                                                  Var{Index{0}},
                                                                  Var{"$e"_sv},
                                                              }}});

  // Passive w/ name.
  ExpectWrite("(elem $e func)"_sv,
              ElementSegment{"$e"_sv, SegmentType::Passive,
                             ElementListWithVars{ExternalKind::Function, {}}});

  // Declared, w/ expression list.
  ExpectWrite("(elem declare funcref (nop) (nop))"_sv,
              ElementSegment{nullopt, SegmentType::Declared,
                             ElementListWithExpressions{
                                 RT_Funcref, ElementExpressionList{
                                                 ElementExpression{I{O::Nop}},
                                                 ElementExpression{I{O::Nop}},
                                             }}});

  // Declared, w/ var list.
  ExpectWrite("(elem declare func 0 $e)"_sv,
              ElementSegment{
                  nullopt, SegmentType::Declared,
                  ElementListWithVars{ExternalKind::Function, VarList{
                                                                  Var{Index{0}},
                                                                  Var{"$e"_sv},
                                                              }}});

  // Declared w/ name.
  ExpectWrite("(elem $e2 declare func)"_sv,
              ElementSegment{"$e2"_sv, SegmentType::Declared,
                             ElementListWithVars{ExternalKind::Function, {}}});

  // Active legacy, empty
  ExpectWrite(
      "(elem (offset nop))"_sv,
      ElementSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}}, {}});

  // Active legacy (i.e. no element type or external kind).
  ExpectWrite("(elem (offset nop) 0 $e)"_sv,
              ElementSegment{
                  nullopt, nullopt, ConstantExpression{I{O::Nop}},
                  ElementListWithVars{ExternalKind::Function, VarList{
                                                                  Var{Index{0}},
                                                                  Var{"$e"_sv},
                                                              }}});

  // Active, w/ expression list.
  ExpectWrite("(elem (offset nop) funcref (nop) (nop))"_sv,
              ElementSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}},
                             ElementListWithExpressions{
                                 RT_Funcref, ElementExpressionList{
                                                 ElementExpression{I{O::Nop}},
                                                 ElementExpression{I{O::Nop}},
                                             }}});

  // Active w/ table use.
  ExpectWrite("(elem (table 0) (offset nop) func 1)"_sv,
              ElementSegment{
                  nullopt, Var{Index{0}}, ConstantExpression{I{O::Nop}},
                  ElementListWithVars{ExternalKind::Function, VarList{
                                                                  Var{Index{1}},
                                                              }}});

  // Active w/ name.
  ExpectWrite("(elem $e3 (offset nop) func)"_sv,
              ElementSegment{"$e3"_sv, nullopt, ConstantExpression{I{O::Nop}},
                             ElementListWithVars{ExternalKind::Function, {}}});
}

TEST(TextWriteTest, DataSegment) {
  // No memory var, empty text list.
  ExpectWrite("(data (offset nop))"_sv,
              DataSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}}, {}});

  // No memory var, text list.
  ExpectWrite("(data (offset nop) \"hi\")"_sv,
              DataSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}},
                          DataItemList{DataItem{Text{"\"hi\""_sv, 2}}}});

  // Memory var.
  ExpectWrite(
      "(data (memory 0) (offset nop))"_sv,
      DataSegment{nullopt, Var{Index{0}}, ConstantExpression{I{O::Nop}}, {}});

  // Memory var as Id.
  ExpectWrite(
      "(data (memory $m) (offset nop))"_sv,
      DataSegment{nullopt, Var{"$m"_sv}, ConstantExpression{I{O::Nop}}, {}});

  // Passive, w/ text list.
  ExpectWrite("(data \"hi\")"_sv,
              DataSegment{nullopt, DataItemList{
                                       DataItem{Text{"\"hi\""_sv, 2}},
                                   }});

  // Passive w/ name.
  ExpectWrite("(data $d)"_sv, DataSegment{"$d"_sv, {}});

  // Active, w/ text list.
  ExpectWrite("(data (offset nop) \"hi\")"_sv,
              DataSegment{nullopt, nullopt, ConstantExpression{I{O::Nop}},
                          DataItemList{
                              DataItem{Text{"\"hi\""_sv, 2}},
                          }});

  // Active w/ memory use.
  ExpectWrite("(data (memory 0) (offset nop) \"hi\")"_sv,
              DataSegment{nullopt, Var{Index{0}}, ConstantExpression{I{O::Nop}},
                          DataItemList{
                              DataItem{Text{"\"hi\""_sv, 2}},
                          }});

  // Active w/ name.
  ExpectWrite(
      "(data $d2 (offset nop))"_sv,
      DataSegment{"$d2"_sv, nullopt, ConstantExpression{I{O::Nop}}, {}});
}

TEST(TextWriteTest, Event) {
  // Simplest event.
  ExpectWrite("(event)"_sv, Event{});

  // Name.
  ExpectWrite("(event $e)"_sv, Event{EventDesc{"$e"_sv, {}}, {}});

  // Inline export.
  ExpectWrite(
      "(event (export \"m\"))"_sv,
      Event{EventDesc{}, InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});

  // Name and inline export.
  ExpectWrite("(event $e2 (export \"m\"))"_sv,
              Event{EventDesc{"$e2"_sv, {}},
                    InlineExportList{InlineExport{Text{"\"m\""_sv, 1}}}});
}

TEST(TextWriteTest, ModuleItem) {
  // Type.
  ExpectWrite("(type (func))"_sv,
              ModuleItem{DefinedType{nullopt, BoundFunctionType{}}});

  // Import.
  ExpectWrite("(import \"m\" \"n\" (func))"_sv,
              ModuleItem{Import{Text{"\"m\""_sv, 1}, Text{"\"n\""_sv, 1},
                                FunctionDesc{}}});

  // Func.
  ExpectWrite("(func)"_sv, ModuleItem{Function{}});

  // Table.
  ExpectWrite(
      "(table 0 funcref)"_sv,
      ModuleItem{Table{
          TableDesc{nullopt, TableType{Limits{u32{0}}, RT_Funcref}}, {}}});

  // Memory.
  ExpectWrite(
      "(memory 0)"_sv,
      ModuleItem{Memory{MemoryDesc{nullopt, MemoryType{Limits{u32{0}}}}, {}}});

  // Global.
  ExpectWrite("(global i32 nop)"_sv,
              ModuleItem{Global{
                  GlobalDesc{nullopt, GlobalType{VT_I32, Mutability::Const}},
                  ConstantExpression{Instruction{Opcode::Nop}},
                  {}}});

  // Export.
  ExpectWrite("(export \"m\" (func 0))"_sv, ModuleItem{Export{
                                                ExternalKind::Function,
                                                Text{"\"m\""_sv, 1},
                                                Var{Index{0}},
                                            }});

  // Start.
  ExpectWrite("(start 0)"_sv, ModuleItem{Start{Var{Index{0}}}});

  // Elem.
  ExpectWrite(
      "(elem (offset nop))"_sv,
      ModuleItem{ElementSegment{
          nullopt, nullopt, ConstantExpression{Instruction{Opcode::Nop}}, {}}});

  // Data.
  ExpectWrite(
      "(data (offset nop))"_sv,
      ModuleItem{DataSegment{
          nullopt, nullopt, ConstantExpression{Instruction{Opcode::Nop}}, {}}});

  // Event.
  ExpectWrite("(event)"_sv,
              ModuleItem{Event{
                  EventDesc{nullopt, EventType{EventAttribute::Exception,
                                               FunctionTypeUse{nullopt, {}}}},
                  {}}});
}

TEST(TextWriteTest, Module) {
  ExpectWrite(
      "(type (func))\n(func\n  nop)\n(start 0)"_sv,
      Module{ModuleItem{DefinedType{nullopt, BoundFunctionType{}}},
             ModuleItem{Function{FunctionDesc{},
                                 {},
                                 InstructionList{Instruction{Opcode::Nop}},
                                 {}}},
             ModuleItem{Start{Var{Index{0}}}}});
}

TEST(TextWriteTest, ScriptModule) {
  // Text module.
  ExpectWrite("(module)"_sv,
              ScriptModule{nullopt, ScriptModuleKind::Text, Module{}});

  // Binary module.
  ExpectWrite("(module binary \"\")"_sv,
              ScriptModule{nullopt, ScriptModuleKind::Binary,
                           TextList{Text{"\"\""_sv, 0}}});

  // Quote module.
  ExpectWrite("(module quote \"\")"_sv,
              ScriptModule{nullopt, ScriptModuleKind::Quote,
                           TextList{Text{"\"\""_sv, 0}}});

  // Text module w/ Name.
  ExpectWrite("(module $m)"_sv,
              ScriptModule{"$m"_sv, ScriptModuleKind::Text, Module{}});

  // Binary module w/ Name.
  ExpectWrite("(module $m binary \"\")"_sv,
              ScriptModule{"$m"_sv, ScriptModuleKind::Binary,
                           TextList{Text{"\"\""_sv, 0}}});

  // Quote module w/ Name.
  ExpectWrite("(module $m quote \"\")"_sv,
              ScriptModule{"$m"_sv, ScriptModuleKind::Quote,
                           TextList{Text{"\"\""_sv, 0}}});
}

TEST(TextWriteTest, Const) {
  // i32.const
  ExpectWrite("(i32.const 0)"_sv, Const{u32{0}});

  // i64.const
  ExpectWrite("(i64.const 0)"_sv, Const{u64{0}});

  // f32.const
  ExpectWrite("(f32.const 0)"_sv, Const{f32{0}});

  // f64.const
  ExpectWrite("(f64.const 0)"_sv, Const{f64{0}});

  // v128.const
  ExpectWrite("(v128.const i32x4 0 0 0 0)"_sv, Const{v128{}});

  // ref.null
  ExpectWrite("(ref.null)"_sv, Const{RefNullConst{HT_Func}});

  // ref.extern 0
  ExpectWrite("(ref.extern 0)"_sv, Const{RefExternConst{u32{0}}});
}

TEST(TextWriteTest, ConstList) {
  ExpectWrite(""_sv, ConstList{});

  ExpectWrite("(i32.const 0) (i64.const 1)"_sv, ConstList{
                                                    Const{u32{0}},
                                                    Const{u64{1}},
                                                });
}

TEST(TextWriteTest, InvokeAction) {
  // Name.
  ExpectWrite("(invoke \"a\")"_sv,
              InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}});

  // Module.
  ExpectWrite("(invoke $m \"a\")"_sv,
              InvokeAction{"$m"_sv, Text{"\"a\""_sv, 1}, {}});

  // Const list.
  ExpectWrite(
      "(invoke \"a\" (i32.const 0))"_sv,
      InvokeAction{nullopt, Text{"\"a\""_sv, 1}, ConstList{Const{u32{0}}}});
}

TEST(TextWriteTest, GetAction) {
  // Name.
  ExpectWrite("(get \"a\")"_sv, GetAction{nullopt, Text{"\"a\""_sv, 1}});

  // Module.
  ExpectWrite("(get $m \"a\")"_sv, GetAction{"$m"_sv, Text{"\"a\""_sv, 1}});
}

TEST(TextWriteTest, Action) {
  // Get action.
  ExpectWrite("(get \"a\")"_sv,
              Action{GetAction{nullopt, Text{"\"a\""_sv, 1}}});

  // Invoke action.
  ExpectWrite("(invoke \"a\")"_sv,
              Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}});
}

TEST(TextWriteTest, ModuleAssertion) {
  ExpectWrite(
      "(module)\n\"msg\""_sv,
      ModuleAssertion{ScriptModule{nullopt, ScriptModuleKind::Text, Module{}},
                      Text{"\"msg\"", 3}});
}

TEST(TextWriteTest, ActionAssertion) {
  ExpectWrite(
      "(invoke \"a\") \"msg\""_sv,
      ActionAssertion{Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}},
                      Text{"\"msg\"", 3}});
}

TEST(TextWriteTest, FloatResult) {
  ExpectWrite("0"_sv, F32Result{f32{0}});
  ExpectWrite("nan:arithmetic"_sv, F32Result{NanKind::Arithmetic});
  ExpectWrite("nan:canonical"_sv, F32Result{NanKind::Canonical});

  ExpectWrite("0"_sv, F64Result{f64{0}});
  ExpectWrite("nan:arithmetic"_sv, F64Result{NanKind::Arithmetic});
  ExpectWrite("nan:canonical"_sv, F64Result{NanKind::Canonical});
}

TEST(TextWriteTest, SimdFloatResult) {
  ExpectWrite("0 0 0 0"_sv, F32x4Result{
                                F32Result{f32{0}},
                                F32Result{f32{0}},
                                F32Result{f32{0}},
                                F32Result{f32{0}},
                            });

  ExpectWrite("0 nan:arithmetic 0 nan:canonical"_sv,
              F32x4Result{
                  F32Result{f32{0}},
                  F32Result{NanKind::Arithmetic},
                  F32Result{f32{0}},
                  F32Result{NanKind::Canonical},
              });

  ExpectWrite("0 0"_sv, F64x2Result{
                            F64Result{f64{0}},
                            F64Result{f64{0}},
                        });

  ExpectWrite("nan:arithmetic 0"_sv, F64x2Result{
                                         F64Result{NanKind::Arithmetic},
                                         F64Result{f64{0}},
                                     });
}

TEST(TextWriteTest, ReturnResult) {
  // MVP
  ExpectWrite("(i32.const 0)"_sv, ReturnResult{u32{0}});

  ExpectWrite("(i64.const 0)"_sv, ReturnResult{u64{0}});

  ExpectWrite("(f32.const 0)"_sv, ReturnResult{F32Result{f32{0}}});
  ExpectWrite("(f32.const nan:arithmetic)"_sv,
              ReturnResult{F32Result{NanKind::Arithmetic}});
  ExpectWrite("(f32.const nan:canonical)"_sv,
              ReturnResult{F32Result{NanKind::Canonical}});

  ExpectWrite("(f64.const 0)"_sv, ReturnResult{F64Result{f64{0}}});
  ExpectWrite("(f64.const nan:arithmetic)"_sv,
              ReturnResult{F64Result{NanKind::Arithmetic}});
  ExpectWrite("(f64.const nan:canonical)"_sv,
              ReturnResult{F64Result{NanKind::Canonical}});

  // simd
  ExpectWrite("(v128.const i32x4 0 0 0 0)"_sv, ReturnResult{v128{}});
  ExpectWrite("(v128.const f32x4 0 0 0 0)"_sv, ReturnResult{F32x4Result{}});
  ExpectWrite("(v128.const f64x2 0 0)"_sv, ReturnResult{F64x2Result{}});

  ExpectWrite("(v128.const f32x4 0 nan:arithmetic 0 nan:canonical)"_sv,
              ReturnResult{F32x4Result{
                  F32Result{f32{0}},
                  F32Result{NanKind::Arithmetic},
                  F32Result{f32{0}},
                  F32Result{NanKind::Canonical},
              }});

  ExpectWrite("(v128.const f64x2 0 nan:arithmetic)"_sv,
              ReturnResult{F64x2Result{
                  F64Result{f64{0}},
                  F64Result{NanKind::Arithmetic},
              }});

  // reference-types
  ExpectWrite("(ref.null)"_sv, ReturnResult{RefNullConst{HT_Func}});
  ExpectWrite("(ref.extern 0)"_sv, ReturnResult{RefExternConst{u32{0}}});
  ExpectWrite("(ref.extern)"_sv, ReturnResult{RefExternResult{}});
  ExpectWrite("(ref.func)"_sv, ReturnResult{RefFuncResult{}});
}

TEST(TextWriteTest, ReturnResultList) {
  ExpectWrite(""_sv, ReturnResultList{});

  ExpectWrite("(i32.const 0) (f32.const nan:canonical)"_sv,
              ReturnResultList{
                  ReturnResult{u32{0}},
                  ReturnResult{F32Result{NanKind::Canonical}},
              });
}

TEST(TextWriteTest, ReturnAssertion) {
  ExpectWrite("(invoke \"a\")"_sv,
              ReturnAssertion{
                  Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}, {}});

  ExpectWrite("(invoke \"a\" (i32.const 0)) (i32.const 1)"_sv,
              ReturnAssertion{Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1},
                                                  ConstList{Const{u32{0}}}}},
                              ReturnResultList{ReturnResult{u32{1}}}});
}

TEST(TextWriteTest, Assertion) {
  // assert_malformed
  ExpectWrite("(assert_malformed\n  (module)\n  \"msg\")"_sv,
              Assertion{AssertionKind::Malformed,
                        ModuleAssertion{
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}},
                            Text{"\"msg\"", 3}}});

  // assert_invalid
  ExpectWrite("(assert_invalid\n  (module)\n  \"msg\")"_sv,
              Assertion{AssertionKind::Invalid,
                        ModuleAssertion{
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}},
                            Text{"\"msg\"", 3}}});

  // assert_unlinkable
  ExpectWrite("(assert_unlinkable\n  (module)\n  \"msg\")"_sv,
              Assertion{AssertionKind::Unlinkable,
                        ModuleAssertion{
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}},
                            Text{"\"msg\"", 3}}});

  // assert_trap (module)
  ExpectWrite("(assert_trap\n  (module)\n  \"msg\")"_sv,
              Assertion{AssertionKind::ModuleTrap,
                        ModuleAssertion{
                            ScriptModule{nullopt, ScriptModuleKind::Text, {}},
                            Text{"\"msg\"", 3}}});

  // assert_return
  ExpectWrite(
      "(assert_return (invoke \"a\"))"_sv,
      Assertion{
          AssertionKind::Return,
          ReturnAssertion{
              Action{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}}, {}}});

  // assert_trap (action)
  ExpectWrite("(assert_trap (invoke \"a\") \"msg\")"_sv,
              Assertion{AssertionKind::ActionTrap,
                        ActionAssertion{Action{InvokeAction{
                                            nullopt, Text{"\"a\""_sv, 1}, {}}},
                                        Text{"\"msg\""_sv, 3}}});

  // assert_exhaustion
  ExpectWrite("(assert_exhaustion (invoke \"a\") \"msg\")"_sv,
              Assertion{AssertionKind::Exhaustion,
                        ActionAssertion{Action{InvokeAction{
                                            nullopt, Text{"\"a\""_sv, 1}, {}}},
                                        Text{"\"msg\""_sv, 3}}});
}

TEST(TextWriteTest, Register) {
  ExpectWrite("(register \"a\")"_sv, Register{Text{"\"a\""_sv, 1}, nullopt});

  ExpectWrite("(register \"a\" $m)"_sv, Register{Text{"\"a\""_sv, 1}, "$m"_sv});
}


TEST(TextWriteTest, Command) {
  // Module.
  ExpectWrite("(module)"_sv,
              Command{ScriptModule{nullopt, ScriptModuleKind::Text, {}}});

  // Action.
  ExpectWrite("(invoke \"a\")"_sv,
              Command{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}});

  // Assertion.
  ExpectWrite(
      "(assert_invalid\n  (module)\n  \"msg\")"_sv,
      Command{Assertion{
          AssertionKind::Invalid,
          ModuleAssertion{ScriptModule{nullopt, ScriptModuleKind::Text, {}},
                          Text{"\"msg\"", 3}}}});

  // Register.
  ExpectWrite("(register \"a\")"_sv,
              Command{Register{Text{"\"a\""_sv, 1}, nullopt}});
}

TEST(TextWriteTest, Script) {
  ExpectWrite(
      "(module)\n(invoke \"a\")\n(assert_invalid\n  (module)\n  \"msg\")"_sv,
      Script{
          Command{ScriptModule{nullopt, ScriptModuleKind::Text, {}}},
          Command{InvokeAction{nullopt, Text{"\"a\""_sv, 1}, {}}},
          Command{Assertion{
              AssertionKind::Invalid,
              ModuleAssertion{ScriptModule{nullopt, ScriptModuleKind::Text, {}},
                              Text{"\"msg\"", 3}}}},
      });
}
