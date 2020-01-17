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

#ifndef WASP_BINARY_TYPES_LINKING_H_
#define WASP_BINARY_TYPES_LINKING_H_

#include <functional>
#include <vector>

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/binary/types_linking.h"

namespace wasp {
namespace binary {

enum class ComdatSymbolKind : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/comdat_symbol_kind.def"
#undef WASP_V
};

enum class LinkingSubsectionId : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/linking_subsection_id.def"
#undef WASP_V
};

enum class RelocationType : u8 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/relocation_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class SymbolInfoKind : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/symbol_info_kind.def"
#undef WASP_V
};

// Relocation section
//
struct RelocationEntry {
  RelocationType type;
  u32 offset;
  Index index;
  optional<s32> addend;
};

// Linking section

struct LinkingSubsection {
  LinkingSubsectionId id;
  SpanU8 data;
};

// Subsection 5: SegmentInfo

struct SegmentInfo {
  string_view name;
  u32 align_log2;
  u32 flags;
};

// Subsection 6: InitFunctions

struct InitFunction {
  u32 priority;
  Index index;  // Symbol index.
};

// Subsection 7: ComdatInfo

struct ComdatSymbol {
  ComdatSymbolKind kind;
  Index index;
};

struct Comdat {
  string_view name;
  u32 flags;
  std::vector<ComdatSymbol> symbols;
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
    SymbolInfoKind kind;
    Index index;
    optional<string_view> name;
  };

  struct Data {
    string_view name;

    struct Defined {
      Index index;
      u32 offset;
      u32 size;
    };
    optional<Defined> defined;
  };

  struct Section {
    u32 section;
  };

  // Function, Global and Event symbols.
  SymbolInfo(Flags, const Base&);

  // Data symbols.
  SymbolInfo(Flags, const Data&);

  // Section symbols.
  SymbolInfo(Flags, const Section&);

  SymbolInfoKind kind() const;
  bool is_base() const;
  bool is_data() const;
  bool is_section() const;

  Base& base();
  const Base& base() const;
  Data& data();
  const Data& data() const;
  Section& section();
  const Section& section() const;

  optional<string_view> name() const;

  Flags flags;
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

#include "wasp/binary/symbol_info-inl.h"

#endif // WASP_BINARY_TYPES_LINKING_H_
