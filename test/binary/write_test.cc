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
#include <iterator>
#include <string>
#include <vector>

#include "gtest/gtest.h"

// Write() functions must be declared here before they can be used by
// ExpectWrite (defined in write_test_utils.h below).
#include "wasp/binary/write/write_block_type.h"
#include "wasp/binary/write/write_br_table_immediate.h"
#include "wasp/binary/write/write_bytes.h"
#include "wasp/binary/write/write_call_indirect_immediate.h"
#include "wasp/binary/write/write_constant_expression.h"
#include "wasp/binary/write/write_copy_immediate.h"
#include "wasp/binary/write/write_data_segment.h"
#include "wasp/binary/write/write_element_expression.h"
#include "wasp/binary/write/write_element_segment.h"
#include "wasp/binary/write/write_element_type.h"
#include "wasp/binary/write/write_export.h"
#include "wasp/binary/write/write_external_kind.h"
#include "wasp/binary/write/write_f32.h"
#include "wasp/binary/write/write_f64.h"
#include "wasp/binary/write/write_fixed_var_int.h"
#include "wasp/binary/write/write_function.h"
#include "wasp/binary/write/write_function_type.h"
#include "wasp/binary/write/write_global.h"
#include "wasp/binary/write/write_global_type.h"
#include "wasp/binary/write/write_import.h"
#include "wasp/binary/write/write_init_immediate.h"
#include "wasp/binary/write/write_instruction.h"
#include "wasp/binary/write/write_locals.h"
#include "wasp/binary/write/write_mem_arg_immediate.h"
#include "wasp/binary/write/write_memory.h"
#include "wasp/binary/write/write_memory_type.h"
#include "wasp/binary/write/write_mutability.h"
#include "wasp/binary/write/write_name_subsection_id.h"
#include "wasp/binary/write/write_opcode.h"
#include "wasp/binary/write/write_s32.h"
#include "wasp/binary/write/write_s64.h"
#include "wasp/binary/write/write_section_id.h"
#include "wasp/binary/write/write_shuffle_immediate.h"
#include "wasp/binary/write/write_start.h"
#include "wasp/binary/write/write_string.h"
#include "wasp/binary/write/write_table.h"
#include "wasp/binary/write/write_table_type.h"
#include "wasp/binary/write/write_type_entry.h"
#include "wasp/binary/write/write_u32.h"
#include "wasp/binary/write/write_u8.h"
#include "wasp/binary/write/write_value_type.h"
#include "wasp/binary/write/write_vector.h"

#include "test/binary/test_utils.h"
#include "test/binary/write_test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(WriteTest, BlockType) {
  ExpectWrite<BlockType>("\x7f"_su8, BlockType::I32);
  ExpectWrite<BlockType>("\x7e"_su8, BlockType::I64);
  ExpectWrite<BlockType>("\x7d"_su8, BlockType::F32);
  ExpectWrite<BlockType>("\x7c"_su8, BlockType::F64);
  ExpectWrite<BlockType>("\x7b"_su8, BlockType::V128);
  ExpectWrite<BlockType>("\x6f"_su8, BlockType::Anyref);
  ExpectWrite<BlockType>("\x40"_su8, BlockType::Void);
}

TEST(WriteTest, BrOnExnImmediate) {
  ExpectWrite<BrOnExnImmediate>("\x00\x00"_su8, BrOnExnImmediate{0, 0});
}

TEST(WriteTest, BrTableImmediate) {
  ExpectWrite<BrTableImmediate>("\x00\x00"_su8, BrTableImmediate{{}, 0});
  ExpectWrite<BrTableImmediate>("\x02\x01\x02\x03"_su8,
                                BrTableImmediate{{1, 2}, 3});
}

TEST(WriteTest, Bytes) {
  const std::vector<u8> input{{0x12, 0x34, 0x56}};
  std::vector<u8> output;
  WriteBytes(input, std::back_inserter(output));
  EXPECT_EQ(input, output);
}

TEST(WriteTest, CallIndirectImmediate) {
  ExpectWrite<CallIndirectImmediate>("\x01\x00"_su8,
                                     CallIndirectImmediate{1, 0});
  ExpectWrite<CallIndirectImmediate>("\x80\x01\x00"_su8,
                                     CallIndirectImmediate{128, 0});
}

TEST(WriteTest, ConstantExpression) {
  // i32.const
  ExpectWrite<ConstantExpression>(
      "\x41\x00\x0b"_su8,
      ConstantExpression{Instruction{Opcode::I32Const, s32{0}}});

  // i64.const
  ExpectWrite<ConstantExpression>(
      "\x42\x80\x80\x80\x80\x80\x01\x0b"_su8,
      ConstantExpression{Instruction{Opcode::I64Const, s64{34359738368}}});

  // f32.const
  ExpectWrite<ConstantExpression>(
      "\x43\x00\x00\x00\x00\x0b"_su8,
      ConstantExpression{Instruction{Opcode::F32Const, f32{0}}});

  // f64.const
  ExpectWrite<ConstantExpression>(
      "\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b"_su8,
      ConstantExpression{Instruction{Opcode::F64Const, f64{0}}});

  // global.get
  ExpectWrite<ConstantExpression>(
      "\x23\x00\x0b"_su8,
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}});
}

TEST(WriteTest, CopyImmediate) {
  ExpectWrite<CopyImmediate>("\x00\x00"_su8, CopyImmediate{0, 0});
}

TEST(WriteTest, DataSegment) {
  ExpectWrite<DataSegment>(
      "\x00\x42\x01\x0b\x04wxyz"_su8,
      DataSegment{0, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  "wxyz"_su8});
}

TEST(WriteTest, DataSegment_BulkMemory) {
  // Active data segment with non-zero memory index.
  ExpectWrite<DataSegment>(
      "\x02\x01\x42\x01\x0b\x04wxyz"_su8,
      DataSegment{1, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  "wxyz"_su8});

  // Passive data segment.
  ExpectWrite<DataSegment>("\x01\x04wxyz"_su8, DataSegment{"wxyz"_su8});
}

TEST(WriteTest, ElementExpression) {
  // ref.null
  ExpectWrite<ElementExpression>(
      "\xd0\x0b"_su8, ElementExpression{Instruction{Opcode::RefNull}});

  // ref.func 2
  ExpectWrite<ElementExpression>(
      "\xd2\x02\x0b"_su8,
      ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}});
}

TEST(WriteTest, ElementSegment) {
  ExpectWrite<ElementSegment>(
      "\x00\x41\x01\x0b\x03\x01\x02\x03"_su8,
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{1}}},
                     {1, 2, 3}});
}

TEST(WriteTest, ElementSegment_BulkMemory) {
  // Active element segment with non-zero table index.
  ExpectWrite<ElementSegment>(
      "\x02\x01\x41\x01\x0b\x03\x01\x02\x03"_su8,
      ElementSegment{1,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{1}}},
                     {1, 2, 3}});

  // Passive element segment.
  ExpectWrite<ElementSegment>(
      "\x01\x70\x02\xd2\x01\x0b\xd0\x0b"_su8,
      ElementSegment{
          ElementType::Funcref,
          {ElementExpression{Instruction{Opcode::RefFunc, Index{1u}}},
           ElementExpression{Instruction{Opcode::RefNull}}}});
}

TEST(WriteTest, ElementType) {
  ExpectWrite<ElementType>("\x70"_su8, ElementType::Funcref);
}

TEST(WriteTest, Export) {
  ExpectWrite<Export>("\x02hi\x00\x03"_su8,
                      Export{ExternalKind::Function, "hi", 3});
  ExpectWrite<Export>("\x00\x01\xe8\x07"_su8,
                      Export{ExternalKind::Table, "", 1000});
  ExpectWrite<Export>("\x03mem\x02\x00"_su8,
                      Export{ExternalKind::Memory, "mem", 0});
  ExpectWrite<Export>("\x01g\x03\x01"_su8,
                      Export{ExternalKind::Global, "g", 1});
}

TEST(WriteTest, ExternalKind) {
  ExpectWrite<ExternalKind>("\x00"_su8, ExternalKind::Function);
  ExpectWrite<ExternalKind>("\x01"_su8, ExternalKind::Table);
  ExpectWrite<ExternalKind>("\x02"_su8, ExternalKind::Memory);
  ExpectWrite<ExternalKind>("\x03"_su8, ExternalKind::Global);
}

TEST(WriteTest, F32) {
  ExpectWrite<f32>("\x00\x00\x00\x00"_su8, 0.0f);
  ExpectWrite<f32>("\x00\x00\x80\xbf"_su8, -1.0f);
  ExpectWrite<f32>("\x38\xb4\x96\x49"_su8, 1234567.0f);
  ExpectWrite<f32>("\x00\x00\x80\x7f"_su8, INFINITY);
  ExpectWrite<f32>("\x00\x00\x80\xff"_su8, -INFINITY);
  // TODO: NaN
}

TEST(WriteTest, F64) {
  ExpectWrite<f64>("\x00\x00\x00\x00\x00\x00\x00\x00"_su8, 0.0);
  ExpectWrite<f64>("\x00\x00\x00\x00\x00\x00\xf0\xbf"_su8, -1.0);
  ExpectWrite<f64>("\xc0\x71\xbc\x93\x84\x43\xd9\x42"_su8, 111111111111111);
  ExpectWrite<f64>("\x00\x00\x00\x00\x00\x00\xf0\x7f"_su8, INFINITY);
  ExpectWrite<f64>("\x00\x00\x00\x00\x00\x00\xf0\xff"_su8, -INFINITY);
  // TODO: NaN
}

namespace {

template <typename T>
void ExpectWriteFixedVarInt(SpanU8 expected, T value, size_t length) {
  std::vector<wasp::u8> result(expected.size());
  auto iter = wasp::binary::WriteFixedVarInt(
      value, MakeClampedIterator(result.begin(), result.end()), length);
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

}  // namespace

TEST(WriteTest, FixedVarInt_u32) {
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

TEST(WriteTest, FixedVarInt_s32) {
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

TEST(WriteTest, Function) {
  ExpectWrite<Function>("\x01"_su8, Function{1});
}

TEST(WriteTest, FunctionType) {
  ExpectWrite<FunctionType>("\x00\x00"_su8, FunctionType{{}, {}});
  ExpectWrite<FunctionType>(
      "\x02\x7f\x7e\x01\x7c"_su8,
      FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}});
}

TEST(WriteTest, Global) {
  ExpectWrite<Global>(
      "\x7f\x01\x41\x00\x0b"_su8,
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}});
}

TEST(WriteTest, GlobalType) {
  ExpectWrite<GlobalType>("\x7f\x00"_su8,
                          GlobalType{ValueType::I32, Mutability::Const});
  ExpectWrite<GlobalType>("\x7d\x01"_su8,
                          GlobalType{ValueType::F32, Mutability::Var});
}

TEST(WriteTest, Import) {
  ExpectWrite<Import>("\x01\x61\x04\x66unc\x00\x0b"_su8,
                      Import{"a", "func", 11u});

  ExpectWrite<Import>(
      "\x01\x62\x05table\x01\x70\x00\x01"_su8,
      Import{"b", "table", TableType{Limits{1}, ElementType::Funcref}});

  ExpectWrite<Import>("\x01\x63\x06memory\x02\x01\x00\x02"_su8,
                      Import{"c", "memory", MemoryType{Limits{0, 2}}});

  ExpectWrite<Import>(
      "\x01\x64\x06global\x03\x7f\x00"_su8,
      Import{"d", "global", GlobalType{ValueType::I32, Mutability::Const}});
}

TEST(WriteTest, InitImmediate) {
  ExpectWrite<InitImmediate>("\x01\x00"_su8, InitImmediate{1, 0});
  ExpectWrite<InitImmediate>("\x80\x01\x00"_su8, InitImmediate{128, 0});
}

