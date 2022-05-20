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
#include "test/binary/constants.h"
#include "test/binary/test_utils.h"
#include "test/test_utils.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/name_section/read.h"
#include "wasp/binary/read/read_ctx.h"
#include "wasp/binary/read/read_vector.h"

#include "wasp/base/concat.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;
using namespace ::wasp::binary::test;

using I = Instruction;
using O = Opcode;

class BinaryReadTest : public ::testing::Test {
 protected:
  void SetUp() { ctx.features.DisableAll(); }

  template <typename Func, typename T, typename... Args>
  void OK(Func&& func, const T& expected, SpanU8 data, Args&&... args) {
    auto actual = func(&data, ctx, std::forward<Args>(args)...);
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
    auto actual = func(&data, ctx, std::forward<Args>(args)...);
    EXPECT_FALSE(actual.has_value());
    ExpectError(error, errors, orig_data);
    errors.Clear();
  }

  void FailUnknownOpcode(u8 code) {
    const u8 span_buffer[] = {code};
    auto msg = concat("Unknown opcode: ", code);
    Fail(Read<Opcode>, {{0, "opcode"}, {0, msg}}, SpanU8{span_buffer, 1});
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
          {0, concat("Unknown opcode: ", prefix, " ", orig_code)}},
         SpanU8{data, length});
  }

  TestErrors errors;
  ReadCtx ctx{errors};
};

TEST_F(BinaryReadTest, ArrayType) {
  ctx.features.enable_gc();

  OK(Read<ArrayType>,
     ArrayType{At{"\x7f\x00"_su8,
                  FieldType{At{"\x7f"_su8, StorageType{At{"\x7f"_su8, VT_I32}}},
                            At{"\x00"_su8, Mutability::Const}}}},
     "\x7f\x00"_su8);
}

TEST_F(BinaryReadTest, BlockType_MVP) {
  OK(Read<BlockType>, BT_I32, "\x7f"_su8);
  OK(Read<BlockType>, BT_I64, "\x7e"_su8);
  OK(Read<BlockType>, BT_F32, "\x7d"_su8);
  OK(Read<BlockType>, BT_F64, "\x7c"_su8);
  OK(Read<BlockType>, BT_Void, "\x40"_su8);
}

TEST_F(BinaryReadTest, BlockType_Basic_multi_value) {
  ctx.features.enable_multi_value();

  OK(Read<BlockType>, BT_I32, "\x7f"_su8);
  OK(Read<BlockType>, BT_I64, "\x7e"_su8);
  OK(Read<BlockType>, BT_F32, "\x7d"_su8);
  OK(Read<BlockType>, BT_F64, "\x7c"_su8);
  OK(Read<BlockType>, BT_Void, "\x40"_su8);
}

TEST_F(BinaryReadTest, BlockType_simd) {
  Fail(Read<BlockType>,
       {{0, "block type"}, {0, "value type"}, {0, "Unknown value type: 123"}},
       "\x7b"_su8);

  ctx.features.enable_simd();
  OK(Read<BlockType>, BT_V128, "\x7b"_su8);
}

