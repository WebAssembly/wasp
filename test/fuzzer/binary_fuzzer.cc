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

#include "wasp/base/errors.h"
#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/read.h"
#include "wasp/binary/types.h"
#include "wasp/binary/visitor.h"

using namespace wasp;
using namespace wasp::binary;
using namespace wasp::binary::visit;

namespace {

class FuzzErrors : public Errors {
  void HandlePushContext(SpanU8 pos, string_view desc) override {}
  void HandlePopContext() override {}
  void HandleOnError(Location loc, string_view message) override {}
};

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzErrors errors;
  Features features;
  Visitor visit;
  SpanU8 span{data, static_cast<span_index_t>(size)};
  LazyModule module = ReadModule(span, features, errors);
  Visit(module, visit);
  return 0;  // Non-zero return values are reserved for future use.
}
