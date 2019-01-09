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

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"
#include "wasp/binary/read/read_block_type.h"
#include "wasp/binary/read/read_br_table_immediate.h"
#include "wasp/binary/read/read_bytes.h"
#include "wasp/binary/read/read_call_indirect_immediate.h"
#include "wasp/binary/read/read_code.h"
#include "wasp/binary/read/read_constant_expression.h"
#include "wasp/binary/read/read_copy_immediate.h"
#include "wasp/binary/read/read_count.h"
#include "wasp/binary/read/read_data_segment.h"
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
#include "wasp/binary/read/read_start.h"
#include "wasp/binary/read/read_string.h"
#include "wasp/binary/read/read_table.h"
#include "wasp/binary/read/read_table_type.h"
#include "wasp/binary/read/read_type_entry.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/read/read_u8.h"
#include "wasp/binary/read/read_value_type.h"
#include "wasp/binary/read/read_vector.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReadTest, BlockType) {
  ExpectRead<BlockType>(BlockType::I32, MakeSpanU8("\x7f"));
  ExpectRead<BlockType>(BlockType::I64, MakeSpanU8("\x7e"));
  ExpectRead<BlockType>(BlockType::F32, MakeSpanU8("\x7d"));
  ExpectRead<BlockType>(BlockType::F64, MakeSpanU8("\x7c"));
  ExpectRead<BlockType>(BlockType::Void, MakeSpanU8("\x40"));
}

TEST(ReadTest, BlockType_Unknown) {
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 0"}}, MakeSpanU8("\x00"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 255"}},
      MakeSpanU8("\xff\x7f"));
}

TEST(ReadTest, BrTableImmediate) {
  ExpectRead<BrTableImmediate>(BrTableImmediate{{}, 0}, MakeSpanU8("\x00\x00"));
  ExpectRead<BrTableImmediate>(BrTableImmediate{{1, 2}, 3},
                               MakeSpanU8("\x02\x01\x02\x03"));
}

TEST(ReadTest, BrTableImmediate_PastEnd) {
  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {0, "targets"}, {0, "count"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {1, "default target"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, ReadBytes) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 3, features, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(data, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadBytes_Leftovers) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, features, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(data.subspan(0, 2), result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReadTest, ReadBytes_Fail) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, features, errors);
  EXPECT_EQ(nullopt, result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

TEST(ReadTest, CallIndirectImmediate) {
  ExpectRead<CallIndirectImmediate>(CallIndirectImmediate{1, 0},
                                    MakeSpanU8("\x01\x00"));
  ExpectRead<CallIndirectImmediate>(CallIndirectImmediate{128, 0},
                                    MakeSpanU8("\x80\x01\x00"));
}

TEST(ReadTest, CallIndirectImmediate_BadReserved) {
  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"},
       {1, "reserved"},
       {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x00\x01"));
}

TEST(ReadTest, CallIndirectImmediate_PastEnd) {
  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"}, {0, "type index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"}, {1, "reserved"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, Code) {
  // Empty body. This will fail validation, but can still be read.
  ExpectRead<Code>(Code{{}, MakeExpression("")}, MakeSpanU8("\x01\x00"));

  // Smallest valid empty body.
  ExpectRead<Code>(Code{{}, MakeExpression("\x0b")},
                   MakeSpanU8("\x02\x00\x0b"));

  // (func
  //   (local i32 i32 i64 i64 i64)
  //   (nop))
  ExpectRead<Code>(Code{{Locals{2, ValueType::I32}, Locals{3, ValueType::I64}},
                        MakeExpression("\x01\x0b")},
                   MakeSpanU8("\x07\x02\x02\x7f\x03\x7e\x01\x0b"));
}

TEST(ReadTest, Code_PastEnd) {
  ExpectReadFailure<Code>(
      {{0, "code"}, {0, "length"}, {0, "Unable to read u8"}}, MakeSpanU8(""));

  ExpectReadFailure<Code>({{0, "code"}, {1, "Length extends past end: 1 > 0"}},
                          MakeSpanU8("\x01"));

  ExpectReadFailure<Code>(
      {{0, "code"}, {1, "locals vector"}, {2, "Count extends past end: 1 > 0"}},
      MakeSpanU8("\x01\x01"));
}

TEST(ReadTest, ConstantExpression) {
  // i32.const
  {
    const auto data = MakeSpanU8("\x41\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // i64.const
  {
    const auto data = MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // f32.const
  {
    const auto data = MakeSpanU8("\x43\x00\x00\x00\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // f64.const
  {
    const auto data = MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // get_global
  {
    const auto data = MakeSpanU8("\x23\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }
}

TEST(ReadTest, ConstantExpression_NoEnd) {
  // i32.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x41\x00"));

  // i64.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {7, "opcode"}, {7, "Unable to read u8"}},
      MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01"));

  // f32.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {5, "opcode"}, {5, "Unable to read u8"}},
      MakeSpanU8("\x43\x00\x00\x00\x00"));

  // f64.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {9, "opcode"}, {9, "Unable to read u8"}},
      MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00"));

  // get_global
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x23\x00"));
}

TEST(ReadTest, ConstantExpression_TooLong) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {3, "Expected end instruction"}},
      MakeSpanU8("\x41\x00\x01\x0b"));
}

TEST(ReadTest, ConstantExpression_InvalidInstruction) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {0, "opcode"}, {1, "Unknown opcode: 6"}},
      MakeSpanU8("\x06"));
}

TEST(ReadTest, ConstantExpression_IllegalInstruction) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"},
       {1, "Illegal instruction in constant expression: unreachable"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, ConstantExpression_PastEnd) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));
}

TEST(ReadTest, CopyImmediate) {
  ExpectRead<CopyImmediate>(CopyImmediate{0, 0}, MakeSpanU8("\x00\x00"));
}

TEST(ReadTest, CopyImmediate_BadReserved) {
  ExpectReadFailure<CopyImmediate>({{0, "copy immediate"},
                                    {0, "reserved"},
                                    {1, "Expected reserved byte 0, got 1"}},
                                   MakeSpanU8("\x01"));

  ExpectReadFailure<CopyImmediate>({{0, "copy immediate"},
                                    {1, "reserved"},
                                    {2, "Expected reserved byte 0, got 1"}},
                                   MakeSpanU8("\x00\x01"));
}

TEST(ReadTest, CopyImmediate_PastEnd) {
  ExpectReadFailure<CopyImmediate>(
      {{0, "copy immediate"}, {0, "reserved"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<CopyImmediate>(
      {{0, "copy immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, ReadCount) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, features, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReadTest, ReadCount_PastEnd) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, features, errors);
  ExpectError({{1, "Count extends past end: 5 > 3"}}, errors, data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReadTest, DataSegment) {
  ExpectRead<DataSegment>(DataSegment{1, MakeConstantExpression("\x42\x01\x0b"),
                                      MakeSpanU8("wxyz")},
                          MakeSpanU8("\x01\x42\x01\x0b\x04wxyz"));
}

TEST(ReadTest, DataSegment_PastEnd) {
  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {0, "memory index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<DataSegment>({{0, "data segment"},
                                  {1, "offset"},
                                  {1, "constant expression"},
                                  {1, "opcode"},
                                  {1, "Unable to read u8"}},
                                 MakeSpanU8("\x00"));

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {4, "length"}, {4, "Unable to read u8"}},
      MakeSpanU8("\x00\x41\x00\x0b"));

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {5, "Length extends past end: 2 > 0"}},
      MakeSpanU8("\x00\x41\x00\x0b\x02"));
}

TEST(ReadTest, ElementSegment) {
  ExpectRead<ElementSegment>(
      ElementSegment{0, MakeConstantExpression("\x41\x01\x0b"), {1, 2, 3}},
      MakeSpanU8("\x00\x41\x01\x0b\x03\x01\x02\x03"));
}

TEST(ReadTest, ElementSegment_PastEnd) {
  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {0, "table index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {1, "offset"},
                                     {1, "constant expression"},
                                     {1, "opcode"},
                                     {1, "Unable to read u8"}},
                                    MakeSpanU8("\x00"));

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {4, "initializers"},
                                     {4, "count"},
                                     {4, "Unable to read u8"}},
                                    MakeSpanU8("\x00\x23\x00\x0b"));
}