TEST_F(BinaryReadTest, BlockType_MultiValueNegative) {
  ctx.features.enable_multi_value();
  Fail(Read<BlockType>,
       {{0, "block type"},
        {0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 119"}},
       "\x77"_su8);
}

TEST_F(BinaryReadTest, BlockType_multi_value) {
  Fail(Read<BlockType>,
       {{0, "block type"},
        {0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 1"}},
       "\x01"_su8);

  ctx.features.enable_multi_value();
  OK(Read<BlockType>, BlockType{At{"\x01"_su8, Index{1}}}, "\x01"_su8);
  OK(Read<BlockType>, BlockType{At{"\xc0\x03"_su8, Index{448}}},
     "\xc0\x03"_su8);
}

TEST_F(BinaryReadTest, BlockType_reference_types) {
  Fail(Read<BlockType>,
       {{0, "block type"},
        {0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 111"}},
       "\x6f"_su8);

  ctx.features.enable_reference_types();
  OK(Read<BlockType>, BT_Externref, "\x6f"_su8);
}

TEST_F(BinaryReadTest, BlockType_gc) {
  Fail(Read<BlockType>,
       {{0, "block type"},
        {0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 110"}},
       "\x6e"_su8);

  ctx.features.enable_gc();

  OK(Read<BlockType>, BT_Anyref, "\x6e"_su8);
  OK(Read<BlockType>, BT_Eqref, "\x6d"_su8);
  OK(Read<BlockType>, BT_I31ref, "\x6a"_su8);
  OK(Read<BlockType>, BT_RefFunc, "\x6b\x70"_su8);
  OK(Read<BlockType>, BT_RefNullFunc, "\x6c\x70"_su8);
  OK(Read<BlockType>, BT_RefExtern, "\x6b\x6f"_su8);
  OK(Read<BlockType>, BT_RefNullExtern, "\x6c\x6f"_su8);
  OK(Read<BlockType>, BT_RefAny, "\x6b\x6e"_su8);
  OK(Read<BlockType>, BT_RefNullAny, "\x6c\x6e"_su8);
  OK(Read<BlockType>, BT_RefEq, "\x6b\x6d"_su8);
  OK(Read<BlockType>, BT_RefNullEq, "\x6c\x6d"_su8);
  OK(Read<BlockType>, BT_RefI31, "\x6b\x6a"_su8);
  OK(Read<BlockType>, BT_RefNullI31, "\x6c\x6a"_su8);
  OK(Read<BlockType>, BT_Ref0, "\x6b\x00"_su8);
  OK(Read<BlockType>, BT_RefNull0, "\x6c\x00"_su8);
  OK(Read<BlockType>, BT_RTT_0_Func, "\x69\x00\x70"_su8);
  OK(Read<BlockType>, BT_RTT_0_Extern, "\x69\x00\x6f"_su8);
  OK(Read<BlockType>, BT_RTT_0_Any, "\x69\x00\x6e"_su8);
  OK(Read<BlockType>, BT_RTT_0_Eq, "\x69\x00\x6d"_su8);
  OK(Read<BlockType>, BT_RTT_0_I31, "\x69\x00\x6a"_su8);
  OK(Read<BlockType>, BT_RTT_0_0, "\x69\x00\x00"_su8);
}

TEST_F(BinaryReadTest, BlockType_Unknown) {
  Fail(Read<BlockType>,
       {{0, "block type"},
        {0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 0"}},
       "\x00"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<BlockType>,
       {{0, "block type"},
        {0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 255"}},
       "\xff\x7f"_su8);
}

TEST_F(BinaryReadTest, BrOnCastImmediate) {
  OK(Read<BrOnCastImmediate>,
     BrOnCastImmediate{
         At{"\x00"_su8, Index{0}},
         At{"\x70\x70"_su8, HeapType2Immediate{At{"\x70"_su8, HT_Func},
                                               At{"\x70"_su8, HT_Func}}}},
     "\x00\x70\x70"_su8);
}

TEST_F(BinaryReadTest, BrTableImmediate) {
  OK(Read<BrTableImmediate>, BrTableImmediate{{}, At{"\x00"_su8, Index{0}}},
     "\x00\x00"_su8);

  OK(Read<BrTableImmediate>,
     BrTableImmediate{{At{"\x01"_su8, Index{1}}, At{"\x02"_su8, Index{2}}},
                      At{"\x03"_su8, Index{3}}},
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
  auto result = ReadBytes(&copy, 3, ctx);
  ExpectNoErrors(errors);
  EXPECT_EQ(data, **result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, ReadBytes_Leftovers) {
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, ctx);
  ExpectNoErrors(errors);
  EXPECT_EQ(data.subspan(0, 2), **result);
  EXPECT_EQ(1u, copy.size());
}

TEST_F(BinaryReadTest, ReadBytes_Fail) {
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, ctx);
  EXPECT_EQ(nullopt, result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

TEST_F(BinaryReadTest, CallIndirectImmediate) {
  OK(Read<CallIndirectImmediate>,
     CallIndirectImmediate{At{"\x01"_su8, Index{1}}, At{"\x00"_su8, Index{0}}},
     "\x01\x00"_su8);
  OK(Read<CallIndirectImmediate>,
     CallIndirectImmediate{At{"\x80\x01"_su8, Index{128}},
                           At{"\x00"_su8, Index{0}}},
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
  OK(Read<Code>, Code{{}, At{""_su8, ""_expr}}, "\x01\x00"_su8);

  // Smallest valid empty body.
  OK(Read<Code>, Code{{}, At{"\x0b"_su8, "\x0b"_expr}}, "\x02\x00\x0b"_su8);

  // (func
  //   (local i32 i32 i64 i64 i64)
  //   (nop))
  OK(Read<Code>,
     Code{{At{"\x02\x7f"_su8,
              Locals{At{"\x02"_su8, Index{2}}, At{"\x7f"_su8, VT_I32}}},
           At{"\x03\x7e"_su8,
              Locals{At{"\x03"_su8, Index{3}}, At{"\x7e"_su8, VT_I64}}}},
          At{"\x01\x0b"_su8, "\x01\x0b"_expr}},
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
     ConstantExpression{
         At{"\x41\x00"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                        At{"\x00"_su8, s32{0}}}}},
     "\x41\x00\x0b"_su8);

  // i64.const
  OK(Read<ConstantExpression>,
     ConstantExpression{
         At{"\x42\x80\x80\x80\x80\x80\x01"_su8,
            Instruction{At{"\x42"_su8, Opcode::I64Const},
                        At{"\x80\x80\x80\x80\x80\x01"_su8, s64{34359738368}}}}},
     "\x42\x80\x80\x80\x80\x80\x01\x0b"_su8);

  // f32.const
  OK(Read<ConstantExpression>,
     ConstantExpression{At{"\x43\x00\x00\x00\x00"_su8,
                           Instruction{At{"\x43"_su8, Opcode::F32Const},
                                       At{"\x00\x00\x00\x00"_su8, f32{0}}}}},
     "\x43\x00\x00\x00\x00\x0b"_su8);

  // f64.const
  OK(Read<ConstantExpression>,
     ConstantExpression{
         At{"\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
            Instruction{At{"\x44"_su8, Opcode::F64Const},
                        At{"\x00\x00\x00\x00\x00\x00\x00\x00"_su8, f64{0}}}}},
     "\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b"_su8);

  // global.get
  OK(Read<ConstantExpression>,
     ConstantExpression{
         At{"\x23\x00"_su8, Instruction{At{"\x23"_su8, Opcode::GlobalGet},
                                        At{"\x00"_su8, Index{0}}}}},
     "\x23\x00\x0b"_su8);

  // Other instructions are invalid, but not malformed.
  OK(Read<ConstantExpression>,
     ConstantExpression{
         At{"\x01"_su8, Instruction{At{"\x01"_su8, Opcode::Nop}}}},
     "\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_ReferenceTypes) {
  // ref.null
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {0, "Unknown opcode: 208"}},
       "\xd0\x70\x0b"_su8);

  // ref.func
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {0, "Unknown opcode: 210"}},
       "\xd2\x00\x0b"_su8);

  ctx.features.enable_reference_types();

  // ref.null
  OK(Read<ConstantExpression>,
     ConstantExpression{
         At{"\xd0\x70"_su8, Instruction{At{"\xd0"_su8, Opcode::RefNull},
                                        At{"\x70"_su8, HT_Func}}}},
     "\xd0\x70\x0b"_su8);

  // ref.func
  OK(Read<ConstantExpression>,
     ConstantExpression{
         At{"\xd2\x00"_su8, Instruction{At{"\xd2"_su8, Opcode::RefFunc},
                                        At{"\x00"_su8, Index{0}}}}},
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
         At{"\x41\x00"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                        At{"\x00"_su8, s32{0}}}},
         At{"\x01"_su8, Instruction{At{"\x01"_su8, Opcode::Nop}}},
     }},
     "\x41\x00\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_InvalidInstruction) {
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {0, "Unknown opcode: 6"}},
       "\x06"_su8);
}

TEST_F(BinaryReadTest, ConstantExpression_PastEnd) {
  Fail(Read<ConstantExpression>,
       {{0, "constant expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
       ""_su8);
}

auto ReadMemoryCopyImmediate_ForTesting(SpanU8* data, ReadCtx& ctx)
    -> OptAt<CopyImmediate> {
  return Read<CopyImmediate>(data, ctx, BulkImmediateKind::Memory);
}

auto ReadTableCopyImmediate_ForTesting(SpanU8* data, ReadCtx& ctx)
    -> OptAt<CopyImmediate> {
  return Read<CopyImmediate>(data, ctx, BulkImmediateKind::Table);
}

TEST_F(BinaryReadTest, CopyImmediate) {
  OK(ReadMemoryCopyImmediate_ForTesting,
     CopyImmediate{At{"\x00"_su8, Index{0}}, At{"\x00"_su8, Index{0}}},
     "\x00\x00"_su8);

  OK(ReadTableCopyImmediate_ForTesting,
     CopyImmediate{At{"\x00"_su8, Index{0}}, At{"\x00"_su8, Index{0}}},
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
  ctx.features.enable_reference_types();

  OK(ReadTableCopyImmediate_ForTesting,
     CopyImmediate{At{"\x80\x01"_su8, Index{128}}, At{"\x01"_su8, Index{1}}},
     "\x80\x01\x01"_su8);

  OK(ReadTableCopyImmediate_ForTesting,
     CopyImmediate{At{"\x01"_su8, Index{1}}, At{"\x80\x01"_su8, Index{128}}},
     "\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, CopyImmediate_Memory_reference_types) {
  ctx.features.enable_reference_types();

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

TEST_F(BinaryReadTest, CopyImmediate_Memory_multi_memory) {
  ctx.features.enable_multi_memory();

  OK(ReadMemoryCopyImmediate_ForTesting,
     CopyImmediate{At{"\x80\x01"_su8, Index{128}}, At{"\x01"_su8, Index{1}}},
     "\x80\x01\x01"_su8);

  OK(ReadMemoryCopyImmediate_ForTesting,
     CopyImmediate{At{"\x01"_su8, Index{1}}, At{"\x80\x01"_su8, Index{128}}},
     "\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, FuncBindImmediate) {
  OK(Read<FuncBindImmediate>, FuncBindImmediate{At{"\x00"_su8, Index{0}}},
     "\x00"_su8);
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
  auto result = ReadCount(&copy, ctx);
  ExpectNoErrors(errors);
  EXPECT_EQ(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST_F(BinaryReadTest, ReadCount_PastEnd) {
  const SpanU8 data = "\x05\x00\x00\x00"_su8;
  SpanU8 copy = data;
  auto result = ReadCount(&copy, ctx);
  ExpectError({{0, "Count extends past end: 5 > 3"}}, errors, data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(3u, copy.size());
}

TEST_F(BinaryReadTest, DataSegment_MVP) {
  OK(Read<DataSegment>,
     DataSegment{
         At{"\x01"_su8, Index{1}},
         At{"\x42\x01\x0b"_su8,
            ConstantExpression{
                At{"\x42\x01"_su8, Instruction{At{"\x42"_su8, Opcode::I64Const},
                                               At{"\x01"_su8, s64{1}}}}}},
         At{"\x04wxyz"_su8, "wxyz"_su8}},
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
  ctx.features.enable_bulk_memory();

  OK(Read<DataSegment>, DataSegment{At{"\x04wxyz"_su8, "wxyz"_su8}},
     "\x01\x04wxyz"_su8);

  OK(Read<DataSegment>,
     DataSegment{
         At{"\x01"_su8, Index{1}},
         At{"\x41\x02\x0b"_su8,
            ConstantExpression{
                At{"\x41\x02"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x02"_su8, s32{2}}}}}},
         At{"\x03xyz"_su8, "xyz"_su8}},
     "\x02\x01\x41\x02\x0b\x03xyz"_su8);
}

TEST_F(BinaryReadTest, DataSegment_BulkMemory_BadFlags) {
  ctx.features.enable_bulk_memory();

  Fail(Read<DataSegment>, {{0, "data segment"}, {0, "Unknown flags: 3"}},
       "\x03"_su8);
}

TEST_F(BinaryReadTest, DataSegment_BulkMemory_PastEnd) {
  ctx.features.enable_bulk_memory();

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
  ctx.features.enable_bulk_memory();

  // ref.null
  OK(Read<ElementExpression>,
     ElementExpression{
         At{"\xd0\x70"_su8, Instruction{At{"\xd0"_su8, Opcode::RefNull},
                                        At{"\x70"_su8, HT_Func}}}},
     "\xd0\x70\x0b"_su8);

  // ref.func 2
  OK(Read<ElementExpression>,
     ElementExpression{
         At{"\xd2\x02"_su8, Instruction{At{"\xd2"_su8, Opcode::RefFunc},
                                        At{"\x02"_su8, Index{2}}}}},
     "\xd2\x02\x0b"_su8);

  // Other instructions are invalid, but not malformed.
  OK(Read<ElementExpression>,
     ElementExpression{
         At{"\x01"_su8, Instruction{At{"\x01"_su8, Opcode::Nop}}}},
     "\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_NoEnd) {
  ctx.features.enable_bulk_memory();

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
  ctx.features.enable_bulk_memory();

  // An instruction sequence of length 0 is invalid, but not malformed.
  OK(Read<ElementExpression>, ElementExpression{}, "\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_TooLong) {
  ctx.features.enable_bulk_memory();

  OK(Read<ElementExpression>,
     ElementExpression{InstructionList{
         At{"\xd0\x70"_su8, Instruction{At{"\xd0"_su8, Opcode::RefNull},
                                        At{"\x70"_su8, HT_Func}}},
         At{"\x01"_su8, Instruction{At{"\x01"_su8, Opcode::Nop}}},
     }},
     "\xd0\x70\x01\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_InvalidInstruction) {
  ctx.features.enable_bulk_memory();

  Fail(Read<ElementExpression>,
       {{0, "element expression"}, {0, "opcode"}, {0, "Unknown opcode: 6"}},
       "\x06"_su8);
}

TEST_F(BinaryReadTest, ElementExpression_PastEnd) {
  ctx.features.enable_bulk_memory();

  Fail(Read<ElementExpression>,
       {{0, "element expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
       ""_su8);
}

TEST_F(BinaryReadTest, ElementSegment_MVP) {
  OK(Read<ElementSegment>,
     ElementSegment{
         At{"\x00"_su8, Index{0}},
         At{"\x41\x01\x0b"_su8,
            ConstantExpression{
                At{"\x41\x01"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x01"_su8, s32{1}}}}}},
         ElementListWithIndexes{
             ExternalKind::Function,
             {At{"\x01"_su8, Index{1}}, At{"\x02"_su8, Index{2}},
              At{"\x03"_su8, Index{3}}}}},
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
  ctx.features.enable_bulk_memory();

  // Flags == 1: Passive, index list
  OK(Read<ElementSegment>,
     ElementSegment{SegmentType::Passive,
                    ElementListWithIndexes{
                        At{"\x00"_su8, ExternalKind::Function},
                        {At{"\x01"_su8, Index{1}}, At{"\x02"_su8, Index{2}}}}},
     "\x01\x00\x02\x01\x02"_su8);

  // Flags == 2: Active, table index, index list
  OK(Read<ElementSegment>,  //*
     ElementSegment{
         At{"\x01"_su8, Index{1}},
         At{"\x41\x02\x0b"_su8,
            ConstantExpression{
                At{"\x41\x02"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x02"_su8, s32{2}}}}}},
         ElementListWithIndexes{
             At{"\x00"_su8, ExternalKind::Function},
             {At{"\x03"_su8, Index{3}}, At{"\x04"_su8, Index{4}}}}},
     "\x02\x01\x41\x02\x0b\x00\x02\x03\x04"_su8);

  // Flags == 4: Active (function only), table 0, expression list
  OK(Read<ElementSegment>,
     ElementSegment{
         Index{0},
         At{"\x41\x05\x0b"_su8,
            ConstantExpression{
                At{"\x41\x05"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x05"_su8, s32{5}}}}}},
         ElementListWithExpressions{
             ReferenceType::Funcref_NoLocation(),
             {At{"\xd2\x06\x0b"_su8,
                 ElementExpression{
                     At{"\xd2\x06"_su8,
                        Instruction{At{"\xd2"_su8, Opcode::RefFunc},
                                    At{"\x06"_su8, Index{6}}}}}}}}},
     "\x04\x41\x05\x0b\x01\xd2\x06\x0b"_su8);

  // Flags == 5: Passive, expression list
  OK(Read<ElementSegment>,
     ElementSegment{SegmentType::Passive,
                    ElementListWithExpressions{
                        At{"\x70"_su8, RT_Funcref},
                        {At{"\xd2\x07\x0b"_su8,
                            ElementExpression{
                                At{"\xd2\x07"_su8,
                                   Instruction{At{"\xd2"_su8, Opcode::RefFunc},
                                               At{"\x07"_su8, Index{7}}}}}},
                         At{"\xd0\x70\x0b"_su8,
                            ElementExpression{
                                At{"\xd0\x70"_su8,
                                   Instruction{At{"\xd0"_su8, Opcode::RefNull},
                                               At{"\x70"_su8, HT_Func}}}}}}}},
     "\x05\x70\x02\xd2\x07\x0b\xd0\x70\x0b"_su8);

  // Flags == 6: Active, table index, expression list
  OK(Read<ElementSegment>,  //*
     ElementSegment{
         At{"\x02"_su8, Index{2}},
         At{"\x41\x08\x0b"_su8,
            ConstantExpression{
                At{"\x41\x08"_su8, Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x08"_su8, s32{8}}}}}},
         ElementListWithExpressions{
             At{"\x70"_su8, RT_Funcref},
             {At{"\xd0\x70\x0b"_su8,
                 ElementExpression{
                     At{"\xd0\x70"_su8,
                        Instruction{At{"\xd0"_su8, Opcode::RefNull},
                                    At{"\x70"_su8, HT_Func}}}}}}}},
     "\x06\x02\x41\x08\x0b\x70\x01\xd0\x70\x0b"_su8);
}

TEST_F(BinaryReadTest, ElementSegment_BulkMemory_BadFlags) {
  ctx.features.enable_bulk_memory();

  // Flags == 3: Declared, index list
  Fail(Read<ElementSegment>, {{0, "element segment"}, {0, "Unknown flags: 3"}},
       "\x03"_su8);

  // Flags == 7: Declared, expression list
  Fail(Read<ElementSegment>, {{0, "element segment"}, {0, "Unknown flags: 7"}},
       "\x07"_su8);
}

TEST_F(BinaryReadTest, ElementSegment_BulkMemory_PastEnd) {
  ctx.features.enable_bulk_memory();

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
  Fail(
      Read<ElementSegment>,
      {{0, "element segment"}, {1, "reference type"}, {1, "Unable to read u8"}},
      "\x05"_su8);

  // Flags == 6: Active, table index, expression list
  Fail(Read<ElementSegment>,
       {{0, "element segment"}, {1, "table index"}, {1, "Unable to read u8"}},
       "\x06"_su8);
}

TEST_F(BinaryReadTest, ReferenceType) {
  OK(Read<ReferenceType>, RT_Funcref, "\x70"_su8);
}

TEST_F(BinaryReadTest, ReferenceType_ReferenceTypes) {
  Fail(Read<ReferenceType>,
       {{0, "reference type"}, {0, "Unknown reference type: 111"}}, "\x6f"_su8);

  ctx.features.enable_reference_types();

  OK(Read<ReferenceType>, RT_Externref, "\x6f"_su8);
}

TEST_F(BinaryReadTest, ReferenceType_gc) {
  Fail(Read<ValueType>,
       {{0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 110"}},
       "\x6e"_su8);

  ctx.features.enable_gc();

  OK(Read<ReferenceType>, RT_Anyref, "\x6e"_su8);
  OK(Read<ReferenceType>, RT_Eqref, "\x6d"_su8);
  OK(Read<ReferenceType>, RT_I31ref, "\x6a"_su8);
  OK(Read<ReferenceType>, RT_RefFunc, "\x6b\x70"_su8);
  OK(Read<ReferenceType>, RT_RefNullFunc, "\x6c\x70"_su8);
  OK(Read<ReferenceType>, RT_RefExtern, "\x6b\x6f"_su8);
  OK(Read<ReferenceType>, RT_RefNullExtern, "\x6c\x6f"_su8);
  OK(Read<ReferenceType>, RT_RefAny, "\x6b\x6e"_su8);
  OK(Read<ReferenceType>, RT_RefNullAny, "\x6c\x6e"_su8);
  OK(Read<ReferenceType>, RT_RefEq, "\x6b\x6d"_su8);
  OK(Read<ReferenceType>, RT_RefNullEq, "\x6c\x6d"_su8);
  OK(Read<ReferenceType>, RT_RefI31, "\x6b\x6a"_su8);
  OK(Read<ReferenceType>, RT_RefNullI31, "\x6c\x6a"_su8);
  OK(Read<ReferenceType>, RT_Ref0, "\x6b\x00"_su8);
  OK(Read<ReferenceType>, RT_RefNull0, "\x6c\x00"_su8);
}

TEST_F(BinaryReadTest, ReferenceType_Unknown) {
  Fail(Read<ReferenceType>,
       {{0, "reference type"}, {0, "Unknown reference type: 0"}}, "\x00"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<ReferenceType>,
       {{0, "reference type"}, {0, "Unknown reference type: 240"}},
       "\xf0\x7f"_su8);
}

TEST_F(BinaryReadTest, Tag) {
  OK(Read<Tag>,
     Tag{At{"\x00\x01"_su8, TagType{At{"\x00"_su8, TagAttribute::Exception},
                                    At{"\x01"_su8, Index{1}}}}},
     "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Tag_PastEnd) {
  Fail(Read<Tag>,
       {{0, "tag"},
        {0, "tag type"},
        {0, "tag attribute"},
        {0, "u32"},
        {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<Tag>,
       {{0, "tag"},
        {0, "tag type"},
        {1, "type index"},
        {1, "Unable to read u8"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, TagType) {
  OK(Read<TagType>,
     TagType{At{"\x00"_su8, TagAttribute::Exception}, At{"\x01"_su8, Index{1}}},
     "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Export) {
  OK(Read<Export>,
     Export{At{"\x00"_su8, ExternalKind::Function}, At{"\x02hi"_su8, "hi"_sv},
            At{"\x03"_su8, Index{3}}},
     "\x02hi\x00\x03"_su8);
  OK(Read<Export>,
     Export{At{"\x01"_su8, ExternalKind::Table}, At{"\x00"_su8, ""_sv},
            At{"\xe8\x07"_su8, Index{1000}}},
     "\x00\x01\xe8\x07"_su8);
  OK(Read<Export>,
     Export{At{"\x02"_su8, ExternalKind::Memory}, At{"\x03mem"_su8, "mem"_sv},
            At{"\x00"_su8, Index{0}}},
     "\x03mem\x02\x00"_su8);
  OK(Read<Export>,
     Export{At{"\x03"_su8, ExternalKind::Global}, At{"\x01g"_su8, "g"_sv},
            At{"\x01"_su8, Index{1}}},
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
       {{0, "export"}, {2, "external kind"}, {2, "Unknown external kind: 4"}},
       "\x01v\x04\x02"_su8);

  ctx.features.enable_exceptions();
  OK(Read<Export>,
     Export{At{"\x04"_su8, ExternalKind::Tag}, At{"\x01v"_su8, "v"_sv},
            At{"\x02"_su8, Index{2}}},
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
       {{0, "external kind"}, {0, "Unknown external kind: 4"}}, "\x04"_su8);

  ctx.features.enable_exceptions();

  OK(Read<ExternalKind>, ExternalKind::Tag, "\x04"_su8);
}

TEST_F(BinaryReadTest, ExternalKind_Unknown) {
  Fail(Read<ExternalKind>,
       {{0, "external kind"}, {0, "Unknown external kind: 5"}}, "\x05"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<ExternalKind>,
       {{0, "external kind"}, {0, "Unknown external kind: 132"}},
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
    auto result = Read<f32>(&data, ctx);
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
    auto result = Read<f64>(&data, ctx);
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

TEST_F(BinaryReadTest, FieldType) {
  ctx.features.enable_gc();

  OK(Read<FieldType>,
     FieldType{At{"\x7f"_su8, StorageType{At{"\x7f"_su8, VT_I32}}},
               At{"\x00"_su8, Mutability::Const}},
     "\x7f\x00"_su8);

  OK(Read<FieldType>,
     FieldType{At{"\x7a"_su8, StorageType{At{"\x7a"_su8, PackedType::I8}}},
               At{"\x01"_su8, Mutability::Var}},
     "\x7a\x01"_su8);
}

TEST_F(BinaryReadTest, Function) {
  OK(Read<Function>, Function{At{"\x01"_su8, Index{1}}}, "\x01"_su8);
}

TEST_F(BinaryReadTest, Function_PastEnd) {
  Fail(Read<Function>,
       {{0, "function"}, {0, "type index"}, {0, "Unable to read u8"}}, ""_su8);
}

TEST_F(BinaryReadTest, FunctionType) {
  OK(Read<FunctionType>, FunctionType{{}, {}}, "\x00\x00"_su8);
  OK(Read<FunctionType>,
     FunctionType{{At{"\x7f"_su8, VT_I32}, At{"\x7e"_su8, VT_I64}},
                  {At{"\x7c"_su8, VT_F64}}},
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
     Global{At{"\x7f\x01"_su8, GlobalType{At{"\x7f"_su8, VT_I32},
                                          At{"\x01"_su8, Mutability::Var}}},
            At{"\x42\x00\x0b"_su8,
               ConstantExpression{At{
                   "\x42\x00"_su8, Instruction{At{"\x42"_su8, Opcode::I64Const},
                                               At{"\x00"_su8, s64{0}}}}}}},
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
     GlobalType{At{"\x7f"_su8, VT_I32}, At{"\x00"_su8, Mutability::Const}},
     "\x7f\x00"_su8);
  OK(Read<GlobalType>,
     GlobalType{At{"\x7d"_su8, VT_F32}, At{"\x01"_su8, Mutability::Var}},
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

TEST_F(BinaryReadTest, HeapType_function_references) {
  ctx.features.enable_function_references();

  OK(Read<HeapType>, HT_Func, "\x70"_su8);
  OK(Read<HeapType>, HT_Extern, "\x6f"_su8);
  OK(Read<HeapType>, HT_0, "\x00"_su8);
}

TEST_F(BinaryReadTest, HeapType_gc) {
  ctx.features.enable_gc();

  OK(Read<HeapType>, HT_Any, "\x6e"_su8);
  OK(Read<HeapType>, HT_Eq, "\x6d"_su8);
  OK(Read<HeapType>, HT_I31, "\x6a"_su8);
}

TEST_F(BinaryReadTest, HeapType2Immediate) {
  ctx.features.enable_function_references();

  OK(Read<HeapType2Immediate>,
     HeapType2Immediate{At{"\x70"_su8, HT_Func}, At{"\x00"_su8, HT_0}},
     "\x70\x00"_su8);
}

TEST_F(BinaryReadTest, Import) {
  OK(Read<Import>,
     Import{At{"\x01"
               "a"_su8,
               "a"_sv},
            At{"\x04"
               "func"_su8,
               "func"_sv},
            At{"\x0b"_su8, Index{11}}},
     "\x01"
     "a\x04"
     "func\x00\x0b"_su8);

  OK(Read<Import>,
     Import{
         At{"\x01"
            "b"_su8,
            "b"_sv},
         At{"\x05table"_su8, "table"_sv},
         At{"\x70\x00\x01"_su8,
            TableType{At{"\x00\x01"_su8, Limits{At{"\x01"_su8, u32{1}}, nullopt,
                                                At{"\x00"_su8, Shared::No}}},
                      At{"\x70"_su8, RT_Funcref}}}},
     "\x01"
     "b\x05table\x01\x70\x00\x01"_su8);

  OK(Read<Import>,
     Import{At{"\x01"
               "c"_su8,
               "c"_sv},
            At{"\x06memory"_su8, "memory"_sv},
            At{"\x01\x00\x02"_su8,
               MemoryType{
                   At{"\x01\x00\x02"_su8,
                      Limits{At{"\x00"_su8, u32{0}}, At{"\x02"_su8, u32{2}},
                             At{"\x01"_su8, Shared::No}}},
               }}},
     "\x01"
     "c\x06memory\x02\x01\x00\x02"_su8);

  OK(Read<Import>,
     Import{At{"\x01"
               "d"_su8,
               "d"_sv},
            At{"\x06global"_su8, "global"_sv},
            At{"\x7f\x00"_su8, GlobalType{At{"\x7f"_su8, VT_I32},
                                          At{"\x00"_su8, Mutability::Const}}}},
     "\x01\x64\x06global\x03\x7f\x00"_su8);
}

TEST_F(BinaryReadTest, Import_exceptions) {
  Fail(Read<Import>,
       {{0, "import"}, {7, "external kind"}, {7, "Unknown external kind: 4"}},
       "\x01v\x04!tag\x04\x00\x02"_su8);

  ctx.features.enable_exceptions();
  OK(Read<Import>,
     Import{At{"\x01v"_su8, "v"_sv}, At{"\x04!tag"_su8, "!tag"_sv},
            At{"\x00\x02"_su8, TagType{At{"\x00"_su8, TagAttribute::Exception},
                                       At{"\x02"_su8, Index{2}}}}},
     "\x01v\x04!tag\x04\x00\x02"_su8);
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
        {3, "reference type"},
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
     IndirectNameAssoc{
         At{"\x64"_su8, Index{100}},
         {At{"\x00\x04zero"_su8, NameAssoc{At{"\x00"_su8, Index{0}},
                                           At{"\x04zero"_su8, "zero"_sv}}},
          At{"\x01\x03one"_su8, NameAssoc{At{"\x01"_su8, Index{1}},
                                          At{"\x03one"_su8, "one"_sv}}}}},
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

auto ReadMemoryInitImmediate_ForTesting(SpanU8* data, ReadCtx& ctx)
    -> OptAt<InitImmediate> {
  return Read<InitImmediate>(data, ctx, BulkImmediateKind::Memory);
}

auto ReadTableInitImmediate_ForTesting(SpanU8* data, ReadCtx& ctx)
    -> OptAt<InitImmediate> {
  return Read<InitImmediate>(data, ctx, BulkImmediateKind::Table);
}

TEST_F(BinaryReadTest, InitImmediate) {
  OK(ReadMemoryInitImmediate_ForTesting,
     InitImmediate{At{"\x01"_su8, Index{1}}, At{"\x00"_su8, Index{0}}},
     "\x01\x00"_su8);

  OK(ReadMemoryInitImmediate_ForTesting,
     InitImmediate{At{"\x80\x01"_su8, Index{128}}, At{"\x00"_su8, Index{0}}},
     "\x80\x01\x00"_su8);

  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{At{"\x01"_su8, Index{1}}, At{"\x00"_su8, Index{0}}},
     "\x01\x00"_su8);

  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{At{"\x80\x01"_su8, Index{128}}, At{"\x00"_su8, Index{0}}},
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
  ctx.features.enable_reference_types();

  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{At{"\x01"_su8, Index{1}}, At{"\x01"_su8, Index{1}}},
     "\x01\x01"_su8);
  OK(ReadTableInitImmediate_ForTesting,
     InitImmediate{At{"\x80\x01"_su8, Index{128}},
                   At{"\x80\x01"_su8, Index{128}}},
     "\x80\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, InitImmediate_Memory_reference_types) {
  ctx.features.enable_reference_types();

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

TEST_F(BinaryReadTest, InitImmediate_Memory_multi_memory) {
  ctx.features.enable_multi_memory();

  OK(ReadMemoryInitImmediate_ForTesting,
     InitImmediate{At{"\x01"_su8, Index{1}}, At{"\x01"_su8, Index{1}}},
     "\x01\x01"_su8);
  OK(ReadMemoryInitImmediate_ForTesting,
     InitImmediate{At{"\x80\x01"_su8, Index{128}},
                   At{"\x80\x01"_su8, Index{128}}},
     "\x80\x01\x80\x01"_su8);
}

TEST_F(BinaryReadTest, PlainInstruction) {
  using MemArg = MemArgImmediate;

  auto memarg = At{"\x01\x02"_su8,
                   MemArg{At{"\x01"_su8, u32{1}}, At{"\x02"_su8, u32{2}}}};

  OK(Read<I>, I{At{"\x00"_su8, O::Unreachable}}, "\x00"_su8);
  OK(Read<I>, I{At{"\x01"_su8, O::Nop}}, "\x01"_su8);
  OK(Read<I>, I{At{"\x0c"_su8, O::Br}, At{"\x01"_su8, Index{1}}},
     "\x0c\x01"_su8);
  OK(Read<I>, I{At{"\x0d"_su8, O::BrIf}, At{"\x02"_su8, Index{2}}},
     "\x0d\x02"_su8);
  OK(Read<I>,
     I{At{"\x0e"_su8, O::BrTable},
       At{"\x03\x03\x04\x05\x06"_su8,
          BrTableImmediate{{At{"\x03"_su8, Index{3}}, At{"\x04"_su8, Index{4}},
                            At{"\x05"_su8, Index{5}}},
                           At{"\x06"_su8, Index{6}}}}},
     "\x0e\x03\x03\x04\x05\x06"_su8);
  OK(Read<I>, I{At{"\x0f"_su8, O::Return}}, "\x0f"_su8);
  OK(Read<I>, I{At{"\x10"_su8, O::Call}, At{"\x07"_su8, Index{7}}},
     "\x10\x07"_su8);
  OK(Read<I>,
     I{At{"\x11"_su8, O::CallIndirect},
       At{"\x08\x00"_su8, CallIndirectImmediate{At{"\x08"_su8, Index{8}},
                                                At{"\x00"_su8, Index{0}}}}},
     "\x11\x08\x00"_su8);
  OK(Read<I>, I{At{"\x1a"_su8, O::Drop}}, "\x1a"_su8);
  OK(Read<I>, I{At{"\x1b"_su8, O::Select}}, "\x1b"_su8);
  OK(Read<I>, I{At{"\x20"_su8, O::LocalGet}, At{"\x05"_su8, Index{5}}},
     "\x20\x05"_su8);
  OK(Read<I>, I{At{"\x21"_su8, O::LocalSet}, At{"\x06"_su8, Index{6}}},
     "\x21\x06"_su8);
  OK(Read<I>, I{At{"\x22"_su8, O::LocalTee}, At{"\x07"_su8, Index{7}}},
     "\x22\x07"_su8);
  OK(Read<I>, I{At{"\x23"_su8, O::GlobalGet}, At{"\x08"_su8, Index{8}}},
     "\x23\x08"_su8);
  OK(Read<I>, I{At{"\x24"_su8, O::GlobalSet}, At{"\x09"_su8, Index{9}}},
     "\x24\x09"_su8);
  OK(Read<I>, I{At{"\x28"_su8, O::I32Load}, memarg}, "\x28\x01\x02"_su8);
  OK(Read<I>, I{At{"\x29"_su8, O::I64Load}, memarg}, "\x29\x01\x02"_su8);
  OK(Read<I>, I{At{"\x2a"_su8, O::F32Load}, memarg}, "\x2a\x01\x02"_su8);
  OK(Read<I>, I{At{"\x2b"_su8, O::F64Load}, memarg}, "\x2b\x01\x02"_su8);
  OK(Read<I>, I{At{"\x2c"_su8, O::I32Load8S}, memarg}, "\x2c\x01\x02"_su8);
  OK(Read<I>, I{At{"\x2d"_su8, O::I32Load8U}, memarg}, "\x2d\x01\x02"_su8);
  OK(Read<I>, I{At{"\x2e"_su8, O::I32Load16S}, memarg}, "\x2e\x01\x02"_su8);
  OK(Read<I>, I{At{"\x2f"_su8, O::I32Load16U}, memarg}, "\x2f\x01\x02"_su8);
  OK(Read<I>, I{At{"\x30"_su8, O::I64Load8S}, memarg}, "\x30\x01\x02"_su8);
  OK(Read<I>, I{At{"\x31"_su8, O::I64Load8U}, memarg}, "\x31\x01\x02"_su8);
  OK(Read<I>, I{At{"\x32"_su8, O::I64Load16S}, memarg}, "\x32\x01\x02"_su8);
  OK(Read<I>, I{At{"\x33"_su8, O::I64Load16U}, memarg}, "\x33\x01\x02"_su8);
  OK(Read<I>, I{At{"\x34"_su8, O::I64Load32S}, memarg}, "\x34\x01\x02"_su8);
  OK(Read<I>, I{At{"\x35"_su8, O::I64Load32U}, memarg}, "\x35\x01\x02"_su8);
  OK(Read<I>, I{At{"\x36"_su8, O::I32Store}, memarg}, "\x36\x01\x02"_su8);
  OK(Read<I>, I{At{"\x37"_su8, O::I64Store}, memarg}, "\x37\x01\x02"_su8);
  OK(Read<I>, I{At{"\x38"_su8, O::F32Store}, memarg}, "\x38\x01\x02"_su8);
  OK(Read<I>, I{At{"\x39"_su8, O::F64Store}, memarg}, "\x39\x01\x02"_su8);
  OK(Read<I>, I{At{"\x3a"_su8, O::I32Store8}, memarg}, "\x3a\x01\x02"_su8);
  OK(Read<I>, I{At{"\x3b"_su8, O::I32Store16}, memarg}, "\x3b\x01\x02"_su8);
  OK(Read<I>, I{At{"\x3c"_su8, O::I64Store8}, memarg}, "\x3c\x01\x02"_su8);
  OK(Read<I>, I{At{"\x3d"_su8, O::I64Store16}, memarg}, "\x3d\x01\x02"_su8);
  OK(Read<I>, I{At{"\x3e"_su8, O::I64Store32}, memarg}, "\x3e\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\x3f"_su8, O::MemorySize},
       At{"\x00"_su8, MemOptImmediate{At{"\x00"_su8, Index{0}}}}},
     "\x3f\x00"_su8);
  OK(Read<I>,
     I{At{"\x40"_su8, O::MemoryGrow},
       At{"\x00"_su8, MemOptImmediate{At{"\x00"_su8, Index{0}}}}},
     "\x40\x00"_su8);
  OK(Read<I>, I{At{"\x41"_su8, O::I32Const}, At{"\x00"_su8, s32{0}}},
     "\x41\x00"_su8);
  OK(Read<I>, I{At{"\x42"_su8, O::I64Const}, At{"\x00"_su8, s64{0}}},
     "\x42\x00"_su8);
  OK(Read<I>,
     I{At{"\x43"_su8, O::F32Const}, At{"\x00\x00\x00\x00"_su8, f32{0}}},
     "\x43\x00\x00\x00\x00"_su8);
  OK(Read<I>,
     I{At{"\x44"_su8, O::F64Const},
       At{"\x00\x00\x00\x00\x00\x00\x00\x00"_su8, f64{0}}},
     "\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<I>, I{At{"\x45"_su8, O::I32Eqz}}, "\x45"_su8);
  OK(Read<I>, I{At{"\x46"_su8, O::I32Eq}}, "\x46"_su8);
  OK(Read<I>, I{At{"\x47"_su8, O::I32Ne}}, "\x47"_su8);
  OK(Read<I>, I{At{"\x48"_su8, O::I32LtS}}, "\x48"_su8);
  OK(Read<I>, I{At{"\x49"_su8, O::I32LtU}}, "\x49"_su8);
  OK(Read<I>, I{At{"\x4a"_su8, O::I32GtS}}, "\x4a"_su8);
  OK(Read<I>, I{At{"\x4b"_su8, O::I32GtU}}, "\x4b"_su8);
  OK(Read<I>, I{At{"\x4c"_su8, O::I32LeS}}, "\x4c"_su8);
  OK(Read<I>, I{At{"\x4d"_su8, O::I32LeU}}, "\x4d"_su8);
  OK(Read<I>, I{At{"\x4e"_su8, O::I32GeS}}, "\x4e"_su8);
  OK(Read<I>, I{At{"\x4f"_su8, O::I32GeU}}, "\x4f"_su8);
  OK(Read<I>, I{At{"\x50"_su8, O::I64Eqz}}, "\x50"_su8);
  OK(Read<I>, I{At{"\x51"_su8, O::I64Eq}}, "\x51"_su8);
  OK(Read<I>, I{At{"\x52"_su8, O::I64Ne}}, "\x52"_su8);
  OK(Read<I>, I{At{"\x53"_su8, O::I64LtS}}, "\x53"_su8);
  OK(Read<I>, I{At{"\x54"_su8, O::I64LtU}}, "\x54"_su8);
  OK(Read<I>, I{At{"\x55"_su8, O::I64GtS}}, "\x55"_su8);
  OK(Read<I>, I{At{"\x56"_su8, O::I64GtU}}, "\x56"_su8);
  OK(Read<I>, I{At{"\x57"_su8, O::I64LeS}}, "\x57"_su8);
  OK(Read<I>, I{At{"\x58"_su8, O::I64LeU}}, "\x58"_su8);
  OK(Read<I>, I{At{"\x59"_su8, O::I64GeS}}, "\x59"_su8);
  OK(Read<I>, I{At{"\x5a"_su8, O::I64GeU}}, "\x5a"_su8);
  OK(Read<I>, I{At{"\x5b"_su8, O::F32Eq}}, "\x5b"_su8);
  OK(Read<I>, I{At{"\x5c"_su8, O::F32Ne}}, "\x5c"_su8);
  OK(Read<I>, I{At{"\x5d"_su8, O::F32Lt}}, "\x5d"_su8);
  OK(Read<I>, I{At{"\x5e"_su8, O::F32Gt}}, "\x5e"_su8);
  OK(Read<I>, I{At{"\x5f"_su8, O::F32Le}}, "\x5f"_su8);
  OK(Read<I>, I{At{"\x60"_su8, O::F32Ge}}, "\x60"_su8);
  OK(Read<I>, I{At{"\x61"_su8, O::F64Eq}}, "\x61"_su8);
  OK(Read<I>, I{At{"\x62"_su8, O::F64Ne}}, "\x62"_su8);
  OK(Read<I>, I{At{"\x63"_su8, O::F64Lt}}, "\x63"_su8);
  OK(Read<I>, I{At{"\x64"_su8, O::F64Gt}}, "\x64"_su8);
  OK(Read<I>, I{At{"\x65"_su8, O::F64Le}}, "\x65"_su8);
  OK(Read<I>, I{At{"\x66"_su8, O::F64Ge}}, "\x66"_su8);
  OK(Read<I>, I{At{"\x67"_su8, O::I32Clz}}, "\x67"_su8);
  OK(Read<I>, I{At{"\x68"_su8, O::I32Ctz}}, "\x68"_su8);
  OK(Read<I>, I{At{"\x69"_su8, O::I32Popcnt}}, "\x69"_su8);
  OK(Read<I>, I{At{"\x6a"_su8, O::I32Add}}, "\x6a"_su8);
  OK(Read<I>, I{At{"\x6b"_su8, O::I32Sub}}, "\x6b"_su8);
  OK(Read<I>, I{At{"\x6c"_su8, O::I32Mul}}, "\x6c"_su8);
  OK(Read<I>, I{At{"\x6d"_su8, O::I32DivS}}, "\x6d"_su8);
  OK(Read<I>, I{At{"\x6e"_su8, O::I32DivU}}, "\x6e"_su8);
  OK(Read<I>, I{At{"\x6f"_su8, O::I32RemS}}, "\x6f"_su8);
  OK(Read<I>, I{At{"\x70"_su8, O::I32RemU}}, "\x70"_su8);
  OK(Read<I>, I{At{"\x71"_su8, O::I32And}}, "\x71"_su8);
  OK(Read<I>, I{At{"\x72"_su8, O::I32Or}}, "\x72"_su8);
  OK(Read<I>, I{At{"\x73"_su8, O::I32Xor}}, "\x73"_su8);
  OK(Read<I>, I{At{"\x74"_su8, O::I32Shl}}, "\x74"_su8);
  OK(Read<I>, I{At{"\x75"_su8, O::I32ShrS}}, "\x75"_su8);
  OK(Read<I>, I{At{"\x76"_su8, O::I32ShrU}}, "\x76"_su8);
  OK(Read<I>, I{At{"\x77"_su8, O::I32Rotl}}, "\x77"_su8);
  OK(Read<I>, I{At{"\x78"_su8, O::I32Rotr}}, "\x78"_su8);
  OK(Read<I>, I{At{"\x79"_su8, O::I64Clz}}, "\x79"_su8);
  OK(Read<I>, I{At{"\x7a"_su8, O::I64Ctz}}, "\x7a"_su8);
  OK(Read<I>, I{At{"\x7b"_su8, O::I64Popcnt}}, "\x7b"_su8);
  OK(Read<I>, I{At{"\x7c"_su8, O::I64Add}}, "\x7c"_su8);
  OK(Read<I>, I{At{"\x7d"_su8, O::I64Sub}}, "\x7d"_su8);
  OK(Read<I>, I{At{"\x7e"_su8, O::I64Mul}}, "\x7e"_su8);
  OK(Read<I>, I{At{"\x7f"_su8, O::I64DivS}}, "\x7f"_su8);
  OK(Read<I>, I{At{"\x80"_su8, O::I64DivU}}, "\x80"_su8);
  OK(Read<I>, I{At{"\x81"_su8, O::I64RemS}}, "\x81"_su8);
  OK(Read<I>, I{At{"\x82"_su8, O::I64RemU}}, "\x82"_su8);
  OK(Read<I>, I{At{"\x83"_su8, O::I64And}}, "\x83"_su8);
  OK(Read<I>, I{At{"\x84"_su8, O::I64Or}}, "\x84"_su8);
  OK(Read<I>, I{At{"\x85"_su8, O::I64Xor}}, "\x85"_su8);
  OK(Read<I>, I{At{"\x86"_su8, O::I64Shl}}, "\x86"_su8);
  OK(Read<I>, I{At{"\x87"_su8, O::I64ShrS}}, "\x87"_su8);
  OK(Read<I>, I{At{"\x88"_su8, O::I64ShrU}}, "\x88"_su8);
  OK(Read<I>, I{At{"\x89"_su8, O::I64Rotl}}, "\x89"_su8);
  OK(Read<I>, I{At{"\x8a"_su8, O::I64Rotr}}, "\x8a"_su8);
  OK(Read<I>, I{At{"\x8b"_su8, O::F32Abs}}, "\x8b"_su8);
  OK(Read<I>, I{At{"\x8c"_su8, O::F32Neg}}, "\x8c"_su8);
  OK(Read<I>, I{At{"\x8d"_su8, O::F32Ceil}}, "\x8d"_su8);
  OK(Read<I>, I{At{"\x8e"_su8, O::F32Floor}}, "\x8e"_su8);
  OK(Read<I>, I{At{"\x8f"_su8, O::F32Trunc}}, "\x8f"_su8);
  OK(Read<I>, I{At{"\x90"_su8, O::F32Nearest}}, "\x90"_su8);
  OK(Read<I>, I{At{"\x91"_su8, O::F32Sqrt}}, "\x91"_su8);
  OK(Read<I>, I{At{"\x92"_su8, O::F32Add}}, "\x92"_su8);
  OK(Read<I>, I{At{"\x93"_su8, O::F32Sub}}, "\x93"_su8);
  OK(Read<I>, I{At{"\x94"_su8, O::F32Mul}}, "\x94"_su8);
  OK(Read<I>, I{At{"\x95"_su8, O::F32Div}}, "\x95"_su8);
  OK(Read<I>, I{At{"\x96"_su8, O::F32Min}}, "\x96"_su8);
  OK(Read<I>, I{At{"\x97"_su8, O::F32Max}}, "\x97"_su8);
  OK(Read<I>, I{At{"\x98"_su8, O::F32Copysign}}, "\x98"_su8);
  OK(Read<I>, I{At{"\x99"_su8, O::F64Abs}}, "\x99"_su8);
  OK(Read<I>, I{At{"\x9a"_su8, O::F64Neg}}, "\x9a"_su8);
  OK(Read<I>, I{At{"\x9b"_su8, O::F64Ceil}}, "\x9b"_su8);
  OK(Read<I>, I{At{"\x9c"_su8, O::F64Floor}}, "\x9c"_su8);
  OK(Read<I>, I{At{"\x9d"_su8, O::F64Trunc}}, "\x9d"_su8);
  OK(Read<I>, I{At{"\x9e"_su8, O::F64Nearest}}, "\x9e"_su8);
  OK(Read<I>, I{At{"\x9f"_su8, O::F64Sqrt}}, "\x9f"_su8);
  OK(Read<I>, I{At{"\xa0"_su8, O::F64Add}}, "\xa0"_su8);
  OK(Read<I>, I{At{"\xa1"_su8, O::F64Sub}}, "\xa1"_su8);
  OK(Read<I>, I{At{"\xa2"_su8, O::F64Mul}}, "\xa2"_su8);
  OK(Read<I>, I{At{"\xa3"_su8, O::F64Div}}, "\xa3"_su8);
  OK(Read<I>, I{At{"\xa4"_su8, O::F64Min}}, "\xa4"_su8);
  OK(Read<I>, I{At{"\xa5"_su8, O::F64Max}}, "\xa5"_su8);
  OK(Read<I>, I{At{"\xa6"_su8, O::F64Copysign}}, "\xa6"_su8);
  OK(Read<I>, I{At{"\xa7"_su8, O::I32WrapI64}}, "\xa7"_su8);
  OK(Read<I>, I{At{"\xa8"_su8, O::I32TruncF32S}}, "\xa8"_su8);
  OK(Read<I>, I{At{"\xa9"_su8, O::I32TruncF32U}}, "\xa9"_su8);
  OK(Read<I>, I{At{"\xaa"_su8, O::I32TruncF64S}}, "\xaa"_su8);
  OK(Read<I>, I{At{"\xab"_su8, O::I32TruncF64U}}, "\xab"_su8);
  OK(Read<I>, I{At{"\xac"_su8, O::I64ExtendI32S}}, "\xac"_su8);
  OK(Read<I>, I{At{"\xad"_su8, O::I64ExtendI32U}}, "\xad"_su8);
  OK(Read<I>, I{At{"\xae"_su8, O::I64TruncF32S}}, "\xae"_su8);
  OK(Read<I>, I{At{"\xaf"_su8, O::I64TruncF32U}}, "\xaf"_su8);
  OK(Read<I>, I{At{"\xb0"_su8, O::I64TruncF64S}}, "\xb0"_su8);
  OK(Read<I>, I{At{"\xb1"_su8, O::I64TruncF64U}}, "\xb1"_su8);
  OK(Read<I>, I{At{"\xb2"_su8, O::F32ConvertI32S}}, "\xb2"_su8);
  OK(Read<I>, I{At{"\xb3"_su8, O::F32ConvertI32U}}, "\xb3"_su8);
  OK(Read<I>, I{At{"\xb4"_su8, O::F32ConvertI64S}}, "\xb4"_su8);
  OK(Read<I>, I{At{"\xb5"_su8, O::F32ConvertI64U}}, "\xb5"_su8);
  OK(Read<I>, I{At{"\xb6"_su8, O::F32DemoteF64}}, "\xb6"_su8);
  OK(Read<I>, I{At{"\xb7"_su8, O::F64ConvertI32S}}, "\xb7"_su8);
  OK(Read<I>, I{At{"\xb8"_su8, O::F64ConvertI32U}}, "\xb8"_su8);
  OK(Read<I>, I{At{"\xb9"_su8, O::F64ConvertI64S}}, "\xb9"_su8);
  OK(Read<I>, I{At{"\xba"_su8, O::F64ConvertI64U}}, "\xba"_su8);
  OK(Read<I>, I{At{"\xbb"_su8, O::F64PromoteF32}}, "\xbb"_su8);
  OK(Read<I>, I{At{"\xbc"_su8, O::I32ReinterpretF32}}, "\xbc"_su8);
  OK(Read<I>, I{At{"\xbd"_su8, O::I64ReinterpretF64}}, "\xbd"_su8);
  OK(Read<I>, I{At{"\xbe"_su8, O::F32ReinterpretI32}}, "\xbe"_su8);
  OK(Read<I>, I{At{"\xbf"_su8, O::F64ReinterpretI64}}, "\xbf"_su8);
}

TEST_F(BinaryReadTest, Instruction_BadMemoryReserved) {
  Fail(Read<Instruction>,
       {{1, "reserved"}, {1, "Expected reserved byte 0, got 1"}},
       "\x3f\x01"_su8);
  Fail(Read<Instruction>,
       {{1, "reserved"}, {1, "Expected reserved byte 0, got 1"}},
       "\x40\x01"_su8);
}

TEST_F(BinaryReadTest, Instruction_multi_memory) {
  ctx.features.enable_multi_memory();

  OK(Read<I>,
     I{At{"\x3f"_su8, O::MemorySize},
       At{"\x01"_su8, MemOptImmediate{At{"\x01"_su8, Index{1}}}}},
     "\x3f\x01"_su8);
  OK(Read<I>,
     I{At{"\x40"_su8, O::MemoryGrow},
       At{"\x01"_su8, MemOptImmediate{At{"\x01"_su8, Index{1}}}}},
     "\x40\x01"_su8);
}

TEST_F(BinaryReadTest, InstructionList_BlockEnd) {
  OK(Read<InstructionList>,
     InstructionList{
         At{"\x02\x40"_su8,
            I{At{"\x02"_su8, O::Block}, At{"\x40"_su8, BT_Void}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
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
         At{"\x03\x40"_su8,
            I{At{"\x03"_su8, O::Loop}, At{"\x40"_su8, BT_Void}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
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
         At{"\x04\x40"_su8, I{At{"\x04"_su8, O::If}, At{"\x40"_su8, BT_Void}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x04\x40\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_IfElseEnd) {
  OK(Read<InstructionList>,
     InstructionList{
         At{"\x04\x40"_su8, I{At{"\x04"_su8, O::If}, At{"\x40"_su8, BT_Void}}},
         At{"\x05"_su8, I{At{"\x05"_su8, O::Else}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x04\x40\x05\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_IfNoEnd) {
  Fail(Read<InstructionList>, {{3, "opcode"}, {3, "Unable to read u8"}},
       "\x04\x40\x0b"_su8);
}

TEST_F(BinaryReadTest, Instruction_exceptions) {
  ctx.features.enable_exceptions();

  OK(Read<I>, I{At{"\x06"_su8, O::Try}, At{"\x40"_su8, BT_Void}},
     "\x06\x40"_su8);
  OK(Read<I>, I{At{"\x07"_su8, O::Catch}, At{"\x00"_su8, Index{0}}},
     "\x07\x00"_su8);
  OK(Read<I>, I{At{"\x08"_su8, O::Throw}, At{"\x00"_su8, Index{0}}},
     "\x08\x00"_su8);
  OK(Read<I>, I{At{"\x09"_su8, O::Rethrow}, At{"\x00"_su8, Index{0}}},
     "\x09\x00"_su8);
  OK(Read<I>, I{At{"\x19"_su8, O::CatchAll}}, "\x19"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryCatchEnd) {
  ctx.features.enable_exceptions();

  OK(Read<InstructionList>,
     InstructionList{
         At{"\x06\x40"_su8, I{At{"\x06"_su8, O::Try}, At{"\x40"_su8, BT_Void}}},
         At{"\x07\x00"_su8,
            I{At{"\x07"_su8, O::Catch}, At{"\x00"_su8, Index{0}}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x06\x40\x07\x00\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryCatchAll) {
  ctx.features.enable_exceptions();

  // Just CatchAll
  OK(Read<InstructionList>,
     InstructionList{
         At{"\x06\x40"_su8, I{At{"\x06"_su8, O::Try}, At{"\x40"_su8, BT_Void}}},
         At{"\x19"_su8, I{At{"\x19"_su8, O::CatchAll}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x06\x40\x19\x0b\x0b"_su8);

  // Catch then CatchAll
  OK(Read<InstructionList>,
     InstructionList{
         At{"\x06\x40"_su8, I{At{"\x06"_su8, O::Try}, At{"\x40"_su8, BT_Void}}},
         At{"\x07\x00"_su8,
            I{At{"\x07"_su8, O::Catch}, At{"\x00"_su8, Index{0}}}},
         At{"\x19"_su8, I{At{"\x19"_su8, O::CatchAll}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x06\x40\x07\x00\x19\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryDelegate) {
  ctx.features.enable_exceptions();

  OK(Read<InstructionList>,
     InstructionList{
         At{"\x06\x40"_su8, I{At{"\x06"_su8, O::Try}, At{"\x40"_su8, BT_Void}}},
         At{"\x18\x00"_su8,
            I{At{"\x18"_su8, O::Delegate}, At{"\x00"_su8, Index{0}}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x06\x40\x18\x00\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_TryNoCatch) {
  ctx.features.enable_exceptions();

  Fail(Read<InstructionList>,
       {{2, "Expected catch or delegate instruction in try block"}},
       "\x06\x40\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_CatchDelegate_OutOfOrder) {
  ctx.features.enable_exceptions();

  // Catch then delegate
  Fail(Read<InstructionList>, {{4, "Unexpected delegate instruction"}},
       "\x06\x40\x07\x00\x18\x00\x0b\x0b"_su8);

  // Delegate then catch
  Fail(Read<InstructionList>, {{4, "Unexpected catch instruction"}},
       "\x06\x40\x18\x00\x07\x00\x0b\x0b"_su8);

  // Delegate then catch_all
  Fail(Read<InstructionList>, {{4, "Unexpected catch_all instruction"}},
       "\x06\x40\x18\x00\x19\x0b\x0b"_su8);
}


TEST_F(BinaryReadTest, InstructionList_TryNoEnd) {
  ctx.features.enable_exceptions();

  Fail(Read<InstructionList>, {{4, "opcode"}, {4, "Unable to read u8"}},
       "\x06\x40\07\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_LetEnd) {
  ctx.features.enable_function_references();

  OK(Read<InstructionList>,
     InstructionList{
         At{"\x17\x40\x00"_su8,
            I{At{"\x17"_su8, O::Let},
              At{"\x40\x00"_su8, LetImmediate{At{"\x40"_su8, BT_Void},
                                              At{"\x00"_su8, LocalsList{}}}}}},
         At{"\x0b"_su8, I{At{"\x0b"_su8, O::End}}},
     },
     "\x17\x40\x00\x0b\x0b"_su8);
}

TEST_F(BinaryReadTest, InstructionList_LetNoEnd) {
  ctx.features.enable_function_references();

  Fail(Read<InstructionList>, {{4, "opcode"}, {4, "Unable to read u8"}},
       "\x17\x40\x00\x0b"_su8);
}

TEST_F(BinaryReadTest, Instruction_tail_call) {
  ctx.features.enable_tail_call();

  OK(Read<I>, I{At{"\x12"_su8, O::ReturnCall}, At{"\x00"_su8, Index{0}}},
     "\x12\x00"_su8);
  OK(Read<I>,
     I{At{"\x13"_su8, O::ReturnCallIndirect},
       At{"\x08\x00"_su8, CallIndirectImmediate{At{"\x08"_su8, Index{8}},
                                                At{"\x00"_su8, Index{0}}}}},
     "\x13\x08\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_sign_extension) {
  ctx.features.enable_sign_extension();

  OK(Read<I>, I{At{"\xc0"_su8, O::I32Extend8S}}, "\xc0"_su8);
  OK(Read<I>, I{At{"\xc1"_su8, O::I32Extend16S}}, "\xc1"_su8);
  OK(Read<I>, I{At{"\xc2"_su8, O::I64Extend8S}}, "\xc2"_su8);
  OK(Read<I>, I{At{"\xc3"_su8, O::I64Extend16S}}, "\xc3"_su8);
  OK(Read<I>, I{At{"\xc4"_su8, O::I64Extend32S}}, "\xc4"_su8);
}

TEST_F(BinaryReadTest, Instruction_reference_types) {
  ctx.features.enable_reference_types();

  OK(Read<I>,
     I{At{"\x1c"_su8, O::SelectT},
       At{"\x02\x7f\x7e"_su8,
          ValueTypeList{At{"\x7f"_su8, VT_I32}, At{"\x7e"_su8, VT_I64}}}},
     "\x1c\x02\x7f\x7e"_su8);
  OK(Read<I>, I{At{"\x25"_su8, O::TableGet}, At{"\x00"_su8, Index{0}}},
     "\x25\x00"_su8);
  OK(Read<I>, I{At{"\x26"_su8, O::TableSet}, At{"\x00"_su8, Index{0}}},
     "\x26\x00"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0c"_su8, O::TableInit},
       At{"\x00\x01"_su8,
          InitImmediate{At{"\x00"_su8, Index{0}}, At{"\x01"_su8, Index{1}}}}},
     "\xfc\x0c\x00\x01"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0e"_su8, O::TableCopy},
       At{"\x00\x01"_su8,
          CopyImmediate{At{"\x00"_su8, Index{0}}, At{"\x01"_su8, Index{1}}}}},
     "\xfc\x0e\x00\x01"_su8);
  OK(Read<I>, I{At{"\xfc\x0f"_su8, O::TableGrow}, At{"\x00"_su8, Index{0}}},
     "\xfc\x0f\x00"_su8);
  OK(Read<I>, I{At{"\xfc\x10"_su8, O::TableSize}, At{"\x00"_su8, Index{0}}},
     "\xfc\x10\x00"_su8);
  OK(Read<I>, I{At{"\xfc\x11"_su8, O::TableFill}, At{"\x00"_su8, Index{0}}},
     "\xfc\x11\x00"_su8);
  OK(Read<I>, I{At{"\xd0"_su8, O::RefNull}, At{"\x70"_su8, HT_Func}},
     "\xd0\x70"_su8);
  OK(Read<I>, I{At{"\xd1"_su8, O::RefIsNull}}, "\xd1"_su8);
  OK(Read<I>, I{At{"\xd2"_su8, O::RefFunc}, At{"\x00"_su8, Index{0}}},
     "\xd2\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_saturating_float_to_int) {
  ctx.features.enable_saturating_float_to_int();

  OK(Read<I>, I{At{"\xfc\x00"_su8, O::I32TruncSatF32S}}, "\xfc\x00"_su8);
  OK(Read<I>, I{At{"\xfc\x01"_su8, O::I32TruncSatF32U}}, "\xfc\x01"_su8);
  OK(Read<I>, I{At{"\xfc\x02"_su8, O::I32TruncSatF64S}}, "\xfc\x02"_su8);
  OK(Read<I>, I{At{"\xfc\x03"_su8, O::I32TruncSatF64U}}, "\xfc\x03"_su8);
  OK(Read<I>, I{At{"\xfc\x04"_su8, O::I64TruncSatF32S}}, "\xfc\x04"_su8);
  OK(Read<I>, I{At{"\xfc\x05"_su8, O::I64TruncSatF32U}}, "\xfc\x05"_su8);
  OK(Read<I>, I{At{"\xfc\x06"_su8, O::I64TruncSatF64S}}, "\xfc\x06"_su8);
  OK(Read<I>, I{At{"\xfc\x07"_su8, O::I64TruncSatF64U}}, "\xfc\x07"_su8);
}

TEST_F(BinaryReadTest, Instruction_bulk_memory) {
  ctx.features.enable_bulk_memory();
  ctx.declared_data_count = 1;

  OK(Read<I>,
     I{At{"\xfc\x08"_su8, O::MemoryInit},
       At{"\x01\x00"_su8,
          InitImmediate{At{"\x01"_su8, Index{1}}, At{"\x00"_su8, Index{0}}}}},
     "\xfc\x08\x01\x00"_su8);
  OK(Read<I>, I{At{"\xfc\x09"_su8, O::DataDrop}, At{"\x02"_su8, Index{2}}},
     "\xfc\x09\x02"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0a"_su8, O::MemoryCopy},
       At{"\x00\x00"_su8,
          CopyImmediate{At{"\x00"_su8, Index{0}}, At{"\x00"_su8, Index{0}}}}},
     "\xfc\x0a\x00\x00"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0b"_su8, O::MemoryFill},
       At{"\x00"_su8, MemOptImmediate{At{"\x00"_su8, Index{0}}}}},
     "\xfc\x0b\x00"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0c"_su8, O::TableInit},
       At{"\x03\x00"_su8,
          InitImmediate{At{"\x03"_su8, Index{3}}, At{"\x00"_su8, Index{0}}}}},
     "\xfc\x0c\x03\x00"_su8);
  OK(Read<I>, I{At{"\xfc\x0d"_su8, O::ElemDrop}, At{"\x04"_su8, Index{4}}},
     "\xfc\x0d\x04"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0e"_su8, O::TableCopy},
       At{"\x00\x00"_su8,
          CopyImmediate{At{"\x00"_su8, Index{0}}, At{"\x00"_su8, Index{0}}}}},
     "\xfc\x0e\x00\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_BadReserved_bulk_memory) {
  ctx.features.enable_bulk_memory();

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

TEST_F(BinaryReadTest, Instruction_multi_memory_bulk_memory) {
  ctx.features.enable_bulk_memory();
  ctx.features.enable_multi_memory();
  ctx.declared_data_count = 1;

  OK(Read<I>,
     I{At{"\xfc\x08"_su8, O::MemoryInit},
       At{"\x01\x02"_su8,
          InitImmediate{At{"\x01"_su8, Index{1}}, At{"\x02"_su8, Index{2}}}}},
     "\xfc\x08\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0a"_su8, O::MemoryCopy},
       At{"\x01\x02"_su8,
          CopyImmediate{At{"\x01"_su8, Index{1}}, At{"\x02"_su8, Index{2}}}}},
     "\xfc\x0a\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfc\x0b"_su8, O::MemoryFill},
       At{"\x01"_su8, MemOptImmediate{At{"\x01"_su8, Index{1}}}}},
     "\xfc\x0b\x01"_su8);
}

TEST_F(BinaryReadTest, Instruction_NoDataCount_bulk_memory) {
  ctx.features.enable_bulk_memory();

  Fail(Read<I>, {{0, "memory.init instruction requires a data count section"}},
       "\xfc\x08\x01\x00"_su8);
  Fail(Read<I>, {{0, "data.drop instruction requires a data count section"}},
       "\xfc\x09\x02"_su8);
}

TEST_F(BinaryReadTest, Instruction_simd) {
  ctx.features.enable_simd();

  auto memarg = At{"\x01\x02"_su8, MemArgImmediate{At{"\x01"_su8, u32{1}},
                                                   At{"\x02"_su8, u32{2}}}};
  auto lane = At{"\x00"_su8, u8{0}};
  auto memory_lane = At{"\x01\x02\x00"_su8, SimdMemoryLaneImmediate{memarg, lane}};

  OK(Read<I>, I{At{"\xfd\x00"_su8, O::V128Load}, memarg},
     "\xfd\x00\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x01"_su8, O::V128Load8X8S}, memarg},
     "\xfd\x01\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x02"_su8, O::V128Load8X8U}, memarg},
     "\xfd\x02\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x03"_su8, O::V128Load16X4S}, memarg},
     "\xfd\x03\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x04"_su8, O::V128Load16X4U}, memarg},
     "\xfd\x04\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x05"_su8, O::V128Load32X2S}, memarg},
     "\xfd\x05\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x06"_su8, O::V128Load32X2U}, memarg},
     "\xfd\x06\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x07"_su8, O::V128Load8Splat}, memarg},
     "\xfd\x07\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x08"_su8, O::V128Load16Splat}, memarg},
     "\xfd\x08\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x09"_su8, O::V128Load32Splat}, memarg},
     "\xfd\x09\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x0a"_su8, O::V128Load64Splat}, memarg},
     "\xfd\x0a\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x0b"_su8, O::V128Store}, memarg},
     "\xfd\x0b\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfd\x0c"_su8, O::V128Const},
       At{"\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00"_su8,
          v128{u64{5}, u64{6}}}},
     "\xfd\x0c\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<I>,
     I{At{"\xfd\x0d"_su8, O::I8X16Shuffle},
       At{"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8,
          ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}}},
     "\xfd\x0d\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x0e"_su8, O::I8X16Swizzle}}, "\xfd\x0e"_su8);
  OK(Read<I>, I{At{"\xfd\x0f"_su8, O::I8X16Splat}}, "\xfd\x0f"_su8);
  OK(Read<I>, I{At{"\xfd\x10"_su8, O::I16X8Splat}}, "\xfd\x10"_su8);
  OK(Read<I>, I{At{"\xfd\x11"_su8, O::I32X4Splat}}, "\xfd\x11"_su8);
  OK(Read<I>, I{At{"\xfd\x12"_su8, O::I64X2Splat}}, "\xfd\x12"_su8);
  OK(Read<I>, I{At{"\xfd\x13"_su8, O::F32X4Splat}}, "\xfd\x13"_su8);
  OK(Read<I>, I{At{"\xfd\x14"_su8, O::F64X2Splat}}, "\xfd\x14"_su8);
  OK(Read<I>,
     I{At{"\xfd\x15"_su8, O::I8X16ExtractLaneS}, At{"\x00"_su8, u8{0}}},
     "\xfd\x15\x00"_su8);
  OK(Read<I>,
     I{At{"\xfd\x16"_su8, O::I8X16ExtractLaneU}, At{"\x00"_su8, u8{0}}},
     "\xfd\x16\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x17"_su8, O::I8X16ReplaceLane}, At{"\x00"_su8, u8{0}}},
     "\xfd\x17\x00"_su8);
  OK(Read<I>,
     I{At{"\xfd\x18"_su8, O::I16X8ExtractLaneS}, At{"\x00"_su8, u8{0}}},
     "\xfd\x18\x00"_su8);
  OK(Read<I>,
     I{At{"\xfd\x19"_su8, O::I16X8ExtractLaneU}, At{"\x00"_su8, u8{0}}},
     "\xfd\x19\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x1a"_su8, O::I16X8ReplaceLane}, lane},
     "\xfd\x1a\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x1b"_su8, O::I32X4ExtractLane}, lane},
     "\xfd\x1b\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x1c"_su8, O::I32X4ReplaceLane}, lane},
     "\xfd\x1c\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x1d"_su8, O::I64X2ExtractLane}, lane},
     "\xfd\x1d\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x1e"_su8, O::I64X2ReplaceLane}, lane},
     "\xfd\x1e\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x1f"_su8, O::F32X4ExtractLane}, lane},
     "\xfd\x1f\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x20"_su8, O::F32X4ReplaceLane}, lane},
     "\xfd\x20\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x21"_su8, O::F64X2ExtractLane}, lane},
     "\xfd\x21\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x22"_su8, O::F64X2ReplaceLane}, lane},
     "\xfd\x22\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x23"_su8, O::I8X16Eq}}, "\xfd\x23"_su8);
  OK(Read<I>, I{At{"\xfd\x24"_su8, O::I8X16Ne}}, "\xfd\x24"_su8);
  OK(Read<I>, I{At{"\xfd\x25"_su8, O::I8X16LtS}}, "\xfd\x25"_su8);
  OK(Read<I>, I{At{"\xfd\x26"_su8, O::I8X16LtU}}, "\xfd\x26"_su8);
  OK(Read<I>, I{At{"\xfd\x27"_su8, O::I8X16GtS}}, "\xfd\x27"_su8);
  OK(Read<I>, I{At{"\xfd\x28"_su8, O::I8X16GtU}}, "\xfd\x28"_su8);
  OK(Read<I>, I{At{"\xfd\x29"_su8, O::I8X16LeS}}, "\xfd\x29"_su8);
  OK(Read<I>, I{At{"\xfd\x2a"_su8, O::I8X16LeU}}, "\xfd\x2a"_su8);
  OK(Read<I>, I{At{"\xfd\x2b"_su8, O::I8X16GeS}}, "\xfd\x2b"_su8);
  OK(Read<I>, I{At{"\xfd\x2c"_su8, O::I8X16GeU}}, "\xfd\x2c"_su8);
  OK(Read<I>, I{At{"\xfd\x2d"_su8, O::I16X8Eq}}, "\xfd\x2d"_su8);
  OK(Read<I>, I{At{"\xfd\x2e"_su8, O::I16X8Ne}}, "\xfd\x2e"_su8);
  OK(Read<I>, I{At{"\xfd\x2f"_su8, O::I16X8LtS}}, "\xfd\x2f"_su8);
  OK(Read<I>, I{At{"\xfd\x30"_su8, O::I16X8LtU}}, "\xfd\x30"_su8);
  OK(Read<I>, I{At{"\xfd\x31"_su8, O::I16X8GtS}}, "\xfd\x31"_su8);
  OK(Read<I>, I{At{"\xfd\x32"_su8, O::I16X8GtU}}, "\xfd\x32"_su8);
  OK(Read<I>, I{At{"\xfd\x33"_su8, O::I16X8LeS}}, "\xfd\x33"_su8);
  OK(Read<I>, I{At{"\xfd\x34"_su8, O::I16X8LeU}}, "\xfd\x34"_su8);
  OK(Read<I>, I{At{"\xfd\x35"_su8, O::I16X8GeS}}, "\xfd\x35"_su8);
  OK(Read<I>, I{At{"\xfd\x36"_su8, O::I16X8GeU}}, "\xfd\x36"_su8);
  OK(Read<I>, I{At{"\xfd\x37"_su8, O::I32X4Eq}}, "\xfd\x37"_su8);
  OK(Read<I>, I{At{"\xfd\x38"_su8, O::I32X4Ne}}, "\xfd\x38"_su8);
  OK(Read<I>, I{At{"\xfd\x39"_su8, O::I32X4LtS}}, "\xfd\x39"_su8);
  OK(Read<I>, I{At{"\xfd\x3a"_su8, O::I32X4LtU}}, "\xfd\x3a"_su8);
  OK(Read<I>, I{At{"\xfd\x3b"_su8, O::I32X4GtS}}, "\xfd\x3b"_su8);
  OK(Read<I>, I{At{"\xfd\x3c"_su8, O::I32X4GtU}}, "\xfd\x3c"_su8);
  OK(Read<I>, I{At{"\xfd\x3d"_su8, O::I32X4LeS}}, "\xfd\x3d"_su8);
  OK(Read<I>, I{At{"\xfd\x3e"_su8, O::I32X4LeU}}, "\xfd\x3e"_su8);
  OK(Read<I>, I{At{"\xfd\x3f"_su8, O::I32X4GeS}}, "\xfd\x3f"_su8);
  OK(Read<I>, I{At{"\xfd\x40"_su8, O::I32X4GeU}}, "\xfd\x40"_su8);
  OK(Read<I>, I{At{"\xfd\x41"_su8, O::F32X4Eq}}, "\xfd\x41"_su8);
  OK(Read<I>, I{At{"\xfd\x42"_su8, O::F32X4Ne}}, "\xfd\x42"_su8);
  OK(Read<I>, I{At{"\xfd\x43"_su8, O::F32X4Lt}}, "\xfd\x43"_su8);
  OK(Read<I>, I{At{"\xfd\x44"_su8, O::F32X4Gt}}, "\xfd\x44"_su8);
  OK(Read<I>, I{At{"\xfd\x45"_su8, O::F32X4Le}}, "\xfd\x45"_su8);
  OK(Read<I>, I{At{"\xfd\x46"_su8, O::F32X4Ge}}, "\xfd\x46"_su8);
  OK(Read<I>, I{At{"\xfd\x47"_su8, O::F64X2Eq}}, "\xfd\x47"_su8);
  OK(Read<I>, I{At{"\xfd\x48"_su8, O::F64X2Ne}}, "\xfd\x48"_su8);
  OK(Read<I>, I{At{"\xfd\x49"_su8, O::F64X2Lt}}, "\xfd\x49"_su8);
  OK(Read<I>, I{At{"\xfd\x4a"_su8, O::F64X2Gt}}, "\xfd\x4a"_su8);
  OK(Read<I>, I{At{"\xfd\x4b"_su8, O::F64X2Le}}, "\xfd\x4b"_su8);
  OK(Read<I>, I{At{"\xfd\x4c"_su8, O::F64X2Ge}}, "\xfd\x4c"_su8);
  OK(Read<I>, I{At{"\xfd\x4d"_su8, O::V128Not}}, "\xfd\x4d"_su8);
  OK(Read<I>, I{At{"\xfd\x4e"_su8, O::V128And}}, "\xfd\x4e"_su8);
  OK(Read<I>, I{At{"\xfd\x4f"_su8, O::V128Andnot}}, "\xfd\x4f"_su8);
  OK(Read<I>, I{At{"\xfd\x50"_su8, O::V128Or}}, "\xfd\x50"_su8);
  OK(Read<I>, I{At{"\xfd\x51"_su8, O::V128Xor}}, "\xfd\x51"_su8);
  OK(Read<I>, I{At{"\xfd\x52"_su8, O::V128BitSelect}}, "\xfd\x52"_su8);
  OK(Read<I>, I{At{"\xfd\x53"_su8, O::V128AnyTrue}}, "\xfd\x53"_su8);
  OK(Read<I>, I{At{"\xfd\x54"_su8, O::V128Load8Lane}, memory_lane}, "\xfd\x54\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x55"_su8, O::V128Load16Lane}, memory_lane}, "\xfd\x55\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x56"_su8, O::V128Load32Lane}, memory_lane}, "\xfd\x56\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x57"_su8, O::V128Load64Lane}, memory_lane}, "\xfd\x57\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x58"_su8, O::V128Store8Lane}, memory_lane}, "\xfd\x58\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x59"_su8, O::V128Store16Lane}, memory_lane}, "\xfd\x59\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x5a"_su8, O::V128Store32Lane}, memory_lane}, "\xfd\x5a\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x5b"_su8, O::V128Store64Lane}, memory_lane}, "\xfd\x5b\x01\x02\x00"_su8);
  OK(Read<I>, I{At{"\xfd\x5c"_su8, O::V128Load32Zero}, memarg}, "\xfd\x5c\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x5d"_su8, O::V128Load64Zero}, memarg}, "\xfd\x5d\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfd\x5e"_su8, O::F32X4DemoteF64X2Zero}}, "\xfd\x5e"_su8);
  OK(Read<I>, I{At{"\xfd\x5f"_su8, O::F64X2PromoteLowF32X4}}, "\xfd\x5f"_su8);
  OK(Read<I>, I{At{"\xfd\x60"_su8, O::I8X16Abs}}, "\xfd\x60"_su8);
  OK(Read<I>, I{At{"\xfd\x61"_su8, O::I8X16Neg}}, "\xfd\x61"_su8);
  OK(Read<I>, I{At{"\xfd\x62"_su8, O::I8X16Popcnt}}, "\xfd\x62"_su8);
  OK(Read<I>, I{At{"\xfd\x63"_su8, O::I8X16AllTrue}}, "\xfd\x63"_su8);
  OK(Read<I>, I{At{"\xfd\x64"_su8, O::I8X16Bitmask}}, "\xfd\x64"_su8);
  OK(Read<I>, I{At{"\xfd\x65"_su8, O::I8X16NarrowI16X8S}}, "\xfd\x65"_su8);
  OK(Read<I>, I{At{"\xfd\x66"_su8, O::I8X16NarrowI16X8U}}, "\xfd\x66"_su8);
  OK(Read<I>, I{At{"\xfd\x67"_su8, O::F32X4Ceil}}, "\xfd\x67"_su8);
  OK(Read<I>, I{At{"\xfd\x68"_su8, O::F32X4Floor}}, "\xfd\x68"_su8);
  OK(Read<I>, I{At{"\xfd\x69"_su8, O::F32X4Trunc}}, "\xfd\x69"_su8);
  OK(Read<I>, I{At{"\xfd\x6a"_su8, O::F32X4Nearest}}, "\xfd\x6a"_su8);
  OK(Read<I>, I{At{"\xfd\x6b"_su8, O::I8X16Shl}}, "\xfd\x6b"_su8);
  OK(Read<I>, I{At{"\xfd\x6c"_su8, O::I8X16ShrS}}, "\xfd\x6c"_su8);
  OK(Read<I>, I{At{"\xfd\x6d"_su8, O::I8X16ShrU}}, "\xfd\x6d"_su8);
  OK(Read<I>, I{At{"\xfd\x6e"_su8, O::I8X16Add}}, "\xfd\x6e"_su8);
  OK(Read<I>, I{At{"\xfd\x6f"_su8, O::I8X16AddSatS}}, "\xfd\x6f"_su8);
  OK(Read<I>, I{At{"\xfd\x70"_su8, O::I8X16AddSatU}}, "\xfd\x70"_su8);
  OK(Read<I>, I{At{"\xfd\x71"_su8, O::I8X16Sub}}, "\xfd\x71"_su8);
  OK(Read<I>, I{At{"\xfd\x72"_su8, O::I8X16SubSatS}}, "\xfd\x72"_su8);
  OK(Read<I>, I{At{"\xfd\x73"_su8, O::I8X16SubSatU}}, "\xfd\x73"_su8);
  OK(Read<I>, I{At{"\xfd\x74"_su8, O::F64X2Ceil}}, "\xfd\x74"_su8);
  OK(Read<I>, I{At{"\xfd\x75"_su8, O::F64X2Floor}}, "\xfd\x75"_su8);
  OK(Read<I>, I{At{"\xfd\x76"_su8, O::I8X16MinS}}, "\xfd\x76"_su8);
  OK(Read<I>, I{At{"\xfd\x77"_su8, O::I8X16MinU}}, "\xfd\x77"_su8);
  OK(Read<I>, I{At{"\xfd\x78"_su8, O::I8X16MaxS}}, "\xfd\x78"_su8);
  OK(Read<I>, I{At{"\xfd\x79"_su8, O::I8X16MaxU}}, "\xfd\x79"_su8);
  OK(Read<I>, I{At{"\xfd\x7a"_su8, O::F64X2Trunc}}, "\xfd\x7a"_su8);
  OK(Read<I>, I{At{"\xfd\x7b"_su8, O::I8X16AvgrU}}, "\xfd\x7b"_su8);
  OK(Read<I>, I{At{"\xfd\x7c"_su8, O::I16X8ExtaddPairwiseI8X16S}}, "\xfd\x7c"_su8);
  OK(Read<I>, I{At{"\xfd\x7d"_su8, O::I16X8ExtaddPairwiseI8X16U}}, "\xfd\x7d"_su8);
  OK(Read<I>, I{At{"\xfd\x7e"_su8, O::I32X4ExtaddPairwiseI16X8S}}, "\xfd\x7e"_su8);
  OK(Read<I>, I{At{"\xfd\x7f"_su8, O::I32X4ExtaddPairwiseI16X8U}}, "\xfd\x7f"_su8);
  OK(Read<I>, I{At{"\xfd\x80\x01"_su8, O::I16X8Abs}}, "\xfd\x80\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x81\x01"_su8, O::I16X8Neg}}, "\xfd\x81\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x82\x01"_su8, O::I16X8Q15mulrSatS}}, "\xfd\x82\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x83\x01"_su8, O::I16X8AllTrue}}, "\xfd\x83\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x84\x01"_su8, O::I16X8Bitmask}}, "\xfd\x84\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x85\x01"_su8, O::I16X8NarrowI32X4S}}, "\xfd\x85\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x86\x01"_su8, O::I16X8NarrowI32X4U}}, "\xfd\x86\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x87\x01"_su8, O::I16X8ExtendLowI8X16S}}, "\xfd\x87\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x88\x01"_su8, O::I16X8ExtendHighI8X16S}}, "\xfd\x88\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x89\x01"_su8, O::I16X8ExtendLowI8X16U}}, "\xfd\x89\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x8a\x01"_su8, O::I16X8ExtendHighI8X16U}}, "\xfd\x8a\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x8b\x01"_su8, O::I16X8Shl}}, "\xfd\x8b\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x8c\x01"_su8, O::I16X8ShrS}}, "\xfd\x8c\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x8d\x01"_su8, O::I16X8ShrU}}, "\xfd\x8d\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x8e\x01"_su8, O::I16X8Add}}, "\xfd\x8e\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x8f\x01"_su8, O::I16X8AddSatS}}, "\xfd\x8f\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x90\x01"_su8, O::I16X8AddSatU}}, "\xfd\x90\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x91\x01"_su8, O::I16X8Sub}}, "\xfd\x91\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x92\x01"_su8, O::I16X8SubSatS}}, "\xfd\x92\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x93\x01"_su8, O::I16X8SubSatU}}, "\xfd\x93\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x94\x01"_su8, O::F64X2Nearest}}, "\xfd\x94\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x95\x01"_su8, O::I16X8Mul}}, "\xfd\x95\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x96\x01"_su8, O::I16X8MinS}}, "\xfd\x96\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x97\x01"_su8, O::I16X8MinU}}, "\xfd\x97\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x98\x01"_su8, O::I16X8MaxS}}, "\xfd\x98\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x99\x01"_su8, O::I16X8MaxU}}, "\xfd\x99\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x9b\x01"_su8, O::I16X8AvgrU}}, "\xfd\x9b\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x9c\x01"_su8, O::I16X8ExtmulLowI8X16S}}, "\xfd\x9c\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x9d\x01"_su8, O::I16X8ExtmulHighI8X16S}}, "\xfd\x9d\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x9e\x01"_su8, O::I16X8ExtmulLowI8X16U}}, "\xfd\x9e\x01"_su8);
  OK(Read<I>, I{At{"\xfd\x9f\x01"_su8, O::I16X8ExtmulHighI8X16U}}, "\xfd\x9f\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa0\x01"_su8, O::I32X4Abs}}, "\xfd\xa0\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa1\x01"_su8, O::I32X4Neg}}, "\xfd\xa1\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa3\x01"_su8, O::I32X4AllTrue}}, "\xfd\xa3\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa4\x01"_su8, O::I32X4Bitmask}}, "\xfd\xa4\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa7\x01"_su8, O::I32X4ExtendLowI16X8S}}, "\xfd\xa7\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa8\x01"_su8, O::I32X4ExtendHighI16X8S}}, "\xfd\xa8\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xa9\x01"_su8, O::I32X4ExtendLowI16X8U}}, "\xfd\xa9\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xaa\x01"_su8, O::I32X4ExtendHighI16X8U}}, "\xfd\xaa\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xab\x01"_su8, O::I32X4Shl}}, "\xfd\xab\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xac\x01"_su8, O::I32X4ShrS}}, "\xfd\xac\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xad\x01"_su8, O::I32X4ShrU}}, "\xfd\xad\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xae\x01"_su8, O::I32X4Add}}, "\xfd\xae\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xb1\x01"_su8, O::I32X4Sub}}, "\xfd\xb1\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xb5\x01"_su8, O::I32X4Mul}}, "\xfd\xb5\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xb6\x01"_su8, O::I32X4MinS}}, "\xfd\xb6\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xb7\x01"_su8, O::I32X4MinU}}, "\xfd\xb7\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xb8\x01"_su8, O::I32X4MaxS}}, "\xfd\xb8\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xb9\x01"_su8, O::I32X4MaxU}}, "\xfd\xb9\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xba\x01"_su8, O::I32X4DotI16X8S}}, "\xfd\xba\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xbc\x01"_su8, O::I32X4ExtmulLowI16X8S}}, "\xfd\xbc\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xbd\x01"_su8, O::I32X4ExtmulHighI16X8S}}, "\xfd\xbd\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xbe\x01"_su8, O::I32X4ExtmulLowI16X8U}}, "\xfd\xbe\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xbf\x01"_su8, O::I32X4ExtmulHighI16X8U}}, "\xfd\xbf\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc0\x01"_su8, O::I64X2Abs}}, "\xfd\xc0\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc1\x01"_su8, O::I64X2Neg}}, "\xfd\xc1\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc3\x01"_su8, O::I64X2AllTrue}}, "\xfd\xc3\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc4\x01"_su8, O::I64X2Bitmask}}, "\xfd\xc4\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc7\x01"_su8, O::I64X2ExtendLowI32X4S}}, "\xfd\xc7\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc8\x01"_su8, O::I64X2ExtendHighI32X4S}}, "\xfd\xc8\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xc9\x01"_su8, O::I64X2ExtendLowI32X4U}}, "\xfd\xc9\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xca\x01"_su8, O::I64X2ExtendHighI32X4U}}, "\xfd\xca\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xcb\x01"_su8, O::I64X2Shl}}, "\xfd\xcb\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xcc\x01"_su8, O::I64X2ShrS}}, "\xfd\xcc\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xcd\x01"_su8, O::I64X2ShrU}}, "\xfd\xcd\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xce\x01"_su8, O::I64X2Add}}, "\xfd\xce\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xd1\x01"_su8, O::I64X2Sub}}, "\xfd\xd1\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xd5\x01"_su8, O::I64X2Mul}}, "\xfd\xd5\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xd6\x01"_su8, O::I64X2Eq}}, "\xfd\xd6\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xd7\x01"_su8, O::I64X2Ne}}, "\xfd\xd7\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xd8\x01"_su8, O::I64X2LtS}}, "\xfd\xd8\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xd9\x01"_su8, O::I64X2GtS}}, "\xfd\xd9\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xda\x01"_su8, O::I64X2LeS}}, "\xfd\xda\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xdb\x01"_su8, O::I64X2GeS}}, "\xfd\xdb\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xdc\x01"_su8, O::I64X2ExtmulLowI32X4S}}, "\xfd\xdc\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xdd\x01"_su8, O::I64X2ExtmulHighI32X4S}}, "\xfd\xdd\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xde\x01"_su8, O::I64X2ExtmulLowI32X4U}}, "\xfd\xde\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xdf\x01"_su8, O::I64X2ExtmulHighI32X4U}}, "\xfd\xdf\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe0\x01"_su8, O::F32X4Abs}}, "\xfd\xe0\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe1\x01"_su8, O::F32X4Neg}}, "\xfd\xe1\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe3\x01"_su8, O::F32X4Sqrt}}, "\xfd\xe3\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe4\x01"_su8, O::F32X4Add}}, "\xfd\xe4\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe5\x01"_su8, O::F32X4Sub}}, "\xfd\xe5\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe6\x01"_su8, O::F32X4Mul}}, "\xfd\xe6\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe7\x01"_su8, O::F32X4Div}}, "\xfd\xe7\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe8\x01"_su8, O::F32X4Min}}, "\xfd\xe8\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xe9\x01"_su8, O::F32X4Max}}, "\xfd\xe9\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xea\x01"_su8, O::F32X4Pmin}}, "\xfd\xea\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xeb\x01"_su8, O::F32X4Pmax}}, "\xfd\xeb\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xec\x01"_su8, O::F64X2Abs}}, "\xfd\xec\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xed\x01"_su8, O::F64X2Neg}}, "\xfd\xed\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xef\x01"_su8, O::F64X2Sqrt}}, "\xfd\xef\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf0\x01"_su8, O::F64X2Add}}, "\xfd\xf0\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf1\x01"_su8, O::F64X2Sub}}, "\xfd\xf1\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf2\x01"_su8, O::F64X2Mul}}, "\xfd\xf2\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf3\x01"_su8, O::F64X2Div}}, "\xfd\xf3\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf4\x01"_su8, O::F64X2Min}}, "\xfd\xf4\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf5\x01"_su8, O::F64X2Max}}, "\xfd\xf5\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf6\x01"_su8, O::F64X2Pmin}}, "\xfd\xf6\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf7\x01"_su8, O::F64X2Pmax}}, "\xfd\xf7\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf8\x01"_su8, O::I32X4TruncSatF32X4S}}, "\xfd\xf8\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xf9\x01"_su8, O::I32X4TruncSatF32X4U}}, "\xfd\xf9\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xfa\x01"_su8, O::F32X4ConvertI32X4S}}, "\xfd\xfa\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xfb\x01"_su8, O::F32X4ConvertI32X4U}}, "\xfd\xfb\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xfc\x01"_su8, O::I32X4TruncSatF64X2SZero}}, "\xfd\xfc\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xfd\x01"_su8, O::I32X4TruncSatF64X2UZero}}, "\xfd\xfd\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xfe\x01"_su8, O::F64X2ConvertLowI32X4S}}, "\xfd\xfe\x01"_su8);
  OK(Read<I>, I{At{"\xfd\xff\x01"_su8, O::F64X2ConvertLowI32X4U}}, "\xfd\xff\x01"_su8);
}

