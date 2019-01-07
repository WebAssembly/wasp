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

namespace wasp {
namespace binary {

inline ExternalKind Import::kind() const {
  return static_cast<ExternalKind>(desc.index());
}

inline bool Import::is_function() const {
  return kind() == ExternalKind::Function;
}

inline bool Import::is_table() const {
  return kind() == ExternalKind::Table;
}

inline bool Import::is_memory() const {
  return kind() == ExternalKind::Memory;
}

inline bool Import::is_global() const {
  return kind() == ExternalKind::Global;
}

inline Index& Import::index() {
  return get<Index>(desc);
}

inline const Index& Import::index() const {
  return get<Index>(desc);
}

inline TableType& Import::table_type() {
  return get<TableType>(desc);
}

inline const TableType& Import::table_type() const {
  return get<TableType>(desc);
}

inline MemoryType& Import::memory_type() {
  return get<MemoryType>(desc);
}

inline const MemoryType& Import::memory_type() const {
  return get<MemoryType>(desc);
}

inline GlobalType& Import::global_type() {
  return get<GlobalType>(desc);
}

inline const GlobalType& Import::global_type() const {
  return get<GlobalType>(desc);
}

}  // namespace binary
}  // namespace wasp
