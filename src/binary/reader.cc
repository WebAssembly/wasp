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

#include "src/binary/reader.h"

#include <cassert>

namespace wasp {
namespace binary {

struct BuildModuleHooks {
  Module module;
  Index code_index;

  HookResult OnError(const std::string&) { return HookResult::Stop; }

  HookResult OnSection(Section&& s) {
    using ::wasp::binary::encoding::Section;

    ReadResult r = ReadResult::Error;
    switch (s.id) {
      case Section::Type:     r = ReadTypeSection(s.data, *this); break;
      case Section::Import:   r = ReadImportSection(s.data, *this); break;
      case Section::Function: r = ReadFunctionSection(s.data, *this); break;
      case Section::Table:    r = ReadTableSection(s.data, *this); break;
      case Section::Memory:   r = ReadMemorySection(s.data, *this); break;
      case Section::Global:   r = ReadGlobalSection(s.data, *this); break;
      case Section::Export:   r = ReadExportSection(s.data, *this); break;
      case Section::Start:    r = ReadStartSection(s.data, *this); break;
      case Section::Element:  r = ReadElementSection(s.data, *this); break;
      case Section::Code:     r = ReadCodeSection(s.data, *this); break;
      case Section::Data:     r = ReadDataSection(s.data, *this); break;
      default: break;
    }

    return StopOnError(r);
  }

  HookResult OnCustomSection(CustomSection&& custom) {
    module.custom_sections.emplace_back(std::move(custom));
    return {};
  }

  HookResult OnTypeCount(Index count) {
    module.types.reserve(count);
    return {};
  }

  HookResult OnFuncType(Index type_index, FuncType&& func_type) {
    assert(type_index == module.types.size());
    module.types.emplace_back(std::move(func_type));
    return {};
  }

  HookResult OnImportCount(Index count) {
    module.imports.reserve(count);
    return {};
  }

  HookResult OnFuncImport(Index import_index, FuncImport&& func_import) {
    assert(import_index == module.imports.size());
    module.imports.emplace_back(std::move(func_import));
    return {};
  }

  HookResult OnTableImport(Index import_index,
                           TableImport&& table_import) {
    assert(import_index == module.imports.size());
    module.imports.emplace_back(std::move(table_import));
    return {};
  }

  HookResult OnMemoryImport(Index import_index, MemoryImport&& memory_import) {
    assert(import_index == module.imports.size());
    module.imports.emplace_back(std::move(memory_import));
    return {};
  }

  HookResult OnGlobalImport(Index import_index, GlobalImport&& global_import) {
    assert(import_index == module.imports.size());
    module.imports.emplace_back(std::move(global_import));
    return {};
  }

  HookResult OnFuncCount(Index count) {
    module.funcs.reserve(count);
    return {};
  }

  HookResult OnFunc(Index func_index, Func&& func) {
    assert(func_index == module.funcs.size());
    module.funcs.emplace_back(std::move(func));
    return {};
  }

  HookResult OnTableCount(Index count) {
    module.tables.reserve(count);
    return {};
  }

  HookResult OnTable(Index table_index, Table&& table) {
    assert(table_index == module.tables.size());
    module.tables.emplace_back(std::move(table));
    return {};
  }

  HookResult OnMemoryCount(Index count) {
    module.memories.reserve(count);
    return {};
  }

  HookResult OnMemory(Index memory_index, Memory&& memory) {
    assert(memory_index == module.memories.size());
    module.memories.emplace_back(std::move(memory));
    return {};
  }

  HookResult OnGlobalCount(Index count) {
    module.globals.reserve(count);
    return {};
  }

  HookResult OnGlobal(Index global_index, Global&& global) {
    assert(global_index == module.globals.size());
    module.globals.emplace_back(std::move(global));
    return {};
  }

  HookResult OnExportCount(Index count) {
    module.exports.reserve(count);
    return {};
  }

  HookResult OnExport(Index export_index, Export&& export_) {
    assert(export_index == module.exports.size());
    module.exports.emplace_back(export_);
    return {};
  }

  HookResult OnStart(Start&& start) {
    module.start = std::move(start);
    return {};
  }

  HookResult OnElementSegmentCount(Index count) {
    module.element_segments.reserve(count);
    return {};
  }

  HookResult OnElementSegment(Index segment_index, ElementSegment&& segment) {
    assert(segment_index == module.element_segments.size());
    module.element_segments.emplace_back(std::move(segment));
    return {};
  }

  HookResult OnCodeCount(Index count) {
    module.codes.reserve(count);
    return {};
  }

  HookResult OnCode(Index code_index, SpanU8 code) {
    this->code_index = code_index;
    return StopOnError(ReadCode(code, *this));
  }

  HookResult OnCodeContents(std::vector<LocalDecl>&& local_decls, Expr body) {
    assert(code_index == module.codes.size());
    module.codes.emplace_back(Code{std::move(local_decls), body});
    return {};
  }

  HookResult OnDataSegmentCount(Index count) {
    module.data_segments.reserve(count);
    return {};
  }

  HookResult OnDataSegment(Index segment_index, DataSegment&& segment) {
    assert(segment_index == module.data_segments.size());
    module.data_segments.emplace_back(std::move(segment));
    return {};
  }
};

optional<Module> ReadModule(SpanU8 data) {
  BuildModuleHooks hooks;
  if (ReadModule(data, hooks) == ReadResult::Ok) {
    return hooks.module;
  }
  return absl::nullopt;
}

}  // namespace binary
}  // namespace wasp
