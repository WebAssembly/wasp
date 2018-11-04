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

#ifndef WASP_BINARY_TYPES_H_
#define WASP_BINARY_TYPES_H_

#include <string>
#include <vector>

#include "src/base/types.h"

namespace wasp {
namespace binary {

enum class ValType {
  I32,
  I64,
  F32,
  F64,
  Anyfunc,
  Func,
  Void,
};

enum class ExternalKind {
  Func,
  Table,
  Memory,
  Global,
};

struct MemArg {
  u32 align_log2;
  u32 offset;
};

struct Limits {
  u32 min;
  optional<u32> max;
};

struct LocalDecl {
  Index count;
  ValType type;
};

struct FuncType {
  std::vector<ValType> param_types;
  std::vector<ValType> result_types;
};

struct TableType {
  Limits limits;
  ValType elemtype;
};

struct MemoryType {
  Limits limits;
};

struct GlobalType {
  ValType valtype;
  bool mut;
};

struct Import {
  string_view module;
  string_view name;
  ExternalKind kind;
};

struct FuncImport : Import {
  FuncImport(string_view module, string_view name, Index type_index)
      : Import{module, name, ExternalKind::Func}, type_index(type_index) {}

  Index type_index;
};

struct TableImport : Import {
  TableImport(string_view module, string_view name, TableType table_type)
      : Import{module, name, ExternalKind::Table}, table_type(table_type) {}

  TableType table_type;
};

struct MemoryImport : Import {
  MemoryImport(string_view module, string_view name, MemoryType memory_type)
      : Import{module, name, ExternalKind::Memory}, memory_type(memory_type) {}

  MemoryType memory_type;
};

struct GlobalImport : Import {
  GlobalImport(string_view module, string_view name, GlobalType global_type)
      : Import{module, name, ExternalKind::Global}, global_type(global_type) {}

  GlobalType global_type;
};

using ImportVariant =
    variant<FuncImport, TableImport, MemoryImport, GlobalImport>;

struct Expr {
  Span<const u8> instrs;
};

struct Func {
  Index type_index;
};

struct Table {
  TableType table_type;
};

struct Memory {
  MemoryType memory_type;
};

struct Global {
  GlobalType global_type;
  Expr init_expr;
};

struct Export {
  string_view name;
  ExternalKind kind;
  Index index;
};

struct Start {
  Index func_index;
};

struct ElementSegment {
  Index table_index;
  Expr offset;
  std::vector<Index> init;
};

struct Code {
  std::vector<LocalDecl> local_decls;
  Expr body;
};

struct DataSegment {
  Index memory_index;
  Expr offset;
  Span<const u8> init;
};

struct Module {
  std::vector<FuncType> types;
  std::vector<ImportVariant> imports;
  std::vector<Func> funcs;
  std::vector<Table> tables;
  std::vector<Memory> memories;
  std::vector<Global> globals;
  std::vector<Export> exports;
  optional<Start> start;
  std::vector<ElementSegment> element_segments;
  std::vector<Code> codes;
  std::vector<DataSegment> data_segments;
};

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_TYPES_H_
