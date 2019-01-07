//
// Copyright 2018 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_SECTION_H_
#define WASP_BINARY_SECTION_H_

#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/variant.h"
#include "wasp/binary/defs.h"

namespace wasp {
namespace binary {

enum class SectionId : u32 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_SECTION(WASP_V)
#undef WASP_V
};

struct KnownSection {
  SectionId id;
  SpanU8 data;
};

bool operator==(const KnownSection&, const KnownSection&);
bool operator!=(const KnownSection&, const KnownSection&);

struct CustomSection {
  string_view name;
  SpanU8 data;
};

bool operator==(const CustomSection&, const CustomSection&);
bool operator!=(const CustomSection&, const CustomSection&);

struct Section {
  bool is_known() const;
  bool is_custom() const;

  KnownSection& known();
  const KnownSection& known() const;
  CustomSection& custom();
  const CustomSection& custom() const;

  variant<KnownSection, CustomSection> contents;
};

bool operator==(const Section&, const Section&);
bool operator!=(const Section&, const Section&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/section-inl.h"

#endif  // WASP_BINARY_SECTION_H_
