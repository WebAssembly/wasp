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

#include "wasp/binary/read.h"

#include <cmath>
#include <vector>

#include "gtest/gtest.h"
#include "test/binary/test_utils.h"
#include "test/test_utils.h"
#include "wasp/binary/name_section/read.h"
#include "wasp/binary/read/context.h"
#include "wasp/binary/read/read_vector.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;
using namespace ::wasp::binary::test;

using I = Instruction;
using O = Opcode;

class BinaryReadTest : public ::testing::Test {
 protected:
  template <typename Func, typename T, typename... Args>
  void OK(Func&& func, const T& expected, SpanU8 data, Args&&... args) {
    auto actual = func(&data, context, std::forward<Args>(args)...);
    ExpectNoErrors(errors);
    EXPECT_EQ(0u, data.size());
    EXPECT_NE(nullptr, actual->loc().data());
    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(expected, **actual);
  }

  template <typename Func, typename... Args>
  void Fail(Func&& func,
            const ExpectedError& error,
            SpanU8 data,
            Args&&... args) {
    auto orig_data = data;
    auto actual = func(&data, context, std::forward<Args>(args)...);
    EXPECT_FALSE(actual.has_value());
    ExpectError(error, errors, orig_data);
    errors.Clear();
  }

  void FailUnknownOpcode(u8 code) {
    const u8 span_buffer[] = {code};
    auto msg = format("Unknown opcode: {}", code);
    Fail(Read<Opcode>, {{0, "opcode"}, {1, msg}}, SpanU8{span_buffer, 1});
  }

  void FailUnknownOpcode(u8 prefix, u32 orig_code) {
    u8 data[] = {prefix, 0, 0, 0, 0, 0};
    u32 code = orig_code;
    size_t length = 1;
    do {
      data[length++] = (code & 0x7f) | (code >= 0x80 ? 0x80 : 0);
      code >>= 7;
    } while (code > 0);

    Fail(Read<Opcode>,
         {{0, "opcode"},
          {0, format("Unknown opcode: {} {}", prefix, orig_code)}},
         SpanU8{data, length});
  }

  TestErrors errors;
  Context context{errors};
};

TEST_F(BinaryReadTest, BlockType_MVP) {
  OK(Read<BlockType>, BlockType::I32, "\x7f"_su8);
  OK(Read<BlockType>, BlockType::I64, "\x7e"_su8);
  OK(Read<BlockType>, BlockType::F32, "\x7d"_su8);
  OK(Read<BlockType>, BlockType::F64, "\x7c"_su8);
  OK(Read<BlockType>, BlockType::Void, "\x40"_su8);
}

TEST_F(BinaryReadTest, BlockType_Basic_multi_value) {
  context.features.enable_multi_value();

  OK(Read<BlockType>, BlockType::I32, "\x7f"_su8);
  OK(Read<BlockType>, BlockType::I64, "\x7e"_su8);
  OK(Read<BlockType>, BlockType::F32, "\x7d"_su8);
  OK(Read<BlockType>, BlockType::F64, "\x7c"_su8);
  OK(Read<BlockType>, BlockType::Void, "\x40"_su8);
}

TEST_F(BinaryReadTest, BlockType_simd) {
  Fail(Read<BlockType>, {{0, "block type"}, {1, "Unknown block type: 123"}},
       "\x7b"_su8);

  context.features.enable_simd();
  OK(Read<BlockType>, BlockType::V128, "\x7b"_su8);
}

TEST_F(BinaryReadTest, BlockType_MultiValueNegative) {
  context.features.enable_multi_value();
  Fail(Read<BlockType>, {{0, "block type"}, {1, "Unknown block type: -9"}},
       "\x77"_su8);
}

TEST_F(BinaryReadTest, BlockType_multi_value) {
  Fail(Read<BlockType>, {{0, "block type"}, {1, "Unknown block type: 1"}},
       "\x01"_su8);

  context.features.enable_multi_value();
  OK(Read<BlockType>, BlockType(1), "\x01"_su8);
  OK(Read<BlockType>, BlockType(448), "\xc0\x03"_su8);
}

TEST_F(BinaryReadTest, BlockType_reference_types) {
  Fail(Read<BlockType>, {{0, "block type"}, {1, "Unknown block type: 111"}},
       "\x6f"_su8);

  context.features.enable_reference_types();
  OK(Read<BlockType>, BlockType::Externref, "\x6f"_su8);
}

TEST_F(BinaryReadTest, BlockType_Unknown) {
  Fail(Read<BlockType>, {{0, "block type"}, {1, "Unknown block type: 0"}},
       "\x00"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<BlockType>, {{0, "block type"}, {1, "Unknown block type: 255"}},
       "\xff\x7f"_su8);
}

TEST_F(BinaryReadTest, BrOnExnImmediate) {
  OK(Read<BrOnExnImmediate>,
       BrOnExnImmediate{MakeAt("\x00"_su8, Index{0}),
                        MakeAt("\x00"_su8, Index{0})},
       "\x00\x00"_su8);
}

