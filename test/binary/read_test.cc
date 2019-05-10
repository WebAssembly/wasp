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
#include <vector>

#include "gtest/gtest.h"

#include "test/binary/read_test_utils.h"
#include "test/binary/test_utils.h"
#include "wasp/binary/read/read_block_type.h"
#include "wasp/binary/read/read_br_on_exn_immediate.h"
#include "wasp/binary/read/read_br_table_immediate.h"
#include "wasp/binary/read/read_bytes.h"
#include "wasp/binary/read/read_call_indirect_immediate.h"
#include "wasp/binary/read/read_code.h"
#include "wasp/binary/read/read_constant_expression.h"
#include "wasp/binary/read/read_copy_immediate.h"
#include "wasp/binary/read/read_count.h"
#include "wasp/binary/read/read_data_segment.h"
#include "wasp/binary/read/read_element_expression.h"
#include "wasp/binary/read/read_element_segment.h"
#include "wasp/binary/read/read_element_type.h"
#include "wasp/binary/read/read_export.h"
#include "wasp/binary/read/read_external_kind.h"
#include "wasp/binary/read/read_f32.h"
#include "wasp/binary/read/read_f64.h"
#include "wasp/binary/read/read_function.h"
#include "wasp/binary/read/read_function_type.h"
#include "wasp/binary/read/read_global.h"
#include "wasp/binary/read/read_global_type.h"
#include "wasp/binary/read/read_import.h"
#include "wasp/binary/read/read_indirect_name_assoc.h"
#include "wasp/binary/read/read_init_immediate.h"
#include "wasp/binary/read/read_instruction.h"
#include "wasp/binary/read/read_limits.h"
#include "wasp/binary/read/read_locals.h"
#include "wasp/binary/read/read_mem_arg_immediate.h"
#include "wasp/binary/read/read_memory.h"
#include "wasp/binary/read/read_memory_type.h"
#include "wasp/binary/read/read_mutability.h"
#include "wasp/binary/read/read_name_assoc.h"
#include "wasp/binary/read/read_name_subsection.h"
#include "wasp/binary/read/read_name_subsection_id.h"
#include "wasp/binary/read/read_opcode.h"
#include "wasp/binary/read/read_s32.h"
#include "wasp/binary/read/read_s64.h"
#include "wasp/binary/read/read_section.h"
#include "wasp/binary/read/read_section_id.h"
#include "wasp/binary/read/read_shuffle_immediate.h"
#include "wasp/binary/read/read_start.h"
#include "wasp/binary/read/read_string.h"
#include "wasp/binary/read/read_table.h"
#include "wasp/binary/read/read_table_type.h"
#include "wasp/binary/read/read_type_entry.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/read/read_u8.h"
#include "wasp/binary/read/read_v128.h"
#include "wasp/binary/read/read_value_type.h"
#include "wasp/binary/read/read_vector.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReadTest, BlockType_MVP) {
  ExpectRead<BlockType>(BlockType::I32, "\x7f"_su8);
  ExpectRead<BlockType>(BlockType::I64, "\x7e"_su8);
  ExpectRead<BlockType>(BlockType::F32, "\x7d"_su8);
  ExpectRead<BlockType>(BlockType::F64, "\x7c"_su8);
  ExpectRead<BlockType>(BlockType::Void, "\x40"_su8);
}

TEST(ReadTest, BlockType_Basic_multi_value) {
  Features features;
  features.enable_multi_value();

  ExpectRead<BlockType>(BlockType::I32, "\x7f"_su8, features);
  ExpectRead<BlockType>(BlockType::I64, "\x7e"_su8, features);
  ExpectRead<BlockType>(BlockType::F32, "\x7d"_su8, features);
  ExpectRead<BlockType>(BlockType::F64, "\x7c"_su8, features);
  ExpectRead<BlockType>(BlockType::Void, "\x40"_su8, features);
}

TEST(ReadTest, BlockType_simd) {
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 123"}}, "\x7b"_su8);

  Features features;
  features.enable_simd();
  ExpectRead<BlockType>(BlockType::V128, "\x7b"_su8, features);
}

TEST(ReadTest, BlockType_MultiValueNegative) {
  Features features;
  features.enable_multi_value();
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: -9"}}, "\x77"_su8, features);
}

TEST(ReadTest, BlockType_multi_value) {
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 1"}}, "\x01"_su8);

  Features features;
  features.enable_multi_value();
  ExpectRead<BlockType>(BlockType(1), "\x01"_su8, features);
  ExpectRead<BlockType>(BlockType(448), "\xc0\x03"_su8, features);
}

TEST(ReadTest, BlockType_reference_types) {
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 111"}}, "\x6f"_su8);

  Features features;
  features.enable_reference_types();
  ExpectRead<BlockType>(BlockType::Anyref, "\x6f"_su8, features);
}

TEST(ReadTest, BlockType_Unknown) {
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 0"}}, "\x00"_su8);

  // Overlong encoding is not allowed.
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 255"}}, "\xff\x7f"_su8);
}

TEST(ReadTest, BrOnExnImmediate) {
  ExpectRead<BrOnExnImmediate>(BrOnExnImmediate{0, 0}, "\x00\x00"_su8);
}

TEST(ReadTest, BrOnExnImmediate_PastEnd) {
  ExpectReadFailure<BrOnExnImmediate>(
      {{0, "br_on_exn"}, {0, "target"}, {0, "Unable to read u8"}}, ""_su8);

  ExpectReadFailure<BrOnExnImmediate>(
      {{0, "br_on_exn"}, {1, "exception index"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST(ReadTest, BrTableImmediate) {
  ExpectRead<BrTableImmediate>(BrTableImmediate{{}, 0}, "\x00\x00"_su8);
  ExpectRead<BrTableImmediate>(BrTableImmediate{{1, 2}, 3},
                               "\x02\x01\x02\x03"_su8);
}

TEST(ReadTest, BrTableImmediate_PastEnd) {
  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {0, "targets"}, {0, "count"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {1, "default target"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST(ReadTest, ReadBytes) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 3, features, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(data, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadBytes_Leftovers) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, features, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(data.subspan(0, 2), result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReadTest, ReadBytes_Fail) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x12\x34\x56"_su8;
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, features, errors);
  EXPECT_EQ(nullopt, result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

TEST(ReadTest, CallIndirectImmediate) {
  ExpectRead<CallIndirectImmediate>(CallIndirectImmediate{1, 0},
                                    "\x01\x00"_su8);
  ExpectRead<CallIndirectImmediate>(CallIndirectImmediate{128, 0},
                                    "\x80\x01\x00"_su8);
}

TEST(ReadTest, CallIndirectImmediate_BadReserved) {
  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"},
       {1, "reserved"},
       {2, "Expected reserved byte 0, got 1"}},
      "\x00\x01"_su8);
}

TEST(ReadTest, CallIndirectImmediate_PastEnd) {
  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"}, {0, "type index"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"}, {1, "reserved"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST(ReadTest, Code) {
  // Empty body. This will fail validation, but can still be read.
  ExpectRead<Code>(Code{{}, ""_expr}, "\x01\x00"_su8);

  // Smallest valid empty body.
  ExpectRead<Code>(Code{{}, "\x0b"_expr}, "\x02\x00\x0b"_su8);

  // (func
  //   (local i32 i32 i64 i64 i64)
  //   (nop))
  ExpectRead<Code>(Code{{Locals{2, ValueType::I32}, Locals{3, ValueType::I64}},
                        "\x01\x0b"_expr},
                   "\x07\x02\x02\x7f\x03\x7e\x01\x0b"_su8);
}

TEST(ReadTest, Code_PastEnd) {
  ExpectReadFailure<Code>(
      {{0, "code"}, {0, "length"}, {0, "Unable to read u8"}}, ""_su8);

  ExpectReadFailure<Code>({{0, "code"}, {1, "Length extends past end: 1 > 0"}},
                          "\x01"_su8);

  ExpectReadFailure<Code>(
      {{0, "code"}, {1, "locals vector"}, {2, "Count extends past end: 1 > 0"}},
      "\x01\x01"_su8);
}

TEST(ReadTest, ConstantExpression) {
  // i32.const
  ExpectRead<ConstantExpression>(
      ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
      "\x41\x00\x0b"_su8);

  // i64.const
  ExpectRead<ConstantExpression>(
      ConstantExpression{Instruction{Opcode::I64Const, s64{34359738368}}},
      "\x42\x80\x80\x80\x80\x80\x01\x0b"_su8);

  // f32.const
  ExpectRead<ConstantExpression>(
      ConstantExpression{Instruction{Opcode::F32Const, f32{0}}},
      "\x43\x00\x00\x00\x00\x0b"_su8);

  // f64.const
  ExpectRead<ConstantExpression>(
      ConstantExpression{Instruction{Opcode::F64Const, f64{0}}},
      "\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b"_su8);

  // global.get
  ExpectRead<ConstantExpression>(
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}},
      "\x23\x00\x0b"_su8);
}

TEST(ReadTest, ConstantExpression_NoEnd) {
  // i32.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      "\x41\x00"_su8);

  // i64.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {7, "opcode"}, {7, "Unable to read u8"}},
      "\x42\x80\x80\x80\x80\x80\x01"_su8);

  // f32.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {5, "opcode"}, {5, "Unable to read u8"}},
      "\x43\x00\x00\x00\x00"_su8);

  // f64.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {9, "opcode"}, {9, "Unable to read u8"}},
      "\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8);

  // global.get
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      "\x23\x00"_su8);
}

TEST(ReadTest, ConstantExpression_TooLong) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {3, "Expected end instruction"}},
      "\x41\x00\x01\x0b"_su8);
}

TEST(ReadTest, ConstantExpression_InvalidInstruction) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {0, "opcode"}, {1, "Unknown opcode: 6"}},
      "\x06"_su8);
}

TEST(ReadTest, ConstantExpression_IllegalInstruction) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"},
       {1, "Illegal instruction in constant expression: unreachable"}},
      "\x00"_su8);
}

TEST(ReadTest, ConstantExpression_PastEnd) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
      ""_su8);
}

TEST(ReadTest, CopyImmediate) {
  ExpectRead<CopyImmediate>(CopyImmediate{0, 0}, "\x00\x00"_su8);
}

TEST(ReadTest, CopyImmediate_BadReserved) {
  ExpectReadFailure<CopyImmediate>({{0, "copy immediate"},
                                    {0, "reserved"},
                                    {1, "Expected reserved byte 0, got 1"}},
                                   "\x01"_su8);

  ExpectReadFailure<CopyImmediate>({{0, "copy immediate"},
                                    {1, "reserved"},
                                    {2, "Expected reserved byte 0, got 1"}},
                                   "\x00\x01"_su8);
}

TEST(ReadTest, CopyImmediate_PastEnd) {
  ExpectReadFailure<CopyImmediate>(
      {{0, "copy immediate"}, {0, "reserved"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<CopyImmediate>(
      {{0, "copy immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST(ReadTest, ShuffleImmediate) {
  ExpectRead<ShuffleImmediate>(
      ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
}

TEST(ReadTest, ShuffleImmediate_PastEnd) {
  ExpectReadFailure<ShuffleImmediate>(
      {{0, "shuffle immediate"}, {0, "Unable to read u8"}}, ""_su8);

  ExpectReadFailure<ShuffleImmediate>(
      {{0, "shuffle immediate"}, {15, "Unable to read u8"}},
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
}

TEST(ReadTest, ReadCount) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x01\x00\x00\x00"_su8;
  SpanU8 copy = data;
  auto result = ReadCount(&copy, features, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReadTest, ReadCount_PastEnd) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x05\x00\x00\x00"_su8;
  SpanU8 copy = data;
  auto result = ReadCount(&copy, features, errors);
  ExpectError({{1, "Count extends past end: 5 > 3"}}, errors, data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReadTest, DataSegment_MVP) {
  ExpectRead<DataSegment>(
      DataSegment{1, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  "wxyz"_su8},
      "\x01\x42\x01\x0b\x04wxyz"_su8);
}

TEST(ReadTest, DataSegment_MVP_PastEnd) {
  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {0, "memory index"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<DataSegment>({{0, "data segment"},
                                  {1, "offset"},
                                  {1, "constant expression"},
                                  {1, "opcode"},
                                  {1, "Unable to read u8"}},
                                 "\x00"_su8);

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {4, "length"}, {4, "Unable to read u8"}},
      "\x00\x41\x00\x0b"_su8);

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {5, "Length extends past end: 2 > 0"}},
      "\x00\x41\x00\x0b\x02"_su8);
}

TEST(ReadTest, DataSegment_BulkMemory) {
  Features features;
  features.enable_bulk_memory();

  ExpectRead<DataSegment>(DataSegment{"wxyz"_su8}, "\x01\x04wxyz"_su8,
                          features);

  ExpectRead<DataSegment>(
      DataSegment{1u, ConstantExpression{Instruction{Opcode::I32Const, s32{2}}},
                  "xyz"_su8},
      "\x02\x01\x41\x02\x0b\x03xyz"_su8, features);
}

TEST(ReadTest, DataSegment_BulkMemory_BadFlags) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<DataSegment>({{0, "data segment"}, {1, "Unknown flags: 3"}},
                                 "\x03"_su8, features);
}

TEST(ReadTest, DataSegment_BulkMemory_PastEnd) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {0, "flags"}, {0, "Unable to read u8"}}, ""_su8,
      features);

  // Passive.
  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {1, "length"}, {1, "Unable to read u8"}},
      "\x01"_su8, features);

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {2, "Length extends past end: 1 > 0"}},
      "\x01\x01"_su8, features);

  // Active w/ memory index.
  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {1, "memory index"}, {1, "Unable to read u8"}},
      "\x02"_su8, features);

  ExpectReadFailure<DataSegment>({{0, "data segment"},
                                  {2, "offset"},
                                  {2, "constant expression"},
                                  {2, "opcode"},
                                  {2, "Unable to read u8"}},
                                 "\x02\x00"_su8, features);

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {5, "length"}, {5, "Unable to read u8"}},
      "\x02\x00\x41\x00\x0b"_su8, features);

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {6, "Length extends past end: 1 > 0"}},
      "\x02\x00\x41\x00\x0b\x01"_su8, features);
}

TEST(ReadTest, ElementExpression) {
  Features features;
  features.enable_bulk_memory();

  // ref.null
  ExpectRead<ElementExpression>(ElementExpression{Instruction{Opcode::RefNull}},
                                "\xd0\x0b"_su8, features);

  // ref.func 2
  ExpectRead<ElementExpression>(
      ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}},
      "\xd2\x02\x0b"_su8, features);
}

TEST(ReadTest, ElementExpression_NoEnd) {
  Features features;
  features.enable_bulk_memory();

  // ref.null
  ExpectReadFailure<ElementExpression>(
      {{0, "element expression"}, {1, "opcode"}, {1, "Unable to read u8"}},
      "\xd0"_su8, features);

  // ref.func
  ExpectReadFailure<ElementExpression>(
      {{0, "element expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      "\xd2\x00"_su8, features);
}

TEST(ReadTest, ElementExpression_TooLong) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<ElementExpression>(
      {{0, "element expression"}, {2, "Expected end instruction"}},
      "\xd0\x00"_su8, features);
}

TEST(ReadTest, ElementExpression_InvalidInstruction) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<ElementExpression>(
      {{0, "element expression"}, {0, "opcode"}, {1, "Unknown opcode: 6"}},
      "\x06"_su8, features);
}

TEST(ReadTest, ElementExpression_IllegalInstruction) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<ElementExpression>(
      {{0, "element expression"},
       {1, "Illegal instruction in element expression: ref.is_null"}},
      "\xd1"_su8, features);
}

