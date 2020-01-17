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

#ifndef WASP_BINARY_WRITE_LINKING_H_
#define WASP_BINARY_WRITE_LINKING_H_

#include "wasp/binary/encoding/comdat_symbol_kind_encoding.h"
#include "wasp/binary/encoding/linking_subsection_id_encoding.h"
#include "wasp/binary/encoding/relocation_type_encoding.h"
#include "wasp/binary/encoding/symbol_info_kind_encoding.h"
#include "wasp/binary/types_linking.h"
#include "wasp/binary/write.h"

namespace wasp {
namespace binary {

template <typename Iterator>
Iterator Write(const Comdat& value, Iterator out) {
  out = Write(value.name, out);
  out = Write(value.flags, out);
  out = WriteVector(value.symbols.begin(), value.symbols.end(), out);
  return out;
}

template <typename Iterator>
Iterator Write(const ComdatSymbol& value, Iterator out) {
  out = Write(value.kind, out);
  out = WriteIndex(value.index, out);
  return out;
}

template <typename Iterator>
Iterator Write(ComdatSymbolKind value, Iterator out) {
  return Write(encoding::ComdatSymbolKind::Encode(value), out);
}

template <typename Iterator>
Iterator Write(const InitFunction& value, Iterator out) {
  out = Write(value.priority, out);
  out = WriteIndex(value.index, out);
  return out;
}

template <typename Iterator>
Iterator Write(LinkingSubsectionId value, Iterator out) {
  return Write(encoding::LinkingSubsectionId::Encode(value), out);
}

template <typename Iterator>
Iterator Write(RelocationType value, Iterator out) {
  return Write(encoding::RelocationType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(SymbolInfoKind value, Iterator out) {
  return Write(encoding::SymbolInfoKind::Encode(value), out);
}


}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_LINKING_H_