TEST(ReadTest, ElementType) {
  ExpectRead<ElementType>(ElementType::Funcref, MakeSpanU8("\x70"));
}

TEST(ReadTest, ElementType_Unknown) {
  ExpectReadFailure<ElementType>(
      {{0, "element type"}, {1, "Unknown element type: 0"}},
      MakeSpanU8("\x00"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ElementType>(
      {{0, "element type"}, {1, "Unknown element type: 240"}},
      MakeSpanU8("\xf0\x7f"));
}

TEST(ReadTest, Export) {
  ExpectRead<Export>(Export{ExternalKind::Function, "hi", 3},
                     MakeSpanU8("\x02hi\x00\x03"));
  ExpectRead<Export>(Export{ExternalKind::Table, "", 1000},
                     MakeSpanU8("\x00\x01\xe8\x07"));
  ExpectRead<Export>(Export{ExternalKind::Memory, "mem", 0},
                     MakeSpanU8("\x03mem\x02\x00"));
  ExpectRead<Export>(Export{ExternalKind::Global, "g", 1},
                     MakeSpanU8("\x01g\x03\x01"));
}

TEST(ReadTest, Export_PastEnd) {
  ExpectReadFailure<Export>(
      {{0, "export"}, {0, "name"}, {0, "length"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<Export>(
      {{0, "export"}, {1, "external kind"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));

  ExpectReadFailure<Export>(
      {{0, "export"}, {2, "index"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x00\x00"));
}

TEST(ReadTest, ExternalKind) {
  ExpectRead<ExternalKind>(ExternalKind::Function, MakeSpanU8("\x00"));
  ExpectRead<ExternalKind>(ExternalKind::Table, MakeSpanU8("\x01"));
  ExpectRead<ExternalKind>(ExternalKind::Memory, MakeSpanU8("\x02"));
  ExpectRead<ExternalKind>(ExternalKind::Global, MakeSpanU8("\x03"));
}

TEST(ReadTest, ExternalKind_Unknown) {
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 4"}},
      MakeSpanU8("\x04"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 132"}},
      MakeSpanU8("\x84\x00"));
}

TEST(ReadTest, F32) {
  ExpectRead<f32>(0.0f, MakeSpanU8("\x00\x00\x00\x00"));
  ExpectRead<f32>(-1.0f, MakeSpanU8("\x00\x00\x80\xbf"));
  ExpectRead<f32>(1234567.0f, MakeSpanU8("\x38\xb4\x96\x49"));
  ExpectRead<f32>(INFINITY, MakeSpanU8("\x00\x00\x80\x7f"));
  ExpectRead<f32>(-INFINITY, MakeSpanU8("\x00\x00\x80\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\xc0\x7f");
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
                         MakeSpanU8("\x00\x00\x00"));
}

TEST(ReadTest, F64) {
  ExpectRead<f64>(0.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00\x00"));
  ExpectRead<f64>(-1.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xbf"));
  ExpectRead<f64>(111111111111111,
                  MakeSpanU8("\xc0\x71\xbc\x93\x84\x43\xd9\x42"));
  ExpectRead<f64>(INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\x7f"));
  ExpectRead<f64>(-INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf8\x7f");
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
                         MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00"));
}

TEST(ReadTest, Function) {
  ExpectRead<Function>(Function{1}, MakeSpanU8("\x01"));
}

TEST(ReadTest, Function_PastEnd) {
  ExpectReadFailure<Function>(
      {{0, "function"}, {0, "type index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));
}

TEST(ReadTest, FunctionType) {
  ExpectRead<FunctionType>(FunctionType{{}, {}}, MakeSpanU8("\x00\x00"));
  ExpectRead<FunctionType>(
      FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}},
      MakeSpanU8("\x02\x7f\x7e\x01\x7c"));
}

TEST(ReadTest, FunctionType_PastEnd) {
  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {0, "count"},
                                   {0, "Unable to read u8"}},
                                  MakeSpanU8(""));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {1, "Count extends past end: 1 > 0"}},
                                  MakeSpanU8("\x01"));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {1, "count"},
                                   {1, "Unable to read u8"}},
                                  MakeSpanU8("\x00"));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {2, "Count extends past end: 1 > 0"}},
                                  MakeSpanU8("\x00\x01"));
}

TEST(ReadTest, Global) {
  // i32 global with i64.const constant expression. This will fail validation
  // but still can be successfully parsed.
  ExpectRead<Global>(Global{GlobalType{ValueType::I32, Mutability::Var},
                            MakeConstantExpression("\x42\x00\x0b")},
                     MakeSpanU8("\x7f\x01\x42\x00\x0b"));
}

TEST(ReadTest, Global_PastEnd) {
  ExpectReadFailure<Global>({{0, "global"},
                             {0, "global type"},
                             {0, "value type"},
                             {0, "Unable to read u8"}},
                            MakeSpanU8(""));

  ExpectReadFailure<Global>({{0, "global"},
                             {2, "constant expression"},
                             {2, "opcode"},
                             {2, "Unable to read u8"}},
                            MakeSpanU8("\x7f\x00"));
}

TEST(ReadTest, GlobalType) {
  ExpectRead<GlobalType>(GlobalType{ValueType::I32, Mutability::Const},
                         MakeSpanU8("\x7f\x00"));
  ExpectRead<GlobalType>(GlobalType{ValueType::F32, Mutability::Var},
                         MakeSpanU8("\x7d\x01"));
}

TEST(ReadTest, GlobalType_PastEnd) {
  ExpectReadFailure<GlobalType>(
      {{0, "global type"}, {0, "value type"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<GlobalType>(
      {{0, "global type"}, {1, "mutability"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x7f"));
}

TEST(ReadTest, Import) {
  ExpectRead<Import>(Import{"a", "func", 11u},
                     MakeSpanU8("\x01\x61\x04\x66unc\x00\x0b"));

  ExpectRead<Import>(
      Import{"b", "table", TableType{Limits{1}, ElementType::Funcref}},
      MakeSpanU8("\x01\x62\x05table\x01\x70\x00\x01"));

  ExpectRead<Import>(Import{"c", "memory", MemoryType{Limits{0, 2}}},
                     MakeSpanU8("\x01\x63\x06memory\x02\x01\x00\x02"));

  ExpectRead<Import>(
      Import{"d", "global", GlobalType{ValueType::I32, Mutability::Const}},
      MakeSpanU8("\x01\x64\x06global\x03\x7f\x00"));
}

TEST(ReadTest, ImportType_PastEnd) {
  ExpectReadFailure<Import>({{0, "import"},
                             {0, "module name"},
                             {0, "length"},
                             {0, "Unable to read u8"}},
                            MakeSpanU8(""));

  ExpectReadFailure<Import>({{0, "import"},
                             {1, "field name"},
                             {1, "length"},
                             {1, "Unable to read u8"}},
                            MakeSpanU8("\x00"));

  ExpectReadFailure<Import>(
      {{0, "import"}, {2, "external kind"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x00\x00"));

  ExpectReadFailure<Import>(
      {{0, "import"}, {3, "function index"}, {3, "Unable to read u8"}},
      MakeSpanU8("\x00\x00\x00"));

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "table type"},
                             {3, "element type"},
                             {3, "Unable to read u8"}},
                            MakeSpanU8("\x00\x00\x01"));

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "memory type"},
                             {3, "limits"},
                             {3, "flags"},
                             {3, "Unable to read u8"}},
                            MakeSpanU8("\x00\x00\x02"));

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "global type"},
                             {3, "value type"},
                             {3, "Unable to read u8"}},
                            MakeSpanU8("\x00\x00\x03"));
}

TEST(ReadTest, IndirectNameAssoc) {
  ExpectRead<IndirectNameAssoc>(
      IndirectNameAssoc{100u, {{0u, "zero"}, {1u, "one"}}},
      MakeSpanU8("\x64"          // Index.
                 "\x02"          // Count.
                 "\x00\x04zero"  // 0 "zero"
                 "\x01\x03one"   // 1 "one"
                 ));
}

TEST(ReadTest, IndirectNameAssoc_PastEnd) {
  ExpectReadFailure<IndirectNameAssoc>(
      {{0, "indirect name assoc"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<IndirectNameAssoc>({{0, "indirect name assoc"},
                                        {1, "name map"},
                                        {1, "count"},
                                        {1, "Unable to read u8"}},
                                       MakeSpanU8("\x00"));

  ExpectReadFailure<IndirectNameAssoc>({{0, "indirect name assoc"},
                                        {1, "name map"},
                                        {2, "Count extends past end: 1 > 0"}},
                                       MakeSpanU8("\x00\x01"));
}

TEST(ReadTest, InitImmediate) {
  ExpectRead<InitImmediate>(InitImmediate{1, 0}, MakeSpanU8("\x01\x00"));
  ExpectRead<InitImmediate>(InitImmediate{128, 0}, MakeSpanU8("\x80\x01\x00"));
}

TEST(ReadTest, InitImmediate_BadReserved) {
  ExpectReadFailure<InitImmediate>({{0, "init immediate"},
                                    {1, "reserved"},
                                    {2, "Expected reserved byte 0, got 1"}},
                                   MakeSpanU8("\x00\x01"));
}

TEST(ReadTest, InitImmediate_PastEnd) {
  ExpectReadFailure<InitImmediate>(
      {{0, "init immediate"}, {0, "segment index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<InitImmediate>(
      {{0, "init immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x01"));
}

TEST(ReadTest, Instruction) {
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

TEST(ReadTest, Instruction_BadMemoryReserved) {
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x3f\x01"));
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x40\x01"));
}

TEST(ReadTest, Instruction_sign_extension) {
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

TEST(ReadTest, Instruction_saturating_float_to_int) {
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

TEST(ReadTest, Instruction_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  Features features;
  features.enable_bulk_memory();

  ExpectRead<I>(I{O::MemoryInit, InitImmediate{1, 0}},
                MakeSpanU8("\xfc\x08\x01\x00"), features);
  ExpectRead<I>(I{O::MemoryDrop, Index{2}}, MakeSpanU8("\xfc\x09\x02"),
                features);
  ExpectRead<I>(I{O::MemoryCopy, CopyImmediate{0, 0}},
                MakeSpanU8("\xfc\x0a\x00\x00"), features);
  ExpectRead<I>(I{O::MemoryFill, u8{0}}, MakeSpanU8("\xfc\x0b\x00"), features);
  ExpectRead<I>(I{O::TableInit, InitImmediate{3, 0}},
                MakeSpanU8("\xfc\x0c\x03\x00"), features);
  ExpectRead<I>(I{O::TableDrop, Index{4}}, MakeSpanU8("\xfc\x0d\x04"),
                features);
  ExpectRead<I>(I{O::TableCopy, CopyImmediate{0, 0}},
                MakeSpanU8("\xfc\x0e\x00\x00"), features);
}

TEST(ReadTest, Limits) {
  ExpectRead<Limits>(Limits{129}, MakeSpanU8("\x00\x81\x01"));
  ExpectRead<Limits>(Limits{2, 1000}, MakeSpanU8("\x01\x02\xe8\x07"));
}

TEST(ReadTest, Limits_BadFlags) {
  ExpectReadFailure<Limits>({{0, "limits"}, {1, "Invalid flags value: 2"}},
                            MakeSpanU8("\x02\x01"));
}

TEST(ReadTest, Limits_PastEnd) {
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {1, "min"}, {1, "u32"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {2, "max"}, {2, "u32"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x01\x00"));
}

TEST(ReadTest, Locals) {
  ExpectRead<Locals>(Locals{2, ValueType::I32}, MakeSpanU8("\x02\x7f"));
  ExpectRead<Locals>(Locals{320, ValueType::F64}, MakeSpanU8("\xc0\x02\x7c"));
}

TEST(ReadTest, Locals_PastEnd) {
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {0, "count"}, {0, "Unable to read u8"}}, MakeSpanU8(""));
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {2, "type"}, {2, "value type"}, {2, "Unable to read u8"}},
      MakeSpanU8("\xc0\x02"));
}

TEST(ReadTest, MemArgImmediate) {
  ExpectRead<MemArgImmediate>(MemArgImmediate{0, 0}, MakeSpanU8("\x00\x00"));
  ExpectRead<MemArgImmediate>(MemArgImmediate{1, 256},
                              MakeSpanU8("\x01\x80\x02"));
}

TEST(ReadTest, Memory) {
  ExpectRead<Memory>(Memory{MemoryType{Limits{1, 2}}},
                     MakeSpanU8("\x01\x01\x02"));
}

TEST(ReadTest, Memory_PastEnd) {
  ExpectReadFailure<Memory>({{0, "memory"},
                             {0, "memory type"},
                             {0, "limits"},
                             {0, "flags"},
                             {0, "Unable to read u8"}},
                            MakeSpanU8(""));
}

TEST(ReadTest, MemoryType) {
  ExpectRead<MemoryType>(MemoryType{Limits{1}}, MakeSpanU8("\x00\x01"));
  ExpectRead<MemoryType>(MemoryType{Limits{0, 128}},
                         MakeSpanU8("\x01\x00\x80\x01"));
}

TEST(ReadTest, MemoryType_PastEnd) {
  ExpectReadFailure<MemoryType>({{0, "memory type"},
                                 {0, "limits"},
                                 {0, "flags"},
                                 {0, "Unable to read u8"}},
                                MakeSpanU8(""));
}

TEST(ReadTest, Mutability) {
  ExpectRead<Mutability>(Mutability::Const, MakeSpanU8("\x00"));
  ExpectRead<Mutability>(Mutability::Var, MakeSpanU8("\x01"));
}

TEST(ReadTest, Mutability_Unknown) {
  ExpectReadFailure<Mutability>(
      {{0, "mutability"}, {1, "Unknown mutability: 4"}}, MakeSpanU8("\x04"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<Mutability>(
      {{0, "mutability"}, {1, "Unknown mutability: 132"}},
      MakeSpanU8("\x84\x00"));
}

TEST(ReadTest, NameAssoc) {
  ExpectRead<NameAssoc>(NameAssoc{2u, "hi"}, MakeSpanU8("\x02\x02hi"));
}

TEST(ReadTest, NameAssoc_PastEnd) {
  ExpectReadFailure<NameAssoc>(
      {{0, "name assoc"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<NameAssoc>(
      {{0, "name assoc"}, {1, "name"}, {1, "length"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, NameSubsectionId) {
  ExpectRead<NameSubsectionId>(NameSubsectionId::ModuleName,
                               MakeSpanU8("\x00"));
  ExpectRead<NameSubsectionId>(NameSubsectionId::FunctionNames,
                               MakeSpanU8("\x01"));
  ExpectRead<NameSubsectionId>(NameSubsectionId::LocalNames,
                               MakeSpanU8("\x02"));
}

TEST(ReadTest, NameSubsectionId_Unknown) {
  ExpectReadFailure<NameSubsectionId>(
      {{0, "name subsection id"}, {1, "Unknown name subsection id: 3"}},
      MakeSpanU8("\x03"));
  ExpectReadFailure<NameSubsectionId>(
      {{0, "name subsection id"}, {1, "Unknown name subsection id: 255"}},
      MakeSpanU8("\xff"));
}

TEST(ReadTest, NameSubsection) {
  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::ModuleName, MakeSpanU8("\0")},
      MakeSpanU8("\x00\x01\0"));

  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::FunctionNames, MakeSpanU8("\0\0")},
      MakeSpanU8("\x01\x02\0\0"));

  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::LocalNames, MakeSpanU8("\0\0\0")},
      MakeSpanU8("\x02\x03\0\0\0"));
}

TEST(ReadTest, NameSubsection_BadSubsectionId) {
  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {0, "name subsection id"},
                                     {1, "Unknown name subsection id: 3"}},
                                    MakeSpanU8("\x03"));
}

TEST(ReadTest, NameSubsection_PastEnd) {
  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {0, "name subsection id"},
                                     {0, "Unable to read u8"}},
                                    MakeSpanU8(""));

  ExpectReadFailure<NameSubsection>(
      {{0, "name subsection"}, {1, "length"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, Opcode) {
  ExpectRead<Opcode>(Opcode::Unreachable, MakeSpanU8("\x00"));
  ExpectRead<Opcode>(Opcode::Nop, MakeSpanU8("\x01"));
  ExpectRead<Opcode>(Opcode::Block, MakeSpanU8("\x02"));
  ExpectRead<Opcode>(Opcode::Loop, MakeSpanU8("\x03"));
  ExpectRead<Opcode>(Opcode::If, MakeSpanU8("\x04"));
  ExpectRead<Opcode>(Opcode::Else, MakeSpanU8("\x05"));
  ExpectRead<Opcode>(Opcode::End, MakeSpanU8("\x0b"));
  ExpectRead<Opcode>(Opcode::Br, MakeSpanU8("\x0c"));
  ExpectRead<Opcode>(Opcode::BrIf, MakeSpanU8("\x0d"));
  ExpectRead<Opcode>(Opcode::BrTable, MakeSpanU8("\x0e"));
  ExpectRead<Opcode>(Opcode::Return, MakeSpanU8("\x0f"));
  ExpectRead<Opcode>(Opcode::Call, MakeSpanU8("\x10"));
  ExpectRead<Opcode>(Opcode::CallIndirect, MakeSpanU8("\x11"));
  ExpectRead<Opcode>(Opcode::Drop, MakeSpanU8("\x1a"));
  ExpectRead<Opcode>(Opcode::Select, MakeSpanU8("\x1b"));
  ExpectRead<Opcode>(Opcode::LocalGet, MakeSpanU8("\x20"));
  ExpectRead<Opcode>(Opcode::LocalSet, MakeSpanU8("\x21"));
  ExpectRead<Opcode>(Opcode::LocalTee, MakeSpanU8("\x22"));
  ExpectRead<Opcode>(Opcode::GlobalGet, MakeSpanU8("\x23"));
  ExpectRead<Opcode>(Opcode::GlobalSet, MakeSpanU8("\x24"));
  ExpectRead<Opcode>(Opcode::I32Load, MakeSpanU8("\x28"));
  ExpectRead<Opcode>(Opcode::I64Load, MakeSpanU8("\x29"));
  ExpectRead<Opcode>(Opcode::F32Load, MakeSpanU8("\x2a"));
  ExpectRead<Opcode>(Opcode::F64Load, MakeSpanU8("\x2b"));
  ExpectRead<Opcode>(Opcode::I32Load8S, MakeSpanU8("\x2c"));
  ExpectRead<Opcode>(Opcode::I32Load8U, MakeSpanU8("\x2d"));
  ExpectRead<Opcode>(Opcode::I32Load16S, MakeSpanU8("\x2e"));
  ExpectRead<Opcode>(Opcode::I32Load16U, MakeSpanU8("\x2f"));
  ExpectRead<Opcode>(Opcode::I64Load8S, MakeSpanU8("\x30"));
  ExpectRead<Opcode>(Opcode::I64Load8U, MakeSpanU8("\x31"));
  ExpectRead<Opcode>(Opcode::I64Load16S, MakeSpanU8("\x32"));
  ExpectRead<Opcode>(Opcode::I64Load16U, MakeSpanU8("\x33"));
  ExpectRead<Opcode>(Opcode::I64Load32S, MakeSpanU8("\x34"));
  ExpectRead<Opcode>(Opcode::I64Load32U, MakeSpanU8("\x35"));
  ExpectRead<Opcode>(Opcode::I32Store, MakeSpanU8("\x36"));
  ExpectRead<Opcode>(Opcode::I64Store, MakeSpanU8("\x37"));
  ExpectRead<Opcode>(Opcode::F32Store, MakeSpanU8("\x38"));
  ExpectRead<Opcode>(Opcode::F64Store, MakeSpanU8("\x39"));
  ExpectRead<Opcode>(Opcode::I32Store8, MakeSpanU8("\x3a"));
  ExpectRead<Opcode>(Opcode::I32Store16, MakeSpanU8("\x3b"));
  ExpectRead<Opcode>(Opcode::I64Store8, MakeSpanU8("\x3c"));
  ExpectRead<Opcode>(Opcode::I64Store16, MakeSpanU8("\x3d"));
  ExpectRead<Opcode>(Opcode::I64Store32, MakeSpanU8("\x3e"));
  ExpectRead<Opcode>(Opcode::MemorySize, MakeSpanU8("\x3f"));
  ExpectRead<Opcode>(Opcode::MemoryGrow, MakeSpanU8("\x40"));
  ExpectRead<Opcode>(Opcode::I32Const, MakeSpanU8("\x41"));
  ExpectRead<Opcode>(Opcode::I64Const, MakeSpanU8("\x42"));
  ExpectRead<Opcode>(Opcode::F32Const, MakeSpanU8("\x43"));
  ExpectRead<Opcode>(Opcode::F64Const, MakeSpanU8("\x44"));
  ExpectRead<Opcode>(Opcode::I32Eqz, MakeSpanU8("\x45"));
  ExpectRead<Opcode>(Opcode::I32Eq, MakeSpanU8("\x46"));
  ExpectRead<Opcode>(Opcode::I32Ne, MakeSpanU8("\x47"));
  ExpectRead<Opcode>(Opcode::I32LtS, MakeSpanU8("\x48"));
  ExpectRead<Opcode>(Opcode::I32LtU, MakeSpanU8("\x49"));
  ExpectRead<Opcode>(Opcode::I32GtS, MakeSpanU8("\x4a"));
  ExpectRead<Opcode>(Opcode::I32GtU, MakeSpanU8("\x4b"));
  ExpectRead<Opcode>(Opcode::I32LeS, MakeSpanU8("\x4c"));
  ExpectRead<Opcode>(Opcode::I32LeU, MakeSpanU8("\x4d"));
  ExpectRead<Opcode>(Opcode::I32GeS, MakeSpanU8("\x4e"));
  ExpectRead<Opcode>(Opcode::I32GeU, MakeSpanU8("\x4f"));
  ExpectRead<Opcode>(Opcode::I64Eqz, MakeSpanU8("\x50"));
  ExpectRead<Opcode>(Opcode::I64Eq, MakeSpanU8("\x51"));
  ExpectRead<Opcode>(Opcode::I64Ne, MakeSpanU8("\x52"));
  ExpectRead<Opcode>(Opcode::I64LtS, MakeSpanU8("\x53"));
  ExpectRead<Opcode>(Opcode::I64LtU, MakeSpanU8("\x54"));
  ExpectRead<Opcode>(Opcode::I64GtS, MakeSpanU8("\x55"));
  ExpectRead<Opcode>(Opcode::I64GtU, MakeSpanU8("\x56"));
  ExpectRead<Opcode>(Opcode::I64LeS, MakeSpanU8("\x57"));
  ExpectRead<Opcode>(Opcode::I64LeU, MakeSpanU8("\x58"));
  ExpectRead<Opcode>(Opcode::I64GeS, MakeSpanU8("\x59"));
  ExpectRead<Opcode>(Opcode::I64GeU, MakeSpanU8("\x5a"));
  ExpectRead<Opcode>(Opcode::F32Eq, MakeSpanU8("\x5b"));
  ExpectRead<Opcode>(Opcode::F32Ne, MakeSpanU8("\x5c"));
  ExpectRead<Opcode>(Opcode::F32Lt, MakeSpanU8("\x5d"));
  ExpectRead<Opcode>(Opcode::F32Gt, MakeSpanU8("\x5e"));
  ExpectRead<Opcode>(Opcode::F32Le, MakeSpanU8("\x5f"));
  ExpectRead<Opcode>(Opcode::F32Ge, MakeSpanU8("\x60"));
  ExpectRead<Opcode>(Opcode::F64Eq, MakeSpanU8("\x61"));
  ExpectRead<Opcode>(Opcode::F64Ne, MakeSpanU8("\x62"));
  ExpectRead<Opcode>(Opcode::F64Lt, MakeSpanU8("\x63"));
  ExpectRead<Opcode>(Opcode::F64Gt, MakeSpanU8("\x64"));
  ExpectRead<Opcode>(Opcode::F64Le, MakeSpanU8("\x65"));
  ExpectRead<Opcode>(Opcode::F64Ge, MakeSpanU8("\x66"));
  ExpectRead<Opcode>(Opcode::I32Clz, MakeSpanU8("\x67"));
  ExpectRead<Opcode>(Opcode::I32Ctz, MakeSpanU8("\x68"));
  ExpectRead<Opcode>(Opcode::I32Popcnt, MakeSpanU8("\x69"));
  ExpectRead<Opcode>(Opcode::I32Add, MakeSpanU8("\x6a"));
  ExpectRead<Opcode>(Opcode::I32Sub, MakeSpanU8("\x6b"));
  ExpectRead<Opcode>(Opcode::I32Mul, MakeSpanU8("\x6c"));
  ExpectRead<Opcode>(Opcode::I32DivS, MakeSpanU8("\x6d"));
  ExpectRead<Opcode>(Opcode::I32DivU, MakeSpanU8("\x6e"));
  ExpectRead<Opcode>(Opcode::I32RemS, MakeSpanU8("\x6f"));
  ExpectRead<Opcode>(Opcode::I32RemU, MakeSpanU8("\x70"));
  ExpectRead<Opcode>(Opcode::I32And, MakeSpanU8("\x71"));
  ExpectRead<Opcode>(Opcode::I32Or, MakeSpanU8("\x72"));
  ExpectRead<Opcode>(Opcode::I32Xor, MakeSpanU8("\x73"));
  ExpectRead<Opcode>(Opcode::I32Shl, MakeSpanU8("\x74"));
  ExpectRead<Opcode>(Opcode::I32ShrS, MakeSpanU8("\x75"));
  ExpectRead<Opcode>(Opcode::I32ShrU, MakeSpanU8("\x76"));
  ExpectRead<Opcode>(Opcode::I32Rotl, MakeSpanU8("\x77"));
  ExpectRead<Opcode>(Opcode::I32Rotr, MakeSpanU8("\x78"));
  ExpectRead<Opcode>(Opcode::I64Clz, MakeSpanU8("\x79"));
  ExpectRead<Opcode>(Opcode::I64Ctz, MakeSpanU8("\x7a"));
  ExpectRead<Opcode>(Opcode::I64Popcnt, MakeSpanU8("\x7b"));
  ExpectRead<Opcode>(Opcode::I64Add, MakeSpanU8("\x7c"));
  ExpectRead<Opcode>(Opcode::I64Sub, MakeSpanU8("\x7d"));
  ExpectRead<Opcode>(Opcode::I64Mul, MakeSpanU8("\x7e"));
  ExpectRead<Opcode>(Opcode::I64DivS, MakeSpanU8("\x7f"));
  ExpectRead<Opcode>(Opcode::I64DivU, MakeSpanU8("\x80"));
  ExpectRead<Opcode>(Opcode::I64RemS, MakeSpanU8("\x81"));
  ExpectRead<Opcode>(Opcode::I64RemU, MakeSpanU8("\x82"));
  ExpectRead<Opcode>(Opcode::I64And, MakeSpanU8("\x83"));
  ExpectRead<Opcode>(Opcode::I64Or, MakeSpanU8("\x84"));
  ExpectRead<Opcode>(Opcode::I64Xor, MakeSpanU8("\x85"));
  ExpectRead<Opcode>(Opcode::I64Shl, MakeSpanU8("\x86"));
  ExpectRead<Opcode>(Opcode::I64ShrS, MakeSpanU8("\x87"));
  ExpectRead<Opcode>(Opcode::I64ShrU, MakeSpanU8("\x88"));
  ExpectRead<Opcode>(Opcode::I64Rotl, MakeSpanU8("\x89"));
  ExpectRead<Opcode>(Opcode::I64Rotr, MakeSpanU8("\x8a"));
  ExpectRead<Opcode>(Opcode::F32Abs, MakeSpanU8("\x8b"));
  ExpectRead<Opcode>(Opcode::F32Neg, MakeSpanU8("\x8c"));
  ExpectRead<Opcode>(Opcode::F32Ceil, MakeSpanU8("\x8d"));
  ExpectRead<Opcode>(Opcode::F32Floor, MakeSpanU8("\x8e"));
  ExpectRead<Opcode>(Opcode::F32Trunc, MakeSpanU8("\x8f"));
  ExpectRead<Opcode>(Opcode::F32Nearest, MakeSpanU8("\x90"));
  ExpectRead<Opcode>(Opcode::F32Sqrt, MakeSpanU8("\x91"));
  ExpectRead<Opcode>(Opcode::F32Add, MakeSpanU8("\x92"));
  ExpectRead<Opcode>(Opcode::F32Sub, MakeSpanU8("\x93"));
  ExpectRead<Opcode>(Opcode::F32Mul, MakeSpanU8("\x94"));
  ExpectRead<Opcode>(Opcode::F32Div, MakeSpanU8("\x95"));
  ExpectRead<Opcode>(Opcode::F32Min, MakeSpanU8("\x96"));
  ExpectRead<Opcode>(Opcode::F32Max, MakeSpanU8("\x97"));
  ExpectRead<Opcode>(Opcode::F32Copysign, MakeSpanU8("\x98"));
  ExpectRead<Opcode>(Opcode::F64Abs, MakeSpanU8("\x99"));
  ExpectRead<Opcode>(Opcode::F64Neg, MakeSpanU8("\x9a"));
  ExpectRead<Opcode>(Opcode::F64Ceil, MakeSpanU8("\x9b"));
  ExpectRead<Opcode>(Opcode::F64Floor, MakeSpanU8("\x9c"));
  ExpectRead<Opcode>(Opcode::F64Trunc, MakeSpanU8("\x9d"));
  ExpectRead<Opcode>(Opcode::F64Nearest, MakeSpanU8("\x9e"));
  ExpectRead<Opcode>(Opcode::F64Sqrt, MakeSpanU8("\x9f"));
  ExpectRead<Opcode>(Opcode::F64Add, MakeSpanU8("\xa0"));
  ExpectRead<Opcode>(Opcode::F64Sub, MakeSpanU8("\xa1"));
  ExpectRead<Opcode>(Opcode::F64Mul, MakeSpanU8("\xa2"));
  ExpectRead<Opcode>(Opcode::F64Div, MakeSpanU8("\xa3"));
  ExpectRead<Opcode>(Opcode::F64Min, MakeSpanU8("\xa4"));
  ExpectRead<Opcode>(Opcode::F64Max, MakeSpanU8("\xa5"));
  ExpectRead<Opcode>(Opcode::F64Copysign, MakeSpanU8("\xa6"));
  ExpectRead<Opcode>(Opcode::I32WrapI64, MakeSpanU8("\xa7"));
  ExpectRead<Opcode>(Opcode::I32TruncF32S, MakeSpanU8("\xa8"));
  ExpectRead<Opcode>(Opcode::I32TruncF32U, MakeSpanU8("\xa9"));
  ExpectRead<Opcode>(Opcode::I32TruncF64S, MakeSpanU8("\xaa"));
  ExpectRead<Opcode>(Opcode::I32TruncF64U, MakeSpanU8("\xab"));
  ExpectRead<Opcode>(Opcode::I64ExtendI32S, MakeSpanU8("\xac"));
  ExpectRead<Opcode>(Opcode::I64ExtendI32U, MakeSpanU8("\xad"));
  ExpectRead<Opcode>(Opcode::I64TruncF32S, MakeSpanU8("\xae"));
  ExpectRead<Opcode>(Opcode::I64TruncF32U, MakeSpanU8("\xaf"));
  ExpectRead<Opcode>(Opcode::I64TruncF64S, MakeSpanU8("\xb0"));
  ExpectRead<Opcode>(Opcode::I64TruncF64U, MakeSpanU8("\xb1"));
  ExpectRead<Opcode>(Opcode::F32ConvertI32S, MakeSpanU8("\xb2"));
  ExpectRead<Opcode>(Opcode::F32ConvertI32U, MakeSpanU8("\xb3"));
  ExpectRead<Opcode>(Opcode::F32ConvertI64S, MakeSpanU8("\xb4"));
  ExpectRead<Opcode>(Opcode::F32ConvertI64U, MakeSpanU8("\xb5"));
  ExpectRead<Opcode>(Opcode::F32DemoteF64, MakeSpanU8("\xb6"));
  ExpectRead<Opcode>(Opcode::F64ConvertI32S, MakeSpanU8("\xb7"));
  ExpectRead<Opcode>(Opcode::F64ConvertI32U, MakeSpanU8("\xb8"));
  ExpectRead<Opcode>(Opcode::F64ConvertI64S, MakeSpanU8("\xb9"));
  ExpectRead<Opcode>(Opcode::F64ConvertI64U, MakeSpanU8("\xba"));
  ExpectRead<Opcode>(Opcode::F64PromoteF32, MakeSpanU8("\xbb"));
  ExpectRead<Opcode>(Opcode::I32ReinterpretF32, MakeSpanU8("\xbc"));
  ExpectRead<Opcode>(Opcode::I64ReinterpretF64, MakeSpanU8("\xbd"));
  ExpectRead<Opcode>(Opcode::F32ReinterpretI32, MakeSpanU8("\xbe"));
  ExpectRead<Opcode>(Opcode::F64ReinterpretI64, MakeSpanU8("\xbf"));
}

namespace {

void ExpectUnknownOpcode(u8 code) {
  const u8 span_buffer[] = {code};
  auto msg = format("Unknown opcode: {}", code);
  ExpectReadFailure<Opcode>({{0, "opcode"}, {1, msg}}, SpanU8{span_buffer, 1});
}

void ExpectUnknownOpcode(u8 prefix,
                         u32 code,
                         SpanU8 span,
                         const Features& features) {
  ExpectReadFailure<Opcode>(
      {{0, "opcode"},
       {span.size(), format("Unknown opcode: {} {}", prefix, code)}},
      span, features);
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

TEST(ReadTest, Opcode_sign_extension) {
  Features features;
  features.enable_sign_extension();

  ExpectRead<Opcode>(Opcode::I32Extend8S, MakeSpanU8("\xc0"), features);
  ExpectRead<Opcode>(Opcode::I32Extend16S, MakeSpanU8("\xc1"), features);
  ExpectRead<Opcode>(Opcode::I64Extend8S, MakeSpanU8("\xc2"), features);
  ExpectRead<Opcode>(Opcode::I64Extend16S, MakeSpanU8("\xc3"), features);
  ExpectRead<Opcode>(Opcode::I64Extend32S, MakeSpanU8("\xc4"), features);
}

TEST(ReadTest, Opcode_saturating_float_to_int) {
  Features features;
  features.enable_saturating_float_to_int();

  ExpectRead<Opcode>(Opcode::I32TruncSatF32S, MakeSpanU8("\xfc\x00"), features);
  ExpectRead<Opcode>(Opcode::I32TruncSatF32U, MakeSpanU8("\xfc\x01"), features);
  ExpectRead<Opcode>(Opcode::I32TruncSatF64S, MakeSpanU8("\xfc\x02"), features);
  ExpectRead<Opcode>(Opcode::I32TruncSatF64U, MakeSpanU8("\xfc\x03"), features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF32S, MakeSpanU8("\xfc\x04"), features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF32U, MakeSpanU8("\xfc\x05"), features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF64S, MakeSpanU8("\xfc\x06"), features);
  ExpectRead<Opcode>(Opcode::I64TruncSatF64U, MakeSpanU8("\xfc\x07"), features);
}

TEST(ReadTest, Opcode_bulk_memory) {
  Features features;
  features.enable_bulk_memory();

  ExpectRead<Opcode>(Opcode::MemoryInit, MakeSpanU8("\xfc\x08"), features);
  ExpectRead<Opcode>(Opcode::MemoryDrop, MakeSpanU8("\xfc\x09"), features);
  ExpectRead<Opcode>(Opcode::MemoryCopy, MakeSpanU8("\xfc\x0a"), features);
  ExpectRead<Opcode>(Opcode::MemoryFill, MakeSpanU8("\xfc\x0b"), features);
  ExpectRead<Opcode>(Opcode::TableInit, MakeSpanU8("\xfc\x0c"), features);
  ExpectRead<Opcode>(Opcode::TableDrop, MakeSpanU8("\xfc\x0d"), features);
  ExpectRead<Opcode>(Opcode::TableCopy, MakeSpanU8("\xfc\x0e"), features);
}

TEST(ReadTest, Opcode_disabled_misc_prefix) {
  {
    Features features;
    features.enable_saturating_float_to_int();
    ExpectUnknownOpcode(0xfc, 8, MakeSpanU8("\xfc\x08"), features);
    ExpectUnknownOpcode(0xfc, 9, MakeSpanU8("\xfc\x09"), features);
    ExpectUnknownOpcode(0xfc, 10, MakeSpanU8("\xfc\x0a"), features);
    ExpectUnknownOpcode(0xfc, 11, MakeSpanU8("\xfc\x0b"), features);
    ExpectUnknownOpcode(0xfc, 12, MakeSpanU8("\xfc\x0c"), features);
    ExpectUnknownOpcode(0xfc, 13, MakeSpanU8("\xfc\x0d"), features);
    ExpectUnknownOpcode(0xfc, 14, MakeSpanU8("\xfc\x0e"), features);
  }

  {
    Features features;
    features.enable_bulk_memory();
    ExpectUnknownOpcode(0xfc, 0, MakeSpanU8("\xfc\x00"), features);
    ExpectUnknownOpcode(0xfc, 1, MakeSpanU8("\xfc\x01"), features);
    ExpectUnknownOpcode(0xfc, 2, MakeSpanU8("\xfc\x02"), features);
    ExpectUnknownOpcode(0xfc, 3, MakeSpanU8("\xfc\x03"), features);
    ExpectUnknownOpcode(0xfc, 4, MakeSpanU8("\xfc\x04"), features);
    ExpectUnknownOpcode(0xfc, 5, MakeSpanU8("\xfc\x05"), features);
    ExpectUnknownOpcode(0xfc, 6, MakeSpanU8("\xfc\x06"), features);
    ExpectUnknownOpcode(0xfc, 7, MakeSpanU8("\xfc\x07"), features);
  }
}

TEST(ReadTest, Opcode_Unknown_misc_prefix) {
  Features features;
  features.enable_saturating_float_to_int();
  features.enable_bulk_memory();

  for (u8 code = 0x0f; code < 0x7f; ++code) {
    const u8 span_buffer[] = {0xfc, code};
    auto msg = format("Unknown opcode: 252 {}", code);
    ExpectReadFailure<Opcode>({{0, "opcode"}, {2, msg}}, SpanU8{span_buffer, 2},
                              features);
  }

  // Test some longer codes too.
  ExpectUnknownOpcode(0xfc, 128, MakeSpanU8("\xfc\x80\x01"), features);
  ExpectUnknownOpcode(0xfc, 16384, MakeSpanU8("\xfc\x80\x80\x01"), features);
  ExpectUnknownOpcode(0xfc, 2097152, MakeSpanU8("\xfc\x80\x80\x80\x01"),
                      features);
  ExpectUnknownOpcode(0xfc, 268435456, MakeSpanU8("\xfc\x80\x80\x80\x80\x01"),
                      features);
}

TEST(ReadTest, S32) {
  ExpectRead<s32>(32, MakeSpanU8("\x20"));
  ExpectRead<s32>(-16, MakeSpanU8("\x70"));
  ExpectRead<s32>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s32>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s32>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s32>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s32>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s32>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s32>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s32>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
}

TEST(ReadTest, S32_TooLong) {
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x5 or 0x7d, got 0x15"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0\x15"));
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x3 or 0x7b, got 0x73"}},
                         MakeSpanU8("\xff\xff\xff\xff\x73"));
}

TEST(ReadTest, S32_PastEnd) {
  ExpectReadFailure<s32>({{0, "s32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s32>({{0, "s32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s32>({{0, "s32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s32>({{0, "s32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s32>({{0, "s32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

TEST(ReadTest, S64) {
  ExpectRead<s64>(32, MakeSpanU8("\x20"));
  ExpectRead<s64>(-16, MakeSpanU8("\x70"));
  ExpectRead<s64>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s64>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s64>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s64>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s64>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s64>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s64>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s64>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
  ExpectRead<s64>(13893120096, MakeSpanU8("\xe0\xe0\xe0\xe0\x33"));
  ExpectRead<s64>(-12413554592, MakeSpanU8("\xe0\xe0\xe0\xe0\x51"));
  ExpectRead<s64>(1533472417872, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x2c"));
  ExpectRead<s64>(-287593715632, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x77"));
  ExpectRead<s64>(139105536057408, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"));
  ExpectRead<s64>(-124777254608832, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x63"));
  ExpectRead<s64>(1338117014066474,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"));
  ExpectRead<s64>(-12172681868045014,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"));
  ExpectRead<s64>(1070725794579330814,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"));
  ExpectRead<s64>(-3540960223848057090,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"));
}

TEST(ReadTest, S64_TooLong) {
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xf0"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xff"}},
      MakeSpanU8("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"));
}

TEST(ReadTest, S64_PastEnd) {
  ExpectReadFailure<s64>({{0, "s64"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s64>({{0, "s64"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s64>({{0, "s64"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>({{0, "s64"}, {5, "Unable to read u8"}},
                         MakeSpanU8("\xe0\xe0\xe0\xe0\xe0"));
  ExpectReadFailure<s64>({{0, "s64"}, {6, "Unable to read u8"}},
                         MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {7, "Unable to read u8"}},
                         MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x84"));
  ExpectReadFailure<s64>({{0, "s64"}, {8, "Unable to read u8"}},
                         MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {9, "Unable to read u8"}},
                         MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"));
}

TEST(ReadTest, SectionId) {
  ExpectRead<SectionId>(SectionId::Custom, MakeSpanU8("\x00"));
  ExpectRead<SectionId>(SectionId::Type, MakeSpanU8("\x01"));
  ExpectRead<SectionId>(SectionId::Import, MakeSpanU8("\x02"));
  ExpectRead<SectionId>(SectionId::Function, MakeSpanU8("\x03"));
  ExpectRead<SectionId>(SectionId::Table, MakeSpanU8("\x04"));
  ExpectRead<SectionId>(SectionId::Memory, MakeSpanU8("\x05"));
  ExpectRead<SectionId>(SectionId::Global, MakeSpanU8("\x06"));
  ExpectRead<SectionId>(SectionId::Export, MakeSpanU8("\x07"));
  ExpectRead<SectionId>(SectionId::Start, MakeSpanU8("\x08"));
  ExpectRead<SectionId>(SectionId::Element, MakeSpanU8("\x09"));
  ExpectRead<SectionId>(SectionId::Code, MakeSpanU8("\x0a"));
  ExpectRead<SectionId>(SectionId::Data, MakeSpanU8("\x0b"));

  // Overlong encoding.
  ExpectRead<SectionId>(SectionId::Custom, MakeSpanU8("\x80\x00"));
}

TEST(ReadTest, SectionId_Unknown) {
  ExpectReadFailure<SectionId>(
      {{0, "section id"}, {1, "Unknown section id: 12"}}, MakeSpanU8("\x0c"));
}

TEST(ReadTest, Section) {
  ExpectRead<Section>(
      Section{KnownSection{SectionId::Type, MakeSpanU8("\x01\x02\x03")}},
      MakeSpanU8("\x01\x03\x01\x02\x03"));

  ExpectRead<Section>(
      Section{CustomSection{"name", MakeSpanU8("\x04\x05\x06")}},
      MakeSpanU8("\x00\x08\x04name\x04\x05\x06"));
}

TEST(ReadTest, Section_PastEnd) {
  ExpectReadFailure<Section>(
      {{0, "section"}, {0, "section id"}, {0, "u32"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<Section>(
      {{0, "section"}, {1, "length"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x01"));

  ExpectReadFailure<Section>(
      {{0, "section"}, {2, "Length extends past end: 1 > 0"}},
      MakeSpanU8("\x01\x01"));
}

TEST(ReadTest, Start) {
  ExpectRead<Start>(Start{256}, MakeSpanU8("\x80\x02"));
}

TEST(ReadTest, ReadString) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ(string_view{"hello"}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadString_Leftovers) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01more");
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
    const SpanU8 data = MakeSpanU8("");
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
    const SpanU8 data = MakeSpanU8("\xc0");
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
  const SpanU8 data = MakeSpanU8("\x06small");
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {1, "Length extends past end: 6 > 5"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(5u, copy.size());
}

TEST(ReadTest, Table) {
  ExpectRead<Table>(Table{TableType{Limits{1}, ElementType::Funcref}},
                    MakeSpanU8("\x70\x00\x01"));
}

TEST(ReadTest, Table_PastEnd) {
  ExpectReadFailure<Table>({{0, "table"},
                            {0, "table type"},
                            {0, "element type"},
                            {0, "Unable to read u8"}},
                           MakeSpanU8(""));
}

TEST(ReadTest, TableType) {
  ExpectRead<TableType>(TableType{Limits{1}, ElementType::Funcref},
                        MakeSpanU8("\x70\x00\x01"));
  ExpectRead<TableType>(TableType{Limits{1, 2}, ElementType::Funcref},
                        MakeSpanU8("\x70\x01\x01\x02"));
}

TEST(ReadTest, TableType_BadElementType) {
  ExpectReadFailure<TableType>(
      {{0, "table type"}, {0, "element type"}, {1, "Unknown element type: 0"}},
      MakeSpanU8("\x00"));
}

TEST(ReadTest, TableType_PastEnd) {
  ExpectReadFailure<TableType>(
      {{0, "table type"}, {0, "element type"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<TableType>({{0, "table type"},
                                {1, "limits"},
                                {1, "flags"},
                                {1, "Unable to read u8"}},
                               MakeSpanU8("\x70"));
}

TEST(ReadTest, TypeEntry) {
  ExpectRead<TypeEntry>(TypeEntry{FunctionType{{}, {ValueType::I32}}},
                        MakeSpanU8("\x60\x00\x01\x7f"));
}

TEST(ReadTest, TypeEntry_BadForm) {
  ExpectReadFailure<TypeEntry>(
      {{0, "type entry"}, {1, "Unknown type form: 64"}}, MakeSpanU8("\x40"));
}

TEST(ReadTest, U32) {
  ExpectRead<u32>(32u, MakeSpanU8("\x20"));
  ExpectRead<u32>(448u, MakeSpanU8("\xc0\x03"));
  ExpectRead<u32>(33360u, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<u32>(101718048u, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<u32>(1042036848u, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
}

TEST(ReadTest, U32_TooLong) {
  ExpectReadFailure<u32>(
      {{0, "u32"},
       {5, "Last byte of u32 must be zero extension: expected 0x2, got 0x12"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\x12"));
}

TEST(ReadTest, U32_PastEnd) {
  ExpectReadFailure<u32>({{0, "u32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<u32>({{0, "u32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<u32>({{0, "u32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<u32>({{0, "u32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<u32>({{0, "u32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

TEST(ReadTest, U8) {
  ExpectRead<u8>(32, MakeSpanU8("\x20"));
  ExpectReadFailure<u8>({{0, "Unable to read u8"}}, MakeSpanU8(""));
}

TEST(ReadTest, ValueType) {
  ExpectRead<ValueType>(ValueType::I32, MakeSpanU8("\x7f"));
  ExpectRead<ValueType>(ValueType::I64, MakeSpanU8("\x7e"));
  ExpectRead<ValueType>(ValueType::F32, MakeSpanU8("\x7d"));
  ExpectRead<ValueType>(ValueType::F64, MakeSpanU8("\x7c"));
}

TEST(ReadTest, ValueType_Unknown) {
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 16"}}, MakeSpanU8("\x10"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 255"}},
      MakeSpanU8("\xff\x7f"));
}

TEST(ReadTest, ReadVector_u8) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadVector<u8>(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<u8>{'h', 'e', 'l', 'l', 'o'}), result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadVector_u32) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<u32>{5, 128, 206412}), result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReadTest, ReadVector_FailLength) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05");
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
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05"
      "\x80");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {2, "u32"}, {3, "Unable to read u8"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(0u, copy.size());
}
