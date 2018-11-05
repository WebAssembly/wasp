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
  HookResult OnInstr(Instr&&) { return {}; }
};

struct ModuleHooksNop : BaseHooksNop {
  HookResult OnSection(Section&&) { return {}; }
  HookResult OnCustomSection(CustomSection<>&&) { return {}; }
};

struct TypeSectionHooksNop : BaseHooksNop {
  HookResult OnTypeCount(Index count) { return {}; }
  HookResult OnFuncType(Index type_index, FuncType&&) { return {}; }
};

struct ImportSectionHooksNop : BaseHooksNop {
  HookResult OnImportCount(Index count) { return {}; }
  HookResult OnImport(Index import_index, Import<BorrowedTraits>&&) {
    return {};
  }
};

struct FunctionSectionHooksNop : BaseHooksNop {
  HookResult OnFuncCount(Index count) { return {}; }
  HookResult OnFunc(Index func_index, Func&&) { return {}; }
};

struct TableSectionHooksNop : BaseHooksNop {
  HookResult OnTableCount(Index count) { return {}; }
  HookResult OnTable(Index table_index, Table&&) { return {}; }
};

struct MemorySectionHooksNop : BaseHooksNop {
  HookResult OnMemoryCount(Index count) { return {}; }
  HookResult OnMemory(Index memory_index, Memory&&) { return {}; }
};

struct GlobalSectionHooksNop : BaseHooksNop {
  HookResult OnGlobalCount(Index count) { return {}; }
  HookResult OnGlobal(Index global_index, Global<BorrowedTraits>&&) {
    return {};
  }
};

struct ExportSectionHooksNop : BaseHooksNop {
  HookResult OnExportCount(Index count) { return {}; }
  HookResult OnExport(Index export_index, Export<BorrowedTraits>&&) {
    return {};
  }
};

struct StartSectionHooksNop : BaseHooksNop {
  HookResult OnStart(Start&&) { return {}; }
};

struct ElementSectionHooksNop : BaseHooksNop {
  HookResult OnElementSegmentCount(Index count) { return {}; }
  HookResult OnElementSegment(Index segment_index,
                              ElementSegment<BorrowedTraits>&&) {
    return {};
  }
};

struct CodeSectionHooksNop : BaseHooksNop {
  HookResult OnCodeCount(Index count) { return {}; }
  HookResult OnCode(Index code_index, SpanU8 code) { return {}; }
};

struct CodeHooksNop : BaseHooksNop {
  HookResult OnCodeContents(std::vector<LocalDecl>&& locals,
                            Expr<BorrowedTraits> body) {
    return {};
  }
};

struct DataSectionHooksNop : BaseHooksNop {
  HookResult OnDataSegmentCount(Index count) { return {}; }
  HookResult OnDataSegment(Index segment_index, DataSegment<BorrowedTraits>&&) {
    return {};
  }
};

////////////////////////////////////////////////////////////////////////////////

struct OnErrorNop {
  void operator()(const std::string&) {}
};

template <typename Traits, typename F = OnErrorNop>
optional<Module<Traits>> ReadModule(SpanU8, F&& on_error = F{});

template <typename F = OnErrorNop>
optional<Instrs> ReadInstrs(SpanU8, F&& on_error = F{});

////////////////////////////////////////////////////////////////////////////////

HookResult StopOnError(ReadResult);

template <typename Hooks = ModuleHooksNop>
ReadResult ReadModuleHook(SpanU8, Hooks&&);

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
