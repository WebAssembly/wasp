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

#include <functional>

#include "wasp/base/variant.h"
#include "wasp/binary/custom_section.h"
#include "wasp/binary/known_section.h"
#include "wasp/binary/section_id.h"

namespace wasp {
namespace binary {

struct Section {
  bool is_known() const;
  bool is_custom() const;

  KnownSection& known();
  const KnownSection& known() const;
  CustomSection& custom();
  const CustomSection& custom() const;

  SectionId id() const;
  SpanU8 data() const;

  variant<KnownSection, CustomSection> contents;
};

bool operator==(const Section&, const Section&);
bool operator!=(const Section&, const Section&);

}  // namespace binary
}  // namespace wasp

namespace std {

template <>
struct hash<::wasp::binary::Section> {
  size_t operator()(const ::wasp::binary::Section&) const;
};

}  // namespace std

#include "wasp/binary/section-inl.h"

#endif  // WASP_BINARY_SECTION_H_
