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

#ifndef WASP_BINARY_LAZY_NAME_SECTION_H_
#define WASP_BINARY_LAZY_NAME_SECTION_H_

#include "src/base/optional.h"
#include "src/base/span.h"
#include "src/base/string_view.h"
#include "src/binary/lazy_section.h"
#include "src/binary/lazy_sequence.h"
#include "src/binary/types.h"

namespace wasp {
namespace binary {

/// ---
template <typename Errors>
using LazyNameSection = LazySequence<NameSubsection, Errors>;

template <typename Errors>
LazyNameSection<Errors> ReadNameSection(SpanU8, Errors&);
template <typename Errors>
LazyNameSection<Errors> ReadNameSection(CustomSection, Errors&);

/// ---
using ModuleNameSubsection = optional<string_view>;

template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(SpanU8, Errors&);
template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(NameSubsection, Errors&);

/// ---
template <typename Errors>
using LazyFunctionNamesSubsection = LazySection<NameAssoc, Errors>;

template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(SpanU8,
                                                                Errors&);
template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(NameSubsection,
                                                                Errors&);

/// ---
template <typename Errors>
using LazyLocalNamesSubsection = LazySection<IndirectNameAssoc, Errors>;

template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(SpanU8, Errors&);
template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(NameSubsection,
                                                          Errors&);

}  // namespace binary
}  // namespace wasp

#include "src/binary/lazy_name_section-inl.h"

#endif // WASP_BINARY_LAZY_NAME_SECTION_H_
