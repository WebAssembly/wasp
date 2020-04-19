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

#ifndef WASP_BINARY_LINKING_SECTION_TYPES_H_
#define WASP_BINARY_LINKING_SECTION_TYPES_H_

#include <functional>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/print_to_macros.h"
#include "wasp/base/span.h"
#include "wasp/base/std_hash_macros.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"

namespace wasp {
namespace binary {

enum class ComdatSymbolKind : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/def/comdat_symbol_kind.def"
#undef WASP_V
};

enum class LinkingSubsectionId : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/def/linking_subsection_id.def"
#undef WASP_V
};

enum class RelocationType : u8 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/def/relocation_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class SymbolInfoKind : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/def/symbol_info_kind.def"
#undef WASP_V
};

// Relocation section
//
struct RelocationEntry {
  At<RelocationType> type;
  At<u32> offset;
  At<Index> index;
  OptAt<s32> addend;
};

// Linking section

struct LinkingSubsection {
  At<LinkingSubsectionId> id;
  SpanU8 data;
};

// Subsection 5: SegmentInfo

struct SegmentInfo {
  At<string_view> name;
  At<u32> align_log2;
  At<u32> flags;
};

// Subsection 6: InitFunctions

struct InitFunction {
  At<u32> priority;
  At<Index> index;  // Symbol index.
};

// Subsection 7: ComdatInfo

struct ComdatSymbol {
  At<ComdatSymbolKind> kind;
  At<Index> index;
};

using ComdatSymbols = std::vector<At<ComdatSymbol>>;

struct Comdat {
  At<string_view> name;
  At<u32> flags;
  ComdatSymbols symbols;
};

// Subsection 8: SymbolTable

struct SymbolInfo {
  struct Flags {
    enum class Binding { Global, Weak, Local };
    enum class Visibility { Default, Hidden };
    enum class Undefined { No, Yes };
    enum class ExplicitName { No, Yes };

    Binding binding;
    Visibility visibility;
    Undefined undefined;
    ExplicitName explicit_name;
  };

  struct Base {
    At<SymbolInfoKind> kind;
    At<Index> index;
    OptAt<string_view> name;
  };

  struct Data {
    At<string_view> name;

    struct Defined {
      At<Index> index;
      At<u32> offset;
      At<u32> size;
    };
    optional<Defined> defined;
  };

  struct Section {
    At<u32> section;
  };

  // Function, Global and Event symbols.
  SymbolInfo(At<Flags>, const Base&);

  // Data symbols.
  SymbolInfo(At<Flags>, const Data&);

  // Section symbols.
  SymbolInfo(At<Flags>, const Section&);

  SymbolInfoKind kind() const;
  bool is_base() const;
  bool is_data() const;
  bool is_section() const;

  auto base() -> Base&;
  auto base() const -> const Base&;
  auto data() -> Data&;
  auto data() const -> const Data&;
  auto section() -> Section&;
  auto section() const -> const Section&;

  optional<string_view> name() const;

  At<Flags> flags;
  variant<Base, Data, Section> desc;
};

#define WASP_BINARY_LINKING_ENUMS(WASP_V) \
  WASP_V(binary::ComdatSymbolKind)        \
  WASP_V(binary::LinkingSubsectionId)     \
  WASP_V(binary::RelocationType)          \
  WASP_V(binary::SymbolInfoKind)

#define WASP_BINARY_LINKING_STRUCTS(WASP_V)                            \
  WASP_V(binary::Comdat, 3, name, flags, symbols)                      \
  WASP_V(binary::ComdatSymbol, 2, kind, index)                         \
  WASP_V(binary::InitFunction, 2, priority, index)                     \
  WASP_V(binary::LinkingSubsection, 2, id, data)                       \
  WASP_V(binary::RelocationEntry, 4, type, offset, index, addend)      \
  WASP_V(binary::SegmentInfo, 3, name, align_log2, flags)              \
  WASP_V(binary::SymbolInfo, 2, flags, desc)                           \
  WASP_V(binary::SymbolInfo::Flags, 4, binding, visibility, undefined, \
         explicit_name)                                                \
  WASP_V(binary::SymbolInfo::Base, 3, kind, index, name)               \
  WASP_V(binary::SymbolInfo::Data, 2, name, defined)                   \
  WASP_V(binary::SymbolInfo::Data::Defined, 3, index, offset, size)    \
  WASP_V(binary::SymbolInfo::Section, 1, section)

#define WASP_BINARY_LINKING_CONTAINERS(WASP_V) \
  WASP_V(binary::ComdatSymbols)

WASP_BINARY_LINKING_STRUCTS(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_BINARY_LINKING_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

// Used for gtest.

WASP_BINARY_LINKING_ENUMS(WASP_DECLARE_PRINT_TO)
WASP_BINARY_LINKING_STRUCTS(WASP_DECLARE_PRINT_TO)

}  // namespace binary
}  // namespace wasp

WASP_BINARY_LINKING_STRUCTS(WASP_DECLARE_STD_HASH)
WASP_BINARY_LINKING_CONTAINERS(WASP_DECLARE_STD_HASH)

#include "wasp/binary/linking_section/types-inl.h"

#endif // WASP_BINARY_LINKING_SECTION_TYPES_H_