TEST(ReadTest, ElementExpression_PastEnd) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<ElementExpression>(
      {{0, "element expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
      ""_su8, features);
}

TEST(ReadTest, ElementSegment_MVP) {
  ExpectRead<ElementSegment>(
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{1}}},
                     {1, 2, 3}},
      "\x00\x41\x01\x0b\x03\x01\x02\x03"_su8);
}

TEST(ReadTest, ElementSegment_MVP_PastEnd) {
  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {0, "table index"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {1, "offset"},
                                     {1, "constant expression"},
                                     {1, "opcode"},
                                     {1, "Unable to read u8"}},
                                    "\x00"_su8);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {4, "initializers"},
                                     {4, "count"},
                                     {4, "Unable to read u8"}},
                                    "\x00\x23\x00\x0b"_su8);
}

TEST(ReadTest, ElementSegment_BulkMemory) {
  Features features;
  features.enable_bulk_memory();

  // Active element segment with non-zero table index.
  ExpectRead<ElementSegment>(
      ElementSegment{1u,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{2}}},
                     {3, 4}},
      "\x02\x01\x41\x02\x0b\x02\x03\x04"_su8, features);

  // Passive element segment.
  ExpectRead<ElementSegment>(
      ElementSegment{
          ElementType::Funcref,
          {ElementExpression{Instruction{Opcode::RefFunc, Index{1u}}},
           ElementExpression{Instruction{Opcode::RefNull}}}},
      "\x01\x70\x02\xd2\x01\x0b\xd0\x0b"_su8, features);
}

TEST(ReadTest, ElementSegment_BulkMemory_BadFlags) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {1, "Unknown flags: 3"}}, "\x03"_su8, features);
}

TEST(ReadTest, ElementSegment_BulkMemory_PastEnd) {
  Features features;
  features.enable_bulk_memory();

  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {0, "flags"}, {0, "Unable to read u8"}}, ""_su8,
      features);

  // Passive.
  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {1, "element type"}, {1, "Unable to read u8"}},
      "\x01"_su8, features);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {2, "initializers"},
                                     {2, "count"},
                                     {2, "Unable to read u8"}},
                                    "\x01\x70"_su8, features);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {2, "initializers"},
                                     {3, "Count extends past end: 1 > 0"}},
                                    "\x01\x70\x01"_su8, features);

  // Active w/ table index.
  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {1, "table index"}, {1, "Unable to read u8"}},
      "\x02"_su8, features);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {2, "offset"},
                                     {2, "constant expression"},
                                     {2, "opcode"},
                                     {2, "Unable to read u8"}},
                                    "\x02\x00"_su8, features);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {5, "initializers"},
                                     {5, "count"},
                                     {5, "Unable to read u8"}},
                                    "\x02\x00\x41\x00\x0b"_su8, features);

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {5, "initializers"},
                                     {6, "Count extends past end: 1 > 0"}},
                                    "\x02\x00\x41\x00\x0b\x01"_su8, features);
}

TEST(ReadTest, ElementType) {
  ExpectRead<ElementType>(ElementType::Funcref, "\x70"_su8);
}

TEST(ReadTest, ElementType_Unknown) {
  ExpectReadFailure<ElementType>(
      {{0, "element type"}, {1, "Unknown element type: 0"}}, "\x00"_su8);

  // Overlong encoding is not allowed.
  ExpectReadFailure<ElementType>(
      {{0, "element type"}, {1, "Unknown element type: 240"}}, "\xf0\x7f"_su8);
}

TEST(ReadTest, Export) {
  ExpectRead<Export>(Export{ExternalKind::Function, "hi", 3},
                     "\x02hi\x00\x03"_su8);
  ExpectRead<Export>(Export{ExternalKind::Table, "", 1000},
                     "\x00\x01\xe8\x07"_su8);
  ExpectRead<Export>(Export{ExternalKind::Memory, "mem", 0},
                     "\x03mem\x02\x00"_su8);
  ExpectRead<Export>(Export{ExternalKind::Global, "g", 1}, "\x01g\x03\x01"_su8);
}

TEST(ReadTest, Export_PastEnd) {
  ExpectReadFailure<Export>(
      {{0, "export"}, {0, "name"}, {0, "length"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<Export>(
      {{0, "export"}, {1, "external kind"}, {1, "Unable to read u8"}},
      "\x00"_su8);

  ExpectReadFailure<Export>(
      {{0, "export"}, {2, "index"}, {2, "Unable to read u8"}}, "\x00\x00"_su8);
}

TEST(ReadTest, ExternalKind) {
  ExpectRead<ExternalKind>(ExternalKind::Function, "\x00"_su8);
  ExpectRead<ExternalKind>(ExternalKind::Table, "\x01"_su8);
  ExpectRead<ExternalKind>(ExternalKind::Memory, "\x02"_su8);
  ExpectRead<ExternalKind>(ExternalKind::Global, "\x03"_su8);
}

TEST(ReadTest, ExternalKind_Unknown) {
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 4"}}, "\x04"_su8);

  // Overlong encoding is not allowed.
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 132"}},
      "\x84\x00"_su8);
}

TEST(ReadTest, F32) {
  ExpectRead<f32>(0.0f, "\x00\x00\x00\x00"_su8);
  ExpectRead<f32>(-1.0f, "\x00\x00\x80\xbf"_su8);
  ExpectRead<f32>(1234567.0f, "\x38\xb4\x96\x49"_su8);
  ExpectRead<f32>(INFINITY, "\x00\x00\x80\x7f"_su8);
  ExpectRead<f32>(-INFINITY, "\x00\x00\x80\xff"_su8);

  // NaN
  {
    auto data = "\x00\x00\xc0\x7f"_su8;
    Features features;
    TestErrors errors;
    auto result = Read<f32>(&data, features, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReadTest, F32_PastEnd) {
  ExpectReadFailure<f32>({{0, "f32"}, {0, "Unable to read 4 bytes"}},
                         "\x00\x00\x00"_su8);
}

TEST(ReadTest, F64) {
  ExpectRead<f64>(0.0, "\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  ExpectRead<f64>(-1.0, "\x00\x00\x00\x00\x00\x00\xf0\xbf"_su8);
  ExpectRead<f64>(111111111111111, "\xc0\x71\xbc\x93\x84\x43\xd9\x42"_su8);
  ExpectRead<f64>(INFINITY, "\x00\x00\x00\x00\x00\x00\xf0\x7f"_su8);
  ExpectRead<f64>(-INFINITY, "\x00\x00\x00\x00\x00\x00\xf0\xff"_su8);

  // NaN
  {
    auto data = "\x00\x00\x00\x00\x00\x00\xf8\x7f"_su8;
    Features features;
    TestErrors errors;
    auto result = Read<f64>(&data, features, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReadTest, F64_PastEnd) {
  ExpectReadFailure<f64>({{0, "f64"}, {0, "Unable to read 8 bytes"}},
                         "\x00\x00\x00\x00\x00\x00\x00"_su8);
}

TEST(ReadTest, Function) {
  ExpectRead<Function>(Function{1}, "\x01"_su8);
}

TEST(ReadTest, Function_PastEnd) {
  ExpectReadFailure<Function>(
      {{0, "function"}, {0, "type index"}, {0, "Unable to read u8"}}, ""_su8);
}

TEST(ReadTest, FunctionType) {
  ExpectRead<FunctionType>(FunctionType{{}, {}}, "\x00\x00"_su8);
  ExpectRead<FunctionType>(
      FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}},
      "\x02\x7f\x7e\x01\x7c"_su8);
}

TEST(ReadTest, FunctionType_PastEnd) {
  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {0, "count"},
                                   {0, "Unable to read u8"}},
                                  ""_su8);

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {1, "Count extends past end: 1 > 0"}},
                                  "\x01"_su8);

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {1, "count"},
                                   {1, "Unable to read u8"}},
                                  "\x00"_su8);

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {2, "Count extends past end: 1 > 0"}},
                                  "\x00\x01"_su8);
}

TEST(ReadTest, Global) {
  // i32 global with i64.const constant expression. This will fail validation
  // but still can be successfully parsed.
  ExpectRead<Global>(
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I64Const, s64{0}}}},
      "\x7f\x01\x42\x00\x0b"_su8);
}

TEST(ReadTest, Global_PastEnd) {
  ExpectReadFailure<Global>({{0, "global"},
                             {0, "global type"},
                             {0, "value type"},
                             {0, "Unable to read u8"}},
                            ""_su8);

  ExpectReadFailure<Global>({{0, "global"},
                             {2, "constant expression"},
                             {2, "opcode"},
                             {2, "Unable to read u8"}},
                            "\x7f\x00"_su8);
}

TEST(ReadTest, GlobalType) {
  ExpectRead<GlobalType>(GlobalType{ValueType::I32, Mutability::Const},
                         "\x7f\x00"_su8);
  ExpectRead<GlobalType>(GlobalType{ValueType::F32, Mutability::Var},
                         "\x7d\x01"_su8);
}

TEST(ReadTest, GlobalType_PastEnd) {
  ExpectReadFailure<GlobalType>(
      {{0, "global type"}, {0, "value type"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<GlobalType>(
      {{0, "global type"}, {1, "mutability"}, {1, "Unable to read u8"}},
      "\x7f"_su8);
}

TEST(ReadTest, Import) {
  ExpectRead<Import>(Import{"a", "func", 11u},
                     "\x01\x61\x04\x66unc\x00\x0b"_su8);

  ExpectRead<Import>(
      Import{"b", "table", TableType{Limits{1}, ElementType::Funcref}},
      "\x01\x62\x05table\x01\x70\x00\x01"_su8);

  ExpectRead<Import>(Import{"c", "memory", MemoryType{Limits{0, 2}}},
                     "\x01\x63\x06memory\x02\x01\x00\x02"_su8);

  ExpectRead<Import>(
      Import{"d", "global", GlobalType{ValueType::I32, Mutability::Const}},
      "\x01\x64\x06global\x03\x7f\x00"_su8);
}

TEST(ReadTest, ImportType_PastEnd) {
  ExpectReadFailure<Import>({{0, "import"},
                             {0, "module name"},
                             {0, "length"},
                             {0, "Unable to read u8"}},
                            ""_su8);

  ExpectReadFailure<Import>({{0, "import"},
                             {1, "field name"},
                             {1, "length"},
                             {1, "Unable to read u8"}},
                            "\x00"_su8);

  ExpectReadFailure<Import>(
      {{0, "import"}, {2, "external kind"}, {2, "Unable to read u8"}},
      "\x00\x00"_su8);

  ExpectReadFailure<Import>(
      {{0, "import"}, {3, "function index"}, {3, "Unable to read u8"}},
      "\x00\x00\x00"_su8);

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "table type"},
                             {3, "element type"},
                             {3, "Unable to read u8"}},
                            "\x00\x00\x01"_su8);

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "memory type"},
                             {3, "limits"},
                             {3, "flags"},
                             {3, "Unable to read u8"}},
                            "\x00\x00\x02"_su8);

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "global type"},
                             {3, "value type"},
                             {3, "Unable to read u8"}},
                            "\x00\x00\x03"_su8);
}

TEST(ReadTest, IndirectNameAssoc) {
  ExpectRead<IndirectNameAssoc>(
      IndirectNameAssoc{100u, {{0u, "zero"}, {1u, "one"}}},
      "\x64"             // Index.
      "\x02"             // Count.
      "\x00\x04zero"     // 0 "zero"
      "\x01\x03one"_su8  // 1 "one"
  );
}

