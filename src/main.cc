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

#include <string>

#include "src/base/file.h"
#include "src/base/to_string.h"
#include "src/base/types.h"
#include "src/binary/encoding.h"
#include "src/binary/to_string.h"
#include "src/binary/types.h"
#include "src/binary/reader.h"

using namespace ::wasp;
using namespace ::wasp::binary;

struct MyHooks {
  void OnError(const std::string& msg) { absl::PrintF("Error: %s\n", msg); }

  void OnSection(u32 code, SpanU8 data) {
    absl::PrintF("Section %d (%zd bytes)\n", code, data.size());
    switch (code) {
      case encoding::Section::Custom:   /*TODO*/ break;
      case encoding::Section::Type:     ReadTypeSection(data, *this); break;
      case encoding::Section::Import:   ReadImportSection(data, *this); break;
      case encoding::Section::Function: ReadFunctionSection(data, *this); break;
      case encoding::Section::Table:    ReadTableSection(data, *this); break;
      case encoding::Section::Memory:   ReadMemorySection(data, *this); break;
      case encoding::Section::Global:   ReadGlobalSection(data, *this); break;
      case encoding::Section::Export:   ReadExportSection(data, *this); break;
      case encoding::Section::Start:    ReadStartSection(data, *this); break;
      case encoding::Section::Element:  ReadElementSection(data, *this); break;
      case encoding::Section::Code:     ReadCodeSection(data, *this); break;
      case encoding::Section::Data:     ReadDataSection(data, *this); break;
      default: break;
    }
  }

  // Type section.
  void OnTypeCount(Index count) { absl::PrintF("Type count: %u\n", count); }
  void OnFuncType(Index type_index, const FuncType& func_type) {
    absl::PrintF("  Type[%u]: %s\n", type_index, ToString(func_type));
  }

  // Import section.
  void OnImportCount(Index count) { absl::PrintF("Import count: %u\n", count); }
  void OnFuncImport(Index import_index, const FuncImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }
  void OnTableImport(Index import_index, const TableImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }
  void OnMemoryImport(Index import_index, const MemoryImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }
  void OnGlobalImport(Index import_index, const GlobalImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }

  // Function section.
  void OnFuncCount(Index count) { absl::PrintF("Func count: %u\n", count); }
  void OnFunc(Index func_index, Index type_index) {
    absl::PrintF("  Func[%u]: {type %u, ...}\n", func_index, type_index);
  }

  // Table section.
  void OnTableCount(Index count) { absl::PrintF("Table count: %u\n", count); }
  void OnTable(Index table_index, const TableType& table_type) {
    absl::PrintF("  Table[%u]: {type %s}\n", table_index, ToString(table_type));
  }

  // Memory section.
  void OnMemoryCount(Index count) { absl::PrintF("Memory count: %u\n", count); }
  void OnMemory(Index memory_index, const MemoryType& memory_type) {
    absl::PrintF("  Memory[%u]: {type %s}\n", memory_index,
                 ToString(memory_type));
  }

  // Global section.
  void OnGlobalCount(Index count) { absl::PrintF("Global count: %u\n", count); }
  void OnGlobal(Index global_index, const Global& global) {
    absl::PrintF("  Global[%u]: %s\n", global_index, ToString(global));
  }

  // Export section.
  void OnExportCount(Index count) { absl::PrintF("Export count: %u\n", count); }
  void OnExport(Index export_index, const Export& export_) {
    absl::PrintF("  Export[%u]: %s\n", export_index, ToString(export_));
  }

  // Start section.
  void OnStart(Index func_index) {
    absl::PrintF("Start: {func %u}\n", func_index);
  }

  // Element section.
  void OnElementSegmentCount(Index count) {
    absl::PrintF("Element segment count: %u\n", count);
  }
  void OnElementSegment(Index segment_index, const ElementSegment& segment) {
    absl::PrintF("  ElementSegment[%u]: %s\n", segment_index,
                 ToString(segment));
  }

  // Code section.
  void OnCodeCount(Index count) { absl::PrintF("Code count: %u\n", count); }
  void OnCode(Index code_index, SpanU8 code) {
    absl::PrintF("  Code[%u]: %zd bytes...\n", code_index, code.size());
    ReadCode(code, *this);
  }

  void OnCodeContents(const std::vector<LocalDecl>& locals, const Expr& body) {
    absl::PrintF("    Locals: %s\n", ToString(locals));
    absl::PrintF("    Body: %s\n", ToString(body));
  }

  // Data section.
  void OnDataSegmentCount(Index count) {
    absl::PrintF("Data segment count: %u\n", count);
  }
  void OnDataSegment(Index segment_index, const DataSegment& segment) {
    absl::PrintF("  DataSegment[%u]: %s\n", segment_index, ToString(segment));
  }
};

int main(int argc, char** argv) {
  argc--;
  argv++;
  if (argc == 0) {
    absl::PrintF("No files.\n");
    return 1;
  }

  std::string filename{argv[0]};
  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    absl::PrintF("Error reading file.\n");
    return 1;
  }

  if (!ReadModule(SpanU8{*optbuf}, MyHooks{})) {
    absl::PrintF("Unable to read module.\n");
  }

  return 0;
}
