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

#ifndef WASP_BINARY_TO_STRING_H_
#define WASP_BINARY_TO_STRING_H_

#include "src/binary/types.h"

namespace wasp {
namespace binary {

std::string ToString(ValType);
std::string ToString(ExternalKind);
std::string ToString(Mutability);
std::string ToString(const MemArg&);
std::string ToString(const Limits&);
std::string ToString(const LocalDecl&);
template <typename Traits>
std::string ToString(const Section<Traits>&);
template <typename Traits>
std::string ToString(const CustomSection<Traits>&);
template <typename Traits>
std::string ToString(const KnownSection<Traits>&);
std::string ToString(const TypeEntry&);
std::string ToString(const FuncType&);
std::string ToString(const TableType&);
std::string ToString(const MemoryType&);
std::string ToString(const GlobalType&);
template <typename Traits>
std::string ToString(const Import<Traits>&);
template <typename Traits>
std::string ToString(const Expr<Traits>&);
std::string ToString(const Opcode&);
std::string ToString(const CallIndirectImmediate&);
std::string ToString(const BrTableImmediate&);
std::string ToString(const Instr&);
std::string ToString(const Func&);
std::string ToString(const Table&);
std::string ToString(const Memory&);
template <typename Traits>
std::string ToString(const Global<Traits>&);
template <typename Traits>
std::string ToString(const Export<Traits>&);
std::string ToString(const Start& start);
template <typename Traits>
std::string ToString(const ElementSegment<Traits>&);
template <typename Traits>
std::string ToString(const Code<Traits>&);
template <typename Traits>
std::string ToString(const DataSegment<Traits>&);

}  // namespace binary
}  // namespace wasp

#include "src/binary/to_string-inl.h"

#endif  // WASP_BINARY_TO_STRING_H_