TEST_F(BinaryReadTest, BrOnExnImmediate_PastEnd) {
  Fail(Read<BrOnExnImmediate>,
       {{0, "br_on_exn"}, {0, "target"}, {0, "Unable to read u8"}}, ""_su8);

  Fail(Read<BrOnExnImmediate>,
       {{0, "br_on_exn"}, {1, "event index"}, {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, BrTableImmediate) {
  OK(Read<BrTableImmediate>, BrTableImmediate{{}, MakeAt("\x00"_su8, Index{0})},
     "\x00\x00"_su8);

  OK(Read<BrTableImmediate>,
     BrTableImmediate{
         {MakeAt("\x01"_su8, Index{1}), MakeAt("\x02"_su8, Index{2})},
         MakeAt("\x03"_su8, Index{3})},
     "\x02\x01\x02\x03"_su8);
}

TEST_F(BinaryReadTest, BrTableImmediate_PastEnd) {
  Fail(
      Read<BrTableImmediate>,
      {{0, "br_table"}, {0, "targets"}, {0, "count"}, {0, "Unable to read u8"}},
      ""_su8);

  Fail(Read<BrTableImmediate>,
       {{0, "br_table"}, {1, "default target"}, {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, ReadBytes) {
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 3, context);
  ExpectNoErrors(errors);
  EXPECT_EQ(data, **result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, ReadBytes_Leftovers) {
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, context);
  ExpectNoErrors(errors);
  EXPECT_EQ(data.subspan(0, 2), **result);
  EXPECT_EQ(1u, copy.size());
}

TEST_F(BinaryReadTest, ReadBytes_Fail) {
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, context);
  EXPECT_EQ(nullopt, result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

TEST_F(BinaryReadTest, CallIndirectImmediate) {
  OK(Read<CallIndirectImmediate>,
     CallIndirectImmediate{MakeAt("\x01"_su8, Index{1}),
                           MakeAt("\x00"_su8, Index{0})},
     "\x01\x00"_su8);
  OK(Read<CallIndirectImmediate>,
     CallIndirectImmediate{MakeAt("\x80\x01"_su8, Index{128}),
                           MakeAt("\x00"_su8, Index{0})},
     "\x80\x01\x00"_su8);
}

TEST_F(BinaryReadTest, CallIndirectImmediate_BadReserved) {
  Fail(Read<CallIndirectImmediate>,
       {{0, "call_indirect"},
        {1, "reserved"},
        {1, "Expected reserved byte 0, got 1"}},
       "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, CallIndirectImmediate_PastEnd) {
  Fail(Read<CallIndirectImmediate>,
       {{0, "call_indirect"}, {0, "type index"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<CallIndirectImmediate>,
       {{0, "call_indirect"}, {1, "reserved"}, {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, Code) {
  // Empty body. This will fail validation, but can still be read.
  OK(Read<Code>, Code{{}, MakeAt(""_su8, ""_expr)}, "\x01\x00"_su8);

  // Smallest valid empty body.
  OK(Read<Code>, Code{{}, MakeAt("\x0b"_su8, "\x0b"_expr)}, "\x02\x00\x0b"_su8);

  // (func
  //   (local i32 i32 i64 i64 i64)
  //   (nop))
  OK(Read<Code>,
     Code{{MakeAt("\x02\x7f"_su8, Locals{MakeAt("\x02"_su8, Index{2}),
                                         MakeAt("\x7f"_su8, ValueType::I32)}),
           MakeAt("\x03\x7e"_su8, Locals{MakeAt("\x03"_su8, Index{3}),
                                         MakeAt("\x7e"_su8, ValueType::I64)})},
          MakeAt("\x01\x0b"_su8, "\x01\x0b"_expr)},
     "\x07\x02\x02\x7f\x03\x7e\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, Code_PastEnd) {
  Fail(Read<Code>, {{0, "code"}, {0, "length"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<Code>, {{0, "code"}, {0, "Length extends past end: 1 > 0"}},
       "\x01"_su8);

  Fail(
      Read<Code>,
      {{0, "code"}, {1, "locals vector"}, {1, "Count extends past end: 1 > 0"}},
      "\x01\x01"_su8);
}

TEST_F(BinaryReadTest, Code_TooManyLocals) {
  Fail(Read<Code>,
       {{0, "code"},
        {1, "locals vector"},
        {8, "locals"},
        {8, "Too many locals: 4294967296"}},
       "\x09"                      // length
       "\x02"                      // local decls count
       "\xfe\xff\xff\xff\x0f\x7f"  // (local i32 ** (2**32) - 2)
       "\x02\x7e"_su8              // (local i64 i64)
  );
}

TEST_F(BinaryReadTest, ConstantExpression) {
  // i32.const
  OK(Read<ConstantExpression>,
     ConstantExpression{MakeAt("\x41\x00"_su8,
                               Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                           MakeAt("\x00"_su8, s32{0})})},
     "\x41\x00\x0b"_su8);

  // i64.const
  OK(Read<ConstantExpression>,
     ConstantExpression{
         MakeAt("\x42\x80\x80\x80\x80\x80\x01"_su8,
                Instruction{
                    MakeAt("\x42"_su8, Opcode::I64Const),
                    MakeAt("\x80\x80\x80\x80\x80\x01"_su8, s64{34359738368})})},
     "\x42\x80\x80\x80\x80\x80\x01\x0b"_su8);

  // f32.const
  OK(Read<ConstantExpression>,
     ConstantExpression{
         MakeAt("\x43\x00\x00\x00\x00"_su8,
                Instruction{MakeAt("\x43"_su8, Opcode::F32Const),
                            MakeAt("\x00\x00\x00\x00"_su8, f32{0})})},
     "\x43\x00\x00\x00\x00\x0b"_su8);

  // f64.const
  OK(Read<ConstantExpression>,
     ConstantExpression{MakeAt(
         "\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
         Instruction{MakeAt("\x44"_su8, Opcode::F64Const),
                     MakeAt("\x00\x00\x00\x00\x00\x00\x00\x00"_su8, f64{0})})},
     "\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b"_su8);

  // global.get
  OK(Read<ConstantExpression>,
     ConstantExpression{MakeAt(
         "\x23\x00"_su8, Instruction{MakeAt("\x23"_su8, Opcode::GlobalGet),
                                     MakeAt("\x00"_su8, Index{0})})},
     "\x23\x00\x0b"_su8);

  // Other instructions are invalid, but not malformed.
  OK(Read<ConstantExpression>,
     ConstantExpression{
         MakeAt("\x01"_su8, Instruction{MakeAt("\x01"_su8, Opcode::Nop)})},
     "\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_ReferenceTypes) {
  // ref.null
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {1, "Unknown opcode: 208"}},
       "\xd0\x70\x0b"_su8);

  // ref.func
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {1, "Unknown opcode: 210"}},
       "\xd2\x00\x0b"_su8);

  context.features.enable_reference_types();

  // ref.null
  OK(Read<ConstantExpression>,
     ConstantExpression{
         MakeAt("\xd0\x70"_su8,
                Instruction{MakeAt("\xd0"_su8, Opcode::RefNull),
                            MakeAt("\x70"_su8, ReferenceType::Funcref)})},
     "\xd0\x70\x0b"_su8);

  // ref.func
  OK(Read<ConstantExpression>,
     ConstantExpression{
         MakeAt("\xd2\x00"_su8, Instruction{MakeAt("\xd2"_su8, Opcode::RefFunc),
                                            MakeAt("\x00"_su8, Index{0})})},
     "\xd2\x00\x0b"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_NoEnd) {
  // i32.const
  Fail(Read<ConstantExpression>,
     {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
     "\x41\x00"_su8);

  // i64.const
  Fail(Read<ConstantExpression>,
     {{0, "constant expression"}, {7, "opcode"}, {7, "Unable to read u8"}},
     "\x42\x80\x80\x80\x80\x80\x01"_su8);

  // f32.const
  Fail(Read<ConstantExpression>,
     {{0, "constant expression"}, {5, "opcode"}, {5, "Unable to read u8"}},
     "\x43\x00\x00\x00\x00"_su8);

  // f64.const
  Fail(Read<ConstantExpression>,
     {{0, "constant expression"}, {9, "opcode"}, {9, "Unable to read u8"}},
     "\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8);

  // global.get
  Fail(Read<ConstantExpression>,
     {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
     "\x23\x00"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_TooShort) {
  // An instruction sequence of length 0 is invalid, but not malformed.
  OK(Read<ConstantExpression>, ConstantExpression{}, "\x0b"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_TooLong) {
  // An instruction sequence of length > 1 is invalid, but not malformed.
  OK(Read<ConstantExpression>,
     ConstantExpression{InstructionList{
         MakeAt("\x41\x00"_su8,
                Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                            MakeAt("\x00"_su8, s32{0})}),
         MakeAt("\x01"_su8, Instruction{MakeAt("\x01"_su8, Opcode::Nop)}),
     }},
     "\x41\x00\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_InvalidInstruction) {
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {1, "Unknown opcode: 6"}},
       "\x06"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_PastEnd) {
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
       ""_su8);
}

auto ReadMemoryCopyImmediate_ForTesting(SpanU8* data, Context& context)
    -> OptAt<CopyImmediate> {
  return Read<CopyImmediate>(data, context, BulkImmediateKind::Memory);
}

auto ReadTableCopyImmediate_ForTesting(SpanU8* data, Context& context)
    -> OptAt<CopyImmediate> {
  return Read<CopyImmediate>(data, context, BulkImmediateKind::Table);
}

TEST_F(BinaryReadTest, CopyImmediate) {
  OK(ReadMemoryCopyImmediate_ForTesting,
     CopyImmediate{MakeAt("\x00"_su8, Index{0}), MakeAt("\x00"_su8, Index{0})},
     "\x00\x00"_su8);

  OK(ReadTableCopyImmediate_ForTesting,
     CopyImmediate{MakeAt("\x00"_su8, Index{0}), MakeAt("\x00"_su8, Index{0})},
     "\x00\x00"_su8);
}

TEST_F(BinaryReadTest, CopyImmediate_BadReserved) {
  Fail(ReadMemoryCopyImmediate_ForTesting,
       {{0, "copy immediate"},
        {0, "reserved"},
        {0, "Expected reserved byte 0, got 1"}},
       "\x01"_su8);

  Fail(ReadMemoryCopyImmediate_ForTesting,
       {{0, "copy immediate"},
        {1, "reserved"},
        {1, "Expected reserved byte 0, got 1"}},
       "\x00\x01"_su8);

  Fail(ReadTableCopyImmediate_ForTesting,
       {{0, "copy immediate"},
        {0, "reserved"},
        {0, "Expected reserved byte 0, got 1"}},
       "\x01"_su8);

  Fail(ReadTableCopyImmediate_ForTesting,
       {{0, "copy immediate"},
        {1, "reserved"},
        {1, "Expected reserved byte 0, got 1"}},
       "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, CopyImmediate_PastEnd) {
  Fail(ReadMemoryCopyImmediate_ForTesting,
       {{0, "copy immediate"}, {0, "reserved"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(ReadMemoryCopyImmediate_ForTesting,
       {{0, "copy immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(ReadTableCopyImmediate_ForTesting,
       {{0, "copy immediate"}, {0, "reserved"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(ReadTableCopyImmediate_ForTesting,
       {{0, "copy immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, CopyImmediate_Table_reference_types) {
  context.features.enable_reference_types();

  OK(ReadTableCopyImmediate_ForTesting,
     CopyImmediate{MakeAt("\x80\x01"_su8, Index{128}),
                   MakeAt("\x01"_su8, Index{1})},
     "\x80\x01\x01"_su8);

  OK(ReadTableCopyImmediate_ForTesting,
     CopyImmediate{MakeAt("\x01"_su8, Index{1}),
                   MakeAt("\x80\x01"_su8, Index{128})},
     "\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, CopyImmediate_Memory_reference_types) {
  context.features.enable_reference_types();

  Fail(ReadMemoryCopyImmediate_ForTesting,
       {{0, "copy immediate"},
        {0, "reserved"},
        {0, "Expected reserved byte 0, got 128"}},
       "\x80\x01\x01"_su8);

  Fail(ReadMemoryCopyImmediate_ForTesting,
       {{0, "copy immediate"},
        {0, "reserved"},
        {0, "Expected reserved byte 0, got 1"}},
       "\x01\x80\x01"_su8);
}


TEST_F(BinaryReadTest, ShuffleImmediate) {
  OK(Read<ShuffleImmediate>,
     ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
     "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
}

TEST_F(BinaryReadTest, ShuffleImmediate_PastEnd) {
  Fail(Read<ShuffleImmediate>,
       {{0, "shuffle immediate"}, {0, "Unable to read u8"}}, ""_su8);

  Fail(Read<ShuffleImmediate>,
       {{0, "shuffle immediate"}, {15, "Unable to read u8"}},
       "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
}

TEST_F(BinaryReadTest, ReadCount) {
  const SpanU8 data = "\x01\x00\x00\x00"_su8;
  SpanU8 copy = data;
  auto result = ReadCount(&copy, context);
  ExpectNoErrors(errors);
  EXPECT_EQ(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST_F(BinaryReadTest, ReadCount_PastEnd) {
  const SpanU8 data = "\x05\x00\x00\x00"_su8;
  SpanU8 copy = data;
  auto result = ReadCount(&copy, context);
  ExpectError({{0, "Count extends past end: 5 > 3"}}, errors, data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(3u, copy.size());
}

TEST_F(BinaryReadTest, DataSegment_MVP) {
  OK(Read<DataSegment>,
     DataSegment{MakeAt("\x01"_su8, Index{1}),
                 MakeAt("\x42\x01\x0b"_su8,
                        ConstantExpression{MakeAt(
                            "\x42\x01"_su8,
                            Instruction{MakeAt("\x42"_su8, Opcode::I64Const),
                                        MakeAt("\x01"_su8, s64{1})})}),
                 MakeAt("\x04wxyz"_su8, "wxyz"_su8)},
     "\x01\x42\x01\x0b\x04wxyz"_su8);
}

TEST_F(BinaryReadTest, DataSegment_MVP_PastEnd) {
  Fail(Read<DataSegment>,
       {{0, "data segment"}, {0, "memory index"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"},
        {1, "offset"},
        {1, "constant expression"},
        {1, "opcode"},
        {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"}, {4, "length"}, {4, "Unable to read u8"}},
       "\x00\x41\x00\x0b"_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"}, {4, "Length extends past end: 2 > 0"}},
       "\x00\x41\x00\x0b\x02"_su8);
}

TEST_F(BinaryReadTest, DataSegment_BulkMemory) {
  context.features.enable_bulk_memory();

  OK(Read<DataSegment>, DataSegment{MakeAt("\x04wxyz"_su8, "wxyz"_su8)},
     "\x01\x04wxyz"_su8);

  OK(Read<DataSegment>,
     DataSegment{MakeAt("\x01"_su8, Index{1}),
                 MakeAt("\x41\x02\x0b"_su8,
                        ConstantExpression{MakeAt(
                            "\x41\x02"_su8,
                            Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                        MakeAt("\x02"_su8, s32{2})})}),
                 MakeAt("\x03xyz"_su8, "xyz"_su8)},
     "\x02\x01\x41\x02\x0b\x03xyz"_su8);
}

TEST_F(BinaryReadTest, DataSegment_BulkMemory_BadFlags) {
  context.features.enable_bulk_memory();

  Fail(Read<DataSegment>, {{0, "data segment"}, {1, "Unknown flags: 3"}},
       "\x03"_su8);
}

TEST_F(BinaryReadTest, DataSegment_BulkMemory_PastEnd) {
  context.features.enable_bulk_memory();

  Fail(Read<DataSegment>,
       {{0, "data segment"}, {0, "flags"}, {0, "Unable to read u8"}}, ""_su8);

  // Passive.
  Fail(Read<DataSegment>,
       {{0, "data segment"}, {1, "length"}, {1, "Unable to read u8"}},
       "\x01"_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"}, {1, "Length extends past end: 1 > 0"}},
       "\x01\x01"_su8);

  // Active w/ memory index.
  Fail(Read<DataSegment>,
       {{0, "data segment"}, {1, "memory index"}, {1, "Unable to read u8"}},
       "\x02"_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"},
        {2, "offset"},
        {2, "constant expression"},
        {2, "opcode"},
        {2, "Unable to read u8"}},
       "\x02\x00"_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"}, {5, "length"}, {5, "Unable to read u8"}},
       "\x02\x00\x41\x00\x0b"_su8);

  Fail(Read<DataSegment>,
       {{0, "data segment"}, {5, "Length extends past end: 1 > 0"}},
       "\x02\x00\x41\x00\x0b\x01"_su8);
}

TEST_F(BinaryReadTest, ElementExpression) {
  context.features.enable_bulk_memory();

  // ref.null
  OK(Read<ElementExpression>,
     ElementExpression{
         MakeAt("\xd0\x70"_su8,
                Instruction{MakeAt("\xd0"_su8, Opcode::RefNull),
                            MakeAt("\x70"_su8, ReferenceType::Funcref)})},
     "\xd0\x70\x0b"_su8);

  // ref.func 2
  OK(Read<ElementExpression>,
     ElementExpression{
         MakeAt("\xd2\x02"_su8, Instruction{MakeAt("\xd2"_su8, Opcode::RefFunc),
                                            MakeAt("\x02"_su8, Index{2})})},
     "\xd2\x02\x0b"_su8);

  // Other instructions are invalid, but not malformed.
  OK(Read<ElementExpression>,
     ElementExpression{
         MakeAt("\x01"_su8, Instruction{MakeAt("\x01"_su8, Opcode::Nop)})},
     "\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_NoEnd) {
  context.features.enable_bulk_memory();

  // ref.null
  Fail(Read<ElementExpression>,
       {{0, "element expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
       "\xd0\x70"_su8);

  // ref.func
  Fail(Read<ElementExpression>,
       {{0, "element expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
       "\xd2\x00"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_TooShort) {
  context.features.enable_bulk_memory();

  // An instruction sequence of length 0 is invalid, but not malformed.
  OK(Read<ElementExpression>, ElementExpression{}, "\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_TooLong) {
  context.features.enable_bulk_memory();

  OK(Read<ElementExpression>,
     ElementExpression{InstructionList{
         MakeAt("\xd0\x70"_su8,
                Instruction{MakeAt("\xd0"_su8, Opcode::RefNull),
                            MakeAt("\x70"_su8, ReferenceType::Funcref)}),
         MakeAt("\x01"_su8, Instruction{MakeAt("\x01"_su8, Opcode::Nop)}),
     }},
     "\xd0\x70\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_InvalidInstruction) {
  context.features.enable_bulk_memory();

  Fail(Read<ElementExpression>,
       {{0, "element expression"}, {0, "opcode"}, {1, "Unknown opcode: 6"}},
       "\x06"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_PastEnd) {
  context.features.enable_bulk_memory();

  Fail(Read<ElementExpression>,
       {{0, "element expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
       ""_su8);
}

TEST_F(BinaryReadTest, ElementSegment_MVP) {
  OK(Read<ElementSegment>,
     ElementSegment{MakeAt("\x00"_su8, Index{0}),
                    MakeAt("\x41\x01\x0b"_su8,
                           ConstantExpression{MakeAt(
                               "\x41\x01"_su8,
                               Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                           MakeAt("\x01"_su8, s32{1})})}),
                    ElementListWithIndexes{ExternalKind::Function,
                                           {MakeAt("\x01"_su8, Index{1}),
                                            MakeAt("\x02"_su8, Index{2}),
                                            MakeAt("\x03"_su8, Index{3})}}},
     "\x00\x41\x01\x0b\x03\x01\x02\x03"_su8);
}

TEST_F(BinaryReadTest, ElementSegment_MVP_PastEnd) {
  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {0, "table index"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<ElementSegment>,
       {{0, "element segment"},
        {1, "offset"},
        {1, "constant expression"},
        {1, "opcode"},
        {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(Read<ElementSegment>,
       {{0, "element segment"},
        {4, "initializers"},
        {4, "count"},
        {4, "Unable to read u8"}},
       "\x00\x23\x00\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementSegment_BulkMemory) {
  context.features.enable_bulk_memory();

  // Flags == 1: Passive, index list
  OK(Read<ElementSegment>,
     ElementSegment{
         SegmentType::Passive,
         ElementListWithIndexes{
             MakeAt("\x00"_su8, ExternalKind::Function),
             {MakeAt("\x01"_su8, Index{1}), MakeAt("\x02"_su8, Index{2})}}},
     "\x01\x00\x02\x01\x02"_su8);

  // Flags == 2: Active, table index, index list
  OK(Read<ElementSegment>,  //*
     ElementSegment{
         MakeAt("\x01"_su8, Index{1}),
         MakeAt("\x41\x02\x0b"_su8,
                ConstantExpression{
                    MakeAt("\x41\x02"_su8,
                           Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                       MakeAt("\x02"_su8, s32{2})})}),
         ElementListWithIndexes{
             MakeAt("\x00"_su8, ExternalKind::Function),
             {MakeAt("\x03"_su8, Index{3}), MakeAt("\x04"_su8, Index{4})}}},
     "\x02\x01\x41\x02\x0b\x00\x02\x03\x04"_su8);

  // Flags == 4: Active (function only), table 0, expression list
  OK(Read<ElementSegment>,
     ElementSegment{
         Index{0},
         MakeAt("\x41\x05\x0b"_su8,
                ConstantExpression{
                    MakeAt("\x41\x05"_su8,
                           Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                       MakeAt("\x05"_su8, s32{5})})}),
         ElementListWithExpressions{
             ReferenceType::Funcref,
             {MakeAt("\xd2\x06\x0b"_su8,
                     ElementExpression{
                         MakeAt("\xd2\x06"_su8,
                                Instruction{MakeAt("\xd2"_su8, Opcode::RefFunc),
                                            MakeAt("\x06"_su8, Index{6})})})}}},
     "\x04\x41\x05\x0b\x01\xd2\x06\x0b"_su8);

  // Flags == 5: Passive, expression list
  OK(Read<ElementSegment>,
     ElementSegment{
         SegmentType::Passive,
         ElementListWithExpressions{
             MakeAt("\x70"_su8, ReferenceType::Funcref),
             {MakeAt("\xd2\x07\x0b"_su8,
                     ElementExpression{
                         MakeAt("\xd2\x07"_su8,
                                Instruction{MakeAt("\xd2"_su8, Opcode::RefFunc),
                                            MakeAt("\x07"_su8, Index{7})})}),
              MakeAt("\xd0\x70\x0b"_su8,
                     ElementExpression{MakeAt(
                         "\xd0\x70"_su8,
                         Instruction{
                             MakeAt("\xd0"_su8, Opcode::RefNull),
                             MakeAt("\x70"_su8, ReferenceType::Funcref)})})}}},
     "\x05\x70\x02\xd2\x07\x0b\xd0\x70\x0b"_su8);

  // Flags == 6: Active, table index, expression list
  OK(Read<ElementSegment>,  //*
     ElementSegment{
         MakeAt("\x02"_su8, Index{2}),
         MakeAt("\x41\x08\x0b"_su8,
                ConstantExpression{
                    MakeAt("\x41\x08"_su8,
                           Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                       MakeAt("\x08"_su8, s32{8})})}),
         ElementListWithExpressions{
             MakeAt("\x70"_su8, ReferenceType::Funcref),
             {MakeAt("\xd0\x70\x0b"_su8,
                     ElementExpression{MakeAt(
                         "\xd0\x70"_su8,
                         Instruction{
                             MakeAt("\xd0"_su8, Opcode::RefNull),
                             MakeAt("\x70"_su8, ReferenceType::Funcref)})})}}},
     "\x06\x02\x41\x08\x0b\x70\x01\xd0\x70\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementSegment_BulkMemory_BadFlags) {
  context.features.enable_bulk_memory();

  // Flags == 3: Declared, index list
  Fail(Read<ElementSegment>, {{0, "element segment"}, {1, "Unknown flags: 3"}},
       "\x03"_su8);

  // Flags == 7: Declared, expression list
  Fail(Read<ElementSegment>, {{0, "element segment"}, {1, "Unknown flags: 7"}},
       "\x07"_su8);
}

TEST_F(BinaryReadTest, ElementSegment_BulkMemory_PastEnd) {
  context.features.enable_bulk_memory();

  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {0, "flags"}, {0, "Unable to read u8"}},
       ""_su8);

  // Flags == 1: Passive, index list
  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {1, "external kind"}, {1, "Unable to read u8"}},
       "\x01"_su8);

  // Flags == 2: Active, table index, index list
  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {1, "table index"}, {1, "Unable to read u8"}},
       "\x02"_su8);

  // Flags == 4: Active (function only), table 0, expression list
  Fail(Read<ElementSegment>,
       {{0, "element segment"},
        {1, "offset"},
        {1, "constant expression"},
        {1, "opcode"},
        {1, "Unable to read u8"}},
       "\x04"_su8);

  // Flags == 5: Passive, expression list
  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {1, "element type"}, {1, "Unable to read u8"}},
       "\x05"_su8);

  // Flags == 6: Active, table index, expression list
  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {1, "table index"}, {1, "Unable to read u8"}},
       "\x06"_su8);
}

TEST_F(BinaryReadTest, ReferenceType) {
  OK(Read<ReferenceType>, ReferenceType::Funcref, "\x70"_su8);
}

TEST_F(BinaryReadTest, ReferenceType_ReferenceTypes) {
  Fail(Read<ReferenceType>,
       {{0, "element type"}, {1, "Unknown element type: 111"}}, "\x6f"_su8);

  context.features.enable_reference_types();

  OK(Read<ReferenceType>, ReferenceType::Externref, "\x6f"_su8);
}

TEST_F(BinaryReadTest, ReferenceType_Exceptions) {
  Fail(Read<ReferenceType>,
       {{0, "element type"}, {1, "Unknown element type: 104"}}, "\x68"_su8);

  context.features.enable_exceptions();

  OK(Read<ReferenceType>, ReferenceType::Exnref, "\x68"_su8);
}


TEST_F(BinaryReadTest, ReferenceType_Unknown) {
  Fail(Read<ReferenceType>,
       {{0, "element type"}, {1, "Unknown element type: 0"}}, "\x00"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<ReferenceType>,
       {{0, "element type"}, {1, "Unknown element type: 240"}}, "\xf0\x7f"_su8);
}

TEST_F(BinaryReadTest, Event) {
  OK(Read<Event>,
     Event{MakeAt("\x00\x01"_su8,
                  EventType{MakeAt("\x00"_su8, EventAttribute::Exception),
                            MakeAt("\x01"_su8, Index{1})})},
     "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Event_PastEnd) {
  Fail(Read<Event>,
       {{0, "event"},
        {0, "event type"},
        {0, "event attribute"},
        {0, "u32"},
        {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<Event>,
       {{0, "event"},
        {0, "event type"},
        {1, "type index"},
        {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, EventType) {
  OK(Read<EventType>,
     EventType{MakeAt("\x00"_su8, EventAttribute::Exception),
               MakeAt("\x01"_su8, Index{1})},
     "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Export) {
  OK(Read<Export>,
     Export{MakeAt("\x00"_su8, ExternalKind::Function),
            MakeAt("\x02hi"_su8, "hi"_sv), MakeAt("\x03"_su8, Index{3})},
     "\x02hi\x00\x03"_su8);
  OK(Read<Export>,
     Export{MakeAt("\x01"_su8, ExternalKind::Table), MakeAt("\x00"_su8, ""_sv),
            MakeAt("\xe8\x07"_su8, Index{1000})},
     "\x00\x01\xe8\x07"_su8);
  OK(Read<Export>,
     Export{MakeAt("\x02"_su8, ExternalKind::Memory),
            MakeAt("\x03mem"_su8, "mem"_sv), MakeAt("\x00"_su8, Index{0})},
     "\x03mem\x02\x00"_su8);
  OK(Read<Export>,
     Export{MakeAt("\x03"_su8, ExternalKind::Global),
            MakeAt("\x01g"_su8, "g"_sv), MakeAt("\x01"_su8, Index{1})},
     "\x01g\x03\x01"_su8);
}

TEST_F(BinaryReadTest, Export_PastEnd) {
  Fail(Read<Export>,
       {{0, "export"}, {0, "name"}, {0, "length"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<Export>,
       {{0, "export"}, {1, "external kind"}, {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(Read<Export>, {{0, "export"}, {2, "index"}, {2, "Unable to read u8"}},
       "\x00\x00"_su8);
}

TEST_F(BinaryReadTest, Export_exceptions) {
  Fail(Read<Export>,
     {{0, "export"}, {2, "external kind"}, {3, "Unknown external kind: 4"}},
     "\x01v\x04\x02"_su8);

  context.features.enable_exceptions();
  OK(Read<Export>,
     Export{MakeAt("\x04"_su8, ExternalKind::Event),
            MakeAt("\x01v"_su8, "v"_sv), MakeAt("\x02"_su8, Index{2})},
     "\x01v\x04\x02"_su8);
}

TEST_F(BinaryReadTest, ExternalKind) {
  OK(Read<ExternalKind>, ExternalKind::Function, "\x00"_su8);
  OK(Read<ExternalKind>, ExternalKind::Table, "\x01"_su8);
  OK(Read<ExternalKind>, ExternalKind::Memory, "\x02"_su8);
  OK(Read<ExternalKind>, ExternalKind::Global, "\x03"_su8);
}

TEST_F(BinaryReadTest, ExternalKind_exceptions) {
  Fail(Read<ExternalKind>,
     {{0, "external kind"}, {1, "Unknown external kind: 4"}}, "\x04"_su8);

  context.features.enable_exceptions();

  OK(Read<ExternalKind>, ExternalKind::Event, "\x04"_su8);
}

TEST_F(BinaryReadTest, ExternalKind_Unknown) {
  Fail(Read<ExternalKind>,
       {{0, "external kind"}, {1, "Unknown external kind: 5"}}, "\x05"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<ExternalKind>,
       {{0, "external kind"}, {1, "Unknown external kind: 132"}},
       "\x84\x00"_su8);
}

TEST_F(BinaryReadTest, F32) {
  OK(Read<f32>, 0.0f, "\x00\x00\x00\x00"_su8);
  OK(Read<f32>, -1.0f, "\x00\x00\x80\xbf"_su8);
  OK(Read<f32>, 1234567.0f, "\x38\xb4\x96\x49"_su8);
  OK(Read<f32>, INFINITY, "\x00\x00\x80\x7f"_su8);
  OK(Read<f32>, -INFINITY, "\x00\x00\x80\xff"_su8);

  // NaN
  {
    auto data = "\x00\x00\xc0\x7f"_su8;
    auto result = Read<f32>(&data, context);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST_F(BinaryReadTest, F32_PastEnd) {
  Fail(Read<f32>, {{0, "f32"}, {0, "Unable to read 4 bytes"}},
       "\x00\x00\x00"_su8);
}

TEST_F(BinaryReadTest, F64) {
  OK(Read<f64>, 0.0, "\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<f64>, -1.0, "\x00\x00\x00\x00\x00\x00\xf0\xbf"_su8);
  OK(Read<f64>, 111111111111111, "\xc0\x71\xbc\x93\x84\x43\xd9\x42"_su8);
  OK(Read<f64>, INFINITY, "\x00\x00\x00\x00\x00\x00\xf0\x7f"_su8);
  OK(Read<f64>, -INFINITY, "\x00\x00\x00\x00\x00\x00\xf0\xff"_su8);

  // NaN
  {
    auto data = "\x00\x00\x00\x00\x00\x00\xf8\x7f"_su8;
    auto result = Read<f64>(&data, context);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST_F(BinaryReadTest, F64_PastEnd) {
  Fail(Read<f64>, {{0, "f64"}, {0, "Unable to read 8 bytes"}},
       "\x00\x00\x00\x00\x00\x00\x00"_su8);
}

TEST_F(BinaryReadTest, Function) {
  OK(Read<Function>, Function{MakeAt("\x01"_su8, Index{1})}, "\x01"_su8);
}

TEST_F(BinaryReadTest, Function_PastEnd) {
  Fail(Read<Function>,
       {{0, "function"}, {0, "type index"}, {0, "Unable to read u8"}}, ""_su8);
}

TEST_F(BinaryReadTest, FunctionType) {
  OK(Read<FunctionType>, FunctionType{{}, {}}, "\x00\x00"_su8);
  OK(Read<FunctionType>,
     FunctionType{{MakeAt("\x7f"_su8, ValueType::I32),
                   MakeAt("\x7e"_su8, ValueType::I64)},
                  {MakeAt("\x7c"_su8, ValueType::F64)}},
     "\x02\x7f\x7e\x01\x7c"_su8);
}

TEST_F(BinaryReadTest, FunctionType_PastEnd) {
  Fail(Read<FunctionType>,
       {{0, "function type"},
        {0, "param types"},
        {0, "count"},
        {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<FunctionType>,
       {{0, "function type"},
        {0, "param types"},
        {0, "Count extends past end: 1 > 0"}},
       "\x01"_su8);

  Fail(Read<FunctionType>,
       {{0, "function type"},
        {1, "result types"},
        {1, "count"},
        {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(Read<FunctionType>,
       {{0, "function type"},
        {1, "result types"},
        {1, "Count extends past end: 1 > 0"}},
       "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Global) {
  // i32 global with i64.const constant expression. This will fail validation
  // but still can be successfully parsed.
  OK(Read<Global>,
     Global{MakeAt("\x7f\x01"_su8,
                   GlobalType{MakeAt("\x7f"_su8, ValueType::I32),
                              MakeAt("\x01"_su8, Mutability::Var)}),
            MakeAt("\x42\x00\x0b"_su8,
                   ConstantExpression{
                       MakeAt("\x42\x00"_su8,
                              Instruction{MakeAt("\x42"_su8, Opcode::I64Const),
                                          MakeAt("\x00"_su8, s64{0})})})},
     "\x7f\x01\x42\x00\x0b"_su8);
}

TEST_F(BinaryReadTest, Global_PastEnd) {
  Fail(Read<Global>,
       {{0, "global"},
        {0, "global type"},
        {0, "value type"},
        {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<Global>,
       {{0, "global"},
        {2, "constant expression"},
        {2, "opcode"},
        {2, "Unable to read u8"}},
       "\x7f\x00"_su8);
}

TEST_F(BinaryReadTest, GlobalType) {
  OK(Read<GlobalType>,
     GlobalType{MakeAt("\x7f"_su8, ValueType::I32),
                MakeAt("\x00"_su8, Mutability::Const)},
     "\x7f\x00"_su8);
  OK(Read<GlobalType>,
     GlobalType{MakeAt("\x7d"_su8, ValueType::F32),
                MakeAt("\x01"_su8, Mutability::Var)},
     "\x7d\x01"_su8);
}

TEST_F(BinaryReadTest, GlobalType_PastEnd) {
  Fail(Read<GlobalType>,
       {{0, "global type"}, {0, "value type"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<GlobalType>,
       {{0, "global type"}, {1, "mutability"}, {1, "Unable to read u8"}},
       "\x7f"_su8);
}

TEST_F(BinaryReadTest, Import) {
  OK(Read<Import>,
     Import{MakeAt("\x01"
                   "a"_su8,
                   "a"_sv),
            MakeAt("\x04"
                   "func"_su8,
                   "func"_sv),
            MakeAt("\x0b"_su8, Index{11})},
     "\x01"
     "a\x04"
     "func\x00\x0b"_su8);

  OK(Read<Import>,
     Import{MakeAt("\x01"
                   "b"_su8,
                   "b"_sv),
            MakeAt("\x05table"_su8, "table"_sv),
            MakeAt("\x70\x00\x01"_su8,
                   TableType{MakeAt("\x00\x01"_su8,
                                    Limits{MakeAt("\x01"_su8, u32{1}), nullopt,
                                           MakeAt("\x00"_su8, Shared::No)}),
                             MakeAt("\x70"_su8, ReferenceType::Funcref)})},
     "\x01"
     "b\x05table\x01\x70\x00\x01"_su8);

  OK(Read<Import>,
     Import{MakeAt("\x01"
                   "c"_su8,
                   "c"_sv),
            MakeAt("\x06memory"_su8, "memory"_sv),
            MakeAt("\x01\x00\x02"_su8,
                   MemoryType{
                       MakeAt("\x01\x00\x02"_su8,
                              Limits{MakeAt("\x00"_su8, u32{0}),
                                     MakeAt("\x02"_su8, u32{2}),
                                     MakeAt("\x01"_su8, Shared::No)}),
                   })},
     "\x01"
     "c\x06memory\x02\x01\x00\x02"_su8);

  OK(Read<Import>,
     Import{MakeAt("\x01"
                   "d"_su8,
                   "d"_sv),
            MakeAt("\x06global"_su8, "global"_sv),
            MakeAt("\x7f\x00"_su8,
                   GlobalType{MakeAt("\x7f"_su8, ValueType::I32),
                              MakeAt("\x00"_su8, Mutability::Const)})},
     "\x01\x64\x06global\x03\x7f\x00"_su8);
}

TEST_F(BinaryReadTest, Import_exceptions) {
  Fail(Read<Import>,
       {{0, "import"}, {9, "external kind"}, {10, "Unknown external kind: 4"}},
       "\x01v\x06!event\x04\x00\x02"_su8);

  context.features.enable_exceptions();
  OK(Read<Import>,
     Import{MakeAt("\x01v"_su8, "v"_sv), MakeAt("\x06!event"_su8, "!event"_sv),
            MakeAt("\x00\x02"_su8,
                   EventType{MakeAt("\x00"_su8, EventAttribute::Exception),
                             MakeAt("\x02"_su8, Index{2})})},
     "\x01v\x06!event\x04\x00\x02"_su8);
}

TEST_F(BinaryReadTest, ImportType_PastEnd) {
  Fail(Read<Import>,
       {{0, "import"},
        {0, "module name"},
        {0, "length"},
        {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<Import>,
       {{0, "import"},
        {1, "field name"},
        {1, "length"},
        {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(Read<Import>,
       {{0, "import"}, {2, "external kind"}, {2, "Unable to read u8"}},
       "\x00\x00"_su8);

  Fail(Read<Import>,
       {{0, "import"}, {3, "function index"}, {3, "Unable to read u8"}},
       "\x00\x00\x00"_su8);

  Fail(Read<Import>,
       {{0, "import"},
        {3, "table type"},
        {3, "element type"},
        {3, "Unable to read u8"}},
       "\x00\x00\x01"_su8);

  Fail(Read<Import>,
       {{0, "import"},
        {3, "memory type"},
        {3, "limits"},
        {3, "flags"},
        {3, "Unable to read u8"}},
       "\x00\x00\x02"_su8);

  Fail(Read<Import>,
       {{0, "import"},
        {3, "global type"},
        {3, "value type"},
        {3, "Unable to read u8"}},
       "\x00\x00\x03"_su8);
}

TEST_F(BinaryReadTest, IndirectNameAssoc) {
  OK(Read<IndirectNameAssoc>,
     IndirectNameAssoc{MakeAt("\x64"_su8, Index{100}),
                       {MakeAt("\x00\x04zero"_su8,
                               NameAssoc{MakeAt("\x00"_su8, Index{0}),
                                         MakeAt("\x04zero"_su8, "zero"_sv)}),
                        MakeAt("\x01\x03one"_su8,
                               NameAssoc{MakeAt("\x01"_su8, Index{1}),
                                         MakeAt("\x03one"_su8, "one"_sv)})}},
     "\x64"             // Index.
     "\x02"             // Count.
     "\x00\x04zero"     // 0 "zero"
     "\x01\x03one"_su8  // 1 "one"
  );
}

TEST_F(BinaryReadTest, IndirectNameAssoc_PastEnd) {
  Fail(Read<IndirectNameAssoc>,
       {{0, "indirect name assoc"}, {0, "index"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<IndirectNameAssoc>,
       {{0, "indirect name assoc"},
        {1, "name map"},
        {1, "count"},
        {1, "Unable to read u8"}},
       "\x00"_su8);

  Fail(Read<IndirectNameAssoc>,
       {{0, "indirect name assoc"},
        {1, "name map"},
        {1, "Count extends past end: 1 > 0"}},
       "\x00\x01"_su8);
}

auto ReadMemoryInitImmediate_ForTesting(SpanU8* data, Context& context)
    -> OptAt<InitImmediate> {
  return Read<InitImmediate>(data, context, BulkImmediateKind::Memory);
}

auto ReadTableInitImmediate_ForTesting(SpanU8* data, Context& context)
    -> OptAt<InitImmediate> {
  return Read<InitImmediate>(data, context, BulkImmediateKind::Table);
}

TEST_F(BinaryReadTest, InitImmediate) {
  OK(ReadMemoryInitImmediate_ForTesting,
     InitImmediate{MakeAt("\x01"_su8, Index{1}), MakeAt("\x00"_su8, Index{0})},
     "\x01\x00"_su8);

  OK(ReadMemoryInitImmediate_ForTesting,
     InitImmediate{MakeAt("\x80\x01"_su8, Index{128}),
                   MakeAt("\x00"_su8, Index{0})},
     "\x80\x01\x00"_su8);

  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{MakeAt("\x01"_su8, Index{1}), MakeAt("\x00"_su8, Index{0})},
     "\x01\x00"_su8);

  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{MakeAt("\x80\x01"_su8, Index{128}),
                   MakeAt("\x00"_su8, Index{0})},
     "\x80\x01\x00"_su8);
}

TEST_F(BinaryReadTest, InitImmediate_BadReserved) {
  Fail(ReadMemoryInitImmediate_ForTesting,
       {{0, "init immediate"},
        {1, "reserved"},
        {1, "Expected reserved byte 0, got 1"}},
       "\x00\x01"_su8);

  Fail(ReadTableInitImmediate_ForTesting,
       {{0, "init immediate"},
        {1, "reserved"},
        {1, "Expected reserved byte 0, got 1"}},
       "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, InitImmediate_PastEnd) {
  Fail(ReadMemoryInitImmediate_ForTesting,
       {{0, "init immediate"}, {0, "segment index"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(ReadMemoryInitImmediate_ForTesting,
       {{0, "init immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
       "\x01"_su8);

  Fail(ReadTableInitImmediate_ForTesting,
       {{0, "init immediate"}, {0, "segment index"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(ReadTableInitImmediate_ForTesting,
       {{0, "init immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
       "\x01"_su8);
}

TEST_F(BinaryReadTest, InitImmediate_Table_reference_types) {
  context.features.enable_reference_types();

  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{MakeAt("\x01"_su8, Index{1}), MakeAt("\x01"_su8, Index{1})},
     "\x01\x01"_su8);
  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{MakeAt("\x80\x01"_su8, Index{128}),
                   MakeAt("\x80\x01"_su8, Index{128})},
     "\x80\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, InitImmediate_Memory_reference_types) {
  context.features.enable_reference_types();

  Fail(ReadMemoryInitImmediate_ForTesting,
       {{0, "init immediate"},
        {1, "reserved"},
        {1, "Expected reserved byte 0, got 1"}},
       "\x01\x01"_su8);
  Fail(ReadMemoryInitImmediate_ForTesting,
       {{0, "init immediate"},
        {2, "reserved"},
        {2, "Expected reserved byte 0, got 128"}},
       "\x80\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, PlainInstruction) {
  using MemArg = MemArgImmediate;

  auto memarg = MakeAt("\x01\x02"_su8, MemArg{MakeAt("\x01"_su8, u32{1}),
                                              MakeAt("\x02"_su8, u32{2})});

  OK(Read<I>, I{MakeAt("\x00"_su8, O::Unreachable)}, "\x00"_su8);
  OK(Read<I>, I{MakeAt("\x01"_su8, O::Nop)}, "\x01"_su8);
  OK(Read<I>, I{MakeAt("\x0c"_su8, O::Br), MakeAt("\x01"_su8, Index{1})},
     "\x0c\x01"_su8);
  OK(Read<I>, I{MakeAt("\x0d"_su8, O::BrIf), MakeAt("\x02"_su8, Index{2})},
     "\x0d\x02"_su8);
  OK(Read<I>,
     I{MakeAt("\x0e"_su8, O::BrTable),
       MakeAt("\x03\x03\x04\x05\x06"_su8,
              BrTableImmediate{
                  {MakeAt("\x03"_su8, Index{3}), MakeAt("\x04"_su8, Index{4}),
                   MakeAt("\x05"_su8, Index{5})},
                  MakeAt("\x06"_su8, Index{6})})},
     "\x0e\x03\x03\x04\x05\x06"_su8);
  OK(Read<I>, I{MakeAt("\x0f"_su8, O::Return)}, "\x0f"_su8);
  OK(Read<I>, I{MakeAt("\x10"_su8, O::Call), MakeAt("\x07"_su8, Index{7})},
     "\x10\x07"_su8);
  OK(Read<I>,
     I{MakeAt("\x11"_su8, O::CallIndirect),
       MakeAt("\x08\x00"_su8,
              CallIndirectImmediate{MakeAt("\x08"_su8, Index{8}),
                                    MakeAt("\x00"_su8, Index{0})})},
     "\x11\x08\x00"_su8);
  OK(Read<I>, I{MakeAt("\x1a"_su8, O::Drop)}, "\x1a"_su8);
  OK(Read<I>, I{MakeAt("\x1b"_su8, O::Select)}, "\x1b"_su8);
  OK(Read<I>, I{MakeAt("\x20"_su8, O::LocalGet), MakeAt("\x05"_su8, Index{5})},
     "\x20\x05"_su8);
  OK(Read<I>, I{MakeAt("\x21"_su8, O::LocalSet), MakeAt("\x06"_su8, Index{6})},
     "\x21\x06"_su8);
  OK(Read<I>, I{MakeAt("\x22"_su8, O::LocalTee), MakeAt("\x07"_su8, Index{7})},
     "\x22\x07"_su8);
  OK(Read<I>, I{MakeAt("\x23"_su8, O::GlobalGet), MakeAt("\x08"_su8, Index{8})},
     "\x23\x08"_su8);
  OK(Read<I>, I{MakeAt("\x24"_su8, O::GlobalSet), MakeAt("\x09"_su8, Index{9})},
     "\x24\x09"_su8);
  OK(Read<I>, I{MakeAt("\x28"_su8, O::I32Load), memarg}, "\x28\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x29"_su8, O::I64Load), memarg}, "\x29\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x2a"_su8, O::F32Load), memarg}, "\x2a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x2b"_su8, O::F64Load), memarg}, "\x2b\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x2c"_su8, O::I32Load8S), memarg}, "\x2c\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x2d"_su8, O::I32Load8U), memarg}, "\x2d\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x2e"_su8, O::I32Load16S), memarg}, "\x2e\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x2f"_su8, O::I32Load16U), memarg}, "\x2f\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x30"_su8, O::I64Load8S), memarg}, "\x30\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x31"_su8, O::I64Load8U), memarg}, "\x31\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x32"_su8, O::I64Load16S), memarg}, "\x32\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x33"_su8, O::I64Load16U), memarg}, "\x33\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x34"_su8, O::I64Load32S), memarg}, "\x34\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x35"_su8, O::I64Load32U), memarg}, "\x35\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x36"_su8, O::I32Store), memarg}, "\x36\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x37"_su8, O::I64Store), memarg}, "\x37\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x38"_su8, O::F32Store), memarg}, "\x38\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x39"_su8, O::F64Store), memarg}, "\x39\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x3a"_su8, O::I32Store8), memarg}, "\x3a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x3b"_su8, O::I32Store16), memarg}, "\x3b\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x3c"_su8, O::I64Store8), memarg}, "\x3c\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x3d"_su8, O::I64Store16), memarg}, "\x3d\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x3e"_su8, O::I64Store32), memarg}, "\x3e\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\x3f"_su8, O::MemorySize), MakeAt("\x00"_su8, u8{0})},
     "\x3f\x00"_su8);
  OK(Read<I>, I{MakeAt("\x40"_su8, O::MemoryGrow), MakeAt("\x00"_su8, u8{0})},
     "\x40\x00"_su8);
  OK(Read<I>, I{MakeAt("\x41"_su8, O::I32Const), MakeAt("\x00"_su8, s32{0})},
     "\x41\x00"_su8);
  OK(Read<I>, I{MakeAt("\x42"_su8, O::I64Const), MakeAt("\x00"_su8, s64{0})},
     "\x42\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\x43"_su8, O::F32Const), MakeAt("\x00\x00\x00\x00"_su8, f32{0})},
     "\x43\x00\x00\x00\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\x44"_su8, O::F64Const),
       MakeAt("\x00\x00\x00\x00\x00\x00\x00\x00"_su8, f64{0})},
     "\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<I>, I{MakeAt("\x45"_su8, O::I32Eqz)}, "\x45"_su8);
  OK(Read<I>, I{MakeAt("\x46"_su8, O::I32Eq)}, "\x46"_su8);
  OK(Read<I>, I{MakeAt("\x47"_su8, O::I32Ne)}, "\x47"_su8);
  OK(Read<I>, I{MakeAt("\x48"_su8, O::I32LtS)}, "\x48"_su8);
  OK(Read<I>, I{MakeAt("\x49"_su8, O::I32LtU)}, "\x49"_su8);
  OK(Read<I>, I{MakeAt("\x4a"_su8, O::I32GtS)}, "\x4a"_su8);
  OK(Read<I>, I{MakeAt("\x4b"_su8, O::I32GtU)}, "\x4b"_su8);
  OK(Read<I>, I{MakeAt("\x4c"_su8, O::I32LeS)}, "\x4c"_su8);
  OK(Read<I>, I{MakeAt("\x4d"_su8, O::I32LeU)}, "\x4d"_su8);
  OK(Read<I>, I{MakeAt("\x4e"_su8, O::I32GeS)}, "\x4e"_su8);
  OK(Read<I>, I{MakeAt("\x4f"_su8, O::I32GeU)}, "\x4f"_su8);
  OK(Read<I>, I{MakeAt("\x50"_su8, O::I64Eqz)}, "\x50"_su8);
  OK(Read<I>, I{MakeAt("\x51"_su8, O::I64Eq)}, "\x51"_su8);
  OK(Read<I>, I{MakeAt("\x52"_su8, O::I64Ne)}, "\x52"_su8);
  OK(Read<I>, I{MakeAt("\x53"_su8, O::I64LtS)}, "\x53"_su8);
  OK(Read<I>, I{MakeAt("\x54"_su8, O::I64LtU)}, "\x54"_su8);
  OK(Read<I>, I{MakeAt("\x55"_su8, O::I64GtS)}, "\x55"_su8);
  OK(Read<I>, I{MakeAt("\x56"_su8, O::I64GtU)}, "\x56"_su8);
  OK(Read<I>, I{MakeAt("\x57"_su8, O::I64LeS)}, "\x57"_su8);
  OK(Read<I>, I{MakeAt("\x58"_su8, O::I64LeU)}, "\x58"_su8);
  OK(Read<I>, I{MakeAt("\x59"_su8, O::I64GeS)}, "\x59"_su8);
  OK(Read<I>, I{MakeAt("\x5a"_su8, O::I64GeU)}, "\x5a"_su8);
  OK(Read<I>, I{MakeAt("\x5b"_su8, O::F32Eq)}, "\x5b"_su8);
  OK(Read<I>, I{MakeAt("\x5c"_su8, O::F32Ne)}, "\x5c"_su8);
  OK(Read<I>, I{MakeAt("\x5d"_su8, O::F32Lt)}, "\x5d"_su8);
  OK(Read<I>, I{MakeAt("\x5e"_su8, O::F32Gt)}, "\x5e"_su8);
  OK(Read<I>, I{MakeAt("\x5f"_su8, O::F32Le)}, "\x5f"_su8);
  OK(Read<I>, I{MakeAt("\x60"_su8, O::F32Ge)}, "\x60"_su8);
  OK(Read<I>, I{MakeAt("\x61"_su8, O::F64Eq)}, "\x61"_su8);
  OK(Read<I>, I{MakeAt("\x62"_su8, O::F64Ne)}, "\x62"_su8);
  OK(Read<I>, I{MakeAt("\x63"_su8, O::F64Lt)}, "\x63"_su8);
  OK(Read<I>, I{MakeAt("\x64"_su8, O::F64Gt)}, "\x64"_su8);
  OK(Read<I>, I{MakeAt("\x65"_su8, O::F64Le)}, "\x65"_su8);
  OK(Read<I>, I{MakeAt("\x66"_su8, O::F64Ge)}, "\x66"_su8);
  OK(Read<I>, I{MakeAt("\x67"_su8, O::I32Clz)}, "\x67"_su8);
  OK(Read<I>, I{MakeAt("\x68"_su8, O::I32Ctz)}, "\x68"_su8);
  OK(Read<I>, I{MakeAt("\x69"_su8, O::I32Popcnt)}, "\x69"_su8);
  OK(Read<I>, I{MakeAt("\x6a"_su8, O::I32Add)}, "\x6a"_su8);
  OK(Read<I>, I{MakeAt("\x6b"_su8, O::I32Sub)}, "\x6b"_su8);
  OK(Read<I>, I{MakeAt("\x6c"_su8, O::I32Mul)}, "\x6c"_su8);
  OK(Read<I>, I{MakeAt("\x6d"_su8, O::I32DivS)}, "\x6d"_su8);
  OK(Read<I>, I{MakeAt("\x6e"_su8, O::I32DivU)}, "\x6e"_su8);
  OK(Read<I>, I{MakeAt("\x6f"_su8, O::I32RemS)}, "\x6f"_su8);
  OK(Read<I>, I{MakeAt("\x70"_su8, O::I32RemU)}, "\x70"_su8);
  OK(Read<I>, I{MakeAt("\x71"_su8, O::I32And)}, "\x71"_su8);
  OK(Read<I>, I{MakeAt("\x72"_su8, O::I32Or)}, "\x72"_su8);
  OK(Read<I>, I{MakeAt("\x73"_su8, O::I32Xor)}, "\x73"_su8);
  OK(Read<I>, I{MakeAt("\x74"_su8, O::I32Shl)}, "\x74"_su8);
  OK(Read<I>, I{MakeAt("\x75"_su8, O::I32ShrS)}, "\x75"_su8);
  OK(Read<I>, I{MakeAt("\x76"_su8, O::I32ShrU)}, "\x76"_su8);
  OK(Read<I>, I{MakeAt("\x77"_su8, O::I32Rotl)}, "\x77"_su8);
  OK(Read<I>, I{MakeAt("\x78"_su8, O::I32Rotr)}, "\x78"_su8);
  OK(Read<I>, I{MakeAt("\x79"_su8, O::I64Clz)}, "\x79"_su8);
  OK(Read<I>, I{MakeAt("\x7a"_su8, O::I64Ctz)}, "\x7a"_su8);
  OK(Read<I>, I{MakeAt("\x7b"_su8, O::I64Popcnt)}, "\x7b"_su8);
  OK(Read<I>, I{MakeAt("\x7c"_su8, O::I64Add)}, "\x7c"_su8);
  OK(Read<I>, I{MakeAt("\x7d"_su8, O::I64Sub)}, "\x7d"_su8);
  OK(Read<I>, I{MakeAt("\x7e"_su8, O::I64Mul)}, "\x7e"_su8);
  OK(Read<I>, I{MakeAt("\x7f"_su8, O::I64DivS)}, "\x7f"_su8);
  OK(Read<I>, I{MakeAt("\x80"_su8, O::I64DivU)}, "\x80"_su8);
  OK(Read<I>, I{MakeAt("\x81"_su8, O::I64RemS)}, "\x81"_su8);
  OK(Read<I>, I{MakeAt("\x82"_su8, O::I64RemU)}, "\x82"_su8);
  OK(Read<I>, I{MakeAt("\x83"_su8, O::I64And)}, "\x83"_su8);
  OK(Read<I>, I{MakeAt("\x84"_su8, O::I64Or)}, "\x84"_su8);
  OK(Read<I>, I{MakeAt("\x85"_su8, O::I64Xor)}, "\x85"_su8);
  OK(Read<I>, I{MakeAt("\x86"_su8, O::I64Shl)}, "\x86"_su8);
  OK(Read<I>, I{MakeAt("\x87"_su8, O::I64ShrS)}, "\x87"_su8);
  OK(Read<I>, I{MakeAt("\x88"_su8, O::I64ShrU)}, "\x88"_su8);
  OK(Read<I>, I{MakeAt("\x89"_su8, O::I64Rotl)}, "\x89"_su8);
  OK(Read<I>, I{MakeAt("\x8a"_su8, O::I64Rotr)}, "\x8a"_su8);
  OK(Read<I>, I{MakeAt("\x8b"_su8, O::F32Abs)}, "\x8b"_su8);
  OK(Read<I>, I{MakeAt("\x8c"_su8, O::F32Neg)}, "\x8c"_su8);
  OK(Read<I>, I{MakeAt("\x8d"_su8, O::F32Ceil)}, "\x8d"_su8);
  OK(Read<I>, I{MakeAt("\x8e"_su8, O::F32Floor)}, "\x8e"_su8);
  OK(Read<I>, I{MakeAt("\x8f"_su8, O::F32Trunc)}, "\x8f"_su8);
  OK(Read<I>, I{MakeAt("\x90"_su8, O::F32Nearest)}, "\x90"_su8);
  OK(Read<I>, I{MakeAt("\x91"_su8, O::F32Sqrt)}, "\x91"_su8);
  OK(Read<I>, I{MakeAt("\x92"_su8, O::F32Add)}, "\x92"_su8);
  OK(Read<I>, I{MakeAt("\x93"_su8, O::F32Sub)}, "\x93"_su8);
  OK(Read<I>, I{MakeAt("\x94"_su8, O::F32Mul)}, "\x94"_su8);
  OK(Read<I>, I{MakeAt("\x95"_su8, O::F32Div)}, "\x95"_su8);
  OK(Read<I>, I{MakeAt("\x96"_su8, O::F32Min)}, "\x96"_su8);
  OK(Read<I>, I{MakeAt("\x97"_su8, O::F32Max)}, "\x97"_su8);
  OK(Read<I>, I{MakeAt("\x98"_su8, O::F32Copysign)}, "\x98"_su8);
  OK(Read<I>, I{MakeAt("\x99"_su8, O::F64Abs)}, "\x99"_su8);
  OK(Read<I>, I{MakeAt("\x9a"_su8, O::F64Neg)}, "\x9a"_su8);
  OK(Read<I>, I{MakeAt("\x9b"_su8, O::F64Ceil)}, "\x9b"_su8);
  OK(Read<I>, I{MakeAt("\x9c"_su8, O::F64Floor)}, "\x9c"_su8);
  OK(Read<I>, I{MakeAt("\x9d"_su8, O::F64Trunc)}, "\x9d"_su8);
  OK(Read<I>, I{MakeAt("\x9e"_su8, O::F64Nearest)}, "\x9e"_su8);
  OK(Read<I>, I{MakeAt("\x9f"_su8, O::F64Sqrt)}, "\x9f"_su8);
  OK(Read<I>, I{MakeAt("\xa0"_su8, O::F64Add)}, "\xa0"_su8);
  OK(Read<I>, I{MakeAt("\xa1"_su8, O::F64Sub)}, "\xa1"_su8);
  OK(Read<I>, I{MakeAt("\xa2"_su8, O::F64Mul)}, "\xa2"_su8);
  OK(Read<I>, I{MakeAt("\xa3"_su8, O::F64Div)}, "\xa3"_su8);
  OK(Read<I>, I{MakeAt("\xa4"_su8, O::F64Min)}, "\xa4"_su8);
  OK(Read<I>, I{MakeAt("\xa5"_su8, O::F64Max)}, "\xa5"_su8);
  OK(Read<I>, I{MakeAt("\xa6"_su8, O::F64Copysign)}, "\xa6"_su8);
  OK(Read<I>, I{MakeAt("\xa7"_su8, O::I32WrapI64)}, "\xa7"_su8);
  OK(Read<I>, I{MakeAt("\xa8"_su8, O::I32TruncF32S)}, "\xa8"_su8);
  OK(Read<I>, I{MakeAt("\xa9"_su8, O::I32TruncF32U)}, "\xa9"_su8);
  OK(Read<I>, I{MakeAt("\xaa"_su8, O::I32TruncF64S)}, "\xaa"_su8);
  OK(Read<I>, I{MakeAt("\xab"_su8, O::I32TruncF64U)}, "\xab"_su8);
  OK(Read<I>, I{MakeAt("\xac"_su8, O::I64ExtendI32S)}, "\xac"_su8);
  OK(Read<I>, I{MakeAt("\xad"_su8, O::I64ExtendI32U)}, "\xad"_su8);
  OK(Read<I>, I{MakeAt("\xae"_su8, O::I64TruncF32S)}, "\xae"_su8);
  OK(Read<I>, I{MakeAt("\xaf"_su8, O::I64TruncF32U)}, "\xaf"_su8);
  OK(Read<I>, I{MakeAt("\xb0"_su8, O::I64TruncF64S)}, "\xb0"_su8);
  OK(Read<I>, I{MakeAt("\xb1"_su8, O::I64TruncF64U)}, "\xb1"_su8);
  OK(Read<I>, I{MakeAt("\xb2"_su8, O::F32ConvertI32S)}, "\xb2"_su8);
  OK(Read<I>, I{MakeAt("\xb3"_su8, O::F32ConvertI32U)}, "\xb3"_su8);
  OK(Read<I>, I{MakeAt("\xb4"_su8, O::F32ConvertI64S)}, "\xb4"_su8);
  OK(Read<I>, I{MakeAt("\xb5"_su8, O::F32ConvertI64U)}, "\xb5"_su8);
  OK(Read<I>, I{MakeAt("\xb6"_su8, O::F32DemoteF64)}, "\xb6"_su8);
  OK(Read<I>, I{MakeAt("\xb7"_su8, O::F64ConvertI32S)}, "\xb7"_su8);
  OK(Read<I>, I{MakeAt("\xb8"_su8, O::F64ConvertI32U)}, "\xb8"_su8);
  OK(Read<I>, I{MakeAt("\xb9"_su8, O::F64ConvertI64S)}, "\xb9"_su8);
  OK(Read<I>, I{MakeAt("\xba"_su8, O::F64ConvertI64U)}, "\xba"_su8);
  OK(Read<I>, I{MakeAt("\xbb"_su8, O::F64PromoteF32)}, "\xbb"_su8);
  OK(Read<I>, I{MakeAt("\xbc"_su8, O::I32ReinterpretF32)}, "\xbc"_su8);
  OK(Read<I>, I{MakeAt("\xbd"_su8, O::I64ReinterpretF64)}, "\xbd"_su8);
  OK(Read<I>, I{MakeAt("\xbe"_su8, O::F32ReinterpretI32)}, "\xbe"_su8);
  OK(Read<I>, I{MakeAt("\xbf"_su8, O::F64ReinterpretI64)}, "\xbf"_su8);
}

TEST_F(BinaryReadTest, Instruction_BadMemoryReserved) {
  Fail(Read<Instruction>,
       {{1, "reserved"}, {1, "Expected reserved byte 0, got 1"}},
       "\x3f\x01"_su8);
  Fail(Read<Instruction>,
       {{1, "reserved"}, {1, "Expected reserved byte 0, got 1"}},
       "\x40\x01"_su8);
}

TEST_F(BinaryReadTest, InstructionList_BlockEnd) {
  OK(Read<InstructionList>,
     InstructionList{
         MakeAt("\x02\x40"_su8, I{MakeAt("\x02"_su8, O::Block),
                                  MakeAt("\x40"_su8, BlockType::Void)}),
         MakeAt("\x0b"_su8, I{MakeAt("\x0b"_su8, O::End)}),
     },
     "\x02\x40\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_BlockNoEnd) {
  Fail(Read<InstructionList>, {{3, "opcode"}, {3, "Unable to read u8"}},
       "\x02\x40\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_LoopEnd) {
  OK(Read<InstructionList>,
     InstructionList{
         MakeAt("\x03\x40"_su8, I{MakeAt("\x03"_su8, O::Loop),
                                  MakeAt("\x40"_su8, BlockType::Void)}),
         MakeAt("\x0b"_su8, I{MakeAt("\x0b"_su8, O::End)}),
     },
     "\x03\x40\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_LoopNoEnd) {
  Fail(Read<InstructionList>, {{3, "opcode"}, {3, "Unable to read u8"}},
       "\x03\x40\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_IfEnd) {
  OK(Read<InstructionList>,
     InstructionList{
         MakeAt("\x04\x40"_su8, I{MakeAt("\x04"_su8, O::If),
                                  MakeAt("\x40"_su8, BlockType::Void)}),
         MakeAt("\x0b"_su8, I{MakeAt("\x0b"_su8, O::End)}),
     },
     "\x04\x40\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_IfElseEnd) {
  OK(Read<InstructionList>,
     InstructionList{
         MakeAt("\x04\x40"_su8, I{MakeAt("\x04"_su8, O::If),
                                  MakeAt("\x40"_su8, BlockType::Void)}),
         MakeAt("\x05"_su8, I{MakeAt("\x05"_su8, O::Else)}),
         MakeAt("\x0b"_su8, I{MakeAt("\x0b"_su8, O::End)}),
     },
     "\x04\x40\x05\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_IfNoEnd) {
  Fail(Read<InstructionList>, {{3, "opcode"}, {3, "Unable to read u8"}},
       "\x04\x40\x0b"_su8);
}

TEST_F(BinaryReadTest, Instruction_exceptions) {
  context.features.enable_exceptions();

  OK(Read<I>,
     I{MakeAt("\x06"_su8, O::Try), MakeAt("\x40"_su8, BlockType::Void)},
     "\x06\x40"_su8);
  OK(Read<I>, I{MakeAt("\x07"_su8, O::Catch)}, "\x07"_su8);
  OK(Read<I>, I{MakeAt("\x08"_su8, O::Throw), MakeAt("\x00"_su8, Index{0})},
     "\x08\x00"_su8);
  OK(Read<I>, I{MakeAt("\x09"_su8, O::Rethrow)}, "\x09"_su8);
  OK(Read<I>,
     I{MakeAt("\x0a"_su8, O::BrOnExn),
       MakeAt("\x01\x02"_su8, BrOnExnImmediate{MakeAt("\x01"_su8, Index{1}),
                                               MakeAt("\x02"_su8, Index{2})})},
     "\x0a\x01\x02"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryCatchEnd) {
  context.features.enable_exceptions();

  OK(Read<InstructionList>,
     InstructionList{
         MakeAt("\x06\x40"_su8, I{MakeAt("\x06"_su8, O::Try),
                                  MakeAt("\x40"_su8, BlockType::Void)}),
         MakeAt("\x07"_su8, I{MakeAt("\x07"_su8, O::Catch)}),
         MakeAt("\x0b"_su8, I{MakeAt("\x0b"_su8, O::End)}),
     },
     "\x06\x40\x07\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryNoCatch) {
  context.features.enable_exceptions();

  Fail(Read<InstructionList>, {{2, "Expected catch instruction in try block"}},
       "\x06\x40\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryNoEnd) {
  context.features.enable_exceptions();

  Fail(Read<InstructionList>, {{4, "opcode"}, {4, "Unable to read u8"}},
       "\x06\x40\07\x0b"_su8);
}

TEST_F(BinaryReadTest, Instruction_tail_call) {
  context.features.enable_tail_call();

  OK(Read<I>,
     I{MakeAt("\x12"_su8, O::ReturnCall), MakeAt("\x00"_su8, Index{0})},
     "\x12\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\x13"_su8, O::ReturnCallIndirect),
       MakeAt("\x08\x00"_su8,
              CallIndirectImmediate{MakeAt("\x08"_su8, Index{8}),
                                    MakeAt("\x00"_su8, Index{0})})},
     "\x13\x08\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_sign_extension) {
  context.features.enable_sign_extension();

  OK(Read<I>, I{MakeAt("\xc0"_su8, O::I32Extend8S)}, "\xc0"_su8);
  OK(Read<I>, I{MakeAt("\xc1"_su8, O::I32Extend16S)}, "\xc1"_su8);
  OK(Read<I>, I{MakeAt("\xc2"_su8, O::I64Extend8S)}, "\xc2"_su8);
  OK(Read<I>, I{MakeAt("\xc3"_su8, O::I64Extend16S)}, "\xc3"_su8);
  OK(Read<I>, I{MakeAt("\xc4"_su8, O::I64Extend32S)}, "\xc4"_su8);
}

TEST_F(BinaryReadTest, Instruction_reference_types) {
  context.features.enable_reference_types();

  OK(Read<I>,
     I{MakeAt("\x1c"_su8, O::SelectT),
       MakeAt("\x02\x7f\x7e"_su8,
              ValueTypeList{MakeAt("\x7f"_su8, ValueType::I32),
                            MakeAt("\x7e"_su8, ValueType::I64)})},
     "\x1c\x02\x7f\x7e"_su8);
  OK(Read<I>, I{MakeAt("\x25"_su8, O::TableGet), MakeAt("\x00"_su8, Index{0})},
     "\x25\x00"_su8);
  OK(Read<I>, I{MakeAt("\x26"_su8, O::TableSet), MakeAt("\x00"_su8, Index{0})},
     "\x26\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0c"_su8, O::TableInit),
       MakeAt("\x00\x01"_su8, InitImmediate{MakeAt("\x00"_su8, Index{0}),
                                            MakeAt("\x01"_su8, Index{1})})},
     "\xfc\x0c\x00\x01"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0e"_su8, O::TableCopy),
       MakeAt("\x00\x01"_su8, CopyImmediate{MakeAt("\x00"_su8, Index{0}),
                                            MakeAt("\x01"_su8, Index{1})})},
     "\xfc\x0e\x00\x01"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0f"_su8, O::TableGrow), MakeAt("\x00"_su8, Index{0})},
     "\xfc\x0f\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x10"_su8, O::TableSize), MakeAt("\x00"_su8, Index{0})},
     "\xfc\x10\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x11"_su8, O::TableFill), MakeAt("\x00"_su8, Index{0})},
     "\xfc\x11\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xd0"_su8, O::RefNull),
       MakeAt("\x70"_su8, ReferenceType::Funcref)},
     "\xd0\x70"_su8);
  OK(Read<I>, I{MakeAt("\xd1"_su8, O::RefIsNull)}, "\xd1"_su8);
  OK(Read<I>, I{MakeAt("\xd2"_su8, O::RefFunc), MakeAt("\x00"_su8, Index{0})},
     "\xd2\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_saturating_float_to_int) {
  context.features.enable_saturating_float_to_int();

  OK(Read<I>, I{MakeAt("\xfc\x00"_su8, O::I32TruncSatF32S)}, "\xfc\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x01"_su8, O::I32TruncSatF32U)}, "\xfc\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x02"_su8, O::I32TruncSatF64S)}, "\xfc\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x03"_su8, O::I32TruncSatF64U)}, "\xfc\x03"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x04"_su8, O::I64TruncSatF32S)}, "\xfc\x04"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x05"_su8, O::I64TruncSatF32U)}, "\xfc\x05"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x06"_su8, O::I64TruncSatF64S)}, "\xfc\x06"_su8);
  OK(Read<I>, I{MakeAt("\xfc\x07"_su8, O::I64TruncSatF64U)}, "\xfc\x07"_su8);
}

TEST_F(BinaryReadTest, Instruction_bulk_memory) {
  context.features.enable_bulk_memory();
  context.declared_data_count = 1;

  OK(Read<I>,
     I{MakeAt("\xfc\x08"_su8, O::MemoryInit),
       MakeAt("\x01\x00"_su8, InitImmediate{MakeAt("\x01"_su8, Index{1}),
                                            MakeAt("\x00"_su8, Index{0})})},
     "\xfc\x08\x01\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x09"_su8, O::DataDrop), MakeAt("\x02"_su8, Index{2})},
     "\xfc\x09\x02"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0a"_su8, O::MemoryCopy),
       MakeAt("\x00\x00"_su8, CopyImmediate{MakeAt("\x00"_su8, Index{0}),
                                            MakeAt("\x00"_su8, Index{0})})},
     "\xfc\x0a\x00\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0b"_su8, O::MemoryFill), MakeAt("\x00"_su8, u8{0})},
     "\xfc\x0b\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0c"_su8, O::TableInit),
       MakeAt("\x03\x00"_su8, InitImmediate{MakeAt("\x03"_su8, Index{3}),
                                            MakeAt("\x00"_su8, Index{0})})},
     "\xfc\x0c\x03\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0d"_su8, O::ElemDrop), MakeAt("\x04"_su8, Index{4})},
     "\xfc\x0d\x04"_su8);
  OK(Read<I>,
     I{MakeAt("\xfc\x0e"_su8, O::TableCopy),
       MakeAt("\x00\x00"_su8, CopyImmediate{MakeAt("\x00"_su8, Index{0}),
                                            MakeAt("\x00"_su8, Index{0})})},
     "\xfc\x0e\x00\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_BadReserved_bulk_memory) {
  context.features.enable_bulk_memory();

  Fail(Read<I>,
       {{2, "init immediate"},
        {3, "reserved"},
        {3, "Expected reserved byte 0, got 1"}},
       "\xfc\x0c\x00\x01"_su8);
  Fail(Read<I>,
       {{2, "copy immediate"},
        {3, "reserved"},
        {3, "Expected reserved byte 0, got 1"}},
       "\xfc\x0e\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Instruction_NoDataCount_bulk_memory) {
  context.features.enable_bulk_memory();

  Fail(Read<I>, {{0, "memory.init instruction requires a data count section"}},
       "\xfc\x08\x01\x00"_su8);
  Fail(Read<I>, {{0, "data.drop instruction requires a data count section"}},
       "\xfc\x09\x02"_su8);
}

TEST_F(BinaryReadTest, Instruction_simd) {
  context.features.enable_simd();

  auto memarg = MakeAt(
      "\x01\x02"_su8,
      MemArgImmediate{MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, u32{2})});

  OK(Read<I>, I{MakeAt("\xfd\x00"_su8, O::V128Load), memarg}, "\xfd\x00\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x01"_su8, O::I16X8Load8X8S), memarg}, "\xfd\x01\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x02"_su8, O::I16X8Load8X8U), memarg}, "\xfd\x02\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x03"_su8, O::I32X4Load16X4S), memarg}, "\xfd\x03\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x04"_su8, O::I32X4Load16X4U), memarg}, "\xfd\x04\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x05"_su8, O::I64X2Load32X2S), memarg}, "\xfd\x05\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x06"_su8, O::I64X2Load32X2U), memarg}, "\xfd\x06\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x07"_su8, O::V8X16LoadSplat), memarg}, "\xfd\x07\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x08"_su8, O::V16X8LoadSplat), memarg}, "\xfd\x08\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x09"_su8, O::V32X4LoadSplat), memarg}, "\xfd\x09\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x0a"_su8, O::V64X2LoadSplat), memarg}, "\xfd\x0a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x0b"_su8, O::V128Store), memarg}, "\xfd\x0b\x01\x02"_su8);
  OK(Read<I>,
     I{MakeAt("\xfd\x0c"_su8, O::V128Const),
       MakeAt(
           "\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00"_su8,
           v128{u64{5}, u64{6}})},
     "\xfd\x0c\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<I>,
     I{MakeAt("\xfd\x0d"_su8, O::V8X16Shuffle),
       MakeAt(
           "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
           ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}})},
     "\xfd\x0d\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x0e"_su8, O::V8X16Swizzle)}, "\xfd\x0e"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x0f"_su8, O::I8X16Splat)}, "\xfd\x0f"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x10"_su8, O::I16X8Splat)}, "\xfd\x10"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x11"_su8, O::I32X4Splat)}, "\xfd\x11"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x12"_su8, O::I64X2Splat)}, "\xfd\x12"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x13"_su8, O::F32X4Splat)}, "\xfd\x13"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x14"_su8, O::F64X2Splat)}, "\xfd\x14"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x15"_su8, O::I8X16ExtractLaneS), MakeAt("\x00"_su8, u8{0})}, "\xfd\x15\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x16"_su8, O::I8X16ExtractLaneU), MakeAt("\x00"_su8, u8{0})}, "\xfd\x16\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x17"_su8, O::I8X16ReplaceLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x17\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x18"_su8, O::I16X8ExtractLaneS), MakeAt("\x00"_su8, u8{0})}, "\xfd\x18\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x19"_su8, O::I16X8ExtractLaneU), MakeAt("\x00"_su8, u8{0})}, "\xfd\x19\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x1a"_su8, O::I16X8ReplaceLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x1a\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x1b"_su8, O::I32X4ExtractLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x1b\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x1c"_su8, O::I32X4ReplaceLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x1c\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x1d"_su8, O::I64X2ExtractLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x1d\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x1e"_su8, O::I64X2ReplaceLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x1e\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x1f"_su8, O::F32X4ExtractLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x1f\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x20"_su8, O::F32X4ReplaceLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x20\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x21"_su8, O::F64X2ExtractLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x21\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x22"_su8, O::F64X2ReplaceLane), MakeAt("\x00"_su8, u8{0})}, "\xfd\x22\x00"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x23"_su8, O::I8X16Eq)}, "\xfd\x23"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x24"_su8, O::I8X16Ne)}, "\xfd\x24"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x25"_su8, O::I8X16LtS)}, "\xfd\x25"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x26"_su8, O::I8X16LtU)}, "\xfd\x26"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x27"_su8, O::I8X16GtS)}, "\xfd\x27"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x28"_su8, O::I8X16GtU)}, "\xfd\x28"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x29"_su8, O::I8X16LeS)}, "\xfd\x29"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x2a"_su8, O::I8X16LeU)}, "\xfd\x2a"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x2b"_su8, O::I8X16GeS)}, "\xfd\x2b"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x2c"_su8, O::I8X16GeU)}, "\xfd\x2c"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x2d"_su8, O::I16X8Eq)}, "\xfd\x2d"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x2e"_su8, O::I16X8Ne)}, "\xfd\x2e"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x2f"_su8, O::I16X8LtS)}, "\xfd\x2f"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x30"_su8, O::I16X8LtU)}, "\xfd\x30"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x31"_su8, O::I16X8GtS)}, "\xfd\x31"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x32"_su8, O::I16X8GtU)}, "\xfd\x32"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x33"_su8, O::I16X8LeS)}, "\xfd\x33"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x34"_su8, O::I16X8LeU)}, "\xfd\x34"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x35"_su8, O::I16X8GeS)}, "\xfd\x35"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x36"_su8, O::I16X8GeU)}, "\xfd\x36"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x37"_su8, O::I32X4Eq)}, "\xfd\x37"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x38"_su8, O::I32X4Ne)}, "\xfd\x38"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x39"_su8, O::I32X4LtS)}, "\xfd\x39"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x3a"_su8, O::I32X4LtU)}, "\xfd\x3a"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x3b"_su8, O::I32X4GtS)}, "\xfd\x3b"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x3c"_su8, O::I32X4GtU)}, "\xfd\x3c"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x3d"_su8, O::I32X4LeS)}, "\xfd\x3d"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x3e"_su8, O::I32X4LeU)}, "\xfd\x3e"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x3f"_su8, O::I32X4GeS)}, "\xfd\x3f"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x40"_su8, O::I32X4GeU)}, "\xfd\x40"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x41"_su8, O::F32X4Eq)}, "\xfd\x41"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x42"_su8, O::F32X4Ne)}, "\xfd\x42"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x43"_su8, O::F32X4Lt)}, "\xfd\x43"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x44"_su8, O::F32X4Gt)}, "\xfd\x44"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x45"_su8, O::F32X4Le)}, "\xfd\x45"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x46"_su8, O::F32X4Ge)}, "\xfd\x46"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x47"_su8, O::F64X2Eq)}, "\xfd\x47"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x48"_su8, O::F64X2Ne)}, "\xfd\x48"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x49"_su8, O::F64X2Lt)}, "\xfd\x49"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x4a"_su8, O::F64X2Gt)}, "\xfd\x4a"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x4b"_su8, O::F64X2Le)}, "\xfd\x4b"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x4c"_su8, O::F64X2Ge)}, "\xfd\x4c"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x4d"_su8, O::V128Not)}, "\xfd\x4d"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x4e"_su8, O::V128And)}, "\xfd\x4e"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x4f"_su8, O::V128Andnot)}, "\xfd\x4f"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x50"_su8, O::V128Or)}, "\xfd\x50"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x51"_su8, O::V128Xor)}, "\xfd\x51"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x52"_su8, O::V128BitSelect)}, "\xfd\x52"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x60"_su8, O::I8X16Abs)}, "\xfd\x60"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x61"_su8, O::I8X16Neg)}, "\xfd\x61"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x62"_su8, O::I8X16AnyTrue)}, "\xfd\x62"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x63"_su8, O::I8X16AllTrue)}, "\xfd\x63"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x65"_su8, O::I8X16NarrowI16X8S)}, "\xfd\x65"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x66"_su8, O::I8X16NarrowI16X8U)}, "\xfd\x66"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x6b"_su8, O::I8X16Shl)}, "\xfd\x6b"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x6c"_su8, O::I8X16ShrS)}, "\xfd\x6c"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x6d"_su8, O::I8X16ShrU)}, "\xfd\x6d"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x6e"_su8, O::I8X16Add)}, "\xfd\x6e"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x6f"_su8, O::I8X16AddSaturateS)}, "\xfd\x6f"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x70"_su8, O::I8X16AddSaturateU)}, "\xfd\x70"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x71"_su8, O::I8X16Sub)}, "\xfd\x71"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x72"_su8, O::I8X16SubSaturateS)}, "\xfd\x72"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x73"_su8, O::I8X16SubSaturateU)}, "\xfd\x73"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x76"_su8, O::I8X16MinS)}, "\xfd\x76"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x77"_su8, O::I8X16MinU)}, "\xfd\x77"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x78"_su8, O::I8X16MaxS)}, "\xfd\x78"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x79"_su8, O::I8X16MaxU)}, "\xfd\x79"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x7b"_su8, O::I8X16AvgrU)}, "\xfd\x7b"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x80\x01"_su8, O::I16X8Abs)}, "\xfd\x80\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x81\x01"_su8, O::I16X8Neg)}, "\xfd\x81\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x82\x01"_su8, O::I16X8AnyTrue)}, "\xfd\x82\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x83\x01"_su8, O::I16X8AllTrue)}, "\xfd\x83\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x85\x01"_su8, O::I16X8NarrowI32X4S)}, "\xfd\x85\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x86\x01"_su8, O::I16X8NarrowI32X4U)}, "\xfd\x86\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x87\x01"_su8, O::I16X8WidenLowI8X16S)}, "\xfd\x87\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x88\x01"_su8, O::I16X8WidenHighI8X16S)}, "\xfd\x88\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x89\x01"_su8, O::I16X8WidenLowI8X16U)}, "\xfd\x89\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x8a\x01"_su8, O::I16X8WidenHighI8X16U)}, "\xfd\x8a\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x8b\x01"_su8, O::I16X8Shl)}, "\xfd\x8b\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x8c\x01"_su8, O::I16X8ShrS)}, "\xfd\x8c\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x8d\x01"_su8, O::I16X8ShrU)}, "\xfd\x8d\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x8e\x01"_su8, O::I16X8Add)}, "\xfd\x8e\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x8f\x01"_su8, O::I16X8AddSaturateS)}, "\xfd\x8f\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x90\x01"_su8, O::I16X8AddSaturateU)}, "\xfd\x90\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x91\x01"_su8, O::I16X8Sub)}, "\xfd\x91\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x92\x01"_su8, O::I16X8SubSaturateS)}, "\xfd\x92\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x93\x01"_su8, O::I16X8SubSaturateU)}, "\xfd\x93\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x95\x01"_su8, O::I16X8Mul)}, "\xfd\x95\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x96\x01"_su8, O::I16X8MinS)}, "\xfd\x96\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x97\x01"_su8, O::I16X8MinU)}, "\xfd\x97\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x98\x01"_su8, O::I16X8MaxS)}, "\xfd\x98\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x99\x01"_su8, O::I16X8MaxU)}, "\xfd\x99\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\x9b\x01"_su8, O::I16X8AvgrU)}, "\xfd\x9b\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa0\x01"_su8, O::I32X4Abs)}, "\xfd\xa0\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa1\x01"_su8, O::I32X4Neg)}, "\xfd\xa1\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa2\x01"_su8, O::I32X4AnyTrue)}, "\xfd\xa2\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa3\x01"_su8, O::I32X4AllTrue)}, "\xfd\xa3\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa7\x01"_su8, O::I32X4WidenLowI16X8S)}, "\xfd\xa7\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa8\x01"_su8, O::I32X4WidenHighI16X8S)}, "\xfd\xa8\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xa9\x01"_su8, O::I32X4WidenLowI16X8U)}, "\xfd\xa9\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xaa\x01"_su8, O::I32X4WidenHighI16X8U)}, "\xfd\xaa\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xab\x01"_su8, O::I32X4Shl)}, "\xfd\xab\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xac\x01"_su8, O::I32X4ShrS)}, "\xfd\xac\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xad\x01"_su8, O::I32X4ShrU)}, "\xfd\xad\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xae\x01"_su8, O::I32X4Add)}, "\xfd\xae\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xb1\x01"_su8, O::I32X4Sub)}, "\xfd\xb1\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xb5\x01"_su8, O::I32X4Mul)}, "\xfd\xb5\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xb6\x01"_su8, O::I32X4MinS)}, "\xfd\xb6\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xb7\x01"_su8, O::I32X4MinU)}, "\xfd\xb7\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xb8\x01"_su8, O::I32X4MaxS)}, "\xfd\xb8\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xb9\x01"_su8, O::I32X4MaxU)}, "\xfd\xb9\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xc1\x01"_su8, O::I64X2Neg)}, "\xfd\xc1\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xcb\x01"_su8, O::I64X2Shl)}, "\xfd\xcb\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xcc\x01"_su8, O::I64X2ShrS)}, "\xfd\xcc\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xcd\x01"_su8, O::I64X2ShrU)}, "\xfd\xcd\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xce\x01"_su8, O::I64X2Add)}, "\xfd\xce\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xd1\x01"_su8, O::I64X2Sub)}, "\xfd\xd1\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xd5\x01"_su8, O::I64X2Mul)}, "\xfd\xd5\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe0\x01"_su8, O::F32X4Abs)}, "\xfd\xe0\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe1\x01"_su8, O::F32X4Neg)}, "\xfd\xe1\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe3\x01"_su8, O::F32X4Sqrt)}, "\xfd\xe3\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe4\x01"_su8, O::F32X4Add)}, "\xfd\xe4\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe5\x01"_su8, O::F32X4Sub)}, "\xfd\xe5\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe6\x01"_su8, O::F32X4Mul)}, "\xfd\xe6\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe7\x01"_su8, O::F32X4Div)}, "\xfd\xe7\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe8\x01"_su8, O::F32X4Min)}, "\xfd\xe8\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xe9\x01"_su8, O::F32X4Max)}, "\xfd\xe9\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xec\x01"_su8, O::F64X2Abs)}, "\xfd\xec\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xed\x01"_su8, O::F64X2Neg)}, "\xfd\xed\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xef\x01"_su8, O::F64X2Sqrt)}, "\xfd\xef\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf0\x01"_su8, O::F64X2Add)}, "\xfd\xf0\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf1\x01"_su8, O::F64X2Sub)}, "\xfd\xf1\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf2\x01"_su8, O::F64X2Mul)}, "\xfd\xf2\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf3\x01"_su8, O::F64X2Div)}, "\xfd\xf3\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf4\x01"_su8, O::F64X2Min)}, "\xfd\xf4\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf5\x01"_su8, O::F64X2Max)}, "\xfd\xf5\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf8\x01"_su8, O::I32X4TruncSatF32X4S)}, "\xfd\xf8\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xf9\x01"_su8, O::I32X4TruncSatF32X4U)}, "\xfd\xf9\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xfa\x01"_su8, O::F32X4ConvertI32X4S)}, "\xfd\xfa\x01"_su8);
  OK(Read<I>, I{MakeAt("\xfd\xfb\x01"_su8, O::F32X4ConvertI32X4U)}, "\xfd\xfb\x01"_su8);
}

