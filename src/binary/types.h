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

enum class Mutability {
  Const,
  Var,
};

struct MemArg {
  MemArg(u32 align_log2, u32 offset) : align_log2(align_log2), offset(offset) {}

  u32 align_log2;
  u32 offset;
};

struct Limits {
  explicit Limits(u32 min) : min(min) {}
  Limits(u32 min, u32 max) : min(min), max(max) {}

  u32 min;
  optional<u32> max;
};

struct LocalDecl {
  LocalDecl(Index count, ValType type) : count(count), type(type) {}

  Index count;
  ValType type;
};

struct Section {
  Section(u32 id, SpanU8 data) : id(id), data(data) {}

  u32 id;
  SpanU8 data;
};

struct CustomSection {
  CustomSection(optional<u32> after_id, string_view name, SpanU8 data)
      : after_id(after_id), name(name), data(data) {}

  optional<u32> after_id;
  string_view name;
  SpanU8 data;
};

using ValTypes = std::vector<ValType>;

struct FuncType {
  FuncType(ValTypes&& param_types, ValTypes&& result_types)
      : param_types(std::move(param_types)),
        result_types(std::move(result_types)) {}

  ValTypes param_types;
  ValTypes result_types;
};

struct TableType {
  TableType(Limits limits, ValType elemtype)
      : limits(limits), elemtype(elemtype) {}

  Limits limits;
  ValType elemtype;
};

struct MemoryType {
  explicit MemoryType(Limits limits) : limits(limits) {}

  Limits limits;
};

struct GlobalType {
  GlobalType(ValType valtype, Mutability mut) : valtype(valtype), mut(mut) {}

  ValType valtype;
  Mutability mut;
};

struct Import {
  Import(ExternalKind kind, string_view module, string_view name)
      : kind(kind), module(module), name(name) {}

  ExternalKind kind;
  string_view module;
  string_view name;
};

struct FuncImport : Import {
  FuncImport(string_view module, string_view name, Index type_index)
      : Import{ExternalKind::Func, module, name}, type_index(type_index) {}

  Index type_index;
};

struct TableImport : Import {
  TableImport(string_view module, string_view name, TableType table_type)
      : Import{ExternalKind::Table, module, name}, table_type(table_type) {}

  TableType table_type;
};

struct MemoryImport : Import {
  MemoryImport(string_view module, string_view name, MemoryType memory_type)
      : Import{ExternalKind::Memory, module, name}, memory_type(memory_type) {}

  MemoryType memory_type;
};

struct GlobalImport : Import {
  GlobalImport(string_view module, string_view name, GlobalType global_type)
      : Import{ExternalKind::Global, module, name}, global_type(global_type) {}

  GlobalType global_type;
};

using ImportVariant =
    variant<FuncImport, TableImport, MemoryImport, GlobalImport>;

struct Expr {
  explicit Expr(SpanU8 instrs) : instrs(instrs) {}

  SpanU8 instrs;
};

struct Func {
  explicit Func(Index type_index) : type_index(type_index) {}

  Index type_index;
};

struct Table {
  explicit Table(TableType table_type) : table_type(table_type) {}

  TableType table_type;
};

struct Memory {
  explicit Memory(MemoryType memory_type) : memory_type(memory_type) {}

  MemoryType memory_type;
};

struct Global {
  Global(GlobalType global_type, Expr init_expr)
      : global_type(global_type), init_expr(init_expr) {}

  GlobalType global_type;
  Expr init_expr;
};

struct Export {
  Export(ExternalKind kind, string_view name, Index index) :
    kind(kind), name(name), index(index) {}

  ExternalKind kind;
  string_view name;
  Index index;
};

struct Start {
  explicit Start(Index func_index) : func_index(func_index) {}

  Index func_index;
};

struct ElementSegment {
  ElementSegment(Index table_index, Expr offset, std::vector<Index>&& init)
      : table_index(table_index), offset(offset), init(std::move(init)) {}

  Index table_index;
  Expr offset;
  std::vector<Index> init;
};

struct Code {
  Code(std::vector<LocalDecl>&& local_decls, Expr body)
      : local_decls(std::move(local_decls)), body(body) {}

  std::vector<LocalDecl> local_decls;
  Expr body;
};

struct DataSegment {
  DataSegment(Index memory_index, Expr offset, SpanU8 init)
      : memory_index(memory_index), offset(offset), init(init) {}

  Index memory_index;
  Expr offset;
  SpanU8 init;
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
  std::vector<CustomSection> custom_sections;
};

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_TYPES_H_
