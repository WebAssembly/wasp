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

#include "wasp/text/read/lex.h"

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/text/read/tokenizer.h"
#include "wasp/text/types.h"

using namespace ::wasp;
using namespace ::wasp::text;

using LI = LiteralInfo;
using HU = HasUnderscores;

struct ExpectedToken {
  ExpectedToken(Location::index_type size, TokenType type)
      : size{size}, type{type} {}
  ExpectedToken(Location::index_type size, TokenType type, LiteralInfo info)
      : size{size}, type{type}, immediate{info} {}
  ExpectedToken(Location::index_type size,
                TokenType type,
                Opcode opcode,
                Features::Bits features = 0)
      : size{size},
        type{type},
        immediate{OpcodeInfo{opcode, Features{features}}} {}
  ExpectedToken(Location::index_type size, TokenType type, ValueType value_type)
      : size{size}, type{type}, immediate{value_type} {}
  ExpectedToken(Location::index_type size,
                TokenType type,
                ReferenceType reftype)
      : size{size}, type{type}, immediate{reftype} {}
  ExpectedToken(Location::index_type size, TokenType type, Text text)
      : size{size}, type{type}, immediate{text} {}

  Location::index_type size;
  TokenType type;
  Token::Immediate immediate;
};

using TT = TokenType;

SpanU8 ExpectLex(ExpectedToken et, SpanU8 data) {
  Token expected{Location{data.begin(), et.size}, et.type, et.immediate};
  auto actual = Lex(&data);
  EXPECT_EQ(actual, expected)
      << format("expected: {} actual: {} (\"{}\")", expected.loc, actual.loc,
                ToStringView(actual.loc));
  return data;
}

TEST(LexTest, Eof) {
  ExpectLex({0, TokenType::Eof}, ""_su8);
}


TEST(LexTest, InvalidBlockComment) {
  ExpectLex({2, TT::InvalidBlockComment}, "(;"_su8);
  ExpectLex({6, TT::InvalidBlockComment}, "(;   ;"_su8);
  ExpectLex({6, TT::InvalidBlockComment}, "(;(;;)"_su8);
}

TEST(LexTest, InvalidChar) {
  for (u8 c : {0,  1,  2,  3,  4,  5,  6,  7,  8,  11, 12, 14, 15, 16, 17,
               18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31}) {
    ExpectLex({1, TT::InvalidChar}, SpanU8{&c, 1});
  }
  for (int i = 128; i < 256; ++i) {
    u8 c = i;
    ExpectLex({1, TT::InvalidChar}, SpanU8{&c, 1});
  }
}

TEST(LexTest, InvalidLineComment) {
  ExpectLex({2, TT::InvalidLineComment}, ";;"_su8);
  ExpectLex({6, TT::InvalidLineComment}, ";;   ;"_su8);
  ExpectLex({14, TT::InvalidLineComment}, ";; end of file"_su8);
}

TEST(LexTest, InvalidText_MissingQuote) {
  ExpectLex({1, TT::InvalidText}, "\""_su8);
  ExpectLex({12, TT::InvalidText}, "\"other stuff"_su8);
}

TEST(LexTest, InvalidText_HasNewline) {
  ExpectLex({2, TT::InvalidText}, "\"\n"_su8);
  ExpectLex({13, TT::InvalidText}, "\"other stuff\n"_su8);
}

TEST(LexTest, InvalidText_BadEscape) {
  const string_view valid_escapes = "nrt\"'\\0123456789abcdefABCDEF";
  for (int c = 0; c < 256; ++c) {
    if (std::find(valid_escapes.begin(), valid_escapes.end(), c) !=
        valid_escapes.end()) {
      continue;
    }
    u8 s[] = {'"', '\\', u8(c), '"', 0};
    ExpectLex({4, TT::InvalidText}, SpanU8(s, sizeof(s) - 1));
  }
}

TEST(LexTest, BlockComment) {
  ExpectLex({4, TT::BlockComment}, "(;;)"_su8);
  ExpectLex({11, TT::BlockComment}, "(;comment;)"_su8);
  ExpectLex({32, TT::BlockComment}, "(; (; nested ;) (; another ;) ;)"_su8);
}

TEST(LexTest, LineComment) {
  ExpectLex({3, TT::LineComment}, ";;\n"_su8);
  ExpectLex({7, TT::LineComment}, ";;   ;\n"_su8);
  ExpectLex({15, TT::LineComment}, ";; end of line\nnext line"_su8);
}

TEST(LexTest, Reserved) {
  ExpectLex({1, TT::Reserved}, "$"_su8);
  ExpectLex({3, TT::Reserved}, "abc"_su8);
  ExpectLex({6, TT::Reserved}, "<html>"_su8);
  ExpectLex({22, TT::Reserved}, "!#$%&\'*+-./:<=>?@\\^_`|"_su8);
  ExpectLex({8, TT::Reserved}, "23skidoo"_su8);
  ExpectLex({8, TT::Reserved}, "i32.addd"_su8);
  ExpectLex({5, TT::Reserved}, "32.5x"_su8);
}

TEST(LexTest, Whitespace) {
  for (u8 c : {' ', '\t', '\n'}) {
    ExpectLex({1, TT::Whitespace}, SpanU8(&c, 1));
  }

  ExpectLex({11, TT::Whitespace}, "           "_su8);
  ExpectLex({6, TT::Whitespace}, "\n\n\n\n\n\n"_su8);
  ExpectLex({6, TT::Whitespace}, "\t\t\t\t\t\t"_su8);
  ExpectLex({9, TT::Whitespace}, " \n\t \n\t \n\t"_su8);
}

TEST(LexTest, AlignEqNat) {
  ExpectLex({9, TT::AlignEqNat, LI::Nat(HU::No)}, "align=123"_su8);
  ExpectLex({11, TT::AlignEqNat, LI::Nat(HU::Yes)}, "align=1_234"_su8);
  ExpectLex({11, TT::AlignEqNat, LI::HexNat(HU::No)}, "align=0xabc"_su8);
  ExpectLex({12, TT::AlignEqNat, LI::HexNat(HU::Yes)}, "align=0xa_bc"_su8);

  ExpectLex({6, TT::Reserved}, "align="_su8);
  ExpectLex({8, TT::Reserved}, "align=1x"_su8);
  ExpectLex({8, TT::Reserved}, "align=$1"_su8);
  ExpectLex({10, TT::Reserved}, "align=0xzq"_su8);
  ExpectLex({10, TT::Reserved}, "align=1__2"_su8);
}

TEST(LexTest, OffsetEqNat) {
  ExpectLex({10, TT::OffsetEqNat, LI::Nat(HU::No)}, "offset=123"_su8);
  ExpectLex({12, TT::OffsetEqNat, LI::Nat(HU::Yes)}, "offset=1_234"_su8);
  ExpectLex({12, TT::OffsetEqNat, LI::HexNat(HU::No)}, "offset=0xabc"_su8);
  ExpectLex({13, TT::OffsetEqNat, LI::HexNat(HU::Yes)}, "offset=0xa_bc"_su8);

  ExpectLex({7, TT::Reserved}, "offset="_su8);
  ExpectLex({9, TT::Reserved}, "offset=1x"_su8);
  ExpectLex({9, TT::Reserved}, "offset=$1"_su8);
  ExpectLex({11, TT::Reserved}, "offset=0xzq"_su8);
  ExpectLex({11, TT::Reserved}, "offset=1__2"_su8);
}

TEST(LexTest, Keyword) {
  struct {
    SpanU8 span;
    TokenType type;
  } tests[] = {
      // .wat keywords
      {"("_su8, TT::Lpar},
      {")"_su8, TT::Rpar},
      {"binary"_su8, TT::Binary},
      {"data"_su8, TT::Data},
      {"elem"_su8, TT::Elem},
      {"event"_su8, TT::Event},
      {"export"_su8, TT::Export},
      {"f32x4"_su8, TT::F32X4},
      {"f64x2"_su8, TT::F64X2},
      {"global"_su8, TT::Global},
      {"i16x8"_su8, TT::I16X8},
      {"i32x4"_su8, TT::I32X4},
      {"i64x2"_su8, TT::I64X2},
      {"i8x16"_su8, TT::I8X16},
      {"import"_su8, TT::Import},
      {"item"_su8, TT::Item},
      {"local"_su8, TT::Local},
      {"memory"_su8, TT::Memory},
      {"module"_su8, TT::Module},
      {"mut"_su8, TT::Mut},
      {"offset"_su8, TT::Offset},
      {"param"_su8, TT::Param},
      {"quote"_su8, TT::Quote},
      {"result"_su8, TT::Result},
      {"shared"_su8, TT::Shared},
      {"start"_su8, TT::Start},
      {"table"_su8, TT::Table},
      {"then"_su8, TT::Then},
      {"type"_su8, TT::Type},

      // .wast keywords
      {"assert_exhaustion"_su8, TT::AssertExhaustion},
      {"assert_invalid"_su8, TT::AssertInvalid},
      {"assert_malformed"_su8, TT::AssertMalformed},
      {"assert_return"_su8, TT::AssertReturn},
      {"assert_trap"_su8, TT::AssertTrap},
      {"assert_unlinkable"_su8, TT::AssertUnlinkable},
      {"get"_su8, TT::Get},
      {"invoke"_su8, TT::Invoke},
      {"nan:arithmetic"_su8, TT::NanArithmetic},
      {"nan:canonical"_su8, TT::NanCanonical},
      {"ref.extern"_su8, TT::RefExtern},
      {"register"_su8, TT::Register},

  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), test.type}, test.span);
  }
}