TEST_F(BinaryReadTest, Instruction_threads) {
  ctx.features.enable_threads();

  auto memarg = At{"\x01\x02"_su8, MemArgImmediate{At{"\x01"_su8, u32{1}},
                                                   At{"\x02"_su8, u32{2}}}};

  OK(Read<I>, I{At{"\xfe\x00"_su8, O::MemoryAtomicNotify}, memarg},
     "\xfe\x00\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x01"_su8, O::MemoryAtomicWait32}, memarg},
     "\xfe\x01\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x02"_su8, O::MemoryAtomicWait64}, memarg},
     "\xfe\x02\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x10"_su8, O::I32AtomicLoad}, memarg},
     "\xfe\x10\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x11"_su8, O::I64AtomicLoad}, memarg},
     "\xfe\x11\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x12"_su8, O::I32AtomicLoad8U}, memarg},
     "\xfe\x12\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x13"_su8, O::I32AtomicLoad16U}, memarg},
     "\xfe\x13\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x14"_su8, O::I64AtomicLoad8U}, memarg},
     "\xfe\x14\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x15"_su8, O::I64AtomicLoad16U}, memarg},
     "\xfe\x15\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x16"_su8, O::I64AtomicLoad32U}, memarg},
     "\xfe\x16\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x17"_su8, O::I32AtomicStore}, memarg},
     "\xfe\x17\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x18"_su8, O::I64AtomicStore}, memarg},
     "\xfe\x18\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x19"_su8, O::I32AtomicStore8}, memarg},
     "\xfe\x19\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x1a"_su8, O::I32AtomicStore16}, memarg},
     "\xfe\x1a\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x1b"_su8, O::I64AtomicStore8}, memarg},
     "\xfe\x1b\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x1c"_su8, O::I64AtomicStore16}, memarg},
     "\xfe\x1c\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x1d"_su8, O::I64AtomicStore32}, memarg},
     "\xfe\x1d\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x1e"_su8, O::I32AtomicRmwAdd}, memarg},
     "\xfe\x1e\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x1f"_su8, O::I64AtomicRmwAdd}, memarg},
     "\xfe\x1f\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x20"_su8, O::I32AtomicRmw8AddU}, memarg},
     "\xfe\x20\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x21"_su8, O::I32AtomicRmw16AddU}, memarg},
     "\xfe\x21\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x22"_su8, O::I64AtomicRmw8AddU}, memarg},
     "\xfe\x22\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x23"_su8, O::I64AtomicRmw16AddU}, memarg},
     "\xfe\x23\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x24"_su8, O::I64AtomicRmw32AddU}, memarg},
     "\xfe\x24\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x25"_su8, O::I32AtomicRmwSub}, memarg},
     "\xfe\x25\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x26"_su8, O::I64AtomicRmwSub}, memarg},
     "\xfe\x26\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x27"_su8, O::I32AtomicRmw8SubU}, memarg},
     "\xfe\x27\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x28"_su8, O::I32AtomicRmw16SubU}, memarg},
     "\xfe\x28\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x29"_su8, O::I64AtomicRmw8SubU}, memarg},
     "\xfe\x29\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x2a"_su8, O::I64AtomicRmw16SubU}, memarg},
     "\xfe\x2a\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x2b"_su8, O::I64AtomicRmw32SubU}, memarg},
     "\xfe\x2b\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x2c"_su8, O::I32AtomicRmwAnd}, memarg},
     "\xfe\x2c\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x2d"_su8, O::I64AtomicRmwAnd}, memarg},
     "\xfe\x2d\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x2e"_su8, O::I32AtomicRmw8AndU}, memarg},
     "\xfe\x2e\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x2f"_su8, O::I32AtomicRmw16AndU}, memarg},
     "\xfe\x2f\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x30"_su8, O::I64AtomicRmw8AndU}, memarg},
     "\xfe\x30\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x31"_su8, O::I64AtomicRmw16AndU}, memarg},
     "\xfe\x31\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x32"_su8, O::I64AtomicRmw32AndU}, memarg},
     "\xfe\x32\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x33"_su8, O::I32AtomicRmwOr}, memarg},
     "\xfe\x33\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x34"_su8, O::I64AtomicRmwOr}, memarg},
     "\xfe\x34\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x35"_su8, O::I32AtomicRmw8OrU}, memarg},
     "\xfe\x35\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x36"_su8, O::I32AtomicRmw16OrU}, memarg},
     "\xfe\x36\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x37"_su8, O::I64AtomicRmw8OrU}, memarg},
     "\xfe\x37\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x38"_su8, O::I64AtomicRmw16OrU}, memarg},
     "\xfe\x38\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x39"_su8, O::I64AtomicRmw32OrU}, memarg},
     "\xfe\x39\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x3a"_su8, O::I32AtomicRmwXor}, memarg},
     "\xfe\x3a\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x3b"_su8, O::I64AtomicRmwXor}, memarg},
     "\xfe\x3b\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x3c"_su8, O::I32AtomicRmw8XorU}, memarg},
     "\xfe\x3c\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x3d"_su8, O::I32AtomicRmw16XorU}, memarg},
     "\xfe\x3d\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x3e"_su8, O::I64AtomicRmw8XorU}, memarg},
     "\xfe\x3e\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x3f"_su8, O::I64AtomicRmw16XorU}, memarg},
     "\xfe\x3f\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x40"_su8, O::I64AtomicRmw32XorU}, memarg},
     "\xfe\x40\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x41"_su8, O::I32AtomicRmwXchg}, memarg},
     "\xfe\x41\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x42"_su8, O::I64AtomicRmwXchg}, memarg},
     "\xfe\x42\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x43"_su8, O::I32AtomicRmw8XchgU}, memarg},
     "\xfe\x43\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x44"_su8, O::I32AtomicRmw16XchgU}, memarg},
     "\xfe\x44\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x45"_su8, O::I64AtomicRmw8XchgU}, memarg},
     "\xfe\x45\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x46"_su8, O::I64AtomicRmw16XchgU}, memarg},
     "\xfe\x46\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x47"_su8, O::I64AtomicRmw32XchgU}, memarg},
     "\xfe\x47\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x48"_su8, O::I32AtomicRmwCmpxchg}, memarg},
     "\xfe\x48\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x49"_su8, O::I64AtomicRmwCmpxchg}, memarg},
     "\xfe\x49\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x4a"_su8, O::I32AtomicRmw8CmpxchgU}, memarg},
     "\xfe\x4a\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x4b"_su8, O::I32AtomicRmw16CmpxchgU}, memarg},
     "\xfe\x4b\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x4c"_su8, O::I64AtomicRmw8CmpxchgU}, memarg},
     "\xfe\x4c\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x4d"_su8, O::I64AtomicRmw16CmpxchgU}, memarg},
     "\xfe\x4d\x01\x02"_su8);
  OK(Read<I>, I{At{"\xfe\x4e"_su8, O::I64AtomicRmw32CmpxchgU}, memarg},
     "\xfe\x4e\x01\x02"_su8);
}