TEST(WriteTest, Instruction) {
  using I = Instruction;
  using O = Opcode;
  using MemArg = MemArgImmediate;

  ExpectWrite<I>("\x00"_su8, I{O::Unreachable});
  ExpectWrite<I>("\x01"_su8, I{O::Nop});
  ExpectWrite<I>("\x02\x7f"_su8, I{O::Block, BlockType::I32});
  ExpectWrite<I>("\x03\x40"_su8, I{O::Loop, BlockType::Void});
  ExpectWrite<I>("\x04\x7c"_su8, I{O::If, BlockType::F64});
  ExpectWrite<I>("\x05"_su8, I{O::Else});
  ExpectWrite<I>("\x0b"_su8, I{O::End});
  ExpectWrite<I>("\x0c\x01"_su8, I{O::Br, Index{1}});
  ExpectWrite<I>("\x0d\x02"_su8, I{O::BrIf, Index{2}});
  ExpectWrite<I>("\x0e\x03\x03\x04\x05\x06"_su8,
                 I{O::BrTable, BrTableImmediate{{3, 4, 5}, 6}});
  ExpectWrite<I>("\x0f"_su8, I{O::Return});
  ExpectWrite<I>("\x10\x07"_su8, I{O::Call, Index{7}});
  ExpectWrite<I>("\x11\x08\x00"_su8,
                 I{O::CallIndirect, CallIndirectImmediate{8, 0}});
  ExpectWrite<I>("\x1a"_su8, I{O::Drop});
  ExpectWrite<I>("\x1b"_su8, I{O::Select});
  ExpectWrite<I>("\x20\x05"_su8, I{O::LocalGet, Index{5}});
  ExpectWrite<I>("\x21\x06"_su8, I{O::LocalSet, Index{6}});
  ExpectWrite<I>("\x22\x07"_su8, I{O::LocalTee, Index{7}});
  ExpectWrite<I>("\x23\x08"_su8, I{O::GlobalGet, Index{8}});
  ExpectWrite<I>("\x24\x09"_su8, I{O::GlobalSet, Index{9}});
  ExpectWrite<I>("\x28\x0a\x0b"_su8, I{O::I32Load, MemArg{10, 11}});
  ExpectWrite<I>("\x29\x0c\x0d"_su8, I{O::I64Load, MemArg{12, 13}});
  ExpectWrite<I>("\x2a\x0e\x0f"_su8, I{O::F32Load, MemArg{14, 15}});
  ExpectWrite<I>("\x2b\x10\x11"_su8, I{O::F64Load, MemArg{16, 17}});
  ExpectWrite<I>("\x2c\x12\x13"_su8, I{O::I32Load8S, MemArg{18, 19}});
  ExpectWrite<I>("\x2d\x14\x15"_su8, I{O::I32Load8U, MemArg{20, 21}});
  ExpectWrite<I>("\x2e\x16\x17"_su8, I{O::I32Load16S, MemArg{22, 23}});
  ExpectWrite<I>("\x2f\x18\x19"_su8, I{O::I32Load16U, MemArg{24, 25}});
  ExpectWrite<I>("\x30\x1a\x1b"_su8, I{O::I64Load8S, MemArg{26, 27}});
  ExpectWrite<I>("\x31\x1c\x1d"_su8, I{O::I64Load8U, MemArg{28, 29}});
  ExpectWrite<I>("\x32\x1e\x1f"_su8, I{O::I64Load16S, MemArg{30, 31}});
  ExpectWrite<I>("\x33\x20\x21"_su8, I{O::I64Load16U, MemArg{32, 33}});
  ExpectWrite<I>("\x34\x22\x23"_su8, I{O::I64Load32S, MemArg{34, 35}});
  ExpectWrite<I>("\x35\x24\x25"_su8, I{O::I64Load32U, MemArg{36, 37}});
  ExpectWrite<I>("\x36\x26\x27"_su8, I{O::I32Store, MemArg{38, 39}});
  ExpectWrite<I>("\x37\x28\x29"_su8, I{O::I64Store, MemArg{40, 41}});
  ExpectWrite<I>("\x38\x2a\x2b"_su8, I{O::F32Store, MemArg{42, 43}});
  ExpectWrite<I>("\x39\x2c\x2d"_su8, I{O::F64Store, MemArg{44, 45}});
  ExpectWrite<I>("\x3a\x2e\x2f"_su8, I{O::I32Store8, MemArg{46, 47}});
  ExpectWrite<I>("\x3b\x30\x31"_su8, I{O::I32Store16, MemArg{48, 49}});
  ExpectWrite<I>("\x3c\x32\x33"_su8, I{O::I64Store8, MemArg{50, 51}});
  ExpectWrite<I>("\x3d\x34\x35"_su8, I{O::I64Store16, MemArg{52, 53}});
  ExpectWrite<I>("\x3e\x36\x37"_su8, I{O::I64Store32, MemArg{54, 55}});
  ExpectWrite<I>("\x3f\x00"_su8, I{O::MemorySize, u8{0}});
  ExpectWrite<I>("\x40\x00"_su8, I{O::MemoryGrow, u8{0}});
  ExpectWrite<I>("\x41\x00"_su8, I{O::I32Const, s32{0}});
  ExpectWrite<I>("\x42\x00"_su8, I{O::I64Const, s64{0}});
  ExpectWrite<I>("\x43\x00\x00\x00\x00"_su8, I{O::F32Const, f32{0}});
  ExpectWrite<I>("\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
                 I{O::F64Const, f64{0}});
  ExpectWrite<I>("\x45"_su8, I{O::I32Eqz});
  ExpectWrite<I>("\x46"_su8, I{O::I32Eq});
  ExpectWrite<I>("\x47"_su8, I{O::I32Ne});
  ExpectWrite<I>("\x48"_su8, I{O::I32LtS});
  ExpectWrite<I>("\x49"_su8, I{O::I32LtU});
  ExpectWrite<I>("\x4a"_su8, I{O::I32GtS});
  ExpectWrite<I>("\x4b"_su8, I{O::I32GtU});
  ExpectWrite<I>("\x4c"_su8, I{O::I32LeS});
  ExpectWrite<I>("\x4d"_su8, I{O::I32LeU});
  ExpectWrite<I>("\x4e"_su8, I{O::I32GeS});
  ExpectWrite<I>("\x4f"_su8, I{O::I32GeU});
  ExpectWrite<I>("\x50"_su8, I{O::I64Eqz});
  ExpectWrite<I>("\x51"_su8, I{O::I64Eq});
  ExpectWrite<I>("\x52"_su8, I{O::I64Ne});
  ExpectWrite<I>("\x53"_su8, I{O::I64LtS});
  ExpectWrite<I>("\x54"_su8, I{O::I64LtU});
  ExpectWrite<I>("\x55"_su8, I{O::I64GtS});
  ExpectWrite<I>("\x56"_su8, I{O::I64GtU});
  ExpectWrite<I>("\x57"_su8, I{O::I64LeS});
  ExpectWrite<I>("\x58"_su8, I{O::I64LeU});
  ExpectWrite<I>("\x59"_su8, I{O::I64GeS});
  ExpectWrite<I>("\x5a"_su8, I{O::I64GeU});
  ExpectWrite<I>("\x5b"_su8, I{O::F32Eq});
  ExpectWrite<I>("\x5c"_su8, I{O::F32Ne});
  ExpectWrite<I>("\x5d"_su8, I{O::F32Lt});
  ExpectWrite<I>("\x5e"_su8, I{O::F32Gt});
  ExpectWrite<I>("\x5f"_su8, I{O::F32Le});
  ExpectWrite<I>("\x60"_su8, I{O::F32Ge});
  ExpectWrite<I>("\x61"_su8, I{O::F64Eq});
  ExpectWrite<I>("\x62"_su8, I{O::F64Ne});
  ExpectWrite<I>("\x63"_su8, I{O::F64Lt});
  ExpectWrite<I>("\x64"_su8, I{O::F64Gt});
  ExpectWrite<I>("\x65"_su8, I{O::F64Le});
  ExpectWrite<I>("\x66"_su8, I{O::F64Ge});
  ExpectWrite<I>("\x67"_su8, I{O::I32Clz});
  ExpectWrite<I>("\x68"_su8, I{O::I32Ctz});
  ExpectWrite<I>("\x69"_su8, I{O::I32Popcnt});
  ExpectWrite<I>("\x6a"_su8, I{O::I32Add});
  ExpectWrite<I>("\x6b"_su8, I{O::I32Sub});
  ExpectWrite<I>("\x6c"_su8, I{O::I32Mul});
  ExpectWrite<I>("\x6d"_su8, I{O::I32DivS});
  ExpectWrite<I>("\x6e"_su8, I{O::I32DivU});
  ExpectWrite<I>("\x6f"_su8, I{O::I32RemS});
  ExpectWrite<I>("\x70"_su8, I{O::I32RemU});
  ExpectWrite<I>("\x71"_su8, I{O::I32And});
  ExpectWrite<I>("\x72"_su8, I{O::I32Or});
  ExpectWrite<I>("\x73"_su8, I{O::I32Xor});
  ExpectWrite<I>("\x74"_su8, I{O::I32Shl});
  ExpectWrite<I>("\x75"_su8, I{O::I32ShrS});
  ExpectWrite<I>("\x76"_su8, I{O::I32ShrU});
  ExpectWrite<I>("\x77"_su8, I{O::I32Rotl});
  ExpectWrite<I>("\x78"_su8, I{O::I32Rotr});
  ExpectWrite<I>("\x79"_su8, I{O::I64Clz});
  ExpectWrite<I>("\x7a"_su8, I{O::I64Ctz});
  ExpectWrite<I>("\x7b"_su8, I{O::I64Popcnt});
  ExpectWrite<I>("\x7c"_su8, I{O::I64Add});
  ExpectWrite<I>("\x7d"_su8, I{O::I64Sub});
  ExpectWrite<I>("\x7e"_su8, I{O::I64Mul});
  ExpectWrite<I>("\x7f"_su8, I{O::I64DivS});
  ExpectWrite<I>("\x80"_su8, I{O::I64DivU});
  ExpectWrite<I>("\x81"_su8, I{O::I64RemS});
  ExpectWrite<I>("\x82"_su8, I{O::I64RemU});
  ExpectWrite<I>("\x83"_su8, I{O::I64And});
  ExpectWrite<I>("\x84"_su8, I{O::I64Or});
  ExpectWrite<I>("\x85"_su8, I{O::I64Xor});
  ExpectWrite<I>("\x86"_su8, I{O::I64Shl});
  ExpectWrite<I>("\x87"_su8, I{O::I64ShrS});
  ExpectWrite<I>("\x88"_su8, I{O::I64ShrU});
  ExpectWrite<I>("\x89"_su8, I{O::I64Rotl});
  ExpectWrite<I>("\x8a"_su8, I{O::I64Rotr});
  ExpectWrite<I>("\x8b"_su8, I{O::F32Abs});
  ExpectWrite<I>("\x8c"_su8, I{O::F32Neg});
  ExpectWrite<I>("\x8d"_su8, I{O::F32Ceil});
  ExpectWrite<I>("\x8e"_su8, I{O::F32Floor});
  ExpectWrite<I>("\x8f"_su8, I{O::F32Trunc});
  ExpectWrite<I>("\x90"_su8, I{O::F32Nearest});
  ExpectWrite<I>("\x91"_su8, I{O::F32Sqrt});
  ExpectWrite<I>("\x92"_su8, I{O::F32Add});
  ExpectWrite<I>("\x93"_su8, I{O::F32Sub});
  ExpectWrite<I>("\x94"_su8, I{O::F32Mul});
  ExpectWrite<I>("\x95"_su8, I{O::F32Div});
  ExpectWrite<I>("\x96"_su8, I{O::F32Min});
  ExpectWrite<I>("\x97"_su8, I{O::F32Max});
  ExpectWrite<I>("\x98"_su8, I{O::F32Copysign});
  ExpectWrite<I>("\x99"_su8, I{O::F64Abs});
  ExpectWrite<I>("\x9a"_su8, I{O::F64Neg});
  ExpectWrite<I>("\x9b"_su8, I{O::F64Ceil});
  ExpectWrite<I>("\x9c"_su8, I{O::F64Floor});
  ExpectWrite<I>("\x9d"_su8, I{O::F64Trunc});
  ExpectWrite<I>("\x9e"_su8, I{O::F64Nearest});
  ExpectWrite<I>("\x9f"_su8, I{O::F64Sqrt});
  ExpectWrite<I>("\xa0"_su8, I{O::F64Add});
  ExpectWrite<I>("\xa1"_su8, I{O::F64Sub});
  ExpectWrite<I>("\xa2"_su8, I{O::F64Mul});
  ExpectWrite<I>("\xa3"_su8, I{O::F64Div});
  ExpectWrite<I>("\xa4"_su8, I{O::F64Min});
  ExpectWrite<I>("\xa5"_su8, I{O::F64Max});
  ExpectWrite<I>("\xa6"_su8, I{O::F64Copysign});
  ExpectWrite<I>("\xa7"_su8, I{O::I32WrapI64});
  ExpectWrite<I>("\xa8"_su8, I{O::I32TruncF32S});
  ExpectWrite<I>("\xa9"_su8, I{O::I32TruncF32U});
  ExpectWrite<I>("\xaa"_su8, I{O::I32TruncF64S});
  ExpectWrite<I>("\xab"_su8, I{O::I32TruncF64U});
  ExpectWrite<I>("\xac"_su8, I{O::I64ExtendI32S});
  ExpectWrite<I>("\xad"_su8, I{O::I64ExtendI32U});
  ExpectWrite<I>("\xae"_su8, I{O::I64TruncF32S});
  ExpectWrite<I>("\xaf"_su8, I{O::I64TruncF32U});
  ExpectWrite<I>("\xb0"_su8, I{O::I64TruncF64S});
  ExpectWrite<I>("\xb1"_su8, I{O::I64TruncF64U});
  ExpectWrite<I>("\xb2"_su8, I{O::F32ConvertI32S});
  ExpectWrite<I>("\xb3"_su8, I{O::F32ConvertI32U});
  ExpectWrite<I>("\xb4"_su8, I{O::F32ConvertI64S});
  ExpectWrite<I>("\xb5"_su8, I{O::F32ConvertI64U});
  ExpectWrite<I>("\xb6"_su8, I{O::F32DemoteF64});
  ExpectWrite<I>("\xb7"_su8, I{O::F64ConvertI32S});
  ExpectWrite<I>("\xb8"_su8, I{O::F64ConvertI32U});
  ExpectWrite<I>("\xb9"_su8, I{O::F64ConvertI64S});
  ExpectWrite<I>("\xba"_su8, I{O::F64ConvertI64U});
  ExpectWrite<I>("\xbb"_su8, I{O::F64PromoteF32});
  ExpectWrite<I>("\xbc"_su8, I{O::I32ReinterpretF32});
  ExpectWrite<I>("\xbd"_su8, I{O::I64ReinterpretF64});
  ExpectWrite<I>("\xbe"_su8, I{O::F32ReinterpretI32});
  ExpectWrite<I>("\xbf"_su8, I{O::F64ReinterpretI64});
}

