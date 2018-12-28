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

#include "src/base/optional.h"
#include "src/base/span.h"
#include "src/base/string_view.h"
#include "src/base/types.h"
#include "src/base/variant.h"
#include "src/binary/defs.h"

namespace wasp {
namespace binary {

enum class ValueType : s32 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_VALUE_TYPE(WASP_V)
#undef WASP_V
};

enum class ElementType : s32 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_ELEMENT_TYPE(WASP_V)
#undef WASP_V
};

enum class ExternalKind : u8 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_EXTERNAL_KIND(WASP_V)
#undef WASP_V
};

enum class Mutability : u8 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_MUTABILITY(WASP_V)
#undef WASP_V
};

struct Limits {
  explicit Limits(u32 min);
  explicit Limits(u32 min, u32 max);

  u32 min;
  optional<u32> max;
};

bool operator==(const Limits&, const Limits&);
bool operator!=(const Limits&, const Limits&);

struct Locals {
  Index count;
  ValueType type;
};

bool operator==(const Locals&, const Locals&);
bool operator!=(const Locals&, const Locals&);

using ValueTypes = std::vector<ValueType>;

struct FunctionType {
  ValueTypes param_types;
  ValueTypes result_types;
};

bool operator==(const FunctionType&, const FunctionType&);
bool operator!=(const FunctionType&, const FunctionType&);

struct TypeEntry {
  FunctionType type;
};

bool operator==(const TypeEntry&, const TypeEntry&);
bool operator!=(const TypeEntry&, const TypeEntry&);

struct TableType {
  Limits limits;
  ElementType elemtype;
};

bool operator==(const TableType&, const TableType&);
bool operator!=(const TableType&, const TableType&);

struct MemoryType {
  Limits limits;
};

bool operator==(const MemoryType&, const MemoryType&);
bool operator!=(const MemoryType&, const MemoryType&);

struct GlobalType {
  ValueType valtype;
  Mutability mut;
};

bool operator==(const GlobalType&, const GlobalType&);
bool operator!=(const GlobalType&, const GlobalType&);

struct Import {
  ExternalKind kind() const;
  bool is_function() const;
  bool is_table() const;
  bool is_memory() const;
  bool is_global() const;

  Index& index();
  const Index& index() const;
  TableType& table_type();
  const TableType& table_type() const;
  MemoryType& memory_type();
  const MemoryType& memory_type() const;
  GlobalType& global_type();
  const GlobalType& global_type() const;

  string_view module;
  string_view name;
  variant<Index, TableType, MemoryType, GlobalType> desc;
};

bool operator==(const Import&, const Import&);
bool operator!=(const Import&, const Import&);

struct Expression {
  SpanU8 data;
};

bool operator==(const Expression&, const Expression&);
bool operator!=(const Expression&, const Expression&);

struct ConstantExpression {
  SpanU8 data;
};

bool operator==(const ConstantExpression&, const ConstantExpression&);
bool operator!=(const ConstantExpression&, const ConstantExpression&);

struct Function {
  Index type_index;
};

bool operator==(const Function&, const Function&);
bool operator!=(const Function&, const Function&);

struct Table {
  TableType table_type;
};

bool operator==(const Table&, const Table&);
bool operator!=(const Table&, const Table&);

struct Memory {
  MemoryType memory_type;
};

bool operator==(const Memory&, const Memory&);
bool operator!=(const Memory&, const Memory&);

struct Global {
  GlobalType global_type;
  ConstantExpression init;
};

bool operator==(const Global&, const Global&);
bool operator!=(const Global&, const Global&);

struct Export {
  ExternalKind kind;
  string_view name;
  Index index;
};

bool operator==(const Export&, const Export&);
bool operator!=(const Export&, const Export&);

struct Start {
  Index func_index;
};

bool operator==(const Start&, const Start&);
bool operator!=(const Start&, const Start&);

struct ElementSegment {
  Index table_index;
  ConstantExpression offset;
  std::vector<Index> init;
};

bool operator==(const ElementSegment&, const ElementSegment&);
bool operator!=(const ElementSegment&, const ElementSegment&);

struct Code {
  std::vector<Locals> locals;
  Expression body;
};

bool operator==(const Code&, const Code&);
bool operator!=(const Code&, const Code&);

struct DataSegment {
  Index memory_index;
  ConstantExpression offset;
  SpanU8 init;
};

bool operator==(const DataSegment&, const DataSegment&);
bool operator!=(const DataSegment&, const DataSegment&);

}  // namespace binary
}  // namespace wasp

#include "src/binary/types-inl.h"

#endif  // WASP_BINARY_TYPES_H_