TEST_F(BinaryReadTest, Instruction_threads) {
  context.features.enable_threads();

  auto memarg = MakeAt(
      "\x01\x02"_su8,
      MemArgImmediate{MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, u32{2})});

  OK(Read<I>, I{MakeAt("\xfe\x00"_su8, O::MemoryAtomicNotify), memarg},
     "\xfe\x00\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x01"_su8, O::MemoryAtomicWait32), memarg},
     "\xfe\x01\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x02"_su8, O::MemoryAtomicWait64), memarg},
     "\xfe\x02\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x10"_su8, O::I32AtomicLoad), memarg},
     "\xfe\x10\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x11"_su8, O::I64AtomicLoad), memarg},
     "\xfe\x11\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x12"_su8, O::I32AtomicLoad8U), memarg},
     "\xfe\x12\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x13"_su8, O::I32AtomicLoad16U), memarg},
     "\xfe\x13\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x14"_su8, O::I64AtomicLoad8U), memarg},
     "\xfe\x14\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x15"_su8, O::I64AtomicLoad16U), memarg},
     "\xfe\x15\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x16"_su8, O::I64AtomicLoad32U), memarg},
     "\xfe\x16\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x17"_su8, O::I32AtomicStore), memarg},
     "\xfe\x17\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x18"_su8, O::I64AtomicStore), memarg},
     "\xfe\x18\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x19"_su8, O::I32AtomicStore8), memarg},
     "\xfe\x19\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x1a"_su8, O::I32AtomicStore16), memarg},
     "\xfe\x1a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x1b"_su8, O::I64AtomicStore8), memarg},
     "\xfe\x1b\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x1c"_su8, O::I64AtomicStore16), memarg},
     "\xfe\x1c\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x1d"_su8, O::I64AtomicStore32), memarg},
     "\xfe\x1d\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x1e"_su8, O::I32AtomicRmwAdd), memarg},
     "\xfe\x1e\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x1f"_su8, O::I64AtomicRmwAdd), memarg},
     "\xfe\x1f\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x20"_su8, O::I32AtomicRmw8AddU), memarg},
     "\xfe\x20\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x21"_su8, O::I32AtomicRmw16AddU), memarg},
     "\xfe\x21\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x22"_su8, O::I64AtomicRmw8AddU), memarg},
     "\xfe\x22\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x23"_su8, O::I64AtomicRmw16AddU), memarg},
     "\xfe\x23\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x24"_su8, O::I64AtomicRmw32AddU), memarg},
     "\xfe\x24\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x25"_su8, O::I32AtomicRmwSub), memarg},
     "\xfe\x25\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x26"_su8, O::I64AtomicRmwSub), memarg},
     "\xfe\x26\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x27"_su8, O::I32AtomicRmw8SubU), memarg},
     "\xfe\x27\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x28"_su8, O::I32AtomicRmw16SubU), memarg},
     "\xfe\x28\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x29"_su8, O::I64AtomicRmw8SubU), memarg},
     "\xfe\x29\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x2a"_su8, O::I64AtomicRmw16SubU), memarg},
     "\xfe\x2a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x2b"_su8, O::I64AtomicRmw32SubU), memarg},
     "\xfe\x2b\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x2c"_su8, O::I32AtomicRmwAnd), memarg},
     "\xfe\x2c\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x2d"_su8, O::I64AtomicRmwAnd), memarg},
     "\xfe\x2d\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x2e"_su8, O::I32AtomicRmw8AndU), memarg},
     "\xfe\x2e\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x2f"_su8, O::I32AtomicRmw16AndU), memarg},
     "\xfe\x2f\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x30"_su8, O::I64AtomicRmw8AndU), memarg},
     "\xfe\x30\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x31"_su8, O::I64AtomicRmw16AndU), memarg},
     "\xfe\x31\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x32"_su8, O::I64AtomicRmw32AndU), memarg},
     "\xfe\x32\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x33"_su8, O::I32AtomicRmwOr), memarg},
     "\xfe\x33\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x34"_su8, O::I64AtomicRmwOr), memarg},
     "\xfe\x34\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x35"_su8, O::I32AtomicRmw8OrU), memarg},
     "\xfe\x35\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x36"_su8, O::I32AtomicRmw16OrU), memarg},
     "\xfe\x36\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x37"_su8, O::I64AtomicRmw8OrU), memarg},
     "\xfe\x37\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x38"_su8, O::I64AtomicRmw16OrU), memarg},
     "\xfe\x38\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x39"_su8, O::I64AtomicRmw32OrU), memarg},
     "\xfe\x39\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x3a"_su8, O::I32AtomicRmwXor), memarg},
     "\xfe\x3a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x3b"_su8, O::I64AtomicRmwXor), memarg},
     "\xfe\x3b\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x3c"_su8, O::I32AtomicRmw8XorU), memarg},
     "\xfe\x3c\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x3d"_su8, O::I32AtomicRmw16XorU), memarg},
     "\xfe\x3d\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x3e"_su8, O::I64AtomicRmw8XorU), memarg},
     "\xfe\x3e\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x3f"_su8, O::I64AtomicRmw16XorU), memarg},
     "\xfe\x3f\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x40"_su8, O::I64AtomicRmw32XorU), memarg},
     "\xfe\x40\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x41"_su8, O::I32AtomicRmwXchg), memarg},
     "\xfe\x41\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x42"_su8, O::I64AtomicRmwXchg), memarg},
     "\xfe\x42\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x43"_su8, O::I32AtomicRmw8XchgU), memarg},
     "\xfe\x43\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x44"_su8, O::I32AtomicRmw16XchgU), memarg},
     "\xfe\x44\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x45"_su8, O::I64AtomicRmw8XchgU), memarg},
     "\xfe\x45\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x46"_su8, O::I64AtomicRmw16XchgU), memarg},
     "\xfe\x46\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x47"_su8, O::I64AtomicRmw32XchgU), memarg},
     "\xfe\x47\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x48"_su8, O::I32AtomicRmwCmpxchg), memarg},
     "\xfe\x48\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x49"_su8, O::I64AtomicRmwCmpxchg), memarg},
     "\xfe\x49\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x4a"_su8, O::I32AtomicRmw8CmpxchgU), memarg},
     "\xfe\x4a\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x4b"_su8, O::I32AtomicRmw16CmpxchgU), memarg},
     "\xfe\x4b\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x4c"_su8, O::I64AtomicRmw8CmpxchgU), memarg},
     "\xfe\x4c\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x4d"_su8, O::I64AtomicRmw16CmpxchgU), memarg},
     "\xfe\x4d\x01\x02"_su8);
  OK(Read<I>, I{MakeAt("\xfe\x4e"_su8, O::I64AtomicRmw32CmpxchgU), memarg},
     "\xfe\x4e\x01\x02"_su8);
}

