//
// Copyright 2019 WebAssembly Community Group participants
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

#include <cmath>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "test/binary/test_utils.h"
#include "test/write_test_utils.h"
#include "wasp/base/buffer.h"
#include "wasp/binary/name_section/write.h"
#include "wasp/binary/write.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

using I = Instruction;
using O = Opcode;

namespace {

template <typename T>
void ExpectWrite(SpanU8 expected, const T& value) {
  Buffer result(expected.size());
  auto iter =
      binary::Write(value, MakeClampedIterator(result.begin(), result.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

}  // namespace

TEST(BinaryWriteTest, BlockType) {
  ExpectWrite("\x7f"_su8, BlockType::I32);
  ExpectWrite("\x7e"_su8, BlockType::I64);
  ExpectWrite("\x7d"_su8, BlockType::F32);
  ExpectWrite("\x7c"_su8, BlockType::F64);
  ExpectWrite("\x7b"_su8, BlockType::V128);
  ExpectWrite("\x6f"_su8, BlockType::Externref);
  ExpectWrite("\x40"_su8, BlockType::Void);
}

TEST(BinaryWriteTest, BrOnExnImmediate) {
  ExpectWrite("\x00\x00"_su8, BrOnExnImmediate{0, 0});
}

TEST(BinaryWriteTest, BrTableImmediate) {
  ExpectWrite("\x00\x00"_su8, BrTableImmediate{{}, 0});
  ExpectWrite("\x02\x01\x02\x03"_su8, BrTableImmediate{{1, 2}, 3});
}

TEST(BinaryWriteTest, Bytes) {
  const Buffer input{{0x12, 0x34, 0x56}};
  Buffer output;
  WriteBytes(input, std::back_inserter(output));
  EXPECT_EQ(input, output);
}

TEST(BinaryWriteTest, CallIndirectImmediate) {
  ExpectWrite("\x01\x00"_su8, CallIndirectImmediate{1, 0});
  ExpectWrite("\x80\x01\x00"_su8, CallIndirectImmediate{128, 0});
}

TEST(BinaryWriteTest, Code) {
  ExpectWrite(
      "\x09"               // code size
      "\x02"               // 2 locals
      "\x02\x7f"           // locals[0]: i32 * 2
      "\x80\x01\x7e"       // locals[1]: i64 * 128
      "\x01\x02\x03"_su8,  // code
      Code{LocalsList{
               Locals{2, ValueType::I32},
               Locals{128, ValueType::I64},
           },
           Expression{"\x01\x02\x03"_su8}});
}

TEST(BinaryWriteTest, ConstantExpression) {
  // i32.const
  ExpectWrite("\x41\x00\x0b"_su8,
              ConstantExpression{Instruction{Opcode::I32Const, s32{0}}});

  // i64.const
  ExpectWrite(
      "\x42\x80\x80\x80\x80\x80\x01\x0b"_su8,
      ConstantExpression{Instruction{Opcode::I64Const, s64{34359738368}}});

  // f32.const
  ExpectWrite("\x43\x00\x00\x00\x00\x0b"_su8,
              ConstantExpression{Instruction{Opcode::F32Const, f32{0}}});

  // f64.const
  ExpectWrite("\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b"_su8,
              ConstantExpression{Instruction{Opcode::F64Const, f64{0}}});

  // global.get
  ExpectWrite("\x23\x00\x0b"_su8,
              ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}});
}

TEST(BinaryWriteTest, CopyImmediate) {
  ExpectWrite("\x00\x00"_su8, CopyImmediate{0, 0});
}

TEST(BinaryWriteTest, DataCount) {
  ExpectWrite("\x0d"_su8, DataCount{13});
}

TEST(BinaryWriteTest, DataSegment) {
  ExpectWrite(
      "\x00\x42\x01\x0b\x04wxyz"_su8,
      DataSegment{0, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  "wxyz"_su8});
}

TEST(BinaryWriteTest, DataSegment_BulkMemory) {
  // Active data segment with non-zero memory index.
  ExpectWrite(
      "\x02\x01\x42\x01\x0b\x04wxyz"_su8,
      DataSegment{1, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  "wxyz"_su8});

  // Passive data segment.
  ExpectWrite("\x01\x04wxyz"_su8, DataSegment{"wxyz"_su8});
}

TEST(BinaryWriteTest, ElementExpression) {
  // ref.null
  ExpectWrite(
      "\xd0\x70\x0b"_su8,
      ElementExpression{Instruction{Opcode::RefNull, ReferenceType::Funcref}});

  // ref.func 2
  ExpectWrite("\xd2\x02\x0b"_su8,
              ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}});
}

TEST(BinaryWriteTest, ElementSegment) {
  ExpectWrite("\x00\x41\x01\x0b\x03\x01\x02\x03"_su8,
              ElementSegment{
                  0, ConstantExpression{Instruction{Opcode::I32Const, s32{1}}},
                  ElementListWithIndexes{ExternalKind::Function, {1, 2, 3}}});
}

TEST(BinaryWriteTest, ElementSegment_BulkMemory) {
  // Flags == 1: Passive, index list
  ExpectWrite(
      "\x01\x00\x02\x01\x02"_su8,
      ElementSegment{SegmentType::Passive,
                     ElementListWithIndexes{ExternalKind::Function, {1, 2}}});

  // Flags == 2: Active, table index, index list
  ExpectWrite("\x02\x01\x41\x02\x0b\x00\x02\x03\x04"_su8,
              ElementSegment{
                  1u, ConstantExpression{Instruction{Opcode::I32Const, s32{2}}},
                  ElementListWithIndexes{ExternalKind::Function, {3, 4}}});

  // Flags == 4: Active (function only), table 0, expression list
  ExpectWrite(
      "\x04\x41\x05\x0b\x01\xd2\x06\x0b"_su8,
      ElementSegment{
          0u, ConstantExpression{Instruction{Opcode::I32Const, s32{5}}},
          ElementListWithExpressions{
              ReferenceType::Funcref,
              {ElementExpression{Instruction{Opcode::RefFunc, Index{6u}}}}}});

  // Flags == 5: Passive, expression list
  ExpectWrite(
      "\x05\x70\x02\xd2\x07\x0b\xd0\x70\x0b"_su8,
      ElementSegment{
          SegmentType::Passive,
          ElementListWithExpressions{
              ReferenceType::Funcref,
              {ElementExpression{Instruction{Opcode::RefFunc, Index{7u}}},
               ElementExpression{
                   Instruction{Opcode::RefNull, ReferenceType::Funcref}}}}});

  // Flags == 6: Active, table index, expression list
  ExpectWrite("\x06\x02\x41\x08\x0b\x70\x01\xd0\x70\x0b"_su8,
              ElementSegment{
                  2u, ConstantExpression{Instruction{Opcode::I32Const, s32{8}}},
                  ElementListWithExpressions{
                      ReferenceType::Funcref,
                      {ElementExpression{Instruction{
                          Opcode::RefNull, ReferenceType::Funcref}}}}});
}

TEST(BinaryWriteTest, ReferenceType) {
  ExpectWrite("\x70"_su8, ReferenceType::Funcref);
}

TEST(BinaryWriteTest, Event) {
  ExpectWrite("\x00\x01"_su8,
                     Event{EventType{EventAttribute::Exception, 1}});
}

TEST(BinaryWriteTest, EventType) {
  ExpectWrite("\x00\x01"_su8,
                         EventType{EventAttribute::Exception, 1});
}

TEST(BinaryWriteTest, Export) {
  ExpectWrite("\x02hi\x00\x03"_su8, Export{ExternalKind::Function, "hi"_sv, 3});
  ExpectWrite("\x00\x01\xe8\x07"_su8, Export{ExternalKind::Table, ""_sv, 1000});
  ExpectWrite("\x03mem\x02\x00"_su8, Export{ExternalKind::Memory, "mem"_sv, 0});
  ExpectWrite("\x01g\x03\x01"_su8, Export{ExternalKind::Global, "g"_sv, 1});
  ExpectWrite("\x01v\x04\x02"_su8, Export{ExternalKind::Event, "v"_sv, 2});
}

TEST(BinaryWriteTest, ExternalKind) {
  ExpectWrite("\x00"_su8, ExternalKind::Function);
  ExpectWrite("\x01"_su8, ExternalKind::Table);
  ExpectWrite("\x02"_su8, ExternalKind::Memory);
  ExpectWrite("\x03"_su8, ExternalKind::Global);
  ExpectWrite("\x04"_su8, ExternalKind::Event);
}

TEST(BinaryWriteTest, F32) {
  ExpectWrite("\x00\x00\x00\x00"_su8, f32{0.0f});
  ExpectWrite("\x00\x00\x80\xbf"_su8, f32{-1.0f});
  ExpectWrite("\x38\xb4\x96\x49"_su8, f32{1234567.0f});
  ExpectWrite("\x00\x00\x80\x7f"_su8, f32{INFINITY});
  ExpectWrite("\x00\x00\x80\xff"_su8, f32{-INFINITY});
  // TODO: NaN
}

TEST(BinaryWriteTest, F64) {
  ExpectWrite("\x00\x00\x00\x00\x00\x00\x00\x00"_su8, f64{0.0});
  ExpectWrite("\x00\x00\x00\x00\x00\x00\xf0\xbf"_su8, f64{-1.0});
  ExpectWrite("\xc0\x71\xbc\x93\x84\x43\xd9\x42"_su8, f64{111111111111111});
  ExpectWrite("\x00\x00\x00\x00\x00\x00\xf0\x7f"_su8, f64{INFINITY});
  ExpectWrite("\x00\x00\x00\x00\x00\x00\xf0\xff"_su8, f64{-INFINITY});
  // TODO: NaN
}

namespace {

template <typename T>
void ExpectWriteFixedVarInt(SpanU8 expected, T value, size_t length) {
  Buffer result(expected.size());
  auto iter = binary::WriteFixedVarInt(
      value, MakeClampedIterator(result.begin(), result.end()), length);
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

}  // namespace

TEST(BinaryWriteTest, FixedVarInt_u32) {
  // Naturally 1 byte.
  ExpectWriteFixedVarInt<u32>("\x11"_su8, 0x11, 1);
  ExpectWriteFixedVarInt<u32>("\x91\x00"_su8, 0x11, 2);
  ExpectWriteFixedVarInt<u32>("\x91\x80\x00"_su8, 0x11, 3);
  ExpectWriteFixedVarInt<u32>("\x91\x80\x80\x00"_su8, 0x11, 4);
  ExpectWriteFixedVarInt<u32>("\x91\x80\x80\x80\x00"_su8, 0x11, 5);

  // Naturally 2 bytes.
  ExpectWriteFixedVarInt<u32>("\x91\x02"_su8, 0x111, 2);
  ExpectWriteFixedVarInt<u32>("\x91\x82\x00"_su8, 0x111, 3);
  ExpectWriteFixedVarInt<u32>("\x91\x82\x80\x00"_su8, 0x111, 4);
  ExpectWriteFixedVarInt<u32>("\x91\x82\x80\x80\x00"_su8, 0x111, 5);

  // Naturally 3 bytes.
  ExpectWriteFixedVarInt<u32>("\x91\xa2\x04"_su8, 0x11111, 3);
  ExpectWriteFixedVarInt<u32>("\x91\xa2\x84\x00"_su8, 0x11111, 4);
  ExpectWriteFixedVarInt<u32>("\x91\xa2\x84\x80\x00"_su8, 0x11111, 5);

  // Naturally 4 bytes.
  ExpectWriteFixedVarInt<u32>("\x91\xa2\xc4\x08"_su8, 0x1111111, 4);
  ExpectWriteFixedVarInt<u32>("\x91\xa2\xc4\x88\x00"_su8, 0x1111111, 5);

  // Naturally 5 bytes.
  ExpectWriteFixedVarInt<u32>("\x91\xa2\xc4\x88\x01"_su8, 0x11111111, 5);
}

TEST(BinaryWriteTest, FixedVarInt_s32) {
  // Naturally 1 byte, positive.
  ExpectWriteFixedVarInt<s32>("\x11"_su8, 0x11, 1);
  ExpectWriteFixedVarInt<s32>("\x91\x00"_su8, 0x11, 2);
  ExpectWriteFixedVarInt<s32>("\x91\x80\x00"_su8, 0x11, 3);
  ExpectWriteFixedVarInt<s32>("\x91\x80\x80\x00"_su8, 0x11, 4);
  ExpectWriteFixedVarInt<s32>("\x91\x80\x80\x80\x00"_su8, 0x11, 5);

  // Naturally 2 bytes, positive.
  ExpectWriteFixedVarInt<s32>("\x91\x02"_su8, 0x111, 2);
  ExpectWriteFixedVarInt<s32>("\x91\x82\x00"_su8, 0x111, 3);
  ExpectWriteFixedVarInt<s32>("\x91\x82\x80\x00"_su8, 0x111, 4);
  ExpectWriteFixedVarInt<s32>("\x91\x82\x80\x80\x00"_su8, 0x111, 5);

  // Naturally 3 bytes, positive.
  ExpectWriteFixedVarInt<s32>("\x91\xa2\x04"_su8, 0x11111, 3);
  ExpectWriteFixedVarInt<s32>("\x91\xa2\x84\x00"_su8, 0x11111, 4);
  ExpectWriteFixedVarInt<s32>("\x91\xa2\x84\x80\x00"_su8, 0x11111, 5);

  // Naturally 4 bytes, positive.
  ExpectWriteFixedVarInt<s32>("\x91\xa2\xc4\x08"_su8, 0x1111111, 4);
  ExpectWriteFixedVarInt<s32>("\x91\xa2\xc4\x88\x00"_su8, 0x1111111, 5);

  // Naturally 5 bytes, positive.
  ExpectWriteFixedVarInt<s32>("\x91\xa2\xc4\x88\x01"_su8, 0x11111111, 5);

  // Naturally 1 byte, negative.
  ExpectWriteFixedVarInt<s32>("\x6f"_su8, -0x11, 1);
  ExpectWriteFixedVarInt<s32>("\xef\x7f"_su8, -0x11, 2);
  ExpectWriteFixedVarInt<s32>("\xef\xff\x7f"_su8, -0x11, 3);
  ExpectWriteFixedVarInt<s32>("\xef\xff\xff\x7f"_su8, -0x11, 4);
  ExpectWriteFixedVarInt<s32>("\xef\xff\xff\xff\x7f"_su8, -0x11, 5);

  // Naturally 2 bytes, negative.
  ExpectWriteFixedVarInt<s32>("\xef\x7d"_su8, -0x111, 2);
  ExpectWriteFixedVarInt<s32>("\xef\xfd\x7f"_su8, -0x111, 3);
  ExpectWriteFixedVarInt<s32>("\xef\xfd\xff\x7f"_su8, -0x111, 4);
  ExpectWriteFixedVarInt<s32>("\xef\xfd\xff\xff\x7f"_su8, -0x111, 5);

  // Naturally 3 bytes, negative.
  ExpectWriteFixedVarInt<s32>("\xef\xdd\x7b"_su8, -0x11111, 3);
  ExpectWriteFixedVarInt<s32>("\xef\xdd\xfb\x7f"_su8, -0x11111, 4);
  ExpectWriteFixedVarInt<s32>("\xef\xdd\xfb\xff\x7f"_su8, -0x11111, 5);

  // Naturally 4 bytes, negative.
  ExpectWriteFixedVarInt<s32>("\xef\xdd\xbb\x77"_su8, -0x1111111, 4);
  ExpectWriteFixedVarInt<s32>("\xef\xdd\xbb\xf7\x7f"_su8, -0x1111111, 5);

  // Naturally 5 bytes, negative.
  ExpectWriteFixedVarInt<s32>("\xef\xdd\xbb\xf7\x7e"_su8, -0x11111111, 5);
}

TEST(BinaryWriteTest, Function) {
  ExpectWrite("\x01"_su8, Function{1});
}

TEST(BinaryWriteTest, FunctionType) {
  ExpectWrite("\x00\x00"_su8, FunctionType{{}, {}});
  ExpectWrite("\x02\x7f\x7e\x01\x7c"_su8,
              FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}});
}