TEST_F(BinaryReadTest, Instruction_function_references) {
  ctx.features.enable_function_references();

  OK(Read<I>, I{At{"\x14"_su8, O::CallRef}}, "\x14"_su8);
  OK(Read<I>, I{At{"\x15"_su8, O::ReturnCallRef}}, "\x15"_su8);
  OK(Read<I>,
     I{At{"\x16"_su8, O::FuncBind},
       At{"\x00"_su8, FuncBindImmediate{At{"\x00"_su8, Index{0}}}}},
     "\x16\x00"_su8);
  OK(Read<I>,
     I{At{"\x17"_su8, O::Let},
       At{"\x40\x01\x02\x7f"_su8,
          LetImmediate{At{"\x40"_su8, BT_Void},
                       {At{"\x02\x7f"_su8, Locals{At{"\x02"_su8, Index{2}},
                                                  At{"\x7f"_su8, VT_I32}}}}}}},
     "\x17\x40\x01\x02\x7f"_su8);
  OK(Read<I>, I{At{"\xd3"_su8, O::RefAsNonNull}}, "\xd3"_su8);
  OK(Read<I>, I{At{"\xd4"_su8, O::BrOnNull}, At{"\x00"_su8, Index{0}}},
     "\xd4\x00"_su8);
  OK(Read<I>, I{At{"\xd6"_su8, O::BrOnNonNull}, At{"\x00"_su8, Index{0}}},
     "\xd6\x00"_su8);
}

