//
// Copyright 2018 WebAssembly Community Group participants
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

#include "wasp/binary/read/read_instruction.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Instruction) {
  using I = Instruction;
  using O = Opcode;
  using MemArg = MemArgImmediate;

  ExpectRead<I>(I{O::Unreachable}, MakeSpanU8("\x00"));
  ExpectRead<I>(I{O::Nop}, MakeSpanU8("\x01"));
  ExpectRead<I>(I{O::Block, BlockType::I32}, MakeSpanU8("\x02\x7f"));
  ExpectRead<I>(I{O::Loop, BlockType::Void}, MakeSpanU8("\x03\x40"));
  ExpectRead<I>(I{O::If, BlockType::F64}, MakeSpanU8("\x04\x7c"));
  ExpectRead<I>(I{O::Else}, MakeSpanU8("\x05"));
  ExpectRead<I>(I{O::End}, MakeSpanU8("\x0b"));
  ExpectRead<I>(I{O::Br, Index{1}}, MakeSpanU8("\x0c\x01"));
  ExpectRead<I>(I{O::BrIf, Index{2}}, MakeSpanU8("\x0d\x02"));
  ExpectRead<I>(I{O::BrTable, BrTableImmediate{{3, 4, 5}, 6}},
                MakeSpanU8("\x0e\x03\x03\x04\x05\x06"));
  ExpectRead<I>(I{O::Return}, MakeSpanU8("\x0f"));
  ExpectRead<I>(I{O::Call, Index{7}}, MakeSpanU8("\x10\x07"));
  ExpectRead<I>(I{O::CallIndirect, CallIndirectImmediate{8, 0}},
                MakeSpanU8("\x11\x08\x00"));
  ExpectRead<I>(I{O::Drop}, MakeSpanU8("\x1a"));
  ExpectRead<I>(I{O::Select}, MakeSpanU8("\x1b"));
  ExpectRead<I>(I{O::LocalGet, Index{5}}, MakeSpanU8("\x20\x05"));
  ExpectRead<I>(I{O::LocalSet, Index{6}}, MakeSpanU8("\x21\x06"));
  ExpectRead<I>(I{O::LocalTee, Index{7}}, MakeSpanU8("\x22\x07"));
  ExpectRead<I>(I{O::GlobalGet, Index{8}}, MakeSpanU8("\x23\x08"));
  ExpectRead<I>(I{O::GlobalSet, Index{9}}, MakeSpanU8("\x24\x09"));
  ExpectRead<I>(I{O::I32Load, MemArg{10, 11}}, MakeSpanU8("\x28\x0a\x0b"));
  ExpectRead<I>(I{O::I64Load, MemArg{12, 13}}, MakeSpanU8("\x29\x0c\x0d"));
  ExpectRead<I>(I{O::F32Load, MemArg{14, 15}}, MakeSpanU8("\x2a\x0e\x0f"));
  ExpectRead<I>(I{O::F64Load, MemArg{16, 17}}, MakeSpanU8("\x2b\x10\x11"));
  ExpectRead<I>(I{O::I32Load8S, MemArg{18, 19}}, MakeSpanU8("\x2c\x12\x13"));
  ExpectRead<I>(I{O::I32Load8U, MemArg{20, 21}}, MakeSpanU8("\x2d\x14\x15"));
  ExpectRead<I>(I{O::I32Load16S, MemArg{22, 23}}, MakeSpanU8("\x2e\x16\x17"));
  ExpectRead<I>(I{O::I32Load16U, MemArg{24, 25}}, MakeSpanU8("\x2f\x18\x19"));
  ExpectRead<I>(I{O::I64Load8S, MemArg{26, 27}}, MakeSpanU8("\x30\x1a\x1b"));
  ExpectRead<I>(I{O::I64Load8U, MemArg{28, 29}}, MakeSpanU8("\x31\x1c\x1d"));
  ExpectRead<I>(I{O::I64Load16S, MemArg{30, 31}}, MakeSpanU8("\x32\x1e\x1f"));
  ExpectRead<I>(I{O::I64Load16U, MemArg{32, 33}}, MakeSpanU8("\x33\x20\x21"));
  ExpectRead<I>(I{O::I64Load32S, MemArg{34, 35}}, MakeSpanU8("\x34\x22\x23"));
  ExpectRead<I>(I{O::I64Load32U, MemArg{36, 37}}, MakeSpanU8("\x35\x24\x25"));
  ExpectRead<I>(I{O::I32Store, MemArg{38, 39}}, MakeSpanU8("\x36\x26\x27"));
  ExpectRead<I>(I{O::I64Store, MemArg{40, 41}}, MakeSpanU8("\x37\x28\x29"));
  ExpectRead<I>(I{O::F32Store, MemArg{42, 43}}, MakeSpanU8("\x38\x2a\x2b"));
  ExpectRead<I>(I{O::F64Store, MemArg{44, 45}}, MakeSpanU8("\x39\x2c\x2d"));
  ExpectRead<I>(I{O::I32Store8, MemArg{46, 47}}, MakeSpanU8("\x3a\x2e\x2f"));
  ExpectRead<I>(I{O::I32Store16, MemArg{48, 49}}, MakeSpanU8("\x3b\x30\x31"));
  ExpectRead<I>(I{O::I64Store8, MemArg{50, 51}}, MakeSpanU8("\x3c\x32\x33"));
  ExpectRead<I>(I{O::I64Store16, MemArg{52, 53}}, MakeSpanU8("\x3d\x34\x35"));
  ExpectRead<I>(I{O::I64Store32, MemArg{54, 55}}, MakeSpanU8("\x3e\x36\x37"));
  ExpectRead<I>(I{O::MemorySize, u8{0}}, MakeSpanU8("\x3f\x00"));
  ExpectRead<I>(I{O::MemoryGrow, u8{0}}, MakeSpanU8("\x40\x00"));
  ExpectRead<I>(I{O::I32Const, s32{0}}, MakeSpanU8("\x41\x00"));
  ExpectRead<I>(I{O::I64Const, s64{0}}, MakeSpanU8("\x42\x00"));
  ExpectRead<I>(I{O::F32Const, f32{0}}, MakeSpanU8("\x43\x00\x00\x00\x00"));
  ExpectRead<I>(I{O::F64Const, f64{0}},
                MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00"));
  ExpectRead<I>(I{O::I32Eqz}, MakeSpanU8("\x45"));
  ExpectRead<I>(I{O::I32Eq}, MakeSpanU8("\x46"));
  ExpectRead<I>(I{O::I32Ne}, MakeSpanU8("\x47"));
  ExpectRead<I>(I{O::I32LtS}, MakeSpanU8("\x48"));
  ExpectRead<I>(I{O::I32LtU}, MakeSpanU8("\x49"));
  ExpectRead<I>(I{O::I32GtS}, MakeSpanU8("\x4a"));
  ExpectRead<I>(I{O::I32GtU}, MakeSpanU8("\x4b"));
  ExpectRead<I>(I{O::I32LeS}, MakeSpanU8("\x4c"));
  ExpectRead<I>(I{O::I32LeU}, MakeSpanU8("\x4d"));
  ExpectRead<I>(I{O::I32GeS}, MakeSpanU8("\x4e"));
  ExpectRead<I>(I{O::I32GeU}, MakeSpanU8("\x4f"));
  ExpectRead<I>(I{O::I64Eqz}, MakeSpanU8("\x50"));
  ExpectRead<I>(I{O::I64Eq}, MakeSpanU8("\x51"));
  ExpectRead<I>(I{O::I64Ne}, MakeSpanU8("\x52"));
  ExpectRead<I>(I{O::I64LtS}, MakeSpanU8("\x53"));
  ExpectRead<I>(I{O::I64LtU}, MakeSpanU8("\x54"));
  ExpectRead<I>(I{O::I64GtS}, MakeSpanU8("\x55"));
  ExpectRead<I>(I{O::I64GtU}, MakeSpanU8("\x56"));
  ExpectRead<I>(I{O::I64LeS}, MakeSpanU8("\x57"));
  ExpectRead<I>(I{O::I64LeU}, MakeSpanU8("\x58"));
  ExpectRead<I>(I{O::I64GeS}, MakeSpanU8("\x59"));
  ExpectRead<I>(I{O::I64GeU}, MakeSpanU8("\x5a"));
  ExpectRead<I>(I{O::F32Eq}, MakeSpanU8("\x5b"));
  ExpectRead<I>(I{O::F32Ne}, MakeSpanU8("\x5c"));
  ExpectRead<I>(I{O::F32Lt}, MakeSpanU8("\x5d"));
  ExpectRead<I>(I{O::F32Gt}, MakeSpanU8("\x5e"));
  ExpectRead<I>(I{O::F32Le}, MakeSpanU8("\x5f"));
  ExpectRead<I>(I{O::F32Ge}, MakeSpanU8("\x60"));
  ExpectRead<I>(I{O::F64Eq}, MakeSpanU8("\x61"));
  ExpectRead<I>(I{O::F64Ne}, MakeSpanU8("\x62"));
  ExpectRead<I>(I{O::F64Lt}, MakeSpanU8("\x63"));
  ExpectRead<I>(I{O::F64Gt}, MakeSpanU8("\x64"));
  ExpectRead<I>(I{O::F64Le}, MakeSpanU8("\x65"));
  ExpectRead<I>(I{O::F64Ge}, MakeSpanU8("\x66"));
  ExpectRead<I>(I{O::I32Clz}, MakeSpanU8("\x67"));
  ExpectRead<I>(I{O::I32Ctz}, MakeSpanU8("\x68"));
  ExpectRead<I>(I{O::I32Popcnt}, MakeSpanU8("\x69"));
  ExpectRead<I>(I{O::I32Add}, MakeSpanU8("\x6a"));
  ExpectRead<I>(I{O::I32Sub}, MakeSpanU8("\x6b"));
  ExpectRead<I>(I{O::I32Mul}, MakeSpanU8("\x6c"));
  ExpectRead<I>(I{O::I32DivS}, MakeSpanU8("\x6d"));
  ExpectRead<I>(I{O::I32DivU}, MakeSpanU8("\x6e"));
  ExpectRead<I>(I{O::I32RemS}, MakeSpanU8("\x6f"));
  ExpectRead<I>(I{O::I32RemU}, MakeSpanU8("\x70"));
  ExpectRead<I>(I{O::I32And}, MakeSpanU8("\x71"));
  ExpectRead<I>(I{O::I32Or}, MakeSpanU8("\x72"));
  ExpectRead<I>(I{O::I32Xor}, MakeSpanU8("\x73"));
  ExpectRead<I>(I{O::I32Shl}, MakeSpanU8("\x74"));
  ExpectRead<I>(I{O::I32ShrS}, MakeSpanU8("\x75"));
  ExpectRead<I>(I{O::I32ShrU}, MakeSpanU8("\x76"));
  ExpectRead<I>(I{O::I32Rotl}, MakeSpanU8("\x77"));
  ExpectRead<I>(I{O::I32Rotr}, MakeSpanU8("\x78"));
  ExpectRead<I>(I{O::I64Clz}, MakeSpanU8("\x79"));
  ExpectRead<I>(I{O::I64Ctz}, MakeSpanU8("\x7a"));
  ExpectRead<I>(I{O::I64Popcnt}, MakeSpanU8("\x7b"));
  ExpectRead<I>(I{O::I64Add}, MakeSpanU8("\x7c"));
  ExpectRead<I>(I{O::I64Sub}, MakeSpanU8("\x7d"));
  ExpectRead<I>(I{O::I64Mul}, MakeSpanU8("\x7e"));
  ExpectRead<I>(I{O::I64DivS}, MakeSpanU8("\x7f"));
  ExpectRead<I>(I{O::I64DivU}, MakeSpanU8("\x80"));
  ExpectRead<I>(I{O::I64RemS}, MakeSpanU8("\x81"));
  ExpectRead<I>(I{O::I64RemU}, MakeSpanU8("\x82"));
  ExpectRead<I>(I{O::I64And}, MakeSpanU8("\x83"));
  ExpectRead<I>(I{O::I64Or}, MakeSpanU8("\x84"));
  ExpectRead<I>(I{O::I64Xor}, MakeSpanU8("\x85"));
  ExpectRead<I>(I{O::I64Shl}, MakeSpanU8("\x86"));
  ExpectRead<I>(I{O::I64ShrS}, MakeSpanU8("\x87"));
  ExpectRead<I>(I{O::I64ShrU}, MakeSpanU8("\x88"));
  ExpectRead<I>(I{O::I64Rotl}, MakeSpanU8("\x89"));
  ExpectRead<I>(I{O::I64Rotr}, MakeSpanU8("\x8a"));
  ExpectRead<I>(I{O::F32Abs}, MakeSpanU8("\x8b"));
  ExpectRead<I>(I{O::F32Neg}, MakeSpanU8("\x8c"));
  ExpectRead<I>(I{O::F32Ceil}, MakeSpanU8("\x8d"));
  ExpectRead<I>(I{O::F32Floor}, MakeSpanU8("\x8e"));
  ExpectRead<I>(I{O::F32Trunc}, MakeSpanU8("\x8f"));
  ExpectRead<I>(I{O::F32Nearest}, MakeSpanU8("\x90"));
  ExpectRead<I>(I{O::F32Sqrt}, MakeSpanU8("\x91"));
  ExpectRead<I>(I{O::F32Add}, MakeSpanU8("\x92"));
  ExpectRead<I>(I{O::F32Sub}, MakeSpanU8("\x93"));
  ExpectRead<I>(I{O::F32Mul}, MakeSpanU8("\x94"));
  ExpectRead<I>(I{O::F32Div}, MakeSpanU8("\x95"));
  ExpectRead<I>(I{O::F32Min}, MakeSpanU8("\x96"));
  ExpectRead<I>(I{O::F32Max}, MakeSpanU8("\x97"));
  ExpectRead<I>(I{O::F32Copysign}, MakeSpanU8("\x98"));
  ExpectRead<I>(I{O::F64Abs}, MakeSpanU8("\x99"));
  ExpectRead<I>(I{O::F64Neg}, MakeSpanU8("\x9a"));
  ExpectRead<I>(I{O::F64Ceil}, MakeSpanU8("\x9b"));
  ExpectRead<I>(I{O::F64Floor}, MakeSpanU8("\x9c"));
  ExpectRead<I>(I{O::F64Trunc}, MakeSpanU8("\x9d"));
  ExpectRead<I>(I{O::F64Nearest}, MakeSpanU8("\x9e"));
  ExpectRead<I>(I{O::F64Sqrt}, MakeSpanU8("\x9f"));
  ExpectRead<I>(I{O::F64Add}, MakeSpanU8("\xa0"));
  ExpectRead<I>(I{O::F64Sub}, MakeSpanU8("\xa1"));
  ExpectRead<I>(I{O::F64Mul}, MakeSpanU8("\xa2"));
  ExpectRead<I>(I{O::F64Div}, MakeSpanU8("\xa3"));
  ExpectRead<I>(I{O::F64Min}, MakeSpanU8("\xa4"));
  ExpectRead<I>(I{O::F64Max}, MakeSpanU8("\xa5"));
  ExpectRead<I>(I{O::F64Copysign}, MakeSpanU8("\xa6"));
  ExpectRead<I>(I{O::I32WrapI64}, MakeSpanU8("\xa7"));
  ExpectRead<I>(I{O::I32TruncF32S}, MakeSpanU8("\xa8"));
  ExpectRead<I>(I{O::I32TruncF32U}, MakeSpanU8("\xa9"));
  ExpectRead<I>(I{O::I32TruncF64S}, MakeSpanU8("\xaa"));
  ExpectRead<I>(I{O::I32TruncF64U}, MakeSpanU8("\xab"));
  ExpectRead<I>(I{O::I64ExtendI32S}, MakeSpanU8("\xac"));
  ExpectRead<I>(I{O::I64ExtendI32U}, MakeSpanU8("\xad"));
  ExpectRead<I>(I{O::I64TruncF32S}, MakeSpanU8("\xae"));
  ExpectRead<I>(I{O::I64TruncF32U}, MakeSpanU8("\xaf"));
  ExpectRead<I>(I{O::I64TruncF64S}, MakeSpanU8("\xb0"));
  ExpectRead<I>(I{O::I64TruncF64U}, MakeSpanU8("\xb1"));
  ExpectRead<I>(I{O::F32ConvertI32S}, MakeSpanU8("\xb2"));
  ExpectRead<I>(I{O::F32ConvertI32U}, MakeSpanU8("\xb3"));
  ExpectRead<I>(I{O::F32ConvertI64S}, MakeSpanU8("\xb4"));
  ExpectRead<I>(I{O::F32ConvertI64U}, MakeSpanU8("\xb5"));
  ExpectRead<I>(I{O::F32DemoteF64}, MakeSpanU8("\xb6"));
  ExpectRead<I>(I{O::F64ConvertI32S}, MakeSpanU8("\xb7"));
  ExpectRead<I>(I{O::F64ConvertI32U}, MakeSpanU8("\xb8"));
  ExpectRead<I>(I{O::F64ConvertI64S}, MakeSpanU8("\xb9"));
  ExpectRead<I>(I{O::F64ConvertI64U}, MakeSpanU8("\xba"));
  ExpectRead<I>(I{O::F64PromoteF32}, MakeSpanU8("\xbb"));
  ExpectRead<I>(I{O::I32ReinterpretF32}, MakeSpanU8("\xbc"));
  ExpectRead<I>(I{O::I64ReinterpretF64}, MakeSpanU8("\xbd"));
  ExpectRead<I>(I{O::F32ReinterpretI32}, MakeSpanU8("\xbe"));
  ExpectRead<I>(I{O::F64ReinterpretI64}, MakeSpanU8("\xbf"));
}

