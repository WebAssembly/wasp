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
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
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

struct Comdat {
  At<string_view> name;
  At<u32> flags;
  std::vector<At<ComdatSymbol>> symbols;
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


#define WASP_TYPES(WASP_V)          \
  WASP_V(Comdat)                    \
  WASP_V(ComdatSymbol)              \
  WASP_V(InitFunction)              \
  WASP_V(LinkingSubsection)         \
  WASP_V(RelocationEntry)           \
  WASP_V(SegmentInfo)               \
  WASP_V(SymbolInfo)                \
  WASP_V(SymbolInfo::Flags)         \
  WASP_V(SymbolInfo::Base)          \
  WASP_V(SymbolInfo::Data)          \
  WASP_V(SymbolInfo::Data::Defined) \
  WASP_V(SymbolInfo::Section) \

#define WASP_DECLARE_OPERATOR_EQ_NE(Type)    \
  bool operator==(const Type&, const Type&); \
  bool operator!=(const Type&, const Type&);

WASP_TYPES(WASP_DECLARE_OPERATOR_EQ_NE)

#undef WASP_DECLARE_OPERATOR_EQ_NE

}  // namespace binary
}  // namespace wasp

namespace std {

#define WASP_DECLARE_STD_HASH(Type)                       \
  template <>                                             \
  struct hash<::wasp::binary::Type> {                     \
    size_t operator()(const ::wasp::binary::Type&) const; \
  };

WASP_TYPES(WASP_DECLARE_STD_HASH)

#undef WASP_DECLARE_STD_HASH
#undef WASP_TYPES

}  // namespace std

#include "wasp/binary/linking_section/types-inl.h"

#endif // WASP_BINARY_LINKING_SECTION_TYPES_H_
