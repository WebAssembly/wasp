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

enum class HookResult {
  Continue,
  Stop,
};

enum class ReadResult {
  Error,
  Ok,
};

struct BaseHooksNop {
  HookResult OnError(const std::string&) { return {}; }
};

struct ExprHooksNop : BaseHooksNop {
  HookResult OnOpcodeBare(u8 opcode) { return {}; }
  HookResult OnOpcodeType(u8 opcode, ValType) { return {}; }
  HookResult OnOpcodeIndex(u8 opcode, Index) { return {}; }
  HookResult OnOpcodeCallIndirect(u8 opcode, Index, u8 reserved) { return {}; }
  HookResult OnOpcodeBrTable(u8 opcode,
                             const std::vector<Index>& targets,
                             Index default_target) {
    return {};
  }
  HookResult OnOpcodeMemarg(u8 opcode, const MemArg&) { return {}; }
  HookResult OnOpcodeI32Const(u8 opcode, s32) { return {}; }
  HookResult OnOpcodeI64Const(u8 opcode, s64) { return {}; }
  HookResult OnOpcodeF32Const(u8 opcode, f32) { return {}; }
  HookResult OnOpcodeF64Const(u8 opcode, f64) { return {}; }
};

struct ModuleHooksNop : BaseHooksNop {
  HookResult OnSection(u8 code, SpanU8 data) { return {}; }
};

struct TypeSectionHooksNop : BaseHooksNop {
  HookResult OnTypeCount(Index count) { return {}; }
  HookResult OnFuncType(Index type_index, const FuncType&) { return {}; }
};

struct ImportSectionHooksNop : BaseHooksNop {
  HookResult OnImportCount(Index count) { return {}; }
  HookResult OnFuncImport(Index import_index, const FuncImport&) { return {}; }
  HookResult OnTableImport(Index import_index, const TableImport&) {
    return {};
  }
  HookResult OnMemoryImport(Index import_index, const MemoryImport&) {
    return {};
  }
  HookResult OnGlobalImport(Index import_index, const GlobalImport&) {
    return {};
  }
};

struct FunctionSectionHooksNop : BaseHooksNop {
  HookResult OnFuncCount(Index count) { return {}; }
  HookResult OnFunc(Index func_index, Index type_index) { return {}; }
};

struct TableSectionHooksNop : BaseHooksNop {
  HookResult OnTableCount(Index count) { return {}; }
  HookResult OnTable(Index table_index, const TableType&) { return {}; }
};

struct MemorySectionHooksNop : BaseHooksNop {
  HookResult OnMemoryCount(Index count) { return {}; }
  HookResult OnMemory(Index memory_index, const MemoryType&) { return {}; }
};

struct GlobalSectionHooksNop : BaseHooksNop {
  HookResult OnGlobalCount(Index count) { return {}; }
  HookResult OnGlobal(Index global_index, const Global&) { return {}; }
};

struct ExportSectionHooksNop : BaseHooksNop {
  HookResult OnExportCount(Index count) { return {}; }
  HookResult OnExport(Index export_index, const Export&) { return {}; }
};

struct StartSectionHooksNop : BaseHooksNop {
  HookResult OnStart(Index func_index) { return {}; }
};

struct ElementSectionHooksNop : BaseHooksNop {
  HookResult OnElementSegmentCount(Index count) { return {}; }
  HookResult OnElementSegment(Index segment_index, const ElementSegment&) {
    return {};
  }
};

struct CodeSectionHooksNop : BaseHooksNop {
  HookResult OnCodeCount(Index count) { return {}; }
  HookResult OnCode(Index code_index, SpanU8 code) { return {}; }
};

struct CodeHooksNop : BaseHooksNop {
  HookResult OnCodeContents(const std::vector<LocalDecl>& locals,
                            const Expr& body) {
    return {};
  }
};

struct DataSectionHooksNop : BaseHooksNop {
  HookResult OnDataSegmentCount(Index count) { return {}; }
  HookResult OnDataSegment(Index segment_index, const DataSegment&) {
    return {};
  }
};

////////////////////////////////////////////////////////////////////////////////

optional<Module> ReadModule(SpanU8);

////////////////////////////////////////////////////////////////////////////////

HookResult StopOnError(ReadResult);

template <typename Hooks = ModuleHooksNop>
ReadResult ReadModule(SpanU8, Hooks&&);

template <typename Hooks = TypeSectionHooksNop>
ReadResult ReadTypeSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = ImportSectionHooksNop>
ReadResult ReadImportSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = FunctionSectionHooksNop>
ReadResult ReadFunctionSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = TableSectionHooksNop>
ReadResult ReadTableSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = MemorySectionHooksNop>
ReadResult ReadMemorySection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = GlobalSectionHooksNop>
ReadResult ReadGlobalSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = ExportSectionHooksNop>
ReadResult ReadExportSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = StartSectionHooksNop>
ReadResult ReadStartSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = ElementSectionHooksNop>
ReadResult ReadElementSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = CodeSectionHooksNop>
ReadResult ReadCodeSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = DataSectionHooksNop>
ReadResult ReadDataSection(SpanU8, Hooks&& = Hooks{});

template <typename Hooks = CodeHooksNop>
ReadResult ReadCode(SpanU8, Hooks&& = Hooks{});

}  // namespace binary
}  // namespace wasp

#include "src/binary/reader-inl.h"

#endif  // WASP_BINARY_READER_H_
