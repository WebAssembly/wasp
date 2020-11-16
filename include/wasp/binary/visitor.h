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

#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/sections.h"

namespace wasp::binary::visit {

enum class Result { Ok, Fail, Skip };

struct Visitor {
  Result BeginModule(const LazyModule&) { return Result::Ok; }
  Result EndModule(const LazyModule&) { return Result::Ok; }

  // All sections, known and custom.
  Result OnSection(At<Section>) { return Result::Ok; }

  // Section 1.
  Result BeginTypeSection(LazyTypeSection) { return Result::Ok; }
  Result OnType(const At<DefinedType>&) { return Result::Ok; }
  Result EndTypeSection(LazyTypeSection) { return Result::Ok; }

  // Section 2.
  Result BeginImportSection(LazyImportSection) { return Result::Ok; }
  Result OnImport(const At<Import>&) { return Result::Ok; }
  Result EndImportSection(LazyImportSection) { return Result::Ok; }

  // Section 3.
  Result BeginFunctionSection(LazyFunctionSection) { return Result::Ok; }
  Result OnFunction(const At<Function>&) { return Result::Ok; }
  Result EndFunctionSection(LazyFunctionSection) { return Result::Ok; }

  // Section 4.
  Result BeginTableSection(LazyTableSection) { return Result::Ok; }
  Result OnTable(const At<Table>&) { return Result::Ok; }
  Result EndTableSection(LazyTableSection) { return Result::Ok; }

  // Section 5.
  Result BeginMemorySection(LazyMemorySection) { return Result::Ok; }
  Result OnMemory(const At<Memory>&) { return Result::Ok; }
  Result EndMemorySection(LazyMemorySection) { return Result::Ok; }

  // Section 6.
  Result BeginGlobalSection(LazyGlobalSection) { return Result::Ok; }
  Result OnGlobal(const At<Global>&) { return Result::Ok; }
  Result EndGlobalSection(LazyGlobalSection) { return Result::Ok; }

  // Section 13.
  Result BeginEventSection(LazyEventSection) { return Result::Ok; }
  Result OnEvent(const At<Event>&) { return Result::Ok; }
  Result EndEventSection(LazyEventSection) { return Result::Ok; }

  // Section 7.
  Result BeginExportSection(LazyExportSection) { return Result::Ok; }
  Result OnExport(const At<Export>&) { return Result::Ok; }
  Result EndExportSection(LazyExportSection) { return Result::Ok; }

  // Section 8.
  Result BeginStartSection(StartSection) { return Result::Ok; }
  Result OnStart(const At<Start>&) { return Result::Ok; }
  Result EndStartSection(StartSection) { return Result::Ok; }

  // Section 9.
  Result BeginElementSection(LazyElementSection) { return Result::Ok; }
  Result OnElement(const At<ElementSegment>&) { return Result::Ok; }
  Result EndElementSection(LazyElementSection) { return Result::Ok; }

  // Section 12.
  Result BeginDataCountSection(DataCountSection) { return Result::Ok; }
  Result OnDataCount(const At<DataCount>&) { return Result::Ok; }
  Result EndDataCountSection(DataCountSection) { return Result::Ok; }

  // Section 10.
  Result BeginCodeSection(LazyCodeSection) { return Result::Ok; }
  Result BeginCode(const At<Code>&) { return Result::Ok; }
  Result OnInstruction(const At<Instruction>&) { return Result::Ok; }
  Result EndCode(const At<Code>&) { return Result::Ok; }
  Result EndCodeSection(LazyCodeSection) { return Result::Ok; }