TEST_F(BinaryReadTest, Limits) {
  OK(Read<Limits>,
     Limits{MakeAt("\x81\x01"_su8, u32{129}), nullopt,
            MakeAt("\x00"_su8, Shared::No)},
     "\x00\x81\x01"_su8);
  OK(Read<Limits>,
     Limits{MakeAt("\x02"_su8, u32{2}), MakeAt("\xe8\x07"_su8, u32{1000}),
            MakeAt("\x01"_su8, Shared::No)},
     "\x01\x02\xe8\x07"_su8);
}

TEST_F(BinaryReadTest, Limits_BadFlags) {
  Fail(Read<Limits>, {{0, "limits"}, {1, "Unknown flags value: 2"}},
       "\x02\x01"_su8);
  Fail(Read<Limits>, {{0, "limits"}, {1, "Unknown flags value: 3"}},
       "\x03\x01"_su8);
}

TEST_F(BinaryReadTest, Limits_threads) {
  context.features.enable_threads();

  OK(Read<Limits>,
     Limits{MakeAt("\x02"_su8, u32{2}), MakeAt("\xe8\x07"_su8, u32{1000}),
            MakeAt("\x03"_su8, Shared::Yes)},
     "\x03\x02\xe8\x07"_su8);
}

TEST_F(BinaryReadTest, Limits_PastEnd) {
  Fail(Read<Limits>,
       {{0, "limits"}, {1, "min"}, {1, "u32"}, {1, "Unable to read u8"}},
       "\x00"_su8);
  Fail(Read<Limits>,
       {{0, "limits"}, {2, "max"}, {2, "u32"}, {2, "Unable to read u8"}},
       "\x01\x00"_su8);
}