TEST_F(BinaryReadTest, Instruction_gc) {
  ctx.features.enable_gc();

  OK(Read<I>, I{At{"\xd5"_su8, O::RefEq}}, "\xd5"_su8);
  OK(Read<I>,
     I{At{"\xfb\x01"_su8, O::StructNewWithRtt}, At{"\x01"_su8, Index{1}}},
     "\xfb\x01\x01"_su8);
  OK(Read<I>,
     I{At{"\xfb\x02"_su8, O::StructNewDefaultWithRtt},
       At{"\x01"_su8, Index{1}}},
     "\xfb\x02\x01"_su8);
  OK(Read<I>,
     I{At{"\xfb\x03"_su8, O::StructGet},
       At{"\x01\x02"_su8, StructFieldImmediate{At{"\x01"_su8, Index{1}},
                                               At{"\x02"_su8, Index{2}}}}},
     "\xfb\x03\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfb\x04"_su8, O::StructGetS},
       At{"\x01\x02"_su8, StructFieldImmediate{At{"\x01"_su8, Index{1}},
                                               At{"\x02"_su8, Index{2}}}}},
     "\xfb\x04\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfb\x05"_su8, O::StructGetU},
       At{"\x01\x02"_su8, StructFieldImmediate{At{"\x01"_su8, Index{1}},
                                               At{"\x02"_su8, Index{2}}}}},
     "\xfb\x05\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfb\x06"_su8, O::StructSet},
       At{"\x01\x02"_su8, StructFieldImmediate{At{"\x01"_su8, Index{1}},
                                               At{"\x02"_su8, Index{2}}}}},
     "\xfb\x06\x01\x02"_su8);
  OK(Read<I>,
     I{At{"\xfb\x11"_su8, O::ArrayNewWithRtt}, At{"\x01"_su8, Index{1}}},
     "\xfb\x11\x01"_su8);
  OK(Read<I>,
     I{At{"\xfb\x12"_su8, O::ArrayNewDefaultWithRtt}, At{"\x01"_su8, Index{1}}},
     "\xfb\x12\x01"_su8);
  OK(Read<I>, I{At{"\xfb\x13"_su8, O::ArrayGet}, At{"\x01"_su8, Index{1}}},
     "\xfb\x13\x01"_su8);
  OK(Read<I>, I{At{"\xfb\x14"_su8, O::ArrayGetS}, At{"\x01"_su8, Index{1}}},
     "\xfb\x14\x01"_su8);
  OK(Read<I>, I{At{"\xfb\x15"_su8, O::ArrayGetU}, At{"\x01"_su8, Index{1}}},
     "\xfb\x15\x01"_su8);
  OK(Read<I>, I{At{"\xfb\x16"_su8, O::ArraySet}, At{"\x01"_su8, Index{1}}},
     "\xfb\x16\x01"_su8);
  OK(Read<I>, I{At{"\xfb\x17"_su8, O::ArrayLen}, At{"\x01"_su8, Index{1}}},
     "\xfb\x17\x01"_su8);
  OK(Read<I>, I{At{"\xfb\x20"_su8, O::I31New}}, "\xfb\x20"_su8);
  OK(Read<I>, I{At{"\xfb\x21"_su8, O::I31GetS}}, "\xfb\x21"_su8);
  OK(Read<I>, I{At{"\xfb\x22"_su8, O::I31GetU}}, "\xfb\x22"_su8);
  OK(Read<I>, I{At{"\xfb\x30"_su8, O::RttCanon}, At{"\x70"_su8, HT_Func}},
     "\xfb\x30\x70"_su8);
#if 0
  OK(Read<I>,
     I{At{"\xfb\x31"_su8, O::RttSub},
       At{"\x01\x70\x6f"_su8, RttSubImmediate{At{"\x01"_su8, Index{1}},
                                              {At{"\x70"_su8, HT_Func},
                                               At{"\x6f"_su8, HT_Extern}}}}},
     "\xfb\x31\x01\x70\x6f"_su8);
#endif
  OK(Read<I>,
     I{At{"\xfb\x40"_su8, O::RefTest},
       At{"\x70\x6f"_su8, HeapType2Immediate{At{"\x70"_su8, HT_Func},
                                             At{"\x6f"_su8, HT_Extern}}}},
     "\xfb\x40\x70\x6f"_su8);
  OK(Read<I>,
     I{At{"\xfb\x41"_su8, O::RefCast},
       At{"\x70\x6f"_su8, HeapType2Immediate{At{"\x70"_su8, HT_Func},
                                             At{"\x6f"_su8, HT_Extern}}}},
     "\xfb\x41\x70\x6f"_su8);
#if 0
  OK(Read<I>,
     I{At{"\xfb\x42"_su8, O::BrOnCast},
       At{"\x01\x70\x6f"_su8,
          BrOnCastImmediate{At{"\x01"_su8, Index{1}},
                            HeapType2Immediate{At{"\x70"_su8, HT_Func},
                                               At{"\x6f"_su8, HT_Extern}}}}},
     "\xfb\x42\x01\x70\x6f"_su8);
#endif
}

auto ReadMemoryLimitsForTesting(SpanU8* data, ReadCtx& ctx)
    -> OptAt<Limits> {
  return Read<Limits>(data, ctx, LimitsKind::Memory);
}

auto ReadTableLimitsForTesting(SpanU8* data, ReadCtx& ctx)
    -> OptAt<Limits> {
  return Read<Limits>(data, ctx, LimitsKind::Table);
}

TEST_F(BinaryReadTest, Limits) {
  OK(ReadMemoryLimitsForTesting,
     Limits{At{"\x81\x01"_su8, u32{129}}, nullopt, At{"\x00"_su8, Shared::No}},
     "\x00\x81\x01"_su8);
  OK(ReadMemoryLimitsForTesting,
     Limits{At{"\x02"_su8, u32{2}}, At{"\xe8\x07"_su8, u32{1000}},
            At{"\x01"_su8, Shared::No}},
     "\x01\x02\xe8\x07"_su8);
}

TEST_F(BinaryReadTest, Limits_BadFlags) {
  Fail(ReadMemoryLimitsForTesting,
       {{0, "limits"}, {0, "Unknown flags value: 2"}}, "\x02\x01"_su8);
  Fail(ReadMemoryLimitsForTesting,
       {{0, "limits"}, {0, "Unknown flags value: 3"}}, "\x03\x01"_su8);
}

TEST_F(BinaryReadTest, Limits_threads) {
  ctx.features.enable_threads();

  OK(ReadMemoryLimitsForTesting,
     Limits{At{"\x02"_su8, u32{2}}, At{"\xe8\x07"_su8, u32{1000}},
            At{"\x03"_su8, Shared::Yes}},
     "\x03\x02\xe8\x07"_su8);

  Fail(ReadTableLimitsForTesting,
       {{0, "limits"}, {0, "shared tables are not allowed"}},
       "\x03\x01\x02"_su8);
}

TEST_F(BinaryReadTest, Limits_memory64) {
  ctx.features.enable_memory64();

  OK(ReadMemoryLimitsForTesting,
     Limits{At{"\x01"_su8, u32{1}}, nullopt, At{"\x04"_su8, Shared::No},
            At{"\x04"_su8, IndexType::I64}},
     "\x04\x01"_su8);
  OK(ReadMemoryLimitsForTesting,
     Limits{At{"\x01"_su8, u32{1}}, At{"\x02"_su8, u32{2}},
            At{"\x05"_su8, Shared::No}, At{"\x05"_su8, IndexType::I64}},
     "\x05\x01\x02"_su8);

  // 64-bit tables are not allowed.
  Fail(ReadTableLimitsForTesting,
       {{0, "limits"}, {0, "i64 index type is not allowed"}}, "\x04\x01"_su8);
  Fail(ReadTableLimitsForTesting,
       {{0, "limits"}, {0, "i64 index type is not allowed"}},
       "\x05\x01\x02"_su8);
}

TEST_F(BinaryReadTest, Limits_PastEnd) {
  Fail(ReadMemoryLimitsForTesting,
       {{0, "limits"}, {1, "min"}, {1, "u32"}, {1, "Unable to read u8"}},
       "\x00"_su8);
  Fail(ReadMemoryLimitsForTesting,
       {{0, "limits"}, {2, "max"}, {2, "u32"}, {2, "Unable to read u8"}},
       "\x01\x00"_su8);
}

