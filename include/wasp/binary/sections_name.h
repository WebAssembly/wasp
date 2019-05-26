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

#ifndef WASP_BINARY_SECTIONS_NAME_H_
#define WASP_BINARY_SECTIONS_NAME_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/read_name.h"
#include "wasp/binary/types.h"
#include "wasp/binary/types_name.h"

namespace wasp {
namespace binary {

using LazyNameSection = LazySequence<NameSubsection>;
using ModuleNameSubsection = optional<string_view>;
using LazyFunctionNamesSubsection = LazySection<NameAssoc>;
using LazyLocalNamesSubsection = LazySection<IndirectNameAssoc>;

LazyNameSection ReadNameSection(SpanU8, Context&);
LazyNameSection ReadNameSection(CustomSection, Context&);

ModuleNameSubsection ReadModuleNameSubsection(SpanU8, Context&);
ModuleNameSubsection ReadModuleNameSubsection(NameSubsection, Context&);

LazyFunctionNamesSubsection ReadFunctionNamesSubsection(SpanU8, Context&);
LazyFunctionNamesSubsection ReadFunctionNamesSubsection(NameSubsection,
                                                        Context&);

LazyLocalNamesSubsection ReadLocalNamesSubsection(SpanU8, Context&);
LazyLocalNamesSubsection ReadLocalNamesSubsection(NameSubsection, Context&);

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SECTIONS_NAME_H_
