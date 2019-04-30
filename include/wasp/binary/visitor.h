//
// Copyright 2019 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_VISITOR_H_
#define WASP_BINARY_VISITOR_H_

#include "wasp/binary/data_count_section.h"
#include "wasp/binary/lazy_code_section.h"
#include "wasp/binary/lazy_data_section.h"
#include "wasp/binary/lazy_element_section.h"
#include "wasp/binary/lazy_export_section.h"
#include "wasp/binary/lazy_function_section.h"
#include "wasp/binary/lazy_global_section.h"
#include "wasp/binary/lazy_import_section.h"
#include "wasp/binary/lazy_memory_section.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_table_section.h"
#include "wasp/binary/lazy_type_section.h"
#include "wasp/binary/start_section.h"

namespace wasp {
namespace binary {
namespace visit {

enum class Result { Ok, Fail, Skip };

struct Visitor {
  // All sections, known and custom.
  Result OnSection(Section) { return Result::Ok; }

  // Section 1.
  Result BeginTypeSection(LazyTypeSection) { return Result::Skip; }
  Result OnType(const TypeEntry&) { return Result::Ok; }
  Result EndTypeSection(LazyTypeSection) { return Result::Ok; }

  // Section 2.
  Result BeginImportSection(LazyImportSection) { return Result::Skip; }
  Result OnImport(const Import&) { return Result::Ok; }
  Result EndImportSection(LazyImportSection) { return Result::Ok; }

  // Section 3.
  Result BeginFunctionSection(LazyFunctionSection) { return Result::Skip; }
  Result OnFunction(const Function&) { return Result::Ok; }
  Result EndFunctionSection(LazyFunctionSection) { return Result::Ok; }

  // Section 4.
  Result BeginTableSection(LazyTableSection) { return Result::Skip; }
  Result OnTable(const Table&) { return Result::Ok; }
  Result EndTableSection(LazyTableSection) { return Result::Ok; }

  // Section 5.
  Result BeginMemorySection(LazyMemorySection) { return Result::Skip; }
  Result OnMemory(const Memory&) { return Result::Ok; }
  Result EndMemorySection(LazyMemorySection) { return Result::Ok; }

  // Section 6.
  Result BeginGlobalSection(LazyGlobalSection) { return Result::Skip; }
  Result OnGlobal(const Global&) { return Result::Ok; }
  Result EndGlobalSection(LazyGlobalSection) { return Result::Ok; }

  // Section 7.
  Result BeginExportSection(LazyExportSection) { return Result::Skip; }
  Result OnExport(const Export&) { return Result::Ok; }
  Result EndExportSection(LazyExportSection) { return Result::Ok; }

  // Section 8.
  Result BeginStartSection(StartSection) { return Result::Skip; }
  Result OnStart(const Start&) { return Result::Ok; }
  Result EndStartSection(StartSection) { return Result::Ok; }

  // Section 9.
  Result BeginElementSection(LazyElementSection) { return Result::Skip; }
  Result OnElement(const ElementSegment&) { return Result::Ok; }
  Result EndElementSection(LazyElementSection) { return Result::Ok; }

  // Section 12.
  Result BeginDataCountSection(DataCountSection) { return Result::Skip; }
  Result OnDataCount(const DataCount&) { return Result::Ok; }
  Result EndDataCountSection(DataCountSection) { return Result::Ok; }

  // Section 10.
  Result BeginCodeSection(LazyCodeSection) { return Result::Skip; }
  Result OnCode(const Code&) { return Result::Ok; }
  Result EndCodeSection(LazyCodeSection) { return Result::Ok; }

  // Section 11.
  Result BeginDataSection(LazyDataSection) { return Result::Skip; }
  Result OnData(const DataSegment&) { return Result::Ok; }
  Result EndDataSection(LazyDataSection) { return Result::Ok; }
};

template <typename Visitor>
Result Visit(LazyModule, Visitor&);

#define WASP_CHECK(x)      \
  if (x == Result::Fail) { \
    return Result::Fail;   \
  }

#define WASP_SECTION(Name)                                                 \
  case SectionId::Name: {                                                  \
    auto sec = Read##Name##Section(known, module.features, module.errors); \
    auto res = visitor.Begin##Name##Section(sec);                          \
    if (res == Result::Ok) {                                               \
      for (const auto& item : sec.sequence) {                              \
        WASP_CHECK(visitor.On##Name(item));                                \
      }                                                                    \
      WASP_CHECK(visitor.End##Name##Section(sec));                         \
    } else if (res == Result::Fail) {                                      \
      return Result::Fail;                                                 \
    }                                                                      \
    break;                                                                 \
  }

#define WASP_OPT_SECTION(Name)                                             \
  case SectionId::Name: {                                                  \
    auto opt = Read##Name##Section(known, module.features, module.errors); \
    auto res = visitor.Begin##Name##Section(opt);                          \
    if (res == Result::Ok) {                                               \
      if (opt) {                                                           \
        WASP_CHECK(visitor.On##Name(*opt));                                \
      }                                                                    \
      WASP_CHECK(visitor.End##Name##Section(opt));                         \
    } else if (res == Result::Fail) {                                      \
      return Result::Fail;                                                 \
    }                                                                      \
    break;                                                                 \
  }

template <typename Visitor>
inline Result Visit(LazyModule module, Visitor& visitor) {
  for (auto section : module.sections) {
    auto res = visitor.OnSection(section);
    if (res == Result::Skip) {
      continue;
    } else if (res == Result::Fail) {
      return Result::Fail;
    }

    if (section.is_known()) {
      const auto& known = section.known();
      switch (known.id) {
        WASP_SECTION(Type)
        WASP_SECTION(Import)
        WASP_SECTION(Function)
        WASP_SECTION(Table)
        WASP_SECTION(Memory)
        WASP_SECTION(Global)
        WASP_SECTION(Export)
        WASP_OPT_SECTION(Start)
        WASP_SECTION(Element)
        WASP_OPT_SECTION(DataCount)
        WASP_SECTION(Code)
        WASP_SECTION(Data)
        default: break;
      }
    }
  }
  return Result::Ok;
}

#undef WASP_CHECK
#undef WASP_SECTION
#undef WASP_OPT_SECTION

}  // namespace visit
}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_VISITOR_H_