TEST(ReadTest, IndirectNameAssoc_PastEnd) {
  ExpectReadFailure<IndirectNameAssoc>(
      {{0, "indirect name assoc"}, {0, "index"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<IndirectNameAssoc>({{0, "indirect name assoc"},
                                        {1, "name map"},
                                        {1, "count"},
                                        {1, "Unable to read u8"}},
                                       "\x00"_su8);

  ExpectReadFailure<IndirectNameAssoc>({{0, "indirect name assoc"},
                                        {1, "name map"},
                                        {2, "Count extends past end: 1 > 0"}},
                                       "\x00\x01"_su8);
}

TEST(ReadTest, InitImmediate) {
  ExpectRead<InitImmediate>(InitImmediate{1, 0}, "\x01\x00"_su8);
  ExpectRead<InitImmediate>(InitImmediate{128, 0}, "\x80\x01\x00"_su8);
}

TEST(ReadTest, InitImmediate_BadReserved) {
  ExpectReadFailure<InitImmediate>({{0, "init immediate"},
                                    {1, "reserved"},
                                    {2, "Expected reserved byte 0, got 1"}},
                                   "\x00\x01"_su8);
}

TEST(ReadTest, InitImmediate_PastEnd) {
  ExpectReadFailure<InitImmediate>(
      {{0, "init immediate"}, {0, "segment index"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<InitImmediate>(
      {{0, "init immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
      "\x01"_su8);
}

TEST(ReadTest, Instruction) {
  using I = Instruction;
  using O = Opcode;
  using MemArg = MemArgImmediate;

  ExpectRead<I>(I{O::Unreachable}, "\x00"_su8);
  ExpectRead<I>(I{O::Nop}, "\x01"_su8);
  ExpectRead<I>(I{O::Block, BlockType::I32}, "\x02\x7f"_su8);
  ExpectRead<I>(I{O::Loop, BlockType::Void}, "\x03\x40"_su8);
  ExpectRead<I>(I{O::If, BlockType::F64}, "\x04\x7c"_su8);
  ExpectRead<I>(I{O::Else}, "\x05"_su8);
  ExpectRead<I>(I{O::End}, "\x0b"_su8);
  ExpectRead<I>(I{O::Br, Index{1}}, "\x0c\x01"_su8);
  ExpectRead<I>(I{O::BrIf, Index{2}}, "\x0d\x02"_su8);
  ExpectRead<I>(I{O::BrTable, BrTableImmediate{{3, 4, 5}, 6}},
                "\x0e\x03\x03\x04\x05\x06"_su8);
  ExpectRead<I>(I{O::Return}, "\x0f"_su8);
  ExpectRead<I>(I{O::Call, Index{7}}, "\x10\x07"_su8);
  ExpectRead<I>(I{O::CallIndirect, CallIndirectImmediate{8, 0}},
                "\x11\x08\x00"_su8);
  ExpectRead<I>(I{O::Drop}, "\x1a"_su8);
  ExpectRead<I>(I{O::Select}, "\x1b"_su8);
  ExpectRead<I>(I{O::LocalGet, Index{5}}, "\x20\x05"_su8);
  ExpectRead<I>(I{O::LocalSet, Index{6}}, "\x21\x06"_su8);
  ExpectRead<I>(I{O::LocalTee, Index{7}}, "\x22\x07"_su8);
  ExpectRead<I>(I{O::GlobalGet, Index{8}}, "\x23\x08"_su8);
  ExpectRead<I>(I{O::GlobalSet, Index{9}}, "\x24\x09"_su8);
  ExpectRead<I>(I{O::I32Load, MemArg{10, 11}}, "\x28\x0a\x0b"_su8);
  ExpectRead<I>(I{O::I64Load, MemArg{12, 13}}, "\x29\x0c\x0d"_su8);
  ExpectRead<I>(I{O::F32Load, MemArg{14, 15}}, "\x2a\x0e\x0f"_su8);
  ExpectRead<I>(I{O::F64Load, MemArg{16, 17}}, "\x2b\x10\x11"_su8);
  ExpectRead<I>(I{O::I32Load8S, MemArg{18, 19}}, "\x2c\x12\x13"_su8);
  ExpectRead<I>(I{O::I32Load8U, MemArg{20, 21}}, "\x2d\x14\x15"_su8);
  ExpectRead<I>(I{O::I32Load16S, MemArg{22, 23}}, "\x2e\x16\x17"_su8);
  ExpectRead<I>(I{O::I32Load16U, MemArg{24, 25}}, "\x2f\x18\x19"_su8);
  ExpectRead<I>(I{O::I64Load8S, MemArg{26, 27}}, "\x30\x1a\x1b"_su8);
  ExpectRead<I>(I{O::I64Load8U, MemArg{28, 29}}, "\x31\x1c\x1d"_su8);
  ExpectRead<I>(I{O::I64Load16S, MemArg{30, 31}}, "\x32\x1e\x1f"_su8);
  ExpectRead<I>(I{O::I64Load16U, MemArg{32, 33}}, "\x33\x20\x21"_su8);
  ExpectRead<I>(I{O::I64Load32S, MemArg{34, 35}}, "\x34\x22\x23"_su8);
  ExpectRead<I>(I{O::I64Load32U, MemArg{36, 37}}, "\x35\x24\x25"_su8);
  ExpectRead<I>(I{O::I32Store, MemArg{38, 39}}, "\x36\x26\x27"_su8);
  ExpectRead<I>(I{O::I64Store, MemArg{40, 41}}, "\x37\x28\x29"_su8);
  ExpectRead<I>(I{O::F32Store, MemArg{42, 43}}, "\x38\x2a\x2b"_su8);
  ExpectRead<I>(I{O::F64Store, MemArg{44, 45}}, "\x39\x2c\x2d"_su8);
  ExpectRead<I>(I{O::I32Store8, MemArg{46, 47}}, "\x3a\x2e\x2f"_su8);
  ExpectRead<I>(I{O::I32Store16, MemArg{48, 49}}, "\x3b\x30\x31"_su8);
  ExpectRead<I>(I{O::I64Store8, MemArg{50, 51}}, "\x3c\x32\x33"_su8);
  ExpectRead<I>(I{O::I64Store16, MemArg{52, 53}}, "\x3d\x34\x35"_su8);
  ExpectRead<I>(I{O::I64Store32, MemArg{54, 55}}, "\x3e\x36\x37"_su8);
  ExpectRead<I>(I{O::MemorySize, u8{0}}, "\x3f\x00"_su8);
  ExpectRead<I>(I{O::MemoryGrow, u8{0}}, "\x40\x00"_su8);
  ExpectRead<I>(I{O::I32Const, s32{0}}, "\x41\x00"_su8);
  ExpectRead<I>(I{O::I64Const, s64{0}}, "\x42\x00"_su8);
  ExpectRead<I>(I{O::F32Const, f32{0}}, "\x43\x00\x00\x00\x00"_su8);
  ExpectRead<I>(I{O::F64Const, f64{0}},
                "\x44\x00\x00\x00\x00\x00\x00\x00\x00"_su8);
  ExpectRead<I>(I{O::I32Eqz}, "\x45"_su8);
  ExpectRead<I>(I{O::I32Eq}, "\x46"_su8);
  ExpectRead<I>(I{O::I32Ne}, "\x47"_su8);
  ExpectRead<I>(I{O::I32LtS}, "\x48"_su8);
  ExpectRead<I>(I{O::I32LtU}, "\x49"_su8);
  ExpectRead<I>(I{O::I32GtS}, "\x4a"_su8);
  ExpectRead<I>(I{O::I32GtU}, "\x4b"_su8);
  ExpectRead<I>(I{O::I32LeS}, "\x4c"_su8);
  ExpectRead<I>(I{O::I32LeU}, "\x4d"_su8);
  ExpectRead<I>(I{O::I32GeS}, "\x4e"_su8);
  ExpectRead<I>(I{O::I32GeU}, "\x4f"_su8);
  ExpectRead<I>(I{O::I64Eqz}, "\x50"_su8);
  ExpectRead<I>(I{O::I64Eq}, "\x51"_su8);
  ExpectRead<I>(I{O::I64Ne}, "\x52"_su8);
  ExpectRead<I>(I{O::I64LtS}, "\x53"_su8);
  ExpectRead<I>(I{O::I64LtU}, "\x54"_su8);
  ExpectRead<I>(I{O::I64GtS}, "\x55"_su8);
  ExpectRead<I>(I{O::I64GtU}, "\x56"_su8);
  ExpectRead<I>(I{O::I64LeS}, "\x57"_su8);
  ExpectRead<I>(I{O::I64LeU}, "\x58"_su8);
  ExpectRead<I>(I{O::I64GeS}, "\x59"_su8);
  ExpectRead<I>(I{O::I64GeU}, "\x5a"_su8);
  ExpectRead<I>(I{O::F32Eq}, "\x5b"_su8);
  ExpectRead<I>(I{O::F32Ne}, "\x5c"_su8);
  ExpectRead<I>(I{O::F32Lt}, "\x5d"_su8);
  ExpectRead<I>(I{O::F32Gt}, "\x5e"_su8);
  ExpectRead<I>(I{O::F32Le}, "\x5f"_su8);
  ExpectRead<I>(I{O::F32Ge}, "\x60"_su8);
  ExpectRead<I>(I{O::F64Eq}, "\x61"_su8);
  ExpectRead<I>(I{O::F64Ne}, "\x62"_su8);
  ExpectRead<I>(I{O::F64Lt}, "\x63"_su8);
  ExpectRead<I>(I{O::F64Gt}, "\x64"_su8);
  ExpectRead<I>(I{O::F64Le}, "\x65"_su8);
  ExpectRead<I>(I{O::F64Ge}, "\x66"_su8);
  ExpectRead<I>(I{O::I32Clz}, "\x67"_su8);
  ExpectRead<I>(I{O::I32Ctz}, "\x68"_su8);
  ExpectRead<I>(I{O::I32Popcnt}, "\x69"_su8);
  ExpectRead<I>(I{O::I32Add}, "\x6a"_su8);
  ExpectRead<I>(I{O::I32Sub}, "\x6b"_su8);
  ExpectRead<I>(I{O::I32Mul}, "\x6c"_su8);
  ExpectRead<I>(I{O::I32DivS}, "\x6d"_su8);
  ExpectRead<I>(I{O::I32DivU}, "\x6e"_su8);
  ExpectRead<I>(I{O::I32RemS}, "\x6f"_su8);
  ExpectRead<I>(I{O::I32RemU}, "\x70"_su8);
  ExpectRead<I>(I{O::I32And}, "\x71"_su8);
  ExpectRead<I>(I{O::I32Or}, "\x72"_su8);
  ExpectRead<I>(I{O::I32Xor}, "\x73"_su8);
  ExpectRead<I>(I{O::I32Shl}, "\x74"_su8);
  ExpectRead<I>(I{O::I32ShrS}, "\x75"_su8);
  ExpectRead<I>(I{O::I32ShrU}, "\x76"_su8);
  ExpectRead<I>(I{O::I32Rotl}, "\x77"_su8);
  ExpectRead<I>(I{O::I32Rotr}, "\x78"_su8);
  ExpectRead<I>(I{O::I64Clz}, "\x79"_su8);
  ExpectRead<I>(I{O::I64Ctz}, "\x7a"_su8);
  ExpectRead<I>(I{O::I64Popcnt}, "\x7b"_su8);
  ExpectRead<I>(I{O::I64Add}, "\x7c"_su8);
  ExpectRead<I>(I{O::I64Sub}, "\x7d"_su8);
  ExpectRead<I>(I{O::I64Mul}, "\x7e"_su8);
  ExpectRead<I>(I{O::I64DivS}, "\x7f"_su8);
  ExpectRead<I>(I{O::I64DivU}, "\x80"_su8);
  ExpectRead<I>(I{O::I64RemS}, "\x81"_su8);
  ExpectRead<I>(I{O::I64RemU}, "\x82"_su8);
  ExpectRead<I>(I{O::I64And}, "\x83"_su8);
  ExpectRead<I>(I{O::I64Or}, "\x84"_su8);
  ExpectRead<I>(I{O::I64Xor}, "\x85"_su8);
  ExpectRead<I>(I{O::I64Shl}, "\x86"_su8);
  ExpectRead<I>(I{O::I64ShrS}, "\x87"_su8);
  ExpectRead<I>(I{O::I64ShrU}, "\x88"_su8);
  ExpectRead<I>(I{O::I64Rotl}, "\x89"_su8);
  ExpectRead<I>(I{O::I64Rotr}, "\x8a"_su8);
  ExpectRead<I>(I{O::F32Abs}, "\x8b"_su8);
  ExpectRead<I>(I{O::F32Neg}, "\x8c"_su8);
  ExpectRead<I>(I{O::F32Ceil}, "\x8d"_su8);
  ExpectRead<I>(I{O::F32Floor}, "\x8e"_su8);
  ExpectRead<I>(I{O::F32Trunc}, "\x8f"_su8);
  ExpectRead<I>(I{O::F32Nearest}, "\x90"_su8);
  ExpectRead<I>(I{O::F32Sqrt}, "\x91"_su8);
  ExpectRead<I>(I{O::F32Add}, "\x92"_su8);
  ExpectRead<I>(I{O::F32Sub}, "\x93"_su8);
  ExpectRead<I>(I{O::F32Mul}, "\x94"_su8);
  ExpectRead<I>(I{O::F32Div}, "\x95"_su8);
  ExpectRead<I>(I{O::F32Min}, "\x96"_su8);
  ExpectRead<I>(I{O::F32Max}, "\x97"_su8);
  ExpectRead<I>(I{O::F32Copysign}, "\x98"_su8);
  ExpectRead<I>(I{O::F64Abs}, "\x99"_su8);
  ExpectRead<I>(I{O::F64Neg}, "\x9a"_su8);
  ExpectRead<I>(I{O::F64Ceil}, "\x9b"_su8);
  ExpectRead<I>(I{O::F64Floor}, "\x9c"_su8);
  ExpectRead<I>(I{O::F64Trunc}, "\x9d"_su8);
  ExpectRead<I>(I{O::F64Nearest}, "\x9e"_su8);
  ExpectRead<I>(I{O::F64Sqrt}, "\x9f"_su8);
  ExpectRead<I>(I{O::F64Add}, "\xa0"_su8);
  ExpectRead<I>(I{O::F64Sub}, "\xa1"_su8);
  ExpectRead<I>(I{O::F64Mul}, "\xa2"_su8);
  ExpectRead<I>(I{O::F64Div}, "\xa3"_su8);
  ExpectRead<I>(I{O::F64Min}, "\xa4"_su8);
  ExpectRead<I>(I{O::F64Max}, "\xa5"_su8);
  ExpectRead<I>(I{O::F64Copysign}, "\xa6"_su8);
  ExpectRead<I>(I{O::I32WrapI64}, "\xa7"_su8);
  ExpectRead<I>(I{O::I32TruncF32S}, "\xa8"_su8);
  ExpectRead<I>(I{O::I32TruncF32U}, "\xa9"_su8);
  ExpectRead<I>(I{O::I32TruncF64S}, "\xaa"_su8);
  ExpectRead<I>(I{O::I32TruncF64U}, "\xab"_su8);
  ExpectRead<I>(I{O::I64ExtendI32S}, "\xac"_su8);
  ExpectRead<I>(I{O::I64ExtendI32U}, "\xad"_su8);
  ExpectRead<I>(I{O::I64TruncF32S}, "\xae"_su8);
  ExpectRead<I>(I{O::I64TruncF32U}, "\xaf"_su8);
  ExpectRead<I>(I{O::I64TruncF64S}, "\xb0"_su8);
  ExpectRead<I>(I{O::I64TruncF64U}, "\xb1"_su8);
  ExpectRead<I>(I{O::F32ConvertI32S}, "\xb2"_su8);
  ExpectRead<I>(I{O::F32ConvertI32U}, "\xb3"_su8);
  ExpectRead<I>(I{O::F32ConvertI64S}, "\xb4"_su8);
  ExpectRead<I>(I{O::F32ConvertI64U}, "\xb5"_su8);
  ExpectRead<I>(I{O::F32DemoteF64}, "\xb6"_su8);
  ExpectRead<I>(I{O::F64ConvertI32S}, "\xb7"_su8);
  ExpectRead<I>(I{O::F64ConvertI32U}, "\xb8"_su8);
  ExpectRead<I>(I{O::F64ConvertI64S}, "\xb9"_su8);
  ExpectRead<I>(I{O::F64ConvertI64U}, "\xba"_su8);
  ExpectRead<I>(I{O::F64PromoteF32}, "\xbb"_su8);
  ExpectRead<I>(I{O::I32ReinterpretF32}, "\xbc"_su8);
  ExpectRead<I>(I{O::I64ReinterpretF64}, "\xbd"_su8);
  ExpectRead<I>(I{O::F32ReinterpretI32}, "\xbe"_su8);
  ExpectRead<I>(I{O::F64ReinterpretI64}, "\xbf"_su8);
}

TEST(ReadTest, Instruction_BadMemoryReserved) {
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      "\x3f\x01"_su8);
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      "\x40\x01"_su8);
}

TEST(ReadTest, Instruction_exceptions) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_exceptions();

  ExpectRead<I>(I{O::Try, BlockType::Void}, "\x06\x40"_su8, features);
  ExpectRead<I>(I{O::Catch}, "\x07"_su8, features);
  ExpectRead<I>(I{O::Throw, Index{0}}, "\x08\x00"_su8, features);
  ExpectRead<I>(I{O::Rethrow}, "\x09"_su8, features);
  ExpectRead<I>(I{O::BrOnExn, BrOnExnImmediate{1, 2}}, "\x0a\x01\x02"_su8,
                features);
}

TEST(ReadTest, Instruction_tail_call) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_tail_call();

  ExpectRead<I>(I{O::ReturnCall, Index{0}}, "\x12\x00"_su8, features);
  ExpectRead<I>(I{O::ReturnCallIndirect, CallIndirectImmediate{8, 0}},
                "\x13\x08\x00"_su8, features);
}

TEST(ReadTest, Instruction_sign_extension) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_sign_extension();

  ExpectRead<I>(I{O::I32Extend8S}, "\xc0"_su8, features);
  ExpectRead<I>(I{O::I32Extend16S}, "\xc1"_su8, features);
  ExpectRead<I>(I{O::I64Extend8S}, "\xc2"_su8, features);
  ExpectRead<I>(I{O::I64Extend16S}, "\xc3"_su8, features);
  ExpectRead<I>(I{O::I64Extend32S}, "\xc4"_su8, features);
}

TEST(ReadTest, Instruction_reference_types) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_reference_types();

  ExpectRead<I>(I{O::TableGet, Index{0}}, "\x25\x00"_su8, features);
  ExpectRead<I>(I{O::TableSet, Index{0}}, "\x26\x00"_su8, features);
  ExpectRead<I>(I{O::TableGrow, Index{0}}, "\xfc\x0f\x00"_su8, features);
  ExpectRead<I>(I{O::TableSize, Index{0}}, "\xfc\x10\x00"_su8, features);
  ExpectRead<I>(I{O::RefNull}, "\xd0"_su8, features);
  ExpectRead<I>(I{O::RefIsNull}, "\xd1"_su8, features);
}

TEST(ReadTest, Instruction_function_references) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_function_references();

  ExpectRead<I>(I{O::RefFunc, Index{0}}, "\xd2\x00"_su8, features);
}

TEST(ReadTest, Instruction_saturating_float_to_int) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_saturating_float_to_int();

  ExpectRead<I>(I{O::I32TruncSatF32S}, "\xfc\x00"_su8, features);
  ExpectRead<I>(I{O::I32TruncSatF32U}, "\xfc\x01"_su8, features);
  ExpectRead<I>(I{O::I32TruncSatF64S}, "\xfc\x02"_su8, features);
  ExpectRead<I>(I{O::I32TruncSatF64U}, "\xfc\x03"_su8, features);
  ExpectRead<I>(I{O::I64TruncSatF32S}, "\xfc\x04"_su8, features);
  ExpectRead<I>(I{O::I64TruncSatF32U}, "\xfc\x05"_su8, features);
  ExpectRead<I>(I{O::I64TruncSatF64S}, "\xfc\x06"_su8, features);
  ExpectRead<I>(I{O::I64TruncSatF64U}, "\xfc\x07"_su8, features);
}

