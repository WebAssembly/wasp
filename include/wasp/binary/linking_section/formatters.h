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

#ifndef WASP_BINARY_LINKING_SECTION_FORMATTERS_H_
#define WASP_BINARY_LINKING_SECTION_FORMATTERS_H_

#include "wasp/base/format.h"
#include "wasp/base/formatter_macros.h"
#include "wasp/binary/linking_section/types.h"

namespace fmt {

WASP_DECLARE_FORMATTER(binary::Comdat);
WASP_DECLARE_FORMATTER(binary::ComdatSymbol);
WASP_DECLARE_FORMATTER(binary::ComdatSymbolKind);
WASP_DECLARE_FORMATTER(binary::LinkingSubsection);
WASP_DECLARE_FORMATTER(binary::LinkingSubsectionId);
WASP_DECLARE_FORMATTER(binary::RelocationEntry);
WASP_DECLARE_FORMATTER(binary::InitFunction);
WASP_DECLARE_FORMATTER(binary::RelocationType);
WASP_DECLARE_FORMATTER(binary::SegmentInfo);
WASP_DECLARE_FORMATTER(binary::SymbolInfo);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Base);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Data);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Data::Defined);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Section);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Flags);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Flags::Binding);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Flags::Visibility);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Flags::Undefined);
WASP_DECLARE_FORMATTER(binary::SymbolInfo::Flags::ExplicitName);
WASP_DECLARE_FORMATTER(binary::SymbolInfoKind);

}  // namespace fmt

#include "wasp/binary/linking_section/formatters-inl.h"

#endif  // WASP_BINARY_LINKING_SECTION_FORMATTERS_H_