  // Section 11.
  Result BeginDataSection(LazyDataSection) { return Result::Ok; }
  Result OnData(const At<DataSegment>&) { return Result::Ok; }
  Result EndDataSection(LazyDataSection) { return Result::Ok; }
};

struct SkipVisitor : Visitor {
  Result BeginModule(LazyModule&) { return Result::Ok; }
  Result EndModule(LazyModule&) { return Result::Skip; }
  Result OnSection(At<Section>) { return Result::Skip; }
  Result BeginTypeSection(LazyTypeSection) { return Result::Skip; }
  Result BeginImportSection(LazyImportSection) { return Result::Skip; }
  Result BeginFunctionSection(LazyFunctionSection) { return Result::Skip; }
  Result BeginTableSection(LazyTableSection) { return Result::Skip; }
  Result BeginMemorySection(LazyMemorySection) { return Result::Skip; }
  Result BeginGlobalSection(LazyGlobalSection) { return Result::Skip; }
  Result BeginEventSection(LazyEventSection) { return Result::Skip; }
  Result BeginExportSection(LazyExportSection) { return Result::Skip; }
  Result BeginStartSection(StartSection) { return Result::Skip; }
  Result BeginElementSection(LazyElementSection) { return Result::Skip; }
  Result BeginDataCountSection(DataCountSection) { return Result::Skip; }
  Result BeginCodeSection(LazyCodeSection) { return Result::Skip; }
  Result BeginCode(const At<Code>&) { return Result::Skip; }
  Result BeginDataSection(LazyDataSection) { return Result::Skip; }
};

template <typename Visitor>
Result Visit(LazyModule&, Visitor&);

#define WASP_CHECK(x)      \
  if (x == Result::Fail) { \
    return Result::Fail;   \
  }

#define WASP_IF_OK(x, body) \
  switch (x) {              \
    case Result::Fail:      \
      return Result::Fail;  \
    case Result::Skip:      \
      break;                \
    case Result::Ok:        \
      body break;           \
  }

#define WASP_IF_OK_ELSE_SKIP(x, body, skip) \
  switch (x) {                              \
    case Result::Fail:                      \
      return Result::Fail;                  \
    case Result::Skip:                      \
      skip break;                           \
    case Result::Ok:                        \
      body break;                           \
  }

#define WASP_SECTION_ELSE_SKIP(Name, skip_section)         \
  case SectionId::Name: {                                  \
    auto sec = Read##Name##Section(known, module.context); \
    WASP_IF_OK_ELSE_SKIP(                                  \
        visitor.Begin##Name##Section(sec),                 \
        {                                                  \
          for (const auto& item : sec.sequence) {          \
            WASP_CHECK(visitor.On##Name(item));            \
          }                                                \
          WASP_CHECK(visitor.End##Name##Section(sec));     \
        },                                                 \
        skip_section)                                      \
    break;                                                 \
  }

#define WASP_SECTION(Name) WASP_SECTION_ELSE_SKIP(Name, {})

#define WASP_OPT_SECTION(Name)                             \
  case SectionId::Name: {                                  \
    auto opt = Read##Name##Section(known, module.context); \
    WASP_IF_OK(visitor.Begin##Name##Section(opt), {        \
      if (opt) {                                           \
        WASP_CHECK(visitor.On##Name(*opt));                \
      }                                                    \
      WASP_CHECK(visitor.End##Name##Section(opt));         \
    })                                                     \
    break;                                                 \
  }

template <typename Visitor>
inline Result Visit(LazyModule& module, Visitor& visitor) {
  module.context.Reset();
  auto begin_res = visitor.BeginModule(module);
  if (begin_res != Result::Ok) {
    return begin_res;
  }

  for (auto section : module.sections) {
    auto res = visitor.OnSection(section);
    if (res == Result::Skip) {
      continue;
    } else if (res == Result::Fail) {
      return Result::Fail;
    }

    if (section->is_known()) {
      const auto& known = section->known();
      switch (known->id) {
        WASP_SECTION(Type)
        WASP_SECTION(Import)
        WASP_SECTION_ELSE_SKIP(Function, {
          module.context.defined_function_count += sec.count->value();
        })
        WASP_SECTION(Table)
        WASP_SECTION(Memory)
        WASP_SECTION(Global)
        WASP_SECTION(Event)
        WASP_SECTION(Export)
        WASP_OPT_SECTION(Start)
        WASP_SECTION(Element)
        WASP_OPT_SECTION(DataCount)

        case SectionId::Code: {
          auto sec = ReadCodeSection(known, module.context);
          WASP_IF_OK_ELSE_SKIP(
              visitor.BeginCodeSection(sec),
              {
                for (const auto& code : sec.sequence) {
                  WASP_IF_OK(
                      visitor.BeginCode(code), {
                        for (auto&& instr :
                             ReadExpression(*code->body, module.context)) {
                          WASP_CHECK(visitor.OnInstruction(instr));
                        }
                        EndCode(code->body->data.last(0), module.context);
                        WASP_CHECK(visitor.EndCode(code));
                      })
                }
                WASP_CHECK(visitor.EndCodeSection(sec));
              },
              // If skipping this section, increment by the number of code
              // items specified in this section.
              { module.context.code_count += sec.count->value(); })
          break;
        }

        WASP_SECTION_ELSE_SKIP(
            Data,
            // If skipping this section, increment by the number of data items
            // specified in this section.
            { module.context.data_count += sec.count->value(); })

        default: break;
      }
    }
  }
  EndModule(module.data, module.context);
  return visitor.EndModule(module);
}

#undef WASP_CHECK
#undef WASP_SECTION
#undef WASP_OPT_SECTION

}  // namespace wasp::binary::visit

#endif  // WASP_BINARY_VISITOR_H_