TEST(WriteTest, Instruction_exceptions) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\x06\x40"_su8, I{O::Try, BlockType::Void});
  ExpectWrite<I>("\x07"_su8, I{O::Catch});
  ExpectWrite<I>("\x08\x00"_su8, I{O::Throw, Index{0}});
  ExpectWrite<I>("\x09"_su8, I{O::Rethrow});
  ExpectWrite<I>("\x0a\x01\x02"_su8, I{O::BrOnExn, BrOnExnImmediate{1, 2}});
}

TEST(WriteTest, Instruction_tail_call) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\x12\x00"_su8, I{O::ReturnCall, Index{0}});
  ExpectWrite<I>("\x13\x08\x00"_su8,
                 I{O::ReturnCallIndirect, CallIndirectImmediate{8, 0}});
}

TEST(WriteTest, Instruction_sign_extension) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\xc0"_su8, I{O::I32Extend8S});
  ExpectWrite<I>("\xc1"_su8, I{O::I32Extend16S});
  ExpectWrite<I>("\xc2"_su8, I{O::I64Extend8S});
  ExpectWrite<I>("\xc3"_su8, I{O::I64Extend16S});
  ExpectWrite<I>("\xc4"_su8, I{O::I64Extend32S});
}

TEST(WriteTest, Instruction_reference_types) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\x25\x00"_su8, I{O::TableGet, Index{0}});
  ExpectWrite<I>("\x26\x00"_su8, I{O::TableSet, Index{0}});
  ExpectWrite<I>("\xfc\x0f\x00"_su8, I{O::TableGrow, Index{0}});
  ExpectWrite<I>("\xfc\x10\x00"_su8, I{O::TableSize, Index{0}});
  ExpectWrite<I>("\xd0"_su8, I{O::RefNull});
  ExpectWrite<I>("\xd1"_su8, I{O::RefIsNull});
}

TEST(WriteTest, Instruction_function_references) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\xd2\x00"_su8, I{O::RefFunc, Index{0}});
}

TEST(WriteTest, Instruction_saturating_float_to_int) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\xfc\x00"_su8, I{O::I32TruncSatF32S});
  ExpectWrite<I>("\xfc\x01"_su8, I{O::I32TruncSatF32U});
  ExpectWrite<I>("\xfc\x02"_su8, I{O::I32TruncSatF64S});
  ExpectWrite<I>("\xfc\x03"_su8, I{O::I32TruncSatF64U});
  ExpectWrite<I>("\xfc\x04"_su8, I{O::I64TruncSatF32S});
  ExpectWrite<I>("\xfc\x05"_su8, I{O::I64TruncSatF32U});
  ExpectWrite<I>("\xfc\x06"_su8, I{O::I64TruncSatF64S});
  ExpectWrite<I>("\xfc\x07"_su8, I{O::I64TruncSatF64U});
}

TEST(WriteTest, Instruction_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\xfc\x08\x01\x00"_su8, I{O::MemoryInit, InitImmediate{1, 0}});
  ExpectWrite<I>("\xfc\x09\x02"_su8, I{O::DataDrop, Index{2}});
  ExpectWrite<I>("\xfc\x0a\x00\x00"_su8, I{O::MemoryCopy, CopyImmediate{0, 0}});
  ExpectWrite<I>("\xfc\x0b\x00"_su8, I{O::MemoryFill, u8{0}});
  ExpectWrite<I>("\xfc\x0c\x03\x00"_su8, I{O::TableInit, InitImmediate{3, 0}});
  ExpectWrite<I>("\xfc\x0d\x04"_su8, I{O::ElemDrop, Index{4}});
  ExpectWrite<I>("\xfc\x0e\x00\x00"_su8, I{O::TableCopy, CopyImmediate{0, 0}});
}