TEST_F(BinaryReadTest, Locals) {
  OK(Read<Locals>, Locals{At{"\x02"_su8, u32{2}}, At{"\x7f"_su8, VT_I32}},
     "\x02\x7f"_su8);
  OK(Read<Locals>, Locals{At{"\xc0\x02"_su8, u32{320}}, At{"\x7c"_su8, VT_F64}},
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

TEST_F(BinaryReadTest, LetImmediate) {
  OK(Read<LetImmediate>,
     LetImmediate{At{"\x40"_su8, BT_Void}, At{"\x00"_su8, LocalsList{}}},
     "\x40\x00"_su8);

  ctx.features.enable_multi_value();

  OK(Read<LetImmediate>,
     LetImmediate{
         At{"\x00"_su8, BlockType{At{"\x00"_su8, Index{0}}}},
         At{"\x01\x02\x7f"_su8,
            LocalsList{At{"\x02\x7f"_su8, Locals{At{"\x02"_su8, Index{2}},
                                                 At{"\x7f"_su8, VT_I32}}}}}},
     "\x00\x01\x02\x7f"_su8);
}

TEST_F(BinaryReadTest, MemArgImmediate) {
  OK(Read<MemArgImmediate>,
     MemArgImmediate{At{"\x00"_su8, u32{0}}, At{"\x00"_su8, u32{0}}},
     "\x00\x00"_su8);
  OK(Read<MemArgImmediate>,
     MemArgImmediate{At{"\x01"_su8, u32{1}}, At{"\x80\x02"_su8, u32{256}}},
     "\x01\x80\x02"_su8);
}

TEST_F(BinaryReadTest, MemArgImmediate_multi_memory) {
  // Multi-memory makes the alignment into a flags field instead, where 0x40 bit
  // is set when there is a memory index. This is backward compatible because
  // 0x40 as an alignment would have always failed validation, because it is
  // too large.
  OK(Read<MemArgImmediate>,
     MemArgImmediate{At{"\x41"_su8, u32{0x41}}, At{"\x42"_su8, u32{0x42}}},
     "\x41\x42"_su8);

  ctx.features.enable_multi_memory();

  // Now the alignment is 1 (not 0x41), and the memarg has a memory index.
  OK(Read<MemArgImmediate>,
     MemArgImmediate{At{"\x41"_su8, u32{1}}, At{"\x42"_su8, u32{0x42}},
                     At{"\x43"_su8, Index{0x43}}},
     "\x41\x42\x43"_su8);
}


TEST_F(BinaryReadTest, Memory) {
  OK(Read<Memory>,
     Memory{
         At{"\x01\x01\x02"_su8,
            MemoryType{At{"\x01\x01\x02"_su8,
                          Limits{At{"\x01"_su8, u32{1}}, At{"\x02"_su8, u32{2}},
                                 At{"\x01"_su8, Shared::No}}}}}},
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
     MemoryType{At{"\x00\x01"_su8, Limits{At{"\x01"_su8, u32{1}}, nullopt,
                                          At{"\x00"_su8, Shared::No}}}},
     "\x00\x01"_su8);
  OK(Read<MemoryType>,
     MemoryType{At{"\x01\x00\x80\x01"_su8,
                   Limits{At{"\x00"_su8, u32{0}}, At{"\x80\x01"_su8, u32{128}},
                          At{"\x01"_su8, Shared::No}}}},
     "\x01\x00\x80\x01"_su8);
}

TEST_F(BinaryReadTest, MemoryType_memory64) {
  ctx.features.enable_memory64();

  OK(Read<MemoryType>,
     MemoryType{At{"\x04\x01"_su8, Limits{At{"\x01"_su8, u32{1}}, nullopt,
                                          At{"\x04"_su8, Shared::No},
                                          At{"\x04"_su8, IndexType::I64}}}},
     "\x04\x01"_su8);
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
  Fail(Read<Mutability>, {{0, "mutability"}, {0, "Unknown mutability: 4"}},
       "\x04"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<Mutability>, {{0, "mutability"}, {0, "Unknown mutability: 132"}},
       "\x84\x00"_su8);
}

TEST_F(BinaryReadTest, NameAssoc) {
  OK(Read<NameAssoc>,
     NameAssoc{At{"\x02"_su8, Index{2}}, At{"\x02hi"_su8, "hi"_sv}},
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
       {{0, "name subsection id"}, {0, "Unknown name subsection id: 3"}},
       "\x03"_su8);
  Fail(Read<NameSubsectionId>,
       {{0, "name subsection id"}, {0, "Unknown name subsection id: 255"}},
       "\xff"_su8);
}

TEST_F(BinaryReadTest, NameSubsection) {
  OK(Read<NameSubsection>,
     NameSubsection{At{"\x00"_su8, NameSubsectionId::ModuleName}, "\0"_su8},
     "\x00\x01\0"_su8);

  OK(Read<NameSubsection>,
     NameSubsection{At{"\x01"_su8, NameSubsectionId::FunctionNames},
                    "\0\0"_su8},
     "\x01\x02\0\0"_su8);

  OK(Read<NameSubsection>,
     NameSubsection{At{"\x02"_su8, NameSubsectionId::LocalNames}, "\0\0\0"_su8},
     "\x02\x03\0\0\0"_su8);
}

TEST_F(BinaryReadTest, NameSubsection_BadSubsectionId) {
  Fail(Read<NameSubsection>,
       {{0, "name subsection"},
        {0, "name subsection id"},
        {0, "Unknown name subsection id: 3"}},
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
  ctx.features.enable_exceptions();

  OK(Read<Opcode>, Opcode::Try, "\x06"_su8);
  OK(Read<Opcode>, Opcode::Catch, "\x07"_su8);
  OK(Read<Opcode>, Opcode::Throw, "\x08"_su8);
  OK(Read<Opcode>, Opcode::Rethrow, "\x09"_su8);
  OK(Read<Opcode>, Opcode::Delegate, "\x18"_su8);
  OK(Read<Opcode>, Opcode::CatchAll, "\x19"_su8);
}

TEST_F(BinaryReadTest, Opcode_tail_call) {
  ctx.features.enable_tail_call();

  OK(Read<Opcode>, Opcode::ReturnCall, "\x12"_su8);
  OK(Read<Opcode>, Opcode::ReturnCallIndirect, "\x13"_su8);
}

TEST_F(BinaryReadTest, Opcode_sign_extension) {
  ctx.features.enable_sign_extension();

  OK(Read<Opcode>, Opcode::I32Extend8S, "\xc0"_su8);
  OK(Read<Opcode>, Opcode::I32Extend16S, "\xc1"_su8);
  OK(Read<Opcode>, Opcode::I64Extend8S, "\xc2"_su8);
  OK(Read<Opcode>, Opcode::I64Extend16S, "\xc3"_su8);
  OK(Read<Opcode>, Opcode::I64Extend32S, "\xc4"_su8);
}

TEST_F(BinaryReadTest, Opcode_reference_types) {
  ctx.features.enable_reference_types();

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
  ctx.features.enable_saturating_float_to_int();

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
  ctx.features.enable_bulk_memory();

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
    ctx.features = Features{Features::SaturatingFloatToInt};
    FailUnknownOpcode(0xfc, 8);
    FailUnknownOpcode(0xfc, 9);
    FailUnknownOpcode(0xfc, 10);
    FailUnknownOpcode(0xfc, 11);
    FailUnknownOpcode(0xfc, 12);
    FailUnknownOpcode(0xfc, 13);
    FailUnknownOpcode(0xfc, 14);
  }

  {
    ctx.features = Features{Features::BulkMemory};
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
  ctx.features.enable_saturating_float_to_int();
  ctx.features.enable_bulk_memory();

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
  ctx.features.enable_simd();

  OK(Read<O>, At{"\xfd\x00"_su8, O::V128Load}, "\xfd\x00"_su8);
  OK(Read<O>, At{"\xfd\x01"_su8, O::V128Load8X8S}, "\xfd\x01"_su8);
  OK(Read<O>, At{"\xfd\x02"_su8, O::V128Load8X8U}, "\xfd\x02"_su8);
  OK(Read<O>, At{"\xfd\x03"_su8, O::V128Load16X4S}, "\xfd\x03"_su8);
  OK(Read<O>, At{"\xfd\x04"_su8, O::V128Load16X4U}, "\xfd\x04"_su8);
  OK(Read<O>, At{"\xfd\x05"_su8, O::V128Load32X2S}, "\xfd\x05"_su8);
  OK(Read<O>, At{"\xfd\x06"_su8, O::V128Load32X2U}, "\xfd\x06"_su8);
  OK(Read<O>, At{"\xfd\x07"_su8, O::V128Load8Splat}, "\xfd\x07"_su8);
  OK(Read<O>, At{"\xfd\x08"_su8, O::V128Load16Splat}, "\xfd\x08"_su8);
  OK(Read<O>, At{"\xfd\x09"_su8, O::V128Load32Splat}, "\xfd\x09"_su8);
  OK(Read<O>, At{"\xfd\x0a"_su8, O::V128Load64Splat}, "\xfd\x0a"_su8);
  OK(Read<O>, At{"\xfd\x0b"_su8, O::V128Store}, "\xfd\x0b"_su8);
  OK(Read<O>, At{"\xfd\x0c"_su8, O::V128Const}, "\xfd\x0c"_su8);
  OK(Read<O>, At{"\xfd\x0d"_su8, O::I8X16Shuffle}, "\xfd\x0d"_su8);
  OK(Read<O>, At{"\xfd\x0e"_su8, O::I8X16Swizzle}, "\xfd\x0e"_su8);
  OK(Read<O>, At{"\xfd\x0f"_su8, O::I8X16Splat}, "\xfd\x0f"_su8);
  OK(Read<O>, At{"\xfd\x10"_su8, O::I16X8Splat}, "\xfd\x10"_su8);
  OK(Read<O>, At{"\xfd\x11"_su8, O::I32X4Splat}, "\xfd\x11"_su8);
  OK(Read<O>, At{"\xfd\x12"_su8, O::I64X2Splat}, "\xfd\x12"_su8);
  OK(Read<O>, At{"\xfd\x13"_su8, O::F32X4Splat}, "\xfd\x13"_su8);
  OK(Read<O>, At{"\xfd\x14"_su8, O::F64X2Splat}, "\xfd\x14"_su8);
  OK(Read<O>, At{"\xfd\x15"_su8, O::I8X16ExtractLaneS}, "\xfd\x15"_su8);
  OK(Read<O>, At{"\xfd\x16"_su8, O::I8X16ExtractLaneU}, "\xfd\x16"_su8);
  OK(Read<O>, At{"\xfd\x17"_su8, O::I8X16ReplaceLane}, "\xfd\x17"_su8);
  OK(Read<O>, At{"\xfd\x18"_su8, O::I16X8ExtractLaneS}, "\xfd\x18"_su8);
  OK(Read<O>, At{"\xfd\x19"_su8, O::I16X8ExtractLaneU}, "\xfd\x19"_su8);
  OK(Read<O>, At{"\xfd\x1a"_su8, O::I16X8ReplaceLane}, "\xfd\x1a"_su8);
  OK(Read<O>, At{"\xfd\x1b"_su8, O::I32X4ExtractLane}, "\xfd\x1b"_su8);
  OK(Read<O>, At{"\xfd\x1c"_su8, O::I32X4ReplaceLane}, "\xfd\x1c"_su8);
  OK(Read<O>, At{"\xfd\x1d"_su8, O::I64X2ExtractLane}, "\xfd\x1d"_su8);
  OK(Read<O>, At{"\xfd\x1e"_su8, O::I64X2ReplaceLane}, "\xfd\x1e"_su8);
  OK(Read<O>, At{"\xfd\x1f"_su8, O::F32X4ExtractLane}, "\xfd\x1f"_su8);
  OK(Read<O>, At{"\xfd\x20"_su8, O::F32X4ReplaceLane}, "\xfd\x20"_su8);
  OK(Read<O>, At{"\xfd\x21"_su8, O::F64X2ExtractLane}, "\xfd\x21"_su8);
  OK(Read<O>, At{"\xfd\x22"_su8, O::F64X2ReplaceLane}, "\xfd\x22"_su8);
  OK(Read<O>, At{"\xfd\x23"_su8, O::I8X16Eq}, "\xfd\x23"_su8);
  OK(Read<O>, At{"\xfd\x24"_su8, O::I8X16Ne}, "\xfd\x24"_su8);
  OK(Read<O>, At{"\xfd\x25"_su8, O::I8X16LtS}, "\xfd\x25"_su8);
  OK(Read<O>, At{"\xfd\x26"_su8, O::I8X16LtU}, "\xfd\x26"_su8);
  OK(Read<O>, At{"\xfd\x27"_su8, O::I8X16GtS}, "\xfd\x27"_su8);
  OK(Read<O>, At{"\xfd\x28"_su8, O::I8X16GtU}, "\xfd\x28"_su8);
  OK(Read<O>, At{"\xfd\x29"_su8, O::I8X16LeS}, "\xfd\x29"_su8);
  OK(Read<O>, At{"\xfd\x2a"_su8, O::I8X16LeU}, "\xfd\x2a"_su8);
  OK(Read<O>, At{"\xfd\x2b"_su8, O::I8X16GeS}, "\xfd\x2b"_su8);
  OK(Read<O>, At{"\xfd\x2c"_su8, O::I8X16GeU}, "\xfd\x2c"_su8);
  OK(Read<O>, At{"\xfd\x2d"_su8, O::I16X8Eq}, "\xfd\x2d"_su8);
  OK(Read<O>, At{"\xfd\x2e"_su8, O::I16X8Ne}, "\xfd\x2e"_su8);
  OK(Read<O>, At{"\xfd\x2f"_su8, O::I16X8LtS}, "\xfd\x2f"_su8);
  OK(Read<O>, At{"\xfd\x30"_su8, O::I16X8LtU}, "\xfd\x30"_su8);
  OK(Read<O>, At{"\xfd\x31"_su8, O::I16X8GtS}, "\xfd\x31"_su8);
  OK(Read<O>, At{"\xfd\x32"_su8, O::I16X8GtU}, "\xfd\x32"_su8);
  OK(Read<O>, At{"\xfd\x33"_su8, O::I16X8LeS}, "\xfd\x33"_su8);
  OK(Read<O>, At{"\xfd\x34"_su8, O::I16X8LeU}, "\xfd\x34"_su8);
  OK(Read<O>, At{"\xfd\x35"_su8, O::I16X8GeS}, "\xfd\x35"_su8);
  OK(Read<O>, At{"\xfd\x36"_su8, O::I16X8GeU}, "\xfd\x36"_su8);
  OK(Read<O>, At{"\xfd\x37"_su8, O::I32X4Eq}, "\xfd\x37"_su8);
  OK(Read<O>, At{"\xfd\x38"_su8, O::I32X4Ne}, "\xfd\x38"_su8);
  OK(Read<O>, At{"\xfd\x39"_su8, O::I32X4LtS}, "\xfd\x39"_su8);
  OK(Read<O>, At{"\xfd\x3a"_su8, O::I32X4LtU}, "\xfd\x3a"_su8);
  OK(Read<O>, At{"\xfd\x3b"_su8, O::I32X4GtS}, "\xfd\x3b"_su8);
  OK(Read<O>, At{"\xfd\x3c"_su8, O::I32X4GtU}, "\xfd\x3c"_su8);
  OK(Read<O>, At{"\xfd\x3d"_su8, O::I32X4LeS}, "\xfd\x3d"_su8);
  OK(Read<O>, At{"\xfd\x3e"_su8, O::I32X4LeU}, "\xfd\x3e"_su8);
  OK(Read<O>, At{"\xfd\x3f"_su8, O::I32X4GeS}, "\xfd\x3f"_su8);
  OK(Read<O>, At{"\xfd\x40"_su8, O::I32X4GeU}, "\xfd\x40"_su8);
  OK(Read<O>, At{"\xfd\x41"_su8, O::F32X4Eq}, "\xfd\x41"_su8);
  OK(Read<O>, At{"\xfd\x42"_su8, O::F32X4Ne}, "\xfd\x42"_su8);
  OK(Read<O>, At{"\xfd\x43"_su8, O::F32X4Lt}, "\xfd\x43"_su8);
  OK(Read<O>, At{"\xfd\x44"_su8, O::F32X4Gt}, "\xfd\x44"_su8);
  OK(Read<O>, At{"\xfd\x45"_su8, O::F32X4Le}, "\xfd\x45"_su8);
  OK(Read<O>, At{"\xfd\x46"_su8, O::F32X4Ge}, "\xfd\x46"_su8);
  OK(Read<O>, At{"\xfd\x47"_su8, O::F64X2Eq}, "\xfd\x47"_su8);
  OK(Read<O>, At{"\xfd\x48"_su8, O::F64X2Ne}, "\xfd\x48"_su8);
  OK(Read<O>, At{"\xfd\x49"_su8, O::F64X2Lt}, "\xfd\x49"_su8);
  OK(Read<O>, At{"\xfd\x4a"_su8, O::F64X2Gt}, "\xfd\x4a"_su8);
  OK(Read<O>, At{"\xfd\x4b"_su8, O::F64X2Le}, "\xfd\x4b"_su8);
  OK(Read<O>, At{"\xfd\x4c"_su8, O::F64X2Ge}, "\xfd\x4c"_su8);
  OK(Read<O>, At{"\xfd\x4d"_su8, O::V128Not}, "\xfd\x4d"_su8);
  OK(Read<O>, At{"\xfd\x4e"_su8, O::V128And}, "\xfd\x4e"_su8);
  OK(Read<O>, At{"\xfd\x4f"_su8, O::V128Andnot}, "\xfd\x4f"_su8);
  OK(Read<O>, At{"\xfd\x50"_su8, O::V128Or}, "\xfd\x50"_su8);
  OK(Read<O>, At{"\xfd\x51"_su8, O::V128Xor}, "\xfd\x51"_su8);
  OK(Read<O>, At{"\xfd\x52"_su8, O::V128BitSelect}, "\xfd\x52"_su8);
  OK(Read<O>, At{"\xfd\x53"_su8, O::V128AnyTrue}, "\xfd\x53"_su8);
  OK(Read<O>, At{"\xfd\x54"_su8, O::V128Load8Lane}, "\xfd\x54"_su8);
  OK(Read<O>, At{"\xfd\x55"_su8, O::V128Load16Lane}, "\xfd\x55"_su8);
  OK(Read<O>, At{"\xfd\x56"_su8, O::V128Load32Lane}, "\xfd\x56"_su8);
  OK(Read<O>, At{"\xfd\x57"_su8, O::V128Load64Lane}, "\xfd\x57"_su8);
  OK(Read<O>, At{"\xfd\x58"_su8, O::V128Store8Lane}, "\xfd\x58"_su8);
  OK(Read<O>, At{"\xfd\x59"_su8, O::V128Store16Lane}, "\xfd\x59"_su8);
  OK(Read<O>, At{"\xfd\x5a"_su8, O::V128Store32Lane}, "\xfd\x5a"_su8);
  OK(Read<O>, At{"\xfd\x5b"_su8, O::V128Store64Lane}, "\xfd\x5b"_su8);
  OK(Read<O>, At{"\xfd\x5c"_su8, O::V128Load32Zero}, "\xfd\x5c"_su8);
  OK(Read<O>, At{"\xfd\x5d"_su8, O::V128Load64Zero}, "\xfd\x5d"_su8);
  OK(Read<O>, At{"\xfd\x5e"_su8, O::F32X4DemoteF64X2Zero}, "\xfd\x5e"_su8);
  OK(Read<O>, At{"\xfd\x5f"_su8, O::F64X2PromoteLowF32X4}, "\xfd\x5f"_su8);
  OK(Read<O>, At{"\xfd\x60"_su8, O::I8X16Abs}, "\xfd\x60"_su8);
  OK(Read<O>, At{"\xfd\x61"_su8, O::I8X16Neg}, "\xfd\x61"_su8);
  OK(Read<O>, At{"\xfd\x62"_su8, O::I8X16Popcnt}, "\xfd\x62"_su8);
  OK(Read<O>, At{"\xfd\x63"_su8, O::I8X16AllTrue}, "\xfd\x63"_su8);
  OK(Read<O>, At{"\xfd\x64"_su8, O::I8X16Bitmask}, "\xfd\x64"_su8);
  OK(Read<O>, At{"\xfd\x65"_su8, O::I8X16NarrowI16X8S}, "\xfd\x65"_su8);
  OK(Read<O>, At{"\xfd\x66"_su8, O::I8X16NarrowI16X8U}, "\xfd\x66"_su8);
  OK(Read<O>, At{"\xfd\x67"_su8, O::F32X4Ceil}, "\xfd\x67"_su8);
  OK(Read<O>, At{"\xfd\x68"_su8, O::F32X4Floor}, "\xfd\x68"_su8);
  OK(Read<O>, At{"\xfd\x69"_su8, O::F32X4Trunc}, "\xfd\x69"_su8);
  OK(Read<O>, At{"\xfd\x6a"_su8, O::F32X4Nearest}, "\xfd\x6a"_su8);
  OK(Read<O>, At{"\xfd\x6b"_su8, O::I8X16Shl}, "\xfd\x6b"_su8);
  OK(Read<O>, At{"\xfd\x6c"_su8, O::I8X16ShrS}, "\xfd\x6c"_su8);
  OK(Read<O>, At{"\xfd\x6d"_su8, O::I8X16ShrU}, "\xfd\x6d"_su8);
  OK(Read<O>, At{"\xfd\x6e"_su8, O::I8X16Add}, "\xfd\x6e"_su8);
  OK(Read<O>, At{"\xfd\x6f"_su8, O::I8X16AddSatS}, "\xfd\x6f"_su8);
  OK(Read<O>, At{"\xfd\x70"_su8, O::I8X16AddSatU}, "\xfd\x70"_su8);
  OK(Read<O>, At{"\xfd\x71"_su8, O::I8X16Sub}, "\xfd\x71"_su8);
  OK(Read<O>, At{"\xfd\x72"_su8, O::I8X16SubSatS}, "\xfd\x72"_su8);
  OK(Read<O>, At{"\xfd\x73"_su8, O::I8X16SubSatU}, "\xfd\x73"_su8);
  OK(Read<O>, At{"\xfd\x74"_su8, O::F64X2Ceil}, "\xfd\x74"_su8);
  OK(Read<O>, At{"\xfd\x75"_su8, O::F64X2Floor}, "\xfd\x75"_su8);
  OK(Read<O>, At{"\xfd\x76"_su8, O::I8X16MinS}, "\xfd\x76"_su8);
  OK(Read<O>, At{"\xfd\x77"_su8, O::I8X16MinU}, "\xfd\x77"_su8);
  OK(Read<O>, At{"\xfd\x78"_su8, O::I8X16MaxS}, "\xfd\x78"_su8);
  OK(Read<O>, At{"\xfd\x79"_su8, O::I8X16MaxU}, "\xfd\x79"_su8);
  OK(Read<O>, At{"\xfd\x7a"_su8, O::F64X2Trunc}, "\xfd\x7a"_su8);
  OK(Read<O>, At{"\xfd\x7b"_su8, O::I8X16AvgrU}, "\xfd\x7b"_su8);
  OK(Read<O>, At{"\xfd\x7c"_su8, O::I16X8ExtaddPairwiseI8X16S}, "\xfd\x7c"_su8);
  OK(Read<O>, At{"\xfd\x7d"_su8, O::I16X8ExtaddPairwiseI8X16U}, "\xfd\x7d"_su8);
  OK(Read<O>, At{"\xfd\x7e"_su8, O::I32X4ExtaddPairwiseI16X8S}, "\xfd\x7e"_su8);
  OK(Read<O>, At{"\xfd\x7f"_su8, O::I32X4ExtaddPairwiseI16X8U}, "\xfd\x7f"_su8);
  OK(Read<O>, At{"\xfd\x80\x01"_su8, O::I16X8Abs}, "\xfd\x80\x01"_su8);
  OK(Read<O>, At{"\xfd\x81\x01"_su8, O::I16X8Neg}, "\xfd\x81\x01"_su8);
  OK(Read<O>, At{"\xfd\x82\x01"_su8, O::I16X8Q15mulrSatS}, "\xfd\x82\x01"_su8);
  OK(Read<O>, At{"\xfd\x83\x01"_su8, O::I16X8AllTrue}, "\xfd\x83\x01"_su8);
  OK(Read<O>, At{"\xfd\x84\x01"_su8, O::I16X8Bitmask}, "\xfd\x84\x01"_su8);
  OK(Read<O>, At{"\xfd\x85\x01"_su8, O::I16X8NarrowI32X4S}, "\xfd\x85\x01"_su8);
  OK(Read<O>, At{"\xfd\x86\x01"_su8, O::I16X8NarrowI32X4U}, "\xfd\x86\x01"_su8);
  OK(Read<O>, At{"\xfd\x87\x01"_su8, O::I16X8ExtendLowI8X16S}, "\xfd\x87\x01"_su8);
  OK(Read<O>, At{"\xfd\x88\x01"_su8, O::I16X8ExtendHighI8X16S}, "\xfd\x88\x01"_su8);
  OK(Read<O>, At{"\xfd\x89\x01"_su8, O::I16X8ExtendLowI8X16U}, "\xfd\x89\x01"_su8);
  OK(Read<O>, At{"\xfd\x8a\x01"_su8, O::I16X8ExtendHighI8X16U}, "\xfd\x8a\x01"_su8);
  OK(Read<O>, At{"\xfd\x8b\x01"_su8, O::I16X8Shl}, "\xfd\x8b\x01"_su8);
  OK(Read<O>, At{"\xfd\x8c\x01"_su8, O::I16X8ShrS}, "\xfd\x8c\x01"_su8);
  OK(Read<O>, At{"\xfd\x8d\x01"_su8, O::I16X8ShrU}, "\xfd\x8d\x01"_su8);
  OK(Read<O>, At{"\xfd\x8e\x01"_su8, O::I16X8Add}, "\xfd\x8e\x01"_su8);
  OK(Read<O>, At{"\xfd\x8f\x01"_su8, O::I16X8AddSatS}, "\xfd\x8f\x01"_su8);
  OK(Read<O>, At{"\xfd\x90\x01"_su8, O::I16X8AddSatU}, "\xfd\x90\x01"_su8);
  OK(Read<O>, At{"\xfd\x91\x01"_su8, O::I16X8Sub}, "\xfd\x91\x01"_su8);
  OK(Read<O>, At{"\xfd\x92\x01"_su8, O::I16X8SubSatS}, "\xfd\x92\x01"_su8);
  OK(Read<O>, At{"\xfd\x93\x01"_su8, O::I16X8SubSatU}, "\xfd\x93\x01"_su8);
  OK(Read<O>, At{"\xfd\x94\x01"_su8, O::F64X2Nearest}, "\xfd\x94\x01"_su8);
  OK(Read<O>, At{"\xfd\x95\x01"_su8, O::I16X8Mul}, "\xfd\x95\x01"_su8);
  OK(Read<O>, At{"\xfd\x96\x01"_su8, O::I16X8MinS}, "\xfd\x96\x01"_su8);
  OK(Read<O>, At{"\xfd\x97\x01"_su8, O::I16X8MinU}, "\xfd\x97\x01"_su8);
  OK(Read<O>, At{"\xfd\x98\x01"_su8, O::I16X8MaxS}, "\xfd\x98\x01"_su8);
  OK(Read<O>, At{"\xfd\x99\x01"_su8, O::I16X8MaxU}, "\xfd\x99\x01"_su8);
  OK(Read<O>, At{"\xfd\x9b\x01"_su8, O::I16X8AvgrU}, "\xfd\x9b\x01"_su8);
  OK(Read<O>, At{"\xfd\x9c\x01"_su8, O::I16X8ExtmulLowI8X16S}, "\xfd\x9c\x01"_su8);
  OK(Read<O>, At{"\xfd\x9d\x01"_su8, O::I16X8ExtmulHighI8X16S}, "\xfd\x9d\x01"_su8);
  OK(Read<O>, At{"\xfd\x9e\x01"_su8, O::I16X8ExtmulLowI8X16U}, "\xfd\x9e\x01"_su8);
  OK(Read<O>, At{"\xfd\x9f\x01"_su8, O::I16X8ExtmulHighI8X16U}, "\xfd\x9f\x01"_su8);
  OK(Read<O>, At{"\xfd\xa0\x01"_su8, O::I32X4Abs}, "\xfd\xa0\x01"_su8);
  OK(Read<O>, At{"\xfd\xa1\x01"_su8, O::I32X4Neg}, "\xfd\xa1\x01"_su8);
  OK(Read<O>, At{"\xfd\xa3\x01"_su8, O::I32X4AllTrue}, "\xfd\xa3\x01"_su8);
  OK(Read<O>, At{"\xfd\xa4\x01"_su8, O::I32X4Bitmask}, "\xfd\xa4\x01"_su8);
  OK(Read<O>, At{"\xfd\xa7\x01"_su8, O::I32X4ExtendLowI16X8S}, "\xfd\xa7\x01"_su8);
  OK(Read<O>, At{"\xfd\xa8\x01"_su8, O::I32X4ExtendHighI16X8S}, "\xfd\xa8\x01"_su8);
  OK(Read<O>, At{"\xfd\xa9\x01"_su8, O::I32X4ExtendLowI16X8U}, "\xfd\xa9\x01"_su8);
  OK(Read<O>, At{"\xfd\xaa\x01"_su8, O::I32X4ExtendHighI16X8U}, "\xfd\xaa\x01"_su8);
  OK(Read<O>, At{"\xfd\xab\x01"_su8, O::I32X4Shl}, "\xfd\xab\x01"_su8);
  OK(Read<O>, At{"\xfd\xac\x01"_su8, O::I32X4ShrS}, "\xfd\xac\x01"_su8);
  OK(Read<O>, At{"\xfd\xad\x01"_su8, O::I32X4ShrU}, "\xfd\xad\x01"_su8);
  OK(Read<O>, At{"\xfd\xae\x01"_su8, O::I32X4Add}, "\xfd\xae\x01"_su8);
  OK(Read<O>, At{"\xfd\xb1\x01"_su8, O::I32X4Sub}, "\xfd\xb1\x01"_su8);
  OK(Read<O>, At{"\xfd\xb5\x01"_su8, O::I32X4Mul}, "\xfd\xb5\x01"_su8);
  OK(Read<O>, At{"\xfd\xb6\x01"_su8, O::I32X4MinS}, "\xfd\xb6\x01"_su8);
  OK(Read<O>, At{"\xfd\xb7\x01"_su8, O::I32X4MinU}, "\xfd\xb7\x01"_su8);
  OK(Read<O>, At{"\xfd\xb8\x01"_su8, O::I32X4MaxS}, "\xfd\xb8\x01"_su8);
  OK(Read<O>, At{"\xfd\xb9\x01"_su8, O::I32X4MaxU}, "\xfd\xb9\x01"_su8);
  OK(Read<O>, At{"\xfd\xba\x01"_su8, O::I32X4DotI16X8S}, "\xfd\xba\x01"_su8);
  OK(Read<O>, At{"\xfd\xbc\x01"_su8, O::I32X4ExtmulLowI16X8S}, "\xfd\xbc\x01"_su8);
  OK(Read<O>, At{"\xfd\xbd\x01"_su8, O::I32X4ExtmulHighI16X8S}, "\xfd\xbd\x01"_su8);
  OK(Read<O>, At{"\xfd\xbe\x01"_su8, O::I32X4ExtmulLowI16X8U}, "\xfd\xbe\x01"_su8);
  OK(Read<O>, At{"\xfd\xbf\x01"_su8, O::I32X4ExtmulHighI16X8U}, "\xfd\xbf\x01"_su8);
  OK(Read<O>, At{"\xfd\xc0\x01"_su8, O::I64X2Abs}, "\xfd\xc0\x01"_su8);
  OK(Read<O>, At{"\xfd\xc1\x01"_su8, O::I64X2Neg}, "\xfd\xc1\x01"_su8);
  OK(Read<O>, At{"\xfd\xc3\x01"_su8, O::I64X2AllTrue}, "\xfd\xc3\x01"_su8);
  OK(Read<O>, At{"\xfd\xc4\x01"_su8, O::I64X2Bitmask}, "\xfd\xc4\x01"_su8);
  OK(Read<O>, At{"\xfd\xc7\x01"_su8, O::I64X2ExtendLowI32X4S}, "\xfd\xc7\x01"_su8);
  OK(Read<O>, At{"\xfd\xc8\x01"_su8, O::I64X2ExtendHighI32X4S}, "\xfd\xc8\x01"_su8);
  OK(Read<O>, At{"\xfd\xc9\x01"_su8, O::I64X2ExtendLowI32X4U}, "\xfd\xc9\x01"_su8);
  OK(Read<O>, At{"\xfd\xca\x01"_su8, O::I64X2ExtendHighI32X4U}, "\xfd\xca\x01"_su8);
  OK(Read<O>, At{"\xfd\xcb\x01"_su8, O::I64X2Shl}, "\xfd\xcb\x01"_su8);
  OK(Read<O>, At{"\xfd\xcc\x01"_su8, O::I64X2ShrS}, "\xfd\xcc\x01"_su8);
  OK(Read<O>, At{"\xfd\xcd\x01"_su8, O::I64X2ShrU}, "\xfd\xcd\x01"_su8);
  OK(Read<O>, At{"\xfd\xce\x01"_su8, O::I64X2Add}, "\xfd\xce\x01"_su8);
  OK(Read<O>, At{"\xfd\xd1\x01"_su8, O::I64X2Sub}, "\xfd\xd1\x01"_su8);
  OK(Read<O>, At{"\xfd\xd5\x01"_su8, O::I64X2Mul}, "\xfd\xd5\x01"_su8);
  OK(Read<O>, At{"\xfd\xd6\x01"_su8, O::I64X2Eq}, "\xfd\xd6\x01"_su8);
  OK(Read<O>, At{"\xfd\xd7\x01"_su8, O::I64X2Ne}, "\xfd\xd7\x01"_su8);
  OK(Read<O>, At{"\xfd\xd8\x01"_su8, O::I64X2LtS}, "\xfd\xd8\x01"_su8);
  OK(Read<O>, At{"\xfd\xd9\x01"_su8, O::I64X2GtS}, "\xfd\xd9\x01"_su8);
  OK(Read<O>, At{"\xfd\xda\x01"_su8, O::I64X2LeS}, "\xfd\xda\x01"_su8);
  OK(Read<O>, At{"\xfd\xdb\x01"_su8, O::I64X2GeS}, "\xfd\xdb\x01"_su8);
  OK(Read<O>, At{"\xfd\xdc\x01"_su8, O::I64X2ExtmulLowI32X4S}, "\xfd\xdc\x01"_su8);
  OK(Read<O>, At{"\xfd\xdd\x01"_su8, O::I64X2ExtmulHighI32X4S}, "\xfd\xdd\x01"_su8);
  OK(Read<O>, At{"\xfd\xde\x01"_su8, O::I64X2ExtmulLowI32X4U}, "\xfd\xde\x01"_su8);
  OK(Read<O>, At{"\xfd\xdf\x01"_su8, O::I64X2ExtmulHighI32X4U}, "\xfd\xdf\x01"_su8);
  OK(Read<O>, At{"\xfd\xe0\x01"_su8, O::F32X4Abs}, "\xfd\xe0\x01"_su8);
  OK(Read<O>, At{"\xfd\xe1\x01"_su8, O::F32X4Neg}, "\xfd\xe1\x01"_su8);
  OK(Read<O>, At{"\xfd\xe3\x01"_su8, O::F32X4Sqrt}, "\xfd\xe3\x01"_su8);
  OK(Read<O>, At{"\xfd\xe4\x01"_su8, O::F32X4Add}, "\xfd\xe4\x01"_su8);
  OK(Read<O>, At{"\xfd\xe5\x01"_su8, O::F32X4Sub}, "\xfd\xe5\x01"_su8);
  OK(Read<O>, At{"\xfd\xe6\x01"_su8, O::F32X4Mul}, "\xfd\xe6\x01"_su8);
  OK(Read<O>, At{"\xfd\xe7\x01"_su8, O::F32X4Div}, "\xfd\xe7\x01"_su8);
  OK(Read<O>, At{"\xfd\xe8\x01"_su8, O::F32X4Min}, "\xfd\xe8\x01"_su8);
  OK(Read<O>, At{"\xfd\xe9\x01"_su8, O::F32X4Max}, "\xfd\xe9\x01"_su8);
  OK(Read<O>, At{"\xfd\xea\x01"_su8, O::F32X4Pmin}, "\xfd\xea\x01"_su8);
  OK(Read<O>, At{"\xfd\xeb\x01"_su8, O::F32X4Pmax}, "\xfd\xeb\x01"_su8);
  OK(Read<O>, At{"\xfd\xec\x01"_su8, O::F64X2Abs}, "\xfd\xec\x01"_su8);
  OK(Read<O>, At{"\xfd\xed\x01"_su8, O::F64X2Neg}, "\xfd\xed\x01"_su8);
  OK(Read<O>, At{"\xfd\xef\x01"_su8, O::F64X2Sqrt}, "\xfd\xef\x01"_su8);
  OK(Read<O>, At{"\xfd\xf0\x01"_su8, O::F64X2Add}, "\xfd\xf0\x01"_su8);
  OK(Read<O>, At{"\xfd\xf1\x01"_su8, O::F64X2Sub}, "\xfd\xf1\x01"_su8);
  OK(Read<O>, At{"\xfd\xf2\x01"_su8, O::F64X2Mul}, "\xfd\xf2\x01"_su8);
  OK(Read<O>, At{"\xfd\xf3\x01"_su8, O::F64X2Div}, "\xfd\xf3\x01"_su8);
  OK(Read<O>, At{"\xfd\xf4\x01"_su8, O::F64X2Min}, "\xfd\xf4\x01"_su8);
  OK(Read<O>, At{"\xfd\xf5\x01"_su8, O::F64X2Max}, "\xfd\xf5\x01"_su8);
  OK(Read<O>, At{"\xfd\xf6\x01"_su8, O::F64X2Pmin}, "\xfd\xf6\x01"_su8);
  OK(Read<O>, At{"\xfd\xf7\x01"_su8, O::F64X2Pmax}, "\xfd\xf7\x01"_su8);
  OK(Read<O>, At{"\xfd\xf8\x01"_su8, O::I32X4TruncSatF32X4S}, "\xfd\xf8\x01"_su8);
  OK(Read<O>, At{"\xfd\xf9\x01"_su8, O::I32X4TruncSatF32X4U}, "\xfd\xf9\x01"_su8);
  OK(Read<O>, At{"\xfd\xfa\x01"_su8, O::F32X4ConvertI32X4S}, "\xfd\xfa\x01"_su8);
  OK(Read<O>, At{"\xfd\xfb\x01"_su8, O::F32X4ConvertI32X4U}, "\xfd\xfb\x01"_su8);
  OK(Read<O>, At{"\xfd\xfc\x01"_su8, O::I32X4TruncSatF64X2SZero}, "\xfd\xfc\x01"_su8);
  OK(Read<O>, At{"\xfd\xfd\x01"_su8, O::I32X4TruncSatF64X2UZero}, "\xfd\xfd\x01"_su8);
  OK(Read<O>, At{"\xfd\xfe\x01"_su8, O::F64X2ConvertLowI32X4S}, "\xfd\xfe\x01"_su8);
  OK(Read<O>, At{"\xfd\xff\x01"_su8, O::F64X2ConvertLowI32X4U}, "\xfd\xff\x01"_su8);
}

TEST_F(BinaryReadTest, Opcode_Unknown_simd_prefix) {
  ctx.features.enable_simd();

  const u8 kInvalidOpcodes[] = {
      0xa2, 0xa5, 0xa6, 0xaf, 0xb0, 0xb2, 0xb3, 0xb4, 0xbb,
      0xc2, 0xc5, 0xc6, 0xcf, 0xd0, 0xd2, 0xd3, 0xd4, 0xe2,
      0xee,
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
  ctx.features.enable_threads();

  OK(Read<O>, At{"\xfe\x00"_su8, O::MemoryAtomicNotify}, "\xfe\x00"_su8);
  OK(Read<O>, At{"\xfe\x01"_su8, O::MemoryAtomicWait32}, "\xfe\x01"_su8);
  OK(Read<O>, At{"\xfe\x02"_su8, O::MemoryAtomicWait64}, "\xfe\x02"_su8);
  OK(Read<O>, At{"\xfe\x10"_su8, O::I32AtomicLoad}, "\xfe\x10"_su8);
  OK(Read<O>, At{"\xfe\x11"_su8, O::I64AtomicLoad}, "\xfe\x11"_su8);
  OK(Read<O>, At{"\xfe\x12"_su8, O::I32AtomicLoad8U}, "\xfe\x12"_su8);
  OK(Read<O>, At{"\xfe\x13"_su8, O::I32AtomicLoad16U}, "\xfe\x13"_su8);
  OK(Read<O>, At{"\xfe\x14"_su8, O::I64AtomicLoad8U}, "\xfe\x14"_su8);
  OK(Read<O>, At{"\xfe\x15"_su8, O::I64AtomicLoad16U}, "\xfe\x15"_su8);
  OK(Read<O>, At{"\xfe\x16"_su8, O::I64AtomicLoad32U}, "\xfe\x16"_su8);
  OK(Read<O>, At{"\xfe\x17"_su8, O::I32AtomicStore}, "\xfe\x17"_su8);
  OK(Read<O>, At{"\xfe\x18"_su8, O::I64AtomicStore}, "\xfe\x18"_su8);
  OK(Read<O>, At{"\xfe\x19"_su8, O::I32AtomicStore8}, "\xfe\x19"_su8);
  OK(Read<O>, At{"\xfe\x1a"_su8, O::I32AtomicStore16}, "\xfe\x1a"_su8);
  OK(Read<O>, At{"\xfe\x1b"_su8, O::I64AtomicStore8}, "\xfe\x1b"_su8);
  OK(Read<O>, At{"\xfe\x1c"_su8, O::I64AtomicStore16}, "\xfe\x1c"_su8);
  OK(Read<O>, At{"\xfe\x1d"_su8, O::I64AtomicStore32}, "\xfe\x1d"_su8);
  OK(Read<O>, At{"\xfe\x1e"_su8, O::I32AtomicRmwAdd}, "\xfe\x1e"_su8);
  OK(Read<O>, At{"\xfe\x1f"_su8, O::I64AtomicRmwAdd}, "\xfe\x1f"_su8);
  OK(Read<O>, At{"\xfe\x20"_su8, O::I32AtomicRmw8AddU}, "\xfe\x20"_su8);
  OK(Read<O>, At{"\xfe\x21"_su8, O::I32AtomicRmw16AddU}, "\xfe\x21"_su8);
  OK(Read<O>, At{"\xfe\x22"_su8, O::I64AtomicRmw8AddU}, "\xfe\x22"_su8);
  OK(Read<O>, At{"\xfe\x23"_su8, O::I64AtomicRmw16AddU}, "\xfe\x23"_su8);
  OK(Read<O>, At{"\xfe\x24"_su8, O::I64AtomicRmw32AddU}, "\xfe\x24"_su8);
  OK(Read<O>, At{"\xfe\x25"_su8, O::I32AtomicRmwSub}, "\xfe\x25"_su8);
  OK(Read<O>, At{"\xfe\x26"_su8, O::I64AtomicRmwSub}, "\xfe\x26"_su8);
  OK(Read<O>, At{"\xfe\x27"_su8, O::I32AtomicRmw8SubU}, "\xfe\x27"_su8);
  OK(Read<O>, At{"\xfe\x28"_su8, O::I32AtomicRmw16SubU}, "\xfe\x28"_su8);
  OK(Read<O>, At{"\xfe\x29"_su8, O::I64AtomicRmw8SubU}, "\xfe\x29"_su8);
  OK(Read<O>, At{"\xfe\x2a"_su8, O::I64AtomicRmw16SubU}, "\xfe\x2a"_su8);
  OK(Read<O>, At{"\xfe\x2b"_su8, O::I64AtomicRmw32SubU}, "\xfe\x2b"_su8);
  OK(Read<O>, At{"\xfe\x2c"_su8, O::I32AtomicRmwAnd}, "\xfe\x2c"_su8);
  OK(Read<O>, At{"\xfe\x2d"_su8, O::I64AtomicRmwAnd}, "\xfe\x2d"_su8);
  OK(Read<O>, At{"\xfe\x2e"_su8, O::I32AtomicRmw8AndU}, "\xfe\x2e"_su8);
  OK(Read<O>, At{"\xfe\x2f"_su8, O::I32AtomicRmw16AndU}, "\xfe\x2f"_su8);
  OK(Read<O>, At{"\xfe\x30"_su8, O::I64AtomicRmw8AndU}, "\xfe\x30"_su8);
  OK(Read<O>, At{"\xfe\x31"_su8, O::I64AtomicRmw16AndU}, "\xfe\x31"_su8);
  OK(Read<O>, At{"\xfe\x32"_su8, O::I64AtomicRmw32AndU}, "\xfe\x32"_su8);
  OK(Read<O>, At{"\xfe\x33"_su8, O::I32AtomicRmwOr}, "\xfe\x33"_su8);
  OK(Read<O>, At{"\xfe\x34"_su8, O::I64AtomicRmwOr}, "\xfe\x34"_su8);
  OK(Read<O>, At{"\xfe\x35"_su8, O::I32AtomicRmw8OrU}, "\xfe\x35"_su8);
  OK(Read<O>, At{"\xfe\x36"_su8, O::I32AtomicRmw16OrU}, "\xfe\x36"_su8);
  OK(Read<O>, At{"\xfe\x37"_su8, O::I64AtomicRmw8OrU}, "\xfe\x37"_su8);
  OK(Read<O>, At{"\xfe\x38"_su8, O::I64AtomicRmw16OrU}, "\xfe\x38"_su8);
  OK(Read<O>, At{"\xfe\x39"_su8, O::I64AtomicRmw32OrU}, "\xfe\x39"_su8);
  OK(Read<O>, At{"\xfe\x3a"_su8, O::I32AtomicRmwXor}, "\xfe\x3a"_su8);
  OK(Read<O>, At{"\xfe\x3b"_su8, O::I64AtomicRmwXor}, "\xfe\x3b"_su8);
  OK(Read<O>, At{"\xfe\x3c"_su8, O::I32AtomicRmw8XorU}, "\xfe\x3c"_su8);
  OK(Read<O>, At{"\xfe\x3d"_su8, O::I32AtomicRmw16XorU}, "\xfe\x3d"_su8);
  OK(Read<O>, At{"\xfe\x3e"_su8, O::I64AtomicRmw8XorU}, "\xfe\x3e"_su8);
  OK(Read<O>, At{"\xfe\x3f"_su8, O::I64AtomicRmw16XorU}, "\xfe\x3f"_su8);
  OK(Read<O>, At{"\xfe\x40"_su8, O::I64AtomicRmw32XorU}, "\xfe\x40"_su8);
  OK(Read<O>, At{"\xfe\x41"_su8, O::I32AtomicRmwXchg}, "\xfe\x41"_su8);
  OK(Read<O>, At{"\xfe\x42"_su8, O::I64AtomicRmwXchg}, "\xfe\x42"_su8);
  OK(Read<O>, At{"\xfe\x43"_su8, O::I32AtomicRmw8XchgU}, "\xfe\x43"_su8);
  OK(Read<O>, At{"\xfe\x44"_su8, O::I32AtomicRmw16XchgU}, "\xfe\x44"_su8);
  OK(Read<O>, At{"\xfe\x45"_su8, O::I64AtomicRmw8XchgU}, "\xfe\x45"_su8);
  OK(Read<O>, At{"\xfe\x46"_su8, O::I64AtomicRmw16XchgU}, "\xfe\x46"_su8);
  OK(Read<O>, At{"\xfe\x47"_su8, O::I64AtomicRmw32XchgU}, "\xfe\x47"_su8);
  OK(Read<O>, At{"\xfe\x48"_su8, O::I32AtomicRmwCmpxchg}, "\xfe\x48"_su8);
  OK(Read<O>, At{"\xfe\x49"_su8, O::I64AtomicRmwCmpxchg}, "\xfe\x49"_su8);
  OK(Read<O>, At{"\xfe\x4a"_su8, O::I32AtomicRmw8CmpxchgU}, "\xfe\x4a"_su8);
  OK(Read<O>, At{"\xfe\x4b"_su8, O::I32AtomicRmw16CmpxchgU}, "\xfe\x4b"_su8);
  OK(Read<O>, At{"\xfe\x4c"_su8, O::I64AtomicRmw8CmpxchgU}, "\xfe\x4c"_su8);
  OK(Read<O>, At{"\xfe\x4d"_su8, O::I64AtomicRmw16CmpxchgU}, "\xfe\x4d"_su8);
  OK(Read<O>, At{"\xfe\x4e"_su8, O::I64AtomicRmw32CmpxchgU}, "\xfe\x4e"_su8);
}

TEST_F(BinaryReadTest, Opcode_Unknown_threads_prefix) {
  ctx.features.enable_threads();

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

TEST_F(BinaryReadTest, Opcode_function_references) {
  ctx.features.enable_function_references();

  OK(Read<O>, At{"\x14"_su8, O::CallRef}, "\x14"_su8);
  OK(Read<O>, At{"\x15"_su8, O::ReturnCallRef}, "\x15"_su8);
  OK(Read<O>, At{"\x16"_su8, O::FuncBind}, "\x16"_su8);
  OK(Read<O>, At{"\x17"_su8, O::Let}, "\x17"_su8);
  OK(Read<O>, At{"\xd3"_su8, O::RefAsNonNull}, "\xd3"_su8);
  OK(Read<O>, At{"\xd4"_su8, O::BrOnNull}, "\xd4"_su8);
  OK(Read<O>, At{"\xd6"_su8, O::BrOnNonNull}, "\xd6"_su8);
}

TEST_F(BinaryReadTest, Opcode_gc) {
  ctx.features.enable_gc();

  OK(Read<O>, O::RefEq, "\xd5"_su8);
  OK(Read<O>, O::StructNewWithRtt, "\xfb\x01"_su8);
  OK(Read<O>, O::StructNewDefaultWithRtt, "\xfb\x02"_su8);
  OK(Read<O>, O::StructGet, "\xfb\x03"_su8);
  OK(Read<O>, O::StructGetS, "\xfb\x04"_su8);
  OK(Read<O>, O::StructGetU, "\xfb\x05"_su8);
  OK(Read<O>, O::StructSet, "\xfb\x06"_su8);
  OK(Read<O>, O::ArrayNewWithRtt, "\xfb\x11"_su8);
  OK(Read<O>, O::ArrayNewDefaultWithRtt, "\xfb\x12"_su8);
  OK(Read<O>, O::ArrayGet, "\xfb\x13"_su8);
  OK(Read<O>, O::ArrayGetS, "\xfb\x14"_su8);
  OK(Read<O>, O::ArrayGetU, "\xfb\x15"_su8);
  OK(Read<O>, O::ArraySet, "\xfb\x16"_su8);
  OK(Read<O>, O::ArrayLen, "\xfb\x17"_su8);
  OK(Read<O>, O::I31New, "\xfb\x20"_su8);
  OK(Read<O>, O::I31GetS, "\xfb\x21"_su8);
  OK(Read<O>, O::I31GetU, "\xfb\x22"_su8);
  OK(Read<O>, O::RttCanon, "\xfb\x30"_su8);
  OK(Read<O>, O::RttSub, "\xfb\x31"_su8);
  OK(Read<O>, O::RefTest, "\xfb\x40"_su8);
  OK(Read<O>, O::RefCast, "\xfb\x41"_su8);
  OK(Read<O>, O::BrOnCast, "\xfb\x42"_su8);
}

TEST_F(BinaryReadTest, RttSubImmediate) {
  OK(Read<RttSubImmediate>,
     RttSubImmediate{
         At{"\x00"_su8, Index{0}},
         At{"\x70\x70"_su8, HeapType2Immediate{At{"\x70"_su8, HT_Func},
                                               At{"\x70"_su8, HT_Func}}}},
     "\x00\x70\x70"_su8);
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
  Fail(Read<s32>, {{0, "s32"}, {4, "Unable to read u8"}},
       "\xf0\xf0\xf0\xf0"_su8);
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
  OK(Read<s64>, 1070725794579330814,
     "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"_su8);
  OK(Read<s64>, -3540960223848057090,
     "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"_su8);
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
  Fail(Read<s64>, {{0, "s64"}, {4, "Unable to read u8"}},
       "\xf0\xf0\xf0\xf0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {5, "Unable to read u8"}},
       "\xe0\xe0\xe0\xe0\xe0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {6, "Unable to read u8"}},
       "\xd0\xd0\xd0\xd0\xd0\xc0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {7, "Unable to read u8"}},
       "\xc0\xc0\xc0\xc0\xc0\xd0\x84"_su8);
  Fail(Read<s64>, {{0, "s64"}, {8, "Unable to read u8"}},
       "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"_su8);
  Fail(Read<s64>, {{0, "s64"}, {9, "Unable to read u8"}},
       "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"_su8);
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

  // Overlong encoding is not allowed.
  Fail(Read<SectionId>, {{0, "section id"}, {0, "Unknown section id: 128"}},
       "\x80\x00"_su8);
}

TEST_F(BinaryReadTest, SectionId_bulk_memory) {
  Fail(Read<SectionId>, {{0, "section id"}, {0, "Unknown section id: 12"}},
       "\x0c"_su8);

  ctx.features.enable_bulk_memory();

  OK(Read<SectionId>, SectionId::DataCount, "\x0c"_su8);
}

TEST_F(BinaryReadTest, SectionId_exceptions) {
  Fail(Read<SectionId>, {{0, "section id"}, {0, "Unknown section id: 13"}},
       "\x0d"_su8);

  ctx.features.enable_exceptions();

  OK(Read<SectionId>, SectionId::Tag, "\x0d"_su8);
}

TEST_F(BinaryReadTest, SectionId_Unknown) {
  Fail(Read<SectionId>, {{0, "section id"}, {0, "Unknown section id: 14"}},
       "\x0e"_su8);
}

TEST_F(BinaryReadTest, Section) {
  OK(Read<Section>,
     Section{
         At{"\x01\x03\x01\x02\x03"_su8,
            KnownSection{At{"\x01"_su8, SectionId::Type}, "\x01\x02\x03"_su8}}},
     "\x01\x03\x01\x02\x03"_su8);

  OK(Read<Section>,
     Section{
         At{"\x00\x08\x04name\x04\x05\x06"_su8,
            CustomSection{At{"\x04name"_su8, "name"_sv}, "\x04\x05\x06"_su8}}},
     "\x00\x08\x04name\x04\x05\x06"_su8);
}

TEST_F(BinaryReadTest, Section_PastEnd) {
  Fail(Read<Section>,
       {{0, "section"}, {0, "section id"}, {0, "Unable to read u8"}}, ""_su8);

  Fail(Read<Section>, {{0, "section"}, {1, "length"}, {1, "Unable to read u8"}},
       "\x01"_su8);

  Fail(Read<Section>, {{0, "section"}, {1, "Length extends past end: 1 > 0"}},
       "\x01\x01"_su8);
}

TEST_F(BinaryReadTest, Section_OutOfOrder) {
  ctx.features.enable_exceptions();
  ctx.last_section_id = SectionId::Global;

  // Can't use OK/Fail, since reading an out-of-order section produces a valid
  // value, but also an error.
  auto data = "\x0d\x01\x00"_su8;
  auto orig_data = data;
  auto expected = Section{
         At{data, KnownSection{At{"\x0d"_su8, SectionId::Tag}, "\x00"_su8}}};

  auto actual = Read<Section>(&data, ctx);
  EXPECT_EQ(0u, data.size());
  EXPECT_NE(nullptr, actual->loc().data());
  ASSERT_TRUE(actual.has_value());
  EXPECT_EQ(expected, **actual);

  ExpectError({{0, "section"},
               {0, "Section out of order: tag cannot occur after global"}},
              errors, orig_data);
}

TEST_F(BinaryReadTest, Start) {
  OK(Read<Start>, Start{At{"\x80\x02"_su8, Index{256}}}, "\x80\x02"_su8);
}

TEST_F(BinaryReadTest, StorageType) {
  ctx.features.enable_gc();

  OK(Read<StorageType>, StorageType{At{"\x7f"_su8, VT_I32}}, "\x7f"_su8);
  OK(Read<StorageType>, StorageType{At{"\x7a"_su8, PackedType::I8}},
     "\x7a"_su8);
  OK(Read<StorageType>, StorageType{At{"\x79"_su8, PackedType::I16}},
     "\x79"_su8);
}

TEST_F(BinaryReadTest, ReadString) {
  OK(ReadString, "hello"_sv, "\x05hello"_su8, "test");
}

TEST_F(BinaryReadTest, ReadString_Leftovers) {
  const SpanU8 data = "\x01more"_su8;
  SpanU8 copy = data;
  auto result = ReadString(&copy, ctx, "test");
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
  auto result = ReadString(&copy, ctx, "test");
  ExpectError({{0, "test"}, {0, "Length extends past end: 6 > 5"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(5u, copy.size());
}

TEST_F(BinaryReadTest, StructType) {
  ctx.features.enable_gc();

  OK(Read<StructType>,
     StructType{FieldTypeList{
         At{"\x7f\x00"_su8,
            FieldType{At{"\x7f"_su8, StorageType{At{"\x7f"_su8, VT_I32}}},
                      At{"\x00"_su8, Mutability::Const}}},
         At{"\x7e\x01"_su8,
            FieldType{At{"\x7e"_su8, StorageType{At{"\x7e"_su8, VT_I64}}},
                      At{"\x01"_su8, Mutability::Var}}},
     }},
     "\x02\x7f\x00\x7e\x01"_su8);
}

TEST_F(BinaryReadTest, StructFieldImmediate) {
  OK(Read<StructFieldImmediate>,
     StructFieldImmediate{At{"\x00"_su8, Index{0}}, At{"\x01"_su8, Index{1}}},
     "\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Table) {
  OK(Read<Table>,
     Table{
         At{"\x70\x00\x01"_su8,
            TableType{At{"\x00\x01"_su8, Limits{At{"\x01"_su8, u32{1}}, nullopt,
                                                At{"\x00"_su8, Shared::No}}},
                      At{"\x70"_su8, RT_Funcref}}}},
     "\x70\x00\x01"_su8);
}

TEST_F(BinaryReadTest, Table_PastEnd) {
  Fail(Read<Table>,
       {{0, "table"},
        {0, "table type"},
        {0, "reference type"},
        {0, "Unable to read u8"}},
       ""_su8);
}

TEST_F(BinaryReadTest, TableType) {
  OK(Read<TableType>,
     TableType{At{"\x00\x01"_su8, Limits{At{"\x01"_su8, u32{1}}, nullopt,
                                         At{"\x00"_su8, Shared::No}}},
               At{"\x70"_su8, RT_Funcref}},
     "\x70\x00\x01"_su8);
  OK(Read<TableType>,
     TableType{At{"\x01\x01\x02"_su8,
                  Limits{At{"\x01"_su8, u32{1}}, At{"\x02"_su8, u32{2}},
                         At{"\x01"_su8, Shared::No}}},
               At{"\x70"_su8, RT_Funcref}},
     "\x70\x01\x01\x02"_su8);
}

TEST_F(BinaryReadTest, TableType_memory64) {
  ctx.features.enable_memory64();

  Fail(Read<TableType>,
       {{0, "table type"}, {1, "limits"}, {1, "i64 index type is not allowed"}},
       "\x70\x04\x01"_su8);
}

TEST_F(BinaryReadTest, TableType_BadReferenceType) {
  Fail(Read<TableType>,
       {{0, "table type"},
        {0, "reference type"},
        {0, "Unknown reference type: 0"}},
       "\x00"_su8);
}

TEST_F(BinaryReadTest, TableType_PastEnd) {
  Fail(Read<TableType>,
       {{0, "table type"}, {0, "reference type"}, {0, "Unable to read u8"}},
       ""_su8);

  Fail(Read<TableType>,
       {{0, "table type"},
        {1, "limits"},
        {1, "flags"},
        {1, "Unable to read u8"}},
       "\x70"_su8);
}

TEST_F(BinaryReadTest, DefinedType) {
  OK(Read<DefinedType>,
     DefinedType{
         At{"\x00\x01\x7f"_su8, FunctionType{{}, {At{"\x7f"_su8, VT_I32}}}}},
     "\x60\x00\x01\x7f"_su8);
}

TEST_F(BinaryReadTest, DefinedType_gc) {
  ctx.features.enable_gc();

  OK(Read<DefinedType>,
     DefinedType{At{
         "\x02\x7f\x00\x7e\x01"_su8,
         StructType{FieldTypeList{
             At{"\x7f\x00"_su8,
                FieldType{At{"\x7f"_su8, StorageType{At{"\x7f"_su8, VT_I32}}},
                          At{"\x00"_su8, Mutability::Const}}},
             At{"\x7e\x01"_su8,
                FieldType{At{"\x7e"_su8, StorageType{At{"\x7e"_su8, VT_I64}}},
                          At{"\x01"_su8, Mutability::Var}}},
         }}}},
     "\x5f\x02\x7f\x00\x7e\x01"_su8);

  OK(Read<DefinedType>,
     DefinedType{At{
         "\x7f\x00"_su8,
         ArrayType{
             At{"\x7f\x00"_su8,
                FieldType{At{"\x7f"_su8, StorageType{At{"\x7f"_su8, VT_I32}}},
                          At{"\x00"_su8, Mutability::Const}}}},
     }},
     "\x5e\x7f\x00"_su8);
}

TEST_F(BinaryReadTest, DefinedType_BadForm) {
  Fail(Read<DefinedType>, {{0, "defined type"}, {0, "Unknown type form: 64"}},
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
  Fail(Read<u32>, {{0, "u32"}, {4, "Unable to read u8"}},
       "\xf0\xf0\xf0\xf0"_su8);
}

TEST_F(BinaryReadTest, U8) {
  OK(Read<u8>, 32, "\x20"_su8);
  Fail(Read<u8>, {{0, "Unable to read u8"}}, ""_su8);
}

TEST_F(BinaryReadTest, ValueType_MVP) {
  OK(Read<ValueType>, VT_I32, "\x7f"_su8);
  OK(Read<ValueType>, VT_I64, "\x7e"_su8);
  OK(Read<ValueType>, VT_F32, "\x7d"_su8);
  OK(Read<ValueType>, VT_F64, "\x7c"_su8);
}

TEST_F(BinaryReadTest, ValueType_simd) {
  Fail(Read<ValueType>, {{0, "value type"}, {0, "Unknown value type: 123"}},
       "\x7b"_su8);

  ctx.features.enable_simd();
  OK(Read<ValueType>, VT_V128, "\x7b"_su8);
}

TEST_F(BinaryReadTest, ValueType_reference_types) {
  Fail(Read<ValueType>, {{0, "value type"}, {0, "funcref not allowed"}},
       "\x70"_su8);
  Fail(Read<ValueType>,
       {{0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 111"}},
       "\x6f"_su8);

  ctx.features.enable_reference_types();
  OK(Read<ValueType>, VT_Funcref, "\x70"_su8);
  OK(Read<ValueType>, VT_Externref, "\x6f"_su8);
}

TEST_F(BinaryReadTest, ValueType_gc) {
  Fail(Read<ValueType>,
       {{0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 110"}},
       "\x6e"_su8);

  ctx.features.enable_gc();

  OK(Read<ValueType>, VT_Anyref, "\x6e"_su8);
  OK(Read<ValueType>, VT_Eqref, "\x6d"_su8);
  OK(Read<ValueType>, VT_I31ref, "\x6a"_su8);
  OK(Read<ValueType>, VT_RefFunc, "\x6b\x70"_su8);
  OK(Read<ValueType>, VT_RefNullFunc, "\x6c\x70"_su8);
  OK(Read<ValueType>, VT_RefExtern, "\x6b\x6f"_su8);
  OK(Read<ValueType>, VT_RefNullExtern, "\x6c\x6f"_su8);
  OK(Read<ValueType>, VT_RefAny, "\x6b\x6e"_su8);
  OK(Read<ValueType>, VT_RefNullAny, "\x6c\x6e"_su8);
  OK(Read<ValueType>, VT_RefEq, "\x6b\x6d"_su8);
  OK(Read<ValueType>, VT_RefNullEq, "\x6c\x6d"_su8);
  OK(Read<ValueType>, VT_RefI31, "\x6b\x6a"_su8);
  OK(Read<ValueType>, VT_RefNullI31, "\x6c\x6a"_su8);
  OK(Read<ValueType>, VT_Ref0, "\x6b\x00"_su8);
  OK(Read<ValueType>, VT_RefNull0, "\x6c\x00"_su8);
  OK(Read<ValueType>, VT_RTT_0_Func, "\x69\x00\x70"_su8);
  OK(Read<ValueType>, VT_RTT_0_Extern, "\x69\x00\x6f"_su8);
  OK(Read<ValueType>, VT_RTT_0_Any, "\x69\x00\x6e"_su8);
  OK(Read<ValueType>, VT_RTT_0_Eq, "\x69\x00\x6d"_su8);
  OK(Read<ValueType>, VT_RTT_0_I31, "\x69\x00\x6a"_su8);
  OK(Read<ValueType>, VT_RTT_0_0, "\x69\x00\x00"_su8);
}

TEST_F(BinaryReadTest, ValueType_Unknown) {
  Fail(Read<ValueType>,
       {{0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 16"}},
       "\x10"_su8);

  // Overlong encoding is not allowed.
  Fail(Read<ValueType>,
       {{0, "value type"},
        {0, "reference type"},
        {0, "Unknown reference type: 255"}},
       "\xff\x7f"_su8);
}

TEST_F(BinaryReadTest, ReadVector_u8) {
  const SpanU8 data = "\x05hello"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u8>(&copy, ctx, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<At<u8>>{
                At{"h"_su8, u8{'h'}},
                At{"e"_su8, u8{'e'}},
                At{"l"_su8, u8{'l'}},
                At{"l"_su8, u8{'l'}},
                At{"o"_su8, u8{'o'}},
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
  auto result = ReadVector<u32>(&copy, ctx, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<At<u32>>{
                At{"\x05"_su8, u32{5}},
                At{"\x80\x01"_su8, u32{128}},
                At{"\xcc\xcc\x0c"_su8, u32{206412}},
            }),
            result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, ReadVector_FailLength) {
  const SpanU8 data =
      "\x02"  // Count.
      "\x05"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, ctx, "test");
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
  auto result = ReadVector<u32>(&copy, ctx, "test");
  ExpectError({{0, "test"}, {2, "u32"}, {3, "Unable to read u8"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(0u, copy.size());
}

TEST_F(BinaryReadTest, EndCode_UnclosedBlock) {
  const SpanU8 data = "\x02\x40"_su8;  // block void
  ctx.open_blocks.push_back(At{data.first(1), Opcode::Block});
  EXPECT_FALSE(EndCode(data.last(0), ctx));
  ExpectError({{0, "Unclosed block instruction"}}, errors, data);
}

TEST_F(BinaryReadTest, EndCode_MissingEnd) {
  const SpanU8 data = "\x01"_su8;  // nop
  ctx.seen_final_end = false;
  EXPECT_FALSE(EndCode(data.last(0), ctx));
  ExpectError({{1, "Expected final end instruction"}}, errors, data);
}

TEST_F(BinaryReadTest, EndModule_FunctionCodeMismatch) {
  const SpanU8 data =
      "\0asm\x01\x00\x00\x00"  // magic + version
      "\x01\x04\x60\x00"       // (type (func))
      "\x03\x02\x01\x00"_su8;  // (func (type 0)), no code section
  ctx.defined_function_count = 1;
  ctx.code_count = 0;
  EXPECT_FALSE(EndModule(data.last(0), ctx));
  ExpectError({{16, "Expected code count of 1, but got 0"}}, errors, data);
}

TEST_F(BinaryReadTest, EndModule_DataCount_DataMissing) {
  const SpanU8 data =
      "\0asm\x01\x00\x00\x00"  // magic + version
      "\x0c\x01\x01"_su8;      // data count = 1
  ctx.declared_data_count = 1;
  ctx.data_count = 0;
  EXPECT_FALSE(EndModule(data.last(0), ctx));
  ExpectError({{11, "Expected data count of 1, but got 0"}}, errors, data);
}

TEST_F(BinaryReadTest, EndModule_DataCountMismatch) {
  const SpanU8 data =
      "\0asm\x01\x00\x00\x00"      // magic + version
      "\x0c\x01\x00"               // data count = 0
      "\x0b\x03\x01\x01\x00"_su8;  // empty passive data segment
  ctx.declared_data_count = 0;
  ctx.data_count = 1;
  EXPECT_FALSE(EndModule(data.last(0), ctx));
  ExpectError({{16, "Expected data count of 0, but got 1"}}, errors, data);
}