TEST(BinaryWriteTest, Global) {
  ExpectWrite(
      "\x7f\x01\x41\x00\x0b"_su8,
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}});
}

TEST(BinaryWriteTest, GlobalType) {
  ExpectWrite("\x7f\x00"_su8, GlobalType{ValueType::I32, Mutability::Const});
  ExpectWrite("\x7d\x01"_su8, GlobalType{ValueType::F32, Mutability::Var});
}

TEST(BinaryWriteTest, Import) {
  ExpectWrite("\x01\x61\x04\x66unc\x00\x0b"_su8, Import{"a", "func", 11u});

  ExpectWrite(
      "\x01\x62\x05table\x01\x70\x00\x01"_su8,
      Import{"b", "table", TableType{Limits{1}, ReferenceType::Funcref}});

  ExpectWrite("\x01\x63\x06memory\x02\x01\x00\x02"_su8,
              Import{"c", "memory", MemoryType{Limits{0, 2}}});

  ExpectWrite(
      "\x01\x64\x06global\x03\x7f\x00"_su8,
      Import{"d", "global", GlobalType{ValueType::I32, Mutability::Const}});

  ExpectWrite("\x01v\x06!event\x04\x00\x02"_su8,
              Import{"v", "!event", EventType{EventAttribute::Exception, 2}});
}

TEST(BinaryWriteTest, InitImmediate) {
  ExpectWrite("\x01\x00"_su8, InitImmediate{1, 0});
  ExpectWrite("\x80\x01\x00"_su8, InitImmediate{128, 0});
}

TEST(BinaryWriteTest, Instruction) {
  using MemArg = MemArgImmediate;

  ExpectWrite("\x00"_su8, I{O::Unreachable});
  ExpectWrite("\x01"_su8, I{O::Nop});
  ExpectWrite("\x02\x7f"_su8, I{O::Block, BlockType::I32});
  ExpectWrite("\x03\x40"_su8, I{O::Loop, BlockType::Void});
  ExpectWrite("\x04\x7c"_su8, I{O::If, BlockType::F64});
  ExpectWrite("\x05"_su8, I{O::Else});
  ExpectWrite("\x0b"_su8, I{O::End});
  ExpectWrite("\x0c\x01"_su8, I{O::Br, Index{1}});
  ExpectWrite("\x0d\x02"_su8, I{O::BrIf, Index{2}});
  ExpectWrite("\x0e\x03\x03\x04\x05\x06"_su8,
              I{O::BrTable, BrTableImmediate{{3, 4, 5}, 6}});
  ExpectWrite("\x0f"_su8, I{O::Return});
  ExpectWrite("\x10\x07"_su8, I{O::Call, Index{7}});
  ExpectWrite("\x11\x08\x00"_su8,
              I{O::CallIndirect, CallIndirectImmediate{8, 0}});
  ExpectWrite("\x1a"_su8, I{O::Drop});
  ExpectWrite("\x1b"_su8, I{O::Select});
  ExpectWrite("\x20\x05"_su8, I{O::LocalGet, Index{5}});
  ExpectWrite("\x21\x06"_su8, I{O::LocalSet, Index{6}});
  ExpectWrite("\x22\x07"_su8, I{O::LocalTee, Index{7}});
  ExpectWrite("\x23\x08"_su8, I{O::GlobalGet, Index{8}});
  ExpectWrite("\x24\x09"_su8, I{O::GlobalSet, Index{9}});
  ExpectWrite("\x28\x0a\x0b"_su8, I{O::I32Load, MemArg{10, 11}});
  ExpectWrite("\x29\x0c\x0d"_su8, I{O::I64Load, MemArg{12, 13}});
  ExpectWrite("\x2a\x0e\x0f"_su8, I{O::F32Load, MemArg{14, 15}});
  ExpectWrite("\x2b\x10\x11"_su8, I{O::F64Load, MemArg{16, 17}});
  ExpectWrite("\x2c\x12\x13"_su8, I{O::I32Load8S, MemArg{18, 19}});
  ExpectWrite("\x2d\x14\x15"_su8, I{O::I32Load8U, MemArg{20, 21}});
  ExpectWrite("\x2e\x16\x17"_su8, I{O::I32Load16S, MemArg{22, 23}});
  ExpectWrite("\x2f\x18\x19"_su8, I{O::I32Load16U, MemArg{24, 25}});
  ExpectWrite("\x30\x1a\x1b"_su8, I{O::I64Load8S, MemArg{26, 27}});
  ExpectWrite("\x31\x1c\x1d"_su8, I{O::I64Load8U, MemArg{28, 29}});
  ExpectWrite("\x32\x1e\x1f"_su8, I{O::I64Load16S, MemArg{30, 31}});
  ExpectWrite("\x33\x20\x21"_su8, I{O::I64Load16U, MemArg{32, 33}});
  ExpectWrite("\x34\x22\x23"_su8, I{O::I64Load32S, MemArg{34, 35}});
  ExpectWrite("\x35\x24\x25"_su8, I{O::I64Load32U, MemArg{36, 37}});
  ExpectWrite("\x36\x26\x27"_su8, I{O::I32Store, MemArg{38, 39}});
  ExpectWrite("\x37\x28\x29"_su8, I{O::I64Store, MemArg{40, 41}});
  ExpectWrite("\x38\x2a\x2b"_su8, I{O::F32Store, MemArg{42, 43}});
  ExpectWrite("\x39\x2c\x2d"_su8, I{O::F64Store, MemArg{44, 45}});
  ExpectWrite("\x3a\x2e\x2f"_su8, I{O::I32Store8, MemArg{46, 47}});
  ExpectWrite("\x3b\x30\x31"_su8, I{O::I32Store16, MemArg{48, 49}});
  ExpectWrite("\x3c\x32\x33"_su8, I{O::I64Store8, MemArg{50, 51}});
  ExpectWrite("\x3d\x34\x35"_su8, I{O::I64Store16, MemArg{52, 53}});
  ExpectWrite("\x3e\x36\x37"_su8, I{O::I64Store32, MemArg{54, 55}});
  ExpectWrite("\x3f\x00"_su8, I{O::MemorySize, u8{0}});
  ExpectWrite("\x40\x00"_su8, I{O::MemoryGrow, u8{0}});
  ExpectWrite("\x41\x00"_su8, I{O::I32Const, s32{0}});
  ExpectWrite("\x42\x00"_su8, I{O::I64Const, s64{0}});
  ExpectWrite("\x43\x00\x00\x00\x00"_su8, I{O::F32Const, f32{0}});
  ExpectWrite("\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
              I{O::F64Const, f64{0}});
  ExpectWrite("\x45"_su8, I{O::I32Eqz});
  ExpectWrite("\x46"_su8, I{O::I32Eq});
  ExpectWrite("\x47"_su8, I{O::I32Ne});
  ExpectWrite("\x48"_su8, I{O::I32LtS});
  ExpectWrite("\x49"_su8, I{O::I32LtU});
  ExpectWrite("\x4a"_su8, I{O::I32GtS});
  ExpectWrite("\x4b"_su8, I{O::I32GtU});
  ExpectWrite("\x4c"_su8, I{O::I32LeS});
  ExpectWrite("\x4d"_su8, I{O::I32LeU});
  ExpectWrite("\x4e"_su8, I{O::I32GeS});
  ExpectWrite("\x4f"_su8, I{O::I32GeU});
  ExpectWrite("\x50"_su8, I{O::I64Eqz});
  ExpectWrite("\x51"_su8, I{O::I64Eq});
  ExpectWrite("\x52"_su8, I{O::I64Ne});
  ExpectWrite("\x53"_su8, I{O::I64LtS});
  ExpectWrite("\x54"_su8, I{O::I64LtU});
  ExpectWrite("\x55"_su8, I{O::I64GtS});
  ExpectWrite("\x56"_su8, I{O::I64GtU});
  ExpectWrite("\x57"_su8, I{O::I64LeS});
  ExpectWrite("\x58"_su8, I{O::I64LeU});
  ExpectWrite("\x59"_su8, I{O::I64GeS});
  ExpectWrite("\x5a"_su8, I{O::I64GeU});
  ExpectWrite("\x5b"_su8, I{O::F32Eq});
  ExpectWrite("\x5c"_su8, I{O::F32Ne});
  ExpectWrite("\x5d"_su8, I{O::F32Lt});
  ExpectWrite("\x5e"_su8, I{O::F32Gt});
  ExpectWrite("\x5f"_su8, I{O::F32Le});
  ExpectWrite("\x60"_su8, I{O::F32Ge});
  ExpectWrite("\x61"_su8, I{O::F64Eq});
  ExpectWrite("\x62"_su8, I{O::F64Ne});
  ExpectWrite("\x63"_su8, I{O::F64Lt});
  ExpectWrite("\x64"_su8, I{O::F64Gt});
  ExpectWrite("\x65"_su8, I{O::F64Le});
  ExpectWrite("\x66"_su8, I{O::F64Ge});
  ExpectWrite("\x67"_su8, I{O::I32Clz});
  ExpectWrite("\x68"_su8, I{O::I32Ctz});
  ExpectWrite("\x69"_su8, I{O::I32Popcnt});
  ExpectWrite("\x6a"_su8, I{O::I32Add});
  ExpectWrite("\x6b"_su8, I{O::I32Sub});
  ExpectWrite("\x6c"_su8, I{O::I32Mul});
  ExpectWrite("\x6d"_su8, I{O::I32DivS});
  ExpectWrite("\x6e"_su8, I{O::I32DivU});
  ExpectWrite("\x6f"_su8, I{O::I32RemS});
  ExpectWrite("\x70"_su8, I{O::I32RemU});
  ExpectWrite("\x71"_su8, I{O::I32And});
  ExpectWrite("\x72"_su8, I{O::I32Or});
  ExpectWrite("\x73"_su8, I{O::I32Xor});
  ExpectWrite("\x74"_su8, I{O::I32Shl});
  ExpectWrite("\x75"_su8, I{O::I32ShrS});
  ExpectWrite("\x76"_su8, I{O::I32ShrU});
  ExpectWrite("\x77"_su8, I{O::I32Rotl});
  ExpectWrite("\x78"_su8, I{O::I32Rotr});
  ExpectWrite("\x79"_su8, I{O::I64Clz});
  ExpectWrite("\x7a"_su8, I{O::I64Ctz});
  ExpectWrite("\x7b"_su8, I{O::I64Popcnt});
  ExpectWrite("\x7c"_su8, I{O::I64Add});
  ExpectWrite("\x7d"_su8, I{O::I64Sub});
  ExpectWrite("\x7e"_su8, I{O::I64Mul});
  ExpectWrite("\x7f"_su8, I{O::I64DivS});
  ExpectWrite("\x80"_su8, I{O::I64DivU});
  ExpectWrite("\x81"_su8, I{O::I64RemS});
  ExpectWrite("\x82"_su8, I{O::I64RemU});
  ExpectWrite("\x83"_su8, I{O::I64And});
  ExpectWrite("\x84"_su8, I{O::I64Or});
  ExpectWrite("\x85"_su8, I{O::I64Xor});
  ExpectWrite("\x86"_su8, I{O::I64Shl});
  ExpectWrite("\x87"_su8, I{O::I64ShrS});
  ExpectWrite("\x88"_su8, I{O::I64ShrU});
  ExpectWrite("\x89"_su8, I{O::I64Rotl});
  ExpectWrite("\x8a"_su8, I{O::I64Rotr});
  ExpectWrite("\x8b"_su8, I{O::F32Abs});
  ExpectWrite("\x8c"_su8, I{O::F32Neg});
  ExpectWrite("\x8d"_su8, I{O::F32Ceil});
  ExpectWrite("\x8e"_su8, I{O::F32Floor});
  ExpectWrite("\x8f"_su8, I{O::F32Trunc});
  ExpectWrite("\x90"_su8, I{O::F32Nearest});
  ExpectWrite("\x91"_su8, I{O::F32Sqrt});
  ExpectWrite("\x92"_su8, I{O::F32Add});
  ExpectWrite("\x93"_su8, I{O::F32Sub});
  ExpectWrite("\x94"_su8, I{O::F32Mul});
  ExpectWrite("\x95"_su8, I{O::F32Div});
  ExpectWrite("\x96"_su8, I{O::F32Min});
  ExpectWrite("\x97"_su8, I{O::F32Max});
  ExpectWrite("\x98"_su8, I{O::F32Copysign});
  ExpectWrite("\x99"_su8, I{O::F64Abs});
  ExpectWrite("\x9a"_su8, I{O::F64Neg});
  ExpectWrite("\x9b"_su8, I{O::F64Ceil});
  ExpectWrite("\x9c"_su8, I{O::F64Floor});
  ExpectWrite("\x9d"_su8, I{O::F64Trunc});
  ExpectWrite("\x9e"_su8, I{O::F64Nearest});
  ExpectWrite("\x9f"_su8, I{O::F64Sqrt});
  ExpectWrite("\xa0"_su8, I{O::F64Add});
  ExpectWrite("\xa1"_su8, I{O::F64Sub});
  ExpectWrite("\xa2"_su8, I{O::F64Mul});
  ExpectWrite("\xa3"_su8, I{O::F64Div});
  ExpectWrite("\xa4"_su8, I{O::F64Min});
  ExpectWrite("\xa5"_su8, I{O::F64Max});
  ExpectWrite("\xa6"_su8, I{O::F64Copysign});
  ExpectWrite("\xa7"_su8, I{O::I32WrapI64});
  ExpectWrite("\xa8"_su8, I{O::I32TruncF32S});
  ExpectWrite("\xa9"_su8, I{O::I32TruncF32U});
  ExpectWrite("\xaa"_su8, I{O::I32TruncF64S});
  ExpectWrite("\xab"_su8, I{O::I32TruncF64U});
  ExpectWrite("\xac"_su8, I{O::I64ExtendI32S});
  ExpectWrite("\xad"_su8, I{O::I64ExtendI32U});
  ExpectWrite("\xae"_su8, I{O::I64TruncF32S});
  ExpectWrite("\xaf"_su8, I{O::I64TruncF32U});
  ExpectWrite("\xb0"_su8, I{O::I64TruncF64S});
  ExpectWrite("\xb1"_su8, I{O::I64TruncF64U});
  ExpectWrite("\xb2"_su8, I{O::F32ConvertI32S});
  ExpectWrite("\xb3"_su8, I{O::F32ConvertI32U});
  ExpectWrite("\xb4"_su8, I{O::F32ConvertI64S});
  ExpectWrite("\xb5"_su8, I{O::F32ConvertI64U});
  ExpectWrite("\xb6"_su8, I{O::F32DemoteF64});
  ExpectWrite("\xb7"_su8, I{O::F64ConvertI32S});
  ExpectWrite("\xb8"_su8, I{O::F64ConvertI32U});
  ExpectWrite("\xb9"_su8, I{O::F64ConvertI64S});
  ExpectWrite("\xba"_su8, I{O::F64ConvertI64U});
  ExpectWrite("\xbb"_su8, I{O::F64PromoteF32});
  ExpectWrite("\xbc"_su8, I{O::I32ReinterpretF32});
  ExpectWrite("\xbd"_su8, I{O::I64ReinterpretF64});
  ExpectWrite("\xbe"_su8, I{O::F32ReinterpretI32});
  ExpectWrite("\xbf"_su8, I{O::F64ReinterpretI64});
}