TEST(ReaderTest, Instruction_BadMemoryReserved) {
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x3f\x01"));
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x40\x01"));
}

TEST(ReaderTest, Instruction_sign_extension) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_sign_extension();

  ExpectRead<I>(I{O::I32Extend8S}, MakeSpanU8("\xc0"), features);
  ExpectRead<I>(I{O::I32Extend16S}, MakeSpanU8("\xc1"), features);
  ExpectRead<I>(I{O::I64Extend8S}, MakeSpanU8("\xc2"), features);
  ExpectRead<I>(I{O::I64Extend16S}, MakeSpanU8("\xc3"), features);
  ExpectRead<I>(I{O::I64Extend32S}, MakeSpanU8("\xc4"), features);
}

TEST(ReaderTest, Instruction_saturating_float_to_int) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_saturating_float_to_int();

  ExpectRead<I>(I{O::I32TruncSatF32S}, MakeSpanU8("\xfc\x00"), features);
  ExpectRead<I>(I{O::I32TruncSatF32U}, MakeSpanU8("\xfc\x01"), features);
  ExpectRead<I>(I{O::I32TruncSatF64S}, MakeSpanU8("\xfc\x02"), features);
  ExpectRead<I>(I{O::I32TruncSatF64U}, MakeSpanU8("\xfc\x03"), features);
  ExpectRead<I>(I{O::I64TruncSatF32S}, MakeSpanU8("\xfc\x04"), features);
  ExpectRead<I>(I{O::I64TruncSatF32U}, MakeSpanU8("\xfc\x05"), features);
  ExpectRead<I>(I{O::I64TruncSatF64S}, MakeSpanU8("\xfc\x06"), features);
  ExpectRead<I>(I{O::I64TruncSatF64U}, MakeSpanU8("\xfc\x07"), features);
}

TEST(ReaderTest, Instruction_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_bulk_memory();

  ExpectRead<I>(I{O::MemoryInit, InitImmediate{0, 1}},
                MakeSpanU8("\xfc\x08\x00\x01"), features);
  ExpectRead<I>(I{O::MemoryDrop, Index{2}}, MakeSpanU8("\xfc\x09\x02"),
                features);
  ExpectRead<I>(I{O::MemoryCopy, u8{0}}, MakeSpanU8("\xfc\x0a\x00"), features);
  ExpectRead<I>(I{O::MemoryFill, u8{0}}, MakeSpanU8("\xfc\x0b\x00"), features);
  ExpectRead<I>(I{O::TableInit, InitImmediate{0, 3}},
                MakeSpanU8("\xfc\x0c\x00\x03"), features);
  ExpectRead<I>(I{O::TableDrop, Index{4}}, MakeSpanU8("\xfc\x0d\x04"),
                features);
  ExpectRead<I>(I{O::TableCopy, u8{0}}, MakeSpanU8("\xfc\x0e\x00"), features);
}
