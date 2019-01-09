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

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/name_section.h"
#include "wasp/binary/section.h"

namespace wasp {

class Features;

namespace binary {

/// ---
template <typename Errors>
using LazyNameSection = LazySequence<NameSubsection, Errors>;

template <typename Errors>
LazyNameSection<Errors> ReadNameSection(SpanU8, const Features&, Errors&);
template <typename Errors>
LazyNameSection<Errors> ReadNameSection(CustomSection,
                                        const Features&,
                                        Errors&);

/// ---
using ModuleNameSubsection = optional<string_view>;

template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(SpanU8, const Features&, Errors&);
template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(NameSubsection,
                                              const Features&,
                                              Errors&);

/// ---
template <typename Errors>
using LazyFunctionNamesSubsection = LazySection<NameAssoc, Errors>;

template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(SpanU8,
                                                                const Features&,
                                                                Errors&);
template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(NameSubsection,
                                                                const Features&,
                                                                Errors&);

/// ---
template <typename Errors>
using LazyLocalNamesSubsection = LazySection<IndirectNameAssoc, Errors>;

template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(SpanU8,
                                                          const Features&,
                                                          Errors&);
template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(NameSubsection,
                                                          const Features&,
                                                          Errors&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/lazy_name_section-inl.h"

#endif // WASP_BINARY_LAZY_NAME_SECTION_H_
