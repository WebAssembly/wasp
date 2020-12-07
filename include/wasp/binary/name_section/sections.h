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

#ifndef WASP_BINARY_NAME_SECTION_SECTIONS_H_
#define WASP_BINARY_NAME_SECTION_SECTIONS_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/name_section/read.h"
#include "wasp/binary/name_section/types.h"
#include "wasp/binary/types.h"

namespace wasp::binary {

using LazyNameSection = LazySequence<NameSubsection>;
using ModuleNameSubsection = optional<string_view>;
using LazyFunctionNamesSubsection = LazySection<NameAssoc>;
using LazyLocalNamesSubsection = LazySection<IndirectNameAssoc>;

auto ReadNameSection(SpanU8, ReadCtx&) -> LazyNameSection;
auto ReadNameSection(CustomSection, ReadCtx&) -> LazyNameSection;

auto ReadModuleNameSubsection(SpanU8, ReadCtx&) -> ModuleNameSubsection;
auto ReadModuleNameSubsection(NameSubsection, ReadCtx&) -> ModuleNameSubsection;

auto ReadFunctionNamesSubsection(SpanU8, ReadCtx&)
    -> LazyFunctionNamesSubsection;
auto ReadFunctionNamesSubsection(NameSubsection, ReadCtx&)
    -> LazyFunctionNamesSubsection;

auto ReadLocalNamesSubsection(SpanU8, ReadCtx&) -> LazyLocalNamesSubsection;
auto ReadLocalNamesSubsection(NameSubsection, ReadCtx&)
    -> LazyLocalNamesSubsection;

}  // namespace wasp::binary

#endif // WASP_BINARY_NAME_SECTION_SECTIONS_H_