TEST(WriteTest, Instruction_simd) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>("\xfd\x00\x01\x02"_su8, I{O::V128Load, MemArgImmediate{1, 2}});
  ExpectWrite<I>("\xfd\x01\x03\x04"_su8,
                 I{O::V128Store, MemArgImmediate{3, 4}});
  ExpectWrite<I>(
      "\xfd\x02\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00"
      "\x00\x00\x00\x00\x00\x00"_su8,
      I{O::V128Const, v128{u64{5}, u64{6}}});
  ExpectWrite<I>(
      "\xfd\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00"_su8,
      I{O::V8X16Shuffle,
        ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}});
  ExpectWrite<I>("\xfd\x04"_su8, I{O::I8X16Splat});
  ExpectWrite<I>("\xfd\x05\x00"_su8, I{O::I8X16ExtractLaneS, u8{0}});
  ExpectWrite<I>("\xfd\x06\x00"_su8, I{O::I8X16ExtractLaneU, u8{0}});
  ExpectWrite<I>("\xfd\x07\x00"_su8, I{O::I8X16ReplaceLane, u8{0}});
  ExpectWrite<I>("\xfd\x08"_su8, I{O::I16X8Splat});
  ExpectWrite<I>("\xfd\x09\x00"_su8, I{O::I16X8ExtractLaneS, u8{0}});
  ExpectWrite<I>("\xfd\x0a\x00"_su8, I{O::I16X8ExtractLaneU, u8{0}});
  ExpectWrite<I>("\xfd\x0b\x00"_su8, I{O::I16X8ReplaceLane, u8{0}});
  ExpectWrite<I>("\xfd\x0c"_su8, I{O::I32X4Splat});
  ExpectWrite<I>("\xfd\x0d\x00"_su8, I{O::I32X4ExtractLane, u8{0}});
  ExpectWrite<I>("\xfd\x0e\x00"_su8, I{O::I32X4ReplaceLane, u8{0}});
  ExpectWrite<I>("\xfd\x0f"_su8, I{O::I64X2Splat});
  ExpectWrite<I>("\xfd\x10\x00"_su8, I{O::I64X2ExtractLane, u8{0}});
  ExpectWrite<I>("\xfd\x11\x00"_su8, I{O::I64X2ReplaceLane, u8{0}});
  ExpectWrite<I>("\xfd\x12"_su8, I{O::F32X4Splat});
  ExpectWrite<I>("\xfd\x13\x00"_su8, I{O::F32X4ExtractLane, u8{0}});
  ExpectWrite<I>("\xfd\x14\x00"_su8, I{O::F32X4ReplaceLane, u8{0}});
  ExpectWrite<I>("\xfd\x15"_su8, I{O::F64X2Splat});
  ExpectWrite<I>("\xfd\x16\x00"_su8, I{O::F64X2ExtractLane, u8{0}});
  ExpectWrite<I>("\xfd\x17\x00"_su8, I{O::F64X2ReplaceLane, u8{0}});
  ExpectWrite<I>("\xfd\x18"_su8, I{O::I8X16Eq});
  ExpectWrite<I>("\xfd\x19"_su8, I{O::I8X16Ne});
  ExpectWrite<I>("\xfd\x1a"_su8, I{O::I8X16LtS});
  ExpectWrite<I>("\xfd\x1b"_su8, I{O::I8X16LtU});
  ExpectWrite<I>("\xfd\x1c"_su8, I{O::I8X16GtS});
  ExpectWrite<I>("\xfd\x1d"_su8, I{O::I8X16GtU});
  ExpectWrite<I>("\xfd\x1e"_su8, I{O::I8X16LeS});
  ExpectWrite<I>("\xfd\x1f"_su8, I{O::I8X16LeU});
  ExpectWrite<I>("\xfd\x20"_su8, I{O::I8X16GeS});
  ExpectWrite<I>("\xfd\x21"_su8, I{O::I8X16GeU});
  ExpectWrite<I>("\xfd\x22"_su8, I{O::I16X8Eq});
  ExpectWrite<I>("\xfd\x23"_su8, I{O::I16X8Ne});
  ExpectWrite<I>("\xfd\x24"_su8, I{O::I16X8LtS});
  ExpectWrite<I>("\xfd\x25"_su8, I{O::I16X8LtU});
  ExpectWrite<I>("\xfd\x26"_su8, I{O::I16X8GtS});
  ExpectWrite<I>("\xfd\x27"_su8, I{O::I16X8GtU});
  ExpectWrite<I>("\xfd\x28"_su8, I{O::I16X8LeS});
  ExpectWrite<I>("\xfd\x29"_su8, I{O::I16X8LeU});
  ExpectWrite<I>("\xfd\x2a"_su8, I{O::I16X8GeS});
  ExpectWrite<I>("\xfd\x2b"_su8, I{O::I16X8GeU});
  ExpectWrite<I>("\xfd\x2c"_su8, I{O::I32X4Eq});
  ExpectWrite<I>("\xfd\x2d"_su8, I{O::I32X4Ne});
  ExpectWrite<I>("\xfd\x2e"_su8, I{O::I32X4LtS});
  ExpectWrite<I>("\xfd\x2f"_su8, I{O::I32X4LtU});
  ExpectWrite<I>("\xfd\x30"_su8, I{O::I32X4GtS});
  ExpectWrite<I>("\xfd\x31"_su8, I{O::I32X4GtU});
  ExpectWrite<I>("\xfd\x32"_su8, I{O::I32X4LeS});
  ExpectWrite<I>("\xfd\x33"_su8, I{O::I32X4LeU});
  ExpectWrite<I>("\xfd\x34"_su8, I{O::I32X4GeS});
  ExpectWrite<I>("\xfd\x35"_su8, I{O::I32X4GeU});
  ExpectWrite<I>("\xfd\x40"_su8, I{O::F32X4Eq});
  ExpectWrite<I>("\xfd\x41"_su8, I{O::F32X4Ne});
  ExpectWrite<I>("\xfd\x42"_su8, I{O::F32X4Lt});
  ExpectWrite<I>("\xfd\x43"_su8, I{O::F32X4Gt});
  ExpectWrite<I>("\xfd\x44"_su8, I{O::F32X4Le});
  ExpectWrite<I>("\xfd\x45"_su8, I{O::F32X4Ge});
  ExpectWrite<I>("\xfd\x46"_su8, I{O::F64X2Eq});
  ExpectWrite<I>("\xfd\x47"_su8, I{O::F64X2Ne});
  ExpectWrite<I>("\xfd\x48"_su8, I{O::F64X2Lt});
  ExpectWrite<I>("\xfd\x49"_su8, I{O::F64X2Gt});
  ExpectWrite<I>("\xfd\x4a"_su8, I{O::F64X2Le});
  ExpectWrite<I>("\xfd\x4b"_su8, I{O::F64X2Ge});
  ExpectWrite<I>("\xfd\x4c"_su8, I{O::V128Not});
  ExpectWrite<I>("\xfd\x4d"_su8, I{O::V128And});
  ExpectWrite<I>("\xfd\x4e"_su8, I{O::V128Or});
  ExpectWrite<I>("\xfd\x4f"_su8, I{O::V128Xor});
  ExpectWrite<I>("\xfd\x50"_su8, I{O::V128BitSelect});
  ExpectWrite<I>("\xfd\x51"_su8, I{O::I8X16Neg});
  ExpectWrite<I>("\xfd\x52"_su8, I{O::I8X16AnyTrue});
  ExpectWrite<I>("\xfd\x53"_su8, I{O::I8X16AllTrue});
  ExpectWrite<I>("\xfd\x54"_su8, I{O::I8X16Shl});
  ExpectWrite<I>("\xfd\x55"_su8, I{O::I8X16ShrS});
  ExpectWrite<I>("\xfd\x56"_su8, I{O::I8X16ShrU});
  ExpectWrite<I>("\xfd\x57"_su8, I{O::I8X16Add});
  ExpectWrite<I>("\xfd\x58"_su8, I{O::I8X16AddSaturateS});
  ExpectWrite<I>("\xfd\x59"_su8, I{O::I8X16AddSaturateU});
  ExpectWrite<I>("\xfd\x5a"_su8, I{O::I8X16Sub});
  ExpectWrite<I>("\xfd\x5b"_su8, I{O::I8X16SubSaturateS});
  ExpectWrite<I>("\xfd\x5c"_su8, I{O::I8X16SubSaturateU});
  ExpectWrite<I>("\xfd\x5d"_su8, I{O::I8X16Mul});
  ExpectWrite<I>("\xfd\x62"_su8, I{O::I16X8Neg});
  ExpectWrite<I>("\xfd\x63"_su8, I{O::I16X8AnyTrue});
  ExpectWrite<I>("\xfd\x64"_su8, I{O::I16X8AllTrue});
  ExpectWrite<I>("\xfd\x65"_su8, I{O::I16X8Shl});
  ExpectWrite<I>("\xfd\x66"_su8, I{O::I16X8ShrS});
  ExpectWrite<I>("\xfd\x67"_su8, I{O::I16X8ShrU});
  ExpectWrite<I>("\xfd\x68"_su8, I{O::I16X8Add});
  ExpectWrite<I>("\xfd\x69"_su8, I{O::I16X8AddSaturateS});
  ExpectWrite<I>("\xfd\x6a"_su8, I{O::I16X8AddSaturateU});
  ExpectWrite<I>("\xfd\x6b"_su8, I{O::I16X8Sub});
  ExpectWrite<I>("\xfd\x6c"_su8, I{O::I16X8SubSaturateS});
  ExpectWrite<I>("\xfd\x6d"_su8, I{O::I16X8SubSaturateU});
  ExpectWrite<I>("\xfd\x6e"_su8, I{O::I16X8Mul});
  ExpectWrite<I>("\xfd\x73"_su8, I{O::I32X4Neg});
  ExpectWrite<I>("\xfd\x74"_su8, I{O::I32X4AnyTrue});
  ExpectWrite<I>("\xfd\x75"_su8, I{O::I32X4AllTrue});
  ExpectWrite<I>("\xfd\x76"_su8, I{O::I32X4Shl});
  ExpectWrite<I>("\xfd\x77"_su8, I{O::I32X4ShrS});
  ExpectWrite<I>("\xfd\x78"_su8, I{O::I32X4ShrU});
  ExpectWrite<I>("\xfd\x79"_su8, I{O::I32X4Add});
  ExpectWrite<I>("\xfd\x7c"_su8, I{O::I32X4Sub});
  ExpectWrite<I>("\xfd\x7f"_su8, I{O::I32X4Mul});
  ExpectWrite<I>("\xfd\x84\x01"_su8, I{O::I64X2Neg});
  ExpectWrite<I>("\xfd\x85\x01"_su8, I{O::I64X2AnyTrue});
  ExpectWrite<I>("\xfd\x86\x01"_su8, I{O::I64X2AllTrue});
  ExpectWrite<I>("\xfd\x87\x01"_su8, I{O::I64X2Shl});
  ExpectWrite<I>("\xfd\x88\x01"_su8, I{O::I64X2ShrS});
  ExpectWrite<I>("\xfd\x89\x01"_su8, I{O::I64X2ShrU});
  ExpectWrite<I>("\xfd\x8a\x01"_su8, I{O::I64X2Add});
  ExpectWrite<I>("\xfd\x8d\x01"_su8, I{O::I64X2Sub});
  ExpectWrite<I>("\xfd\x95\x01"_su8, I{O::F32X4Abs});
  ExpectWrite<I>("\xfd\x96\x01"_su8, I{O::F32X4Neg});
  ExpectWrite<I>("\xfd\x97\x01"_su8, I{O::F32X4Sqrt});
  ExpectWrite<I>("\xfd\x9a\x01"_su8, I{O::F32X4Add});
  ExpectWrite<I>("\xfd\x9b\x01"_su8, I{O::F32X4Sub});
  ExpectWrite<I>("\xfd\x9c\x01"_su8, I{O::F32X4Mul});
  ExpectWrite<I>("\xfd\x9d\x01"_su8, I{O::F32X4Div});
  ExpectWrite<I>("\xfd\x9e\x01"_su8, I{O::F32X4Min});
  ExpectWrite<I>("\xfd\x9f\x01"_su8, I{O::F32X4Max});
  ExpectWrite<I>("\xfd\xa0\x01"_su8, I{O::F64X2Abs});
  ExpectWrite<I>("\xfd\xa1\x01"_su8, I{O::F64X2Neg});
  ExpectWrite<I>("\xfd\xa2\x01"_su8, I{O::F64X2Sqrt});
  ExpectWrite<I>("\xfd\xa5\x01"_su8, I{O::F64X2Add});
  ExpectWrite<I>("\xfd\xa6\x01"_su8, I{O::F64X2Sub});
  ExpectWrite<I>("\xfd\xa7\x01"_su8, I{O::F64X2Mul});
  ExpectWrite<I>("\xfd\xa8\x01"_su8, I{O::F64X2Div});
  ExpectWrite<I>("\xfd\xa9\x01"_su8, I{O::F64X2Min});
  ExpectWrite<I>("\xfd\xaa\x01"_su8, I{O::F64X2Max});
  ExpectWrite<I>("\xfd\xab\x01"_su8, I{O::I32X4TruncSatF32X4S});
  ExpectWrite<I>("\xfd\xac\x01"_su8, I{O::I32X4TruncSatF32X4U});
  ExpectWrite<I>("\xfd\xad\x01"_su8, I{O::I64X2TruncSatF64X2S});
  ExpectWrite<I>("\xfd\xae\x01"_su8, I{O::I64X2TruncSatF64X2U});
  ExpectWrite<I>("\xfd\xaf\x01"_su8, I{O::F32X4ConvertI32X4S});
  ExpectWrite<I>("\xfd\xb0\x01"_su8, I{O::F32X4ConvertI32X4U});
  ExpectWrite<I>("\xfd\xb1\x01"_su8, I{O::F64X2ConvertI64X2S});
  ExpectWrite<I>("\xfd\xb2\x01"_su8, I{O::F64X2ConvertI64X2U});
}

TEST(WriteTest, Instruction_threads) {
  using I = Instruction;
  using O = Opcode;

  const MemArgImmediate m{0, 0};

  ExpectWrite<I>("\xfe\x00\x00\x00"_su8, I{O::AtomicNotify, m});
  ExpectWrite<I>("\xfe\x01\x00\x00"_su8, I{O::I32AtomicWait, m});
  ExpectWrite<I>("\xfe\x02\x00\x00"_su8, I{O::I64AtomicWait, m});
  ExpectWrite<I>("\xfe\x10\x00\x00"_su8, I{O::I32AtomicLoad, m});
  ExpectWrite<I>("\xfe\x11\x00\x00"_su8, I{O::I64AtomicLoad, m});
  ExpectWrite<I>("\xfe\x12\x00\x00"_su8, I{O::I32AtomicLoad8U, m});
  ExpectWrite<I>("\xfe\x13\x00\x00"_su8, I{O::I32AtomicLoad16U, m});
  ExpectWrite<I>("\xfe\x14\x00\x00"_su8, I{O::I64AtomicLoad8U, m});
  ExpectWrite<I>("\xfe\x15\x00\x00"_su8, I{O::I64AtomicLoad16U, m});
  ExpectWrite<I>("\xfe\x16\x00\x00"_su8, I{O::I64AtomicLoad32U, m});
  ExpectWrite<I>("\xfe\x17\x00\x00"_su8, I{O::I32AtomicStore, m});
  ExpectWrite<I>("\xfe\x18\x00\x00"_su8, I{O::I64AtomicStore, m});
  ExpectWrite<I>("\xfe\x19\x00\x00"_su8, I{O::I32AtomicStore8, m});
  ExpectWrite<I>("\xfe\x1a\x00\x00"_su8, I{O::I32AtomicStore16, m});
  ExpectWrite<I>("\xfe\x1b\x00\x00"_su8, I{O::I64AtomicStore8, m});
  ExpectWrite<I>("\xfe\x1c\x00\x00"_su8, I{O::I64AtomicStore16, m});
  ExpectWrite<I>("\xfe\x1d\x00\x00"_su8, I{O::I64AtomicStore32, m});
  ExpectWrite<I>("\xfe\x1e\x00\x00"_su8, I{O::I32AtomicRmwAdd, m});
  ExpectWrite<I>("\xfe\x1f\x00\x00"_su8, I{O::I64AtomicRmwAdd, m});
  ExpectWrite<I>("\xfe\x20\x00\x00"_su8, I{O::I32AtomicRmw8AddU, m});
  ExpectWrite<I>("\xfe\x21\x00\x00"_su8, I{O::I32AtomicRmw16AddU, m});
  ExpectWrite<I>("\xfe\x22\x00\x00"_su8, I{O::I64AtomicRmw8AddU, m});
  ExpectWrite<I>("\xfe\x23\x00\x00"_su8, I{O::I64AtomicRmw16AddU, m});
  ExpectWrite<I>("\xfe\x24\x00\x00"_su8, I{O::I64AtomicRmw32AddU, m});
  ExpectWrite<I>("\xfe\x25\x00\x00"_su8, I{O::I32AtomicRmwSub, m});
  ExpectWrite<I>("\xfe\x26\x00\x00"_su8, I{O::I64AtomicRmwSub, m});
  ExpectWrite<I>("\xfe\x27\x00\x00"_su8, I{O::I32AtomicRmw8SubU, m});
  ExpectWrite<I>("\xfe\x28\x00\x00"_su8, I{O::I32AtomicRmw16SubU, m});
  ExpectWrite<I>("\xfe\x29\x00\x00"_su8, I{O::I64AtomicRmw8SubU, m});
  ExpectWrite<I>("\xfe\x2a\x00\x00"_su8, I{O::I64AtomicRmw16SubU, m});
  ExpectWrite<I>("\xfe\x2b\x00\x00"_su8, I{O::I64AtomicRmw32SubU, m});
  ExpectWrite<I>("\xfe\x2c\x00\x00"_su8, I{O::I32AtomicRmwAnd, m});
  ExpectWrite<I>("\xfe\x2d\x00\x00"_su8, I{O::I64AtomicRmwAnd, m});
  ExpectWrite<I>("\xfe\x2e\x00\x00"_su8, I{O::I32AtomicRmw8AndU, m});
  ExpectWrite<I>("\xfe\x2f\x00\x00"_su8, I{O::I32AtomicRmw16AndU, m});
  ExpectWrite<I>("\xfe\x30\x00\x00"_su8, I{O::I64AtomicRmw8AndU, m});
  ExpectWrite<I>("\xfe\x31\x00\x00"_su8, I{O::I64AtomicRmw16AndU, m});
  ExpectWrite<I>("\xfe\x32\x00\x00"_su8, I{O::I64AtomicRmw32AndU, m});
  ExpectWrite<I>("\xfe\x33\x00\x00"_su8, I{O::I32AtomicRmwOr, m});
  ExpectWrite<I>("\xfe\x34\x00\x00"_su8, I{O::I64AtomicRmwOr, m});
  ExpectWrite<I>("\xfe\x35\x00\x00"_su8, I{O::I32AtomicRmw8OrU, m});
  ExpectWrite<I>("\xfe\x36\x00\x00"_su8, I{O::I32AtomicRmw16OrU, m});
  ExpectWrite<I>("\xfe\x37\x00\x00"_su8, I{O::I64AtomicRmw8OrU, m});
  ExpectWrite<I>("\xfe\x38\x00\x00"_su8, I{O::I64AtomicRmw16OrU, m});
  ExpectWrite<I>("\xfe\x39\x00\x00"_su8, I{O::I64AtomicRmw32OrU, m});
  ExpectWrite<I>("\xfe\x3a\x00\x00"_su8, I{O::I32AtomicRmwXor, m});
  ExpectWrite<I>("\xfe\x3b\x00\x00"_su8, I{O::I64AtomicRmwXor, m});
  ExpectWrite<I>("\xfe\x3c\x00\x00"_su8, I{O::I32AtomicRmw8XorU, m});
  ExpectWrite<I>("\xfe\x3d\x00\x00"_su8, I{O::I32AtomicRmw16XorU, m});
  ExpectWrite<I>("\xfe\x3e\x00\x00"_su8, I{O::I64AtomicRmw8XorU, m});
  ExpectWrite<I>("\xfe\x3f\x00\x00"_su8, I{O::I64AtomicRmw16XorU, m});
  ExpectWrite<I>("\xfe\x40\x00\x00"_su8, I{O::I64AtomicRmw32XorU, m});
  ExpectWrite<I>("\xfe\x41\x00\x00"_su8, I{O::I32AtomicRmwXchg, m});
  ExpectWrite<I>("\xfe\x42\x00\x00"_su8, I{O::I64AtomicRmwXchg, m});
  ExpectWrite<I>("\xfe\x43\x00\x00"_su8, I{O::I32AtomicRmw8XchgU, m});
  ExpectWrite<I>("\xfe\x44\x00\x00"_su8, I{O::I32AtomicRmw16XchgU, m});
  ExpectWrite<I>("\xfe\x45\x00\x00"_su8, I{O::I64AtomicRmw8XchgU, m});
  ExpectWrite<I>("\xfe\x46\x00\x00"_su8, I{O::I64AtomicRmw16XchgU, m});
  ExpectWrite<I>("\xfe\x47\x00\x00"_su8, I{O::I64AtomicRmw32XchgU, m});
  ExpectWrite<I>("\xfe\x48\x00\x00"_su8, I{O::I32AtomicRmwCmpxchg, m});
  ExpectWrite<I>("\xfe\x49\x00\x00"_su8, I{O::I64AtomicRmwCmpxchg, m});
  ExpectWrite<I>("\xfe\x4a\x00\x00"_su8, I{O::I32AtomicRmw8CmpxchgU, m});
  ExpectWrite<I>("\xfe\x4b\x00\x00"_su8, I{O::I32AtomicRmw16CmpxchgU, m});
  ExpectWrite<I>("\xfe\x4c\x00\x00"_su8, I{O::I64AtomicRmw8CmpxchgU, m});
  ExpectWrite<I>("\xfe\x4d\x00\x00"_su8, I{O::I64AtomicRmw16CmpxchgU, m});
  ExpectWrite<I>("\xfe\x4e\x00\x00"_su8, I{O::I64AtomicRmw32CmpxchgU, m});
}