TEST(ReadTest, Instruction_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_bulk_memory();

  ExpectRead<I>(I{O::MemoryInit, InitImmediate{1, 0}}, "\xfc\x08\x01\x00"_su8,
                features);
  ExpectRead<I>(I{O::DataDrop, Index{2}}, "\xfc\x09\x02"_su8, features);
  ExpectRead<I>(I{O::MemoryCopy, CopyImmediate{0, 0}}, "\xfc\x0a\x00\x00"_su8,
                features);
  ExpectRead<I>(I{O::MemoryFill, u8{0}}, "\xfc\x0b\x00"_su8, features);
  ExpectRead<I>(I{O::TableInit, InitImmediate{3, 0}}, "\xfc\x0c\x03\x00"_su8,
                features);
  ExpectRead<I>(I{O::ElemDrop, Index{4}}, "\xfc\x0d\x04"_su8, features);
  ExpectRead<I>(I{O::TableCopy, CopyImmediate{0, 0}}, "\xfc\x0e\x00\x00"_su8,
                features);
}

TEST(ReadTest, Instruction_simd) {
  using I = Instruction;
  using O = Opcode;

  Features f;
  f.enable_simd();

  ExpectRead<I>(I{O::V128Load, MemArgImmediate{1, 2}}, "\xfd\x00\x01\x02"_su8,
                f);
  ExpectRead<I>(I{O::V128Store, MemArgImmediate{3, 4}}, "\xfd\x01\x03\x04"_su8,
                f);
  ExpectRead<I>(I{O::V128Const, v128{u64{5}, u64{6}}},
                "\xfd\x02\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00"
                "\x00\x00\x00\x00\x00\x00"_su8,
                f);
  ExpectRead<I>(I{O::V8X16Shuffle, ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                     0, 0, 0, 0, 0, 0, 0}}},
                "\xfd\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00"_su8,
                f);
  ExpectRead<I>(I{O::I8X16Splat}, "\xfd\x04"_su8, f);
  ExpectRead<I>(I{O::I8X16ExtractLaneS, u8{0}}, "\xfd\x05\x00"_su8, f);
  ExpectRead<I>(I{O::I8X16ExtractLaneU, u8{0}}, "\xfd\x06\x00"_su8, f);
  ExpectRead<I>(I{O::I8X16ReplaceLane, u8{0}}, "\xfd\x07\x00"_su8, f);
  ExpectRead<I>(I{O::I16X8Splat}, "\xfd\x08"_su8, f);
  ExpectRead<I>(I{O::I16X8ExtractLaneS, u8{0}}, "\xfd\x09\x00"_su8, f);
  ExpectRead<I>(I{O::I16X8ExtractLaneU, u8{0}}, "\xfd\x0a\x00"_su8, f);
  ExpectRead<I>(I{O::I16X8ReplaceLane, u8{0}}, "\xfd\x0b\x00"_su8, f);
  ExpectRead<I>(I{O::I32X4Splat}, "\xfd\x0c"_su8, f);
  ExpectRead<I>(I{O::I32X4ExtractLane, u8{0}}, "\xfd\x0d\x00"_su8, f);
  ExpectRead<I>(I{O::I32X4ReplaceLane, u8{0}}, "\xfd\x0e\x00"_su8, f);
  ExpectRead<I>(I{O::I64X2Splat}, "\xfd\x0f"_su8, f);
  ExpectRead<I>(I{O::I64X2ExtractLane, u8{0}}, "\xfd\x10\x00"_su8, f);
  ExpectRead<I>(I{O::I64X2ReplaceLane, u8{0}}, "\xfd\x11\x00"_su8, f);
  ExpectRead<I>(I{O::F32X4Splat}, "\xfd\x12"_su8, f);
  ExpectRead<I>(I{O::F32X4ExtractLane, u8{0}}, "\xfd\x13\x00"_su8, f);
  ExpectRead<I>(I{O::F32X4ReplaceLane, u8{0}}, "\xfd\x14\x00"_su8, f);
  ExpectRead<I>(I{O::F64X2Splat}, "\xfd\x15"_su8, f);
  ExpectRead<I>(I{O::F64X2ExtractLane, u8{0}}, "\xfd\x16\x00"_su8, f);
  ExpectRead<I>(I{O::F64X2ReplaceLane, u8{0}}, "\xfd\x17\x00"_su8, f);
  ExpectRead<I>(I{O::I8X16Eq}, "\xfd\x18"_su8, f);
  ExpectRead<I>(I{O::I8X16Ne}, "\xfd\x19"_su8, f);
  ExpectRead<I>(I{O::I8X16LtS}, "\xfd\x1a"_su8, f);
  ExpectRead<I>(I{O::I8X16LtU}, "\xfd\x1b"_su8, f);
  ExpectRead<I>(I{O::I8X16GtS}, "\xfd\x1c"_su8, f);
  ExpectRead<I>(I{O::I8X16GtU}, "\xfd\x1d"_su8, f);
  ExpectRead<I>(I{O::I8X16LeS}, "\xfd\x1e"_su8, f);
  ExpectRead<I>(I{O::I8X16LeU}, "\xfd\x1f"_su8, f);
  ExpectRead<I>(I{O::I8X16GeS}, "\xfd\x20"_su8, f);
  ExpectRead<I>(I{O::I8X16GeU}, "\xfd\x21"_su8, f);
  ExpectRead<I>(I{O::I16X8Eq}, "\xfd\x22"_su8, f);
  ExpectRead<I>(I{O::I16X8Ne}, "\xfd\x23"_su8, f);
  ExpectRead<I>(I{O::I16X8LtS}, "\xfd\x24"_su8, f);
  ExpectRead<I>(I{O::I16X8LtU}, "\xfd\x25"_su8, f);
  ExpectRead<I>(I{O::I16X8GtS}, "\xfd\x26"_su8, f);
  ExpectRead<I>(I{O::I16X8GtU}, "\xfd\x27"_su8, f);
  ExpectRead<I>(I{O::I16X8LeS}, "\xfd\x28"_su8, f);
  ExpectRead<I>(I{O::I16X8LeU}, "\xfd\x29"_su8, f);
  ExpectRead<I>(I{O::I16X8GeS}, "\xfd\x2a"_su8, f);
  ExpectRead<I>(I{O::I16X8GeU}, "\xfd\x2b"_su8, f);
  ExpectRead<I>(I{O::I32X4Eq}, "\xfd\x2c"_su8, f);
  ExpectRead<I>(I{O::I32X4Ne}, "\xfd\x2d"_su8, f);
  ExpectRead<I>(I{O::I32X4LtS}, "\xfd\x2e"_su8, f);
  ExpectRead<I>(I{O::I32X4LtU}, "\xfd\x2f"_su8, f);
  ExpectRead<I>(I{O::I32X4GtS}, "\xfd\x30"_su8, f);
  ExpectRead<I>(I{O::I32X4GtU}, "\xfd\x31"_su8, f);
  ExpectRead<I>(I{O::I32X4LeS}, "\xfd\x32"_su8, f);
  ExpectRead<I>(I{O::I32X4LeU}, "\xfd\x33"_su8, f);
  ExpectRead<I>(I{O::I32X4GeS}, "\xfd\x34"_su8, f);
  ExpectRead<I>(I{O::I32X4GeU}, "\xfd\x35"_su8, f);
  ExpectRead<I>(I{O::F32X4Eq}, "\xfd\x40"_su8, f);
  ExpectRead<I>(I{O::F32X4Ne}, "\xfd\x41"_su8, f);
  ExpectRead<I>(I{O::F32X4Lt}, "\xfd\x42"_su8, f);
  ExpectRead<I>(I{O::F32X4Gt}, "\xfd\x43"_su8, f);
  ExpectRead<I>(I{O::F32X4Le}, "\xfd\x44"_su8, f);
  ExpectRead<I>(I{O::F32X4Ge}, "\xfd\x45"_su8, f);
  ExpectRead<I>(I{O::F64X2Eq}, "\xfd\x46"_su8, f);
  ExpectRead<I>(I{O::F64X2Ne}, "\xfd\x47"_su8, f);
  ExpectRead<I>(I{O::F64X2Lt}, "\xfd\x48"_su8, f);
  ExpectRead<I>(I{O::F64X2Gt}, "\xfd\x49"_su8, f);
  ExpectRead<I>(I{O::F64X2Le}, "\xfd\x4a"_su8, f);
  ExpectRead<I>(I{O::F64X2Ge}, "\xfd\x4b"_su8, f);
  ExpectRead<I>(I{O::V128Not}, "\xfd\x4c"_su8, f);
  ExpectRead<I>(I{O::V128And}, "\xfd\x4d"_su8, f);
  ExpectRead<I>(I{O::V128Or}, "\xfd\x4e"_su8, f);
  ExpectRead<I>(I{O::V128Xor}, "\xfd\x4f"_su8, f);
  ExpectRead<I>(I{O::V128BitSelect}, "\xfd\x50"_su8, f);
  ExpectRead<I>(I{O::I8X16Neg}, "\xfd\x51"_su8, f);
  ExpectRead<I>(I{O::I8X16AnyTrue}, "\xfd\x52"_su8, f);
  ExpectRead<I>(I{O::I8X16AllTrue}, "\xfd\x53"_su8, f);
  ExpectRead<I>(I{O::I8X16Shl}, "\xfd\x54"_su8, f);
  ExpectRead<I>(I{O::I8X16ShrS}, "\xfd\x55"_su8, f);
  ExpectRead<I>(I{O::I8X16ShrU}, "\xfd\x56"_su8, f);
  ExpectRead<I>(I{O::I8X16Add}, "\xfd\x57"_su8, f);
  ExpectRead<I>(I{O::I8X16AddSaturateS}, "\xfd\x58"_su8, f);
  ExpectRead<I>(I{O::I8X16AddSaturateU}, "\xfd\x59"_su8, f);
  ExpectRead<I>(I{O::I8X16Sub}, "\xfd\x5a"_su8, f);
  ExpectRead<I>(I{O::I8X16SubSaturateS}, "\xfd\x5b"_su8, f);
  ExpectRead<I>(I{O::I8X16SubSaturateU}, "\xfd\x5c"_su8, f);
  ExpectRead<I>(I{O::I8X16Mul}, "\xfd\x5d"_su8, f);
  ExpectRead<I>(I{O::I16X8Neg}, "\xfd\x62"_su8, f);
  ExpectRead<I>(I{O::I16X8AnyTrue}, "\xfd\x63"_su8, f);
  ExpectRead<I>(I{O::I16X8AllTrue}, "\xfd\x64"_su8, f);
  ExpectRead<I>(I{O::I16X8Shl}, "\xfd\x65"_su8, f);
  ExpectRead<I>(I{O::I16X8ShrS}, "\xfd\x66"_su8, f);
  ExpectRead<I>(I{O::I16X8ShrU}, "\xfd\x67"_su8, f);
  ExpectRead<I>(I{O::I16X8Add}, "\xfd\x68"_su8, f);
  ExpectRead<I>(I{O::I16X8AddSaturateS}, "\xfd\x69"_su8, f);
  ExpectRead<I>(I{O::I16X8AddSaturateU}, "\xfd\x6a"_su8, f);
  ExpectRead<I>(I{O::I16X8Sub}, "\xfd\x6b"_su8, f);
  ExpectRead<I>(I{O::I16X8SubSaturateS}, "\xfd\x6c"_su8, f);
  ExpectRead<I>(I{O::I16X8SubSaturateU}, "\xfd\x6d"_su8, f);
  ExpectRead<I>(I{O::I16X8Mul}, "\xfd\x6e"_su8, f);
  ExpectRead<I>(I{O::I32X4Neg}, "\xfd\x73"_su8, f);
  ExpectRead<I>(I{O::I32X4AnyTrue}, "\xfd\x74"_su8, f);
  ExpectRead<I>(I{O::I32X4AllTrue}, "\xfd\x75"_su8, f);
  ExpectRead<I>(I{O::I32X4Shl}, "\xfd\x76"_su8, f);
  ExpectRead<I>(I{O::I32X4ShrS}, "\xfd\x77"_su8, f);
  ExpectRead<I>(I{O::I32X4ShrU}, "\xfd\x78"_su8, f);
  ExpectRead<I>(I{O::I32X4Add}, "\xfd\x79"_su8, f);
  ExpectRead<I>(I{O::I32X4Sub}, "\xfd\x7c"_su8, f);
  ExpectRead<I>(I{O::I32X4Mul}, "\xfd\x7f"_su8, f);
  ExpectRead<I>(I{O::I64X2Neg}, "\xfd\x84\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2AnyTrue}, "\xfd\x85\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2AllTrue}, "\xfd\x86\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2Shl}, "\xfd\x87\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2ShrS}, "\xfd\x88\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2ShrU}, "\xfd\x89\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2Add}, "\xfd\x8a\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2Sub}, "\xfd\x8d\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Abs}, "\xfd\x95\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Neg}, "\xfd\x96\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Sqrt}, "\xfd\x97\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Add}, "\xfd\x9a\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Sub}, "\xfd\x9b\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Mul}, "\xfd\x9c\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Div}, "\xfd\x9d\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Min}, "\xfd\x9e\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4Max}, "\xfd\x9f\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Abs}, "\xfd\xa0\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Neg}, "\xfd\xa1\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Sqrt}, "\xfd\xa2\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Add}, "\xfd\xa5\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Sub}, "\xfd\xa6\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Mul}, "\xfd\xa7\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Div}, "\xfd\xa8\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Min}, "\xfd\xa9\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2Max}, "\xfd\xaa\x01"_su8, f);
  ExpectRead<I>(I{O::I32X4TruncSatF32X4S}, "\xfd\xab\x01"_su8, f);
  ExpectRead<I>(I{O::I32X4TruncSatF32X4U}, "\xfd\xac\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2TruncSatF64X2S}, "\xfd\xad\x01"_su8, f);
  ExpectRead<I>(I{O::I64X2TruncSatF64X2U}, "\xfd\xae\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4ConvertI32X4S}, "\xfd\xaf\x01"_su8, f);
  ExpectRead<I>(I{O::F32X4ConvertI32X4U}, "\xfd\xb0\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2ConvertI64X2S}, "\xfd\xb1\x01"_su8, f);
  ExpectRead<I>(I{O::F64X2ConvertI64X2U}, "\xfd\xb2\x01"_su8, f);
}

TEST(ReadTest, Instruction_threads) {
  using I = Instruction;
  using O = Opcode;

  const MemArgImmediate m{0, 0};

  Features f;
  f.enable_threads();

  ExpectRead<I>(I{O::AtomicNotify, m}, "\xfe\x00\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicWait, m}, "\xfe\x01\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicWait, m}, "\xfe\x02\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicLoad, m}, "\xfe\x10\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicLoad, m}, "\xfe\x11\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicLoad8U, m}, "\xfe\x12\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicLoad16U, m}, "\xfe\x13\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicLoad8U, m}, "\xfe\x14\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicLoad16U, m}, "\xfe\x15\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicLoad32U, m}, "\xfe\x16\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicStore, m}, "\xfe\x17\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicStore, m}, "\xfe\x18\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicStore8, m}, "\xfe\x19\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicStore16, m}, "\xfe\x1a\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicStore8, m}, "\xfe\x1b\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicStore16, m}, "\xfe\x1c\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicStore32, m}, "\xfe\x1d\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwAdd, m}, "\xfe\x1e\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwAdd, m}, "\xfe\x1f\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8AddU, m}, "\xfe\x20\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16AddU, m}, "\xfe\x21\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8AddU, m}, "\xfe\x22\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16AddU, m}, "\xfe\x23\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32AddU, m}, "\xfe\x24\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwSub, m}, "\xfe\x25\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwSub, m}, "\xfe\x26\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8SubU, m}, "\xfe\x27\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16SubU, m}, "\xfe\x28\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8SubU, m}, "\xfe\x29\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16SubU, m}, "\xfe\x2a\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32SubU, m}, "\xfe\x2b\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwAnd, m}, "\xfe\x2c\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwAnd, m}, "\xfe\x2d\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8AndU, m}, "\xfe\x2e\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16AndU, m}, "\xfe\x2f\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8AndU, m}, "\xfe\x30\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16AndU, m}, "\xfe\x31\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32AndU, m}, "\xfe\x32\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwOr, m}, "\xfe\x33\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwOr, m}, "\xfe\x34\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8OrU, m}, "\xfe\x35\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16OrU, m}, "\xfe\x36\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8OrU, m}, "\xfe\x37\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16OrU, m}, "\xfe\x38\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32OrU, m}, "\xfe\x39\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwXor, m}, "\xfe\x3a\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwXor, m}, "\xfe\x3b\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8XorU, m}, "\xfe\x3c\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16XorU, m}, "\xfe\x3d\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8XorU, m}, "\xfe\x3e\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16XorU, m}, "\xfe\x3f\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32XorU, m}, "\xfe\x40\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwXchg, m}, "\xfe\x41\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwXchg, m}, "\xfe\x42\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8XchgU, m}, "\xfe\x43\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16XchgU, m}, "\xfe\x44\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8XchgU, m}, "\xfe\x45\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16XchgU, m}, "\xfe\x46\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32XchgU, m}, "\xfe\x47\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmwCmpxchg, m}, "\xfe\x48\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmwCmpxchg, m}, "\xfe\x49\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw8CmpxchgU, m}, "\xfe\x4a\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I32AtomicRmw16CmpxchgU, m}, "\xfe\x4b\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw8CmpxchgU, m}, "\xfe\x4c\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw16CmpxchgU, m}, "\xfe\x4d\x00\x00"_su8, f);
  ExpectRead<I>(I{O::I64AtomicRmw32CmpxchgU, m}, "\xfe\x4e\x00\x00"_su8, f);
}