TEST(BinaryWriteTest, Instruction_exceptions) {
  ExpectWrite("\x06\x40"_su8, I{O::Try, BlockType::Void});
  ExpectWrite("\x07"_su8, I{O::Catch});
  ExpectWrite("\x08\x00"_su8, I{O::Throw, Index{0}});
  ExpectWrite("\x09"_su8, I{O::Rethrow});
  ExpectWrite("\x0a\x01\x02"_su8, I{O::BrOnExn, BrOnExnImmediate{1, 2}});
}

TEST(BinaryWriteTest, Instruction_tail_call) {
  ExpectWrite("\x12\x00"_su8, I{O::ReturnCall, Index{0}});
  ExpectWrite("\x13\x08\x00"_su8,
                 I{O::ReturnCallIndirect, CallIndirectImmediate{8, 0}});
}

TEST(BinaryWriteTest, Instruction_sign_extension) {
  ExpectWrite("\xc0"_su8, I{O::I32Extend8S});
  ExpectWrite("\xc1"_su8, I{O::I32Extend16S});
  ExpectWrite("\xc2"_su8, I{O::I64Extend8S});
  ExpectWrite("\xc3"_su8, I{O::I64Extend16S});
  ExpectWrite("\xc4"_su8, I{O::I64Extend32S});
}

TEST(BinaryWriteTest, Instruction_reference_types) {
  ExpectWrite("\x1c\x02\x7f\x7e"_su8,
              I{O::SelectT, ValueTypeList{ValueType::I32, ValueType::I64}});
  ExpectWrite("\x25\x00"_su8, I{O::TableGet, Index{0}});
  ExpectWrite("\x26\x00"_su8, I{O::TableSet, Index{0}});
  ExpectWrite("\xfc\x0f\x00"_su8, I{O::TableGrow, Index{0}});
  ExpectWrite("\xfc\x10\x00"_su8, I{O::TableSize, Index{0}});
  ExpectWrite("\xfc\x11\x00"_su8, I{O::TableFill, Index{0}});
  ExpectWrite("\xd0\x70"_su8, I{O::RefNull, ReferenceType::Funcref});
  ExpectWrite("\xd1\x70"_su8, I{O::RefIsNull, ReferenceType::Funcref});
}

TEST(BinaryWriteTest, Instruction_function_references) {
  ExpectWrite("\xd2\x00"_su8, I{O::RefFunc, Index{0}});
}

TEST(BinaryWriteTest, Instruction_saturating_float_to_int) {
  ExpectWrite("\xfc\x00"_su8, I{O::I32TruncSatF32S});
  ExpectWrite("\xfc\x01"_su8, I{O::I32TruncSatF32U});
  ExpectWrite("\xfc\x02"_su8, I{O::I32TruncSatF64S});
  ExpectWrite("\xfc\x03"_su8, I{O::I32TruncSatF64U});
  ExpectWrite("\xfc\x04"_su8, I{O::I64TruncSatF32S});
  ExpectWrite("\xfc\x05"_su8, I{O::I64TruncSatF32U});
  ExpectWrite("\xfc\x06"_su8, I{O::I64TruncSatF64S});
  ExpectWrite("\xfc\x07"_su8, I{O::I64TruncSatF64U});
}

TEST(BinaryWriteTest, Instruction_bulk_memory) {
  ExpectWrite("\xfc\x08\x01\x00"_su8, I{O::MemoryInit, InitImmediate{1, 0}});
  ExpectWrite("\xfc\x09\x02"_su8, I{O::DataDrop, Index{2}});
  ExpectWrite("\xfc\x0a\x00\x00"_su8, I{O::MemoryCopy, CopyImmediate{0, 0}});
  ExpectWrite("\xfc\x0b\x00"_su8, I{O::MemoryFill, u8{0}});
  ExpectWrite("\xfc\x0c\x03\x00"_su8, I{O::TableInit, InitImmediate{3, 0}});
  ExpectWrite("\xfc\x0d\x04"_su8, I{O::ElemDrop, Index{4}});
  ExpectWrite("\xfc\x0e\x00\x00"_su8, I{O::TableCopy, CopyImmediate{0, 0}});
}

