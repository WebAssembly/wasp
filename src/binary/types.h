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
#include "src/binary/defs.h"

namespace wasp {
namespace binary {

struct BorrowedTraits {
  using Name = string_view;
  using Buffer = SpanU8;

  static Name ToName(string_view x) { return x; }
  static Buffer ToBuffer(SpanU8 x) { return x; }
};

struct OwnedTraits {
  using Name = std::string;
  using Buffer = std::vector<u8>;

  static Name ToName(std::string x) { return x; }
  static Name ToName(string_view x) { return Name{x}; }
  static Buffer ToBuffer(std::vector<u8> x) { return x; }
  static Buffer ToBuffer(SpanU8 x) { return Buffer{x.begin(), x.end()}; }
};

enum class ValueType : s32 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_VALUE_TYPE(WASP_V)
#undef WASP_V
};

enum class BlockType : s32 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_BLOCK_TYPE(WASP_V)
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

enum class SectionId : u32 {
#define WASP_V(val, Name, str) Name,
  WASP_FOREACH_SECTION(WASP_V)
#undef WASP_V
};

enum class Opcode : u32 {
#define WASP_V(prefix, val, Name, str) Name,
  WASP_FOREACH_OPCODE(WASP_V)
#undef WASP_V
};

struct MemArg {
  MemArg(u32 align_log2, u32 offset) : align_log2(align_log2), offset(offset) {}

  u32 align_log2;
  u32 offset;
};

bool operator==(const MemArg&, const MemArg&);
bool operator!=(const MemArg&, const MemArg&);

struct Limits {
  explicit Limits(u32 min) : min(min) {}
  Limits(u32 min, u32 max) : min(min), max(max) {}

  u32 min;
  optional<u32> max;
};

bool operator==(const Limits&, const Limits&);
bool operator!=(const Limits&, const Limits&);

struct Locals {
  Locals(Index count, ValueType type) : count(count), type(type) {}

  Index count;
  ValueType type;
};

bool operator==(const Locals&, const Locals&);
bool operator!=(const Locals&, const Locals&);

template <typename Traits = BorrowedTraits>
struct KnownSection {
  KnownSection(SectionId id, SpanU8 data)
      : id(id), data(Traits::ToBuffer(data)) {}

  SectionId id;
  typename Traits::Buffer data;
};

template <typename Traits = BorrowedTraits>
struct CustomSection {
  CustomSection(string_view name, SpanU8 data)
      : name(Traits::ToName(name)), data(Traits::ToBuffer(data)) {}

  typename Traits::Name name;
  typename Traits::Buffer data;
};

template <typename Traits = BorrowedTraits>
struct Section {
  template <typename T>
  explicit Section(T&& contents) : contents(std::move(contents)) {}

  bool is_known() const { return contents.index() == 0; }
  bool is_custom() const { return contents.index() == 1; }

  KnownSection<Traits>& known() { return get<0>(contents); }
  const KnownSection<Traits>& known() const { return get<0>(contents); }
  CustomSection<Traits>& custom() { return get<1>(contents); }
  const CustomSection<Traits>& custom() const { return get<1>(contents); }

  variant<KnownSection<Traits>, CustomSection<Traits>> contents;
};

using ValueTypes = std::vector<ValueType>;

struct FunctionType {
  FunctionType(ValueTypes&& param_types, ValueTypes&& result_types)
      : param_types(std::move(param_types)),
        result_types(std::move(result_types)) {}

  ValueTypes param_types;
  ValueTypes result_types;
};

bool operator==(const FunctionType&, const FunctionType&);
bool operator!=(const FunctionType&, const FunctionType&);

struct TypeEntry {
  TypeEntry(FunctionType&& type) : type(std::move(type)) {}

  FunctionType type;
};

bool operator==(const TypeEntry&, const TypeEntry&);
bool operator!=(const TypeEntry&, const TypeEntry&);

struct TableType {
  TableType(Limits limits, ElementType elemtype)
      : limits(limits), elemtype(elemtype) {}

  Limits limits;
  ElementType elemtype;
};

bool operator==(const TableType&, const TableType&);
bool operator!=(const TableType&, const TableType&);

struct MemoryType {
  explicit MemoryType(Limits limits) : limits(limits) {}

  Limits limits;
};

bool operator==(const MemoryType&, const MemoryType&);
bool operator!=(const MemoryType&, const MemoryType&);

struct GlobalType {
  GlobalType(ValueType valtype, Mutability mut) : valtype(valtype), mut(mut) {}

  ValueType valtype;
  Mutability mut;
};

bool operator==(const GlobalType&, const GlobalType&);
bool operator!=(const GlobalType&, const GlobalType&);

template <typename Traits = BorrowedTraits>
struct Import {
  template <typename T>
  Import(string_view module, string_view name, T&& desc)
      : module(Traits::ToName(module)),
        name(Traits::ToName(name)),
        desc(std::move(desc)) {}

  ExternalKind kind() const { return static_cast<ExternalKind>(desc.index()); }

  typename Traits::Name module;
  typename Traits::Name name;
  variant<Index, TableType, MemoryType, GlobalType> desc;
};

template <typename Traits>
bool operator==(const Import<Traits>&, const Import<Traits>&);
template <typename Traits>
bool operator!=(const Import<Traits>&, const Import<Traits>&);

template <typename Traits = BorrowedTraits>
struct Expression {
  explicit Expression(SpanU8 data) : data(Traits::ToBuffer(data)) {}
  template <typename U>
  Expression(const Expression<U>& other) : data(Traits::ToBuffer(other.data)) {}