TEST(ReadTest, Limits) {
  ExpectRead<Limits>(Limits{129}, "\x00\x81\x01"_su8);
  ExpectRead<Limits>(Limits{2, 1000}, "\x01\x02\xe8\x07"_su8);
}

TEST(ReadTest, Limits_BadFlags) {
  ExpectReadFailure<Limits>({{0, "limits"}, {1, "Invalid flags value: 2"}},
                            "\x02\x01"_su8);
  ExpectReadFailure<Limits>({{0, "limits"}, {1, "Invalid flags value: 3"}},
                            "\x03\x01"_su8);
}

TEST(ReadTest, Limits_threads) {
  Features features;
  features.enable_threads();

  ExpectRead<Limits>(Limits{2, 1000, Shared::Yes}, "\x03\x02\xe8\x07"_su8,
                     features);
}

TEST(ReadTest, Limits_PastEnd) {
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {1, "min"}, {1, "u32"}, {1, "Unable to read u8"}},
      "\x00"_su8);
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {2, "max"}, {2, "u32"}, {2, "Unable to read u8"}},
      "\x01\x00"_su8);
}

TEST(ReadTest, Locals) {
  ExpectRead<Locals>(Locals{2, ValueType::I32}, "\x02\x7f"_su8);
  ExpectRead<Locals>(Locals{320, ValueType::F64}, "\xc0\x02\x7c"_su8);
}

TEST(ReadTest, Locals_PastEnd) {
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {0, "count"}, {0, "Unable to read u8"}}, ""_su8);
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {2, "type"}, {2, "value type"}, {2, "Unable to read u8"}},
      "\xc0\x02"_su8);
}

TEST(ReadTest, MemArgImmediate) {
  ExpectRead<MemArgImmediate>(MemArgImmediate{0, 0}, "\x00\x00"_su8);
  ExpectRead<MemArgImmediate>(MemArgImmediate{1, 256}, "\x01\x80\x02"_su8);
}

TEST(ReadTest, Memory) {
  ExpectRead<Memory>(Memory{MemoryType{Limits{1, 2}}}, "\x01\x01\x02"_su8);
}

TEST(ReadTest, Memory_PastEnd) {
  ExpectReadFailure<Memory>({{0, "memory"},
                             {0, "memory type"},
                             {0, "limits"},
                             {0, "flags"},
                             {0, "Unable to read u8"}},
                            ""_su8);
}

TEST(ReadTest, MemoryType) {
  ExpectRead<MemoryType>(MemoryType{Limits{1}}, "\x00\x01"_su8);
  ExpectRead<MemoryType>(MemoryType{Limits{0, 128}}, "\x01\x00\x80\x01"_su8);
}

TEST(ReadTest, MemoryType_PastEnd) {
  ExpectReadFailure<MemoryType>({{0, "memory type"},
                                 {0, "limits"},
                                 {0, "flags"},
                                 {0, "Unable to read u8"}},
                                ""_su8);
}

TEST(ReadTest, Mutability) {
  ExpectRead<Mutability>(Mutability::Const, "\x00"_su8);
  ExpectRead<Mutability>(Mutability::Var, "\x01"_su8);
}

TEST(ReadTest, Mutability_Unknown) {
  ExpectReadFailure<Mutability>(
      {{0, "mutability"}, {1, "Unknown mutability: 4"}}, "\x04"_su8);

  // Overlong encoding is not allowed.
  ExpectReadFailure<Mutability>(
      {{0, "mutability"}, {1, "Unknown mutability: 132"}}, "\x84\x00"_su8);
}

TEST(ReadTest, NameAssoc) {
  ExpectRead<NameAssoc>(NameAssoc{2u, "hi"}, "\x02\x02hi"_su8);
}

TEST(ReadTest, NameAssoc_PastEnd) {
  ExpectReadFailure<NameAssoc>(
      {{0, "name assoc"}, {0, "index"}, {0, "Unable to read u8"}}, ""_su8);

  ExpectReadFailure<NameAssoc>(
      {{0, "name assoc"}, {1, "name"}, {1, "length"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST(ReadTest, NameSubsectionId) {
  ExpectRead<NameSubsectionId>(NameSubsectionId::ModuleName, "\x00"_su8);
  ExpectRead<NameSubsectionId>(NameSubsectionId::FunctionNames, "\x01"_su8);
  ExpectRead<NameSubsectionId>(NameSubsectionId::LocalNames, "\x02"_su8);
}

TEST(ReadTest, NameSubsectionId_Unknown) {
  ExpectReadFailure<NameSubsectionId>(
      {{0, "name subsection id"}, {1, "Unknown name subsection id: 3"}},
      "\x03"_su8);
  ExpectReadFailure<NameSubsectionId>(
      {{0, "name subsection id"}, {1, "Unknown name subsection id: 255"}},
      "\xff"_su8);
}

TEST(ReadTest, NameSubsection) {
  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::ModuleName, "\0"_su8}, "\x00\x01\0"_su8);

  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::FunctionNames, "\0\0"_su8},
      "\x01\x02\0\0"_su8);

  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::LocalNames, "\0\0\0"_su8},
      "\x02\x03\0\0\0"_su8);
}

TEST(ReadTest, NameSubsection_BadSubsectionId) {
  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {0, "name subsection id"},
                                     {1, "Unknown name subsection id: 3"}},
                                    "\x03"_su8);
}

TEST(ReadTest, NameSubsection_PastEnd) {
  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {0, "name subsection id"},
                                     {0, "Unable to read u8"}},
                                    ""_su8);

  ExpectReadFailure<NameSubsection>(
      {{0, "name subsection"}, {1, "length"}, {1, "Unable to read u8"}},
      "\x00"_su8);
}

