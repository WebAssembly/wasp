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

#include "wasp/binary/visitor.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/binary/lazy_module.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

namespace {

// (module
//   (type (;0;) (func (param i32) (result i32)))
//   (type (;1;) (func (param f32) (result f32)))
//   (type (;2;) (func))
//   (import "foo" "bar" (func (;0;) (type 0)))
//   (func (;1;) (type 1) (param f32) (result f32)
//     (f32.const 0x1.5p+5 (;=42;)))
//   (func (;2;) (type 2))
//   (table (;0;) 1 2 funcref)
//   (memory (;0;) 1)
//   (global (;0;) i32 (i32.const 1))
//   (export "quux" (func 1))
//   (start 2)
//   (elem (;0;) (i32.const 0) 0 1)
//   (data (;0;) (i32.const 2) "hello"))
const u8 kTestModule[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0e, 0x03, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7d, 0x01, 0x7d, 0x60, 0x00, 0x00,
    0x02, 0x0b, 0x01, 0x03, 0x66, 0x6f, 0x6f, 0x03, 0x62, 0x61, 0x72, 0x00,
    0x00, 0x03, 0x03, 0x02, 0x01, 0x02, 0x04, 0x05, 0x01, 0x70, 0x01, 0x01,
    0x02, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x06, 0x01, 0x7f, 0x00, 0x41,
    0x01, 0x0b, 0x07, 0x08, 0x01, 0x04, 0x71, 0x75, 0x75, 0x78, 0x00, 0x01,
    0x08, 0x01, 0x02, 0x09, 0x08, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x02, 0x00,
    0x01, 0x0a, 0x0c, 0x02, 0x07, 0x00, 0x43, 0x00, 0x00, 0x28, 0x42, 0x0b,
    0x02, 0x00, 0x0b, 0x0b, 0x0b, 0x01, 0x00, 0x41, 0x02, 0x0b, 0x05, 0x68,
    0x65, 0x6c, 0x6c, 0x6f,
};

const int kSectionCount = 11;
const int kTypeCount = 3;
const int kFunctionCount = 2;
const int kInstructionCount = 3;  // f32.const; two implicit `end` instructions.

struct VisitorMock {
  MOCK_METHOD1(BeginModule, visit::Result(LazyModule&));
  MOCK_METHOD1(EndModule, visit::Result(LazyModule&));
  MOCK_METHOD1(OnSection, visit::Result(At<Section>));
  MOCK_METHOD1(BeginTypeSection, visit::Result(LazyTypeSection));
  MOCK_METHOD1(OnType, visit::Result(const At<TypeEntry>&));
  MOCK_METHOD1(EndTypeSection, visit::Result(LazyTypeSection));
  MOCK_METHOD1(BeginImportSection, visit::Result(LazyImportSection));
  MOCK_METHOD1(OnImport, visit::Result(const At<Import>&));
  MOCK_METHOD1(EndImportSection, visit::Result(LazyImportSection));
  MOCK_METHOD1(BeginFunctionSection, visit::Result(LazyFunctionSection));
  MOCK_METHOD1(OnFunction,  visit::Result(const At<Function>&));
  MOCK_METHOD1(EndFunctionSection, visit::Result(LazyFunctionSection));
  MOCK_METHOD1(BeginTableSection, visit::Result(LazyTableSection));
  MOCK_METHOD1(OnTable, visit::Result(const At<Table>&));
  MOCK_METHOD1(EndTableSection, visit::Result(LazyTableSection));
  MOCK_METHOD1(BeginMemorySection, visit::Result(LazyMemorySection));
  MOCK_METHOD1(OnMemory, visit::Result(const At<Memory>&));
  MOCK_METHOD1(EndMemorySection, visit::Result(LazyMemorySection));
  MOCK_METHOD1(BeginGlobalSection, visit::Result(LazyGlobalSection));
  MOCK_METHOD1(OnGlobal, visit::Result(const At<Global>&));
  MOCK_METHOD1(EndGlobalSection, visit::Result(LazyGlobalSection));
  MOCK_METHOD1(BeginEventSection, visit::Result(LazyEventSection));
  MOCK_METHOD1(OnEvent, visit::Result(const At<Event>&));
  MOCK_METHOD1(EndEventSection, visit::Result(LazyEventSection));
  MOCK_METHOD1(BeginExportSection, visit::Result(LazyExportSection));
  MOCK_METHOD1(OnExport, visit::Result(const At<Export>&));
  MOCK_METHOD1(EndExportSection, visit::Result(LazyExportSection));
  MOCK_METHOD1(BeginStartSection, visit::Result(StartSection));
  MOCK_METHOD1(OnStart, visit::Result(const At<Start>&));
  MOCK_METHOD1(EndStartSection, visit::Result(StartSection));
  MOCK_METHOD1(BeginElementSection, visit::Result(LazyElementSection));
  MOCK_METHOD1(OnElement, visit::Result(const At<ElementSegment>&));
  MOCK_METHOD1(EndElementSection, visit::Result(LazyElementSection));
  MOCK_METHOD1(BeginDataCountSection, visit::Result(DataCountSection));
  MOCK_METHOD1(OnDataCount, visit::Result(const At<DataCount>&));
  MOCK_METHOD1(EndDataCountSection, visit::Result(DataCountSection));
  MOCK_METHOD1(BeginCodeSection, visit::Result(LazyCodeSection));
  MOCK_METHOD1(BeginCode, visit::Result(const At<Code>&));
  MOCK_METHOD1(OnInstruction, visit::Result(const At<Instruction>&));
  MOCK_METHOD1(EndCode, visit::Result(const At<Code>&));
  MOCK_METHOD1(EndCodeSection, visit::Result(LazyCodeSection));
  MOCK_METHOD1(BeginDataSection, visit::Result(LazyDataSection));
  MOCK_METHOD1(OnData, visit::Result(const At<DataSegment>&));
  MOCK_METHOD1(EndDataSection, visit::Result(LazyDataSection));
};

class BinaryVisitorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}

  template <typename Visitor>
  visit::Result Visit(Visitor& visitor) {
    LazyModule module = ReadModule(SpanU8{kTestModule}, features, errors);
    return visit::Visit(module, visitor);
  }

  VisitorMock v;
  Features features;
  TestErrors errors;
};

}  // namespace