TEST_F(BinaryReadTest, Locals) {
  OK(Read<Locals>,
     Locals{MakeAt("\x02"_su8, u32{2}), MakeAt("\x7f"_su8, ValueType::I32)},
     "\x02\x7f"_su8);
  OK(Read<Locals>,
     Locals{MakeAt("\xc0\x02"_su8, u32{320}),
            MakeAt("\x7c"_su8, ValueType::F64)},
     "\xc0\x02\x7c"_su8);
}

TEST_F(BinaryReadTest, Locals_PastEnd) {
  Fail(Read<Locals>, {{0, "locals"}, {0, "count"}, {0, "Unable to read u8"}},
       ""_su8);
  Fail(
      Read<Locals>,
      {{0, "locals"}, {2, "type"}, {2, "value type"}, {2, "Unable to read u8"}},
      "\xc0\x02"_su8);
}

TEST_F(BinaryReadTest, MemArgImmediate) {
  OK(Read<MemArgImmediate>,
     MemArgImmediate{MakeAt("\x00"_su8, u32{0}), MakeAt("\x00"_su8, u32{0})},
     "\x00\x00"_su8);
  OK(Read<MemArgImmediate>,
     MemArgImmediate{MakeAt("\x01"_su8, u32{1}),
                     MakeAt("\x80\x02"_su8, u32{256})},
     "\x01\x80\x02"_su8);
}

TEST_F(BinaryReadTest, Memory) {
  OK(Read<Memory>,
     Memory{MakeAt("\x01\x01\x02"_su8,
                   MemoryType{MakeAt("\x01\x01\x02"_su8,
                                     Limits{MakeAt("\x01"_su8, u32{1}),
                                            MakeAt("\x02"_su8, u32{2}),
                                            MakeAt("\x01"_su8, Shared::No)})})},
     "\x01\x01\x02"_su8);
}

TEST_F(BinaryReadTest, Memory_PastEnd) {
  Fail(Read<Memory>,
       {{0, "memory"},
        {0, "memory type"},
        {0, "limits"},
        {0, "flags"},
        {0, "Unable to read u8"}},
       ""_su8);
}

TEST_F(BinaryReadTest, MemoryType) {
  OK(Read<MemoryType>,
     MemoryType{
         MakeAt("\x00\x01"_su8, Limits{MakeAt("\x01"_su8, u32{1}), nullopt,
                                       MakeAt("\x00"_su8, Shared::No)})},
     "\x00\x01"_su8);
  OK(Read<MemoryType>,
     MemoryType{MakeAt(
         "\x01\x00\x80\x01"_su8,
         Limits{MakeAt("\x00"_su8, u32{0}), MakeAt("\x80\x01"_su8, u32{128}),
                MakeAt("\x01"_su8, Shared::No)})},
     "\x01\x00\x80\x01"_su8);
}

TEST_F(BinaryReadTest, MemoryType_PastEnd) {
  Fail(Read<MemoryType>,
       {{0, "memory type"},
        {0, "limits"},
        {0, "flags"},
        {0, "Unable to read u8"}},
       ""_su8);
}

TEST_F(BinaryReadTest, Mutability) {
  OK(Read<Mutability>, Mutability::Const, "\x00"_su8);
  OK(Read<Mutability>, Mutability::Var, "\x01"_su8);
}

TEST_F(BinaryReadTest, Mutability_Unknown) {
  Fail(Read<Mutability>, {{0, "mutability"}, {1, "Unknown mutability: 4"}},
       "\x04"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<Mutability>, {{0, "mutability"}, {1, "Unknown mutability: 132"}},
       "\x84\x00"_su8);
}

TEST_F(BinaryReadTest, NameAssoc) {
  OK(Read<NameAssoc>,
     NameAssoc{MakeAt("\x02"_su8, Index{2}), MakeAt("\x02hi"_su8, "hi"_sv)},
     "\x02\x02hi"_su8);
}