TEST(ReadTest, Opcode) {
  ExpectRead<Opcode>(Opcode::Unreachable, "\x00"_su8);
  ExpectRead<Opcode>(Opcode::Nop, "\x01"_su8);
  ExpectRead<Opcode>(Opcode::Block, "\x02"_su8);
  ExpectRead<Opcode>(Opcode::Loop, "\x03"_su8);
  ExpectRead<Opcode>(Opcode::If, "\x04"_su8);
  ExpectRead<Opcode>(Opcode::Else, "\x05"_su8);
  ExpectRead<Opcode>(Opcode::End, "\x0b"_su8);
  ExpectRead<Opcode>(Opcode::Br, "\x0c"_su8);
  ExpectRead<Opcode>(Opcode::BrIf, "\x0d"_su8);
  ExpectRead<Opcode>(Opcode::BrTable, "\x0e"_su8);
  ExpectRead<Opcode>(Opcode::Return, "\x0f"_su8);
  ExpectRead<Opcode>(Opcode::Call, "\x10"_su8);
  ExpectRead<Opcode>(Opcode::CallIndirect, "\x11"_su8);
  ExpectRead<Opcode>(Opcode::Drop, "\x1a"_su8);
  ExpectRead<Opcode>(Opcode::Select, "\x1b"_su8);
  ExpectRead<Opcode>(Opcode::LocalGet, "\x20"_su8);
  ExpectRead<Opcode>(Opcode::LocalSet, "\x21"_su8);
  ExpectRead<Opcode>(Opcode::LocalTee, "\x22"_su8);
  ExpectRead<Opcode>(Opcode::GlobalGet, "\x23"_su8);
  ExpectRead<Opcode>(Opcode::GlobalSet, "\x24"_su8);
  ExpectRead<Opcode>(Opcode::I32Load, "\x28"_su8);
  ExpectRead<Opcode>(Opcode::I64Load, "\x29"_su8);
  ExpectRead<Opcode>(Opcode::F32Load, "\x2a"_su8);
  ExpectRead<Opcode>(Opcode::F64Load, "\x2b"_su8);
  ExpectRead<Opcode>(Opcode::I32Load8S, "\x2c"_su8);
  ExpectRead<Opcode>(Opcode::I32Load8U, "\x2d"_su8);
  ExpectRead<Opcode>(Opcode::I32Load16S, "\x2e"_su8);
  ExpectRead<Opcode>(Opcode::I32Load16U, "\x2f"_su8);
  ExpectRead<Opcode>(Opcode::I64Load8S, "\x30"_su8);
  ExpectRead<Opcode>(Opcode::I64Load8U, "\x31"_su8);
  ExpectRead<Opcode>(Opcode::I64Load16S, "\x32"_su8);
  ExpectRead<Opcode>(Opcode::I64Load16U, "\x33"_su8);
  ExpectRead<Opcode>(Opcode::I64Load32S, "\x34"_su8);
  ExpectRead<Opcode>(Opcode::I64Load32U, "\x35"_su8);
  ExpectRead<Opcode>(Opcode::I32Store, "\x36"_su8);
  ExpectRead<Opcode>(Opcode::I64Store, "\x37"_su8);
  ExpectRead<Opcode>(Opcode::F32Store, "\x38"_su8);
  ExpectRead<Opcode>(Opcode::F64Store, "\x39"_su8);
  ExpectRead<Opcode>(Opcode::I32Store8, "\x3a"_su8);
  ExpectRead<Opcode>(Opcode::I32Store16, "\x3b"_su8);
  ExpectRead<Opcode>(Opcode::I64Store8, "\x3c"_su8);
  ExpectRead<Opcode>(Opcode::I64Store16, "\x3d"_su8);
  ExpectRead<Opcode>(Opcode::I64Store32, "\x3e"_su8);
  ExpectRead<Opcode>(Opcode::MemorySize, "\x3f"_su8);
  ExpectRead<Opcode>(Opcode::MemoryGrow, "\x40"_su8);
  ExpectRead<Opcode>(Opcode::I32Const, "\x41"_su8);
  ExpectRead<Opcode>(Opcode::I64Const, "\x42"_su8);
  ExpectRead<Opcode>(Opcode::F32Const, "\x43"_su8);
  ExpectRead<Opcode>(Opcode::F64Const, "\x44"_su8);
  ExpectRead<Opcode>(Opcode::I32Eqz, "\x45"_su8);
  ExpectRead<Opcode>(Opcode::I32Eq, "\x46"_su8);
  ExpectRead<Opcode>(Opcode::I32Ne, "\x47"_su8);
  ExpectRead<Opcode>(Opcode::I32LtS, "\x48"_su8);
  ExpectRead<Opcode>(Opcode::I32LtU, "\x49"_su8);
  ExpectRead<Opcode>(Opcode::I32GtS, "\x4a"_su8);
  ExpectRead<Opcode>(Opcode::I32GtU, "\x4b"_su8);
  ExpectRead<Opcode>(Opcode::I32LeS, "\x4c"_su8);
  ExpectRead<Opcode>(Opcode::I32LeU, "\x4d"_su8);
  ExpectRead<Opcode>(Opcode::I32GeS, "\x4e"_su8);
  ExpectRead<Opcode>(Opcode::I32GeU, "\x4f"_su8);
  ExpectRead<Opcode>(Opcode::I64Eqz, "\x50"_su8);
  ExpectRead<Opcode>(Opcode::I64Eq, "\x51"_su8);
  ExpectRead<Opcode>(Opcode::I64Ne, "\x52"_su8);
  ExpectRead<Opcode>(Opcode::I64LtS, "\x53"_su8);
  ExpectRead<Opcode>(Opcode::I64LtU, "\x54"_su8);
  ExpectRead<Opcode>(Opcode::I64GtS, "\x55"_su8);
  ExpectRead<Opcode>(Opcode::I64GtU, "\x56"_su8);
  ExpectRead<Opcode>(Opcode::I64LeS, "\x57"_su8);
  ExpectRead<Opcode>(Opcode::I64LeU, "\x58"_su8);
  ExpectRead<Opcode>(Opcode::I64GeS, "\x59"_su8);
  ExpectRead<Opcode>(Opcode::I64GeU, "\x5a"_su8);
  ExpectRead<Opcode>(Opcode::F32Eq, "\x5b"_su8);
  ExpectRead<Opcode>(Opcode::F32Ne, "\x5c"_su8);
  ExpectRead<Opcode>(Opcode::F32Lt, "\x5d"_su8);
  ExpectRead<Opcode>(Opcode::F32Gt, "\x5e"_su8);
  ExpectRead<Opcode>(Opcode::F32Le, "\x5f"_su8);
  ExpectRead<Opcode>(Opcode::F32Ge, "\x60"_su8);
  ExpectRead<Opcode>(Opcode::F64Eq, "\x61"_su8);
  ExpectRead<Opcode>(Opcode::F64Ne, "\x62"_su8);
  ExpectRead<Opcode>(Opcode::F64Lt, "\x63"_su8);
  ExpectRead<Opcode>(Opcode::F64Gt, "\x64"_su8);
  ExpectRead<Opcode>(Opcode::F64Le, "\x65"_su8);
  ExpectRead<Opcode>(Opcode::F64Ge, "\x66"_su8);
  ExpectRead<Opcode>(Opcode::I32Clz, "\x67"_su8);
  ExpectRead<Opcode>(Opcode::I32Ctz, "\x68"_su8);
  ExpectRead<Opcode>(Opcode::I32Popcnt, "\x69"_su8);
  ExpectRead<Opcode>(Opcode::I32Add, "\x6a"_su8);
  ExpectRead<Opcode>(Opcode::I32Sub, "\x6b"_su8);
  ExpectRead<Opcode>(Opcode::I32Mul, "\x6c"_su8);
  ExpectRead<Opcode>(Opcode::I32DivS, "\x6d"_su8);
  ExpectRead<Opcode>(Opcode::I32DivU, "\x6e"_su8);
  ExpectRead<Opcode>(Opcode::I32RemS, "\x6f"_su8);
  ExpectRead<Opcode>(Opcode::I32RemU, "\x70"_su8);
  ExpectRead<Opcode>(Opcode::I32And, "\x71"_su8);
  ExpectRead<Opcode>(Opcode::I32Or, "\x72"_su8);
  ExpectRead<Opcode>(Opcode::I32Xor, "\x73"_su8);
  ExpectRead<Opcode>(Opcode::I32Shl, "\x74"_su8);
  ExpectRead<Opcode>(Opcode::I32ShrS, "\x75"_su8);
  ExpectRead<Opcode>(Opcode::I32ShrU, "\x76"_su8);
  ExpectRead<Opcode>(Opcode::I32Rotl, "\x77"_su8);
  ExpectRead<Opcode>(Opcode::I32Rotr, "\x78"_su8);
  ExpectRead<Opcode>(Opcode::I64Clz, "\x79"_su8);
  ExpectRead<Opcode>(Opcode::I64Ctz, "\x7a"_su8);
  ExpectRead<Opcode>(Opcode::I64Popcnt, "\x7b"_su8);
  ExpectRead<Opcode>(Opcode::I64Add, "\x7c"_su8);
  ExpectRead<Opcode>(Opcode::I64Sub, "\x7d"_su8);
  ExpectRead<Opcode>(Opcode::I64Mul, "\x7e"_su8);
  ExpectRead<Opcode>(Opcode::I64DivS, "\x7f"_su8);
  ExpectRead<Opcode>(Opcode::I64DivU, "\x80"_su8);
  ExpectRead<Opcode>(Opcode::I64RemS, "\x81"_su8);
  ExpectRead<Opcode>(Opcode::I64RemU, "\x82"_su8);
  ExpectRead<Opcode>(Opcode::I64And, "\x83"_su8);
  ExpectRead<Opcode>(Opcode::I64Or, "\x84"_su8);
  ExpectRead<Opcode>(Opcode::I64Xor, "\x85"_su8);
  ExpectRead<Opcode>(Opcode::I64Shl, "\x86"_su8);
  ExpectRead<Opcode>(Opcode::I64ShrS, "\x87"_su8);
  ExpectRead<Opcode>(Opcode::I64ShrU, "\x88"_su8);
  ExpectRead<Opcode>(Opcode::I64Rotl, "\x89"_su8);
  ExpectRead<Opcode>(Opcode::I64Rotr, "\x8a"_su8);
  ExpectRead<Opcode>(Opcode::F32Abs, "\x8b"_su8);
  ExpectRead<Opcode>(Opcode::F32Neg, "\x8c"_su8);
  ExpectRead<Opcode>(Opcode::F32Ceil, "\x8d"_su8);
  ExpectRead<Opcode>(Opcode::F32Floor, "\x8e"_su8);
  ExpectRead<Opcode>(Opcode::F32Trunc, "\x8f"_su8);
  ExpectRead<Opcode>(Opcode::F32Nearest, "\x90"_su8);
  ExpectRead<Opcode>(Opcode::F32Sqrt, "\x91"_su8);
  ExpectRead<Opcode>(Opcode::F32Add, "\x92"_su8);
  ExpectRead<Opcode>(Opcode::F32Sub, "\x93"_su8);
  ExpectRead<Opcode>(Opcode::F32Mul, "\x94"_su8);
  ExpectRead<Opcode>(Opcode::F32Div, "\x95"_su8);
  ExpectRead<Opcode>(Opcode::F32Min, "\x96"_su8);
  ExpectRead<Opcode>(Opcode::F32Max, "\x97"_su8);
  ExpectRead<Opcode>(Opcode::F32Copysign, "\x98"_su8);
  ExpectRead<Opcode>(Opcode::F64Abs, "\x99"_su8);
  ExpectRead<Opcode>(Opcode::F64Neg, "\x9a"_su8);
  ExpectRead<Opcode>(Opcode::F64Ceil, "\x9b"_su8);
  ExpectRead<Opcode>(Opcode::F64Floor, "\x9c"_su8);
  ExpectRead<Opcode>(Opcode::F64Trunc, "\x9d"_su8);
  ExpectRead<Opcode>(Opcode::F64Nearest, "\x9e"_su8);
  ExpectRead<Opcode>(Opcode::F64Sqrt, "\x9f"_su8);
  ExpectRead<Opcode>(Opcode::F64Add, "\xa0"_su8);
  ExpectRead<Opcode>(Opcode::F64Sub, "\xa1"_su8);
  ExpectRead<Opcode>(Opcode::F64Mul, "\xa2"_su8);
  ExpectRead<Opcode>(Opcode::F64Div, "\xa3"_su8);
  ExpectRead<Opcode>(Opcode::F64Min, "\xa4"_su8);
  ExpectRead<Opcode>(Opcode::F64Max, "\xa5"_su8);
  ExpectRead<Opcode>(Opcode::F64Copysign, "\xa6"_su8);
  ExpectRead<Opcode>(Opcode::I32WrapI64, "\xa7"_su8);
  ExpectRead<Opcode>(Opcode::I32TruncF32S, "\xa8"_su8);
  ExpectRead<Opcode>(Opcode::I32TruncF32U, "\xa9"_su8);
  ExpectRead<Opcode>(Opcode::I32TruncF64S, "\xaa"_su8);
  ExpectRead<Opcode>(Opcode::I32TruncF64U, "\xab"_su8);
  ExpectRead<Opcode>(Opcode::I64ExtendI32S, "\xac"_su8);
  ExpectRead<Opcode>(Opcode::I64ExtendI32U, "\xad"_su8);
  ExpectRead<Opcode>(Opcode::I64TruncF32S, "\xae"_su8);
  ExpectRead<Opcode>(Opcode::I64TruncF32U, "\xaf"_su8);
  ExpectRead<Opcode>(Opcode::I64TruncF64S, "\xb0"_su8);
  ExpectRead<Opcode>(Opcode::I64TruncF64U, "\xb1"_su8);
  ExpectRead<Opcode>(Opcode::F32ConvertI32S, "\xb2"_su8);
  ExpectRead<Opcode>(Opcode::F32ConvertI32U, "\xb3"_su8);
  ExpectRead<Opcode>(Opcode::F32ConvertI64S, "\xb4"_su8);
  ExpectRead<Opcode>(Opcode::F32ConvertI64U, "\xb5"_su8);
  ExpectRead<Opcode>(Opcode::F32DemoteF64, "\xb6"_su8);
  ExpectRead<Opcode>(Opcode::F64ConvertI32S, "\xb7"_su8);
  ExpectRead<Opcode>(Opcode::F64ConvertI32U, "\xb8"_su8);
  ExpectRead<Opcode>(Opcode::F64ConvertI64S, "\xb9"_su8);
  ExpectRead<Opcode>(Opcode::F64ConvertI64U, "\xba"_su8);
  ExpectRead<Opcode>(Opcode::F64PromoteF32, "\xbb"_su8);
  ExpectRead<Opcode>(Opcode::I32ReinterpretF32, "\xbc"_su8);
  ExpectRead<Opcode>(Opcode::I64ReinterpretF64, "\xbd"_su8);
  ExpectRead<Opcode>(Opcode::F32ReinterpretI32, "\xbe"_su8);
  ExpectRead<Opcode>(Opcode::F64ReinterpretI64, "\xbf"_su8);
}

namespace {

void ExpectUnknownOpcode(u8 code) {
  const u8 span_buffer[] = {code};
  auto msg = format("Unknown opcode: {}", code);
  ExpectReadFailure<Opcode>({{0, "opcode"}, {1, msg}}, SpanU8{span_buffer, 1});
}

void ExpectUnknownOpcode(u8 prefix, u32 orig_code, const Features& features) {
  u8 data[] = {prefix, 0, 0, 0, 0, 0};
  u32 code = orig_code;
  int length = 1;
  do {
    data[length++] = (code & 0x7f) | (code >= 0x80 ? 0x80 : 0);
    code >>= 7;
  } while (code > 0);

  ExpectReadFailure<Opcode>(
      {{0, "opcode"},
       {length, format("Unknown opcode: {} {}", prefix, orig_code)}},
      SpanU8{data, length}, features);
}

}  // namespace

TEST(ReadTest, Opcode_Unknown) {
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
    ExpectUnknownOpcode(code);
  }
}

TEST(ReadTest, Opcode_exceptions) {
  Features features;
  features.enable_exceptions();

  ExpectRead<Opcode>(Opcode::Try, "\x06"_su8, features);
  ExpectRead<Opcode>(Opcode::Catch, "\x07"_su8, features);
  ExpectRead<Opcode>(Opcode::Throw, "\x08"_su8, features);
  ExpectRead<Opcode>(Opcode::Rethrow, "\x09"_su8, features);
  ExpectRead<Opcode>(Opcode::BrOnExn, "\x0a"_su8, features);
}

TEST(ReadTest, Opcode_tail_call) {
  Features features;
  features.enable_tail_call();

  ExpectRead<Opcode>(Opcode::ReturnCall, "\x12"_su8, features);
  ExpectRead<Opcode>(Opcode::ReturnCallIndirect, "\x13"_su8, features);
}

TEST(ReadTest, Opcode_sign_extension) {
  Features features;
  features.enable_sign_extension();

  ExpectRead<Opcode>(Opcode::I32Extend8S, "\xc0"_su8, features);
  ExpectRead<Opcode>(Opcode::I32Extend16S, "\xc1"_su8, features);
  ExpectRead<Opcode>(Opcode::I64Extend8S, "\xc2"_su8, features);
  ExpectRead<Opcode>(Opcode::I64Extend16S, "\xc3"_su8, features);
  ExpectRead<Opcode>(Opcode::I64Extend32S, "\xc4"_su8, features);
}

TEST(ReadTest, Opcode_reference_types) {
  Features features;
  features.enable_reference_types();

  ExpectRead<Opcode>(Opcode::TableGet, "\x25"_su8, features);
  ExpectRead<Opcode>(Opcode::TableSet, "\x26"_su8, features);
  ExpectRead<Opcode>(Opcode::TableGrow, "\xfc\x0f"_su8, features);
  ExpectRead<Opcode>(Opcode::TableSize, "\xfc\x10"_su8, features);
  ExpectRead<Opcode>(Opcode::RefNull, "\xd0"_su8, features);
  ExpectRead<Opcode>(Opcode::RefIsNull, "\xd1"_su8, features);
}

TEST(ReadTest, Opcode_function_references) {
  Features features;
  features.enable_function_references();

  ExpectRead<Opcode>(Opcode::RefFunc, "\xd2"_su8, features);
}

TEST(ReadTest, Opcode_saturating_float_to_int) {
  Features features;
  features.enable_saturating_float_to_int();

  ExpectRead<Opcode>(Opcode::I32TruncSatF32S, "\xfc\x00"_su8, features);
  ExpectRead<Opcode>(Opcode::I32TruncSatF32U, "\xfc\x01"_su8, features);
  ExpectRead<Opcode>(Opcode::I32TruncSatF64S, "\xfc\x02"_su8, features);
  ExpectRead<Opcode>(Opcode::I32TruncSatF64U, "\xfc\x03"_su8, features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF32S, "\xfc\x04"_su8, features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF32U, "\xfc\x05"_su8, features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF64S, "\xfc\x06"_su8, features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF64U, "\xfc\x07"_su8, features);
}

TEST(ReadTest, Opcode_bulk_memory) {
  Features features;
  features.enable_bulk_memory();

  ExpectRead<Opcode>(Opcode::MemoryInit, "\xfc\x08"_su8, features);
  ExpectRead<Opcode>(Opcode::DataDrop, "\xfc\x09"_su8, features);
  ExpectRead<Opcode>(Opcode::MemoryCopy, "\xfc\x0a"_su8, features);
  ExpectRead<Opcode>(Opcode::MemoryFill, "\xfc\x0b"_su8, features);
  ExpectRead<Opcode>(Opcode::TableInit, "\xfc\x0c"_su8, features);
  ExpectRead<Opcode>(Opcode::ElemDrop, "\xfc\x0d"_su8, features);
  ExpectRead<Opcode>(Opcode::TableCopy, "\xfc\x0e"_su8, features);
}

TEST(ReadTest, Opcode_disabled_misc_prefix) {
  {
    Features features;
    features.enable_saturating_float_to_int();
    ExpectUnknownOpcode(0xfc, 8, features);
    ExpectUnknownOpcode(0xfc, 9, features);
    ExpectUnknownOpcode(0xfc, 10, features);
    ExpectUnknownOpcode(0xfc, 11, features);
    ExpectUnknownOpcode(0xfc, 12, features);
    ExpectUnknownOpcode(0xfc, 13, features);
    ExpectUnknownOpcode(0xfc, 14, features);
  }

  {
    Features features;
    features.enable_bulk_memory();
    ExpectUnknownOpcode(0xfc, 0, features);
    ExpectUnknownOpcode(0xfc, 1, features);
    ExpectUnknownOpcode(0xfc, 2, features);
    ExpectUnknownOpcode(0xfc, 3, features);
    ExpectUnknownOpcode(0xfc, 4, features);
    ExpectUnknownOpcode(0xfc, 5, features);
    ExpectUnknownOpcode(0xfc, 6, features);
    ExpectUnknownOpcode(0xfc, 7, features);
  }
}

TEST(ReadTest, Opcode_Unknown_misc_prefix) {
  Features features;
  features.enable_saturating_float_to_int();
  features.enable_bulk_memory();

  for (u8 code = 0x0f; code < 0x7f; ++code) {
    ExpectUnknownOpcode(0xfc, code, features);
  }

  // Test some longer codes too.
  ExpectUnknownOpcode(0xfc, 128, features);
  ExpectUnknownOpcode(0xfc, 16384, features);
  ExpectUnknownOpcode(0xfc, 2097152, features);
  ExpectUnknownOpcode(0xfc, 268435456, features);
}

