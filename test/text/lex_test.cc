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

#include "wasp/text/lex.h"

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"

using namespace ::wasp;
using namespace ::wasp::text;

using LI = LiteralInfo;
using HU = HasUnderscores;

struct ExpectedToken {
  ExpectedToken(Location::index_type size, TokenType type)
      : size{size}, type{type} {}
  ExpectedToken(Location::index_type size, TokenType type, LiteralInfo info)
      : size{size}, type{type}, immediate{info} {}
  ExpectedToken(Location::index_type size, TokenType type, Opcode opcode)
      : size{size}, type{type}, immediate{opcode} {}
  ExpectedToken(Location::index_type size, TokenType type, ValueType value_type)
      : size{size}, type{type}, immediate{value_type} {}
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
      {"func"_su8, TT::Func},
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
      {"ref.host"_su8, TT::RefHost},
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
  } tests[] = {
      {"block"_su8, Opcode::Block},
      {"if"_su8, Opcode::If},
      {"loop"_su8, Opcode::Loop},
      {"try"_su8, Opcode::Try},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::BlockInstr, test.opcode},
              test.span);
  }
}

TEST(LexTest, PlainInstr) {
  struct {
    SpanU8 span;
    TokenType type;
    Opcode opcode;
  } tests[] = {
      {"atomic.notify"_su8, TokenType::MemoryInstr, Opcode::AtomicNotify},
      {"br_if"_su8, TokenType::VarInstr, Opcode::BrIf},
      {"br_on_exn"_su8, TokenType::BrOnExnInstr, Opcode::BrOnExn},
      {"br_table"_su8, TokenType::BrTableInstr, Opcode::BrTable},
      {"br"_su8, TokenType::VarInstr, Opcode::Br},
      {"call_indirect"_su8, TokenType::CallIndirectInstr, Opcode::CallIndirect},
      {"call"_su8, TokenType::VarInstr, Opcode::Call},
      {"data.drop"_su8, TokenType::VarInstr, Opcode::DataDrop},
      {"drop"_su8, TokenType::BareInstr, Opcode::Drop},
      {"elem.drop"_su8, TokenType::VarInstr, Opcode::ElemDrop},
      {"f32.abs"_su8, TokenType::BareInstr, Opcode::F32Abs},
      {"f32.add"_su8, TokenType::BareInstr, Opcode::F32Add},
      {"f32.ceil"_su8, TokenType::BareInstr, Opcode::F32Ceil},
      {"f32.const"_su8, TokenType::F32ConstInstr, Opcode::F32Const},
      {"f32.convert_i32_s"_su8, TokenType::BareInstr, Opcode::F32ConvertI32S},
      {"f32.convert_i32_u"_su8, TokenType::BareInstr, Opcode::F32ConvertI32U},
      {"f32.convert_i64_s"_su8, TokenType::BareInstr, Opcode::F32ConvertI64S},
      {"f32.convert_i64_u"_su8, TokenType::BareInstr, Opcode::F32ConvertI64U},
      {"f32.copysign"_su8, TokenType::BareInstr, Opcode::F32Copysign},
      {"f32.demote_f64"_su8, TokenType::BareInstr, Opcode::F32DemoteF64},
      {"f32.div"_su8, TokenType::BareInstr, Opcode::F32Div},
      {"f32.eq"_su8, TokenType::BareInstr, Opcode::F32Eq},
      {"f32.floor"_su8, TokenType::BareInstr, Opcode::F32Floor},
      {"f32.ge"_su8, TokenType::BareInstr, Opcode::F32Ge},
      {"f32.gt"_su8, TokenType::BareInstr, Opcode::F32Gt},
      {"f32.le"_su8, TokenType::BareInstr, Opcode::F32Le},
      {"f32.load"_su8, TokenType::MemoryInstr, Opcode::F32Load},
      {"f32.lt"_su8, TokenType::BareInstr, Opcode::F32Lt},
      {"f32.max"_su8, TokenType::BareInstr, Opcode::F32Max},
      {"f32.min"_su8, TokenType::BareInstr, Opcode::F32Min},
      {"f32.mul"_su8, TokenType::BareInstr, Opcode::F32Mul},
      {"f32.nearest"_su8, TokenType::BareInstr, Opcode::F32Nearest},
      {"f32.neg"_su8, TokenType::BareInstr, Opcode::F32Neg},
      {"f32.ne"_su8, TokenType::BareInstr, Opcode::F32Ne},
      {"f32.reinterpret_i32"_su8, TokenType::BareInstr, Opcode::F32ReinterpretI32},
      {"f32.sqrt"_su8, TokenType::BareInstr, Opcode::F32Sqrt},
      {"f32.store"_su8, TokenType::MemoryInstr, Opcode::F32Store},
      {"f32.sub"_su8, TokenType::BareInstr, Opcode::F32Sub},
      {"f32.trunc"_su8, TokenType::BareInstr, Opcode::F32Trunc},
      {"f32x4.abs"_su8, TokenType::BareInstr, Opcode::F32X4Abs},
      {"f32x4.add"_su8, TokenType::BareInstr, Opcode::F32X4Add},
      {"f32x4.convert_i32x4_s"_su8, TokenType::BareInstr, Opcode::F32X4ConvertI32X4S},
      {"f32x4.convert_i32x4_u"_su8, TokenType::BareInstr, Opcode::F32X4ConvertI32X4U},
      {"f32x4.div"_su8, TokenType::BareInstr, Opcode::F32X4Div},
      {"f32x4.eq"_su8, TokenType::BareInstr, Opcode::F32X4Eq},
      {"f32x4.extract_lane"_su8, TokenType::SimdLaneInstr, Opcode::F32X4ExtractLane},
      {"f32x4.ge"_su8, TokenType::BareInstr, Opcode::F32X4Ge},
      {"f32x4.gt"_su8, TokenType::BareInstr, Opcode::F32X4Gt},
      {"f32x4.le"_su8, TokenType::BareInstr, Opcode::F32X4Le},
      {"f32x4.lt"_su8, TokenType::BareInstr, Opcode::F32X4Lt},
      {"f32x4.max"_su8, TokenType::BareInstr, Opcode::F32X4Max},
      {"f32x4.min"_su8, TokenType::BareInstr, Opcode::F32X4Min},
      {"f32x4.mul"_su8, TokenType::BareInstr, Opcode::F32X4Mul},
      {"f32x4.neg"_su8, TokenType::BareInstr, Opcode::F32X4Neg},
      {"f32x4.ne"_su8, TokenType::BareInstr, Opcode::F32X4Ne},
      {"f32x4.replace_lane"_su8, TokenType::SimdLaneInstr, Opcode::F32X4ReplaceLane},
      {"f32x4.splat"_su8, TokenType::BareInstr, Opcode::F32X4Splat},
      {"f32x4.sqrt"_su8, TokenType::BareInstr, Opcode::F32X4Sqrt},
      {"f32x4.sub"_su8, TokenType::BareInstr, Opcode::F32X4Sub},
      {"f64.abs"_su8, TokenType::BareInstr, Opcode::F64Abs},
      {"f64.add"_su8, TokenType::BareInstr, Opcode::F64Add},
      {"f64.ceil"_su8, TokenType::BareInstr, Opcode::F64Ceil},
      {"f64.const"_su8, TokenType::F64ConstInstr, Opcode::F64Const},
      {"f64.convert_i32_s"_su8, TokenType::BareInstr, Opcode::F64ConvertI32S},
      {"f64.convert_i32_u"_su8, TokenType::BareInstr, Opcode::F64ConvertI32U},
      {"f64.convert_i64_s"_su8, TokenType::BareInstr, Opcode::F64ConvertI64S},
      {"f64.convert_i64_u"_su8, TokenType::BareInstr, Opcode::F64ConvertI64U},
      {"f64.copysign"_su8, TokenType::BareInstr, Opcode::F64Copysign},
      {"f64.div"_su8, TokenType::BareInstr, Opcode::F64Div},
      {"f64.eq"_su8, TokenType::BareInstr, Opcode::F64Eq},
      {"f64.floor"_su8, TokenType::BareInstr, Opcode::F64Floor},
      {"f64.ge"_su8, TokenType::BareInstr, Opcode::F64Ge},
      {"f64.gt"_su8, TokenType::BareInstr, Opcode::F64Gt},
      {"f64.le"_su8, TokenType::BareInstr, Opcode::F64Le},
      {"f64.load"_su8, TokenType::MemoryInstr, Opcode::F64Load},
      {"f64.lt"_su8, TokenType::BareInstr, Opcode::F64Lt},
      {"f64.max"_su8, TokenType::BareInstr, Opcode::F64Max},
      {"f64.min"_su8, TokenType::BareInstr, Opcode::F64Min},
      {"f64.mul"_su8, TokenType::BareInstr, Opcode::F64Mul},
      {"f64.nearest"_su8, TokenType::BareInstr, Opcode::F64Nearest},
      {"f64.neg"_su8, TokenType::BareInstr, Opcode::F64Neg},
      {"f64.ne"_su8, TokenType::BareInstr, Opcode::F64Ne},
      {"f64.promote_f32"_su8, TokenType::BareInstr, Opcode::F64PromoteF32},
      {"f64.reinterpret_i64"_su8, TokenType::BareInstr, Opcode::F64ReinterpretI64},
      {"f64.sqrt"_su8, TokenType::BareInstr, Opcode::F64Sqrt},
      {"f64.store"_su8, TokenType::MemoryInstr, Opcode::F64Store},
      {"f64.sub"_su8, TokenType::BareInstr, Opcode::F64Sub},
      {"f64.trunc"_su8, TokenType::BareInstr, Opcode::F64Trunc},
      {"f64x2.abs"_su8, TokenType::BareInstr, Opcode::F64X2Abs},
      {"f64x2.add"_su8, TokenType::BareInstr, Opcode::F64X2Add},
      {"f64x2.div"_su8, TokenType::BareInstr, Opcode::F64X2Div},
      {"f64x2.eq"_su8, TokenType::BareInstr, Opcode::F64X2Eq},
      {"f64x2.extract_lane"_su8, TokenType::SimdLaneInstr, Opcode::F64X2ExtractLane},
      {"f64x2.ge"_su8, TokenType::BareInstr, Opcode::F64X2Ge},
      {"f64x2.gt"_su8, TokenType::BareInstr, Opcode::F64X2Gt},
      {"f64x2.le"_su8, TokenType::BareInstr, Opcode::F64X2Le},
      {"f64x2.lt"_su8, TokenType::BareInstr, Opcode::F64X2Lt},
      {"f64x2.max"_su8, TokenType::BareInstr, Opcode::F64X2Max},
      {"f64x2.min"_su8, TokenType::BareInstr, Opcode::F64X2Min},
      {"f64x2.mul"_su8, TokenType::BareInstr, Opcode::F64X2Mul},
      {"f64x2.neg"_su8, TokenType::BareInstr, Opcode::F64X2Neg},
      {"f64x2.ne"_su8, TokenType::BareInstr, Opcode::F64X2Ne},
      {"f64x2.replace_lane"_su8, TokenType::SimdLaneInstr, Opcode::F64X2ReplaceLane},
      {"f64x2.splat"_su8, TokenType::BareInstr, Opcode::F64X2Splat},
      {"f64x2.sqrt"_su8, TokenType::BareInstr, Opcode::F64X2Sqrt},
      {"f64x2.sub"_su8, TokenType::BareInstr, Opcode::F64X2Sub},
      {"global.get"_su8, TokenType::VarInstr, Opcode::GlobalGet},
      {"global.set"_su8, TokenType::VarInstr, Opcode::GlobalSet},
      {"i16x8.add_saturate_s"_su8, TokenType::BareInstr, Opcode::I16X8AddSaturateS},
      {"i16x8.add_saturate_u"_su8, TokenType::BareInstr, Opcode::I16X8AddSaturateU},
      {"i16x8.add"_su8, TokenType::BareInstr, Opcode::I16X8Add},
      {"i16x8.all_true"_su8, TokenType::BareInstr, Opcode::I16X8AllTrue},
      {"i16x8.any_true"_su8, TokenType::BareInstr, Opcode::I16X8AnyTrue},
      {"i16x8.avgr_u"_su8, TokenType::BareInstr, Opcode::I16X8AvgrU},
      {"i16x8.eq"_su8, TokenType::BareInstr, Opcode::I16X8Eq},
      {"i16x8.extract_lane_s"_su8, TokenType::SimdLaneInstr, Opcode::I16X8ExtractLaneS},
      {"i16x8.extract_lane_u"_su8, TokenType::SimdLaneInstr, Opcode::I16X8ExtractLaneU},
      {"i16x8.ge_s"_su8, TokenType::BareInstr, Opcode::I16X8GeS},
      {"i16x8.ge_u"_su8, TokenType::BareInstr, Opcode::I16X8GeU},
      {"i16x8.gt_s"_su8, TokenType::BareInstr, Opcode::I16X8GtS},
      {"i16x8.gt_u"_su8, TokenType::BareInstr, Opcode::I16X8GtU},
      {"i16x8.le_s"_su8, TokenType::BareInstr, Opcode::I16X8LeS},
      {"i16x8.le_u"_su8, TokenType::BareInstr, Opcode::I16X8LeU},
      {"i16x8.load8x8_s"_su8, TokenType::MemoryInstr, Opcode::I16X8Load8X8S},
      {"i16x8.load8x8_u"_su8, TokenType::MemoryInstr, Opcode::I16X8Load8X8U},
      {"i16x8.lt_s"_su8, TokenType::BareInstr, Opcode::I16X8LtS},
      {"i16x8.lt_u"_su8, TokenType::BareInstr, Opcode::I16X8LtU},
      {"i16x8.max_s"_su8, TokenType::BareInstr, Opcode::I16X8MaxS},
      {"i16x8.max_u"_su8, TokenType::BareInstr, Opcode::I16X8MaxU},
      {"i16x8.min_s"_su8, TokenType::BareInstr, Opcode::I16X8MinS},
      {"i16x8.min_u"_su8, TokenType::BareInstr, Opcode::I16X8MinU},
      {"i16x8.mul"_su8, TokenType::BareInstr, Opcode::I16X8Mul},
      {"i16x8.narrow_i32x4_s"_su8, TokenType::BareInstr, Opcode::I16X8NarrowI32X4S},
      {"i16x8.narrow_i32x4_u"_su8, TokenType::BareInstr, Opcode::I16X8NarrowI32X4U},
      {"i16x8.neg"_su8, TokenType::BareInstr, Opcode::I16X8Neg},
      {"i16x8.ne"_su8, TokenType::BareInstr, Opcode::I16X8Ne},
      {"i16x8.replace_lane"_su8, TokenType::SimdLaneInstr, Opcode::I16X8ReplaceLane},
      {"i16x8.shl"_su8, TokenType::BareInstr, Opcode::I16X8Shl},
      {"i16x8.shr_s"_su8, TokenType::BareInstr, Opcode::I16X8ShrS},
      {"i16x8.shr_u"_su8, TokenType::BareInstr, Opcode::I16X8ShrU},
      {"i16x8.splat"_su8, TokenType::BareInstr, Opcode::I16X8Splat},
      {"i16x8.sub_saturate_s"_su8, TokenType::BareInstr, Opcode::I16X8SubSaturateS},
      {"i16x8.sub_saturate_u"_su8, TokenType::BareInstr, Opcode::I16X8SubSaturateU},
      {"i16x8.sub"_su8, TokenType::BareInstr, Opcode::I16X8Sub},
      {"i16x8.widen_high_i8x16_s"_su8, TokenType::BareInstr, Opcode::I16X8WidenHighI8X16S},
      {"i16x8.widen_high_i8x16_u"_su8, TokenType::BareInstr, Opcode::I16X8WidenHighI8X16U},
      {"i16x8.widen_low_i8x16_s"_su8, TokenType::BareInstr, Opcode::I16X8WidenLowI8X16S},
      {"i16x8.widen_low_i8x16_u"_su8, TokenType::BareInstr, Opcode::I16X8WidenLowI8X16U},
      {"i32.add"_su8, TokenType::BareInstr, Opcode::I32Add},
      {"i32.and"_su8, TokenType::BareInstr, Opcode::I32And},
      {"i32.atomic.load16_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicLoad16U},
      {"i32.atomic.load8_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicLoad8U},
      {"i32.atomic.load"_su8, TokenType::MemoryInstr, Opcode::I32AtomicLoad},
      {"i32.atomic.rmw16.add_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16AddU},
      {"i32.atomic.rmw16.and_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16AndU},
      {"i32.atomic.rmw16.cmpxchg_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16CmpxchgU},
      {"i32.atomic.rmw16.or_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16OrU},
      {"i32.atomic.rmw16.sub_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16SubU},
      {"i32.atomic.rmw16.xchg_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16XchgU},
      {"i32.atomic.rmw16.xor_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw16XorU},
      {"i32.atomic.rmw8.add_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8AddU},
      {"i32.atomic.rmw8.and_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8AndU},
      {"i32.atomic.rmw8.cmpxchg_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8CmpxchgU},
      {"i32.atomic.rmw8.or_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8OrU},
      {"i32.atomic.rmw8.sub_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8SubU},
      {"i32.atomic.rmw8.xchg_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8XchgU},
      {"i32.atomic.rmw8.xor_u"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmw8XorU},
      {"i32.atomic.rmw.add"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwAdd},
      {"i32.atomic.rmw.and"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwAnd},
      {"i32.atomic.rmw.cmpxchg"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwCmpxchg},
      {"i32.atomic.rmw.or"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwOr},
      {"i32.atomic.rmw.sub"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwSub},
      {"i32.atomic.rmw.xchg"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwXchg},
      {"i32.atomic.rmw.xor"_su8, TokenType::MemoryInstr, Opcode::I32AtomicRmwXor},
      {"i32.atomic.store16"_su8, TokenType::MemoryInstr, Opcode::I32AtomicStore16},
      {"i32.atomic.store8"_su8, TokenType::MemoryInstr, Opcode::I32AtomicStore8},
      {"i32.atomic.store"_su8, TokenType::MemoryInstr, Opcode::I32AtomicStore},
      {"i32.atomic.wait"_su8, TokenType::MemoryInstr, Opcode::I32AtomicWait},
      {"i32.clz"_su8, TokenType::BareInstr, Opcode::I32Clz},
      {"i32.const"_su8, TokenType::I32ConstInstr, Opcode::I32Const},
      {"i32.ctz"_su8, TokenType::BareInstr, Opcode::I32Ctz},
      {"i32.div_s"_su8, TokenType::BareInstr, Opcode::I32DivS},
      {"i32.div_u"_su8, TokenType::BareInstr, Opcode::I32DivU},
      {"i32.eq"_su8, TokenType::BareInstr, Opcode::I32Eq},
      {"i32.eqz"_su8, TokenType::BareInstr, Opcode::I32Eqz},
      {"i32.extend16_s"_su8, TokenType::BareInstr, Opcode::I32Extend16S},
      {"i32.extend8_s"_su8, TokenType::BareInstr, Opcode::I32Extend8S},
      {"i32.ge_s"_su8, TokenType::BareInstr, Opcode::I32GeS},
      {"i32.ge_u"_su8, TokenType::BareInstr, Opcode::I32GeU},
      {"i32.gt_s"_su8, TokenType::BareInstr, Opcode::I32GtS},
      {"i32.gt_u"_su8, TokenType::BareInstr, Opcode::I32GtU},
      {"i32.le_s"_su8, TokenType::BareInstr, Opcode::I32LeS},
      {"i32.le_u"_su8, TokenType::BareInstr, Opcode::I32LeU},
      {"i32.load16_s"_su8, TokenType::MemoryInstr, Opcode::I32Load16S},
      {"i32.load16_u"_su8, TokenType::MemoryInstr, Opcode::I32Load16U},
      {"i32.load8_s"_su8, TokenType::MemoryInstr, Opcode::I32Load8S},
      {"i32.load8_u"_su8, TokenType::MemoryInstr, Opcode::I32Load8U},
      {"i32.load"_su8, TokenType::MemoryInstr, Opcode::I32Load},
      {"i32.lt_s"_su8, TokenType::BareInstr, Opcode::I32LtS},
      {"i32.lt_u"_su8, TokenType::BareInstr, Opcode::I32LtU},
      {"i32.mul"_su8, TokenType::BareInstr, Opcode::I32Mul},
      {"i32.ne"_su8, TokenType::BareInstr, Opcode::I32Ne},
      {"i32.or"_su8, TokenType::BareInstr, Opcode::I32Or},
      {"i32.popcnt"_su8, TokenType::BareInstr, Opcode::I32Popcnt},
      {"i32.reinterpret_f32"_su8, TokenType::BareInstr, Opcode::I32ReinterpretF32},
      {"i32.rem_s"_su8, TokenType::BareInstr, Opcode::I32RemS},
      {"i32.rem_u"_su8, TokenType::BareInstr, Opcode::I32RemU},
      {"i32.rotl"_su8, TokenType::BareInstr, Opcode::I32Rotl},
      {"i32.rotr"_su8, TokenType::BareInstr, Opcode::I32Rotr},
      {"i32.shl"_su8, TokenType::BareInstr, Opcode::I32Shl},
      {"i32.shr_s"_su8, TokenType::BareInstr, Opcode::I32ShrS},
      {"i32.shr_u"_su8, TokenType::BareInstr, Opcode::I32ShrU},
      {"i32.store16"_su8, TokenType::MemoryInstr, Opcode::I32Store16},
      {"i32.store8"_su8, TokenType::MemoryInstr, Opcode::I32Store8},
      {"i32.store"_su8, TokenType::MemoryInstr, Opcode::I32Store},
      {"i32.sub"_su8, TokenType::BareInstr, Opcode::I32Sub},
      {"i32.trunc_f32_s"_su8, TokenType::BareInstr, Opcode::I32TruncF32S},
      {"i32.trunc_f32_u"_su8, TokenType::BareInstr, Opcode::I32TruncF32U},
      {"i32.trunc_f64_s"_su8, TokenType::BareInstr, Opcode::I32TruncF64S},
      {"i32.trunc_f64_u"_su8, TokenType::BareInstr, Opcode::I32TruncF64U},
      {"i32.trunc_sat_f32_s"_su8, TokenType::BareInstr, Opcode::I32TruncSatF32S},
      {"i32.trunc_sat_f32_u"_su8, TokenType::BareInstr, Opcode::I32TruncSatF32U},
      {"i32.trunc_sat_f64_s"_su8, TokenType::BareInstr, Opcode::I32TruncSatF64S},
      {"i32.trunc_sat_f64_u"_su8, TokenType::BareInstr, Opcode::I32TruncSatF64U},
      {"i32.wrap_i64"_su8, TokenType::BareInstr, Opcode::I32WrapI64},
      {"i32x4.add"_su8, TokenType::BareInstr, Opcode::I32X4Add},
      {"i32x4.all_true"_su8, TokenType::BareInstr, Opcode::I32X4AllTrue},
      {"i32x4.any_true"_su8, TokenType::BareInstr, Opcode::I32X4AnyTrue},
      {"i32x4.eq"_su8, TokenType::BareInstr, Opcode::I32X4Eq},
      {"i32x4.extract_lane"_su8, TokenType::SimdLaneInstr, Opcode::I32X4ExtractLane},
      {"i32x4.ge_s"_su8, TokenType::BareInstr, Opcode::I32X4GeS},
      {"i32x4.ge_u"_su8, TokenType::BareInstr, Opcode::I32X4GeU},
      {"i32x4.gt_s"_su8, TokenType::BareInstr, Opcode::I32X4GtS},
      {"i32x4.gt_u"_su8, TokenType::BareInstr, Opcode::I32X4GtU},
      {"i32x4.le_s"_su8, TokenType::BareInstr, Opcode::I32X4LeS},
      {"i32x4.le_u"_su8, TokenType::BareInstr, Opcode::I32X4LeU},
      {"i32x4.load16x4_s"_su8, TokenType::MemoryInstr, Opcode::I32X4Load16X4S},
      {"i32x4.load16x4_u"_su8, TokenType::MemoryInstr, Opcode::I32X4Load16X4U},
      {"i32x4.lt_s"_su8, TokenType::BareInstr, Opcode::I32X4LtS},
      {"i32x4.lt_u"_su8, TokenType::BareInstr, Opcode::I32X4LtU},
      {"i32x4.max_s"_su8, TokenType::BareInstr, Opcode::I32X4MaxS},
      {"i32x4.max_u"_su8, TokenType::BareInstr, Opcode::I32X4MaxU},
      {"i32x4.min_s"_su8, TokenType::BareInstr, Opcode::I32X4MinS},
      {"i32x4.min_u"_su8, TokenType::BareInstr, Opcode::I32X4MinU},
      {"i32x4.mul"_su8, TokenType::BareInstr, Opcode::I32X4Mul},
      {"i32x4.neg"_su8, TokenType::BareInstr, Opcode::I32X4Neg},
      {"i32x4.ne"_su8, TokenType::BareInstr, Opcode::I32X4Ne},
      {"i32x4.replace_lane"_su8, TokenType::SimdLaneInstr, Opcode::I32X4ReplaceLane},
      {"i32x4.shl"_su8, TokenType::BareInstr, Opcode::I32X4Shl},
      {"i32x4.shr_s"_su8, TokenType::BareInstr, Opcode::I32X4ShrS},
      {"i32x4.shr_u"_su8, TokenType::BareInstr, Opcode::I32X4ShrU},
      {"i32x4.splat"_su8, TokenType::BareInstr, Opcode::I32X4Splat},
      {"i32x4.sub"_su8, TokenType::BareInstr, Opcode::I32X4Sub},
      {"i32x4.trunc_sat_f32x4_s"_su8, TokenType::BareInstr, Opcode::I32X4TruncSatF32X4S},
      {"i32x4.trunc_sat_f32x4_u"_su8, TokenType::BareInstr, Opcode::I32X4TruncSatF32X4U},
      {"i32x4.widen_high_i16x8_s"_su8, TokenType::BareInstr, Opcode::I32X4WidenHighI16X8S},
      {"i32x4.widen_high_i16x8_u"_su8, TokenType::BareInstr, Opcode::I32X4WidenHighI16X8U},
      {"i32x4.widen_low_i16x8_s"_su8, TokenType::BareInstr, Opcode::I32X4WidenLowI16X8S},
      {"i32x4.widen_low_i16x8_u"_su8, TokenType::BareInstr, Opcode::I32X4WidenLowI16X8U},
      {"i32.xor"_su8, TokenType::BareInstr, Opcode::I32Xor},
      {"i64.add"_su8, TokenType::BareInstr, Opcode::I64Add},
      {"i64.and"_su8, TokenType::BareInstr, Opcode::I64And},
      {"i64.atomic.load16_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicLoad16U},
      {"i64.atomic.load32_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicLoad32U},
      {"i64.atomic.load8_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicLoad8U},
      {"i64.atomic.load"_su8, TokenType::MemoryInstr, Opcode::I64AtomicLoad},
      {"i64.atomic.rmw16.add_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16AddU},
      {"i64.atomic.rmw16.and_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16AndU},
      {"i64.atomic.rmw16.cmpxchg_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16CmpxchgU},
      {"i64.atomic.rmw16.or_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16OrU},
      {"i64.atomic.rmw16.sub_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16SubU},
      {"i64.atomic.rmw16.xchg_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16XchgU},
      {"i64.atomic.rmw16.xor_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw16XorU},
      {"i64.atomic.rmw32.add_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32AddU},
      {"i64.atomic.rmw32.and_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32AndU},
      {"i64.atomic.rmw32.cmpxchg_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32CmpxchgU},
      {"i64.atomic.rmw32.or_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32OrU},
      {"i64.atomic.rmw32.sub_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32SubU},
      {"i64.atomic.rmw32.xchg_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32XchgU},
      {"i64.atomic.rmw32.xor_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw32XorU},
      {"i64.atomic.rmw8.add_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8AddU},
      {"i64.atomic.rmw8.and_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8AndU},
      {"i64.atomic.rmw8.cmpxchg_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8CmpxchgU},
      {"i64.atomic.rmw8.or_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8OrU},
      {"i64.atomic.rmw8.sub_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8SubU},
      {"i64.atomic.rmw8.xchg_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8XchgU},
      {"i64.atomic.rmw8.xor_u"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmw8XorU},
      {"i64.atomic.rmw.add"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwAdd},
      {"i64.atomic.rmw.and"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwAnd},
      {"i64.atomic.rmw.cmpxchg"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwCmpxchg},
      {"i64.atomic.rmw.or"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwOr},
      {"i64.atomic.rmw.sub"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwSub},
      {"i64.atomic.rmw.xchg"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwXchg},
      {"i64.atomic.rmw.xor"_su8, TokenType::MemoryInstr, Opcode::I64AtomicRmwXor},
      {"i64.atomic.store16"_su8, TokenType::MemoryInstr, Opcode::I64AtomicStore16},
      {"i64.atomic.store32"_su8, TokenType::MemoryInstr, Opcode::I64AtomicStore32},
      {"i64.atomic.store8"_su8, TokenType::MemoryInstr, Opcode::I64AtomicStore8},
      {"i64.atomic.store"_su8, TokenType::MemoryInstr, Opcode::I64AtomicStore},
      {"i64.atomic.wait"_su8, TokenType::MemoryInstr, Opcode::I64AtomicWait},
      {"i64.clz"_su8, TokenType::BareInstr, Opcode::I64Clz},
      {"i64.const"_su8, TokenType::I64ConstInstr, Opcode::I64Const},
      {"i64.ctz"_su8, TokenType::BareInstr, Opcode::I64Ctz},
      {"i64.div_s"_su8, TokenType::BareInstr, Opcode::I64DivS},
      {"i64.div_u"_su8, TokenType::BareInstr, Opcode::I64DivU},
      {"i64.eq"_su8, TokenType::BareInstr, Opcode::I64Eq},
      {"i64.eqz"_su8, TokenType::BareInstr, Opcode::I64Eqz},
      {"i64.extend16_s"_su8, TokenType::BareInstr, Opcode::I64Extend16S},
      {"i64.extend32_s"_su8, TokenType::BareInstr, Opcode::I64Extend32S},
      {"i64.extend8_s"_su8, TokenType::BareInstr, Opcode::I64Extend8S},
      {"i64.extend_i32_s"_su8, TokenType::BareInstr, Opcode::I64ExtendI32S},
      {"i64.extend_i32_u"_su8, TokenType::BareInstr, Opcode::I64ExtendI32U},
      {"i64.ge_s"_su8, TokenType::BareInstr, Opcode::I64GeS},
      {"i64.ge_u"_su8, TokenType::BareInstr, Opcode::I64GeU},
      {"i64.gt_s"_su8, TokenType::BareInstr, Opcode::I64GtS},
      {"i64.gt_u"_su8, TokenType::BareInstr, Opcode::I64GtU},
      {"i64.le_s"_su8, TokenType::BareInstr, Opcode::I64LeS},
      {"i64.le_u"_su8, TokenType::BareInstr, Opcode::I64LeU},
      {"i64.load16_s"_su8, TokenType::MemoryInstr, Opcode::I64Load16S},
      {"i64.load16_u"_su8, TokenType::MemoryInstr, Opcode::I64Load16U},
      {"i64.load32_s"_su8, TokenType::MemoryInstr, Opcode::I64Load32S},
      {"i64.load32_u"_su8, TokenType::MemoryInstr, Opcode::I64Load32U},
      {"i64.load8_s"_su8, TokenType::MemoryInstr, Opcode::I64Load8S},
      {"i64.load8_u"_su8, TokenType::MemoryInstr, Opcode::I64Load8U},
      {"i64.load"_su8, TokenType::MemoryInstr, Opcode::I64Load},
      {"i64.lt_s"_su8, TokenType::BareInstr, Opcode::I64LtS},
      {"i64.lt_u"_su8, TokenType::BareInstr, Opcode::I64LtU},
      {"i64.mul"_su8, TokenType::BareInstr, Opcode::I64Mul},
      {"i64.ne"_su8, TokenType::BareInstr, Opcode::I64Ne},
      {"i64.or"_su8, TokenType::BareInstr, Opcode::I64Or},
      {"i64.popcnt"_su8, TokenType::BareInstr, Opcode::I64Popcnt},
      {"i64.reinterpret_f64"_su8, TokenType::BareInstr, Opcode::I64ReinterpretF64},
      {"i64.rem_s"_su8, TokenType::BareInstr, Opcode::I64RemS},
      {"i64.rem_u"_su8, TokenType::BareInstr, Opcode::I64RemU},
      {"i64.rotl"_su8, TokenType::BareInstr, Opcode::I64Rotl},
      {"i64.rotr"_su8, TokenType::BareInstr, Opcode::I64Rotr},
      {"i64.shl"_su8, TokenType::BareInstr, Opcode::I64Shl},
      {"i64.shr_s"_su8, TokenType::BareInstr, Opcode::I64ShrS},
      {"i64.shr_u"_su8, TokenType::BareInstr, Opcode::I64ShrU},
      {"i64.store16"_su8, TokenType::MemoryInstr, Opcode::I64Store16},
      {"i64.store32"_su8, TokenType::MemoryInstr, Opcode::I64Store32},
      {"i64.store8"_su8, TokenType::MemoryInstr, Opcode::I64Store8},
      {"i64.store"_su8, TokenType::MemoryInstr, Opcode::I64Store},
      {"i64.sub"_su8, TokenType::BareInstr, Opcode::I64Sub},
      {"i64.trunc_f32_s"_su8, TokenType::BareInstr, Opcode::I64TruncF32S},
      {"i64.trunc_f32_u"_su8, TokenType::BareInstr, Opcode::I64TruncF32U},
      {"i64.trunc_f64_s"_su8, TokenType::BareInstr, Opcode::I64TruncF64S},
      {"i64.trunc_f64_u"_su8, TokenType::BareInstr, Opcode::I64TruncF64U},
      {"i64.trunc_sat_f32_s"_su8, TokenType::BareInstr, Opcode::I64TruncSatF32S},
      {"i64.trunc_sat_f32_u"_su8, TokenType::BareInstr, Opcode::I64TruncSatF32U},
      {"i64.trunc_sat_f64_s"_su8, TokenType::BareInstr, Opcode::I64TruncSatF64S},
      {"i64.trunc_sat_f64_u"_su8, TokenType::BareInstr, Opcode::I64TruncSatF64U},
      {"i64x2.add"_su8, TokenType::BareInstr, Opcode::I64X2Add},
      {"i64x2.extract_lane"_su8, TokenType::SimdLaneInstr, Opcode::I64X2ExtractLane},
      {"i64x2.load32x2_s"_su8, TokenType::MemoryInstr, Opcode::I64X2Load32X2S},
      {"i64x2.load32x2_u"_su8, TokenType::MemoryInstr, Opcode::I64X2Load32X2U},
      {"i64x2.mul"_su8, TokenType::BareInstr, Opcode::I64X2Mul},
      {"i64x2.neg"_su8, TokenType::BareInstr, Opcode::I64X2Neg},
      {"i64x2.replace_lane"_su8, TokenType::SimdLaneInstr, Opcode::I64X2ReplaceLane},
      {"i64x2.shl"_su8, TokenType::BareInstr, Opcode::I64X2Shl},
      {"i64x2.shr_s"_su8, TokenType::BareInstr, Opcode::I64X2ShrS},
      {"i64x2.shr_u"_su8, TokenType::BareInstr, Opcode::I64X2ShrU},
      {"i64x2.splat"_su8, TokenType::BareInstr, Opcode::I64X2Splat},
      {"i64x2.sub"_su8, TokenType::BareInstr, Opcode::I64X2Sub},
      {"i64.xor"_su8, TokenType::BareInstr, Opcode::I64Xor},
      {"i8x16.add_saturate_s"_su8, TokenType::BareInstr, Opcode::I8X16AddSaturateS},
      {"i8x16.add_saturate_u"_su8, TokenType::BareInstr, Opcode::I8X16AddSaturateU},
      {"i8x16.add"_su8, TokenType::BareInstr, Opcode::I8X16Add},
      {"i8x16.all_true"_su8, TokenType::BareInstr, Opcode::I8X16AllTrue},
      {"i8x16.any_true"_su8, TokenType::BareInstr, Opcode::I8X16AnyTrue},
      {"i8x16.avgr_u"_su8, TokenType::BareInstr, Opcode::I8X16AvgrU},
      {"i8x16.eq"_su8, TokenType::BareInstr, Opcode::I8X16Eq},
      {"i8x16.extract_lane_s"_su8, TokenType::SimdLaneInstr, Opcode::I8X16ExtractLaneS},
      {"i8x16.extract_lane_u"_su8, TokenType::SimdLaneInstr, Opcode::I8X16ExtractLaneU},
      {"i8x16.ge_s"_su8, TokenType::BareInstr, Opcode::I8X16GeS},
      {"i8x16.ge_u"_su8, TokenType::BareInstr, Opcode::I8X16GeU},
      {"i8x16.gt_s"_su8, TokenType::BareInstr, Opcode::I8X16GtS},
      {"i8x16.gt_u"_su8, TokenType::BareInstr, Opcode::I8X16GtU},
      {"i8x16.le_s"_su8, TokenType::BareInstr, Opcode::I8X16LeS},
      {"i8x16.le_u"_su8, TokenType::BareInstr, Opcode::I8X16LeU},
      {"i8x16.lt_s"_su8, TokenType::BareInstr, Opcode::I8X16LtS},
      {"i8x16.lt_u"_su8, TokenType::BareInstr, Opcode::I8X16LtU},
      {"i8x16.max_s"_su8, TokenType::BareInstr, Opcode::I8X16MaxS},
      {"i8x16.max_u"_su8, TokenType::BareInstr, Opcode::I8X16MaxU},
      {"i8x16.min_s"_su8, TokenType::BareInstr, Opcode::I8X16MinS},
      {"i8x16.min_u"_su8, TokenType::BareInstr, Opcode::I8X16MinU},
      {"i8x16.narrow_i16x8_s"_su8, TokenType::BareInstr, Opcode::I8X16NarrowI16X8S},
      {"i8x16.narrow_i16x8_u"_su8, TokenType::BareInstr, Opcode::I8X16NarrowI16X8U},
      {"i8x16.neg"_su8, TokenType::BareInstr, Opcode::I8X16Neg},
      {"i8x16.ne"_su8, TokenType::BareInstr, Opcode::I8X16Ne},
      {"i8x16.replace_lane"_su8, TokenType::SimdLaneInstr, Opcode::I8X16ReplaceLane},
      {"i8x16.shl"_su8, TokenType::BareInstr, Opcode::I8X16Shl},
      {"i8x16.shr_s"_su8, TokenType::BareInstr, Opcode::I8X16ShrS},
      {"i8x16.shr_u"_su8, TokenType::BareInstr, Opcode::I8X16ShrU},
      {"i8x16.splat"_su8, TokenType::BareInstr, Opcode::I8X16Splat},
      {"i8x16.sub_saturate_s"_su8, TokenType::BareInstr, Opcode::I8X16SubSaturateS},
      {"i8x16.sub_saturate_u"_su8, TokenType::BareInstr, Opcode::I8X16SubSaturateU},
      {"i8x16.sub"_su8, TokenType::BareInstr, Opcode::I8X16Sub},
      {"local.get"_su8, TokenType::VarInstr, Opcode::LocalGet},
      {"local.set"_su8, TokenType::VarInstr, Opcode::LocalSet},
      {"local.tee"_su8, TokenType::VarInstr, Opcode::LocalTee},
      {"memory.copy"_su8, TokenType::BareInstr, Opcode::MemoryCopy},
      {"memory.fill"_su8, TokenType::BareInstr, Opcode::MemoryFill},
      {"memory.grow"_su8, TokenType::BareInstr, Opcode::MemoryGrow},
      {"memory.init"_su8, TokenType::VarInstr, Opcode::MemoryInit},
      {"memory.size"_su8, TokenType::BareInstr, Opcode::MemorySize},
      {"nop"_su8, TokenType::BareInstr, Opcode::Nop},
      {"ref.func"_su8, TokenType::VarInstr, Opcode::RefFunc},
      {"ref.is_null"_su8, TokenType::BareInstr, Opcode::RefIsNull},
      {"ref.null"_su8, TokenType::BareInstr, Opcode::RefNull},
      {"rethrow"_su8, TokenType::BareInstr, Opcode::Rethrow},
      {"return_call_indirect"_su8, TokenType::CallIndirectInstr, Opcode::ReturnCallIndirect},
      {"return_call"_su8, TokenType::VarInstr, Opcode::ReturnCall},
      {"return"_su8, TokenType::BareInstr, Opcode::Return},
      {"select"_su8, TokenType::SelectInstr, Opcode::Select},
      {"table.copy"_su8, TokenType::TableCopyInstr, Opcode::TableCopy},
      {"table.fill"_su8, TokenType::VarInstr, Opcode::TableFill},
      {"table.get"_su8, TokenType::VarInstr, Opcode::TableGet},
      {"table.grow"_su8, TokenType::VarInstr, Opcode::TableGrow},
      {"table.init"_su8, TokenType::TableInitInstr, Opcode::TableInit},
      {"table.set"_su8, TokenType::VarInstr, Opcode::TableSet},
      {"table.size"_su8, TokenType::VarInstr, Opcode::TableSize},
      {"throw"_su8, TokenType::VarInstr, Opcode::Throw},
      {"unreachable"_su8, TokenType::BareInstr, Opcode::Unreachable},
      {"v128.andnot"_su8, TokenType::BareInstr, Opcode::V128Andnot},
      {"v128.and"_su8, TokenType::BareInstr, Opcode::V128And},
      {"v128.bitselect"_su8, TokenType::BareInstr, Opcode::V128BitSelect},
      {"v128.const"_su8, TokenType::SimdConstInstr, Opcode::V128Const},
      {"v128.load"_su8, TokenType::MemoryInstr, Opcode::V128Load},
      {"v128.not"_su8, TokenType::BareInstr, Opcode::V128Not},
      {"v128.or"_su8, TokenType::BareInstr, Opcode::V128Or},
      {"v128.store"_su8, TokenType::MemoryInstr, Opcode::V128Store},
      {"v128.xor"_su8, TokenType::BareInstr, Opcode::V128Xor},
      {"v16x8.load_splat"_su8, TokenType::MemoryInstr, Opcode::V16X8LoadSplat},
      {"v32x4.load_splat"_su8, TokenType::MemoryInstr, Opcode::V32X4LoadSplat},
      {"v64x2.load_splat"_su8, TokenType::MemoryInstr, Opcode::V64X2LoadSplat},
      {"v8x16.load_splat"_su8, TokenType::MemoryInstr, Opcode::V8X16LoadSplat},
      {"v8x16.shuffle"_su8, TokenType::SimdShuffleInstr, Opcode::V8X16Shuffle},
      {"v8x16.swizzle"_su8, TokenType::BareInstr, Opcode::V8X16Swizzle},
      {"current_memory"_su8, TokenType::BareInstr, Opcode::MemorySize},
      {"f32.convert_s/i32"_su8, TokenType::BareInstr, Opcode::F32ConvertI32S},
      {"f32.convert_s/i64"_su8, TokenType::BareInstr, Opcode::F32ConvertI64S},
      {"f32.convert_u/i32"_su8, TokenType::BareInstr, Opcode::F32ConvertI32U},
      {"f32.convert_u/i64"_su8, TokenType::BareInstr, Opcode::F32ConvertI64U},
      {"f32.demote/f64"_su8, TokenType::BareInstr, Opcode::F32DemoteF64},
      {"f32.reinterpret/i32"_su8, TokenType::BareInstr, Opcode::F32ReinterpretI32},
      {"f64.convert_s/i32"_su8, TokenType::BareInstr, Opcode::F64ConvertI32S},
      {"f64.convert_s/i64"_su8, TokenType::BareInstr, Opcode::F64ConvertI64S},
      {"f64.convert_u/i32"_su8, TokenType::BareInstr, Opcode::F64ConvertI32U},
      {"f64.convert_u/i64"_su8, TokenType::BareInstr, Opcode::F64ConvertI64U},
      {"f64.promote/f32"_su8, TokenType::BareInstr, Opcode::F64PromoteF32},
      {"f64.reinterpret/i64"_su8, TokenType::BareInstr, Opcode::F64ReinterpretI64},
      {"get_global"_su8, TokenType::VarInstr, Opcode::GlobalGet},
      {"get_local"_su8, TokenType::VarInstr, Opcode::LocalGet},
      {"grow_memory"_su8, TokenType::BareInstr, Opcode::MemoryGrow},
      {"i32.reinterpret/f32"_su8, TokenType::BareInstr, Opcode::I32ReinterpretF32},
      {"i32.trunc_s/f32"_su8, TokenType::BareInstr, Opcode::I32TruncF32S},
      {"i32.trunc_s/f64"_su8, TokenType::BareInstr, Opcode::I32TruncF64S},
      {"i32.trunc_s:sat/f32"_su8, TokenType::BareInstr, Opcode::I32TruncSatF32S},
      {"i32.trunc_s:sat/f64"_su8, TokenType::BareInstr, Opcode::I32TruncSatF64S},
      {"i32.trunc_u/f32"_su8, TokenType::BareInstr, Opcode::I32TruncF32U},
      {"i32.trunc_u/f64"_su8, TokenType::BareInstr, Opcode::I32TruncF64U},
      {"i32.trunc_u:sat/f32"_su8, TokenType::BareInstr, Opcode::I32TruncSatF32U},
      {"i32.trunc_u:sat/f64"_su8, TokenType::BareInstr, Opcode::I32TruncSatF64U},
      {"i32.wrap/i64"_su8, TokenType::BareInstr, Opcode::I32WrapI64},
      {"i64.extend_s/i32"_su8, TokenType::BareInstr, Opcode::I64ExtendI32S},
      {"i64.extend_u/i32"_su8, TokenType::BareInstr, Opcode::I64ExtendI32U},
      {"i64.reinterpret/f64"_su8, TokenType::BareInstr, Opcode::I64ReinterpretF64},
      {"i64.trunc_s/f32"_su8, TokenType::BareInstr, Opcode::I64TruncF32S},
      {"i64.trunc_s/f64"_su8, TokenType::BareInstr, Opcode::I64TruncF64S},
      {"i64.trunc_s:sat/f32"_su8, TokenType::BareInstr, Opcode::I64TruncSatF32S},
      {"i64.trunc_s:sat/f64"_su8, TokenType::BareInstr, Opcode::I64TruncSatF64S},
      {"i64.trunc_u/f32"_su8, TokenType::BareInstr, Opcode::I64TruncF32U},
      {"i64.trunc_u/f64"_su8, TokenType::BareInstr, Opcode::I64TruncF64U},
      {"i64.trunc_u:sat/f32"_su8, TokenType::BareInstr, Opcode::I64TruncSatF32U},
      {"i64.trunc_u:sat/f64"_su8, TokenType::BareInstr, Opcode::I64TruncSatF64U},
      {"set_global"_su8, TokenType::VarInstr, Opcode::GlobalSet},
      {"set_local"_su8, TokenType::VarInstr, Opcode::LocalSet},
      {"tee_local"_su8, TokenType::VarInstr, Opcode::LocalTee},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), test.type, test.opcode}, test.span);
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
      {"anyref"_su8, ValueType::Anyref},
      {"exnref"_su8, ValueType::Exnref},
      {"f32"_su8, ValueType::F32},
      {"f64"_su8, ValueType::F64},
      {"funcref"_su8, ValueType::Funcref},
      {"i32"_su8, ValueType::I32},
      {"i64"_su8, ValueType::I64},
      {"nullref"_su8, ValueType::Nullref},
      {"v128"_su8, ValueType::V128},
  };
  for (auto test : tests) {
    ExpectLex({test.span.size(), TokenType::ValueType, test.value_type},
              test.span);
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
      {4, TokenType::Func},
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
      {{1, TokenType::Lpar}, 2}, {{6, TokenType::Module}, 17},
      {{1, TokenType::Lpar}, 2}, {{4, TokenType::Func}, 2},
      {{1, TokenType::Rpar}, 1}, {{1, TokenType::Rpar}, 1},
      {{1, TokenType::Rpar}, 0}, {{0, TokenType::Eof}, 0},
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
    {span.subspan(9, 4), TokenType::Func},
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