TEST_F(BinaryReadTest, NameAssoc_PastEnd) {
  Fail(Read<NameAssoc>,
       {{0, "name assoc"}, {0, "index"}, {0, "Unable to read u8"}}, ""_su8);

  Fail(
      Read<NameAssoc>,
      {{0, "name assoc"}, {1, "name"}, {1, "length"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST_F(BinaryReadTest, NameSubsectionId) {
  OK(Read<NameSubsectionId>, NameSubsectionId::ModuleName, "\x00"_su8);
  OK(Read<NameSubsectionId>, NameSubsectionId::FunctionNames, "\x01"_su8);
  OK(Read<NameSubsectionId>, NameSubsectionId::LocalNames, "\x02"_su8);
}

TEST_F(BinaryReadTest, NameSubsectionId_Unknown) {
  Fail(Read<NameSubsectionId>,
       {{0, "name subsection id"}, {1, "Unknown name subsection id: 3"}},
       "\x03"_su8);
  Fail(Read<NameSubsectionId>,
       {{0, "name subsection id"}, {1, "Unknown name subsection id: 255"}},
       "\xff"_su8);
}

TEST_F(BinaryReadTest, NameSubsection) {
  OK(Read<NameSubsection>,
     NameSubsection{MakeAt("\x00"_su8, NameSubsectionId::ModuleName), "\0"_su8},
     "\x00\x01\0"_su8);

  OK(Read<NameSubsection>,
     NameSubsection{MakeAt("\x01"_su8, NameSubsectionId::FunctionNames),
                    "\0\0"_su8},
     "\x01\x02\0\0"_su8);

  OK(Read<NameSubsection>,
     NameSubsection{MakeAt("\x02"_su8, NameSubsectionId::LocalNames),
                    "\0\0\0"_su8},
     "\x02\x03\0\0\0"_su8);
}

TEST_F(BinaryReadTest, NameSubsection_BadSubsectionId) {
  Fail(Read<NameSubsection>,
       {{0, "name subsection"},
        {0, "name subsection id"},
        {1, "Unknown name subsection id: 3"}},
       "\x03"_su8);
}

TEST_F(BinaryReadTest, NameSubsection_PastEnd) {
  Fail(Read<NameSubsection>,
       {{0, "name subsection"},
        {0, "name subsection id"},
        {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<NameSubsection>,
       {{0, "name subsection"}, {1, "length"}, {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, Opcode) {
  OK(Read<Opcode>, Opcode::Unreachable, "\x00"_su8);
  OK(Read<Opcode>, Opcode::Nop, "\x01"_su8);
  OK(Read<Opcode>, Opcode::Block, "\x02"_su8);
  OK(Read<Opcode>, Opcode::Loop, "\x03"_su8);
  OK(Read<Opcode>, Opcode::If, "\x04"_su8);
  OK(Read<Opcode>, Opcode::Else, "\x05"_su8);
  OK(Read<Opcode>, Opcode::End, "\x0b"_su8);
  OK(Read<Opcode>, Opcode::Br, "\x0c"_su8);
  OK(Read<Opcode>, Opcode::BrIf, "\x0d"_su8);
  OK(Read<Opcode>, Opcode::BrTable, "\x0e"_su8);
  OK(Read<Opcode>, Opcode::Return, "\x0f"_su8);
  OK(Read<Opcode>, Opcode::Call, "\x10"_su8);
  OK(Read<Opcode>, Opcode::CallIndirect, "\x11"_su8);
  OK(Read<Opcode>, Opcode::Drop, "\x1a"_su8);
  OK(Read<Opcode>, Opcode::Select, "\x1b"_su8);
  OK(Read<Opcode>, Opcode::LocalGet, "\x20"_su8);
  OK(Read<Opcode>, Opcode::LocalSet, "\x21"_su8);
  OK(Read<Opcode>, Opcode::LocalTee, "\x22"_su8);
  OK(Read<Opcode>, Opcode::GlobalGet, "\x23"_su8);
  OK(Read<Opcode>, Opcode::GlobalSet, "\x24"_su8);
  OK(Read<Opcode>, Opcode::I32Load, "\x28"_su8);
  OK(Read<Opcode>, Opcode::I64Load, "\x29"_su8);
  OK(Read<Opcode>, Opcode::F32Load, "\x2a"_su8);
  OK(Read<Opcode>, Opcode::F64Load, "\x2b"_su8);
  OK(Read<Opcode>, Opcode::I32Load8S, "\x2c"_su8);
  OK(Read<Opcode>, Opcode::I32Load8U, "\x2d"_su8);
  OK(Read<Opcode>, Opcode::I32Load16S, "\x2e"_su8);
  OK(Read<Opcode>, Opcode::I32Load16U, "\x2f"_su8);
  OK(Read<Opcode>, Opcode::I64Load8S, "\x30"_su8);
  OK(Read<Opcode>, Opcode::I64Load8U, "\x31"_su8);
  OK(Read<Opcode>, Opcode::I64Load16S, "\x32"_su8);
  OK(Read<Opcode>, Opcode::I64Load16U, "\x33"_su8);
  OK(Read<Opcode>, Opcode::I64Load32S, "\x34"_su8);
  OK(Read<Opcode>, Opcode::I64Load32U, "\x35"_su8);
  OK(Read<Opcode>, Opcode::I32Store, "\x36"_su8);
  OK(Read<Opcode>, Opcode::I64Store, "\x37"_su8);
  OK(Read<Opcode>, Opcode::F32Store, "\x38"_su8);
  OK(Read<Opcode>, Opcode::F64Store, "\x39"_su8);
  OK(Read<Opcode>, Opcode::I32Store8, "\x3a"_su8);
  OK(Read<Opcode>, Opcode::I32Store16, "\x3b"_su8);
  OK(Read<Opcode>, Opcode::I64Store8, "\x3c"_su8);
  OK(Read<Opcode>, Opcode::I64Store16, "\x3d"_su8);
  OK(Read<Opcode>, Opcode::I64Store32, "\x3e"_su8);
  OK(Read<Opcode>, Opcode::MemorySize, "\x3f"_su8);
  OK(Read<Opcode>, Opcode::MemoryGrow, "\x40"_su8);
  OK(Read<Opcode>, Opcode::I32Const, "\x41"_su8);
  OK(Read<Opcode>, Opcode::I64Const, "\x42"_su8);
  OK(Read<Opcode>, Opcode::F32Const, "\x43"_su8);
  OK(Read<Opcode>, Opcode::F64Const, "\x44"_su8);
  OK(Read<Opcode>, Opcode::I32Eqz, "\x45"_su8);
  OK(Read<Opcode>, Opcode::I32Eq, "\x46"_su8);
  OK(Read<Opcode>, Opcode::I32Ne, "\x47"_su8);
  OK(Read<Opcode>, Opcode::I32LtS, "\x48"_su8);
  OK(Read<Opcode>, Opcode::I32LtU, "\x49"_su8);
  OK(Read<Opcode>, Opcode::I32GtS, "\x4a"_su8);
  OK(Read<Opcode>, Opcode::I32GtU, "\x4b"_su8);
  OK(Read<Opcode>, Opcode::I32LeS, "\x4c"_su8);
  OK(Read<Opcode>, Opcode::I32LeU, "\x4d"_su8);
  OK(Read<Opcode>, Opcode::I32GeS, "\x4e"_su8);
  OK(Read<Opcode>, Opcode::I32GeU, "\x4f"_su8);
  OK(Read<Opcode>, Opcode::I64Eqz, "\x50"_su8);
  OK(Read<Opcode>, Opcode::I64Eq, "\x51"_su8);
  OK(Read<Opcode>, Opcode::I64Ne, "\x52"_su8);
  OK(Read<Opcode>, Opcode::I64LtS, "\x53"_su8);
  OK(Read<Opcode>, Opcode::I64LtU, "\x54"_su8);
  OK(Read<Opcode>, Opcode::I64GtS, "\x55"_su8);
  OK(Read<Opcode>, Opcode::I64GtU, "\x56"_su8);
  OK(Read<Opcode>, Opcode::I64LeS, "\x57"_su8);
  OK(Read<Opcode>, Opcode::I64LeU, "\x58"_su8);
  OK(Read<Opcode>, Opcode::I64GeS, "\x59"_su8);
  OK(Read<Opcode>, Opcode::I64GeU, "\x5a"_su8);
  OK(Read<Opcode>, Opcode::F32Eq, "\x5b"_su8);
  OK(Read<Opcode>, Opcode::F32Ne, "\x5c"_su8);
  OK(Read<Opcode>, Opcode::F32Lt, "\x5d"_su8);
  OK(Read<Opcode>, Opcode::F32Gt, "\x5e"_su8);
  OK(Read<Opcode>, Opcode::F32Le, "\x5f"_su8);
  OK(Read<Opcode>, Opcode::F32Ge, "\x60"_su8);
  OK(Read<Opcode>, Opcode::F64Eq, "\x61"_su8);
  OK(Read<Opcode>, Opcode::F64Ne, "\x62"_su8);
  OK(Read<Opcode>, Opcode::F64Lt, "\x63"_su8);
  OK(Read<Opcode>, Opcode::F64Gt, "\x64"_su8);
  OK(Read<Opcode>, Opcode::F64Le, "\x65"_su8);
  OK(Read<Opcode>, Opcode::F64Ge, "\x66"_su8);
  OK(Read<Opcode>, Opcode::I32Clz, "\x67"_su8);
  OK(Read<Opcode>, Opcode::I32Ctz, "\x68"_su8);
  OK(Read<Opcode>, Opcode::I32Popcnt, "\x69"_su8);
  OK(Read<Opcode>, Opcode::I32Add, "\x6a"_su8);
  OK(Read<Opcode>, Opcode::I32Sub, "\x6b"_su8);
  OK(Read<Opcode>, Opcode::I32Mul, "\x6c"_su8);
  OK(Read<Opcode>, Opcode::I32DivS, "\x6d"_su8);
  OK(Read<Opcode>, Opcode::I32DivU, "\x6e"_su8);
  OK(Read<Opcode>, Opcode::I32RemS, "\x6f"_su8);
  OK(Read<Opcode>, Opcode::I32RemU, "\x70"_su8);
  OK(Read<Opcode>, Opcode::I32And, "\x71"_su8);
  OK(Read<Opcode>, Opcode::I32Or, "\x72"_su8);
  OK(Read<Opcode>, Opcode::I32Xor, "\x73"_su8);
  OK(Read<Opcode>, Opcode::I32Shl, "\x74"_su8);
  OK(Read<Opcode>, Opcode::I32ShrS, "\x75"_su8);
  OK(Read<Opcode>, Opcode::I32ShrU, "\x76"_su8);
  OK(Read<Opcode>, Opcode::I32Rotl, "\x77"_su8);
  OK(Read<Opcode>, Opcode::I32Rotr, "\x78"_su8);
  OK(Read<Opcode>, Opcode::I64Clz, "\x79"_su8);
  OK(Read<Opcode>, Opcode::I64Ctz, "\x7a"_su8);
  OK(Read<Opcode>, Opcode::I64Popcnt, "\x7b"_su8);
  OK(Read<Opcode>, Opcode::I64Add, "\x7c"_su8);
  OK(Read<Opcode>, Opcode::I64Sub, "\x7d"_su8);
  OK(Read<Opcode>, Opcode::I64Mul, "\x7e"_su8);
  OK(Read<Opcode>, Opcode::I64DivS, "\x7f"_su8);
  OK(Read<Opcode>, Opcode::I64DivU, "\x80"_su8);
  OK(Read<Opcode>, Opcode::I64RemS, "\x81"_su8);
  OK(Read<Opcode>, Opcode::I64RemU, "\x82"_su8);
  OK(Read<Opcode>, Opcode::I64And, "\x83"_su8);
  OK(Read<Opcode>, Opcode::I64Or, "\x84"_su8);
  OK(Read<Opcode>, Opcode::I64Xor, "\x85"_su8);
  OK(Read<Opcode>, Opcode::I64Shl, "\x86"_su8);
  OK(Read<Opcode>, Opcode::I64ShrS, "\x87"_su8);
  OK(Read<Opcode>, Opcode::I64ShrU, "\x88"_su8);
  OK(Read<Opcode>, Opcode::I64Rotl, "\x89"_su8);
  OK(Read<Opcode>, Opcode::I64Rotr, "\x8a"_su8);
  OK(Read<Opcode>, Opcode::F32Abs, "\x8b"_su8);
  OK(Read<Opcode>, Opcode::F32Neg, "\x8c"_su8);
  OK(Read<Opcode>, Opcode::F32Ceil, "\x8d"_su8);
  OK(Read<Opcode>, Opcode::F32Floor, "\x8e"_su8);
  OK(Read<Opcode>, Opcode::F32Trunc, "\x8f"_su8);
  OK(Read<Opcode>, Opcode::F32Nearest, "\x90"_su8);
  OK(Read<Opcode>, Opcode::F32Sqrt, "\x91"_su8);
  OK(Read<Opcode>, Opcode::F32Add, "\x92"_su8);
  OK(Read<Opcode>, Opcode::F32Sub, "\x93"_su8);
  OK(Read<Opcode>, Opcode::F32Mul, "\x94"_su8);
  OK(Read<Opcode>, Opcode::F32Div, "\x95"_su8);
  OK(Read<Opcode>, Opcode::F32Min, "\x96"_su8);
  OK(Read<Opcode>, Opcode::F32Max, "\x97"_su8);
  OK(Read<Opcode>, Opcode::F32Copysign, "\x98"_su8);
  OK(Read<Opcode>, Opcode::F64Abs, "\x99"_su8);
  OK(Read<Opcode>, Opcode::F64Neg, "\x9a"_su8);
  OK(Read<Opcode>, Opcode::F64Ceil, "\x9b"_su8);
  OK(Read<Opcode>, Opcode::F64Floor, "\x9c"_su8);
  OK(Read<Opcode>, Opcode::F64Trunc, "\x9d"_su8);
  OK(Read<Opcode>, Opcode::F64Nearest, "\x9e"_su8);
  OK(Read<Opcode>, Opcode::F64Sqrt, "\x9f"_su8);
  OK(Read<Opcode>, Opcode::F64Add, "\xa0"_su8);
  OK(Read<Opcode>, Opcode::F64Sub, "\xa1"_su8);
  OK(Read<Opcode>, Opcode::F64Mul, "\xa2"_su8);
  OK(Read<Opcode>, Opcode::F64Div, "\xa3"_su8);
  OK(Read<Opcode>, Opcode::F64Min, "\xa4"_su8);
  OK(Read<Opcode>, Opcode::F64Max, "\xa5"_su8);
  OK(Read<Opcode>, Opcode::F64Copysign, "\xa6"_su8);
  OK(Read<Opcode>, Opcode::I32WrapI64, "\xa7"_su8);
  OK(Read<Opcode>, Opcode::I32TruncF32S, "\xa8"_su8);
  OK(Read<Opcode>, Opcode::I32TruncF32U, "\xa9"_su8);
  OK(Read<Opcode>, Opcode::I32TruncF64S, "\xaa"_su8);
  OK(Read<Opcode>, Opcode::I32TruncF64U, "\xab"_su8);
  OK(Read<Opcode>, Opcode::I64ExtendI32S, "\xac"_su8);
  OK(Read<Opcode>, Opcode::I64ExtendI32U, "\xad"_su8);
  OK(Read<Opcode>, Opcode::I64TruncF32S, "\xae"_su8);
  OK(Read<Opcode>, Opcode::I64TruncF32U, "\xaf"_su8);
  OK(Read<Opcode>, Opcode::I64TruncF64S, "\xb0"_su8);
  OK(Read<Opcode>, Opcode::I64TruncF64U, "\xb1"_su8);
  OK(Read<Opcode>, Opcode::F32ConvertI32S, "\xb2"_su8);
  OK(Read<Opcode>, Opcode::F32ConvertI32U, "\xb3"_su8);
  OK(Read<Opcode>, Opcode::F32ConvertI64S, "\xb4"_su8);
  OK(Read<Opcode>, Opcode::F32ConvertI64U, "\xb5"_su8);
  OK(Read<Opcode>, Opcode::F32DemoteF64, "\xb6"_su8);
  OK(Read<Opcode>, Opcode::F64ConvertI32S, "\xb7"_su8);
  OK(Read<Opcode>, Opcode::F64ConvertI32U, "\xb8"_su8);
  OK(Read<Opcode>, Opcode::F64ConvertI64S, "\xb9"_su8);
  OK(Read<Opcode>, Opcode::F64ConvertI64U, "\xba"_su8);
  OK(Read<Opcode>, Opcode::F64PromoteF32, "\xbb"_su8);
  OK(Read<Opcode>, Opcode::I32ReinterpretF32, "\xbc"_su8);
  OK(Read<Opcode>, Opcode::I64ReinterpretF64, "\xbd"_su8);
  OK(Read<Opcode>, Opcode::F32ReinterpretI32, "\xbe"_su8);
  OK(Read<Opcode>, Opcode::F64ReinterpretI64, "\xbf"_su8);
}

TEST_F(BinaryReadTest, Opcode_Unknown) {
  const u8 kInvalidOpcodes[] = {
      0x06, 0x07, 0x08, 0x09, 0x0a, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
      0x19, 0x1c, 0x1d, 0x1e, 0x1f, 0x25, 0x26, 0x27, 0xc0, 0xc1, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
      0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb,
      0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
      0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3,
      0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
  };
  for (auto code : SpanU8{kInvalidOpcodes, sizeof(kInvalidOpcodes)}) {
    FailUnknownOpcode(code);
  }
}

TEST_F(BinaryReadTest, Opcode_exceptions) {
  context.features.enable_exceptions();

  OK(Read<Opcode>, Opcode::Try, "\x06"_su8);
  OK(Read<Opcode>, Opcode::Catch, "\x07"_su8);
  OK(Read<Opcode>, Opcode::Throw, "\x08"_su8);
  OK(Read<Opcode>, Opcode::Rethrow, "\x09"_su8);
  OK(Read<Opcode>, Opcode::BrOnExn, "\x0a"_su8);
}

TEST_F(BinaryReadTest, Opcode_tail_call) {
  context.features.enable_tail_call();

  OK(Read<Opcode>, Opcode::ReturnCall, "\x12"_su8);
  OK(Read<Opcode>, Opcode::ReturnCallIndirect, "\x13"_su8);
}

TEST_F(BinaryReadTest, Opcode_sign_extension) {
  context.features.enable_sign_extension();

  OK(Read<Opcode>, Opcode::I32Extend8S, "\xc0"_su8);
  OK(Read<Opcode>, Opcode::I32Extend16S, "\xc1"_su8);
  OK(Read<Opcode>, Opcode::I64Extend8S, "\xc2"_su8);
  OK(Read<Opcode>, Opcode::I64Extend16S, "\xc3"_su8);
  OK(Read<Opcode>, Opcode::I64Extend32S, "\xc4"_su8);
}

TEST_F(BinaryReadTest, Opcode_reference_types) {
  context.features.enable_reference_types();

  OK(Read<Opcode>, Opcode::SelectT, "\x1c"_su8);
  OK(Read<Opcode>, Opcode::TableGet, "\x25"_su8);
  OK(Read<Opcode>, Opcode::TableSet, "\x26"_su8);
  OK(Read<Opcode>, Opcode::TableGrow, "\xfc\x0f"_su8);
  OK(Read<Opcode>, Opcode::TableSize, "\xfc\x10"_su8);
  OK(Read<Opcode>, Opcode::TableFill, "\xfc\x11"_su8);
  OK(Read<Opcode>, Opcode::RefNull, "\xd0"_su8);
  OK(Read<Opcode>, Opcode::RefIsNull, "\xd1"_su8);
  OK(Read<Opcode>, Opcode::RefFunc, "\xd2"_su8);
}

TEST_F(BinaryReadTest, Opcode_saturating_float_to_int) {
  context.features.enable_saturating_float_to_int();

  OK(Read<Opcode>, Opcode::I32TruncSatF32S, "\xfc\x00"_su8);
  OK(Read<Opcode>, Opcode::I32TruncSatF32U, "\xfc\x01"_su8);
  OK(Read<Opcode>, Opcode::I32TruncSatF64S, "\xfc\x02"_su8);
  OK(Read<Opcode>, Opcode::I32TruncSatF64U, "\xfc\x03"_su8);
  OK(Read<Opcode>, Opcode::I64TruncSatF32S, "\xfc\x04"_su8);
  OK(Read<Opcode>, Opcode::I64TruncSatF32U, "\xfc\x05"_su8);
  OK(Read<Opcode>, Opcode::I64TruncSatF64S, "\xfc\x06"_su8);
  OK(Read<Opcode>, Opcode::I64TruncSatF64U, "\xfc\x07"_su8);
}

TEST_F(BinaryReadTest, Opcode_bulk_memory) {
  context.features.enable_bulk_memory();

  OK(Read<Opcode>, Opcode::MemoryInit, "\xfc\x08"_su8);
  OK(Read<Opcode>, Opcode::DataDrop, "\xfc\x09"_su8);
  OK(Read<Opcode>, Opcode::MemoryCopy, "\xfc\x0a"_su8);
  OK(Read<Opcode>, Opcode::MemoryFill, "\xfc\x0b"_su8);
  OK(Read<Opcode>, Opcode::TableInit, "\xfc\x0c"_su8);
  OK(Read<Opcode>, Opcode::ElemDrop, "\xfc\x0d"_su8);
  OK(Read<Opcode>, Opcode::TableCopy, "\xfc\x0e"_su8);
}

TEST_F(BinaryReadTest, Opcode_disabled_misc_prefix) {
  {
    context.features = Features{Features::SaturatingFloatToInt};
    FailUnknownOpcode(0xfc, 8);
    FailUnknownOpcode(0xfc, 9);
    FailUnknownOpcode(0xfc, 10);
    FailUnknownOpcode(0xfc, 11);
    FailUnknownOpcode(0xfc, 12);
    FailUnknownOpcode(0xfc, 13);
    FailUnknownOpcode(0xfc, 14);
  }

  {
    context.features = Features{Features::BulkMemory};
    FailUnknownOpcode(0xfc, 0);
    FailUnknownOpcode(0xfc, 1);
    FailUnknownOpcode(0xfc, 2);
    FailUnknownOpcode(0xfc, 3);
    FailUnknownOpcode(0xfc, 4);
    FailUnknownOpcode(0xfc, 5);
    FailUnknownOpcode(0xfc, 6);
    FailUnknownOpcode(0xfc, 7);
  }
}

TEST_F(BinaryReadTest, Opcode_Unknown_misc_prefix) {
  context.features.enable_saturating_float_to_int();
  context.features.enable_bulk_memory();

  for (u8 code = 0x0f; code < 0x7f; ++code) {
    FailUnknownOpcode(0xfc, code);
  }

  // Test some longer codes too.
  FailUnknownOpcode(0xfc, 128);
  FailUnknownOpcode(0xfc, 16384);
  FailUnknownOpcode(0xfc, 2097152);
  FailUnknownOpcode(0xfc, 268435456);
}

TEST_F(BinaryReadTest, Opcode_simd) {
  using O = Opcode;

  context.features.enable_simd();

  OK(Read<O>, MakeAt("\xfd\x00"_su8, O::V128Load), "\xfd\x00"_su8);
  OK(Read<O>, MakeAt("\xfd\x01"_su8, O::I16X8Load8X8S), "\xfd\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x02"_su8, O::I16X8Load8X8U), "\xfd\x02"_su8);
  OK(Read<O>, MakeAt("\xfd\x03"_su8, O::I32X4Load16X4S), "\xfd\x03"_su8);
  OK(Read<O>, MakeAt("\xfd\x04"_su8, O::I32X4Load16X4U), "\xfd\x04"_su8);
  OK(Read<O>, MakeAt("\xfd\x05"_su8, O::I64X2Load32X2S), "\xfd\x05"_su8);
  OK(Read<O>, MakeAt("\xfd\x06"_su8, O::I64X2Load32X2U), "\xfd\x06"_su8);
  OK(Read<O>, MakeAt("\xfd\x07"_su8, O::V8X16LoadSplat), "\xfd\x07"_su8);
  OK(Read<O>, MakeAt("\xfd\x08"_su8, O::V16X8LoadSplat), "\xfd\x08"_su8);
  OK(Read<O>, MakeAt("\xfd\x09"_su8, O::V32X4LoadSplat), "\xfd\x09"_su8);
  OK(Read<O>, MakeAt("\xfd\x0a"_su8, O::V64X2LoadSplat), "\xfd\x0a"_su8);
  OK(Read<O>, MakeAt("\xfd\x0b"_su8, O::V128Store), "\xfd\x0b"_su8);
  OK(Read<O>, MakeAt("\xfd\x0c"_su8, O::V128Const), "\xfd\x0c"_su8);
  OK(Read<O>, MakeAt("\xfd\x0d"_su8, O::V8X16Shuffle), "\xfd\x0d"_su8);
  OK(Read<O>, MakeAt("\xfd\x0e"_su8, O::V8X16Swizzle), "\xfd\x0e"_su8);
  OK(Read<O>, MakeAt("\xfd\x0f"_su8, O::I8X16Splat), "\xfd\x0f"_su8);
  OK(Read<O>, MakeAt("\xfd\x10"_su8, O::I16X8Splat), "\xfd\x10"_su8);
  OK(Read<O>, MakeAt("\xfd\x11"_su8, O::I32X4Splat), "\xfd\x11"_su8);
  OK(Read<O>, MakeAt("\xfd\x12"_su8, O::I64X2Splat), "\xfd\x12"_su8);
  OK(Read<O>, MakeAt("\xfd\x13"_su8, O::F32X4Splat), "\xfd\x13"_su8);
  OK(Read<O>, MakeAt("\xfd\x14"_su8, O::F64X2Splat), "\xfd\x14"_su8);
  OK(Read<O>, MakeAt("\xfd\x15"_su8, O::I8X16ExtractLaneS), "\xfd\x15"_su8);
  OK(Read<O>, MakeAt("\xfd\x16"_su8, O::I8X16ExtractLaneU), "\xfd\x16"_su8);
  OK(Read<O>, MakeAt("\xfd\x17"_su8, O::I8X16ReplaceLane), "\xfd\x17"_su8);
  OK(Read<O>, MakeAt("\xfd\x18"_su8, O::I16X8ExtractLaneS), "\xfd\x18"_su8);
  OK(Read<O>, MakeAt("\xfd\x19"_su8, O::I16X8ExtractLaneU), "\xfd\x19"_su8);
  OK(Read<O>, MakeAt("\xfd\x1a"_su8, O::I16X8ReplaceLane), "\xfd\x1a"_su8);
  OK(Read<O>, MakeAt("\xfd\x1b"_su8, O::I32X4ExtractLane), "\xfd\x1b"_su8);
  OK(Read<O>, MakeAt("\xfd\x1c"_su8, O::I32X4ReplaceLane), "\xfd\x1c"_su8);
  OK(Read<O>, MakeAt("\xfd\x1d"_su8, O::I64X2ExtractLane), "\xfd\x1d"_su8);
  OK(Read<O>, MakeAt("\xfd\x1e"_su8, O::I64X2ReplaceLane), "\xfd\x1e"_su8);
  OK(Read<O>, MakeAt("\xfd\x1f"_su8, O::F32X4ExtractLane), "\xfd\x1f"_su8);
  OK(Read<O>, MakeAt("\xfd\x20"_su8, O::F32X4ReplaceLane), "\xfd\x20"_su8);
  OK(Read<O>, MakeAt("\xfd\x21"_su8, O::F64X2ExtractLane), "\xfd\x21"_su8);
  OK(Read<O>, MakeAt("\xfd\x22"_su8, O::F64X2ReplaceLane), "\xfd\x22"_su8);
  OK(Read<O>, MakeAt("\xfd\x23"_su8, O::I8X16Eq), "\xfd\x23"_su8);
  OK(Read<O>, MakeAt("\xfd\x24"_su8, O::I8X16Ne), "\xfd\x24"_su8);
  OK(Read<O>, MakeAt("\xfd\x25"_su8, O::I8X16LtS), "\xfd\x25"_su8);
  OK(Read<O>, MakeAt("\xfd\x26"_su8, O::I8X16LtU), "\xfd\x26"_su8);
  OK(Read<O>, MakeAt("\xfd\x27"_su8, O::I8X16GtS), "\xfd\x27"_su8);
  OK(Read<O>, MakeAt("\xfd\x28"_su8, O::I8X16GtU), "\xfd\x28"_su8);
  OK(Read<O>, MakeAt("\xfd\x29"_su8, O::I8X16LeS), "\xfd\x29"_su8);
  OK(Read<O>, MakeAt("\xfd\x2a"_su8, O::I8X16LeU), "\xfd\x2a"_su8);
  OK(Read<O>, MakeAt("\xfd\x2b"_su8, O::I8X16GeS), "\xfd\x2b"_su8);
  OK(Read<O>, MakeAt("\xfd\x2c"_su8, O::I8X16GeU), "\xfd\x2c"_su8);
  OK(Read<O>, MakeAt("\xfd\x2d"_su8, O::I16X8Eq), "\xfd\x2d"_su8);
  OK(Read<O>, MakeAt("\xfd\x2e"_su8, O::I16X8Ne), "\xfd\x2e"_su8);
  OK(Read<O>, MakeAt("\xfd\x2f"_su8, O::I16X8LtS), "\xfd\x2f"_su8);
  OK(Read<O>, MakeAt("\xfd\x30"_su8, O::I16X8LtU), "\xfd\x30"_su8);
  OK(Read<O>, MakeAt("\xfd\x31"_su8, O::I16X8GtS), "\xfd\x31"_su8);
  OK(Read<O>, MakeAt("\xfd\x32"_su8, O::I16X8GtU), "\xfd\x32"_su8);
  OK(Read<O>, MakeAt("\xfd\x33"_su8, O::I16X8LeS), "\xfd\x33"_su8);
  OK(Read<O>, MakeAt("\xfd\x34"_su8, O::I16X8LeU), "\xfd\x34"_su8);
  OK(Read<O>, MakeAt("\xfd\x35"_su8, O::I16X8GeS), "\xfd\x35"_su8);
  OK(Read<O>, MakeAt("\xfd\x36"_su8, O::I16X8GeU), "\xfd\x36"_su8);
  OK(Read<O>, MakeAt("\xfd\x37"_su8, O::I32X4Eq), "\xfd\x37"_su8);
  OK(Read<O>, MakeAt("\xfd\x38"_su8, O::I32X4Ne), "\xfd\x38"_su8);
  OK(Read<O>, MakeAt("\xfd\x39"_su8, O::I32X4LtS), "\xfd\x39"_su8);
  OK(Read<O>, MakeAt("\xfd\x3a"_su8, O::I32X4LtU), "\xfd\x3a"_su8);
  OK(Read<O>, MakeAt("\xfd\x3b"_su8, O::I32X4GtS), "\xfd\x3b"_su8);
  OK(Read<O>, MakeAt("\xfd\x3c"_su8, O::I32X4GtU), "\xfd\x3c"_su8);
  OK(Read<O>, MakeAt("\xfd\x3d"_su8, O::I32X4LeS), "\xfd\x3d"_su8);
  OK(Read<O>, MakeAt("\xfd\x3e"_su8, O::I32X4LeU), "\xfd\x3e"_su8);
  OK(Read<O>, MakeAt("\xfd\x3f"_su8, O::I32X4GeS), "\xfd\x3f"_su8);
  OK(Read<O>, MakeAt("\xfd\x40"_su8, O::I32X4GeU), "\xfd\x40"_su8);
  OK(Read<O>, MakeAt("\xfd\x41"_su8, O::F32X4Eq), "\xfd\x41"_su8);
  OK(Read<O>, MakeAt("\xfd\x42"_su8, O::F32X4Ne), "\xfd\x42"_su8);
  OK(Read<O>, MakeAt("\xfd\x43"_su8, O::F32X4Lt), "\xfd\x43"_su8);
  OK(Read<O>, MakeAt("\xfd\x44"_su8, O::F32X4Gt), "\xfd\x44"_su8);
  OK(Read<O>, MakeAt("\xfd\x45"_su8, O::F32X4Le), "\xfd\x45"_su8);
  OK(Read<O>, MakeAt("\xfd\x46"_su8, O::F32X4Ge), "\xfd\x46"_su8);
  OK(Read<O>, MakeAt("\xfd\x47"_su8, O::F64X2Eq), "\xfd\x47"_su8);
  OK(Read<O>, MakeAt("\xfd\x48"_su8, O::F64X2Ne), "\xfd\x48"_su8);
  OK(Read<O>, MakeAt("\xfd\x49"_su8, O::F64X2Lt), "\xfd\x49"_su8);
  OK(Read<O>, MakeAt("\xfd\x4a"_su8, O::F64X2Gt), "\xfd\x4a"_su8);
  OK(Read<O>, MakeAt("\xfd\x4b"_su8, O::F64X2Le), "\xfd\x4b"_su8);
  OK(Read<O>, MakeAt("\xfd\x4c"_su8, O::F64X2Ge), "\xfd\x4c"_su8);
  OK(Read<O>, MakeAt("\xfd\x4d"_su8, O::V128Not), "\xfd\x4d"_su8);
  OK(Read<O>, MakeAt("\xfd\x4e"_su8, O::V128And), "\xfd\x4e"_su8);
  OK(Read<O>, MakeAt("\xfd\x4f"_su8, O::V128Andnot), "\xfd\x4f"_su8);
  OK(Read<O>, MakeAt("\xfd\x50"_su8, O::V128Or), "\xfd\x50"_su8);
  OK(Read<O>, MakeAt("\xfd\x51"_su8, O::V128Xor), "\xfd\x51"_su8);
  OK(Read<O>, MakeAt("\xfd\x52"_su8, O::V128BitSelect), "\xfd\x52"_su8);
  OK(Read<O>, MakeAt("\xfd\x60"_su8, O::I8X16Abs), "\xfd\x60"_su8);
  OK(Read<O>, MakeAt("\xfd\x61"_su8, O::I8X16Neg), "\xfd\x61"_su8);
  OK(Read<O>, MakeAt("\xfd\x62"_su8, O::I8X16AnyTrue), "\xfd\x62"_su8);
  OK(Read<O>, MakeAt("\xfd\x63"_su8, O::I8X16AllTrue), "\xfd\x63"_su8);
  OK(Read<O>, MakeAt("\xfd\x65"_su8, O::I8X16NarrowI16X8S), "\xfd\x65"_su8);
  OK(Read<O>, MakeAt("\xfd\x66"_su8, O::I8X16NarrowI16X8U), "\xfd\x66"_su8);
  OK(Read<O>, MakeAt("\xfd\x6b"_su8, O::I8X16Shl), "\xfd\x6b"_su8);
  OK(Read<O>, MakeAt("\xfd\x6c"_su8, O::I8X16ShrS), "\xfd\x6c"_su8);
  OK(Read<O>, MakeAt("\xfd\x6d"_su8, O::I8X16ShrU), "\xfd\x6d"_su8);
  OK(Read<O>, MakeAt("\xfd\x6e"_su8, O::I8X16Add), "\xfd\x6e"_su8);
  OK(Read<O>, MakeAt("\xfd\x6f"_su8, O::I8X16AddSaturateS), "\xfd\x6f"_su8);
  OK(Read<O>, MakeAt("\xfd\x70"_su8, O::I8X16AddSaturateU), "\xfd\x70"_su8);
  OK(Read<O>, MakeAt("\xfd\x71"_su8, O::I8X16Sub), "\xfd\x71"_su8);
  OK(Read<O>, MakeAt("\xfd\x72"_su8, O::I8X16SubSaturateS), "\xfd\x72"_su8);
  OK(Read<O>, MakeAt("\xfd\x73"_su8, O::I8X16SubSaturateU), "\xfd\x73"_su8);
  OK(Read<O>, MakeAt("\xfd\x76"_su8, O::I8X16MinS), "\xfd\x76"_su8);
  OK(Read<O>, MakeAt("\xfd\x77"_su8, O::I8X16MinU), "\xfd\x77"_su8);
  OK(Read<O>, MakeAt("\xfd\x78"_su8, O::I8X16MaxS), "\xfd\x78"_su8);
  OK(Read<O>, MakeAt("\xfd\x79"_su8, O::I8X16MaxU), "\xfd\x79"_su8);
  OK(Read<O>, MakeAt("\xfd\x7b"_su8, O::I8X16AvgrU), "\xfd\x7b"_su8);
  OK(Read<O>, MakeAt("\xfd\x80\x01"_su8, O::I16X8Abs), "\xfd\x80\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x81\x01"_su8, O::I16X8Neg), "\xfd\x81\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x82\x01"_su8, O::I16X8AnyTrue), "\xfd\x82\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x83\x01"_su8, O::I16X8AllTrue), "\xfd\x83\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x85\x01"_su8, O::I16X8NarrowI32X4S), "\xfd\x85\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x86\x01"_su8, O::I16X8NarrowI32X4U), "\xfd\x86\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x87\x01"_su8, O::I16X8WidenLowI8X16S), "\xfd\x87\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x88\x01"_su8, O::I16X8WidenHighI8X16S), "\xfd\x88\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x89\x01"_su8, O::I16X8WidenLowI8X16U), "\xfd\x89\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x8a\x01"_su8, O::I16X8WidenHighI8X16U), "\xfd\x8a\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x8b\x01"_su8, O::I16X8Shl), "\xfd\x8b\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x8c\x01"_su8, O::I16X8ShrS), "\xfd\x8c\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x8d\x01"_su8, O::I16X8ShrU), "\xfd\x8d\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x8e\x01"_su8, O::I16X8Add), "\xfd\x8e\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x8f\x01"_su8, O::I16X8AddSaturateS), "\xfd\x8f\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x90\x01"_su8, O::I16X8AddSaturateU), "\xfd\x90\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x91\x01"_su8, O::I16X8Sub), "\xfd\x91\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x92\x01"_su8, O::I16X8SubSaturateS), "\xfd\x92\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x93\x01"_su8, O::I16X8SubSaturateU), "\xfd\x93\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x95\x01"_su8, O::I16X8Mul), "\xfd\x95\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x96\x01"_su8, O::I16X8MinS), "\xfd\x96\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x97\x01"_su8, O::I16X8MinU), "\xfd\x97\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x98\x01"_su8, O::I16X8MaxS), "\xfd\x98\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x99\x01"_su8, O::I16X8MaxU), "\xfd\x99\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\x9b\x01"_su8, O::I16X8AvgrU), "\xfd\x9b\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa0\x01"_su8, O::I32X4Abs), "\xfd\xa0\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa1\x01"_su8, O::I32X4Neg), "\xfd\xa1\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa2\x01"_su8, O::I32X4AnyTrue), "\xfd\xa2\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa3\x01"_su8, O::I32X4AllTrue), "\xfd\xa3\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa7\x01"_su8, O::I32X4WidenLowI16X8S), "\xfd\xa7\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa8\x01"_su8, O::I32X4WidenHighI16X8S), "\xfd\xa8\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xa9\x01"_su8, O::I32X4WidenLowI16X8U), "\xfd\xa9\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xaa\x01"_su8, O::I32X4WidenHighI16X8U), "\xfd\xaa\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xab\x01"_su8, O::I32X4Shl), "\xfd\xab\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xac\x01"_su8, O::I32X4ShrS), "\xfd\xac\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xad\x01"_su8, O::I32X4ShrU), "\xfd\xad\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xae\x01"_su8, O::I32X4Add), "\xfd\xae\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xb1\x01"_su8, O::I32X4Sub), "\xfd\xb1\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xb5\x01"_su8, O::I32X4Mul), "\xfd\xb5\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xb6\x01"_su8, O::I32X4MinS), "\xfd\xb6\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xb7\x01"_su8, O::I32X4MinU), "\xfd\xb7\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xb8\x01"_su8, O::I32X4MaxS), "\xfd\xb8\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xb9\x01"_su8, O::I32X4MaxU), "\xfd\xb9\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xc1\x01"_su8, O::I64X2Neg), "\xfd\xc1\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xcb\x01"_su8, O::I64X2Shl), "\xfd\xcb\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xcc\x01"_su8, O::I64X2ShrS), "\xfd\xcc\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xcd\x01"_su8, O::I64X2ShrU), "\xfd\xcd\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xce\x01"_su8, O::I64X2Add), "\xfd\xce\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xd1\x01"_su8, O::I64X2Sub), "\xfd\xd1\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xd5\x01"_su8, O::I64X2Mul), "\xfd\xd5\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe0\x01"_su8, O::F32X4Abs), "\xfd\xe0\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe1\x01"_su8, O::F32X4Neg), "\xfd\xe1\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe3\x01"_su8, O::F32X4Sqrt), "\xfd\xe3\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe4\x01"_su8, O::F32X4Add), "\xfd\xe4\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe5\x01"_su8, O::F32X4Sub), "\xfd\xe5\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe6\x01"_su8, O::F32X4Mul), "\xfd\xe6\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe7\x01"_su8, O::F32X4Div), "\xfd\xe7\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe8\x01"_su8, O::F32X4Min), "\xfd\xe8\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xe9\x01"_su8, O::F32X4Max), "\xfd\xe9\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xec\x01"_su8, O::F64X2Abs), "\xfd\xec\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xed\x01"_su8, O::F64X2Neg), "\xfd\xed\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xef\x01"_su8, O::F64X2Sqrt), "\xfd\xef\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf0\x01"_su8, O::F64X2Add), "\xfd\xf0\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf1\x01"_su8, O::F64X2Sub), "\xfd\xf1\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf2\x01"_su8, O::F64X2Mul), "\xfd\xf2\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf3\x01"_su8, O::F64X2Div), "\xfd\xf3\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf4\x01"_su8, O::F64X2Min), "\xfd\xf4\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf5\x01"_su8, O::F64X2Max), "\xfd\xf5\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf8\x01"_su8, O::I32X4TruncSatF32X4S), "\xfd\xf8\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xf9\x01"_su8, O::I32X4TruncSatF32X4U), "\xfd\xf9\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xfa\x01"_su8, O::F32X4ConvertI32X4S), "\xfd\xfa\x01"_su8);
  OK(Read<O>, MakeAt("\xfd\xfb\x01"_su8, O::F32X4ConvertI32X4U), "\xfd\xfb\x01"_su8);
}