TEST(WriteTest, Limits) {
  ExpectWrite<Limits>("\x00\x81\x01"_su8, Limits{129});
  ExpectWrite<Limits>("\x01\x02\xe8\x07"_su8, Limits{2, 1000});
}

TEST(WriteTest, Locals) {
  ExpectWrite<Locals>("\x02\x7f"_su8, Locals{2, ValueType::I32});
  ExpectWrite<Locals>("\xc0\x02\x7c"_su8, Locals{320, ValueType::F64});
}

TEST(WriteTest, MemArgImmediate) {
  ExpectWrite<MemArgImmediate>("\x00\x00"_su8, MemArgImmediate{0, 0});
  ExpectWrite<MemArgImmediate>("\x01\x80\x02"_su8, MemArgImmediate{1, 256});
}

TEST(WriteTest, Memory) {
  ExpectWrite<Memory>("\x01\x01\x02"_su8, Memory{MemoryType{Limits{1, 2}}});
}

TEST(WriteTest, MemoryType) {
  ExpectWrite<MemoryType>("\x00\x01"_su8, MemoryType{Limits{1}});
  ExpectWrite<MemoryType>("\x01\x00\x80\x01"_su8, MemoryType{Limits{0, 128}});
}

TEST(WriteTest, Mutability) {
  ExpectWrite<Mutability>("\x00"_su8, Mutability::Const);
  ExpectWrite<Mutability>("\x01"_su8, Mutability::Var);
}

TEST(WriteTest, NameSubsectionId) {
  ExpectWrite<NameSubsectionId>("\x00"_su8, NameSubsectionId::ModuleName);
  ExpectWrite<NameSubsectionId>("\x01"_su8, NameSubsectionId::FunctionNames);
  ExpectWrite<NameSubsectionId>("\x02"_su8, NameSubsectionId::LocalNames);
}

