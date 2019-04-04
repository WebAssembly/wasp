//
// Copyright 2019 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_SYMBOL_INFO_H_
#define WASP_BINARY_SYMBOL_INFO_H_

#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/binary/symbol_info_kind.h"

namespace wasp {
namespace binary {

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

  Flags flags;
  variant<Base, Data, Section> desc;
};

bool operator==(const SymbolInfo&, const SymbolInfo&);
bool operator!=(const SymbolInfo&, const SymbolInfo&);

bool operator==(const SymbolInfo::Flags&, const SymbolInfo::Flags&);
bool operator!=(const SymbolInfo::Flags&, const SymbolInfo::Flags&);

bool operator==(const SymbolInfo::Base&, const SymbolInfo::Base&);
bool operator!=(const SymbolInfo::Base&, const SymbolInfo::Base&);

bool operator==(const SymbolInfo::Data&, const SymbolInfo::Data&);
bool operator!=(const SymbolInfo::Data&, const SymbolInfo::Data&);

bool operator==(const SymbolInfo::Data::Defined&,
                const SymbolInfo::Data::Defined&);
bool operator!=(const SymbolInfo::Data::Defined&,
                const SymbolInfo::Data::Defined&);

bool operator==(const SymbolInfo::Section&, const SymbolInfo::Section&);
bool operator!=(const SymbolInfo::Section&, const SymbolInfo::Section&);

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SYMBOL_INFO_H_