TEST_F(BinaryVisitorTest, AllOk) {
  using ::testing::_;
  using ::testing::Return;
  using ::wasp::binary::visit::Result;

  EXPECT_CALL(v, BeginModule(_)).Times(1);
  EXPECT_CALL(v, EndModule(_)).Times(1);

  EXPECT_CALL(v, OnSection(_)).Times(kSectionCount);

  EXPECT_CALL(v, BeginTypeSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnType(_))
      .Times(kTypeCount)
      .WillRepeatedly(Return(Result::Ok));
  EXPECT_CALL(v, EndTypeSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginImportSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnImport(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndImportSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginFunctionSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnFunction(_))
      .Times(kFunctionCount)
      .WillRepeatedly(Return(Result::Ok));
  EXPECT_CALL(v, EndFunctionSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginTableSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnTable(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndTableSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginMemorySection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnMemory(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndMemorySection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginGlobalSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnGlobal(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndGlobalSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginExportSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnExport(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndExportSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginStartSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnStart(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndStartSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginElementSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnElement(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndElementSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginCodeSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, BeginCode(_))
      .Times(kFunctionCount)
      .WillRepeatedly(Return(Result::Ok));
  EXPECT_CALL(v, OnInstruction(_))
      .Times(kInstructionCount)
      .WillRepeatedly(Return(Result::Ok));
  EXPECT_CALL(v, EndCode(_))
      .Times(kFunctionCount)
      .WillRepeatedly(Return(Result::Ok));
  EXPECT_CALL(v, EndCodeSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginDataSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnData(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, EndDataSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_EQ(Result::Ok, Visit(v));
}

TEST_F(BinaryVisitorTest, AllSkipped) {
  using ::testing::_;
  using ::testing::Return;
  using ::wasp::binary::visit::Result;

  EXPECT_CALL(v, BeginModule(_)).Times(1);
  EXPECT_CALL(v, EndModule(_)).Times(1);
  EXPECT_CALL(v, OnSection(_)).Times(kSectionCount);
  EXPECT_CALL(v, BeginTypeSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginImportSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginFunctionSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginTableSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginMemorySection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginGlobalSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginExportSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginStartSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginElementSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginCodeSection(_)).WillOnce(Return(Result::Skip));
  EXPECT_CALL(v, BeginDataSection(_)).WillOnce(Return(Result::Skip));

  EXPECT_EQ(Result::Ok, Visit(v));
}

TEST_F(BinaryVisitorTest, TypeSectionFailed) {
  using ::testing::_;
  using ::testing::Return;
  using ::wasp::binary::visit::Result;

  EXPECT_CALL(v, BeginModule(_)).Times(1);
  EXPECT_CALL(v, OnSection(_)).Times(1);
  EXPECT_CALL(v, BeginTypeSection(_)).WillOnce(Return(Result::Fail));
  EXPECT_EQ(Result::Fail, Visit(v));
}

TEST_F(BinaryVisitorTest, OnTypeFailed) {
  using ::testing::_;
  using ::testing::Return;
  using ::wasp::binary::visit::Result;

  EXPECT_CALL(v, BeginModule(_)).Times(1);
  EXPECT_CALL(v, OnSection(_)).Times(1);
  EXPECT_CALL(v, BeginTypeSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnType(_)).WillOnce(Return(Result::Fail));
  EXPECT_EQ(Result::Fail, Visit(v));
}

TEST_F(BinaryVisitorTest, OnTypeFailedAfter1) {
  using ::testing::_;
  using ::testing::Return;
  using ::wasp::binary::visit::Result;

  EXPECT_CALL(v, BeginModule(_)).Times(1);
  EXPECT_CALL(v, OnSection(_)).Times(1);
  EXPECT_CALL(v, BeginTypeSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnType(_))
      .Times(2)
      .WillOnce(Return(Result::Ok))
      .WillOnce(Return(Result::Fail));
  EXPECT_EQ(Result::Fail, Visit(v));
}

TEST_F(BinaryVisitorTest, OkSkipFail) {
  using ::testing::_;
  using ::testing::Return;
  using ::wasp::binary::visit::Result;

  EXPECT_CALL(v, BeginModule(_)).Times(1);
  EXPECT_CALL(v, OnSection(_)).Times(3);

  EXPECT_CALL(v, BeginTypeSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnType(_))
      .Times(kTypeCount)
      .WillRepeatedly(Return(Result::Ok));
  EXPECT_CALL(v, EndTypeSection(_)).WillOnce(Return(Result::Ok));

  EXPECT_CALL(v, BeginImportSection(_)).WillOnce(Return(Result::Skip));

  EXPECT_CALL(v, BeginFunctionSection(_)).WillOnce(Return(Result::Ok));
  EXPECT_CALL(v, OnFunction(_))
      .Times(kFunctionCount)
      .WillOnce(Return(Result::Ok))
      .WillOnce(Return(Result::Fail));

  EXPECT_EQ(Result::Fail, Visit(v));
}
