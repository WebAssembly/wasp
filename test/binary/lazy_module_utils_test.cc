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

#include "wasp/binary/lazy_module_utils.h"

#include <iterator>
#include <map>

#include "gtest/gtest.h"
#include "test/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

namespace {

SpanU8 GetModuleData() {
  return "\0asm\x01\0\0\0"
         "\x01\x04\x01\x60\0\0"          // 1 type: params:[] results:[]
         "\x02\x0b\x01\0\x06import\0\0"  // 1 import: func mod:"" name:"import"
         "\x03\x03\x02\0\0"              // 2 funcs: type 0, type 0
         "\x07\x0a\x01\x06"
         "export\0\x01"                      // 1 export: func 1 name:"export"
         "\x0a\x07\x02\x02\0\x0b\x02\0\x0b"  // 2 code: both empty
         "\0\x10\x04name"                    // "name" section
         "\x01\x09\x01\x02\x06"
         "custom"_su8;  // func 2, name "custom"
}

}  // namespace

TEST(BinaryLazyModuleUtilsTest, ForEachFunctionName) {
  Features features;
  TestErrors errors;
  auto module = ReadLazyModule(GetModuleData(), features, errors);

  ForEachFunctionName(module, [](const std::pair<Index, string_view>& pair) {
    switch (pair.first) {
      case 0:
        EXPECT_EQ("import", pair.second);
        break;

      case 1:
        EXPECT_EQ("export", pair.second);
        break;

      case 2:
        EXPECT_EQ("custom", pair.second);
        break;

      default:
        EXPECT_TRUE(false);
        break;
    }
  });
}

TEST(BinaryLazyModuleUtilsTest, CopyFunctionNames) {
  Features features;
  TestErrors errors;
  auto module = ReadLazyModule(GetModuleData(), features, errors);

  using FunctionNameMap = std::map<Index, string_view>;

  FunctionNameMap function_names;
  CopyFunctionNames(module,
                    std::inserter(function_names, function_names.end()));

  EXPECT_EQ((FunctionNameMap{{0, "import"}, {1, "export"}, {2, "custom"}}),
            function_names);
  ExpectNoErrors(errors);
}

TEST(BinaryLazyModuleUtilsTest, GetImportCount) {
  Features features;
  TestErrors errors;
  auto data =
      "\0asm\x01\0\0\0"
      "\x01\x04\x01\x60\0\0"
      "\x02\x13\x03"
      "\0\x01w\0\0"
      "\0\x01x\x03\x7f\0"
      "\0\x01z\x01\x70\0\0"_su8;

  auto module = ReadLazyModule(data, features, errors);

  EXPECT_EQ(1u, GetImportCount(module, ExternalKind::Function));
  EXPECT_EQ(1u, GetImportCount(module, ExternalKind::Global));
  EXPECT_EQ(0u, GetImportCount(module, ExternalKind::Memory));
  EXPECT_EQ(1u, GetImportCount(module, ExternalKind::Table));
  ExpectNoErrors(errors);
}