TEST(ReadTest, Opcode_simd) {
  using O = Opcode;

  Features features;
  features.enable_simd();

  ExpectRead<O>(O::V128Load, "\xfd\x00"_su8, features);
  ExpectRead<O>(O::V128Store, "\xfd\x01"_su8, features);
  ExpectRead<O>(O::V128Const, "\xfd\x02"_su8, features);
  ExpectRead<O>(O::V8X16Shuffle, "\xfd\x03"_su8, features);
  ExpectRead<O>(O::I8X16Splat, "\xfd\x04"_su8, features);
  ExpectRead<O>(O::I8X16ExtractLaneS, "\xfd\x05"_su8, features);
  ExpectRead<O>(O::I8X16ExtractLaneU, "\xfd\x06"_su8, features);
  ExpectRead<O>(O::I8X16ReplaceLane, "\xfd\x07"_su8, features);
  ExpectRead<O>(O::I16X8Splat, "\xfd\x08"_su8, features);
  ExpectRead<O>(O::I16X8ExtractLaneS, "\xfd\x09"_su8, features);
  ExpectRead<O>(O::I16X8ExtractLaneU, "\xfd\x0a"_su8, features);
  ExpectRead<O>(O::I16X8ReplaceLane, "\xfd\x0b"_su8, features);
  ExpectRead<O>(O::I32X4Splat, "\xfd\x0c"_su8, features);
  ExpectRead<O>(O::I32X4ExtractLane, "\xfd\x0d"_su8, features);
  ExpectRead<O>(O::I32X4ReplaceLane, "\xfd\x0e"_su8, features);
  ExpectRead<O>(O::I64X2Splat, "\xfd\x0f"_su8, features);
  ExpectRead<O>(O::I64X2ExtractLane, "\xfd\x10"_su8, features);
  ExpectRead<O>(O::I64X2ReplaceLane, "\xfd\x11"_su8, features);
  ExpectRead<O>(O::F32X4Splat, "\xfd\x12"_su8, features);
  ExpectRead<O>(O::F32X4ExtractLane, "\xfd\x13"_su8, features);
  ExpectRead<O>(O::F32X4ReplaceLane, "\xfd\x14"_su8, features);
  ExpectRead<O>(O::F64X2Splat, "\xfd\x15"_su8, features);
  ExpectRead<O>(O::F64X2ExtractLane, "\xfd\x16"_su8, features);
  ExpectRead<O>(O::F64X2ReplaceLane, "\xfd\x17"_su8, features);
  ExpectRead<O>(O::I8X16Eq, "\xfd\x18"_su8, features);
  ExpectRead<O>(O::I8X16Ne, "\xfd\x19"_su8, features);
  ExpectRead<O>(O::I8X16LtS, "\xfd\x1a"_su8, features);
  ExpectRead<O>(O::I8X16LtU, "\xfd\x1b"_su8, features);
  ExpectRead<O>(O::I8X16GtS, "\xfd\x1c"_su8, features);
  ExpectRead<O>(O::I8X16GtU, "\xfd\x1d"_su8, features);
  ExpectRead<O>(O::I8X16LeS, "\xfd\x1e"_su8, features);
  ExpectRead<O>(O::I8X16LeU, "\xfd\x1f"_su8, features);
  ExpectRead<O>(O::I8X16GeS, "\xfd\x20"_su8, features);
  ExpectRead<O>(O::I8X16GeU, "\xfd\x21"_su8, features);
  ExpectRead<O>(O::I16X8Eq, "\xfd\x22"_su8, features);
  ExpectRead<O>(O::I16X8Ne, "\xfd\x23"_su8, features);
  ExpectRead<O>(O::I16X8LtS, "\xfd\x24"_su8, features);
  ExpectRead<O>(O::I16X8LtU, "\xfd\x25"_su8, features);
  ExpectRead<O>(O::I16X8GtS, "\xfd\x26"_su8, features);
  ExpectRead<O>(O::I16X8GtU, "\xfd\x27"_su8, features);
  ExpectRead<O>(O::I16X8LeS, "\xfd\x28"_su8, features);
  ExpectRead<O>(O::I16X8LeU, "\xfd\x29"_su8, features);
  ExpectRead<O>(O::I16X8GeS, "\xfd\x2a"_su8, features);
  ExpectRead<O>(O::I16X8GeU, "\xfd\x2b"_su8, features);
  ExpectRead<O>(O::I32X4Eq, "\xfd\x2c"_su8, features);
  ExpectRead<O>(O::I32X4Ne, "\xfd\x2d"_su8, features);
  ExpectRead<O>(O::I32X4LtS, "\xfd\x2e"_su8, features);
  ExpectRead<O>(O::I32X4LtU, "\xfd\x2f"_su8, features);
  ExpectRead<O>(O::I32X4GtS, "\xfd\x30"_su8, features);
  ExpectRead<O>(O::I32X4GtU, "\xfd\x31"_su8, features);
  ExpectRead<O>(O::I32X4LeS, "\xfd\x32"_su8, features);
  ExpectRead<O>(O::I32X4LeU, "\xfd\x33"_su8, features);
  ExpectRead<O>(O::I32X4GeS, "\xfd\x34"_su8, features);
  ExpectRead<O>(O::I32X4GeU, "\xfd\x35"_su8, features);
  ExpectRead<O>(O::F32X4Eq, "\xfd\x40"_su8, features);
  ExpectRead<O>(O::F32X4Ne, "\xfd\x41"_su8, features);
  ExpectRead<O>(O::F32X4Lt, "\xfd\x42"_su8, features);
  ExpectRead<O>(O::F32X4Gt, "\xfd\x43"_su8, features);
  ExpectRead<O>(O::F32X4Le, "\xfd\x44"_su8, features);
  ExpectRead<O>(O::F32X4Ge, "\xfd\x45"_su8, features);
  ExpectRead<O>(O::F64X2Eq, "\xfd\x46"_su8, features);
  ExpectRead<O>(O::F64X2Ne, "\xfd\x47"_su8, features);
  ExpectRead<O>(O::F64X2Lt, "\xfd\x48"_su8, features);
  ExpectRead<O>(O::F64X2Gt, "\xfd\x49"_su8, features);
  ExpectRead<O>(O::F64X2Le, "\xfd\x4a"_su8, features);
  ExpectRead<O>(O::F64X2Ge, "\xfd\x4b"_su8, features);
  ExpectRead<O>(O::V128Not, "\xfd\x4c"_su8, features);
  ExpectRead<O>(O::V128And, "\xfd\x4d"_su8, features);
  ExpectRead<O>(O::V128Or, "\xfd\x4e"_su8, features);
  ExpectRead<O>(O::V128Xor, "\xfd\x4f"_su8, features);
  ExpectRead<O>(O::V128BitSelect, "\xfd\x50"_su8, features);
  ExpectRead<O>(O::I8X16Neg, "\xfd\x51"_su8, features);
  ExpectRead<O>(O::I8X16AnyTrue, "\xfd\x52"_su8, features);
  ExpectRead<O>(O::I8X16AllTrue, "\xfd\x53"_su8, features);
  ExpectRead<O>(O::I8X16Shl, "\xfd\x54"_su8, features);
  ExpectRead<O>(O::I8X16ShrS, "\xfd\x55"_su8, features);
  ExpectRead<O>(O::I8X16ShrU, "\xfd\x56"_su8, features);
  ExpectRead<O>(O::I8X16Add, "\xfd\x57"_su8, features);
  ExpectRead<O>(O::I8X16AddSaturateS, "\xfd\x58"_su8, features);
  ExpectRead<O>(O::I8X16AddSaturateU, "\xfd\x59"_su8, features);
  ExpectRead<O>(O::I8X16Sub, "\xfd\x5a"_su8, features);
  ExpectRead<O>(O::I8X16SubSaturateS, "\xfd\x5b"_su8, features);
  ExpectRead<O>(O::I8X16SubSaturateU, "\xfd\x5c"_su8, features);
  ExpectRead<O>(O::I8X16Mul, "\xfd\x5d"_su8, features);
  ExpectRead<O>(O::I16X8Neg, "\xfd\x62"_su8, features);
  ExpectRead<O>(O::I16X8AnyTrue, "\xfd\x63"_su8, features);
  ExpectRead<O>(O::I16X8AllTrue, "\xfd\x64"_su8, features);
  ExpectRead<O>(O::I16X8Shl, "\xfd\x65"_su8, features);
  ExpectRead<O>(O::I16X8ShrS, "\xfd\x66"_su8, features);
  ExpectRead<O>(O::I16X8ShrU, "\xfd\x67"_su8, features);
  ExpectRead<O>(O::I16X8Add, "\xfd\x68"_su8, features);
  ExpectRead<O>(O::I16X8AddSaturateS, "\xfd\x69"_su8, features);
  ExpectRead<O>(O::I16X8AddSaturateU, "\xfd\x6a"_su8, features);
  ExpectRead<O>(O::I16X8Sub, "\xfd\x6b"_su8, features);
  ExpectRead<O>(O::I16X8SubSaturateS, "\xfd\x6c"_su8, features);
  ExpectRead<O>(O::I16X8SubSaturateU, "\xfd\x6d"_su8, features);
  ExpectRead<O>(O::I16X8Mul, "\xfd\x6e"_su8, features);
  ExpectRead<O>(O::I32X4Neg, "\xfd\x73"_su8, features);
  ExpectRead<O>(O::I32X4AnyTrue, "\xfd\x74"_su8, features);
  ExpectRead<O>(O::I32X4AllTrue, "\xfd\x75"_su8, features);
  ExpectRead<O>(O::I32X4Shl, "\xfd\x76"_su8, features);
  ExpectRead<O>(O::I32X4ShrS, "\xfd\x77"_su8, features);
  ExpectRead<O>(O::I32X4ShrU, "\xfd\x78"_su8, features);
  ExpectRead<O>(O::I32X4Add, "\xfd\x79"_su8, features);
  ExpectRead<O>(O::I32X4Sub, "\xfd\x7c"_su8, features);
  ExpectRead<O>(O::I32X4Mul, "\xfd\x7f"_su8, features);
  ExpectRead<O>(O::I64X2Neg, "\xfd\x84\x01"_su8, features);
  ExpectRead<O>(O::I64X2AnyTrue, "\xfd\x85\x01"_su8, features);
  ExpectRead<O>(O::I64X2AllTrue, "\xfd\x86\x01"_su8, features);
  ExpectRead<O>(O::I64X2Shl, "\xfd\x87\x01"_su8, features);
  ExpectRead<O>(O::I64X2ShrS, "\xfd\x88\x01"_su8, features);
  ExpectRead<O>(O::I64X2ShrU, "\xfd\x89\x01"_su8, features);
  ExpectRead<O>(O::I64X2Add, "\xfd\x8a\x01"_su8, features);
  ExpectRead<O>(O::I64X2Sub, "\xfd\x8d\x01"_su8, features);
  ExpectRead<O>(O::F32X4Abs, "\xfd\x95\x01"_su8, features);
  ExpectRead<O>(O::F32X4Neg, "\xfd\x96\x01"_su8, features);
  ExpectRead<O>(O::F32X4Sqrt, "\xfd\x97\x01"_su8, features);
  ExpectRead<O>(O::F32X4Add, "\xfd\x9a\x01"_su8, features);
  ExpectRead<O>(O::F32X4Sub, "\xfd\x9b\x01"_su8, features);
  ExpectRead<O>(O::F32X4Mul, "\xfd\x9c\x01"_su8, features);
  ExpectRead<O>(O::F32X4Div, "\xfd\x9d\x01"_su8, features);
  ExpectRead<O>(O::F32X4Min, "\xfd\x9e\x01"_su8, features);
  ExpectRead<O>(O::F32X4Max, "\xfd\x9f\x01"_su8, features);
  ExpectRead<O>(O::F64X2Abs, "\xfd\xa0\x01"_su8, features);
  ExpectRead<O>(O::F64X2Neg, "\xfd\xa1\x01"_su8, features);
  ExpectRead<O>(O::F64X2Sqrt, "\xfd\xa2\x01"_su8, features);
  ExpectRead<O>(O::F64X2Add, "\xfd\xa5\x01"_su8, features);
  ExpectRead<O>(O::F64X2Sub, "\xfd\xa6\x01"_su8, features);
  ExpectRead<O>(O::F64X2Mul, "\xfd\xa7\x01"_su8, features);
  ExpectRead<O>(O::F64X2Div, "\xfd\xa8\x01"_su8, features);
  ExpectRead<O>(O::F64X2Min, "\xfd\xa9\x01"_su8, features);
  ExpectRead<O>(O::F64X2Max, "\xfd\xaa\x01"_su8, features);
  ExpectRead<O>(O::I32X4TruncSatF32X4S, "\xfd\xab\x01"_su8, features);
  ExpectRead<O>(O::I32X4TruncSatF32X4U, "\xfd\xac\x01"_su8, features);
  ExpectRead<O>(O::I64X2TruncSatF64X2S, "\xfd\xad\x01"_su8, features);
  ExpectRead<O>(O::I64X2TruncSatF64X2U, "\xfd\xae\x01"_su8, features);
  ExpectRead<O>(O::F32X4ConvertI32X4S, "\xfd\xaf\x01"_su8, features);
  ExpectRead<O>(O::F32X4ConvertI32X4U, "\xfd\xb0\x01"_su8, features);
  ExpectRead<O>(O::F64X2ConvertI64X2S, "\xfd\xb1\x01"_su8, features);
  ExpectRead<O>(O::F64X2ConvertI64X2U, "\xfd\xb2\x01"_su8, features);
}

TEST(ReadTest, Opcode_Unknown_simd_prefix) {
  Features features;
  features.enable_simd();

  const u8 kInvalidOpcodes[] = {
      0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
      0x5e, 0x5f, 0x60, 0x61, 0x6f, 0x70, 0x71, 0x72, 0x7a, 0x7b,
      0x7d, 0x7e, 0x80, 0x81, 0x82, 0x83, 0x8b, 0x8c, 0x8e, 0x8f,
      0x90, 0x91, 0x92, 0x93, 0x94, 0xa3, 0xa4, 0xb3,
  };
  for (auto code : SpanU8{kInvalidOpcodes, sizeof(kInvalidOpcodes)}) {
    ExpectUnknownOpcode(0xfd, code, features);
  }

  // Test some longer codes too.
  ExpectUnknownOpcode(0xfd, 16384, features);
  ExpectUnknownOpcode(0xfd, 2097152, features);
  ExpectUnknownOpcode(0xfd, 268435456, features);
}

TEST(ReadTest, Opcode_threads) {
  using O = Opcode;

  Features features;
  features.enable_threads();

  ExpectRead<O>(O::AtomicNotify, "\xfe\x00"_su8, features);
  ExpectRead<O>(O::I32AtomicWait, "\xfe\x01"_su8, features);
  ExpectRead<O>(O::I64AtomicWait, "\xfe\x02"_su8, features);
  ExpectRead<O>(O::I32AtomicLoad, "\xfe\x10"_su8, features);
  ExpectRead<O>(O::I64AtomicLoad, "\xfe\x11"_su8, features);
  ExpectRead<O>(O::I32AtomicLoad8U, "\xfe\x12"_su8, features);
  ExpectRead<O>(O::I32AtomicLoad16U, "\xfe\x13"_su8, features);
  ExpectRead<O>(O::I64AtomicLoad8U, "\xfe\x14"_su8, features);
  ExpectRead<O>(O::I64AtomicLoad16U, "\xfe\x15"_su8, features);
  ExpectRead<O>(O::I64AtomicLoad32U, "\xfe\x16"_su8, features);
  ExpectRead<O>(O::I32AtomicStore, "\xfe\x17"_su8, features);
  ExpectRead<O>(O::I64AtomicStore, "\xfe\x18"_su8, features);
  ExpectRead<O>(O::I32AtomicStore8, "\xfe\x19"_su8, features);
  ExpectRead<O>(O::I32AtomicStore16, "\xfe\x1a"_su8, features);
  ExpectRead<O>(O::I64AtomicStore8, "\xfe\x1b"_su8, features);
  ExpectRead<O>(O::I64AtomicStore16, "\xfe\x1c"_su8, features);
  ExpectRead<O>(O::I64AtomicStore32, "\xfe\x1d"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwAdd, "\xfe\x1e"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwAdd, "\xfe\x1f"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8AddU, "\xfe\x20"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16AddU, "\xfe\x21"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8AddU, "\xfe\x22"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16AddU, "\xfe\x23"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32AddU, "\xfe\x24"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwSub, "\xfe\x25"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwSub, "\xfe\x26"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8SubU, "\xfe\x27"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16SubU, "\xfe\x28"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8SubU, "\xfe\x29"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16SubU, "\xfe\x2a"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32SubU, "\xfe\x2b"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwAnd, "\xfe\x2c"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwAnd, "\xfe\x2d"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8AndU, "\xfe\x2e"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16AndU, "\xfe\x2f"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8AndU, "\xfe\x30"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16AndU, "\xfe\x31"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32AndU, "\xfe\x32"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwOr, "\xfe\x33"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwOr, "\xfe\x34"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8OrU, "\xfe\x35"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16OrU, "\xfe\x36"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8OrU, "\xfe\x37"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16OrU, "\xfe\x38"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32OrU, "\xfe\x39"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwXor, "\xfe\x3a"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwXor, "\xfe\x3b"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8XorU, "\xfe\x3c"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16XorU, "\xfe\x3d"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8XorU, "\xfe\x3e"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16XorU, "\xfe\x3f"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32XorU, "\xfe\x40"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwXchg, "\xfe\x41"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwXchg, "\xfe\x42"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8XchgU, "\xfe\x43"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16XchgU, "\xfe\x44"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8XchgU, "\xfe\x45"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16XchgU, "\xfe\x46"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32XchgU, "\xfe\x47"_su8, features);
  ExpectRead<O>(O::I32AtomicRmwCmpxchg, "\xfe\x48"_su8, features);
  ExpectRead<O>(O::I64AtomicRmwCmpxchg, "\xfe\x49"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw8CmpxchgU, "\xfe\x4a"_su8, features);
  ExpectRead<O>(O::I32AtomicRmw16CmpxchgU, "\xfe\x4b"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw8CmpxchgU, "\xfe\x4c"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw16CmpxchgU, "\xfe\x4d"_su8, features);
  ExpectRead<O>(O::I64AtomicRmw32CmpxchgU, "\xfe\x4e"_su8, features);
}