TEST_F(BinaryReadTest, Opcode_Unknown_simd_prefix) {
  context.features.enable_simd();

  const u8 kInvalidOpcodes[] = {
      0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e,
      0x5f, 0x64, 0x67, 0x68, 0x69, 0x6a, 0x74, 0x75, 0x7a, 0x7c, 0x7d, 0x7e,
      0x7f, 0x84, 0x94, 0x9a, 0x9c, 0x9d, 0x9e, 0x9f, 0xa4, 0xa5, 0xa6, 0xaf,
      0xb0, 0xb2, 0xb3, 0xba, 0xbb, 0xc0, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
      0xc8, 0xc9, 0xca, 0xcf, 0xd0, 0xd2, 0xd3, 0xd4, 0xd6, 0xd7, 0xd8, 0xd9,
      0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe2, 0xea, 0xeb, 0xee, 0xf6, 0xf7,
  };
  for (auto code : SpanU8{kInvalidOpcodes, sizeof(kInvalidOpcodes)}) {
    FailUnknownOpcode(0xfd, code);
  }

  // Test some longer codes too.
  FailUnknownOpcode(0xfd, 16384);
  FailUnknownOpcode(0xfd, 2097152);
  FailUnknownOpcode(0xfd, 268435456);
}

TEST_F(BinaryReadTest, Opcode_threads) {
  using O = Opcode;

  context.features.enable_threads();

  OK(Read<O>, MakeAt("\xfe\x00"_su8, O::MemoryAtomicNotify), "\xfe\x00"_su8);
  OK(Read<O>, MakeAt("\xfe\x01"_su8, O::MemoryAtomicWait32), "\xfe\x01"_su8);
  OK(Read<O>, MakeAt("\xfe\x02"_su8, O::MemoryAtomicWait64), "\xfe\x02"_su8);
  OK(Read<O>, MakeAt("\xfe\x10"_su8, O::I32AtomicLoad), "\xfe\x10"_su8);
  OK(Read<O>, MakeAt("\xfe\x11"_su8, O::I64AtomicLoad), "\xfe\x11"_su8);
  OK(Read<O>, MakeAt("\xfe\x12"_su8, O::I32AtomicLoad8U), "\xfe\x12"_su8);
  OK(Read<O>, MakeAt("\xfe\x13"_su8, O::I32AtomicLoad16U), "\xfe\x13"_su8);
  OK(Read<O>, MakeAt("\xfe\x14"_su8, O::I64AtomicLoad8U), "\xfe\x14"_su8);
  OK(Read<O>, MakeAt("\xfe\x15"_su8, O::I64AtomicLoad16U), "\xfe\x15"_su8);
  OK(Read<O>, MakeAt("\xfe\x16"_su8, O::I64AtomicLoad32U), "\xfe\x16"_su8);
  OK(Read<O>, MakeAt("\xfe\x17"_su8, O::I32AtomicStore), "\xfe\x17"_su8);
  OK(Read<O>, MakeAt("\xfe\x18"_su8, O::I64AtomicStore), "\xfe\x18"_su8);
  OK(Read<O>, MakeAt("\xfe\x19"_su8, O::I32AtomicStore8), "\xfe\x19"_su8);
  OK(Read<O>, MakeAt("\xfe\x1a"_su8, O::I32AtomicStore16), "\xfe\x1a"_su8);
  OK(Read<O>, MakeAt("\xfe\x1b"_su8, O::I64AtomicStore8), "\xfe\x1b"_su8);
  OK(Read<O>, MakeAt("\xfe\x1c"_su8, O::I64AtomicStore16), "\xfe\x1c"_su8);
  OK(Read<O>, MakeAt("\xfe\x1d"_su8, O::I64AtomicStore32), "\xfe\x1d"_su8);
  OK(Read<O>, MakeAt("\xfe\x1e"_su8, O::I32AtomicRmwAdd), "\xfe\x1e"_su8);
  OK(Read<O>, MakeAt("\xfe\x1f"_su8, O::I64AtomicRmwAdd), "\xfe\x1f"_su8);
  OK(Read<O>, MakeAt("\xfe\x20"_su8, O::I32AtomicRmw8AddU), "\xfe\x20"_su8);
  OK(Read<O>, MakeAt("\xfe\x21"_su8, O::I32AtomicRmw16AddU), "\xfe\x21"_su8);
  OK(Read<O>, MakeAt("\xfe\x22"_su8, O::I64AtomicRmw8AddU), "\xfe\x22"_su8);
  OK(Read<O>, MakeAt("\xfe\x23"_su8, O::I64AtomicRmw16AddU), "\xfe\x23"_su8);
  OK(Read<O>, MakeAt("\xfe\x24"_su8, O::I64AtomicRmw32AddU), "\xfe\x24"_su8);
  OK(Read<O>, MakeAt("\xfe\x25"_su8, O::I32AtomicRmwSub), "\xfe\x25"_su8);
  OK(Read<O>, MakeAt("\xfe\x26"_su8, O::I64AtomicRmwSub), "\xfe\x26"_su8);
  OK(Read<O>, MakeAt("\xfe\x27"_su8, O::I32AtomicRmw8SubU), "\xfe\x27"_su8);
  OK(Read<O>, MakeAt("\xfe\x28"_su8, O::I32AtomicRmw16SubU), "\xfe\x28"_su8);
  OK(Read<O>, MakeAt("\xfe\x29"_su8, O::I64AtomicRmw8SubU), "\xfe\x29"_su8);
  OK(Read<O>, MakeAt("\xfe\x2a"_su8, O::I64AtomicRmw16SubU), "\xfe\x2a"_su8);
  OK(Read<O>, MakeAt("\xfe\x2b"_su8, O::I64AtomicRmw32SubU), "\xfe\x2b"_su8);
  OK(Read<O>, MakeAt("\xfe\x2c"_su8, O::I32AtomicRmwAnd), "\xfe\x2c"_su8);
  OK(Read<O>, MakeAt("\xfe\x2d"_su8, O::I64AtomicRmwAnd), "\xfe\x2d"_su8);
  OK(Read<O>, MakeAt("\xfe\x2e"_su8, O::I32AtomicRmw8AndU), "\xfe\x2e"_su8);
  OK(Read<O>, MakeAt("\xfe\x2f"_su8, O::I32AtomicRmw16AndU), "\xfe\x2f"_su8);
  OK(Read<O>, MakeAt("\xfe\x30"_su8, O::I64AtomicRmw8AndU), "\xfe\x30"_su8);
  OK(Read<O>, MakeAt("\xfe\x31"_su8, O::I64AtomicRmw16AndU), "\xfe\x31"_su8);
  OK(Read<O>, MakeAt("\xfe\x32"_su8, O::I64AtomicRmw32AndU), "\xfe\x32"_su8);
  OK(Read<O>, MakeAt("\xfe\x33"_su8, O::I32AtomicRmwOr), "\xfe\x33"_su8);
  OK(Read<O>, MakeAt("\xfe\x34"_su8, O::I64AtomicRmwOr), "\xfe\x34"_su8);
  OK(Read<O>, MakeAt("\xfe\x35"_su8, O::I32AtomicRmw8OrU), "\xfe\x35"_su8);
  OK(Read<O>, MakeAt("\xfe\x36"_su8, O::I32AtomicRmw16OrU), "\xfe\x36"_su8);
  OK(Read<O>, MakeAt("\xfe\x37"_su8, O::I64AtomicRmw8OrU), "\xfe\x37"_su8);
  OK(Read<O>, MakeAt("\xfe\x38"_su8, O::I64AtomicRmw16OrU), "\xfe\x38"_su8);
  OK(Read<O>, MakeAt("\xfe\x39"_su8, O::I64AtomicRmw32OrU), "\xfe\x39"_su8);
  OK(Read<O>, MakeAt("\xfe\x3a"_su8, O::I32AtomicRmwXor), "\xfe\x3a"_su8);
  OK(Read<O>, MakeAt("\xfe\x3b"_su8, O::I64AtomicRmwXor), "\xfe\x3b"_su8);
  OK(Read<O>, MakeAt("\xfe\x3c"_su8, O::I32AtomicRmw8XorU), "\xfe\x3c"_su8);
  OK(Read<O>, MakeAt("\xfe\x3d"_su8, O::I32AtomicRmw16XorU), "\xfe\x3d"_su8);
  OK(Read<O>, MakeAt("\xfe\x3e"_su8, O::I64AtomicRmw8XorU), "\xfe\x3e"_su8);
  OK(Read<O>, MakeAt("\xfe\x3f"_su8, O::I64AtomicRmw16XorU), "\xfe\x3f"_su8);
  OK(Read<O>, MakeAt("\xfe\x40"_su8, O::I64AtomicRmw32XorU), "\xfe\x40"_su8);
  OK(Read<O>, MakeAt("\xfe\x41"_su8, O::I32AtomicRmwXchg), "\xfe\x41"_su8);
  OK(Read<O>, MakeAt("\xfe\x42"_su8, O::I64AtomicRmwXchg), "\xfe\x42"_su8);
  OK(Read<O>, MakeAt("\xfe\x43"_su8, O::I32AtomicRmw8XchgU), "\xfe\x43"_su8);
  OK(Read<O>, MakeAt("\xfe\x44"_su8, O::I32AtomicRmw16XchgU), "\xfe\x44"_su8);
  OK(Read<O>, MakeAt("\xfe\x45"_su8, O::I64AtomicRmw8XchgU), "\xfe\x45"_su8);
  OK(Read<O>, MakeAt("\xfe\x46"_su8, O::I64AtomicRmw16XchgU), "\xfe\x46"_su8);
  OK(Read<O>, MakeAt("\xfe\x47"_su8, O::I64AtomicRmw32XchgU), "\xfe\x47"_su8);
  OK(Read<O>, MakeAt("\xfe\x48"_su8, O::I32AtomicRmwCmpxchg), "\xfe\x48"_su8);
  OK(Read<O>, MakeAt("\xfe\x49"_su8, O::I64AtomicRmwCmpxchg), "\xfe\x49"_su8);
  OK(Read<O>, MakeAt("\xfe\x4a"_su8, O::I32AtomicRmw8CmpxchgU), "\xfe\x4a"_su8);
  OK(Read<O>, MakeAt("\xfe\x4b"_su8, O::I32AtomicRmw16CmpxchgU), "\xfe\x4b"_su8);
  OK(Read<O>, MakeAt("\xfe\x4c"_su8, O::I64AtomicRmw8CmpxchgU), "\xfe\x4c"_su8);
  OK(Read<O>, MakeAt("\xfe\x4d"_su8, O::I64AtomicRmw16CmpxchgU), "\xfe\x4d"_su8);
  OK(Read<O>, MakeAt("\xfe\x4e"_su8, O::I64AtomicRmw32CmpxchgU), "\xfe\x4e"_su8);
}