TEST(WriteTest, Opcode) {
  ExpectWrite<Opcode>("\x00"_su8, Opcode::Unreachable);
  ExpectWrite<Opcode>("\x01"_su8, Opcode::Nop);
  ExpectWrite<Opcode>("\x02"_su8, Opcode::Block);
  ExpectWrite<Opcode>("\x03"_su8, Opcode::Loop);
  ExpectWrite<Opcode>("\x04"_su8, Opcode::If);
  ExpectWrite<Opcode>("\x05"_su8, Opcode::Else);
  ExpectWrite<Opcode>("\x0b"_su8, Opcode::End);
  ExpectWrite<Opcode>("\x0c"_su8, Opcode::Br);
  ExpectWrite<Opcode>("\x0d"_su8, Opcode::BrIf);
  ExpectWrite<Opcode>("\x0e"_su8, Opcode::BrTable);
  ExpectWrite<Opcode>("\x0f"_su8, Opcode::Return);
  ExpectWrite<Opcode>("\x10"_su8, Opcode::Call);
  ExpectWrite<Opcode>("\x11"_su8, Opcode::CallIndirect);
  ExpectWrite<Opcode>("\x1a"_su8, Opcode::Drop);
  ExpectWrite<Opcode>("\x1b"_su8, Opcode::Select);
  ExpectWrite<Opcode>("\x20"_su8, Opcode::LocalGet);
  ExpectWrite<Opcode>("\x21"_su8, Opcode::LocalSet);
  ExpectWrite<Opcode>("\x22"_su8, Opcode::LocalTee);
  ExpectWrite<Opcode>("\x23"_su8, Opcode::GlobalGet);
  ExpectWrite<Opcode>("\x24"_su8, Opcode::GlobalSet);
  ExpectWrite<Opcode>("\x28"_su8, Opcode::I32Load);
  ExpectWrite<Opcode>("\x29"_su8, Opcode::I64Load);
  ExpectWrite<Opcode>("\x2a"_su8, Opcode::F32Load);
  ExpectWrite<Opcode>("\x2b"_su8, Opcode::F64Load);
  ExpectWrite<Opcode>("\x2c"_su8, Opcode::I32Load8S);
  ExpectWrite<Opcode>("\x2d"_su8, Opcode::I32Load8U);
  ExpectWrite<Opcode>("\x2e"_su8, Opcode::I32Load16S);
  ExpectWrite<Opcode>("\x2f"_su8, Opcode::I32Load16U);
  ExpectWrite<Opcode>("\x30"_su8, Opcode::I64Load8S);
  ExpectWrite<Opcode>("\x31"_su8, Opcode::I64Load8U);
  ExpectWrite<Opcode>("\x32"_su8, Opcode::I64Load16S);
  ExpectWrite<Opcode>("\x33"_su8, Opcode::I64Load16U);
  ExpectWrite<Opcode>("\x34"_su8, Opcode::I64Load32S);
  ExpectWrite<Opcode>("\x35"_su8, Opcode::I64Load32U);
  ExpectWrite<Opcode>("\x36"_su8, Opcode::I32Store);
  ExpectWrite<Opcode>("\x37"_su8, Opcode::I64Store);
  ExpectWrite<Opcode>("\x38"_su8, Opcode::F32Store);
  ExpectWrite<Opcode>("\x39"_su8, Opcode::F64Store);
  ExpectWrite<Opcode>("\x3a"_su8, Opcode::I32Store8);
  ExpectWrite<Opcode>("\x3b"_su8, Opcode::I32Store16);
  ExpectWrite<Opcode>("\x3c"_su8, Opcode::I64Store8);
  ExpectWrite<Opcode>("\x3d"_su8, Opcode::I64Store16);
  ExpectWrite<Opcode>("\x3e"_su8, Opcode::I64Store32);
  ExpectWrite<Opcode>("\x3f"_su8, Opcode::MemorySize);
  ExpectWrite<Opcode>("\x40"_su8, Opcode::MemoryGrow);
  ExpectWrite<Opcode>("\x41"_su8, Opcode::I32Const);
  ExpectWrite<Opcode>("\x42"_su8, Opcode::I64Const);
  ExpectWrite<Opcode>("\x43"_su8, Opcode::F32Const);
  ExpectWrite<Opcode>("\x44"_su8, Opcode::F64Const);
  ExpectWrite<Opcode>("\x45"_su8, Opcode::I32Eqz);
  ExpectWrite<Opcode>("\x46"_su8, Opcode::I32Eq);
  ExpectWrite<Opcode>("\x47"_su8, Opcode::I32Ne);
  ExpectWrite<Opcode>("\x48"_su8, Opcode::I32LtS);
  ExpectWrite<Opcode>("\x49"_su8, Opcode::I32LtU);
  ExpectWrite<Opcode>("\x4a"_su8, Opcode::I32GtS);
  ExpectWrite<Opcode>("\x4b"_su8, Opcode::I32GtU);
  ExpectWrite<Opcode>("\x4c"_su8, Opcode::I32LeS);
  ExpectWrite<Opcode>("\x4d"_su8, Opcode::I32LeU);
  ExpectWrite<Opcode>("\x4e"_su8, Opcode::I32GeS);
  ExpectWrite<Opcode>("\x4f"_su8, Opcode::I32GeU);
  ExpectWrite<Opcode>("\x50"_su8, Opcode::I64Eqz);
  ExpectWrite<Opcode>("\x51"_su8, Opcode::I64Eq);
  ExpectWrite<Opcode>("\x52"_su8, Opcode::I64Ne);
  ExpectWrite<Opcode>("\x53"_su8, Opcode::I64LtS);
  ExpectWrite<Opcode>("\x54"_su8, Opcode::I64LtU);
  ExpectWrite<Opcode>("\x55"_su8, Opcode::I64GtS);
  ExpectWrite<Opcode>("\x56"_su8, Opcode::I64GtU);
  ExpectWrite<Opcode>("\x57"_su8, Opcode::I64LeS);
  ExpectWrite<Opcode>("\x58"_su8, Opcode::I64LeU);
  ExpectWrite<Opcode>("\x59"_su8, Opcode::I64GeS);
  ExpectWrite<Opcode>("\x5a"_su8, Opcode::I64GeU);
  ExpectWrite<Opcode>("\x5b"_su8, Opcode::F32Eq);
  ExpectWrite<Opcode>("\x5c"_su8, Opcode::F32Ne);
  ExpectWrite<Opcode>("\x5d"_su8, Opcode::F32Lt);
  ExpectWrite<Opcode>("\x5e"_su8, Opcode::F32Gt);
  ExpectWrite<Opcode>("\x5f"_su8, Opcode::F32Le);
  ExpectWrite<Opcode>("\x60"_su8, Opcode::F32Ge);
  ExpectWrite<Opcode>("\x61"_su8, Opcode::F64Eq);
  ExpectWrite<Opcode>("\x62"_su8, Opcode::F64Ne);
  ExpectWrite<Opcode>("\x63"_su8, Opcode::F64Lt);
  ExpectWrite<Opcode>("\x64"_su8, Opcode::F64Gt);
  ExpectWrite<Opcode>("\x65"_su8, Opcode::F64Le);
  ExpectWrite<Opcode>("\x66"_su8, Opcode::F64Ge);
  ExpectWrite<Opcode>("\x67"_su8, Opcode::I32Clz);
  ExpectWrite<Opcode>("\x68"_su8, Opcode::I32Ctz);
  ExpectWrite<Opcode>("\x69"_su8, Opcode::I32Popcnt);
  ExpectWrite<Opcode>("\x6a"_su8, Opcode::I32Add);
  ExpectWrite<Opcode>("\x6b"_su8, Opcode::I32Sub);
  ExpectWrite<Opcode>("\x6c"_su8, Opcode::I32Mul);
  ExpectWrite<Opcode>("\x6d"_su8, Opcode::I32DivS);
  ExpectWrite<Opcode>("\x6e"_su8, Opcode::I32DivU);
  ExpectWrite<Opcode>("\x6f"_su8, Opcode::I32RemS);
  ExpectWrite<Opcode>("\x70"_su8, Opcode::I32RemU);
  ExpectWrite<Opcode>("\x71"_su8, Opcode::I32And);
  ExpectWrite<Opcode>("\x72"_su8, Opcode::I32Or);
  ExpectWrite<Opcode>("\x73"_su8, Opcode::I32Xor);
  ExpectWrite<Opcode>("\x74"_su8, Opcode::I32Shl);
  ExpectWrite<Opcode>("\x75"_su8, Opcode::I32ShrS);
  ExpectWrite<Opcode>("\x76"_su8, Opcode::I32ShrU);
  ExpectWrite<Opcode>("\x77"_su8, Opcode::I32Rotl);
  ExpectWrite<Opcode>("\x78"_su8, Opcode::I32Rotr);
  ExpectWrite<Opcode>("\x79"_su8, Opcode::I64Clz);
  ExpectWrite<Opcode>("\x7a"_su8, Opcode::I64Ctz);
  ExpectWrite<Opcode>("\x7b"_su8, Opcode::I64Popcnt);
  ExpectWrite<Opcode>("\x7c"_su8, Opcode::I64Add);
  ExpectWrite<Opcode>("\x7d"_su8, Opcode::I64Sub);
  ExpectWrite<Opcode>("\x7e"_su8, Opcode::I64Mul);
  ExpectWrite<Opcode>("\x7f"_su8, Opcode::I64DivS);
  ExpectWrite<Opcode>("\x80"_su8, Opcode::I64DivU);
  ExpectWrite<Opcode>("\x81"_su8, Opcode::I64RemS);
  ExpectWrite<Opcode>("\x82"_su8, Opcode::I64RemU);
  ExpectWrite<Opcode>("\x83"_su8, Opcode::I64And);
  ExpectWrite<Opcode>("\x84"_su8, Opcode::I64Or);
  ExpectWrite<Opcode>("\x85"_su8, Opcode::I64Xor);
  ExpectWrite<Opcode>("\x86"_su8, Opcode::I64Shl);
  ExpectWrite<Opcode>("\x87"_su8, Opcode::I64ShrS);
  ExpectWrite<Opcode>("\x88"_su8, Opcode::I64ShrU);
  ExpectWrite<Opcode>("\x89"_su8, Opcode::I64Rotl);
  ExpectWrite<Opcode>("\x8a"_su8, Opcode::I64Rotr);
  ExpectWrite<Opcode>("\x8b"_su8, Opcode::F32Abs);
  ExpectWrite<Opcode>("\x8c"_su8, Opcode::F32Neg);
  ExpectWrite<Opcode>("\x8d"_su8, Opcode::F32Ceil);
  ExpectWrite<Opcode>("\x8e"_su8, Opcode::F32Floor);
  ExpectWrite<Opcode>("\x8f"_su8, Opcode::F32Trunc);
  ExpectWrite<Opcode>("\x90"_su8, Opcode::F32Nearest);
  ExpectWrite<Opcode>("\x91"_su8, Opcode::F32Sqrt);
  ExpectWrite<Opcode>("\x92"_su8, Opcode::F32Add);
  ExpectWrite<Opcode>("\x93"_su8, Opcode::F32Sub);
  ExpectWrite<Opcode>("\x94"_su8, Opcode::F32Mul);
  ExpectWrite<Opcode>("\x95"_su8, Opcode::F32Div);
  ExpectWrite<Opcode>("\x96"_su8, Opcode::F32Min);
  ExpectWrite<Opcode>("\x97"_su8, Opcode::F32Max);
  ExpectWrite<Opcode>("\x98"_su8, Opcode::F32Copysign);
  ExpectWrite<Opcode>("\x99"_su8, Opcode::F64Abs);
  ExpectWrite<Opcode>("\x9a"_su8, Opcode::F64Neg);
  ExpectWrite<Opcode>("\x9b"_su8, Opcode::F64Ceil);
  ExpectWrite<Opcode>("\x9c"_su8, Opcode::F64Floor);
  ExpectWrite<Opcode>("\x9d"_su8, Opcode::F64Trunc);
  ExpectWrite<Opcode>("\x9e"_su8, Opcode::F64Nearest);
  ExpectWrite<Opcode>("\x9f"_su8, Opcode::F64Sqrt);
  ExpectWrite<Opcode>("\xa0"_su8, Opcode::F64Add);
  ExpectWrite<Opcode>("\xa1"_su8, Opcode::F64Sub);
  ExpectWrite<Opcode>("\xa2"_su8, Opcode::F64Mul);
  ExpectWrite<Opcode>("\xa3"_su8, Opcode::F64Div);
  ExpectWrite<Opcode>("\xa4"_su8, Opcode::F64Min);
  ExpectWrite<Opcode>("\xa5"_su8, Opcode::F64Max);
  ExpectWrite<Opcode>("\xa6"_su8, Opcode::F64Copysign);
  ExpectWrite<Opcode>("\xa7"_su8, Opcode::I32WrapI64);
  ExpectWrite<Opcode>("\xa8"_su8, Opcode::I32TruncF32S);
  ExpectWrite<Opcode>("\xa9"_su8, Opcode::I32TruncF32U);
  ExpectWrite<Opcode>("\xaa"_su8, Opcode::I32TruncF64S);
  ExpectWrite<Opcode>("\xab"_su8, Opcode::I32TruncF64U);
  ExpectWrite<Opcode>("\xac"_su8, Opcode::I64ExtendI32S);
  ExpectWrite<Opcode>("\xad"_su8, Opcode::I64ExtendI32U);
  ExpectWrite<Opcode>("\xae"_su8, Opcode::I64TruncF32S);
  ExpectWrite<Opcode>("\xaf"_su8, Opcode::I64TruncF32U);
  ExpectWrite<Opcode>("\xb0"_su8, Opcode::I64TruncF64S);
  ExpectWrite<Opcode>("\xb1"_su8, Opcode::I64TruncF64U);
  ExpectWrite<Opcode>("\xb2"_su8, Opcode::F32ConvertI32S);
  ExpectWrite<Opcode>("\xb3"_su8, Opcode::F32ConvertI32U);
  ExpectWrite<Opcode>("\xb4"_su8, Opcode::F32ConvertI64S);
  ExpectWrite<Opcode>("\xb5"_su8, Opcode::F32ConvertI64U);
  ExpectWrite<Opcode>("\xb6"_su8, Opcode::F32DemoteF64);
  ExpectWrite<Opcode>("\xb7"_su8, Opcode::F64ConvertI32S);
  ExpectWrite<Opcode>("\xb8"_su8, Opcode::F64ConvertI32U);
  ExpectWrite<Opcode>("\xb9"_su8, Opcode::F64ConvertI64S);
  ExpectWrite<Opcode>("\xba"_su8, Opcode::F64ConvertI64U);
  ExpectWrite<Opcode>("\xbb"_su8, Opcode::F64PromoteF32);
  ExpectWrite<Opcode>("\xbc"_su8, Opcode::I32ReinterpretF32);
  ExpectWrite<Opcode>("\xbd"_su8, Opcode::I64ReinterpretF64);
  ExpectWrite<Opcode>("\xbe"_su8, Opcode::F32ReinterpretI32);
  ExpectWrite<Opcode>("\xbf"_su8, Opcode::F64ReinterpretI64);
}

TEST(WriteTest, Opcode_exceptions) {
  ExpectWrite<Opcode>("\x06"_su8, Opcode::Try);
  ExpectWrite<Opcode>("\x07"_su8, Opcode::Catch);
  ExpectWrite<Opcode>("\x08"_su8, Opcode::Throw);
  ExpectWrite<Opcode>("\x09"_su8, Opcode::Rethrow);
  ExpectWrite<Opcode>("\x0a"_su8, Opcode::BrOnExn);
}

TEST(WriteTest, Opcode_tail_call) {
  ExpectWrite<Opcode>("\x12"_su8, Opcode::ReturnCall);
  ExpectWrite<Opcode>("\x13"_su8, Opcode::ReturnCallIndirect);
}

TEST(WriteTest, Opcode_sign_extension) {
  ExpectWrite<Opcode>("\xc0"_su8, Opcode::I32Extend8S);
  ExpectWrite<Opcode>("\xc1"_su8, Opcode::I32Extend16S);
  ExpectWrite<Opcode>("\xc2"_su8, Opcode::I64Extend8S);
  ExpectWrite<Opcode>("\xc3"_su8, Opcode::I64Extend16S);
  ExpectWrite<Opcode>("\xc4"_su8, Opcode::I64Extend32S);
}

TEST(WriteTest, Opcode_reference_types) {
  ExpectWrite<Opcode>("\x25"_su8, Opcode::TableGet);
  ExpectWrite<Opcode>("\x26"_su8, Opcode::TableSet);
  ExpectWrite<Opcode>("\xfc\x0f"_su8, Opcode::TableGrow);
  ExpectWrite<Opcode>("\xfc\x10"_su8, Opcode::TableSize);
  ExpectWrite<Opcode>("\xd0"_su8, Opcode::RefNull);
  ExpectWrite<Opcode>("\xd1"_su8, Opcode::RefIsNull);
}

TEST(WriteTest, Opcode_function_references) {
  ExpectWrite<Opcode>("\xd2"_su8, Opcode::RefFunc);
}

TEST(WriteTest, Opcode_saturating_float_to_int) {
  ExpectWrite<Opcode>("\xfc\x00"_su8, Opcode::I32TruncSatF32S);
  ExpectWrite<Opcode>("\xfc\x01"_su8, Opcode::I32TruncSatF32U);
  ExpectWrite<Opcode>("\xfc\x02"_su8, Opcode::I32TruncSatF64S);
  ExpectWrite<Opcode>("\xfc\x03"_su8, Opcode::I32TruncSatF64U);
  ExpectWrite<Opcode>("\xfc\x04"_su8, Opcode::I64TruncSatF32S);
  ExpectWrite<Opcode>("\xfc\x05"_su8, Opcode::I64TruncSatF32U);
  ExpectWrite<Opcode>("\xfc\x06"_su8, Opcode::I64TruncSatF64S);
  ExpectWrite<Opcode>("\xfc\x07"_su8, Opcode::I64TruncSatF64U);
}

TEST(WriteTest, Opcode_bulk_memory) {
  ExpectWrite<Opcode>("\xfc\x08"_su8, Opcode::MemoryInit);
  ExpectWrite<Opcode>("\xfc\x09"_su8, Opcode::DataDrop);
  ExpectWrite<Opcode>("\xfc\x0a"_su8, Opcode::MemoryCopy);
  ExpectWrite<Opcode>("\xfc\x0b"_su8, Opcode::MemoryFill);
  ExpectWrite<Opcode>("\xfc\x0c"_su8, Opcode::TableInit);
  ExpectWrite<Opcode>("\xfc\x0d"_su8, Opcode::ElemDrop);
  ExpectWrite<Opcode>("\xfc\x0e"_su8, Opcode::TableCopy);
}