TEST(BinaryWriteTest, Instruction_simd) {
  ExpectWrite("\xfd\x00\x01\x02"_su8, I{O::V128Load, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x01\x01\x02"_su8, I{O::I16X8Load8X8S, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x02\x01\x02"_su8, I{O::I16X8Load8X8U, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x03\x01\x02"_su8, I{O::I32X4Load16X4S, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x04\x01\x02"_su8, I{O::I32X4Load16X4U, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x05\x01\x02"_su8, I{O::I64X2Load32X2S, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x06\x01\x02"_su8, I{O::I64X2Load32X2U, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x07\x01\x02"_su8, I{O::V8X16LoadSplat, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x08\x01\x02"_su8, I{O::V16X8LoadSplat, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x09\x01\x02"_su8, I{O::V32X4LoadSplat, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x0a\x01\x02"_su8, I{O::V64X2LoadSplat, MemArgImmediate{1, 2}});
  ExpectWrite("\xfd\x0b\x03\x04"_su8, I{O::V128Store, MemArgImmediate{3, 4}});
  ExpectWrite(
      "\xfd\x0c\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00"_su8,
      I{O::V128Const, v128{u64{5}, u64{6}}});
  ExpectWrite(
      "\xfd\x0d\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
      I{O::V8X16Shuffle,
        ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}});
  ExpectWrite("\xfd\x0e"_su8, I{O::V8X16Swizzle});
  ExpectWrite("\xfd\x0f"_su8, I{O::I8X16Splat});
  ExpectWrite("\xfd\x10"_su8, I{O::I16X8Splat});
  ExpectWrite("\xfd\x11"_su8, I{O::I32X4Splat});
  ExpectWrite("\xfd\x12"_su8, I{O::I64X2Splat});
  ExpectWrite("\xfd\x13"_su8, I{O::F32X4Splat});
  ExpectWrite("\xfd\x14"_su8, I{O::F64X2Splat});
  ExpectWrite("\xfd\x15\x00"_su8, I{O::I8X16ExtractLaneS, u8{0}});
  ExpectWrite("\xfd\x16\x00"_su8, I{O::I8X16ExtractLaneU, u8{0}});
  ExpectWrite("\xfd\x17\x00"_su8, I{O::I8X16ReplaceLane, u8{0}});
  ExpectWrite("\xfd\x18\x00"_su8, I{O::I16X8ExtractLaneS, u8{0}});
  ExpectWrite("\xfd\x19\x00"_su8, I{O::I16X8ExtractLaneU, u8{0}});
  ExpectWrite("\xfd\x1a\x00"_su8, I{O::I16X8ReplaceLane, u8{0}});
  ExpectWrite("\xfd\x1b\x00"_su8, I{O::I32X4ExtractLane, u8{0}});
  ExpectWrite("\xfd\x1c\x00"_su8, I{O::I32X4ReplaceLane, u8{0}});
  ExpectWrite("\xfd\x1d\x00"_su8, I{O::I64X2ExtractLane, u8{0}});
  ExpectWrite("\xfd\x1e\x00"_su8, I{O::I64X2ReplaceLane, u8{0}});
  ExpectWrite("\xfd\x1f\x00"_su8, I{O::F32X4ExtractLane, u8{0}});
  ExpectWrite("\xfd\x20\x00"_su8, I{O::F32X4ReplaceLane, u8{0}});
  ExpectWrite("\xfd\x21\x00"_su8, I{O::F64X2ExtractLane, u8{0}});
  ExpectWrite("\xfd\x22\x00"_su8, I{O::F64X2ReplaceLane, u8{0}});
  ExpectWrite("\xfd\x23"_su8, I{O::I8X16Eq});
  ExpectWrite("\xfd\x24"_su8, I{O::I8X16Ne});
  ExpectWrite("\xfd\x25"_su8, I{O::I8X16LtS});
  ExpectWrite("\xfd\x26"_su8, I{O::I8X16LtU});
  ExpectWrite("\xfd\x27"_su8, I{O::I8X16GtS});
  ExpectWrite("\xfd\x28"_su8, I{O::I8X16GtU});
  ExpectWrite("\xfd\x29"_su8, I{O::I8X16LeS});
  ExpectWrite("\xfd\x2a"_su8, I{O::I8X16LeU});
  ExpectWrite("\xfd\x2b"_su8, I{O::I8X16GeS});
  ExpectWrite("\xfd\x2c"_su8, I{O::I8X16GeU});
  ExpectWrite("\xfd\x2d"_su8, I{O::I16X8Eq});
  ExpectWrite("\xfd\x2e"_su8, I{O::I16X8Ne});
  ExpectWrite("\xfd\x2f"_su8, I{O::I16X8LtS});
  ExpectWrite("\xfd\x30"_su8, I{O::I16X8LtU});
  ExpectWrite("\xfd\x31"_su8, I{O::I16X8GtS});
  ExpectWrite("\xfd\x32"_su8, I{O::I16X8GtU});
  ExpectWrite("\xfd\x33"_su8, I{O::I16X8LeS});
  ExpectWrite("\xfd\x34"_su8, I{O::I16X8LeU});
  ExpectWrite("\xfd\x35"_su8, I{O::I16X8GeS});
  ExpectWrite("\xfd\x36"_su8, I{O::I16X8GeU});
  ExpectWrite("\xfd\x37"_su8, I{O::I32X4Eq});
  ExpectWrite("\xfd\x38"_su8, I{O::I32X4Ne});
  ExpectWrite("\xfd\x39"_su8, I{O::I32X4LtS});
  ExpectWrite("\xfd\x3a"_su8, I{O::I32X4LtU});
  ExpectWrite("\xfd\x3b"_su8, I{O::I32X4GtS});
  ExpectWrite("\xfd\x3c"_su8, I{O::I32X4GtU});
  ExpectWrite("\xfd\x3d"_su8, I{O::I32X4LeS});
  ExpectWrite("\xfd\x3e"_su8, I{O::I32X4LeU});
  ExpectWrite("\xfd\x3f"_su8, I{O::I32X4GeS});
  ExpectWrite("\xfd\x40"_su8, I{O::I32X4GeU});
  ExpectWrite("\xfd\x41"_su8, I{O::F32X4Eq});
  ExpectWrite("\xfd\x42"_su8, I{O::F32X4Ne});
  ExpectWrite("\xfd\x43"_su8, I{O::F32X4Lt});
  ExpectWrite("\xfd\x44"_su8, I{O::F32X4Gt});
  ExpectWrite("\xfd\x45"_su8, I{O::F32X4Le});
  ExpectWrite("\xfd\x46"_su8, I{O::F32X4Ge});
  ExpectWrite("\xfd\x47"_su8, I{O::F64X2Eq});
  ExpectWrite("\xfd\x48"_su8, I{O::F64X2Ne});
  ExpectWrite("\xfd\x49"_su8, I{O::F64X2Lt});
  ExpectWrite("\xfd\x4a"_su8, I{O::F64X2Gt});
  ExpectWrite("\xfd\x4b"_su8, I{O::F64X2Le});
  ExpectWrite("\xfd\x4c"_su8, I{O::F64X2Ge});
  ExpectWrite("\xfd\x4d"_su8, I{O::V128Not});
  ExpectWrite("\xfd\x4e"_su8, I{O::V128And});
  ExpectWrite("\xfd\x4f"_su8, I{O::V128Andnot});
  ExpectWrite("\xfd\x50"_su8, I{O::V128Or});
  ExpectWrite("\xfd\x51"_su8, I{O::V128Xor});
  ExpectWrite("\xfd\x52"_su8, I{O::V128BitSelect});
  ExpectWrite("\xfd\x60"_su8, I{O::I8X16Abs});
  ExpectWrite("\xfd\x61"_su8, I{O::I8X16Neg});
  ExpectWrite("\xfd\x62"_su8, I{O::I8X16AnyTrue});
  ExpectWrite("\xfd\x63"_su8, I{O::I8X16AllTrue});
  ExpectWrite("\xfd\x65"_su8, I{O::I8X16NarrowI16X8S});
  ExpectWrite("\xfd\x66"_su8, I{O::I8X16NarrowI16X8U});
  ExpectWrite("\xfd\x6b"_su8, I{O::I8X16Shl});
  ExpectWrite("\xfd\x6c"_su8, I{O::I8X16ShrS});
  ExpectWrite("\xfd\x6d"_su8, I{O::I8X16ShrU});
  ExpectWrite("\xfd\x6e"_su8, I{O::I8X16Add});
  ExpectWrite("\xfd\x6f"_su8, I{O::I8X16AddSaturateS});
  ExpectWrite("\xfd\x70"_su8, I{O::I8X16AddSaturateU});
  ExpectWrite("\xfd\x71"_su8, I{O::I8X16Sub});
  ExpectWrite("\xfd\x72"_su8, I{O::I8X16SubSaturateS});
  ExpectWrite("\xfd\x73"_su8, I{O::I8X16SubSaturateU});
  ExpectWrite("\xfd\x76"_su8, I{O::I8X16MinS});
  ExpectWrite("\xfd\x77"_su8, I{O::I8X16MinU});
  ExpectWrite("\xfd\x78"_su8, I{O::I8X16MaxS});
  ExpectWrite("\xfd\x79"_su8, I{O::I8X16MaxU});
  ExpectWrite("\xfd\x7b"_su8, I{O::I8X16AvgrU});
  ExpectWrite("\xfd\x80\x01"_su8, I{O::I16X8Abs});
  ExpectWrite("\xfd\x81\x01"_su8, I{O::I16X8Neg});
  ExpectWrite("\xfd\x82\x01"_su8, I{O::I16X8AnyTrue});
  ExpectWrite("\xfd\x83\x01"_su8, I{O::I16X8AllTrue});
  ExpectWrite("\xfd\x85\x01"_su8, I{O::I16X8NarrowI32X4S});
  ExpectWrite("\xfd\x86\x01"_su8, I{O::I16X8NarrowI32X4U});
  ExpectWrite("\xfd\x87\x01"_su8, I{O::I16X8WidenLowI8X16S});
  ExpectWrite("\xfd\x88\x01"_su8, I{O::I16X8WidenHighI8X16S});
  ExpectWrite("\xfd\x89\x01"_su8, I{O::I16X8WidenLowI8X16U});
  ExpectWrite("\xfd\x8a\x01"_su8, I{O::I16X8WidenHighI8X16U});
  ExpectWrite("\xfd\x8b\x01"_su8, I{O::I16X8Shl});
  ExpectWrite("\xfd\x8c\x01"_su8, I{O::I16X8ShrS});
  ExpectWrite("\xfd\x8d\x01"_su8, I{O::I16X8ShrU});
  ExpectWrite("\xfd\x8e\x01"_su8, I{O::I16X8Add});
  ExpectWrite("\xfd\x8f\x01"_su8, I{O::I16X8AddSaturateS});
  ExpectWrite("\xfd\x90\x01"_su8, I{O::I16X8AddSaturateU});
  ExpectWrite("\xfd\x91\x01"_su8, I{O::I16X8Sub});
  ExpectWrite("\xfd\x92\x01"_su8, I{O::I16X8SubSaturateS});
  ExpectWrite("\xfd\x93\x01"_su8, I{O::I16X8SubSaturateU});
  ExpectWrite("\xfd\x95\x01"_su8, I{O::I16X8Mul});
  ExpectWrite("\xfd\x96\x01"_su8, I{O::I16X8MinS});
  ExpectWrite("\xfd\x97\x01"_su8, I{O::I16X8MinU});
  ExpectWrite("\xfd\x98\x01"_su8, I{O::I16X8MaxS});
  ExpectWrite("\xfd\x99\x01"_su8, I{O::I16X8MaxU});
  ExpectWrite("\xfd\x9b\x01"_su8, I{O::I16X8AvgrU});
  ExpectWrite("\xfd\xa0\x01"_su8, I{O::I32X4Abs});
  ExpectWrite("\xfd\xa1\x01"_su8, I{O::I32X4Neg});
  ExpectWrite("\xfd\xa2\x01"_su8, I{O::I32X4AnyTrue});
  ExpectWrite("\xfd\xa3\x01"_su8, I{O::I32X4AllTrue});
  ExpectWrite("\xfd\xa7\x01"_su8, I{O::I32X4WidenLowI16X8S});
  ExpectWrite("\xfd\xa8\x01"_su8, I{O::I32X4WidenHighI16X8S});
  ExpectWrite("\xfd\xa9\x01"_su8, I{O::I32X4WidenLowI16X8U});
  ExpectWrite("\xfd\xaa\x01"_su8, I{O::I32X4WidenHighI16X8U});
  ExpectWrite("\xfd\xab\x01"_su8, I{O::I32X4Shl});
  ExpectWrite("\xfd\xac\x01"_su8, I{O::I32X4ShrS});
  ExpectWrite("\xfd\xad\x01"_su8, I{O::I32X4ShrU});
  ExpectWrite("\xfd\xae\x01"_su8, I{O::I32X4Add});
  ExpectWrite("\xfd\xb1\x01"_su8, I{O::I32X4Sub});
  ExpectWrite("\xfd\xb5\x01"_su8, I{O::I32X4Mul});
  ExpectWrite("\xfd\xb6\x01"_su8, I{O::I32X4MinS});
  ExpectWrite("\xfd\xb7\x01"_su8, I{O::I32X4MinU});
  ExpectWrite("\xfd\xb8\x01"_su8, I{O::I32X4MaxS});
  ExpectWrite("\xfd\xb9\x01"_su8, I{O::I32X4MaxU});
  ExpectWrite("\xfd\xc1\x01"_su8, I{O::I64X2Neg});
  ExpectWrite("\xfd\xcb\x01"_su8, I{O::I64X2Shl});
  ExpectWrite("\xfd\xcc\x01"_su8, I{O::I64X2ShrS});
  ExpectWrite("\xfd\xcd\x01"_su8, I{O::I64X2ShrU});
  ExpectWrite("\xfd\xce\x01"_su8, I{O::I64X2Add});
  ExpectWrite("\xfd\xd1\x01"_su8, I{O::I64X2Sub});
  ExpectWrite("\xfd\xd5\x01"_su8, I{O::I64X2Mul});
  ExpectWrite("\xfd\xe0\x01"_su8, I{O::F32X4Abs});
  ExpectWrite("\xfd\xe1\x01"_su8, I{O::F32X4Neg});
  ExpectWrite("\xfd\xe3\x01"_su8, I{O::F32X4Sqrt});
  ExpectWrite("\xfd\xe4\x01"_su8, I{O::F32X4Add});
  ExpectWrite("\xfd\xe5\x01"_su8, I{O::F32X4Sub});
  ExpectWrite("\xfd\xe6\x01"_su8, I{O::F32X4Mul});
  ExpectWrite("\xfd\xe7\x01"_su8, I{O::F32X4Div});
  ExpectWrite("\xfd\xe8\x01"_su8, I{O::F32X4Min});
  ExpectWrite("\xfd\xe9\x01"_su8, I{O::F32X4Max});
  ExpectWrite("\xfd\xec\x01"_su8, I{O::F64X2Abs});
  ExpectWrite("\xfd\xed\x01"_su8, I{O::F64X2Neg});
  ExpectWrite("\xfd\xef\x01"_su8, I{O::F64X2Sqrt});
  ExpectWrite("\xfd\xf0\x01"_su8, I{O::F64X2Add});
  ExpectWrite("\xfd\xf1\x01"_su8, I{O::F64X2Sub});
  ExpectWrite("\xfd\xf2\x01"_su8, I{O::F64X2Mul});
  ExpectWrite("\xfd\xf3\x01"_su8, I{O::F64X2Div});
  ExpectWrite("\xfd\xf4\x01"_su8, I{O::F64X2Min});
  ExpectWrite("\xfd\xf5\x01"_su8, I{O::F64X2Max});
  ExpectWrite("\xfd\xf8\x01"_su8, I{O::I32X4TruncSatF32X4S});
  ExpectWrite("\xfd\xf9\x01"_su8, I{O::I32X4TruncSatF32X4U});
  ExpectWrite("\xfd\xfa\x01"_su8, I{O::F32X4ConvertI32X4S});
  ExpectWrite("\xfd\xfb\x01"_su8, I{O::F32X4ConvertI32X4U});
}

TEST(BinaryWriteTest, Instruction_threads) {
  const MemArgImmediate m{0, 0};

  ExpectWrite("\xfe\x00\x00\x00"_su8, I{O::MemoryAtomicNotify, m});
  ExpectWrite("\xfe\x01\x00\x00"_su8, I{O::MemoryAtomicWait32, m});
  ExpectWrite("\xfe\x02\x00\x00"_su8, I{O::MemoryAtomicWait64, m});
  ExpectWrite("\xfe\x10\x00\x00"_su8, I{O::I32AtomicLoad, m});
  ExpectWrite("\xfe\x11\x00\x00"_su8, I{O::I64AtomicLoad, m});
  ExpectWrite("\xfe\x12\x00\x00"_su8, I{O::I32AtomicLoad8U, m});
  ExpectWrite("\xfe\x13\x00\x00"_su8, I{O::I32AtomicLoad16U, m});
  ExpectWrite("\xfe\x14\x00\x00"_su8, I{O::I64AtomicLoad8U, m});
  ExpectWrite("\xfe\x15\x00\x00"_su8, I{O::I64AtomicLoad16U, m});
  ExpectWrite("\xfe\x16\x00\x00"_su8, I{O::I64AtomicLoad32U, m});
  ExpectWrite("\xfe\x17\x00\x00"_su8, I{O::I32AtomicStore, m});
  ExpectWrite("\xfe\x18\x00\x00"_su8, I{O::I64AtomicStore, m});
  ExpectWrite("\xfe\x19\x00\x00"_su8, I{O::I32AtomicStore8, m});
  ExpectWrite("\xfe\x1a\x00\x00"_su8, I{O::I32AtomicStore16, m});
  ExpectWrite("\xfe\x1b\x00\x00"_su8, I{O::I64AtomicStore8, m});
  ExpectWrite("\xfe\x1c\x00\x00"_su8, I{O::I64AtomicStore16, m});
  ExpectWrite("\xfe\x1d\x00\x00"_su8, I{O::I64AtomicStore32, m});
  ExpectWrite("\xfe\x1e\x00\x00"_su8, I{O::I32AtomicRmwAdd, m});
  ExpectWrite("\xfe\x1f\x00\x00"_su8, I{O::I64AtomicRmwAdd, m});
  ExpectWrite("\xfe\x20\x00\x00"_su8, I{O::I32AtomicRmw8AddU, m});
  ExpectWrite("\xfe\x21\x00\x00"_su8, I{O::I32AtomicRmw16AddU, m});
  ExpectWrite("\xfe\x22\x00\x00"_su8, I{O::I64AtomicRmw8AddU, m});
  ExpectWrite("\xfe\x23\x00\x00"_su8, I{O::I64AtomicRmw16AddU, m});
  ExpectWrite("\xfe\x24\x00\x00"_su8, I{O::I64AtomicRmw32AddU, m});
  ExpectWrite("\xfe\x25\x00\x00"_su8, I{O::I32AtomicRmwSub, m});
  ExpectWrite("\xfe\x26\x00\x00"_su8, I{O::I64AtomicRmwSub, m});
  ExpectWrite("\xfe\x27\x00\x00"_su8, I{O::I32AtomicRmw8SubU, m});
  ExpectWrite("\xfe\x28\x00\x00"_su8, I{O::I32AtomicRmw16SubU, m});
  ExpectWrite("\xfe\x29\x00\x00"_su8, I{O::I64AtomicRmw8SubU, m});
  ExpectWrite("\xfe\x2a\x00\x00"_su8, I{O::I64AtomicRmw16SubU, m});
  ExpectWrite("\xfe\x2b\x00\x00"_su8, I{O::I64AtomicRmw32SubU, m});
  ExpectWrite("\xfe\x2c\x00\x00"_su8, I{O::I32AtomicRmwAnd, m});
  ExpectWrite("\xfe\x2d\x00\x00"_su8, I{O::I64AtomicRmwAnd, m});
  ExpectWrite("\xfe\x2e\x00\x00"_su8, I{O::I32AtomicRmw8AndU, m});
  ExpectWrite("\xfe\x2f\x00\x00"_su8, I{O::I32AtomicRmw16AndU, m});
  ExpectWrite("\xfe\x30\x00\x00"_su8, I{O::I64AtomicRmw8AndU, m});
  ExpectWrite("\xfe\x31\x00\x00"_su8, I{O::I64AtomicRmw16AndU, m});
  ExpectWrite("\xfe\x32\x00\x00"_su8, I{O::I64AtomicRmw32AndU, m});
  ExpectWrite("\xfe\x33\x00\x00"_su8, I{O::I32AtomicRmwOr, m});
  ExpectWrite("\xfe\x34\x00\x00"_su8, I{O::I64AtomicRmwOr, m});
  ExpectWrite("\xfe\x35\x00\x00"_su8, I{O::I32AtomicRmw8OrU, m});
  ExpectWrite("\xfe\x36\x00\x00"_su8, I{O::I32AtomicRmw16OrU, m});
  ExpectWrite("\xfe\x37\x00\x00"_su8, I{O::I64AtomicRmw8OrU, m});
  ExpectWrite("\xfe\x38\x00\x00"_su8, I{O::I64AtomicRmw16OrU, m});
  ExpectWrite("\xfe\x39\x00\x00"_su8, I{O::I64AtomicRmw32OrU, m});
  ExpectWrite("\xfe\x3a\x00\x00"_su8, I{O::I32AtomicRmwXor, m});
  ExpectWrite("\xfe\x3b\x00\x00"_su8, I{O::I64AtomicRmwXor, m});
  ExpectWrite("\xfe\x3c\x00\x00"_su8, I{O::I32AtomicRmw8XorU, m});
  ExpectWrite("\xfe\x3d\x00\x00"_su8, I{O::I32AtomicRmw16XorU, m});
  ExpectWrite("\xfe\x3e\x00\x00"_su8, I{O::I64AtomicRmw8XorU, m});
  ExpectWrite("\xfe\x3f\x00\x00"_su8, I{O::I64AtomicRmw16XorU, m});
  ExpectWrite("\xfe\x40\x00\x00"_su8, I{O::I64AtomicRmw32XorU, m});
  ExpectWrite("\xfe\x41\x00\x00"_su8, I{O::I32AtomicRmwXchg, m});
  ExpectWrite("\xfe\x42\x00\x00"_su8, I{O::I64AtomicRmwXchg, m});
  ExpectWrite("\xfe\x43\x00\x00"_su8, I{O::I32AtomicRmw8XchgU, m});
  ExpectWrite("\xfe\x44\x00\x00"_su8, I{O::I32AtomicRmw16XchgU, m});
  ExpectWrite("\xfe\x45\x00\x00"_su8, I{O::I64AtomicRmw8XchgU, m});
  ExpectWrite("\xfe\x46\x00\x00"_su8, I{O::I64AtomicRmw16XchgU, m});
  ExpectWrite("\xfe\x47\x00\x00"_su8, I{O::I64AtomicRmw32XchgU, m});
  ExpectWrite("\xfe\x48\x00\x00"_su8, I{O::I32AtomicRmwCmpxchg, m});
  ExpectWrite("\xfe\x49\x00\x00"_su8, I{O::I64AtomicRmwCmpxchg, m});
  ExpectWrite("\xfe\x4a\x00\x00"_su8, I{O::I32AtomicRmw8CmpxchgU, m});
  ExpectWrite("\xfe\x4b\x00\x00"_su8, I{O::I32AtomicRmw16CmpxchgU, m});
  ExpectWrite("\xfe\x4c\x00\x00"_su8, I{O::I64AtomicRmw8CmpxchgU, m});
  ExpectWrite("\xfe\x4d\x00\x00"_su8, I{O::I64AtomicRmw16CmpxchgU, m});
  ExpectWrite("\xfe\x4e\x00\x00"_su8, I{O::I64AtomicRmw32CmpxchgU, m});
}

TEST(BinaryWriteTest, KnownSection_Vector) {
  std::vector<TypeEntry> types{
      TypeEntry{FunctionType{{ValueType::I32, ValueType::I64}, {}}},
      TypeEntry{FunctionType{{}, {ValueType::I32, ValueType::I64}}},
  };
  SpanU8 expected =
      "\x01"                       // section "type"
      "\x0b"                       // section length
      "\x02"                       // type count
      "\x60\x02\x7f\x7e\x00"       // (func (param i32 i64))
      "\x60\x00\x02\x7f\x7e"_su8;  // (func (result i32 i64))

  Buffer result(expected.size());
  auto iter =
      WriteKnownSection(SectionId::Type, types.begin(), types.end(),
                        MakeClampedIterator(result.begin(), result.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

TEST(BinaryWriteTest, KnownSection_Optional_Exists) {
  optional<Start> start{Start{13}};
  SpanU8 expected =
      "\x08"       // section "start"
      "\x01"       // section length
      "\x0d"_su8;  // index 13.

  Buffer result(expected.size());
  auto iter = WriteNonEmptyKnownSection(
      SectionId::Start, start,
      MakeClampedIterator(result.begin(), result.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

TEST(BinaryWriteTest, KnownSection_Optional_DoesNotExist) {
  optional<Start> start;
  SpanU8 expected = ""_su8;

  Buffer result(expected.size());
  auto iter = WriteNonEmptyKnownSection(
      SectionId::Start, start,
      MakeClampedIterator(result.begin(), result.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

TEST(BinaryWriteTest, Limits) {
  ExpectWrite("\x00\x81\x01"_su8, Limits{129});
  ExpectWrite("\x01\x02\xe8\x07"_su8, Limits{2, 1000});
}

TEST(BinaryWriteTest, Locals) {
  ExpectWrite("\x02\x7f"_su8, Locals{2, ValueType::I32});
  ExpectWrite("\xc0\x02\x7c"_su8, Locals{320, ValueType::F64});
}

TEST(BinaryWriteTest, MemArgImmediate) {
  ExpectWrite("\x00\x00"_su8, MemArgImmediate{0, 0});
  ExpectWrite("\x01\x80\x02"_su8, MemArgImmediate{1, 256});
}

TEST(BinaryWriteTest, Memory) {
  ExpectWrite("\x01\x01\x02"_su8, Memory{MemoryType{Limits{1, 2}}});
}

TEST(BinaryWriteTest, MemoryType) {
  ExpectWrite("\x00\x01"_su8, MemoryType{Limits{1}});
  ExpectWrite("\x01\x00\x80\x01"_su8, MemoryType{Limits{0, 128}});
}

TEST(BinaryWriteTest, Module) {
  ExpectWrite("\x00\x61\x73\x6d\x01\x00\x00\x00"_su8, Module{});
}

TEST(BinaryWriteTest, Module_TypeEntry) {
  Module module;
  module.types.push_back(TypeEntry{FunctionType{}});
  module.types.push_back(
      TypeEntry{FunctionType{{ValueType::I32}, {ValueType::I64}}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x01"                              // type section
      "\x09"                              // section length
      "\x02"                              // function type count
      "\x60\x00\x00"                      // (func)
      "\x60\x01\x7f\x01\x7e"_su8,         // (func (param i32) (result i64)
      module);
}

TEST(BinaryWriteTest, Module_Import) {
  Module module;
  module.imports.push_back(Import{"v"_sv, "w"_sv, Index{3}});
  module.imports.push_back(Import{"x"_sv, "y"_sv, MemoryType{Limits{1}}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x02"                              // import section
      "\x0e"                              // section length
      "\x02"                              // import count
      "\x01v\x01w\x00\x03"                // (import "v" "w" (func (type 13)))
      "\x01x\x01y\x02\x00\x01"_su8,       // (import "x" "y" (memory 1))
      module);
}

TEST(BinaryWriteTest, Module_Function) {
  Module module;
  module.functions.push_back(Function{Index{3}});
  module.functions.push_back(Function{Index{4}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x03"                              // function section
      "\x03"                              // section length
      "\x02"                              // function count
      "\x03\x04"_su8,                     // type indexes
      module);
}

TEST(BinaryWriteTest, Module_Table) {
  Module module;
  module.tables.push_back(Table{TableType{Limits{1}, ReferenceType::Funcref}});
  module.tables.push_back(Table{TableType{Limits{2}, ReferenceType::Externref}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x04"                              // table section
      "\x07"                              // section length
      "\x02"                              // table count
      "\x70\x00\x01"                      // (table 1 funcref)
      "\x6f\x00\x02"_su8,                 // (table 2 externref)
      module);
}

TEST(BinaryWriteTest, Module_Memory) {
  Module module;
  module.memories.push_back(Memory{MemoryType{Limits{1}}});
  module.memories.push_back(Memory{MemoryType{Limits{2}}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x05"                              // memory section
      "\x05"                              // section length
      "\x02"                              // memory count
      "\x00\x01"                          // (memory 1)
      "\x00\x02"_su8,                     // (memory 2)
      module);
}

TEST(BinaryWriteTest, Module_Global) {
  Module module;
  module.globals.push_back(
      Global{GlobalType{ValueType::I32, Mutability::Const},
             ConstantExpression{Instruction{Opcode::I32Const, s32{1}}}});
  module.globals.push_back(
      Global{GlobalType{ValueType::I64, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I64Const, s64{2}}}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x06"                              // global section
      "\x0b"                              // section length
      "\x02"                              // global count
      "\x7f\x00\x41\x01\x0b"              // (global i32 i32.const 1)
      "\x7e\x01\x42\x02\x0b"_su8,         // (global (mut i64) i64.const 2)
      module);
}

TEST(BinaryWriteTest, Module_Event) {
  Module module;
  module.events.push_back(Event{EventType{EventAttribute::Exception, Index{1}}});
  module.events.push_back(Event{EventType{EventAttribute::Exception, Index{2}}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x0d"                              // event section
      "\x05"                              // section length
      "\x02"                              // event count
      "\x00\x01"                          // (event (func (type 1)))
      "\x00\x02"_su8,                     // (event (func (type 2)))
      module);
}

TEST(BinaryWriteTest, Module_Export) {
  Module module;
  module.exports.push_back(Export{ExternalKind::Function, "x"_sv, Index{1}});
  module.exports.push_back(Export{ExternalKind::Table, "y"_sv, Index{2}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x07"                              // export section
      "\x09"                              // section length
      "\x02"                              // export count
      "\x01x\x00\x01"                     // (export "x" (func 1))
      "\x01y\x01\x02"_su8,                // (export "y" (table 1))
      module);
}

TEST(BinaryWriteTest, Module_Start) {
  Module module;
  module.start = Start{Index{3}};

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x08"                              // start section
      "\x01"                              // section length
      "\x03"_su8,                         // (start 3)
      module);
}

TEST(BinaryWriteTest, Module_Element) {
  Module module;
  module.element_segments.push_back(ElementSegment{
      SegmentType::Passive,
      ElementList{ElementListWithIndexes{ExternalKind::Function, {1, 2}}}});
  module.element_segments.push_back(ElementSegment{
      Index{3}, ConstantExpression{Instruction{Opcode::I32Const, s32{4}}},
      ElementList{ElementListWithIndexes{ExternalKind::Function, {5, 6}}}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x09"                              // element section
      "\x0f"                              // section length
      "\x02"                              // element count
      "\x01\x00"                          // (elem func
      "\x02\x01\x02"                      //  1 2)
      "\x02\x03\x41\x04\x0b"              // (elem (table 3) (i32.const 4)
      "\x00\x02\x05\x06"_su8,             //  func 5 6)
      module);
}

TEST(BinaryWriteTest, Module_DataCount) {
  Module module;
  module.data_count = DataCount{Index{3}};

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x0c"                              // data count section
      "\x01"                              // section length
      "\x03"_su8,                         // data count
      module);
}

TEST(BinaryWriteTest, Module_Code) {
  Module module;
  module.codes.push_back(
      Code{LocalsList{Locals{2, ValueType::I32}, Locals{1, ValueType::I64}},
           Expression{"\x00\x0b"_su8}});
  module.codes.push_back(Code{LocalsList{}, Expression{"\x41\x01\x0b"_su8}});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x0a"                              // code section
      "\x0e"                              // section length
      "\x02"                              // code count
      "\x07"                              // code 0 size
      "\x02\x02\x7f\x01\x7e"              // (local i32 i32 i64)
      "\x00\x0b"                          //  nop
      "\x04"                              // code 1 size
      "\x00"                              // no locals
      "\x41\x01\x0b"_su8,                 // (i32.const 1)
      module);
}

TEST(BinaryWriteTest, Module_Data) {
  Module module;
  module.data_segments.push_back(DataSegment{"hi"_su8});
  module.data_segments.push_back(DataSegment{
      Index{1}, ConstantExpression{Instruction{Opcode::I32Const, s32{2}}},
      "X"_su8});

  ExpectWrite(
      "\x00\x61\x73\x6d\x01\x00\x00\x00"  // magic/version
      "\x0b"                              // data section
      "\x0c"                              // section length
      "\x02"                              // data count
      "\x01\x02hi"                        // (data "hi")
      "\x02\x01\x41\x02\x0b\x01X"_su8,    // (data (memory 1) (i32.const 2) "X")
      module);
}


TEST(BinaryWriteTest, Mutability) {
  ExpectWrite("\x00"_su8, Mutability::Const);
  ExpectWrite("\x01"_su8, Mutability::Var);
}

TEST(BinaryWriteTest, NameSubsectionId) {
  ExpectWrite("\x00"_su8, NameSubsectionId::ModuleName);
  ExpectWrite("\x01"_su8, NameSubsectionId::FunctionNames);
  ExpectWrite("\x02"_su8, NameSubsectionId::LocalNames);
}

TEST(BinaryWriteTest, Opcode) {
  ExpectWrite("\x00"_su8, Opcode::Unreachable);
  ExpectWrite("\x01"_su8, Opcode::Nop);
  ExpectWrite("\x02"_su8, Opcode::Block);
  ExpectWrite("\x03"_su8, Opcode::Loop);
  ExpectWrite("\x04"_su8, Opcode::If);
  ExpectWrite("\x05"_su8, Opcode::Else);
  ExpectWrite("\x0b"_su8, Opcode::End);
  ExpectWrite("\x0c"_su8, Opcode::Br);
  ExpectWrite("\x0d"_su8, Opcode::BrIf);
  ExpectWrite("\x0e"_su8, Opcode::BrTable);
  ExpectWrite("\x0f"_su8, Opcode::Return);
  ExpectWrite("\x10"_su8, Opcode::Call);
  ExpectWrite("\x11"_su8, Opcode::CallIndirect);
  ExpectWrite("\x1a"_su8, Opcode::Drop);
  ExpectWrite("\x1b"_su8, Opcode::Select);
  ExpectWrite("\x20"_su8, Opcode::LocalGet);
  ExpectWrite("\x21"_su8, Opcode::LocalSet);
  ExpectWrite("\x22"_su8, Opcode::LocalTee);
  ExpectWrite("\x23"_su8, Opcode::GlobalGet);
  ExpectWrite("\x24"_su8, Opcode::GlobalSet);
  ExpectWrite("\x28"_su8, Opcode::I32Load);
  ExpectWrite("\x29"_su8, Opcode::I64Load);
  ExpectWrite("\x2a"_su8, Opcode::F32Load);
  ExpectWrite("\x2b"_su8, Opcode::F64Load);
  ExpectWrite("\x2c"_su8, Opcode::I32Load8S);
  ExpectWrite("\x2d"_su8, Opcode::I32Load8U);
  ExpectWrite("\x2e"_su8, Opcode::I32Load16S);
  ExpectWrite("\x2f"_su8, Opcode::I32Load16U);
  ExpectWrite("\x30"_su8, Opcode::I64Load8S);
  ExpectWrite("\x31"_su8, Opcode::I64Load8U);
  ExpectWrite("\x32"_su8, Opcode::I64Load16S);
  ExpectWrite("\x33"_su8, Opcode::I64Load16U);
  ExpectWrite("\x34"_su8, Opcode::I64Load32S);
  ExpectWrite("\x35"_su8, Opcode::I64Load32U);
  ExpectWrite("\x36"_su8, Opcode::I32Store);
  ExpectWrite("\x37"_su8, Opcode::I64Store);
  ExpectWrite("\x38"_su8, Opcode::F32Store);
  ExpectWrite("\x39"_su8, Opcode::F64Store);
  ExpectWrite("\x3a"_su8, Opcode::I32Store8);
  ExpectWrite("\x3b"_su8, Opcode::I32Store16);
  ExpectWrite("\x3c"_su8, Opcode::I64Store8);
  ExpectWrite("\x3d"_su8, Opcode::I64Store16);
  ExpectWrite("\x3e"_su8, Opcode::I64Store32);
  ExpectWrite("\x3f"_su8, Opcode::MemorySize);
  ExpectWrite("\x40"_su8, Opcode::MemoryGrow);
  ExpectWrite("\x41"_su8, Opcode::I32Const);
  ExpectWrite("\x42"_su8, Opcode::I64Const);
  ExpectWrite("\x43"_su8, Opcode::F32Const);
  ExpectWrite("\x44"_su8, Opcode::F64Const);
  ExpectWrite("\x45"_su8, Opcode::I32Eqz);
  ExpectWrite("\x46"_su8, Opcode::I32Eq);
  ExpectWrite("\x47"_su8, Opcode::I32Ne);
  ExpectWrite("\x48"_su8, Opcode::I32LtS);
  ExpectWrite("\x49"_su8, Opcode::I32LtU);
  ExpectWrite("\x4a"_su8, Opcode::I32GtS);
  ExpectWrite("\x4b"_su8, Opcode::I32GtU);
  ExpectWrite("\x4c"_su8, Opcode::I32LeS);
  ExpectWrite("\x4d"_su8, Opcode::I32LeU);
  ExpectWrite("\x4e"_su8, Opcode::I32GeS);
  ExpectWrite("\x4f"_su8, Opcode::I32GeU);
  ExpectWrite("\x50"_su8, Opcode::I64Eqz);
  ExpectWrite("\x51"_su8, Opcode::I64Eq);
  ExpectWrite("\x52"_su8, Opcode::I64Ne);
  ExpectWrite("\x53"_su8, Opcode::I64LtS);
  ExpectWrite("\x54"_su8, Opcode::I64LtU);
  ExpectWrite("\x55"_su8, Opcode::I64GtS);
  ExpectWrite("\x56"_su8, Opcode::I64GtU);
  ExpectWrite("\x57"_su8, Opcode::I64LeS);
  ExpectWrite("\x58"_su8, Opcode::I64LeU);
  ExpectWrite("\x59"_su8, Opcode::I64GeS);
  ExpectWrite("\x5a"_su8, Opcode::I64GeU);
  ExpectWrite("\x5b"_su8, Opcode::F32Eq);
  ExpectWrite("\x5c"_su8, Opcode::F32Ne);
  ExpectWrite("\x5d"_su8, Opcode::F32Lt);
  ExpectWrite("\x5e"_su8, Opcode::F32Gt);
  ExpectWrite("\x5f"_su8, Opcode::F32Le);
  ExpectWrite("\x60"_su8, Opcode::F32Ge);
  ExpectWrite("\x61"_su8, Opcode::F64Eq);
  ExpectWrite("\x62"_su8, Opcode::F64Ne);
  ExpectWrite("\x63"_su8, Opcode::F64Lt);
  ExpectWrite("\x64"_su8, Opcode::F64Gt);
  ExpectWrite("\x65"_su8, Opcode::F64Le);
  ExpectWrite("\x66"_su8, Opcode::F64Ge);
  ExpectWrite("\x67"_su8, Opcode::I32Clz);
  ExpectWrite("\x68"_su8, Opcode::I32Ctz);
  ExpectWrite("\x69"_su8, Opcode::I32Popcnt);
  ExpectWrite("\x6a"_su8, Opcode::I32Add);
  ExpectWrite("\x6b"_su8, Opcode::I32Sub);
  ExpectWrite("\x6c"_su8, Opcode::I32Mul);
  ExpectWrite("\x6d"_su8, Opcode::I32DivS);
  ExpectWrite("\x6e"_su8, Opcode::I32DivU);
  ExpectWrite("\x6f"_su8, Opcode::I32RemS);
  ExpectWrite("\x70"_su8, Opcode::I32RemU);
  ExpectWrite("\x71"_su8, Opcode::I32And);
  ExpectWrite("\x72"_su8, Opcode::I32Or);
  ExpectWrite("\x73"_su8, Opcode::I32Xor);
  ExpectWrite("\x74"_su8, Opcode::I32Shl);
  ExpectWrite("\x75"_su8, Opcode::I32ShrS);
  ExpectWrite("\x76"_su8, Opcode::I32ShrU);
  ExpectWrite("\x77"_su8, Opcode::I32Rotl);
  ExpectWrite("\x78"_su8, Opcode::I32Rotr);
  ExpectWrite("\x79"_su8, Opcode::I64Clz);
  ExpectWrite("\x7a"_su8, Opcode::I64Ctz);
  ExpectWrite("\x7b"_su8, Opcode::I64Popcnt);
  ExpectWrite("\x7c"_su8, Opcode::I64Add);
  ExpectWrite("\x7d"_su8, Opcode::I64Sub);
  ExpectWrite("\x7e"_su8, Opcode::I64Mul);
  ExpectWrite("\x7f"_su8, Opcode::I64DivS);
  ExpectWrite("\x80"_su8, Opcode::I64DivU);
  ExpectWrite("\x81"_su8, Opcode::I64RemS);
  ExpectWrite("\x82"_su8, Opcode::I64RemU);
  ExpectWrite("\x83"_su8, Opcode::I64And);
  ExpectWrite("\x84"_su8, Opcode::I64Or);
  ExpectWrite("\x85"_su8, Opcode::I64Xor);
  ExpectWrite("\x86"_su8, Opcode::I64Shl);
  ExpectWrite("\x87"_su8, Opcode::I64ShrS);
  ExpectWrite("\x88"_su8, Opcode::I64ShrU);
  ExpectWrite("\x89"_su8, Opcode::I64Rotl);
  ExpectWrite("\x8a"_su8, Opcode::I64Rotr);
  ExpectWrite("\x8b"_su8, Opcode::F32Abs);
  ExpectWrite("\x8c"_su8, Opcode::F32Neg);
  ExpectWrite("\x8d"_su8, Opcode::F32Ceil);
  ExpectWrite("\x8e"_su8, Opcode::F32Floor);
  ExpectWrite("\x8f"_su8, Opcode::F32Trunc);
  ExpectWrite("\x90"_su8, Opcode::F32Nearest);
  ExpectWrite("\x91"_su8, Opcode::F32Sqrt);
  ExpectWrite("\x92"_su8, Opcode::F32Add);
  ExpectWrite("\x93"_su8, Opcode::F32Sub);
  ExpectWrite("\x94"_su8, Opcode::F32Mul);
  ExpectWrite("\x95"_su8, Opcode::F32Div);
  ExpectWrite("\x96"_su8, Opcode::F32Min);
  ExpectWrite("\x97"_su8, Opcode::F32Max);
  ExpectWrite("\x98"_su8, Opcode::F32Copysign);
  ExpectWrite("\x99"_su8, Opcode::F64Abs);
  ExpectWrite("\x9a"_su8, Opcode::F64Neg);
  ExpectWrite("\x9b"_su8, Opcode::F64Ceil);
  ExpectWrite("\x9c"_su8, Opcode::F64Floor);
  ExpectWrite("\x9d"_su8, Opcode::F64Trunc);
  ExpectWrite("\x9e"_su8, Opcode::F64Nearest);
  ExpectWrite("\x9f"_su8, Opcode::F64Sqrt);
  ExpectWrite("\xa0"_su8, Opcode::F64Add);
  ExpectWrite("\xa1"_su8, Opcode::F64Sub);
  ExpectWrite("\xa2"_su8, Opcode::F64Mul);
  ExpectWrite("\xa3"_su8, Opcode::F64Div);
  ExpectWrite("\xa4"_su8, Opcode::F64Min);
  ExpectWrite("\xa5"_su8, Opcode::F64Max);
  ExpectWrite("\xa6"_su8, Opcode::F64Copysign);
  ExpectWrite("\xa7"_su8, Opcode::I32WrapI64);
  ExpectWrite("\xa8"_su8, Opcode::I32TruncF32S);
  ExpectWrite("\xa9"_su8, Opcode::I32TruncF32U);
  ExpectWrite("\xaa"_su8, Opcode::I32TruncF64S);
  ExpectWrite("\xab"_su8, Opcode::I32TruncF64U);
  ExpectWrite("\xac"_su8, Opcode::I64ExtendI32S);
  ExpectWrite("\xad"_su8, Opcode::I64ExtendI32U);
  ExpectWrite("\xae"_su8, Opcode::I64TruncF32S);
  ExpectWrite("\xaf"_su8, Opcode::I64TruncF32U);
  ExpectWrite("\xb0"_su8, Opcode::I64TruncF64S);
  ExpectWrite("\xb1"_su8, Opcode::I64TruncF64U);
  ExpectWrite("\xb2"_su8, Opcode::F32ConvertI32S);
  ExpectWrite("\xb3"_su8, Opcode::F32ConvertI32U);
  ExpectWrite("\xb4"_su8, Opcode::F32ConvertI64S);
  ExpectWrite("\xb5"_su8, Opcode::F32ConvertI64U);
  ExpectWrite("\xb6"_su8, Opcode::F32DemoteF64);
  ExpectWrite("\xb7"_su8, Opcode::F64ConvertI32S);
  ExpectWrite("\xb8"_su8, Opcode::F64ConvertI32U);
  ExpectWrite("\xb9"_su8, Opcode::F64ConvertI64S);
  ExpectWrite("\xba"_su8, Opcode::F64ConvertI64U);
  ExpectWrite("\xbb"_su8, Opcode::F64PromoteF32);
  ExpectWrite("\xbc"_su8, Opcode::I32ReinterpretF32);
  ExpectWrite("\xbd"_su8, Opcode::I64ReinterpretF64);
  ExpectWrite("\xbe"_su8, Opcode::F32ReinterpretI32);
  ExpectWrite("\xbf"_su8, Opcode::F64ReinterpretI64);
}

TEST(BinaryWriteTest, Opcode_exceptions) {
  ExpectWrite("\x06"_su8, Opcode::Try);
  ExpectWrite("\x07"_su8, Opcode::Catch);
  ExpectWrite("\x08"_su8, Opcode::Throw);
  ExpectWrite("\x09"_su8, Opcode::Rethrow);
  ExpectWrite("\x0a"_su8, Opcode::BrOnExn);
}

TEST(BinaryWriteTest, Opcode_tail_call) {
  ExpectWrite("\x12"_su8, Opcode::ReturnCall);
  ExpectWrite("\x13"_su8, Opcode::ReturnCallIndirect);
}

TEST(BinaryWriteTest, Opcode_sign_extension) {
  ExpectWrite("\xc0"_su8, Opcode::I32Extend8S);
  ExpectWrite("\xc1"_su8, Opcode::I32Extend16S);
  ExpectWrite("\xc2"_su8, Opcode::I64Extend8S);
  ExpectWrite("\xc3"_su8, Opcode::I64Extend16S);
  ExpectWrite("\xc4"_su8, Opcode::I64Extend32S);
}

TEST(BinaryWriteTest, Opcode_reference_types) {
  ExpectWrite("\x1c"_su8, Opcode::SelectT);
  ExpectWrite("\x25"_su8, Opcode::TableGet);
  ExpectWrite("\x26"_su8, Opcode::TableSet);
  ExpectWrite("\xfc\x0f"_su8, Opcode::TableGrow);
  ExpectWrite("\xfc\x10"_su8, Opcode::TableSize);
  ExpectWrite("\xfc\x11"_su8, Opcode::TableFill);
  ExpectWrite("\xd0"_su8, Opcode::RefNull);
  ExpectWrite("\xd1"_su8, Opcode::RefIsNull);
}

TEST(BinaryWriteTest, Opcode_function_references) {
  ExpectWrite("\xd2"_su8, Opcode::RefFunc);
}

TEST(BinaryWriteTest, Opcode_saturating_float_to_int) {
  ExpectWrite("\xfc\x00"_su8, Opcode::I32TruncSatF32S);
  ExpectWrite("\xfc\x01"_su8, Opcode::I32TruncSatF32U);
  ExpectWrite("\xfc\x02"_su8, Opcode::I32TruncSatF64S);
  ExpectWrite("\xfc\x03"_su8, Opcode::I32TruncSatF64U);
  ExpectWrite("\xfc\x04"_su8, Opcode::I64TruncSatF32S);
  ExpectWrite("\xfc\x05"_su8, Opcode::I64TruncSatF32U);
  ExpectWrite("\xfc\x06"_su8, Opcode::I64TruncSatF64S);
  ExpectWrite("\xfc\x07"_su8, Opcode::I64TruncSatF64U);
}

TEST(BinaryWriteTest, Opcode_bulk_memory) {
  ExpectWrite("\xfc\x08"_su8, Opcode::MemoryInit);
  ExpectWrite("\xfc\x09"_su8, Opcode::DataDrop);
  ExpectWrite("\xfc\x0a"_su8, Opcode::MemoryCopy);
  ExpectWrite("\xfc\x0b"_su8, Opcode::MemoryFill);
  ExpectWrite("\xfc\x0c"_su8, Opcode::TableInit);
  ExpectWrite("\xfc\x0d"_su8, Opcode::ElemDrop);
  ExpectWrite("\xfc\x0e"_su8, Opcode::TableCopy);
}

TEST(BinaryWriteTest, Opcode_simd) {
  using O = Opcode;

  ExpectWrite("\xfd\x00"_su8, O::V128Load);
  ExpectWrite("\xfd\x01"_su8, O::I16X8Load8X8S);
  ExpectWrite("\xfd\x02"_su8, O::I16X8Load8X8U);
  ExpectWrite("\xfd\x03"_su8, O::I32X4Load16X4S);
  ExpectWrite("\xfd\x04"_su8, O::I32X4Load16X4U);
  ExpectWrite("\xfd\x05"_su8, O::I64X2Load32X2S);
  ExpectWrite("\xfd\x06"_su8, O::I64X2Load32X2U);
  ExpectWrite("\xfd\x07"_su8, O::V8X16LoadSplat);
  ExpectWrite("\xfd\x08"_su8, O::V16X8LoadSplat);
  ExpectWrite("\xfd\x09"_su8, O::V32X4LoadSplat);
  ExpectWrite("\xfd\x0a"_su8, O::V64X2LoadSplat);
  ExpectWrite("\xfd\x0b"_su8, O::V128Store);
  ExpectWrite("\xfd\x0c"_su8, O::V128Const);
  ExpectWrite("\xfd\x0d"_su8, O::V8X16Shuffle);
  ExpectWrite("\xfd\x0e"_su8, O::V8X16Swizzle);
  ExpectWrite("\xfd\x0f"_su8, O::I8X16Splat);
  ExpectWrite("\xfd\x10"_su8, O::I16X8Splat);
  ExpectWrite("\xfd\x11"_su8, O::I32X4Splat);
  ExpectWrite("\xfd\x12"_su8, O::I64X2Splat);
  ExpectWrite("\xfd\x13"_su8, O::F32X4Splat);
  ExpectWrite("\xfd\x14"_su8, O::F64X2Splat);
  ExpectWrite("\xfd\x15"_su8, O::I8X16ExtractLaneS);
  ExpectWrite("\xfd\x16"_su8, O::I8X16ExtractLaneU);
  ExpectWrite("\xfd\x17"_su8, O::I8X16ReplaceLane);
  ExpectWrite("\xfd\x18"_su8, O::I16X8ExtractLaneS);
  ExpectWrite("\xfd\x19"_su8, O::I16X8ExtractLaneU);
  ExpectWrite("\xfd\x1a"_su8, O::I16X8ReplaceLane);
  ExpectWrite("\xfd\x1b"_su8, O::I32X4ExtractLane);
  ExpectWrite("\xfd\x1c"_su8, O::I32X4ReplaceLane);
  ExpectWrite("\xfd\x1d"_su8, O::I64X2ExtractLane);
  ExpectWrite("\xfd\x1e"_su8, O::I64X2ReplaceLane);
  ExpectWrite("\xfd\x1f"_su8, O::F32X4ExtractLane);
  ExpectWrite("\xfd\x20"_su8, O::F32X4ReplaceLane);
  ExpectWrite("\xfd\x21"_su8, O::F64X2ExtractLane);
  ExpectWrite("\xfd\x22"_su8, O::F64X2ReplaceLane);
  ExpectWrite("\xfd\x23"_su8, O::I8X16Eq);
  ExpectWrite("\xfd\x24"_su8, O::I8X16Ne);
  ExpectWrite("\xfd\x25"_su8, O::I8X16LtS);
  ExpectWrite("\xfd\x26"_su8, O::I8X16LtU);
  ExpectWrite("\xfd\x27"_su8, O::I8X16GtS);
  ExpectWrite("\xfd\x28"_su8, O::I8X16GtU);
  ExpectWrite("\xfd\x29"_su8, O::I8X16LeS);
  ExpectWrite("\xfd\x2a"_su8, O::I8X16LeU);
  ExpectWrite("\xfd\x2b"_su8, O::I8X16GeS);
  ExpectWrite("\xfd\x2c"_su8, O::I8X16GeU);
  ExpectWrite("\xfd\x2d"_su8, O::I16X8Eq);
  ExpectWrite("\xfd\x2e"_su8, O::I16X8Ne);
  ExpectWrite("\xfd\x2f"_su8, O::I16X8LtS);
  ExpectWrite("\xfd\x30"_su8, O::I16X8LtU);
  ExpectWrite("\xfd\x31"_su8, O::I16X8GtS);
  ExpectWrite("\xfd\x32"_su8, O::I16X8GtU);
  ExpectWrite("\xfd\x33"_su8, O::I16X8LeS);
  ExpectWrite("\xfd\x34"_su8, O::I16X8LeU);
  ExpectWrite("\xfd\x35"_su8, O::I16X8GeS);
  ExpectWrite("\xfd\x36"_su8, O::I16X8GeU);
  ExpectWrite("\xfd\x37"_su8, O::I32X4Eq);
  ExpectWrite("\xfd\x38"_su8, O::I32X4Ne);
  ExpectWrite("\xfd\x39"_su8, O::I32X4LtS);
  ExpectWrite("\xfd\x3a"_su8, O::I32X4LtU);
  ExpectWrite("\xfd\x3b"_su8, O::I32X4GtS);
  ExpectWrite("\xfd\x3c"_su8, O::I32X4GtU);
  ExpectWrite("\xfd\x3d"_su8, O::I32X4LeS);
  ExpectWrite("\xfd\x3e"_su8, O::I32X4LeU);
  ExpectWrite("\xfd\x3f"_su8, O::I32X4GeS);
  ExpectWrite("\xfd\x40"_su8, O::I32X4GeU);
  ExpectWrite("\xfd\x41"_su8, O::F32X4Eq);
  ExpectWrite("\xfd\x42"_su8, O::F32X4Ne);
  ExpectWrite("\xfd\x43"_su8, O::F32X4Lt);
  ExpectWrite("\xfd\x44"_su8, O::F32X4Gt);
  ExpectWrite("\xfd\x45"_su8, O::F32X4Le);
  ExpectWrite("\xfd\x46"_su8, O::F32X4Ge);
  ExpectWrite("\xfd\x47"_su8, O::F64X2Eq);
  ExpectWrite("\xfd\x48"_su8, O::F64X2Ne);
  ExpectWrite("\xfd\x49"_su8, O::F64X2Lt);
  ExpectWrite("\xfd\x4a"_su8, O::F64X2Gt);
  ExpectWrite("\xfd\x4b"_su8, O::F64X2Le);
  ExpectWrite("\xfd\x4c"_su8, O::F64X2Ge);
  ExpectWrite("\xfd\x4d"_su8, O::V128Not);
  ExpectWrite("\xfd\x4e"_su8, O::V128And);
  ExpectWrite("\xfd\x4f"_su8, O::V128Andnot);
  ExpectWrite("\xfd\x50"_su8, O::V128Or);
  ExpectWrite("\xfd\x51"_su8, O::V128Xor);
  ExpectWrite("\xfd\x52"_su8, O::V128BitSelect);
  ExpectWrite("\xfd\x60"_su8, O::I8X16Abs);
  ExpectWrite("\xfd\x61"_su8, O::I8X16Neg);
  ExpectWrite("\xfd\x62"_su8, O::I8X16AnyTrue);
  ExpectWrite("\xfd\x63"_su8, O::I8X16AllTrue);
  ExpectWrite("\xfd\x65"_su8, O::I8X16NarrowI16X8S);
  ExpectWrite("\xfd\x66"_su8, O::I8X16NarrowI16X8U);
  ExpectWrite("\xfd\x6b"_su8, O::I8X16Shl);
  ExpectWrite("\xfd\x6c"_su8, O::I8X16ShrS);
  ExpectWrite("\xfd\x6d"_su8, O::I8X16ShrU);
  ExpectWrite("\xfd\x6e"_su8, O::I8X16Add);
  ExpectWrite("\xfd\x6f"_su8, O::I8X16AddSaturateS);
  ExpectWrite("\xfd\x70"_su8, O::I8X16AddSaturateU);
  ExpectWrite("\xfd\x71"_su8, O::I8X16Sub);
  ExpectWrite("\xfd\x72"_su8, O::I8X16SubSaturateS);
  ExpectWrite("\xfd\x73"_su8, O::I8X16SubSaturateU);
  ExpectWrite("\xfd\x76"_su8, O::I8X16MinS);
  ExpectWrite("\xfd\x77"_su8, O::I8X16MinU);
  ExpectWrite("\xfd\x78"_su8, O::I8X16MaxS);
  ExpectWrite("\xfd\x79"_su8, O::I8X16MaxU);
  ExpectWrite("\xfd\x7b"_su8, O::I8X16AvgrU);
  ExpectWrite("\xfd\x80\x01"_su8, O::I16X8Abs);
  ExpectWrite("\xfd\x81\x01"_su8, O::I16X8Neg);
  ExpectWrite("\xfd\x82\x01"_su8, O::I16X8AnyTrue);
  ExpectWrite("\xfd\x83\x01"_su8, O::I16X8AllTrue);
  ExpectWrite("\xfd\x85\x01"_su8, O::I16X8NarrowI32X4S);
  ExpectWrite("\xfd\x86\x01"_su8, O::I16X8NarrowI32X4U);
  ExpectWrite("\xfd\x87\x01"_su8, O::I16X8WidenLowI8X16S);
  ExpectWrite("\xfd\x88\x01"_su8, O::I16X8WidenHighI8X16S);
  ExpectWrite("\xfd\x89\x01"_su8, O::I16X8WidenLowI8X16U);
  ExpectWrite("\xfd\x8a\x01"_su8, O::I16X8WidenHighI8X16U);
  ExpectWrite("\xfd\x8b\x01"_su8, O::I16X8Shl);
  ExpectWrite("\xfd\x8c\x01"_su8, O::I16X8ShrS);
  ExpectWrite("\xfd\x8d\x01"_su8, O::I16X8ShrU);
  ExpectWrite("\xfd\x8e\x01"_su8, O::I16X8Add);
  ExpectWrite("\xfd\x8f\x01"_su8, O::I16X8AddSaturateS);
  ExpectWrite("\xfd\x90\x01"_su8, O::I16X8AddSaturateU);
  ExpectWrite("\xfd\x91\x01"_su8, O::I16X8Sub);
  ExpectWrite("\xfd\x92\x01"_su8, O::I16X8SubSaturateS);
  ExpectWrite("\xfd\x93\x01"_su8, O::I16X8SubSaturateU);
  ExpectWrite("\xfd\x95\x01"_su8, O::I16X8Mul);
  ExpectWrite("\xfd\x96\x01"_su8, O::I16X8MinS);
  ExpectWrite("\xfd\x97\x01"_su8, O::I16X8MinU);
  ExpectWrite("\xfd\x98\x01"_su8, O::I16X8MaxS);
  ExpectWrite("\xfd\x99\x01"_su8, O::I16X8MaxU);
  ExpectWrite("\xfd\x9b\x01"_su8, O::I16X8AvgrU);
  ExpectWrite("\xfd\xa0\x01"_su8, O::I32X4Abs);
  ExpectWrite("\xfd\xa1\x01"_su8, O::I32X4Neg);
  ExpectWrite("\xfd\xa2\x01"_su8, O::I32X4AnyTrue);
  ExpectWrite("\xfd\xa3\x01"_su8, O::I32X4AllTrue);
  ExpectWrite("\xfd\xa7\x01"_su8, O::I32X4WidenLowI16X8S);
  ExpectWrite("\xfd\xa8\x01"_su8, O::I32X4WidenHighI16X8S);
  ExpectWrite("\xfd\xa9\x01"_su8, O::I32X4WidenLowI16X8U);
  ExpectWrite("\xfd\xaa\x01"_su8, O::I32X4WidenHighI16X8U);
  ExpectWrite("\xfd\xab\x01"_su8, O::I32X4Shl);
  ExpectWrite("\xfd\xac\x01"_su8, O::I32X4ShrS);
  ExpectWrite("\xfd\xad\x01"_su8, O::I32X4ShrU);
  ExpectWrite("\xfd\xae\x01"_su8, O::I32X4Add);
  ExpectWrite("\xfd\xb1\x01"_su8, O::I32X4Sub);
  ExpectWrite("\xfd\xb5\x01"_su8, O::I32X4Mul);
  ExpectWrite("\xfd\xb6\x01"_su8, O::I32X4MinS);
  ExpectWrite("\xfd\xb7\x01"_su8, O::I32X4MinU);
  ExpectWrite("\xfd\xb8\x01"_su8, O::I32X4MaxS);
  ExpectWrite("\xfd\xb9\x01"_su8, O::I32X4MaxU);
  ExpectWrite("\xfd\xc1\x01"_su8, O::I64X2Neg);
  ExpectWrite("\xfd\xcb\x01"_su8, O::I64X2Shl);
  ExpectWrite("\xfd\xcc\x01"_su8, O::I64X2ShrS);
  ExpectWrite("\xfd\xcd\x01"_su8, O::I64X2ShrU);
  ExpectWrite("\xfd\xce\x01"_su8, O::I64X2Add);
  ExpectWrite("\xfd\xd1\x01"_su8, O::I64X2Sub);
  ExpectWrite("\xfd\xd5\x01"_su8, O::I64X2Mul);
  ExpectWrite("\xfd\xe0\x01"_su8, O::F32X4Abs);
  ExpectWrite("\xfd\xe1\x01"_su8, O::F32X4Neg);
  ExpectWrite("\xfd\xe3\x01"_su8, O::F32X4Sqrt);
  ExpectWrite("\xfd\xe4\x01"_su8, O::F32X4Add);
  ExpectWrite("\xfd\xe5\x01"_su8, O::F32X4Sub);
  ExpectWrite("\xfd\xe6\x01"_su8, O::F32X4Mul);
  ExpectWrite("\xfd\xe7\x01"_su8, O::F32X4Div);
  ExpectWrite("\xfd\xe8\x01"_su8, O::F32X4Min);
  ExpectWrite("\xfd\xe9\x01"_su8, O::F32X4Max);
  ExpectWrite("\xfd\xec\x01"_su8, O::F64X2Abs);
  ExpectWrite("\xfd\xed\x01"_su8, O::F64X2Neg);
  ExpectWrite("\xfd\xef\x01"_su8, O::F64X2Sqrt);
  ExpectWrite("\xfd\xf0\x01"_su8, O::F64X2Add);
  ExpectWrite("\xfd\xf1\x01"_su8, O::F64X2Sub);
  ExpectWrite("\xfd\xf2\x01"_su8, O::F64X2Mul);
  ExpectWrite("\xfd\xf3\x01"_su8, O::F64X2Div);
  ExpectWrite("\xfd\xf4\x01"_su8, O::F64X2Min);
  ExpectWrite("\xfd\xf5\x01"_su8, O::F64X2Max);
  ExpectWrite("\xfd\xf8\x01"_su8, O::I32X4TruncSatF32X4S);
  ExpectWrite("\xfd\xf9\x01"_su8, O::I32X4TruncSatF32X4U);
  ExpectWrite("\xfd\xfa\x01"_su8, O::F32X4ConvertI32X4S);
  ExpectWrite("\xfd\xfb\x01"_su8, O::F32X4ConvertI32X4U);
}

TEST(BinaryWriteTest, Opcode_threads) {
  using O = Opcode;

  ExpectWrite("\xfe\x00"_su8, O::MemoryAtomicNotify);
  ExpectWrite("\xfe\x01"_su8, O::MemoryAtomicWait32);
  ExpectWrite("\xfe\x02"_su8, O::MemoryAtomicWait64);
  ExpectWrite("\xfe\x10"_su8, O::I32AtomicLoad);
  ExpectWrite("\xfe\x11"_su8, O::I64AtomicLoad);
  ExpectWrite("\xfe\x12"_su8, O::I32AtomicLoad8U);
  ExpectWrite("\xfe\x13"_su8, O::I32AtomicLoad16U);
  ExpectWrite("\xfe\x14"_su8, O::I64AtomicLoad8U);
  ExpectWrite("\xfe\x15"_su8, O::I64AtomicLoad16U);
  ExpectWrite("\xfe\x16"_su8, O::I64AtomicLoad32U);
  ExpectWrite("\xfe\x17"_su8, O::I32AtomicStore);
  ExpectWrite("\xfe\x18"_su8, O::I64AtomicStore);
  ExpectWrite("\xfe\x19"_su8, O::I32AtomicStore8);
  ExpectWrite("\xfe\x1a"_su8, O::I32AtomicStore16);
  ExpectWrite("\xfe\x1b"_su8, O::I64AtomicStore8);
  ExpectWrite("\xfe\x1c"_su8, O::I64AtomicStore16);
  ExpectWrite("\xfe\x1d"_su8, O::I64AtomicStore32);
  ExpectWrite("\xfe\x1e"_su8, O::I32AtomicRmwAdd);
  ExpectWrite("\xfe\x1f"_su8, O::I64AtomicRmwAdd);
  ExpectWrite("\xfe\x20"_su8, O::I32AtomicRmw8AddU);
  ExpectWrite("\xfe\x21"_su8, O::I32AtomicRmw16AddU);
  ExpectWrite("\xfe\x22"_su8, O::I64AtomicRmw8AddU);
  ExpectWrite("\xfe\x23"_su8, O::I64AtomicRmw16AddU);
  ExpectWrite("\xfe\x24"_su8, O::I64AtomicRmw32AddU);
  ExpectWrite("\xfe\x25"_su8, O::I32AtomicRmwSub);
  ExpectWrite("\xfe\x26"_su8, O::I64AtomicRmwSub);
  ExpectWrite("\xfe\x27"_su8, O::I32AtomicRmw8SubU);
  ExpectWrite("\xfe\x28"_su8, O::I32AtomicRmw16SubU);
  ExpectWrite("\xfe\x29"_su8, O::I64AtomicRmw8SubU);
  ExpectWrite("\xfe\x2a"_su8, O::I64AtomicRmw16SubU);
  ExpectWrite("\xfe\x2b"_su8, O::I64AtomicRmw32SubU);
  ExpectWrite("\xfe\x2c"_su8, O::I32AtomicRmwAnd);
  ExpectWrite("\xfe\x2d"_su8, O::I64AtomicRmwAnd);
  ExpectWrite("\xfe\x2e"_su8, O::I32AtomicRmw8AndU);
  ExpectWrite("\xfe\x2f"_su8, O::I32AtomicRmw16AndU);
  ExpectWrite("\xfe\x30"_su8, O::I64AtomicRmw8AndU);
  ExpectWrite("\xfe\x31"_su8, O::I64AtomicRmw16AndU);
  ExpectWrite("\xfe\x32"_su8, O::I64AtomicRmw32AndU);
  ExpectWrite("\xfe\x33"_su8, O::I32AtomicRmwOr);
  ExpectWrite("\xfe\x34"_su8, O::I64AtomicRmwOr);
  ExpectWrite("\xfe\x35"_su8, O::I32AtomicRmw8OrU);
  ExpectWrite("\xfe\x36"_su8, O::I32AtomicRmw16OrU);
  ExpectWrite("\xfe\x37"_su8, O::I64AtomicRmw8OrU);
  ExpectWrite("\xfe\x38"_su8, O::I64AtomicRmw16OrU);
  ExpectWrite("\xfe\x39"_su8, O::I64AtomicRmw32OrU);
  ExpectWrite("\xfe\x3a"_su8, O::I32AtomicRmwXor);
  ExpectWrite("\xfe\x3b"_su8, O::I64AtomicRmwXor);
  ExpectWrite("\xfe\x3c"_su8, O::I32AtomicRmw8XorU);
  ExpectWrite("\xfe\x3d"_su8, O::I32AtomicRmw16XorU);
  ExpectWrite("\xfe\x3e"_su8, O::I64AtomicRmw8XorU);
  ExpectWrite("\xfe\x3f"_su8, O::I64AtomicRmw16XorU);
  ExpectWrite("\xfe\x40"_su8, O::I64AtomicRmw32XorU);
  ExpectWrite("\xfe\x41"_su8, O::I32AtomicRmwXchg);
  ExpectWrite("\xfe\x42"_su8, O::I64AtomicRmwXchg);
  ExpectWrite("\xfe\x43"_su8, O::I32AtomicRmw8XchgU);
  ExpectWrite("\xfe\x44"_su8, O::I32AtomicRmw16XchgU);
  ExpectWrite("\xfe\x45"_su8, O::I64AtomicRmw8XchgU);
  ExpectWrite("\xfe\x46"_su8, O::I64AtomicRmw16XchgU);
  ExpectWrite("\xfe\x47"_su8, O::I64AtomicRmw32XchgU);
  ExpectWrite("\xfe\x48"_su8, O::I32AtomicRmwCmpxchg);
  ExpectWrite("\xfe\x49"_su8, O::I64AtomicRmwCmpxchg);
  ExpectWrite("\xfe\x4a"_su8, O::I32AtomicRmw8CmpxchgU);
  ExpectWrite("\xfe\x4b"_su8, O::I32AtomicRmw16CmpxchgU);
  ExpectWrite("\xfe\x4c"_su8, O::I64AtomicRmw8CmpxchgU);
  ExpectWrite("\xfe\x4d"_su8, O::I64AtomicRmw16CmpxchgU);
  ExpectWrite("\xfe\x4e"_su8, O::I64AtomicRmw32CmpxchgU);
}

TEST(BinaryWriteTest, S32) {
  ExpectWrite("\x20"_su8, s32{32});
  ExpectWrite("\x70"_su8, s32{-16});
  ExpectWrite("\xc0\x03"_su8, s32{448});
  ExpectWrite("\xc0\x63"_su8, s32{-3648});
  ExpectWrite("\xd0\x84\x02"_su8, s32{33360});
  ExpectWrite("\xd0\x84\x52"_su8, s32{-753072});
  ExpectWrite("\xa0\xb0\xc0\x30"_su8, s32{101718048});
  ExpectWrite("\xa0\xb0\xc0\x70"_su8, s32{-32499680});
  ExpectWrite("\xf0\xf0\xf0\xf0\x03"_su8, s32{1042036848});
  ExpectWrite("\xf0\xf0\xf0\xf0\x7c"_su8, s32{-837011344});
}

TEST(BinaryWriteTest, S64) {
  ExpectWrite("\x20"_su8, s64{32});
  ExpectWrite("\x70"_su8, s64{-16});
  ExpectWrite("\xc0\x03"_su8, s64{448});
  ExpectWrite("\xc0\x63"_su8, s64{-3648});
  ExpectWrite("\xd0\x84\x02"_su8, s64{33360});
  ExpectWrite("\xd0\x84\x52"_su8, s64{-753072});
  ExpectWrite("\xa0\xb0\xc0\x30"_su8, s64{101718048});
  ExpectWrite("\xa0\xb0\xc0\x70"_su8, s64{-32499680});
  ExpectWrite("\xf0\xf0\xf0\xf0\x03"_su8, s64{1042036848});
  ExpectWrite("\xf0\xf0\xf0\xf0\x7c"_su8, s64{-837011344});
  ExpectWrite("\xe0\xe0\xe0\xe0\x33"_su8, s64{13893120096});
  ExpectWrite("\xe0\xe0\xe0\xe0\x51"_su8, s64{-12413554592});
  ExpectWrite("\xd0\xd0\xd0\xd0\xd0\x2c"_su8, s64{1533472417872});
  ExpectWrite("\xd0\xd0\xd0\xd0\xd0\x77"_su8, s64{-287593715632});
  ExpectWrite("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"_su8, s64{139105536057408});
  ExpectWrite("\xc0\xc0\xc0\xc0\xc0\xd0\x63"_su8, s64{-124777254608832});
  ExpectWrite("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"_su8, s64{1338117014066474});
  ExpectWrite("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"_su8, s64{-12172681868045014});
  ExpectWrite("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"_su8, s64{1070725794579330814});
  ExpectWrite("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"_su8, s64{-3540960223848057090});
}

TEST(BinaryWriteTest, SectionId) {
  ExpectWrite("\x00"_su8, SectionId::Custom);
  ExpectWrite("\x01"_su8, SectionId::Type);
  ExpectWrite("\x02"_su8, SectionId::Import);
  ExpectWrite("\x03"_su8, SectionId::Function);
  ExpectWrite("\x04"_su8, SectionId::Table);
  ExpectWrite("\x05"_su8, SectionId::Memory);
  ExpectWrite("\x06"_su8, SectionId::Global);
  ExpectWrite("\x07"_su8, SectionId::Export);
  ExpectWrite("\x08"_su8, SectionId::Start);
  ExpectWrite("\x09"_su8, SectionId::Element);
  ExpectWrite("\x0a"_su8, SectionId::Code);
  ExpectWrite("\x0b"_su8, SectionId::Data);
  ExpectWrite("\x0c"_su8, SectionId::DataCount);
  ExpectWrite("\x0d"_su8, SectionId::Event);
}

TEST(BinaryWriteTest, ShuffleImmediate) {
  ExpectWrite(
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
      ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
}

TEST(BinaryWriteTest, Start) {
  ExpectWrite("\x80\x02"_su8, Start{256});
}

TEST(BinaryWriteTest, String) {
  ExpectWrite("\x05hello"_su8, "hello");
  ExpectWrite("\x02hi"_su8, std::string{"hi"});
}

TEST(BinaryWriteTest, Table) {
  ExpectWrite("\x70\x00\x01"_su8,
                     Table{TableType{Limits{1}, ReferenceType::Funcref}});
}

TEST(BinaryWriteTest, TableType) {
  ExpectWrite("\x70\x00\x01"_su8,
                         TableType{Limits{1}, ReferenceType::Funcref});
  ExpectWrite("\x70\x01\x01\x02"_su8,
                         TableType{Limits{1, 2}, ReferenceType::Funcref});
}

TEST(BinaryWriteTest, TypeEntry) {
  ExpectWrite("\x60\x00\x01\x7f"_su8,
                         TypeEntry{FunctionType{{}, {ValueType::I32}}});
}

TEST(BinaryWriteTest, U8) {
  ExpectWrite("\x2a"_su8, u8{42});
}

TEST(BinaryWriteTest, U32) {
  ExpectWrite("\x20"_su8, u32{32u});
  ExpectWrite("\xc0\x03"_su8, u32{448u});
  ExpectWrite("\xd0\x84\x02"_su8, u32{33360u});
  ExpectWrite("\xa0\xb0\xc0\x30"_su8, u32{101718048u});
  ExpectWrite("\xf0\xf0\xf0\xf0\x03"_su8, u32{1042036848u});
}

TEST(BinaryWriteTest, ValueType) {
  ExpectWrite("\x7f"_su8, ValueType::I32);
  ExpectWrite("\x7e"_su8, ValueType::I64);
  ExpectWrite("\x7d"_su8, ValueType::F32);
  ExpectWrite("\x7c"_su8, ValueType::F64);
  ExpectWrite("\x7b"_su8, ValueType::V128);
  ExpectWrite("\x70"_su8, ValueType::Funcref);
  ExpectWrite("\x6f"_su8, ValueType::Externref);
  ExpectWrite("\x68"_su8, ValueType::Exnref);
}

TEST(BinaryWriteTest, WriteVector_u8) {
  const auto expected = "\x05hello"_su8;
  const std::vector<u8> input{{'h', 'e', 'l', 'l', 'o'}};
  std::vector<u8> output(expected.size());
  auto iter = WriteVector(input.begin(), input.end(),
                          MakeClampedIterator(output.begin(), output.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), output.end());
  EXPECT_EQ(expected, SpanU8{output});
}

TEST(BinaryWriteTest, WriteVector_u32) {
  const auto expected =
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c"_su8;
  const std::vector<u32> input{5, 128, 206412};
  std::vector<u8> output(expected.size());
  auto iter = WriteVector(input.begin(), input.end(),
                          MakeClampedIterator(output.begin(), output.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), output.end());
  EXPECT_EQ(expected, SpanU8{output});
}