TEST_F(BinaryReadTest, Opcode_Unknown_threads_prefix) {
  context.features.enable_threads();

  const u8 kInvalidOpcodes[] = {
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x4f, 0x50,
  };
  for (auto code : SpanU8{kInvalidOpcodes, sizeof(kInvalidOpcodes)}) {
    FailUnknownOpcode(0xfe, code);
  }

  // Test some longer codes too.
  FailUnknownOpcode(0xfe, 128);
  FailUnknownOpcode(0xfe, 16384);
  FailUnknownOpcode(0xfe, 2097152);
  FailUnknownOpcode(0xfe, 268435456);
}

TEST_F(BinaryReadTest, S32) {
  OK(Read<s32>, 32, "\x20"_su8);
  OK(Read<s32>, -16, "\x70"_su8);
  OK(Read<s32>, 448, "\xc0\x03"_su8);
  OK(Read<s32>, -3648, "\xc0\x63"_su8);
  OK(Read<s32>, 33360, "\xd0\x84\x02"_su8);
  OK(Read<s32>, -753072, "\xd0\x84\x52"_su8);
  OK(Read<s32>, 101718048, "\xa0\xb0\xc0\x30"_su8);
  OK(Read<s32>, -32499680, "\xa0\xb0\xc0\x70"_su8);
  OK(Read<s32>, 1042036848, "\xf0\xf0\xf0\xf0\x03"_su8);
  OK(Read<s32>, -837011344, "\xf0\xf0\xf0\xf0\x7c"_su8);
}

TEST_F(BinaryReadTest, S32_TooLong) {
  Fail(Read<s32>,
       {{0, "s32"},
        {4,
         "Last byte of s32 must be sign extension: expected "
         "0x5 or 0x7d, got 0x15"}},
       "\xf0\xf0\xf0\xf0\x15"_su8);
  Fail(Read<s32>,
       {{0, "s32"},
        {4,
         "Last byte of s32 must be sign extension: expected "
         "0x3 or 0x7b, got 0x73"}},
       "\xff\xff\xff\xff\x73"_su8);
}

TEST_F(BinaryReadTest, S32_PastEnd) {
  Fail(Read<s32>, {{0, "s32"}, {0, "Unable to read u8"}}, ""_su8);
  Fail(Read<s32>, {{0, "s32"}, {1, "Unable to read u8"}}, "\xc0"_su8);
  Fail(Read<s32>, {{0, "s32"}, {2, "Unable to read u8"}}, "\xd0\x84"_su8);
  Fail(Read<s32>, {{0, "s32"}, {3, "Unable to read u8"}}, "\xa0\xb0\xc0"_su8);
  Fail(Read<s32>, {{0, "s32"}, {4, "Unable to read u8"}}, "\xf0\xf0\xf0\xf0"_su8);
}

TEST_F(BinaryReadTest, S64) {
  OK(Read<s64>, 32, "\x20"_su8);
  OK(Read<s64>, -16, "\x70"_su8);
  OK(Read<s64>, 448, "\xc0\x03"_su8);
  OK(Read<s64>, -3648, "\xc0\x63"_su8);
  OK(Read<s64>, 33360, "\xd0\x84\x02"_su8);
  OK(Read<s64>, -753072, "\xd0\x84\x52"_su8);
  OK(Read<s64>, 101718048, "\xa0\xb0\xc0\x30"_su8);
  OK(Read<s64>, -32499680, "\xa0\xb0\xc0\x70"_su8);
  OK(Read<s64>, 1042036848, "\xf0\xf0\xf0\xf0\x03"_su8);
  OK(Read<s64>, -837011344, "\xf0\xf0\xf0\xf0\x7c"_su8);
  OK(Read<s64>, 13893120096, "\xe0\xe0\xe0\xe0\x33"_su8);
  OK(Read<s64>, -12413554592, "\xe0\xe0\xe0\xe0\x51"_su8);
  OK(Read<s64>, 1533472417872, "\xd0\xd0\xd0\xd0\xd0\x2c"_su8);
  OK(Read<s64>, -287593715632, "\xd0\xd0\xd0\xd0\xd0\x77"_su8);
  OK(Read<s64>, 139105536057408, "\xc0\xc0\xc0\xc0\xc0\xd0\x1f"_su8);
  OK(Read<s64>, -124777254608832, "\xc0\xc0\xc0\xc0\xc0\xd0\x63"_su8);
  OK(Read<s64>, 1338117014066474, "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"_su8);
  OK(Read<s64>, -12172681868045014, "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"_su8);
  OK(Read<s64>, 1070725794579330814, "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"_su8);
  OK(Read<s64>, -3540960223848057090, "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"_su8);
}

TEST_F(BinaryReadTest, S64_TooLong) {
  Fail(Read<s64>,
       {{0, "s64"},
        {9,
         "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
         "0xf0"}},
       "\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"_su8);
  Fail(Read<s64>,
       {{0, "s64"},
        {9,
         "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
         "0xff"}},
       "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"_su8);
}

TEST_F(BinaryReadTest, S64_PastEnd) {
  Fail(Read<s64>, {{0, "s64"}, {0, "Unable to read u8"}}, ""_su8);
  Fail(Read<s64>, {{0, "s64"}, {1, "Unable to read u8"}}, "\xc0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {2, "Unable to read u8"}}, "\xd0\x84"_su8);
  Fail(Read<s64>, {{0, "s64"}, {3, "Unable to read u8"}}, "\xa0\xb0\xc0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {4, "Unable to read u8"}}, "\xf0\xf0\xf0\xf0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {5, "Unable to read u8"}}, "\xe0\xe0\xe0\xe0\xe0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {6, "Unable to read u8"}}, "\xd0\xd0\xd0\xd0\xd0\xc0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {7, "Unable to read u8"}}, "\xc0\xc0\xc0\xc0\xc0\xd0\x84"_su8);
  Fail(Read<s64>, {{0, "s64"}, {8, "Unable to read u8"}}, "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {9, "Unable to read u8"}}, "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"_su8);
}

TEST_F(BinaryReadTest, SectionId) {
  OK(Read<SectionId>, SectionId::Custom, "\x00"_su8);
  OK(Read<SectionId>, SectionId::Type, "\x01"_su8);
  OK(Read<SectionId>, SectionId::Import, "\x02"_su8);
  OK(Read<SectionId>, SectionId::Function, "\x03"_su8);
  OK(Read<SectionId>, SectionId::Table, "\x04"_su8);
  OK(Read<SectionId>, SectionId::Memory, "\x05"_su8);
  OK(Read<SectionId>, SectionId::Global, "\x06"_su8);
  OK(Read<SectionId>, SectionId::Export, "\x07"_su8);
  OK(Read<SectionId>, SectionId::Start, "\x08"_su8);
  OK(Read<SectionId>, SectionId::Element, "\x09"_su8);
  OK(Read<SectionId>, SectionId::Code, "\x0a"_su8);
  OK(Read<SectionId>, SectionId::Data, "\x0b"_su8);

  // Overlong encoding.
  OK(Read<SectionId>, SectionId::Custom, "\x80\x00"_su8);
}

TEST_F(BinaryReadTest, SectionId_bulk_memory) {
  Fail(Read<SectionId>, {{0, "section id"}, {1, "Unknown section id: 12"}},
       "\x0c"_su8);

  context.features.enable_bulk_memory();

  OK(Read<SectionId>, SectionId::DataCount, "\x0c"_su8);
}

TEST_F(BinaryReadTest, SectionId_exceptions) {
  Fail(Read<SectionId>,
       {{0, "section id"}, {1, "Unknown section id: 13"}}, "\x0d"_su8);

  context.features.enable_exceptions();

  OK(Read<SectionId>, SectionId::Event, "\x0d"_su8);
}

TEST_F(BinaryReadTest, SectionId_Unknown) {
  Fail(Read<SectionId>, {{0, "section id"}, {1, "Unknown section id: 14"}},
       "\x0e"_su8);
}

TEST_F(BinaryReadTest, Section) {
  OK(Read<Section>,
     Section{MakeAt("\x01\x03\x01\x02\x03"_su8,
                    KnownSection{MakeAt("\x01"_su8, SectionId::Type),
                                 "\x01\x02\x03"_su8})},
     "\x01\x03\x01\x02\x03"_su8);

  OK(Read<Section>,
     Section{MakeAt(
         "\x00\x08\x04name\x04\x05\x06"_su8,
         CustomSection{MakeAt("\x04name"_su8, "name"_sv), "\x04\x05\x06"_su8})},
     "\x00\x08\x04name\x04\x05\x06"_su8);
}

TEST_F(BinaryReadTest, Section_PastEnd) {
  Fail(
      Read<Section>,
      {{0, "section"}, {0, "section id"}, {0, "u32"}, {0, "Unable to read u8"}},
      ""_su8);

  Fail(Read<Section>, {{0, "section"}, {1, "length"}, {1, "Unable to read u8"}},
       "\x01"_su8);

  Fail(Read<Section>, {{0, "section"}, {1, "Length extends past end: 1 > 0"}},
       "\x01\x01"_su8);
}

TEST_F(BinaryReadTest, Start) {
  OK(Read<Start>, Start{MakeAt("\x80\x02"_su8, Index{256})}, "\x80\x02"_su8);
}

TEST_F(BinaryReadTest, ReadString) {
  OK(ReadString, "hello"_sv, "\x05hello"_su8, "test");
}

TEST_F(BinaryReadTest, ReadString_Leftovers) {
  const SpanU8 data = "\x01more"_su8;
  SpanU8 copy = data;
  auto result = ReadString(&copy, context, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ(string_view{"m"}, result);
  EXPECT_EQ(3u, copy.size());
}

TEST_F(BinaryReadTest, ReadString_BadLength) {
  Fail(ReadString, {{0, "test"}, {0, "length"}, {0, "Unable to read u8"}},
       ""_su8, "test");

  Fail(ReadString, {{0, "test"}, {0, "length"}, {1, "Unable to read u8"}},
       "\xc0"_su8, "test");
}

TEST_F(BinaryReadTest, ReadString_Fail) {
  const SpanU8 data = "\x06small"_su8;
  SpanU8 copy = data;
  auto result = ReadString(&copy, context, "test");
  ExpectError({{0, "test"}, {0, "Length extends past end: 6 > 5"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(5u, copy.size());
}

TEST_F(BinaryReadTest, Table) {
  OK(Read<Table>,
     Table{MakeAt("\x70\x00\x01"_su8,
                  TableType{MakeAt("\x00\x01"_su8,
                                   Limits{MakeAt("\x01"_su8, u32{1}), nullopt,
                                          MakeAt("\x00"_su8, Shared::No)}),
                            MakeAt("\x70"_su8, ReferenceType::Funcref)})},
     "\x70\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Table_PastEnd) {
  Fail(Read<Table>,
       {{0, "table"},
        {0, "table type"},
        {0, "element type"},
        {0, "Unable to read u8"}},
       ""_su8);
}

TEST_F(BinaryReadTest, TableType) {
  OK(Read<TableType>,
     TableType{
         MakeAt("\x00\x01"_su8, Limits{MakeAt("\x01"_su8, u32{1}), nullopt,
                                       MakeAt("\x00"_su8, Shared::No)}),
         MakeAt("\x70"_su8, ReferenceType::Funcref)},
     "\x70\x00\x01"_su8);
  OK(Read<TableType>,
       TableType{
          MakeAt("\x01\x01\x02"_su8,
                 Limits{MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, u32{2}),
                        MakeAt("\x01"_su8, Shared::No)}),
          MakeAt("\x70"_su8, ReferenceType::Funcref)},
      "\x70\x01\x01\x02"_su8);
}

TEST_F(BinaryReadTest, TableType_BadReferenceType) {
  Fail(Read<TableType>,
       {{0, "table type"}, {0, "element type"}, {1, "Unknown element type: 0"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, TableType_PastEnd) {
  Fail(Read<TableType>,
       {{0, "table type"}, {0, "element type"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<TableType>,
       {{0, "table type"},
        {1, "limits"},
        {1, "flags"},
        {1, "Unable to read u8"}},
       "\x70"_su8);
}

TEST_F(BinaryReadTest, TypeEntry) {
  OK(Read<TypeEntry>,
     TypeEntry{MakeAt("\x00\x01\x7f"_su8,
                      FunctionType{{}, {MakeAt("\x7f"_su8, ValueType::I32)}})},
     "\x60\x00\x01\x7f"_su8);
}

TEST_F(BinaryReadTest, TypeEntry_BadForm) {
  Fail(Read<TypeEntry>, {{0, "type entry"}, {0, "Unknown type form: 64"}},
       "\x40"_su8);
}

TEST_F(BinaryReadTest, U32) {
  OK(Read<u32>, 32u, "\x20"_su8);
  OK(Read<u32>, 448u, "\xc0\x03"_su8);
  OK(Read<u32>, 33360u, "\xd0\x84\x02"_su8);
  OK(Read<u32>, 101718048u, "\xa0\xb0\xc0\x30"_su8);
  OK(Read<u32>, 1042036848u, "\xf0\xf0\xf0\xf0\x03"_su8);
}

TEST_F(BinaryReadTest, U32_TooLong) {
  Fail(Read<u32>,
       {{0, "u32"},
        {4, "Last byte of u32 must be zero extension: expected 0x2, got 0x12"}},
       "\xf0\xf0\xf0\xf0\x12"_su8);
}

TEST_F(BinaryReadTest, U32_PastEnd) {
  Fail(Read<u32>, {{0, "u32"}, {0, "Unable to read u8"}}, ""_su8);
  Fail(Read<u32>, {{0, "u32"}, {1, "Unable to read u8"}}, "\xc0"_su8);
  Fail(Read<u32>, {{0, "u32"}, {2, "Unable to read u8"}}, "\xd0\x84"_su8);
  Fail(Read<u32>, {{0, "u32"}, {3, "Unable to read u8"}}, "\xa0\xb0\xc0"_su8);
  Fail(Read<u32>, {{0, "u32"}, {4, "Unable to read u8"}}, "\xf0\xf0\xf0\xf0"_su8);
}

TEST_F(BinaryReadTest, U8) {
  OK(Read<u8>, 32, "\x20"_su8);
  Fail(Read<u8>, {{0, "Unable to read u8"}}, ""_su8);
}

TEST_F(BinaryReadTest, ValueType_MVP) {
  OK(Read<ValueType>, ValueType::I32, "\x7f"_su8);
  OK(Read<ValueType>, ValueType::I64, "\x7e"_su8);
  OK(Read<ValueType>, ValueType::F32, "\x7d"_su8);
  OK(Read<ValueType>, ValueType::F64, "\x7c"_su8);
}

TEST_F(BinaryReadTest, ValueType_simd) {
  Fail(Read<ValueType>, {{0, "value type"}, {1, "Unknown value type: 123"}},
       "\x7b"_su8);

  context.features.enable_simd();
  OK(Read<ValueType>, ValueType::V128, "\x7b"_su8);
}

TEST_F(BinaryReadTest, ValueType_reference_types) {
  Fail(Read<ValueType>, {{0, "value type"}, {1, "Unknown value type: 112"}},
       "\x70"_su8);
  Fail(Read<ValueType>, {{0, "value type"}, {1, "Unknown value type: 111"}},
       "\x6f"_su8);

  context.features.enable_reference_types();
  OK(Read<ValueType>, ValueType::Funcref, "\x70"_su8);
  OK(Read<ValueType>, ValueType::Externref, "\x6f"_su8);
}

TEST_F(BinaryReadTest, ValueType_exceptions) {
  Fail(Read<ValueType>,
       {{0, "value type"}, {1, "Unknown value type: 104"}}, "\x68"_su8);

  context.features.enable_exceptions();
  OK(Read<ValueType>, ValueType::Exnref, "\x68"_su8);
}

TEST_F(BinaryReadTest, ValueType_Unknown) {
  Fail(Read<ValueType>, {{0, "value type"}, {1, "Unknown value type: 16"}},
       "\x10"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<ValueType>, {{0, "value type"}, {1, "Unknown value type: 255"}},
       "\xff\x7f"_su8);
}

TEST_F(BinaryReadTest, ReadVector_u8) {
  const SpanU8 data = "\x05hello"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u8>(&copy, context, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<At<u8>>{
                MakeAt("h"_su8, u8{'h'}),
                MakeAt("e"_su8, u8{'e'}),
                MakeAt("l"_su8, u8{'l'}),
                MakeAt("l"_su8, u8{'l'}),
                MakeAt("o"_su8, u8{'o'}),
            }),
            result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, ReadVector_u32) {
  const SpanU8 data =
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, context, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<At<u32>>{
                MakeAt("\x05"_su8, u32{5}),
                MakeAt("\x80\x01"_su8, u32{128}),
                MakeAt("\xcc\xcc\x0c"_su8, u32{206412}),
            }),
            result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, ReadVector_FailLength) {
  const SpanU8 data =
      "\x02"  // Count.
      "\x05"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, context, "test");
  ExpectError({{0, "test"}, {0, "Count extends past end: 2 > 1"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(1u, copy.size());
}

TEST_F(BinaryReadTest, ReadVector_PastEnd) {
  const SpanU8 data =
      "\x02"  // Count.
      "\x05"
      "\x80"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, context, "test");
  ExpectError({{0, "test"}, {2, "u32"}, {3, "Unable to read u8"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, EndCode_UnclosedBlock) {
  const SpanU8 data = "\x02\x40"_su8;  // block void
  context.open_blocks.push_back(MakeAt(data.first(1), Opcode::Block));
  EXPECT_FALSE(EndCode(data.last(0), context));
  ExpectError({{0, "Unclosed block instruction"}}, errors, data);
}

TEST_F(BinaryReadTest, EndCode_MissingEnd) {
  const SpanU8 data = "\x01"_su8;  // nop
  context.seen_final_end = false;
  EXPECT_FALSE(EndCode(data.last(0), context));
  ExpectError({{1, "Expected final end instruction"}}, errors, data);
}

TEST_F(BinaryReadTest, EndModule_FunctionCodeMismatch) {
  const SpanU8 data =
      "\0asm\x01\x00\x00\x00"  // magic + version
      "\x01\x04\x60\x00"       // (type (func))
      "\x03\x02\x01\x00"_su8;  // (func (type 0)), no code section
  context.defined_function_count = 1;
  context.code_count = 0;
  EXPECT_FALSE(EndModule(data.last(0), context));
  ExpectError({{16, "Expected code count of 1, but got 0"}}, errors, data);
}

TEST_F(BinaryReadTest, EndModule_DataCount_DataMissing) {
  const SpanU8 data =
      "\0asm\x01\x00\x00\x00"  // magic + version
      "\x0c\x01\x01"_su8;      // data count = 1
  context.declared_data_count = 1;
  context.data_count = 0;
  EXPECT_FALSE(EndModule(data.last(0), context));
  ExpectError({{11, "Expected data count of 1, but got 0"}}, errors, data);
}

TEST_F(BinaryReadTest, EndModule_DataCountMismatch) {
  const SpanU8 data =
      "\0asm\x01\x00\x00\x00"      // magic + version
      "\x0c\x01\x00"               // data count = 0
      "\x0b\x03\x01\x01\x00"_su8;  // empty passive data segment
  context.declared_data_count = 0;
  context.data_count = 1;
  EXPECT_FALSE(EndModule(data.last(0), context));
  ExpectError({{16, "Expected data count of 0, but got 1"}}, errors, data);
}