TEST(WriteTest, Opcode_simd) {
  using O = Opcode;

  ExpectWrite<O>("\xfd\x00"_su8, O::V128Load);
  ExpectWrite<O>("\xfd\x01"_su8, O::V128Store);
  ExpectWrite<O>("\xfd\x02"_su8, O::V128Const);
  ExpectWrite<O>("\xfd\x03"_su8, O::V8X16Shuffle);
  ExpectWrite<O>("\xfd\x04"_su8, O::I8X16Splat);
  ExpectWrite<O>("\xfd\x05"_su8, O::I8X16ExtractLaneS);
  ExpectWrite<O>("\xfd\x06"_su8, O::I8X16ExtractLaneU);
  ExpectWrite<O>("\xfd\x07"_su8, O::I8X16ReplaceLane);
  ExpectWrite<O>("\xfd\x08"_su8, O::I16X8Splat);
  ExpectWrite<O>("\xfd\x09"_su8, O::I16X8ExtractLaneS);
  ExpectWrite<O>("\xfd\x0a"_su8, O::I16X8ExtractLaneU);
  ExpectWrite<O>("\xfd\x0b"_su8, O::I16X8ReplaceLane);
  ExpectWrite<O>("\xfd\x0c"_su8, O::I32X4Splat);
  ExpectWrite<O>("\xfd\x0d"_su8, O::I32X4ExtractLane);
  ExpectWrite<O>("\xfd\x0e"_su8, O::I32X4ReplaceLane);
  ExpectWrite<O>("\xfd\x0f"_su8, O::I64X2Splat);
  ExpectWrite<O>("\xfd\x10"_su8, O::I64X2ExtractLane);
  ExpectWrite<O>("\xfd\x11"_su8, O::I64X2ReplaceLane);
  ExpectWrite<O>("\xfd\x12"_su8, O::F32X4Splat);
  ExpectWrite<O>("\xfd\x13"_su8, O::F32X4ExtractLane);
  ExpectWrite<O>("\xfd\x14"_su8, O::F32X4ReplaceLane);
  ExpectWrite<O>("\xfd\x15"_su8, O::F64X2Splat);
  ExpectWrite<O>("\xfd\x16"_su8, O::F64X2ExtractLane);
  ExpectWrite<O>("\xfd\x17"_su8, O::F64X2ReplaceLane);
  ExpectWrite<O>("\xfd\x18"_su8, O::I8X16Eq);
  ExpectWrite<O>("\xfd\x19"_su8, O::I8X16Ne);
  ExpectWrite<O>("\xfd\x1a"_su8, O::I8X16LtS);
  ExpectWrite<O>("\xfd\x1b"_su8, O::I8X16LtU);
  ExpectWrite<O>("\xfd\x1c"_su8, O::I8X16GtS);
  ExpectWrite<O>("\xfd\x1d"_su8, O::I8X16GtU);
  ExpectWrite<O>("\xfd\x1e"_su8, O::I8X16LeS);
  ExpectWrite<O>("\xfd\x1f"_su8, O::I8X16LeU);
  ExpectWrite<O>("\xfd\x20"_su8, O::I8X16GeS);
  ExpectWrite<O>("\xfd\x21"_su8, O::I8X16GeU);
  ExpectWrite<O>("\xfd\x22"_su8, O::I16X8Eq);
  ExpectWrite<O>("\xfd\x23"_su8, O::I16X8Ne);
  ExpectWrite<O>("\xfd\x24"_su8, O::I16X8LtS);
  ExpectWrite<O>("\xfd\x25"_su8, O::I16X8LtU);
  ExpectWrite<O>("\xfd\x26"_su8, O::I16X8GtS);
  ExpectWrite<O>("\xfd\x27"_su8, O::I16X8GtU);
  ExpectWrite<O>("\xfd\x28"_su8, O::I16X8LeS);
  ExpectWrite<O>("\xfd\x29"_su8, O::I16X8LeU);
  ExpectWrite<O>("\xfd\x2a"_su8, O::I16X8GeS);
  ExpectWrite<O>("\xfd\x2b"_su8, O::I16X8GeU);
  ExpectWrite<O>("\xfd\x2c"_su8, O::I32X4Eq);
  ExpectWrite<O>("\xfd\x2d"_su8, O::I32X4Ne);
  ExpectWrite<O>("\xfd\x2e"_su8, O::I32X4LtS);
  ExpectWrite<O>("\xfd\x2f"_su8, O::I32X4LtU);
  ExpectWrite<O>("\xfd\x30"_su8, O::I32X4GtS);
  ExpectWrite<O>("\xfd\x31"_su8, O::I32X4GtU);
  ExpectWrite<O>("\xfd\x32"_su8, O::I32X4LeS);
  ExpectWrite<O>("\xfd\x33"_su8, O::I32X4LeU);
  ExpectWrite<O>("\xfd\x34"_su8, O::I32X4GeS);
  ExpectWrite<O>("\xfd\x35"_su8, O::I32X4GeU);
  ExpectWrite<O>("\xfd\x40"_su8, O::F32X4Eq);
  ExpectWrite<O>("\xfd\x41"_su8, O::F32X4Ne);
  ExpectWrite<O>("\xfd\x42"_su8, O::F32X4Lt);
  ExpectWrite<O>("\xfd\x43"_su8, O::F32X4Gt);
  ExpectWrite<O>("\xfd\x44"_su8, O::F32X4Le);
  ExpectWrite<O>("\xfd\x45"_su8, O::F32X4Ge);
  ExpectWrite<O>("\xfd\x46"_su8, O::F64X2Eq);
  ExpectWrite<O>("\xfd\x47"_su8, O::F64X2Ne);
  ExpectWrite<O>("\xfd\x48"_su8, O::F64X2Lt);
  ExpectWrite<O>("\xfd\x49"_su8, O::F64X2Gt);
  ExpectWrite<O>("\xfd\x4a"_su8, O::F64X2Le);
  ExpectWrite<O>("\xfd\x4b"_su8, O::F64X2Ge);
  ExpectWrite<O>("\xfd\x4c"_su8, O::V128Not);
  ExpectWrite<O>("\xfd\x4d"_su8, O::V128And);
  ExpectWrite<O>("\xfd\x4e"_su8, O::V128Or);
  ExpectWrite<O>("\xfd\x4f"_su8, O::V128Xor);
  ExpectWrite<O>("\xfd\x50"_su8, O::V128BitSelect);
  ExpectWrite<O>("\xfd\x51"_su8, O::I8X16Neg);
  ExpectWrite<O>("\xfd\x52"_su8, O::I8X16AnyTrue);
  ExpectWrite<O>("\xfd\x53"_su8, O::I8X16AllTrue);
  ExpectWrite<O>("\xfd\x54"_su8, O::I8X16Shl);
  ExpectWrite<O>("\xfd\x55"_su8, O::I8X16ShrS);
  ExpectWrite<O>("\xfd\x56"_su8, O::I8X16ShrU);
  ExpectWrite<O>("\xfd\x57"_su8, O::I8X16Add);
  ExpectWrite<O>("\xfd\x58"_su8, O::I8X16AddSaturateS);
  ExpectWrite<O>("\xfd\x59"_su8, O::I8X16AddSaturateU);
  ExpectWrite<O>("\xfd\x5a"_su8, O::I8X16Sub);
  ExpectWrite<O>("\xfd\x5b"_su8, O::I8X16SubSaturateS);
  ExpectWrite<O>("\xfd\x5c"_su8, O::I8X16SubSaturateU);
  ExpectWrite<O>("\xfd\x5d"_su8, O::I8X16Mul);
  ExpectWrite<O>("\xfd\x62"_su8, O::I16X8Neg);
  ExpectWrite<O>("\xfd\x63"_su8, O::I16X8AnyTrue);
  ExpectWrite<O>("\xfd\x64"_su8, O::I16X8AllTrue);
  ExpectWrite<O>("\xfd\x65"_su8, O::I16X8Shl);
  ExpectWrite<O>("\xfd\x66"_su8, O::I16X8ShrS);
  ExpectWrite<O>("\xfd\x67"_su8, O::I16X8ShrU);
  ExpectWrite<O>("\xfd\x68"_su8, O::I16X8Add);
  ExpectWrite<O>("\xfd\x69"_su8, O::I16X8AddSaturateS);
  ExpectWrite<O>("\xfd\x6a"_su8, O::I16X8AddSaturateU);
  ExpectWrite<O>("\xfd\x6b"_su8, O::I16X8Sub);
  ExpectWrite<O>("\xfd\x6c"_su8, O::I16X8SubSaturateS);
  ExpectWrite<O>("\xfd\x6d"_su8, O::I16X8SubSaturateU);
  ExpectWrite<O>("\xfd\x6e"_su8, O::I16X8Mul);
  ExpectWrite<O>("\xfd\x73"_su8, O::I32X4Neg);
  ExpectWrite<O>("\xfd\x74"_su8, O::I32X4AnyTrue);
  ExpectWrite<O>("\xfd\x75"_su8, O::I32X4AllTrue);
  ExpectWrite<O>("\xfd\x76"_su8, O::I32X4Shl);
  ExpectWrite<O>("\xfd\x77"_su8, O::I32X4ShrS);
  ExpectWrite<O>("\xfd\x78"_su8, O::I32X4ShrU);
  ExpectWrite<O>("\xfd\x79"_su8, O::I32X4Add);
  ExpectWrite<O>("\xfd\x7c"_su8, O::I32X4Sub);
  ExpectWrite<O>("\xfd\x7f"_su8, O::I32X4Mul);
  ExpectWrite<O>("\xfd\x84\x01"_su8, O::I64X2Neg);
  ExpectWrite<O>("\xfd\x85\x01"_su8, O::I64X2AnyTrue);
  ExpectWrite<O>("\xfd\x86\x01"_su8, O::I64X2AllTrue);
  ExpectWrite<O>("\xfd\x87\x01"_su8, O::I64X2Shl);
  ExpectWrite<O>("\xfd\x88\x01"_su8, O::I64X2ShrS);
  ExpectWrite<O>("\xfd\x89\x01"_su8, O::I64X2ShrU);
  ExpectWrite<O>("\xfd\x8a\x01"_su8, O::I64X2Add);
  ExpectWrite<O>("\xfd\x8d\x01"_su8, O::I64X2Sub);
  ExpectWrite<O>("\xfd\x95\x01"_su8, O::F32X4Abs);
  ExpectWrite<O>("\xfd\x96\x01"_su8, O::F32X4Neg);
  ExpectWrite<O>("\xfd\x97\x01"_su8, O::F32X4Sqrt);
  ExpectWrite<O>("\xfd\x9a\x01"_su8, O::F32X4Add);
  ExpectWrite<O>("\xfd\x9b\x01"_su8, O::F32X4Sub);
  ExpectWrite<O>("\xfd\x9c\x01"_su8, O::F32X4Mul);
  ExpectWrite<O>("\xfd\x9d\x01"_su8, O::F32X4Div);
  ExpectWrite<O>("\xfd\x9e\x01"_su8, O::F32X4Min);
  ExpectWrite<O>("\xfd\x9f\x01"_su8, O::F32X4Max);
  ExpectWrite<O>("\xfd\xa0\x01"_su8, O::F64X2Abs);
  ExpectWrite<O>("\xfd\xa1\x01"_su8, O::F64X2Neg);
  ExpectWrite<O>("\xfd\xa2\x01"_su8, O::F64X2Sqrt);
  ExpectWrite<O>("\xfd\xa5\x01"_su8, O::F64X2Add);
  ExpectWrite<O>("\xfd\xa6\x01"_su8, O::F64X2Sub);
  ExpectWrite<O>("\xfd\xa7\x01"_su8, O::F64X2Mul);
  ExpectWrite<O>("\xfd\xa8\x01"_su8, O::F64X2Div);
  ExpectWrite<O>("\xfd\xa9\x01"_su8, O::F64X2Min);
  ExpectWrite<O>("\xfd\xaa\x01"_su8, O::F64X2Max);
  ExpectWrite<O>("\xfd\xab\x01"_su8, O::I32X4TruncSatF32X4S);
  ExpectWrite<O>("\xfd\xac\x01"_su8, O::I32X4TruncSatF32X4U);
  ExpectWrite<O>("\xfd\xad\x01"_su8, O::I64X2TruncSatF64X2S);
  ExpectWrite<O>("\xfd\xae\x01"_su8, O::I64X2TruncSatF64X2U);
  ExpectWrite<O>("\xfd\xaf\x01"_su8, O::F32X4ConvertI32X4S);
  ExpectWrite<O>("\xfd\xb0\x01"_su8, O::F32X4ConvertI32X4U);
  ExpectWrite<O>("\xfd\xb1\x01"_su8, O::F64X2ConvertI64X2S);
  ExpectWrite<O>("\xfd\xb2\x01"_su8, O::F64X2ConvertI64X2U);
}