  typename Traits::Buffer data;
};

template <typename Traits = BorrowedTraits>
struct ConstantExpression {
  explicit ConstantExpression(SpanU8 data) : data(Traits::ToBuffer(data)) {}
  template <typename U>
  ConstantExpression(const ConstantExpression<U>& other)
      : data(Traits::ToBuffer(other.data)) {}

  typename Traits::Buffer data;
};

template <typename Traits>
bool operator==(const ConstantExpression<Traits>&,
                const ConstantExpression<Traits>&);
template <typename Traits>
bool operator!=(const ConstantExpression<Traits>&,
                const ConstantExpression<Traits>&);

struct EmptyImmediate {};

bool operator==(const EmptyImmediate&, const EmptyImmediate&);
bool operator!=(const EmptyImmediate&, const EmptyImmediate&);

struct CallIndirectImmediate {
  CallIndirectImmediate(Index index, u8 reserved)
      : index(index), reserved(reserved) {}

  Index index;
  u8 reserved;
};

bool operator==(const CallIndirectImmediate&, const CallIndirectImmediate&);
bool operator!=(const CallIndirectImmediate&, const CallIndirectImmediate&);

struct BrTableImmediate {
  BrTableImmediate(std::vector<Index>&& targets, Index default_target)
      : targets(std::move(targets)), default_target(default_target) {}

  std::vector<Index> targets;
  Index default_target;
};

bool operator==(const BrTableImmediate&, const BrTableImmediate&);
bool operator!=(const BrTableImmediate&, const BrTableImmediate&);

struct Instruction {
  explicit Instruction(Opcode opcode)
      : opcode(opcode), immediate(EmptyImmediate{}) {}
  template <typename T>
  Instruction(Opcode opcode, T&& value)
      : opcode(opcode), immediate(std::move(value)) {}

  Opcode opcode;
  variant<EmptyImmediate,
          BlockType,
          Index,
          CallIndirectImmediate,
          BrTableImmediate,
          u8,
          MemArg,
          s32,
          s64,
          f32,
          f64>
      immediate;
};

bool operator==(const Instruction&, const Instruction&);
bool operator!=(const Instruction&, const Instruction&);

using Instrs = std::vector<Instruction>;

struct Function {
  explicit Function(Index type_index) : type_index(type_index) {}

  Index type_index;
};

bool operator==(const Function&, const Function&);
bool operator!=(const Function&, const Function&);

struct Table {
  explicit Table(TableType table_type) : table_type(table_type) {}

  TableType table_type;
};

bool operator==(const Table&, const Table&);
bool operator!=(const Table&, const Table&);

struct Memory {
  explicit Memory(MemoryType memory_type) : memory_type(memory_type) {}

  MemoryType memory_type;
};

bool operator==(const Memory&, const Memory&);
bool operator!=(const Memory&, const Memory&);

template <typename Traits = BorrowedTraits>
struct Global {
  Global(GlobalType global_type, ConstantExpression<> init)
      : global_type(global_type), init(init) {}

  GlobalType global_type;
  ConstantExpression<Traits> init;
};

template <typename Traits>
bool operator==(const Global<Traits>&, const Global<Traits>&);
template <typename Traits>
bool operator!=(const Global<Traits>&, const Global<Traits>&);

template <typename Traits = BorrowedTraits>
struct Export {
  Export(ExternalKind kind, string_view name, Index index) :
    kind(kind), name(name), index(index) {}

  ExternalKind kind;
  typename Traits::Name name;
  Index index;
};

template <typename Traits>
bool operator==(const Export<Traits>&, const Export<Traits>&);
template <typename Traits>
bool operator!=(const Export<Traits>&, const Export<Traits>&);

struct Start {
  explicit Start(Index func_index) : func_index(func_index) {}

  Index func_index;
};

bool operator==(const Start&, const Start&);
bool operator!=(const Start&, const Start&);

template <typename Traits = BorrowedTraits>
struct ElementSegment {
  ElementSegment(Index table_index,
                 ConstantExpression<> offset,
                 std::vector<Index>&& init)
      : table_index(table_index), offset(offset), init(std::move(init)) {}

  Index table_index;
  ConstantExpression<Traits> offset;
  std::vector<Index> init;
};

template <typename Traits = BorrowedTraits>
struct Code {
  Code(std::vector<Locals>&& locals, Expression<> body)
      : locals(std::move(locals)), body(body) {}

  std::vector<Locals> locals;
  Expression<Traits> body;
};

template <typename Traits = BorrowedTraits>
struct DataSegment {
  DataSegment(Index memory_index, ConstantExpression<> offset, SpanU8 init)
      : memory_index(memory_index),
        offset(offset),
        init(Traits::ToBuffer(init)) {}

  Index memory_index;
  ConstantExpression<Traits> offset;
  typename Traits::Buffer init;
};

template <typename Traits = BorrowedTraits>
struct Module {
  std::vector<FunctionType> types;
  std::vector<Import<Traits>> imports;
  std::vector<Function> functions;
  std::vector<Table> tables;
  std::vector<Memory> memories;
  std::vector<Global<Traits>> globals;
  std::vector<Export<Traits>> exports;
  optional<Start> start;
  std::vector<ElementSegment<Traits>> element_segments;
  std::vector<Code<Traits>> codes;
  std::vector<DataSegment<Traits>> data_segments;
  std::vector<CustomSection<Traits>> custom_sections;
};

}  // namespace binary
}  // namespace wasp

#include "src/binary/types-inl.h"

#endif  // WASP_BINARY_TYPES_H_