TEST(ReadTest, Opcode_Unknown_threads_prefix) {
  Features features;
  features.enable_threads();

  const u8 kInvalidOpcodes[] = {
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x4f, 0x50,
  };
  for (auto code : SpanU8{kInvalidOpcodes, sizeof(kInvalidOpcodes)}) {
    ExpectUnknownOpcode(0xfe, code, features);
  }

  // Test some longer codes too.
  ExpectUnknownOpcode(0xfe, 128, features);
  ExpectUnknownOpcode(0xfe, 16384, features);
  ExpectUnknownOpcode(0xfe, 2097152, features);
  ExpectUnknownOpcode(0xfe, 268435456, features);
}

TEST(ReadTest, S32) {
  ExpectRead<s32>(32, "\x20"_su8);
  ExpectRead<s32>(-16, "\x70"_su8);
  ExpectRead<s32>(448, "\xc0\x03"_su8);
  ExpectRead<s32>(-3648, "\xc0\x63"_su8);
  ExpectRead<s32>(33360, "\xd0\x84\x02"_su8);
  ExpectRead<s32>(-753072, "\xd0\x84\x52"_su8);
  ExpectRead<s32>(101718048, "\xa0\xb0\xc0\x30"_su8);
  ExpectRead<s32>(-32499680, "\xa0\xb0\xc0\x70"_su8);
  ExpectRead<s32>(1042036848, "\xf0\xf0\xf0\xf0\x03"_su8);
  ExpectRead<s32>(-837011344, "\xf0\xf0\xf0\xf0\x7c"_su8);
}

TEST(ReadTest, S32_TooLong) {
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x5 or 0x7d, got 0x15"}},
                         "\xf0\xf0\xf0\xf0\x15"_su8);
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x3 or 0x7b, got 0x73"}},
                         "\xff\xff\xff\xff\x73"_su8);
}

TEST(ReadTest, S32_PastEnd) {
  ExpectReadFailure<s32>({{0, "s32"}, {0, "Unable to read u8"}}, ""_su8);
  ExpectReadFailure<s32>({{0, "s32"}, {1, "Unable to read u8"}}, "\xc0"_su8);
  ExpectReadFailure<s32>({{0, "s32"}, {2, "Unable to read u8"}},
                         "\xd0\x84"_su8);
  ExpectReadFailure<s32>({{0, "s32"}, {3, "Unable to read u8"}},
                         "\xa0\xb0\xc0"_su8);
  ExpectReadFailure<s32>({{0, "s32"}, {4, "Unable to read u8"}},
                         "\xf0\xf0\xf0\xf0"_su8);
}

TEST(ReadTest, S64) {
  ExpectRead<s64>(32, "\x20"_su8);
  ExpectRead<s64>(-16, "\x70"_su8);
  ExpectRead<s64>(448, "\xc0\x03"_su8);
  ExpectRead<s64>(-3648, "\xc0\x63"_su8);
  ExpectRead<s64>(33360, "\xd0\x84\x02"_su8);
  ExpectRead<s64>(-753072, "\xd0\x84\x52"_su8);
  ExpectRead<s64>(101718048, "\xa0\xb0\xc0\x30"_su8);
  ExpectRead<s64>(-32499680, "\xa0\xb0\xc0\x70"_su8);
  ExpectRead<s64>(1042036848, "\xf0\xf0\xf0\xf0\x03"_su8);
  ExpectRead<s64>(-837011344, "\xf0\xf0\xf0\xf0\x7c"_su8);
  ExpectRead<s64>(13893120096, "\xe0\xe0\xe0\xe0\x33"_su8);
  ExpectRead<s64>(-12413554592, "\xe0\xe0\xe0\xe0\x51"_su8);
  ExpectRead<s64>(1533472417872, "\xd0\xd0\xd0\xd0\xd0\x2c"_su8);
  ExpectRead<s64>(-287593715632, "\xd0\xd0\xd0\xd0\xd0\x77"_su8);
  ExpectRead<s64>(139105536057408, "\xc0\xc0\xc0\xc0\xc0\xd0\x1f"_su8);
  ExpectRead<s64>(-124777254608832, "\xc0\xc0\xc0\xc0\xc0\xd0\x63"_su8);
  ExpectRead<s64>(1338117014066474, "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"_su8);
  ExpectRead<s64>(-12172681868045014, "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"_su8);
  ExpectRead<s64>(1070725794579330814,
                  "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"_su8);
  ExpectRead<s64>(-3540960223848057090,
                  "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"_su8);
}

TEST(ReadTest, S64_TooLong) {
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xf0"}},
      "\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"_su8);
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xff"}},
      "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"_su8);
}

TEST(ReadTest, S64_PastEnd) {
  ExpectReadFailure<s64>({{0, "s64"}, {0, "Unable to read u8"}}, ""_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {1, "Unable to read u8"}}, "\xc0"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {2, "Unable to read u8"}},
                         "\xd0\x84"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {3, "Unable to read u8"}},
                         "\xa0\xb0\xc0"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {4, "Unable to read u8"}},
                         "\xf0\xf0\xf0\xf0"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {5, "Unable to read u8"}},
                         "\xe0\xe0\xe0\xe0\xe0"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {6, "Unable to read u8"}},
                         "\xd0\xd0\xd0\xd0\xd0\xc0"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {7, "Unable to read u8"}},
                         "\xc0\xc0\xc0\xc0\xc0\xd0\x84"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {8, "Unable to read u8"}},
                         "\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"_su8);
  ExpectReadFailure<s64>({{0, "s64"}, {9, "Unable to read u8"}},
                         "\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"_su8);
}

TEST(ReadTest, SectionId) {
  ExpectRead<SectionId>(SectionId::Custom, "\x00"_su8);
  ExpectRead<SectionId>(SectionId::Type, "\x01"_su8);
  ExpectRead<SectionId>(SectionId::Import, "\x02"_su8);
  ExpectRead<SectionId>(SectionId::Function, "\x03"_su8);
  ExpectRead<SectionId>(SectionId::Table, "\x04"_su8);
  ExpectRead<SectionId>(SectionId::Memory, "\x05"_su8);
  ExpectRead<SectionId>(SectionId::Global, "\x06"_su8);
  ExpectRead<SectionId>(SectionId::Export, "\x07"_su8);
  ExpectRead<SectionId>(SectionId::Start, "\x08"_su8);
  ExpectRead<SectionId>(SectionId::Element, "\x09"_su8);
  ExpectRead<SectionId>(SectionId::Code, "\x0a"_su8);
  ExpectRead<SectionId>(SectionId::Data, "\x0b"_su8);
  ExpectRead<SectionId>(SectionId::DataCount, "\x0c"_su8);

  // Overlong encoding.
  ExpectRead<SectionId>(SectionId::Custom, "\x80\x00"_su8);
}

TEST(ReadTest, SectionId_Unknown) {
  ExpectReadFailure<SectionId>(
      {{0, "section id"}, {1, "Unknown section id: 13"}}, "\x0d"_su8);
}

TEST(ReadTest, Section) {
  ExpectRead<Section>(
      Section{KnownSection{SectionId::Type, "\x01\x02\x03"_su8}},
      "\x01\x03\x01\x02\x03"_su8);

  ExpectRead<Section>(Section{CustomSection{"name", "\x04\x05\x06"_su8}},
                      "\x00\x08\x04name\x04\x05\x06"_su8);
}

TEST(ReadTest, Section_PastEnd) {
  ExpectReadFailure<Section>(
      {{0, "section"}, {0, "section id"}, {0, "u32"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<Section>(
      {{0, "section"}, {1, "length"}, {1, "Unable to read u8"}}, "\x01"_su8);

  ExpectReadFailure<Section>(
      {{0, "section"}, {2, "Length extends past end: 1 > 0"}}, "\x01\x01"_su8);
}

TEST(ReadTest, Start) {
  ExpectRead<Start>(Start{256}, "\x80\x02"_su8);
}

TEST(ReadTest, ReadString) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x05hello"_su8;
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ(string_view{"hello"}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadString_Leftovers) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x01more"_su8;
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ(string_view{"m"}, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReadTest, ReadString_BadLength) {
  {
    Features features;
    TestErrors errors;
    const SpanU8 data = ""_su8;
    SpanU8 copy = data;
    auto result = ReadString(&copy, features, errors, "test");
    ExpectError({{0, "test"}, {0, "length"}, {0, "Unable to read u8"}}, errors,
                data);
    EXPECT_EQ(nullopt, result);
    EXPECT_EQ(0u, copy.size());
  }

  {
    Features features;
    TestErrors errors;
    const SpanU8 data = "\xc0"_su8;
    SpanU8 copy = data;
    auto result = ReadString(&copy, features, errors, "test");
    ExpectError({{0, "test"}, {0, "length"}, {1, "Unable to read u8"}}, errors,
                data);
    EXPECT_EQ(nullopt, result);
    EXPECT_EQ(0u, copy.size());
  }
}

TEST(ReadTest, ReadString_Fail) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x06small"_su8;
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {1, "Length extends past end: 6 > 5"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(5u, copy.size());
}

TEST(ReadTest, Table) {
  ExpectRead<Table>(Table{TableType{Limits{1}, ElementType::Funcref}},
                    "\x70\x00\x01"_su8);
}

TEST(ReadTest, Table_PastEnd) {
  ExpectReadFailure<Table>({{0, "table"},
                            {0, "table type"},
                            {0, "element type"},
                            {0, "Unable to read u8"}},
                           ""_su8);
}

TEST(ReadTest, TableType) {
  ExpectRead<TableType>(TableType{Limits{1}, ElementType::Funcref},
                        "\x70\x00\x01"_su8);
  ExpectRead<TableType>(TableType{Limits{1, 2}, ElementType::Funcref},
                        "\x70\x01\x01\x02"_su8);
}

TEST(ReadTest, TableType_BadElementType) {
  ExpectReadFailure<TableType>(
      {{0, "table type"}, {0, "element type"}, {1, "Unknown element type: 0"}},
      "\x00"_su8);
}

TEST(ReadTest, TableType_PastEnd) {
  ExpectReadFailure<TableType>(
      {{0, "table type"}, {0, "element type"}, {0, "Unable to read u8"}},
      ""_su8);

  ExpectReadFailure<TableType>({{0, "table type"},
                                {1, "limits"},
                                {1, "flags"},
                                {1, "Unable to read u8"}},
                               "\x70"_su8);
}

TEST(ReadTest, TypeEntry) {
  ExpectRead<TypeEntry>(TypeEntry{FunctionType{{}, {ValueType::I32}}},
                        "\x60\x00\x01\x7f"_su8);
}

TEST(ReadTest, TypeEntry_BadForm) {
  ExpectReadFailure<TypeEntry>(
      {{0, "type entry"}, {1, "Unknown type form: 64"}}, "\x40"_su8);
}

TEST(ReadTest, U32) {
  ExpectRead<u32>(32u, "\x20"_su8);
  ExpectRead<u32>(448u, "\xc0\x03"_su8);
  ExpectRead<u32>(33360u, "\xd0\x84\x02"_su8);
  ExpectRead<u32>(101718048u, "\xa0\xb0\xc0\x30"_su8);
  ExpectRead<u32>(1042036848u, "\xf0\xf0\xf0\xf0\x03"_su8);
}

TEST(ReadTest, U32_TooLong) {
  ExpectReadFailure<u32>(
      {{0, "u32"},
       {5, "Last byte of u32 must be zero extension: expected 0x2, got 0x12"}},
      "\xf0\xf0\xf0\xf0\x12"_su8);
}

TEST(ReadTest, U32_PastEnd) {
  ExpectReadFailure<u32>({{0, "u32"}, {0, "Unable to read u8"}}, ""_su8);
  ExpectReadFailure<u32>({{0, "u32"}, {1, "Unable to read u8"}}, "\xc0"_su8);
  ExpectReadFailure<u32>({{0, "u32"}, {2, "Unable to read u8"}},
                         "\xd0\x84"_su8);
  ExpectReadFailure<u32>({{0, "u32"}, {3, "Unable to read u8"}},
                         "\xa0\xb0\xc0"_su8);
  ExpectReadFailure<u32>({{0, "u32"}, {4, "Unable to read u8"}},
                         "\xf0\xf0\xf0\xf0"_su8);
}

TEST(ReadTest, U8) {
  ExpectRead<u8>(32, "\x20"_su8);
  ExpectReadFailure<u8>({{0, "Unable to read u8"}}, ""_su8);
}

TEST(ReadTest, ValueType_MVP) {
  ExpectRead<ValueType>(ValueType::I32, "\x7f"_su8);
  ExpectRead<ValueType>(ValueType::I64, "\x7e"_su8);
  ExpectRead<ValueType>(ValueType::F32, "\x7d"_su8);
  ExpectRead<ValueType>(ValueType::F64, "\x7c"_su8);
}

TEST(ReadTest, ValueType_simd) {
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 123"}}, "\x7b"_su8);

  Features features;
  features.enable_simd();
  ExpectRead<ValueType>(ValueType::V128, "\x7b"_su8, features);
}

TEST(ReadTest, ValueType_reference_types) {
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 111"}}, "\x6f"_su8);

  Features features;
  features.enable_reference_types();
  ExpectRead<ValueType>(ValueType::Anyref, "\x6f"_su8, features);
}

TEST(ReadTest, ValueType_Unknown) {
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 16"}}, "\x10"_su8);

  // Overlong encoding is not allowed.
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 255"}}, "\xff\x7f"_su8);
}

TEST(ReadTest, ReadVector_u8) {
  Features features;
  TestErrors errors;
  const SpanU8 data = "\x05hello"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u8>(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<u8>{'h', 'e', 'l', 'l', 'o'}), result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadVector_u32) {
  Features features;
  TestErrors errors;
  const SpanU8 data =
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<u32>{5, 128, 206412}), result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadVector_FailLength) {
  Features features;
  TestErrors errors;
  const SpanU8 data =
      "\x02"  // Count.
      "\x05"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {1, "Count extends past end: 2 > 1"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReadTest, ReadVector_PastEnd) {
  Features features;
  TestErrors errors;
  const SpanU8 data =
      "\x02"  // Count.
      "\x05"
      "\x80"_su8;
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {2, "u32"}, {3, "Unable to read u8"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(0u, copy.size());
}