TEST(WriteTest, Opcode_threads) {
  using O = Opcode;

  ExpectWrite<O>("\xfe\x00"_su8, O::AtomicNotify);
  ExpectWrite<O>("\xfe\x01"_su8, O::I32AtomicWait);
  ExpectWrite<O>("\xfe\x02"_su8, O::I64AtomicWait);
  ExpectWrite<O>("\xfe\x10"_su8, O::I32AtomicLoad);
  ExpectWrite<O>("\xfe\x11"_su8, O::I64AtomicLoad);
  ExpectWrite<O>("\xfe\x12"_su8, O::I32AtomicLoad8U);
  ExpectWrite<O>("\xfe\x13"_su8, O::I32AtomicLoad16U);
  ExpectWrite<O>("\xfe\x14"_su8, O::I64AtomicLoad8U);
  ExpectWrite<O>("\xfe\x15"_su8, O::I64AtomicLoad16U);
  ExpectWrite<O>("\xfe\x16"_su8, O::I64AtomicLoad32U);
  ExpectWrite<O>("\xfe\x17"_su8, O::I32AtomicStore);
  ExpectWrite<O>("\xfe\x18"_su8, O::I64AtomicStore);
  ExpectWrite<O>("\xfe\x19"_su8, O::I32AtomicStore8);
  ExpectWrite<O>("\xfe\x1a"_su8, O::I32AtomicStore16);
  ExpectWrite<O>("\xfe\x1b"_su8, O::I64AtomicStore8);
  ExpectWrite<O>("\xfe\x1c"_su8, O::I64AtomicStore16);
  ExpectWrite<O>("\xfe\x1d"_su8, O::I64AtomicStore32);
  ExpectWrite<O>("\xfe\x1e"_su8, O::I32AtomicRmwAdd);
  ExpectWrite<O>("\xfe\x1f"_su8, O::I64AtomicRmwAdd);
  ExpectWrite<O>("\xfe\x20"_su8, O::I32AtomicRmw8AddU);
  ExpectWrite<O>("\xfe\x21"_su8, O::I32AtomicRmw16AddU);
  ExpectWrite<O>("\xfe\x22"_su8, O::I64AtomicRmw8AddU);
  ExpectWrite<O>("\xfe\x23"_su8, O::I64AtomicRmw16AddU);
  ExpectWrite<O>("\xfe\x24"_su8, O::I64AtomicRmw32AddU);
  ExpectWrite<O>("\xfe\x25"_su8, O::I32AtomicRmwSub);
  ExpectWrite<O>("\xfe\x26"_su8, O::I64AtomicRmwSub);
  ExpectWrite<O>("\xfe\x27"_su8, O::I32AtomicRmw8SubU);
  ExpectWrite<O>("\xfe\x28"_su8, O::I32AtomicRmw16SubU);
  ExpectWrite<O>("\xfe\x29"_su8, O::I64AtomicRmw8SubU);
  ExpectWrite<O>("\xfe\x2a"_su8, O::I64AtomicRmw16SubU);
  ExpectWrite<O>("\xfe\x2b"_su8, O::I64AtomicRmw32SubU);
  ExpectWrite<O>("\xfe\x2c"_su8, O::I32AtomicRmwAnd);
  ExpectWrite<O>("\xfe\x2d"_su8, O::I64AtomicRmwAnd);
  ExpectWrite<O>("\xfe\x2e"_su8, O::I32AtomicRmw8AndU);
  ExpectWrite<O>("\xfe\x2f"_su8, O::I32AtomicRmw16AndU);
  ExpectWrite<O>("\xfe\x30"_su8, O::I64AtomicRmw8AndU);
  ExpectWrite<O>("\xfe\x31"_su8, O::I64AtomicRmw16AndU);
  ExpectWrite<O>("\xfe\x32"_su8, O::I64AtomicRmw32AndU);
  ExpectWrite<O>("\xfe\x33"_su8, O::I32AtomicRmwOr);
  ExpectWrite<O>("\xfe\x34"_su8, O::I64AtomicRmwOr);
  ExpectWrite<O>("\xfe\x35"_su8, O::I32AtomicRmw8OrU);
  ExpectWrite<O>("\xfe\x36"_su8, O::I32AtomicRmw16OrU);
  ExpectWrite<O>("\xfe\x37"_su8, O::I64AtomicRmw8OrU);
  ExpectWrite<O>("\xfe\x38"_su8, O::I64AtomicRmw16OrU);
  ExpectWrite<O>("\xfe\x39"_su8, O::I64AtomicRmw32OrU);
  ExpectWrite<O>("\xfe\x3a"_su8, O::I32AtomicRmwXor);
  ExpectWrite<O>("\xfe\x3b"_su8, O::I64AtomicRmwXor);
  ExpectWrite<O>("\xfe\x3c"_su8, O::I32AtomicRmw8XorU);
  ExpectWrite<O>("\xfe\x3d"_su8, O::I32AtomicRmw16XorU);
  ExpectWrite<O>("\xfe\x3e"_su8, O::I64AtomicRmw8XorU);
  ExpectWrite<O>("\xfe\x3f"_su8, O::I64AtomicRmw16XorU);
  ExpectWrite<O>("\xfe\x40"_su8, O::I64AtomicRmw32XorU);
  ExpectWrite<O>("\xfe\x41"_su8, O::I32AtomicRmwXchg);
  ExpectWrite<O>("\xfe\x42"_su8, O::I64AtomicRmwXchg);
  ExpectWrite<O>("\xfe\x43"_su8, O::I32AtomicRmw8XchgU);
  ExpectWrite<O>("\xfe\x44"_su8, O::I32AtomicRmw16XchgU);
  ExpectWrite<O>("\xfe\x45"_su8, O::I64AtomicRmw8XchgU);
  ExpectWrite<O>("\xfe\x46"_su8, O::I64AtomicRmw16XchgU);
  ExpectWrite<O>("\xfe\x47"_su8, O::I64AtomicRmw32XchgU);
  ExpectWrite<O>("\xfe\x48"_su8, O::I32AtomicRmwCmpxchg);
  ExpectWrite<O>("\xfe\x49"_su8, O::I64AtomicRmwCmpxchg);
  ExpectWrite<O>("\xfe\x4a"_su8, O::I32AtomicRmw8CmpxchgU);
  ExpectWrite<O>("\xfe\x4b"_su8, O::I32AtomicRmw16CmpxchgU);
  ExpectWrite<O>("\xfe\x4c"_su8, O::I64AtomicRmw8CmpxchgU);
  ExpectWrite<O>("\xfe\x4d"_su8, O::I64AtomicRmw16CmpxchgU);
  ExpectWrite<O>("\xfe\x4e"_su8, O::I64AtomicRmw32CmpxchgU);
}

TEST(WriteTest, S32) {
  ExpectWrite<s32>("\x20"_su8, 32);
  ExpectWrite<s32>("\x70"_su8, -16);
  ExpectWrite<s32>("\xc0\x03"_su8, 448);
  ExpectWrite<s32>("\xc0\x63"_su8, -3648);
  ExpectWrite<s32>("\xd0\x84\x02"_su8, 33360);
  ExpectWrite<s32>("\xd0\x84\x52"_su8, -753072);
  ExpectWrite<s32>("\xa0\xb0\xc0\x30"_su8, 101718048);
  ExpectWrite<s32>("\xa0\xb0\xc0\x70"_su8, -32499680);
  ExpectWrite<s32>("\xf0\xf0\xf0\xf0\x03"_su8, 1042036848);
  ExpectWrite<s32>("\xf0\xf0\xf0\xf0\x7c"_su8, -837011344);
}

TEST(WriteTest, S64) {
  ExpectWrite<s64>("\x20"_su8, 32);
  ExpectWrite<s64>("\x70"_su8, -16);
  ExpectWrite<s64>("\xc0\x03"_su8, 448);
  ExpectWrite<s64>("\xc0\x63"_su8, -3648);
  ExpectWrite<s64>("\xd0\x84\x02"_su8, 33360);
  ExpectWrite<s64>("\xd0\x84\x52"_su8, -753072);
  ExpectWrite<s64>("\xa0\xb0\xc0\x30"_su8, 101718048);
  ExpectWrite<s64>("\xa0\xb0\xc0\x70"_su8, -32499680);
  ExpectWrite<s64>("\xf0\xf0\xf0\xf0\x03"_su8, 1042036848);
  ExpectWrite<s64>("\xf0\xf0\xf0\xf0\x7c"_su8, -837011344);
  ExpectWrite<s64>("\xe0\xe0\xe0\xe0\x33"_su8, 13893120096);
  ExpectWrite<s64>("\xe0\xe0\xe0\xe0\x51"_su8, -12413554592);
  ExpectWrite<s64>("\xd0\xd0\xd0\xd0\xd0\x2c"_su8, 1533472417872);
  ExpectWrite<s64>("\xd0\xd0\xd0\xd0\xd0\x77"_su8, -287593715632);
  ExpectWrite<s64>("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"_su8, 139105536057408);
  ExpectWrite<s64>("\xc0\xc0\xc0\xc0\xc0\xd0\x63"_su8, -124777254608832);
  ExpectWrite<s64>("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"_su8, 1338117014066474);
  ExpectWrite<s64>("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"_su8, -12172681868045014);
  ExpectWrite<s64>("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"_su8,
                   1070725794579330814);
  ExpectWrite<s64>("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"_su8,
                   -3540960223848057090);
}

TEST(WriteTest, SectionId) {
  ExpectWrite<SectionId>("\x00"_su8, SectionId::Custom);
  ExpectWrite<SectionId>("\x01"_su8, SectionId::Type);
  ExpectWrite<SectionId>("\x02"_su8, SectionId::Import);
  ExpectWrite<SectionId>("\x03"_su8, SectionId::Function);
  ExpectWrite<SectionId>("\x04"_su8, SectionId::Table);
  ExpectWrite<SectionId>("\x05"_su8, SectionId::Memory);
  ExpectWrite<SectionId>("\x06"_su8, SectionId::Global);
  ExpectWrite<SectionId>("\x07"_su8, SectionId::Export);
  ExpectWrite<SectionId>("\x08"_su8, SectionId::Start);
  ExpectWrite<SectionId>("\x09"_su8, SectionId::Element);
  ExpectWrite<SectionId>("\x0a"_su8, SectionId::Code);
  ExpectWrite<SectionId>("\x0b"_su8, SectionId::Data);
  ExpectWrite<SectionId>("\x0c"_su8, SectionId::DataCount);
}

TEST(WriteTest, ShuffleImmediate) {
  ExpectWrite<ShuffleImmediate>(
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
      ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
}

TEST(WriteTest, Start) {
  ExpectWrite<Start>("\x80\x02"_su8, Start{256});
}

TEST(WriteTest, String) {
  ExpectWrite<string_view>("\x05hello"_su8, "hello");
  ExpectWrite<string_view>("\x02hi"_su8, std::string{"hi"});
}

TEST(WriteTest, Table) {
  ExpectWrite<Table>("\x70\x00\x01"_su8,
                     Table{TableType{Limits{1}, ElementType::Funcref}});
}

TEST(WriteTest, TableType) {
  ExpectWrite<TableType>("\x70\x00\x01"_su8,
                         TableType{Limits{1}, ElementType::Funcref});
  ExpectWrite<TableType>("\x70\x01\x01\x02"_su8,
                         TableType{Limits{1, 2}, ElementType::Funcref});
}

TEST(WriteTest, TypeEntry) {
  ExpectWrite<TypeEntry>("\x60\x00\x01\x7f"_su8,
                         TypeEntry{FunctionType{{}, {ValueType::I32}}});
}

TEST(WriteTest, U8) {
  ExpectWrite<u8>("\x2a"_su8, 42);
}

TEST(WriteTest, U32) {
  ExpectWrite<u32>("\x20"_su8, 32u);
  ExpectWrite<u32>("\xc0\x03"_su8, 448u);
  ExpectWrite<u32>("\xd0\x84\x02"_su8, 33360u);
  ExpectWrite<u32>("\xa0\xb0\xc0\x30"_su8, 101718048u);
  ExpectWrite<u32>("\xf0\xf0\xf0\xf0\x03"_su8, 1042036848u);
}

TEST(WriteTest, ValueType) {
  ExpectWrite<ValueType>("\x7f"_su8, ValueType::I32);
  ExpectWrite<ValueType>("\x7e"_su8, ValueType::I64);
  ExpectWrite<ValueType>("\x7d"_su8, ValueType::F32);
  ExpectWrite<ValueType>("\x7c"_su8, ValueType::F64);
  ExpectWrite<ValueType>("\x7b"_su8, ValueType::V128);
  ExpectWrite<ValueType>("\x6f"_su8, ValueType::Anyref);
}

TEST(WriteTest, WriteVector_u8) {
  const auto expected = "\x05hello"_su8;
  const std::vector<u8> input{{'h', 'e', 'l', 'l', 'o'}};
  std::vector<u8> output(expected.size());
  auto iter = WriteVector(input.begin(), input.end(),
                          MakeClampedIterator(output.begin(), output.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), output.end());
  EXPECT_EQ(expected, wasp::SpanU8{output});
}

TEST(WriteTest, WriteVector_u32) {
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
  EXPECT_EQ(expected, wasp::SpanU8{output});
}