TEST(LexTest, OpcodeKeywords) {
  struct {
    SpanU8 span;
    TokenType type;
    Opcode opcode;
  } tests[] = {
      {"catch"_su8, TT::Catch, Opcode::Catch},
      {"else"_su8, TT::Else, Opcode::Else},
      {"end"_su8, TT::End, Opcode::End},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), test.type, test.opcode}, test.span);
  }
}

TEST(LexTest, BlockInstr) {
  struct {
    SpanU8 span;
    Opcode opcode;
    Features::Bits features;
  } tests[] = {
      {"block"_su8, Opcode::Block, 0},
      {"if"_su8, Opcode::If, 0},
      {"loop"_su8, Opcode::Loop, 0},
      {"try"_su8, Opcode::Try, Features::Exceptions},
  };
  for (auto test : tests) {
    ExpectLex(
        {test.span.size(), TokenType::BlockInstr, test.opcode, test.features},
        test.span);
  }
}

TEST(LexTest, PlainInstr) {
  using TT = TokenType;
  using O = Opcode;
  using F = Features;

  struct {
    SpanU8 span;
    TT type;
    O opcode;
    F::Bits features;
  } tests[] = {
      {"br_if"_su8, TT::VarInstr, O::BrIf, 0},
      {"br_on_exn"_su8, TT::BrOnExnInstr, O::BrOnExn, F::Exceptions},
      {"br_table"_su8, TT::BrTableInstr, O::BrTable, 0},
      {"br"_su8, TT::VarInstr, O::Br, 0},
      {"call_indirect"_su8, TT::CallIndirectInstr, O::CallIndirect, 0},
      {"call"_su8, TT::VarInstr, O::Call, 0},
      {"data.drop"_su8, TT::VarInstr, O::DataDrop, F::BulkMemory},
      {"drop"_su8, TT::BareInstr, O::Drop, 0},
      {"elem.drop"_su8, TT::VarInstr, O::ElemDrop, F::BulkMemory},
      {"f32.abs"_su8, TT::BareInstr, O::F32Abs, 0},
      {"f32.add"_su8, TT::BareInstr, O::F32Add, 0},
      {"f32.ceil"_su8, TT::BareInstr, O::F32Ceil, 0},
      {"f32.const"_su8, TT::F32ConstInstr, O::F32Const, 0},
      {"f32.convert_i32_s"_su8, TT::BareInstr, O::F32ConvertI32S, 0},
      {"f32.convert_i32_u"_su8, TT::BareInstr, O::F32ConvertI32U, 0},
      {"f32.convert_i64_s"_su8, TT::BareInstr, O::F32ConvertI64S, 0},
      {"f32.convert_i64_u"_su8, TT::BareInstr, O::F32ConvertI64U, 0},
      {"f32.copysign"_su8, TT::BareInstr, O::F32Copysign, 0},
      {"f32.demote_f64"_su8, TT::BareInstr, O::F32DemoteF64, 0},
      {"f32.div"_su8, TT::BareInstr, O::F32Div, 0},
      {"f32.eq"_su8, TT::BareInstr, O::F32Eq, 0},
      {"f32.floor"_su8, TT::BareInstr, O::F32Floor, 0},
      {"f32.ge"_su8, TT::BareInstr, O::F32Ge, 0},
      {"f32.gt"_su8, TT::BareInstr, O::F32Gt, 0},
      {"f32.le"_su8, TT::BareInstr, O::F32Le, 0},
      {"f32.load"_su8, TT::MemoryInstr, O::F32Load, 0},
      {"f32.lt"_su8, TT::BareInstr, O::F32Lt, 0},
      {"f32.max"_su8, TT::BareInstr, O::F32Max, 0},
      {"f32.min"_su8, TT::BareInstr, O::F32Min, 0},
      {"f32.mul"_su8, TT::BareInstr, O::F32Mul, 0},
      {"f32.nearest"_su8, TT::BareInstr, O::F32Nearest, 0},
      {"f32.neg"_su8, TT::BareInstr, O::F32Neg, 0},
      {"f32.ne"_su8, TT::BareInstr, O::F32Ne, 0},
      {"f32.reinterpret_i32"_su8, TT::BareInstr, O::F32ReinterpretI32, 0},
      {"f32.sqrt"_su8, TT::BareInstr, O::F32Sqrt, 0},
      {"f32.store"_su8, TT::MemoryInstr, O::F32Store, 0},
      {"f32.sub"_su8, TT::BareInstr, O::F32Sub, 0},
      {"f32.trunc"_su8, TT::BareInstr, O::F32Trunc, 0},
      {"f32x4.abs"_su8, TT::BareInstr, O::F32X4Abs, F::Simd},
      {"f32x4.add"_su8, TT::BareInstr, O::F32X4Add, F::Simd},
      {"f32x4.convert_i32x4_s"_su8, TT::BareInstr, O::F32X4ConvertI32X4S, F::Simd},
      {"f32x4.convert_i32x4_u"_su8, TT::BareInstr, O::F32X4ConvertI32X4U, F::Simd},
      {"f32x4.div"_su8, TT::BareInstr, O::F32X4Div, F::Simd},
      {"f32x4.eq"_su8, TT::BareInstr, O::F32X4Eq, F::Simd},
      {"f32x4.extract_lane"_su8, TT::SimdLaneInstr, O::F32X4ExtractLane, F::Simd},
      {"f32x4.ge"_su8, TT::BareInstr, O::F32X4Ge, F::Simd},
      {"f32x4.gt"_su8, TT::BareInstr, O::F32X4Gt, F::Simd},
      {"f32x4.le"_su8, TT::BareInstr, O::F32X4Le, F::Simd},
      {"f32x4.lt"_su8, TT::BareInstr, O::F32X4Lt, F::Simd},
      {"f32x4.max"_su8, TT::BareInstr, O::F32X4Max, F::Simd},
      {"f32x4.min"_su8, TT::BareInstr, O::F32X4Min, F::Simd},
      {"f32x4.mul"_su8, TT::BareInstr, O::F32X4Mul, F::Simd},
      {"f32x4.neg"_su8, TT::BareInstr, O::F32X4Neg, F::Simd},
      {"f32x4.ne"_su8, TT::BareInstr, O::F32X4Ne, F::Simd},
      {"f32x4.replace_lane"_su8, TT::SimdLaneInstr, O::F32X4ReplaceLane, F::Simd},
      {"f32x4.splat"_su8, TT::BareInstr, O::F32X4Splat, F::Simd},
      {"f32x4.sqrt"_su8, TT::BareInstr, O::F32X4Sqrt, F::Simd},
      {"f32x4.sub"_su8, TT::BareInstr, O::F32X4Sub, F::Simd},
      {"f64.abs"_su8, TT::BareInstr, O::F64Abs, 0},
      {"f64.add"_su8, TT::BareInstr, O::F64Add, 0},
      {"f64.ceil"_su8, TT::BareInstr, O::F64Ceil, 0},
      {"f64.const"_su8, TT::F64ConstInstr, O::F64Const, 0},
      {"f64.convert_i32_s"_su8, TT::BareInstr, O::F64ConvertI32S, 0},
      {"f64.convert_i32_u"_su8, TT::BareInstr, O::F64ConvertI32U, 0},
      {"f64.convert_i64_s"_su8, TT::BareInstr, O::F64ConvertI64S, 0},
      {"f64.convert_i64_u"_su8, TT::BareInstr, O::F64ConvertI64U, 0},
      {"f64.copysign"_su8, TT::BareInstr, O::F64Copysign, 0},
      {"f64.div"_su8, TT::BareInstr, O::F64Div, 0},
      {"f64.eq"_su8, TT::BareInstr, O::F64Eq, 0},
      {"f64.floor"_su8, TT::BareInstr, O::F64Floor, 0},
      {"f64.ge"_su8, TT::BareInstr, O::F64Ge, 0},
      {"f64.gt"_su8, TT::BareInstr, O::F64Gt, 0},
      {"f64.le"_su8, TT::BareInstr, O::F64Le, 0},
      {"f64.load"_su8, TT::MemoryInstr, O::F64Load, 0},
      {"f64.lt"_su8, TT::BareInstr, O::F64Lt, 0},
      {"f64.max"_su8, TT::BareInstr, O::F64Max, 0},
      {"f64.min"_su8, TT::BareInstr, O::F64Min, 0},
      {"f64.mul"_su8, TT::BareInstr, O::F64Mul, 0},
      {"f64.nearest"_su8, TT::BareInstr, O::F64Nearest, 0},
      {"f64.neg"_su8, TT::BareInstr, O::F64Neg, 0},
      {"f64.ne"_su8, TT::BareInstr, O::F64Ne, 0},
      {"f64.promote_f32"_su8, TT::BareInstr, O::F64PromoteF32, 0},
      {"f64.reinterpret_i64"_su8, TT::BareInstr, O::F64ReinterpretI64, 0},
      {"f64.sqrt"_su8, TT::BareInstr, O::F64Sqrt, 0},
      {"f64.store"_su8, TT::MemoryInstr, O::F64Store, 0},
      {"f64.sub"_su8, TT::BareInstr, O::F64Sub, 0},
      {"f64.trunc"_su8, TT::BareInstr, O::F64Trunc, 0},
      {"f64x2.abs"_su8, TT::BareInstr, O::F64X2Abs, F::Simd},
      {"f64x2.add"_su8, TT::BareInstr, O::F64X2Add, F::Simd},
      {"f64x2.div"_su8, TT::BareInstr, O::F64X2Div, F::Simd},
      {"f64x2.eq"_su8, TT::BareInstr, O::F64X2Eq, F::Simd},
      {"f64x2.extract_lane"_su8, TT::SimdLaneInstr, O::F64X2ExtractLane, F::Simd},
      {"f64x2.ge"_su8, TT::BareInstr, O::F64X2Ge, F::Simd},
      {"f64x2.gt"_su8, TT::BareInstr, O::F64X2Gt, F::Simd},
      {"f64x2.le"_su8, TT::BareInstr, O::F64X2Le, F::Simd},
      {"f64x2.lt"_su8, TT::BareInstr, O::F64X2Lt, F::Simd},
      {"f64x2.max"_su8, TT::BareInstr, O::F64X2Max, F::Simd},
      {"f64x2.min"_su8, TT::BareInstr, O::F64X2Min, F::Simd},
      {"f64x2.mul"_su8, TT::BareInstr, O::F64X2Mul, F::Simd},
      {"f64x2.neg"_su8, TT::BareInstr, O::F64X2Neg, F::Simd},
      {"f64x2.ne"_su8, TT::BareInstr, O::F64X2Ne, F::Simd},
      {"f64x2.replace_lane"_su8, TT::SimdLaneInstr, O::F64X2ReplaceLane, F::Simd},
      {"f64x2.splat"_su8, TT::BareInstr, O::F64X2Splat, F::Simd},
      {"f64x2.sqrt"_su8, TT::BareInstr, O::F64X2Sqrt, F::Simd},
      {"f64x2.sub"_su8, TT::BareInstr, O::F64X2Sub, F::Simd},
      {"global.get"_su8, TT::VarInstr, O::GlobalGet, 0},
      {"global.set"_su8, TT::VarInstr, O::GlobalSet, 0},
      {"i16x8.add_saturate_s"_su8, TT::BareInstr, O::I16X8AddSaturateS, F::Simd},
      {"i16x8.add_saturate_u"_su8, TT::BareInstr, O::I16X8AddSaturateU, F::Simd},
      {"i16x8.add"_su8, TT::BareInstr, O::I16X8Add, F::Simd},
      {"i16x8.all_true"_su8, TT::BareInstr, O::I16X8AllTrue, F::Simd},
      {"i16x8.any_true"_su8, TT::BareInstr, O::I16X8AnyTrue, F::Simd},
      {"i16x8.avgr_u"_su8, TT::BareInstr, O::I16X8AvgrU, F::Simd},
      {"i16x8.eq"_su8, TT::BareInstr, O::I16X8Eq, F::Simd},
      {"i16x8.extract_lane_s"_su8, TT::SimdLaneInstr, O::I16X8ExtractLaneS, F::Simd},
      {"i16x8.extract_lane_u"_su8, TT::SimdLaneInstr, O::I16X8ExtractLaneU, F::Simd},
      {"i16x8.ge_s"_su8, TT::BareInstr, O::I16X8GeS, F::Simd},
      {"i16x8.ge_u"_su8, TT::BareInstr, O::I16X8GeU, F::Simd},
      {"i16x8.gt_s"_su8, TT::BareInstr, O::I16X8GtS, F::Simd},
      {"i16x8.gt_u"_su8, TT::BareInstr, O::I16X8GtU, F::Simd},
      {"i16x8.le_s"_su8, TT::BareInstr, O::I16X8LeS, F::Simd},
      {"i16x8.le_u"_su8, TT::BareInstr, O::I16X8LeU, F::Simd},
      {"i16x8.load8x8_s"_su8, TT::MemoryInstr, O::I16X8Load8X8S, F::Simd},
      {"i16x8.load8x8_u"_su8, TT::MemoryInstr, O::I16X8Load8X8U, F::Simd},
      {"i16x8.lt_s"_su8, TT::BareInstr, O::I16X8LtS, F::Simd},
      {"i16x8.lt_u"_su8, TT::BareInstr, O::I16X8LtU, F::Simd},
      {"i16x8.max_s"_su8, TT::BareInstr, O::I16X8MaxS, F::Simd},
      {"i16x8.max_u"_su8, TT::BareInstr, O::I16X8MaxU, F::Simd},
      {"i16x8.min_s"_su8, TT::BareInstr, O::I16X8MinS, F::Simd},
      {"i16x8.min_u"_su8, TT::BareInstr, O::I16X8MinU, F::Simd},
      {"i16x8.mul"_su8, TT::BareInstr, O::I16X8Mul, F::Simd},
      {"i16x8.narrow_i32x4_s"_su8, TT::BareInstr, O::I16X8NarrowI32X4S, F::Simd},
      {"i16x8.narrow_i32x4_u"_su8, TT::BareInstr, O::I16X8NarrowI32X4U, F::Simd},
      {"i16x8.neg"_su8, TT::BareInstr, O::I16X8Neg, F::Simd},
      {"i16x8.ne"_su8, TT::BareInstr, O::I16X8Ne, F::Simd},
      {"i16x8.replace_lane"_su8, TT::SimdLaneInstr, O::I16X8ReplaceLane, F::Simd},
      {"i16x8.shl"_su8, TT::BareInstr, O::I16X8Shl, F::Simd},
      {"i16x8.shr_s"_su8, TT::BareInstr, O::I16X8ShrS, F::Simd},
      {"i16x8.shr_u"_su8, TT::BareInstr, O::I16X8ShrU, F::Simd},
      {"i16x8.splat"_su8, TT::BareInstr, O::I16X8Splat, F::Simd},
      {"i16x8.sub_saturate_s"_su8, TT::BareInstr, O::I16X8SubSaturateS, F::Simd},
      {"i16x8.sub_saturate_u"_su8, TT::BareInstr, O::I16X8SubSaturateU, F::Simd},
      {"i16x8.sub"_su8, TT::BareInstr, O::I16X8Sub, F::Simd},
      {"i16x8.widen_high_i8x16_s"_su8, TT::BareInstr, O::I16X8WidenHighI8X16S, F::Simd},
      {"i16x8.widen_high_i8x16_u"_su8, TT::BareInstr, O::I16X8WidenHighI8X16U, F::Simd},
      {"i16x8.widen_low_i8x16_s"_su8, TT::BareInstr, O::I16X8WidenLowI8X16S, F::Simd},
      {"i16x8.widen_low_i8x16_u"_su8, TT::BareInstr, O::I16X8WidenLowI8X16U, F::Simd},
      {"i32.add"_su8, TT::BareInstr, O::I32Add, 0},
      {"i32.and"_su8, TT::BareInstr, O::I32And, 0},
      {"i32.atomic.load16_u"_su8, TT::MemoryInstr, O::I32AtomicLoad16U, F::Threads},
      {"i32.atomic.load8_u"_su8, TT::MemoryInstr, O::I32AtomicLoad8U, F::Threads},
      {"i32.atomic.load"_su8, TT::MemoryInstr, O::I32AtomicLoad, F::Threads},
      {"i32.atomic.rmw16.add_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16AddU, F::Threads},
      {"i32.atomic.rmw16.and_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16AndU, F::Threads},
      {"i32.atomic.rmw16.cmpxchg_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16CmpxchgU, F::Threads},
      {"i32.atomic.rmw16.or_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16OrU, F::Threads},
      {"i32.atomic.rmw16.sub_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16SubU, F::Threads},
      {"i32.atomic.rmw16.xchg_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16XchgU, F::Threads},
      {"i32.atomic.rmw16.xor_u"_su8, TT::MemoryInstr, O::I32AtomicRmw16XorU, F::Threads},
      {"i32.atomic.rmw8.add_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8AddU, F::Threads},
      {"i32.atomic.rmw8.and_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8AndU, F::Threads},
      {"i32.atomic.rmw8.cmpxchg_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8CmpxchgU, F::Threads},
      {"i32.atomic.rmw8.or_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8OrU, F::Threads},
      {"i32.atomic.rmw8.sub_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8SubU, F::Threads},
      {"i32.atomic.rmw8.xchg_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8XchgU, F::Threads},
      {"i32.atomic.rmw8.xor_u"_su8, TT::MemoryInstr, O::I32AtomicRmw8XorU, F::Threads},
      {"i32.atomic.rmw.add"_su8, TT::MemoryInstr, O::I32AtomicRmwAdd, F::Threads},
      {"i32.atomic.rmw.and"_su8, TT::MemoryInstr, O::I32AtomicRmwAnd, F::Threads},
      {"i32.atomic.rmw.cmpxchg"_su8, TT::MemoryInstr, O::I32AtomicRmwCmpxchg, F::Threads},
      {"i32.atomic.rmw.or"_su8, TT::MemoryInstr, O::I32AtomicRmwOr, F::Threads},
      {"i32.atomic.rmw.sub"_su8, TT::MemoryInstr, O::I32AtomicRmwSub, F::Threads},
      {"i32.atomic.rmw.xchg"_su8, TT::MemoryInstr, O::I32AtomicRmwXchg, F::Threads},
      {"i32.atomic.rmw.xor"_su8, TT::MemoryInstr, O::I32AtomicRmwXor, F::Threads},
      {"i32.atomic.store16"_su8, TT::MemoryInstr, O::I32AtomicStore16, F::Threads},
      {"i32.atomic.store8"_su8, TT::MemoryInstr, O::I32AtomicStore8, F::Threads},
      {"i32.atomic.store"_su8, TT::MemoryInstr, O::I32AtomicStore, F::Threads},
      {"i32.clz"_su8, TT::BareInstr, O::I32Clz, 0},
      {"i32.const"_su8, TT::I32ConstInstr, O::I32Const, 0},
      {"i32.ctz"_su8, TT::BareInstr, O::I32Ctz, 0},
      {"i32.div_s"_su8, TT::BareInstr, O::I32DivS, 0},
      {"i32.div_u"_su8, TT::BareInstr, O::I32DivU, 0},
      {"i32.eq"_su8, TT::BareInstr, O::I32Eq, 0},
      {"i32.eqz"_su8, TT::BareInstr, O::I32Eqz, 0},
      {"i32.extend16_s"_su8, TT::BareInstr, O::I32Extend16S, F::SignExtension},
      {"i32.extend8_s"_su8, TT::BareInstr, O::I32Extend8S, F::SignExtension},
      {"i32.ge_s"_su8, TT::BareInstr, O::I32GeS, 0},
      {"i32.ge_u"_su8, TT::BareInstr, O::I32GeU, 0},
      {"i32.gt_s"_su8, TT::BareInstr, O::I32GtS, 0},
      {"i32.gt_u"_su8, TT::BareInstr, O::I32GtU, 0},
      {"i32.le_s"_su8, TT::BareInstr, O::I32LeS, 0},
      {"i32.le_u"_su8, TT::BareInstr, O::I32LeU, 0},
      {"i32.load16_s"_su8, TT::MemoryInstr, O::I32Load16S, 0},
      {"i32.load16_u"_su8, TT::MemoryInstr, O::I32Load16U, 0},
      {"i32.load8_s"_su8, TT::MemoryInstr, O::I32Load8S, 0},
      {"i32.load8_u"_su8, TT::MemoryInstr, O::I32Load8U, 0},
      {"i32.load"_su8, TT::MemoryInstr, O::I32Load, 0},
      {"i32.lt_s"_su8, TT::BareInstr, O::I32LtS, 0},
      {"i32.lt_u"_su8, TT::BareInstr, O::I32LtU, 0},
      {"i32.mul"_su8, TT::BareInstr, O::I32Mul, 0},
      {"i32.ne"_su8, TT::BareInstr, O::I32Ne, 0},
      {"i32.or"_su8, TT::BareInstr, O::I32Or, 0},
      {"i32.popcnt"_su8, TT::BareInstr, O::I32Popcnt, 0},
      {"i32.reinterpret_f32"_su8, TT::BareInstr, O::I32ReinterpretF32, 0},
      {"i32.rem_s"_su8, TT::BareInstr, O::I32RemS, 0},
      {"i32.rem_u"_su8, TT::BareInstr, O::I32RemU, 0},
      {"i32.rotl"_su8, TT::BareInstr, O::I32Rotl, 0},
      {"i32.rotr"_su8, TT::BareInstr, O::I32Rotr, 0},
      {"i32.shl"_su8, TT::BareInstr, O::I32Shl, 0},
      {"i32.shr_s"_su8, TT::BareInstr, O::I32ShrS, 0},
      {"i32.shr_u"_su8, TT::BareInstr, O::I32ShrU, 0},
      {"i32.store16"_su8, TT::MemoryInstr, O::I32Store16, 0},
      {"i32.store8"_su8, TT::MemoryInstr, O::I32Store8, 0},
      {"i32.store"_su8, TT::MemoryInstr, O::I32Store, 0},
      {"i32.sub"_su8, TT::BareInstr, O::I32Sub, 0},
      {"i32.trunc_f32_s"_su8, TT::BareInstr, O::I32TruncF32S, 0},
      {"i32.trunc_f32_u"_su8, TT::BareInstr, O::I32TruncF32U, 0},
      {"i32.trunc_f64_s"_su8, TT::BareInstr, O::I32TruncF64S, 0},
      {"i32.trunc_f64_u"_su8, TT::BareInstr, O::I32TruncF64U, 0},
      {"i32.trunc_sat_f32_s"_su8, TT::BareInstr, O::I32TruncSatF32S, F::SaturatingFloatToInt},
      {"i32.trunc_sat_f32_u"_su8, TT::BareInstr, O::I32TruncSatF32U, F::SaturatingFloatToInt},
      {"i32.trunc_sat_f64_s"_su8, TT::BareInstr, O::I32TruncSatF64S, F::SaturatingFloatToInt},
      {"i32.trunc_sat_f64_u"_su8, TT::BareInstr, O::I32TruncSatF64U, F::SaturatingFloatToInt},
      {"i32.wrap_i64"_su8, TT::BareInstr, O::I32WrapI64, 0},
      {"i32x4.add"_su8, TT::BareInstr, O::I32X4Add, F::Simd},
      {"i32x4.all_true"_su8, TT::BareInstr, O::I32X4AllTrue, F::Simd},
      {"i32x4.any_true"_su8, TT::BareInstr, O::I32X4AnyTrue, F::Simd},
      {"i32x4.eq"_su8, TT::BareInstr, O::I32X4Eq, F::Simd},
      {"i32x4.extract_lane"_su8, TT::SimdLaneInstr, O::I32X4ExtractLane, F::Simd},
      {"i32x4.ge_s"_su8, TT::BareInstr, O::I32X4GeS, F::Simd},
      {"i32x4.ge_u"_su8, TT::BareInstr, O::I32X4GeU, F::Simd},
      {"i32x4.gt_s"_su8, TT::BareInstr, O::I32X4GtS, F::Simd},
      {"i32x4.gt_u"_su8, TT::BareInstr, O::I32X4GtU, F::Simd},
      {"i32x4.le_s"_su8, TT::BareInstr, O::I32X4LeS, F::Simd},
      {"i32x4.le_u"_su8, TT::BareInstr, O::I32X4LeU, F::Simd},
      {"i32x4.load16x4_s"_su8, TT::MemoryInstr, O::I32X4Load16X4S, F::Simd},
      {"i32x4.load16x4_u"_su8, TT::MemoryInstr, O::I32X4Load16X4U, F::Simd},
      {"i32x4.lt_s"_su8, TT::BareInstr, O::I32X4LtS, F::Simd},
      {"i32x4.lt_u"_su8, TT::BareInstr, O::I32X4LtU, F::Simd},
      {"i32x4.max_s"_su8, TT::BareInstr, O::I32X4MaxS, F::Simd},
      {"i32x4.max_u"_su8, TT::BareInstr, O::I32X4MaxU, F::Simd},
      {"i32x4.min_s"_su8, TT::BareInstr, O::I32X4MinS, F::Simd},
      {"i32x4.min_u"_su8, TT::BareInstr, O::I32X4MinU, F::Simd},
      {"i32x4.mul"_su8, TT::BareInstr, O::I32X4Mul, F::Simd},
      {"i32x4.neg"_su8, TT::BareInstr, O::I32X4Neg, F::Simd},
      {"i32x4.ne"_su8, TT::BareInstr, O::I32X4Ne, F::Simd},
      {"i32x4.replace_lane"_su8, TT::SimdLaneInstr, O::I32X4ReplaceLane, F::Simd},
      {"i32x4.shl"_su8, TT::BareInstr, O::I32X4Shl, F::Simd},
      {"i32x4.shr_s"_su8, TT::BareInstr, O::I32X4ShrS, F::Simd},
      {"i32x4.shr_u"_su8, TT::BareInstr, O::I32X4ShrU, F::Simd},
      {"i32x4.splat"_su8, TT::BareInstr, O::I32X4Splat, F::Simd},
      {"i32x4.sub"_su8, TT::BareInstr, O::I32X4Sub, F::Simd},
      {"i32x4.trunc_sat_f32x4_s"_su8, TT::BareInstr, O::I32X4TruncSatF32X4S, F::Simd},
      {"i32x4.trunc_sat_f32x4_u"_su8, TT::BareInstr, O::I32X4TruncSatF32X4U, F::Simd},
      {"i32x4.widen_high_i16x8_s"_su8, TT::BareInstr, O::I32X4WidenHighI16X8S, F::Simd},
      {"i32x4.widen_high_i16x8_u"_su8, TT::BareInstr, O::I32X4WidenHighI16X8U, F::Simd},
      {"i32x4.widen_low_i16x8_s"_su8, TT::BareInstr, O::I32X4WidenLowI16X8S, F::Simd},
      {"i32x4.widen_low_i16x8_u"_su8, TT::BareInstr, O::I32X4WidenLowI16X8U, F::Simd},
      {"i32.xor"_su8, TT::BareInstr, O::I32Xor, 0},
      {"i64.add"_su8, TT::BareInstr, O::I64Add, 0},
      {"i64.and"_su8, TT::BareInstr, O::I64And, 0},
      {"i64.atomic.load16_u"_su8, TT::MemoryInstr, O::I64AtomicLoad16U, F::Threads},
      {"i64.atomic.load32_u"_su8, TT::MemoryInstr, O::I64AtomicLoad32U, F::Threads},
      {"i64.atomic.load8_u"_su8, TT::MemoryInstr, O::I64AtomicLoad8U, F::Threads},
      {"i64.atomic.load"_su8, TT::MemoryInstr, O::I64AtomicLoad, F::Threads},
      {"i64.atomic.rmw16.add_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16AddU, F::Threads},
      {"i64.atomic.rmw16.and_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16AndU, F::Threads},
      {"i64.atomic.rmw16.cmpxchg_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16CmpxchgU, F::Threads},
      {"i64.atomic.rmw16.or_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16OrU, F::Threads},
      {"i64.atomic.rmw16.sub_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16SubU, F::Threads},
      {"i64.atomic.rmw16.xchg_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16XchgU, F::Threads},
      {"i64.atomic.rmw16.xor_u"_su8, TT::MemoryInstr, O::I64AtomicRmw16XorU, F::Threads},
      {"i64.atomic.rmw32.add_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32AddU, F::Threads},
      {"i64.atomic.rmw32.and_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32AndU, F::Threads},
      {"i64.atomic.rmw32.cmpxchg_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32CmpxchgU, F::Threads},
      {"i64.atomic.rmw32.or_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32OrU, F::Threads},
      {"i64.atomic.rmw32.sub_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32SubU, F::Threads},
      {"i64.atomic.rmw32.xchg_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32XchgU, F::Threads},
      {"i64.atomic.rmw32.xor_u"_su8, TT::MemoryInstr, O::I64AtomicRmw32XorU, F::Threads},
      {"i64.atomic.rmw8.add_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8AddU, F::Threads},
      {"i64.atomic.rmw8.and_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8AndU, F::Threads},
      {"i64.atomic.rmw8.cmpxchg_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8CmpxchgU, F::Threads},
      {"i64.atomic.rmw8.or_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8OrU, F::Threads},
      {"i64.atomic.rmw8.sub_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8SubU, F::Threads},
      {"i64.atomic.rmw8.xchg_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8XchgU, F::Threads},
      {"i64.atomic.rmw8.xor_u"_su8, TT::MemoryInstr, O::I64AtomicRmw8XorU, F::Threads},
      {"i64.atomic.rmw.add"_su8, TT::MemoryInstr, O::I64AtomicRmwAdd, F::Threads},
      {"i64.atomic.rmw.and"_su8, TT::MemoryInstr, O::I64AtomicRmwAnd, F::Threads},
      {"i64.atomic.rmw.cmpxchg"_su8, TT::MemoryInstr, O::I64AtomicRmwCmpxchg, F::Threads},
      {"i64.atomic.rmw.or"_su8, TT::MemoryInstr, O::I64AtomicRmwOr, F::Threads},
      {"i64.atomic.rmw.sub"_su8, TT::MemoryInstr, O::I64AtomicRmwSub, F::Threads},
      {"i64.atomic.rmw.xchg"_su8, TT::MemoryInstr, O::I64AtomicRmwXchg, F::Threads},
      {"i64.atomic.rmw.xor"_su8, TT::MemoryInstr, O::I64AtomicRmwXor, F::Threads},
      {"i64.atomic.store16"_su8, TT::MemoryInstr, O::I64AtomicStore16, F::Threads},
      {"i64.atomic.store32"_su8, TT::MemoryInstr, O::I64AtomicStore32, F::Threads},
      {"i64.atomic.store8"_su8, TT::MemoryInstr, O::I64AtomicStore8, F::Threads},
      {"i64.atomic.store"_su8, TT::MemoryInstr, O::I64AtomicStore, F::Threads},
      {"i64.clz"_su8, TT::BareInstr, O::I64Clz, 0},
      {"i64.const"_su8, TT::I64ConstInstr, O::I64Const, 0},
      {"i64.ctz"_su8, TT::BareInstr, O::I64Ctz, 0},
      {"i64.div_s"_su8, TT::BareInstr, O::I64DivS, 0},
      {"i64.div_u"_su8, TT::BareInstr, O::I64DivU, 0},
      {"i64.eq"_su8, TT::BareInstr, O::I64Eq, 0},
      {"i64.eqz"_su8, TT::BareInstr, O::I64Eqz, 0},
      {"i64.extend16_s"_su8, TT::BareInstr, O::I64Extend16S, F::SignExtension},
      {"i64.extend32_s"_su8, TT::BareInstr, O::I64Extend32S, F::SignExtension},
      {"i64.extend8_s"_su8, TT::BareInstr, O::I64Extend8S, F::SignExtension},
      {"i64.extend_i32_s"_su8, TT::BareInstr, O::I64ExtendI32S, 0},
      {"i64.extend_i32_u"_su8, TT::BareInstr, O::I64ExtendI32U, 0},
      {"i64.ge_s"_su8, TT::BareInstr, O::I64GeS, 0},
      {"i64.ge_u"_su8, TT::BareInstr, O::I64GeU, 0},
      {"i64.gt_s"_su8, TT::BareInstr, O::I64GtS, 0},
      {"i64.gt_u"_su8, TT::BareInstr, O::I64GtU, 0},
      {"i64.le_s"_su8, TT::BareInstr, O::I64LeS, 0},
      {"i64.le_u"_su8, TT::BareInstr, O::I64LeU, 0},
      {"i64.load16_s"_su8, TT::MemoryInstr, O::I64Load16S, 0},
      {"i64.load16_u"_su8, TT::MemoryInstr, O::I64Load16U, 0},
      {"i64.load32_s"_su8, TT::MemoryInstr, O::I64Load32S, 0},
      {"i64.load32_u"_su8, TT::MemoryInstr, O::I64Load32U, 0},
      {"i64.load8_s"_su8, TT::MemoryInstr, O::I64Load8S, 0},
      {"i64.load8_u"_su8, TT::MemoryInstr, O::I64Load8U, 0},
      {"i64.load"_su8, TT::MemoryInstr, O::I64Load, 0},
      {"i64.lt_s"_su8, TT::BareInstr, O::I64LtS, 0},
      {"i64.lt_u"_su8, TT::BareInstr, O::I64LtU, 0},
      {"i64.mul"_su8, TT::BareInstr, O::I64Mul, 0},
      {"i64.ne"_su8, TT::BareInstr, O::I64Ne, 0},
      {"i64.or"_su8, TT::BareInstr, O::I64Or, 0},
      {"i64.popcnt"_su8, TT::BareInstr, O::I64Popcnt, 0},
      {"i64.reinterpret_f64"_su8, TT::BareInstr, O::I64ReinterpretF64, 0},
      {"i64.rem_s"_su8, TT::BareInstr, O::I64RemS, 0},
      {"i64.rem_u"_su8, TT::BareInstr, O::I64RemU, 0},
      {"i64.rotl"_su8, TT::BareInstr, O::I64Rotl, 0},
      {"i64.rotr"_su8, TT::BareInstr, O::I64Rotr, 0},
      {"i64.shl"_su8, TT::BareInstr, O::I64Shl, 0},
      {"i64.shr_s"_su8, TT::BareInstr, O::I64ShrS, 0},
      {"i64.shr_u"_su8, TT::BareInstr, O::I64ShrU, 0},
      {"i64.store16"_su8, TT::MemoryInstr, O::I64Store16, 0},
      {"i64.store32"_su8, TT::MemoryInstr, O::I64Store32, 0},
      {"i64.store8"_su8, TT::MemoryInstr, O::I64Store8, 0},
      {"i64.store"_su8, TT::MemoryInstr, O::I64Store, 0},
      {"i64.sub"_su8, TT::BareInstr, O::I64Sub, 0},
      {"i64.trunc_f32_s"_su8, TT::BareInstr, O::I64TruncF32S, 0},
      {"i64.trunc_f32_u"_su8, TT::BareInstr, O::I64TruncF32U, 0},
      {"i64.trunc_f64_s"_su8, TT::BareInstr, O::I64TruncF64S, 0},
      {"i64.trunc_f64_u"_su8, TT::BareInstr, O::I64TruncF64U, 0},
      {"i64.trunc_sat_f32_s"_su8, TT::BareInstr, O::I64TruncSatF32S, F::SaturatingFloatToInt},
      {"i64.trunc_sat_f32_u"_su8, TT::BareInstr, O::I64TruncSatF32U, F::SaturatingFloatToInt},
      {"i64.trunc_sat_f64_s"_su8, TT::BareInstr, O::I64TruncSatF64S, F::SaturatingFloatToInt},
      {"i64.trunc_sat_f64_u"_su8, TT::BareInstr, O::I64TruncSatF64U, F::SaturatingFloatToInt},
      {"i64x2.add"_su8, TT::BareInstr, O::I64X2Add, F::Simd},
      {"i64x2.extract_lane"_su8, TT::SimdLaneInstr, O::I64X2ExtractLane, F::Simd},
      {"i64x2.load32x2_s"_su8, TT::MemoryInstr, O::I64X2Load32X2S, F::Simd},
      {"i64x2.load32x2_u"_su8, TT::MemoryInstr, O::I64X2Load32X2U, F::Simd},
      {"i64x2.mul"_su8, TT::BareInstr, O::I64X2Mul, F::Simd},
      {"i64x2.neg"_su8, TT::BareInstr, O::I64X2Neg, F::Simd},
      {"i64x2.replace_lane"_su8, TT::SimdLaneInstr, O::I64X2ReplaceLane, F::Simd},
      {"i64x2.shl"_su8, TT::BareInstr, O::I64X2Shl, F::Simd},
      {"i64x2.shr_s"_su8, TT::BareInstr, O::I64X2ShrS, F::Simd},
      {"i64x2.shr_u"_su8, TT::BareInstr, O::I64X2ShrU, F::Simd},
      {"i64x2.splat"_su8, TT::BareInstr, O::I64X2Splat, F::Simd},
      {"i64x2.sub"_su8, TT::BareInstr, O::I64X2Sub, F::Simd},
      {"i64.xor"_su8, TT::BareInstr, O::I64Xor, 0},
      {"i8x16.add_saturate_s"_su8, TT::BareInstr, O::I8X16AddSaturateS, F::Simd},
      {"i8x16.add_saturate_u"_su8, TT::BareInstr, O::I8X16AddSaturateU, F::Simd},
      {"i8x16.add"_su8, TT::BareInstr, O::I8X16Add, F::Simd},
      {"i8x16.all_true"_su8, TT::BareInstr, O::I8X16AllTrue, F::Simd},
      {"i8x16.any_true"_su8, TT::BareInstr, O::I8X16AnyTrue, F::Simd},
      {"i8x16.avgr_u"_su8, TT::BareInstr, O::I8X16AvgrU, F::Simd},
      {"i8x16.eq"_su8, TT::BareInstr, O::I8X16Eq, F::Simd},
      {"i8x16.extract_lane_s"_su8, TT::SimdLaneInstr, O::I8X16ExtractLaneS, F::Simd},
      {"i8x16.extract_lane_u"_su8, TT::SimdLaneInstr, O::I8X16ExtractLaneU, F::Simd},
      {"i8x16.ge_s"_su8, TT::BareInstr, O::I8X16GeS, F::Simd},
      {"i8x16.ge_u"_su8, TT::BareInstr, O::I8X16GeU, F::Simd},
      {"i8x16.gt_s"_su8, TT::BareInstr, O::I8X16GtS, F::Simd},
      {"i8x16.gt_u"_su8, TT::BareInstr, O::I8X16GtU, F::Simd},
      {"i8x16.le_s"_su8, TT::BareInstr, O::I8X16LeS, F::Simd},
      {"i8x16.le_u"_su8, TT::BareInstr, O::I8X16LeU, F::Simd},
      {"i8x16.lt_s"_su8, TT::BareInstr, O::I8X16LtS, F::Simd},
      {"i8x16.lt_u"_su8, TT::BareInstr, O::I8X16LtU, F::Simd},
      {"i8x16.max_s"_su8, TT::BareInstr, O::I8X16MaxS, F::Simd},
      {"i8x16.max_u"_su8, TT::BareInstr, O::I8X16MaxU, F::Simd},
      {"i8x16.min_s"_su8, TT::BareInstr, O::I8X16MinS, F::Simd},
      {"i8x16.min_u"_su8, TT::BareInstr, O::I8X16MinU, F::Simd},
      {"i8x16.narrow_i16x8_s"_su8, TT::BareInstr, O::I8X16NarrowI16X8S, F::Simd},
      {"i8x16.narrow_i16x8_u"_su8, TT::BareInstr, O::I8X16NarrowI16X8U, F::Simd},
      {"i8x16.neg"_su8, TT::BareInstr, O::I8X16Neg, F::Simd},
      {"i8x16.ne"_su8, TT::BareInstr, O::I8X16Ne, F::Simd},
      {"i8x16.replace_lane"_su8, TT::SimdLaneInstr, O::I8X16ReplaceLane, F::Simd},
      {"i8x16.shl"_su8, TT::BareInstr, O::I8X16Shl, F::Simd},
      {"i8x16.shr_s"_su8, TT::BareInstr, O::I8X16ShrS, F::Simd},
      {"i8x16.shr_u"_su8, TT::BareInstr, O::I8X16ShrU, F::Simd},
      {"i8x16.splat"_su8, TT::BareInstr, O::I8X16Splat, F::Simd},
      {"i8x16.sub_saturate_s"_su8, TT::BareInstr, O::I8X16SubSaturateS, F::Simd},
      {"i8x16.sub_saturate_u"_su8, TT::BareInstr, O::I8X16SubSaturateU, F::Simd},
      {"i8x16.sub"_su8, TT::BareInstr, O::I8X16Sub, F::Simd},
      {"local.get"_su8, TT::VarInstr, O::LocalGet, 0},
      {"local.set"_su8, TT::VarInstr, O::LocalSet, 0},
      {"local.tee"_su8, TT::VarInstr, O::LocalTee, 0},
      {"memory.atomic.notify"_su8, TT::MemoryInstr, O::MemoryAtomicNotify, F::Threads},
      {"memory.atomic.wait32"_su8, TT::MemoryInstr, O::MemoryAtomicWait32, F::Threads},
      {"memory.atomic.wait64"_su8, TT::MemoryInstr, O::MemoryAtomicWait64, F::Threads},
      {"memory.copy"_su8, TT::MemoryCopyInstr, O::MemoryCopy, F::BulkMemory},
      {"memory.fill"_su8, TT::BareInstr, O::MemoryFill, F::BulkMemory},
      {"memory.grow"_su8, TT::BareInstr, O::MemoryGrow, 0},
      {"memory.init"_su8, TT::MemoryInitInstr, O::MemoryInit, F::BulkMemory},
      {"memory.size"_su8, TT::BareInstr, O::MemorySize, 0},
      {"nop"_su8, TT::BareInstr, O::Nop, 0},
      {"ref.func"_su8, TT::RefFuncInstr, O::RefFunc, F::ReferenceTypes},
      {"ref.is_null"_su8, TT::RefIsNullInstr, O::RefIsNull, F::ReferenceTypes},
      {"ref.null"_su8, TT::RefNullInstr, O::RefNull, F::ReferenceTypes},
      {"rethrow"_su8, TT::BareInstr, O::Rethrow, F::Exceptions},
      {"return_call_indirect"_su8, TT::CallIndirectInstr, O::ReturnCallIndirect, F::TailCall},
      {"return_call"_su8, TT::VarInstr, O::ReturnCall, F::TailCall},
      {"return"_su8, TT::BareInstr, O::Return, 0},
      {"select"_su8, TT::SelectInstr, O::Select, 0},
      {"table.copy"_su8, TT::TableCopyInstr, O::TableCopy, F::BulkMemory},
      {"table.fill"_su8, TT::VarInstr, O::TableFill, F::ReferenceTypes},
      {"table.get"_su8, TT::VarInstr, O::TableGet, F::ReferenceTypes},
      {"table.grow"_su8, TT::VarInstr, O::TableGrow, F::ReferenceTypes},
      {"table.init"_su8, TT::TableInitInstr, O::TableInit, F::BulkMemory},
      {"table.set"_su8, TT::VarInstr, O::TableSet, F::ReferenceTypes},
      {"table.size"_su8, TT::VarInstr, O::TableSize, F::ReferenceTypes},
      {"throw"_su8, TT::VarInstr, O::Throw, F::Exceptions},
      {"unreachable"_su8, TT::BareInstr, O::Unreachable, 0},
      {"v128.andnot"_su8, TT::BareInstr, O::V128Andnot, F::Simd},
      {"v128.and"_su8, TT::BareInstr, O::V128And, F::Simd},
      {"v128.bitselect"_su8, TT::BareInstr, O::V128BitSelect, F::Simd},
      {"v128.const"_su8, TT::SimdConstInstr, O::V128Const, F::Simd},
      {"v128.load"_su8, TT::MemoryInstr, O::V128Load, F::Simd},
      {"v128.not"_su8, TT::BareInstr, O::V128Not, F::Simd},
      {"v128.or"_su8, TT::BareInstr, O::V128Or, F::Simd},
      {"v128.store"_su8, TT::MemoryInstr, O::V128Store, F::Simd},
      {"v128.xor"_su8, TT::BareInstr, O::V128Xor, F::Simd},
      {"v16x8.load_splat"_su8, TT::MemoryInstr, O::V16X8LoadSplat, F::Simd},
      {"v32x4.load_splat"_su8, TT::MemoryInstr, O::V32X4LoadSplat, F::Simd},
      {"v64x2.load_splat"_su8, TT::MemoryInstr, O::V64X2LoadSplat, F::Simd},
      {"v8x16.load_splat"_su8, TT::MemoryInstr, O::V8X16LoadSplat, F::Simd},
      {"v8x16.shuffle"_su8, TT::SimdShuffleInstr, O::V8X16Shuffle, F::Simd},
      {"v8x16.swizzle"_su8, TT::BareInstr, O::V8X16Swizzle, F::Simd},
      {"current_memory"_su8, TT::BareInstr, O::MemorySize, 0},
      {"f32.convert_s/i32"_su8, TT::BareInstr, O::F32ConvertI32S, 0},
      {"f32.convert_s/i64"_su8, TT::BareInstr, O::F32ConvertI64S, 0},
      {"f32.convert_u/i32"_su8, TT::BareInstr, O::F32ConvertI32U, 0},
      {"f32.convert_u/i64"_su8, TT::BareInstr, O::F32ConvertI64U, 0},
      {"f32.demote/f64"_su8, TT::BareInstr, O::F32DemoteF64, 0},
      {"f32.reinterpret/i32"_su8, TT::BareInstr, O::F32ReinterpretI32, 0},
      {"f64.convert_s/i32"_su8, TT::BareInstr, O::F64ConvertI32S, 0},
      {"f64.convert_s/i64"_su8, TT::BareInstr, O::F64ConvertI64S, 0},
      {"f64.convert_u/i32"_su8, TT::BareInstr, O::F64ConvertI32U, 0},
      {"f64.convert_u/i64"_su8, TT::BareInstr, O::F64ConvertI64U, 0},
      {"f64.promote/f32"_su8, TT::BareInstr, O::F64PromoteF32, 0},
      {"f64.reinterpret/i64"_su8, TT::BareInstr, O::F64ReinterpretI64, 0},
      {"get_global"_su8, TT::VarInstr, O::GlobalGet, 0},
      {"get_local"_su8, TT::VarInstr, O::LocalGet, 0},
      {"grow_memory"_su8, TT::BareInstr, O::MemoryGrow, 0},
      {"i32.reinterpret/f32"_su8, TT::BareInstr, O::I32ReinterpretF32, 0},
      {"i32.trunc_s/f32"_su8, TT::BareInstr, O::I32TruncF32S, 0},
      {"i32.trunc_s/f64"_su8, TT::BareInstr, O::I32TruncF64S, 0},
      {"i32.trunc_s:sat/f32"_su8, TT::BareInstr, O::I32TruncSatF32S, F::SaturatingFloatToInt},
      {"i32.trunc_s:sat/f64"_su8, TT::BareInstr, O::I32TruncSatF64S, F::SaturatingFloatToInt},
      {"i32.trunc_u/f32"_su8, TT::BareInstr, O::I32TruncF32U, 0},
      {"i32.trunc_u/f64"_su8, TT::BareInstr, O::I32TruncF64U, 0},
      {"i32.trunc_u:sat/f32"_su8, TT::BareInstr, O::I32TruncSatF32U, F::SaturatingFloatToInt},
      {"i32.trunc_u:sat/f64"_su8, TT::BareInstr, O::I32TruncSatF64U, F::SaturatingFloatToInt},
      {"i32.wrap/i64"_su8, TT::BareInstr, O::I32WrapI64, 0},
      {"i64.extend_s/i32"_su8, TT::BareInstr, O::I64ExtendI32S, 0},
      {"i64.extend_u/i32"_su8, TT::BareInstr, O::I64ExtendI32U, 0},
      {"i64.reinterpret/f64"_su8, TT::BareInstr, O::I64ReinterpretF64, 0},
      {"i64.trunc_s/f32"_su8, TT::BareInstr, O::I64TruncF32S, 0},
      {"i64.trunc_s/f64"_su8, TT::BareInstr, O::I64TruncF64S, 0},
      {"i64.trunc_s:sat/f32"_su8, TT::BareInstr, O::I64TruncSatF32S, F::SaturatingFloatToInt},
      {"i64.trunc_s:sat/f64"_su8, TT::BareInstr, O::I64TruncSatF64S, F::SaturatingFloatToInt},
      {"i64.trunc_u/f32"_su8, TT::BareInstr, O::I64TruncF32U, 0},
      {"i64.trunc_u/f64"_su8, TT::BareInstr, O::I64TruncF64U, 0},
      {"i64.trunc_u:sat/f32"_su8, TT::BareInstr, O::I64TruncSatF32U, F::SaturatingFloatToInt},
      {"i64.trunc_u:sat/f64"_su8, TT::BareInstr, O::I64TruncSatF64U, F::SaturatingFloatToInt},
      {"set_global"_su8, TT::VarInstr, O::GlobalSet, 0},
      {"set_local"_su8, TT::VarInstr, O::LocalSet, 0},
      {"tee_local"_su8, TT::VarInstr, O::LocalTee, 0},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), test.type, test.opcode, test.features},
              test.span);
  }
}

TEST(LexTest, Float) {
  struct {
    SpanU8 span;
    LiteralInfo info;
  } tests[] = {
      {"3."_su8, LI::Number(Sign::None, HU::No)},
      {"3e5"_su8, LI::Number(Sign::None, HU::No)},
      {"3E5"_su8, LI::Number(Sign::None, HU::No)},
      {"3e+14"_su8, LI::Number(Sign::None, HU::No)},
      {"3E+14"_su8, LI::Number(Sign::None, HU::No)},
      {"3e-14"_su8, LI::Number(Sign::None, HU::No)},
      {"3E-14"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14e15"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14E15"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14e+15"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14E+15"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14e-15"_su8, LI::Number(Sign::None, HU::No)},
      {"3.14E-15"_su8, LI::Number(Sign::None, HU::No)},
      {"+3."_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3e5"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3E5"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3e+14"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3E+14"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3e-14"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3E-14"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14e15"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14E15"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14e+15"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14E+15"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14e-15"_su8, LI::Number(Sign::Plus, HU::No)},
      {"+3.14E-15"_su8, LI::Number(Sign::Plus, HU::No)},
      {"-3."_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3e5"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3E5"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3e+14"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3E+14"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3e-14"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3E-14"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14e15"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14E15"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14e+15"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14E+15"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14e-15"_su8, LI::Number(Sign::Minus, HU::No)},
      {"-3.14E-15"_su8, LI::Number(Sign::Minus, HU::No)},

      {"0x3."_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3p5"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3P5"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3p+14"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3P+14"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3p-14"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3P-14"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1a"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1ap15"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1aP15"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1ap+15"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1aP+15"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1ap-15"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"0x3.1aP-15"_su8, LI::HexNumber(Sign::None, HU::No)},
      {"+0x3."_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3p5"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3P5"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3p+14"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3P+14"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3p-14"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3P-14"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1a"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1ap15"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1aP15"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1ap+15"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1aP+15"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1ap-15"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"+0x3.1aP-15"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"-0x3."_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3p5"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3P5"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3p+14"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3P+14"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3p-14"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3P-14"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1a"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1ap15"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1aP15"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1ap+15"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1aP+15"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1ap-15"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"-0x3.1aP-15"_su8, LI::HexNumber(Sign::Minus, HU::No)},

      {"inf"_su8, LI::Infinity(Sign::None)},
      {"+inf"_su8, LI::Infinity(Sign::Plus)},
      {"-inf"_su8, LI::Infinity(Sign::Minus)},

      {"nan"_su8, LI::Nan(Sign::None)},
      {"+nan"_su8, LI::Nan(Sign::Plus)},
      {"-nan"_su8, LI::Nan(Sign::Minus)},

      {"nan:0x1"_su8, LI::NanPayload(Sign::None, HU::No)},
      {"nan:0x123"_su8, LI::NanPayload(Sign::None, HU::No)},
      {"nan:0x123abc"_su8, LI::NanPayload(Sign::None, HU::No)},
      {"+nan:0x1"_su8, LI::NanPayload(Sign::Plus, HU::No)},
      {"+nan:0x123"_su8, LI::NanPayload(Sign::Plus, HU::No)},
      {"+nan:0x123abc"_su8, LI::NanPayload(Sign::Plus, HU::No)},
      {"-nan:0x1"_su8, LI::NanPayload(Sign::Minus, HU::No)},
      {"-nan:0x123"_su8, LI::NanPayload(Sign::Minus, HU::No)},
      {"-nan:0x123abc"_su8, LI::NanPayload(Sign::Minus, HU::No)},

      // A single underscore is allowed between any two digits.
      {"3_1.4_1"_su8, LI::Number(Sign::None, HU::Yes)},
      {"-3_1.4_1e5_9"_su8, LI::Number(Sign::Minus, HU::Yes)},
      {"+0xab_c.c_dep+0_1"_su8, LI::HexNumber(Sign::Plus, HU::Yes)},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::Float, test.info}, test.span);
  }
}

TEST(LexTest, Id) {
  ExpectLex({4, TokenType::Id}, "$abc"_su8);
  ExpectLex({12, TokenType::Id}, "$123'456_789"_su8);
  ExpectLex({4, TokenType::Id}, "$<p>"_su8);
}

TEST(LexTest, Int) {
  struct {
    SpanU8 span;
    LI kind;
  } tests[] = {
      {"-0"_su8, LI::Number(Sign::Minus, HU::No)},
      {"+0"_su8, LI::Number(Sign::Plus, HU::No)},
      {"-123"_su8, LI::Number(Sign::Minus, HU::No)},
      {"+123"_su8, LI::Number(Sign::Plus, HU::No)},

      {"-0x123"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"+0x123"_su8, LI::HexNumber(Sign::Plus, HU::No)},
      {"-0x123abcdef"_su8, LI::HexNumber(Sign::Minus, HU::No)},
      {"+0x123abcdef"_su8, LI::HexNumber(Sign::Plus, HU::No)},

      // A single underscore is allowed between any two digits.
      {"-0_0"_su8, LI::Number(Sign::Minus, HU::Yes)},
      {"+0_0"_su8, LI::Number(Sign::Plus, HU::Yes)},
      {"-12_3"_su8, LI::Number(Sign::Minus, HU::Yes)},
      {"+1_23"_su8, LI::Number(Sign::Plus, HU::Yes)},
      {"-12_34_56"_su8, LI::Number(Sign::Minus, HU::Yes)},
      {"+123_456"_su8, LI::Number(Sign::Plus, HU::Yes)},
      {"-0x12_3"_su8, LI::HexNumber(Sign::Minus, HU::Yes)},
      {"+0x1_23"_su8, LI::HexNumber(Sign::Plus, HU::Yes)},
      {"-0x12_3ab_cde_f"_su8, LI::HexNumber(Sign::Minus, HU::Yes)},
      {"+0x123_a_b_cde_f"_su8, LI::HexNumber(Sign::Plus, HU::Yes)},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::Int, test.kind}, test.span);
  }
}

TEST(LexTest, Nat) {
  struct {
    SpanU8 span;
    LI kind;
  } tests[] = {
      {"0"_su8, LI::Nat(HU::No)},
      {"123"_su8, LI::Nat(HU::No)},

      {"0x123"_su8, LI::HexNat(HU::No)},
      {"0x123abcdef"_su8, LI::HexNat(HU::No)},

      // A single underscore is allowed between any two digits.
      {"0_0"_su8, LI::Nat(HU::Yes)},
      {"123_456"_su8, LI::Nat(HU::Yes)},
      {"0x1_23_456"_su8, LI::HexNat(HU::Yes)},
      {"0x12_3a_bcd_ef"_su8, LI::HexNat(HU::Yes)},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::Nat, test.kind}, test.span);
  }
}

TEST(LexTest, Text) {
  struct {
    SpanU8 span;
    u32 byte_size;
  } tests[] = {
      {R"("")"_su8, 0},
      {R"("hello, world")"_su8, 12},
      {R"("\t\n\r\'\"")"_su8, 5},
      {R"("\00\01\02\03\04\05\06\07\08\09\0a\0b\0c\0d\0e\0f")"_su8, 16},
      {R"("\10\11\12\13\14\15\16\17\18\19\1a\1b\1c\1d\1e\1f")"_su8, 16},
      {R"("\20\21\22\23\24\25\26\27\28\29\2a\2b\2c\2d\2e\2f")"_su8, 16},
      {R"("\30\31\32\33\34\35\36\37\38\39\3a\3b\3c\3d\3e\3f")"_su8, 16},
      {R"("\40\41\42\43\44\45\46\47\48\49\4a\4b\4c\4d\4e\4f")"_su8, 16},
      {R"("\50\51\52\53\54\55\56\57\58\59\5a\5b\5c\5d\5e\5f")"_su8, 16},
      {R"("\60\61\62\63\64\65\66\67\68\69\6a\6b\6c\6d\6e\6f")"_su8, 16},
      {R"("\70\71\72\73\74\75\76\77\78\79\7a\7b\7c\7d\7e\7f")"_su8, 16},
      {R"("\80\81\82\83\84\85\86\87\88\89\8a\8b\8c\8d\8e\8f")"_su8, 16},
      {R"("\90\91\92\93\94\95\96\97\98\99\9a\9b\9c\9d\9e\9f")"_su8, 16},
      {R"("\a0\a1\a2\a3\a4\a5\a6\a7\a8\a9\aa\ab\ac\ad\ae\af")"_su8, 16},
      {R"("\b0\b1\b2\b3\b4\b5\b6\b7\b8\b9\ba\bb\bc\bd\be\bf")"_su8, 16},
      {R"("\c0\c1\c2\c3\c4\c5\c6\c7\c8\c9\ca\cb\cc\cd\ce\cf")"_su8, 16},
      {R"("\d0\d1\d2\d3\d4\d5\d6\d7\d8\d9\da\db\dc\dd\de\df")"_su8, 16},
      {R"("\e0\e1\e2\e3\e4\e5\e6\e7\e8\e9\ea\eb\ec\ed\ee\ef")"_su8, 16},
      {R"("\f0\f1\f2\f3\f4\f5\f6\f7\f8\f9\fa\fb\fc\fd\fe\ff")"_su8, 16},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::Text,
               Text{ToStringView(test.span), test.byte_size}},
              test.span);
  }
}

TEST(LexTest, ValueType) {
  struct {
    SpanU8 span;
    ValueType value_type;
  } tests[] = {
      {"anyfunc"_su8, ValueType::Funcref},
      {"externref"_su8, ValueType::Externref},
      {"exnref"_su8, ValueType::Exnref},
      {"f32"_su8, ValueType::F32},
      {"f64"_su8, ValueType::F64},
      {"funcref"_su8, ValueType::Funcref},
      {"i32"_su8, ValueType::I32},
      {"i64"_su8, ValueType::I64},
      {"v128"_su8, ValueType::V128},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::ValueType, test.value_type},
              test.span);
  }
}

TEST(LexTest, ReferenceKind) {
  struct {
    SpanU8 span;
    TokenType token_type;
    ReferenceType reftype;
  } tests[] = {
      {"extern"_su8, TokenType::Extern, ReferenceType::Externref},
      {"exn"_su8, TokenType::Exn, ReferenceType::Exnref},
      {"func"_su8, TokenType::Func, ReferenceType::Funcref},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), test.token_type, test.reftype}, test.span);
  }
}

TEST(LexTest, Basic) {
  auto span =
R"((module
  (func (export "add") (param i32 i32) (result i32)
    (i32.add (local.get 0) (local.get 1)))))"_su8;

  std::vector<ExpectedToken> expected_tokens = {
      {1, TokenType::Lpar},
      {6, TokenType::Module},
      {3, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {4, TokenType::Func, ReferenceType::Funcref},
      {1, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {6, TokenType::Export},
      {1, TokenType::Whitespace},
      {5, TokenType::Text, Text{"\"add\""_sv, 3}},
      {1, TokenType::Rpar},
      {1, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {5, TokenType::Param},
      {1, TokenType::Whitespace},
      {3, TokenType::ValueType, ValueType::I32},
      {1, TokenType::Whitespace},
      {3, TokenType::ValueType, ValueType::I32},
      {1, TokenType::Rpar},
      {1, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {6, TokenType::Result},
      {1, TokenType::Whitespace},
      {3, TokenType::ValueType, ValueType::I32},
      {1, TokenType::Rpar},
      {5, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {7, TokenType::BareInstr, Opcode::I32Add},
      {1, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {9, TokenType::VarInstr, Opcode::LocalGet},
      {1, TokenType::Whitespace},
      {1, TokenType::Nat, LI::Nat(HU::No)},
      {1, TokenType::Rpar},
      {1, TokenType::Whitespace},
      {1, TokenType::Lpar},
      {9, TokenType::VarInstr, Opcode::LocalGet},
      {1, TokenType::Whitespace},
      {1, TokenType::Nat, LI::Nat(HU::No)},
      {1, TokenType::Rpar},
      {1, TokenType::Rpar},
      {1, TokenType::Rpar},
      {1, TokenType::Rpar},
      {0, TokenType::Eof},
  };

  for (auto&& expected : expected_tokens) {
    span = ExpectLex(expected, span);
  }
}

TEST(LexTest, LexNoWhitespace) {
  auto span =
R"((  module (; a comment ;) (  func  ) ) ))"_su8;

  struct {
    ExpectedToken token;
    size_t gap;
  } expected_tokens[] = {
      {{1, TokenType::Lpar}, 2},
      {{6, TokenType::Module}, 17},
      {{1, TokenType::Lpar}, 2},
      {{4, TokenType::Func, ReferenceType::Funcref}, 2},
      {{1, TokenType::Rpar}, 1},
      {{1, TokenType::Rpar}, 1},
      {{1, TokenType::Rpar}, 0},
      {{0, TokenType::Eof}, 0},
  };

  for (auto&& pair : expected_tokens) {
    span = ExpectLex(pair.token, span);
    remove_prefix(&span, pair.gap);
  }
}

TEST(LexTest, Tokenizer) {
  auto span = "(module (func (param i32)))"_su8;
  Tokenizer t{span};

  std::vector<Token> tokens = {
      {span.subspan(0, 1), TokenType::Lpar},
      {span.subspan(1, 6), TokenType::Module},
      {span.subspan(8, 1), TokenType::Lpar},
      {span.subspan(9, 4), TokenType::Func, ReferenceType::Funcref},
      {span.subspan(14, 1), TokenType::Lpar},
      {span.subspan(15, 5), TokenType::Param},
      {span.subspan(21, 3), TokenType::ValueType, ValueType::I32},
      {span.subspan(24, 1), TokenType::Rpar},
      {span.subspan(25, 1), TokenType::Rpar},
      {span.subspan(26, 1), TokenType::Rpar},
      {span.subspan(27, 0), TokenType::Eof},
      {span.subspan(27, 0), TokenType::Eof},
  };

  EXPECT_EQ(0, t.count());

  for (size_t i = 0; i < tokens.size(); i += 2) {
    EXPECT_EQ(tokens[i], t.Peek());
    EXPECT_EQ(1, t.count());
    EXPECT_EQ(tokens[i + 1], t.Peek(1));
    EXPECT_EQ(2, t.count());
    EXPECT_EQ(tokens[i], t.Read());
    EXPECT_EQ(1, t.count());
    EXPECT_EQ(tokens[i + 1], t.Read());
    EXPECT_EQ(0, t.count());
  }
}
