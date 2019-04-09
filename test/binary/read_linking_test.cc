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

#include "gtest/gtest.h"

#include "test/binary/read_test_utils.h"
#include "test/binary/test_utils.h"
#include "wasp/binary/read/read_comdat.h"
#include "wasp/binary/read/read_comdat_symbol.h"
#include "wasp/binary/read/read_comdat_symbol_kind.h"
#include "wasp/binary/read/read_linking_subsection.h"
#include "wasp/binary/read/read_linking_subsection_id.h"
#include "wasp/binary/read/read_relocation_entry.h"
#include "wasp/binary/read/read_relocation_type.h"
#include "wasp/binary/read/read_symbol_info.h"
#include "wasp/binary/read/read_symbol_info_kind.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReadLinkingTest, Comdat) {
  ExpectRead<Comdat>(Comdat{}, MakeSpanU8(""));
}
