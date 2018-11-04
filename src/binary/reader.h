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

#ifndef WASP_BINARY_READER_H_
#define WASP_BINARY_READER_H_

#include "src/base/types.h"
#include "src/binary/types.h"

namespace wasp {
namespace binary {

struct BaseHooks {
  void OnError(const std::string&) {}
};

struct ExprHooks : BaseHooks {
  void OnOpcodeBare(u8 opcode) {}
  void OnOpcodeType(u8 opcode, ValType) {}
  void OnOpcodeIndex(u8 opcode, Index) {}
  void OnOpcodeCallIndirect(u8 opcode, Index, u8 reserved) {}
  void OnOpcodeBrTable(u8 opcode,
                       const std::vector<Index>& targets,
                       Index default_target) {}
  void OnOpcodeMemarg(u8 opcode, const MemArg&) {}
  void OnOpcodeI32Const(u8 opcode, s32) {}
  void OnOpcodeI64Const(u8 opcode, s64) {}
  void OnOpcodeF32Const(u8 opcode, f32) {}
  void OnOpcodeF64Const(u8 opcode, f64) {}
};

struct ModuleHooks : BaseHooks {
  void OnSection(u8 code, SpanU8 data) {}
};

struct TypeSectionHooks : BaseHooks {
  void OnTypeCount(Index count) {}
  void OnFuncType(Index type_index, const FuncType&) {}
};

struct ImportSectionHooks : BaseHooks {
  void OnImportCount(Index count) {}
  void OnFuncImport(Index import_index, const FuncImport&) {}
  void OnTableImport(Index import_index, const TableImport&) {}
  void OnMemoryImport(Index import_index, const MemoryImport&) {}
  void OnGlobalImport(Index import_index, const GlobalImport&) {}
};

struct FunctionSectionHooks : BaseHooks {
  void OnFuncCount(Index count) {}
  void OnFunc(Index func_index, Index type_index) {}
};

struct TableSectionHooks : BaseHooks {
  void OnTableCount(Index count) {}
  void OnTable(Index table_index, const TableType&) {}
};

struct MemorySectionHooks : BaseHooks {
  void OnMemoryCount(Index count) {}
  void OnMemory(Index memory_index, const MemoryType&) {}
};

struct GlobalSectionHooks : BaseHooks {
  void OnGlobalCount(Index count) {}
  void OnGlobal(Index global_index, const Global&) {}
};

struct ExportSectionHooks : BaseHooks {
  void OnExportCount(Index count) {}
  void OnExport(Index export_index, const Export&) {}
};

struct StartSectionHooks : BaseHooks {
  void OnStart(Index func_index) {}
};

struct ElementSectionHooks : BaseHooks {
  void OnElementSegmentCount(Index count) {}
  void OnElementSegment(Index segment_index, const ElementSegment&) {}
};

struct CodeSectionHooks : BaseHooks {
  void OnCodeCount(Index count) {}
  void OnCode(Index code_index, SpanU8 code) {}
};

struct CodeHooks : BaseHooks {
  void OnCodeContents(const std::vector<LocalDecl>& locals, const Expr& body) {}
};

struct DataSectionHooks : BaseHooks {
  void OnDataSegmentCount(Index count) {}
  void OnDataSegment(Index segment_index, const DataSegment&) {}
};

////////////////////////////////////////////////////////////////////////////////

template <typename Hooks = ModuleHooks>
bool ReadModule(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = TypeSectionHooks>
bool ReadTypeSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = ImportSectionHooks>
bool ReadImportSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = FunctionSectionHooks>
bool ReadFunctionSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = TableSectionHooks>
bool ReadTableSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = MemorySectionHooks>
bool ReadMemorySection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = GlobalSectionHooks>
bool ReadGlobalSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = ExportSectionHooks>
bool ReadExportSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = StartSectionHooks>
bool ReadStartSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = ElementSectionHooks>
bool ReadElementSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = CodeSectionHooks>
bool ReadCodeSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = DataSectionHooks>
bool ReadDataSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = CodeHooks>
bool ReadCode(SpanU8, Hooks&& = Hooks{});

}  // namespace binary
}  // namespace wasp

#include "src/binary/reader-inl.h"

#endif  // WASP_BINARY_READER_H_
