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

#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/errors.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/lazy_data_section.h"
#include "wasp/binary/lazy_element_section.h"
#include "wasp/binary/lazy_export_section.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_function_section.h"
#include "wasp/binary/lazy_global_section.h"
#include "wasp/binary/lazy_import_section.h"
#include "wasp/binary/lazy_memory_section.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_table_section.h"
#include "wasp/binary/lazy_type_section.h"
#include "wasp/binary/start_section.h"

#include "third_party/sqlite/sqlite3.h"

namespace wasp {

using namespace ::wasp::binary;

struct Options {
  Features features;
};

struct Tool {
  explicit Tool(string_view filename, SpanU8 data, Options);
  ~Tool();
  Tool(const Tool&) = delete;
  Tool& operator=(const Tool&) = delete;

  void Run();
  bool OpenDB();
  template <typename... Args>
  bool Exec(const char* sql, const Args&...);
  bool CreateTables();
  bool InsertConstantExpression(const ConstantExpression&,
                                string_view table,
                                Index);

  void DoTypeSection(LazyTypeSection);
  void DoImportSection(LazyImportSection);
  void DoFunctionSection(LazyFunctionSection);
  void DoTableSection(LazyTableSection);
  void DoMemorySection(LazyMemorySection);
  void DoGlobalSection(LazyGlobalSection);
  void DoExportSection(LazyExportSection);
  void DoStartSection(StartSection);
  void DoElementSection(LazyElementSection);
  void DoDataSection(LazyDataSection);

  std::string filename;
  Options options;
  SpanU8 data;
  ErrorsNop errors;  // XXX
  LazyModule module;
  sqlite3* db = nullptr;
  Index imported_function_count = 0;
  Index imported_table_count = 0;
  Index imported_memory_count = 0;
  Index imported_global_count = 0;
};

}  // namespace wasp

using namespace ::wasp;

void PrintHelp() {
  print("usage: wasp_sqllite <filename>\n");
}

int main(int argc, char** argv) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  for (int i = 1; i < argc; ++i) {
    string_view arg = argv[i];
    if (arg[0] == '-') {
      print("Unknown short argument {}\n", arg[0]);
      break;
    } else {
      filename = arg;
    }
  }

  if (filename.empty()) {
    print("No filename given.\n");
    return 1;
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print("Error reading file {}.\n", filename);
    return 1;
  }

  SpanU8 data{*optbuf};
  Tool tool{filename, data, options};
  tool.Run();
  return 0;
}

Tool::Tool(string_view filename, SpanU8 data, Options options)
    : filename(filename),
      options{options},
      data{data},
      module{ReadModule(data, options.features, errors)} {}

Tool::~Tool() {
  sqlite3_close(db);
}

bool Tool::OpenDB() {
  if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
    print("Unable to open database.\n");
    return false;
  }
  return true;
}

template <typename... Args>
bool Tool::Exec(const char* sql, const Args&... args) {
  auto fmt_sql = vformat(sql, make_format_args(args...));
  print(">> Executing \"{}\"\n", fmt_sql);

  auto cb = [](void*, int columns, char** values, char** names) {
    string_view first = "";
    for (int i = 0; i < columns; ++i) {
      print("{}{} = {}", first, names[i], values[i]);
      first = ", ";
    }
    print("\n");
    return 0;
  };

  char* errmsg;
  if (sqlite3_exec(db, fmt_sql.c_str(), cb, nullptr, &errmsg) !=
      SQLITE_OK) {
    print("Error: {}\n", errmsg);
    sqlite3_free(errmsg);
    return false;
  }
  return true;
}

#define CHECK(x)  \
  if (!(x)) {     \
    return false; \
  } else {        \
  }

bool Tool::CreateTables() {
  CHECK(Exec("create table section (idx int primary key, code int, offset int, size int);"));
  CHECK(Exec("create table param_type (type_idx int, idx int, code int);"));
  CHECK(Exec("create table result_type (type_idx int, idx int, code int);"));
  CHECK(Exec("create table function_type (idx int primary key);"));
  CHECK(Exec("create table import (idx int primary key, module text, name text);"));
  CHECK(Exec("create table function_import (idx int primary key, import int, type int);"));
  CHECK(Exec("create table table_import (idx int primary key, import int, min int, max, elem_type int);"));
  CHECK(Exec("create table memory_import (idx int primary key, import int, min int, max);"));
  CHECK(Exec("create table global_import (idx int primary key, import int, valtype code, mut int);"));
  CHECK(Exec("create table function (idx int primary key, type int);"));
  CHECK(Exec("create table table_ (idx int primary key, min int, max, elem_type int);"));
  CHECK(Exec("create table memory (idx int primary key, min int, max);"));
  CHECK(Exec("create table global (idx int primary key, valtype code, mut int);"));
  CHECK(Exec("create table export (idx int primary key, kind int, name text, export_idx int);"));
  CHECK(Exec("create table global_init (global_idx int, opcode int, value);"));
  CHECK(Exec("create table start (func_idx int);"));
  CHECK(Exec("create table element (idx int primary key, kind int, table_idx);"));
  CHECK(Exec("create table element_offset (element_idx int, opcode int, value);"));
  CHECK(Exec("create table element_init (element_idx int, opcode, value);"));
  CHECK(Exec("create table data (idx int primary key, kind int, memory_idx, data);"));
  CHECK(Exec("create table data_offset (data_idx int, opcode int, value);"));
  return true;
}

bool Tool::InsertConstantExpression(const ConstantExpression& expr,
                                    string_view table, Index index) {
  auto instr = expr.instruction;
  auto opcode_val = static_cast<int>(instr.opcode);
  switch (instr.opcode) {
    case Opcode::I32Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr.s32_immediate());
      break;

    case Opcode::I64Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr.s64_immediate());
      break;

    case Opcode::F32Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr.f32_immediate());
      break;

    case Opcode::F64Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr.f64_immediate());
      break;

    case Opcode::GlobalGet:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr.index_immediate());
      break;

    default:
      return false;
  }

  return true;
}

void Tool::Run() {
  if (!(module.magic && module.version)) return;
  if (!OpenDB()) return;
  if (!CreateTables()) return;

  Index section_index = 0;
  auto module = ReadModule(data, options.features, errors);
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      auto offset = section.data().begin() - module.data.begin();
      auto size = section.data().size();

      Exec("insert into section values ({}, {}, {}, {});", section_index,
           static_cast<int>(known.id), offset, size);

      switch (known.id) {
        case SectionId::Type:
          DoTypeSection(ReadTypeSection(known, options.features, errors));
          break;

        case SectionId::Import:
          DoImportSection(ReadImportSection(known, options.features, errors));
          break;

        case SectionId::Function:
          DoFunctionSection(
              ReadFunctionSection(known, options.features, errors));
          break;

        case SectionId::Table:
          DoTableSection(ReadTableSection(known, options.features, errors));
          break;

        case SectionId::Memory:
          DoMemorySection(ReadMemorySection(known, options.features, errors));
          break;

        case SectionId::Global:
          DoGlobalSection(ReadGlobalSection(known, options.features, errors));
          break;

        case SectionId::Export:
          DoExportSection(ReadExportSection(known, options.features, errors));
          break;

        case SectionId::Start:
          DoStartSection(ReadStartSection(known, options.features, errors));
          break;

        case SectionId::Element:
          DoElementSection(ReadElementSection(known, options.features, errors));
          break;

        case SectionId::Data:
          DoDataSection(ReadDataSection(known, options.features, errors));
          break;

        default:
          break;
      }
    }
    ++section_index;
  }
}

void Tool::DoTypeSection(LazyTypeSection section) {
  Index type_index = 0;
  for (auto entry : section.sequence) {
    Exec("insert into function_type values ({});", type_index);
    Index param_index = 0;
    for (auto param : entry.type.param_types) {
      Exec("insert into param_type values ({}, {}, {});", type_index,
           param_index, static_cast<int>(param));
      ++param_index;
    }
    Index result_index = 0;
    for (auto result : entry.type.result_types) {
      Exec("insert into result_type values ({}, {}, {});", type_index,
           result_index, static_cast<int>(result));
      ++result_index;
    }
    ++type_index;
  }
}

namespace {

template <typename T>
std::string OrNull(optional<T> x) {
  return x ? format("{}", *x) : "null";
}

}  // namespace

void Tool::DoImportSection(LazyImportSection section) {
  Index import_index = 0;
  for (auto import : section.sequence) {
    Exec("insert into import values ({}, \"{}\", \"{}\");", import_index,
         import.module, import.name);
    switch (import.kind()) {
      case ExternalKind::Function:
        Exec("insert into function_import values ({}, {}, {});",
             imported_function_count++, import_index, import.index());
        break;

      case ExternalKind::Table: {
        auto table_type = import.table_type();
        Exec("insert into table_import values ({}, {}, {}, {}, {});",
             imported_table_count++, import_index, table_type.limits.min,
             OrNull(table_type.limits.max),
             static_cast<int>(table_type.elemtype));
        break;
      }

      case ExternalKind::Memory: {
        auto memory_type = import.memory_type();
        Exec("insert into memory_import values ({}, {}, {}, {});",
             imported_memory_count++, import_index, memory_type.limits.min,
             OrNull(memory_type.limits.max));
        break;
      }

      case ExternalKind::Global:
        auto global_type = import.global_type();
        Exec("insert into global_import values ({}, {});",
             imported_global_count++, import_index,
             static_cast<int>(global_type.valtype),
             static_cast<int>(global_type.mut));
        break;
    }
    ++import_index;
  }
}

void Tool::DoFunctionSection(LazyFunctionSection section) {
  Index function_index = imported_function_count;
  for (auto func : section.sequence) {
    Exec("insert into function values ({}, {});", function_index,
         func.type_index);
    ++function_index;
  }
}

void Tool::DoTableSection(LazyTableSection section) {
  Index table_index = imported_table_count;
  for (auto table : section.sequence) {
    auto table_type = table.table_type;
    Exec("insert into table_ values ({}, {}, {}, {});", table_index,
         table_type.limits.min, OrNull(table_type.limits.max),
         static_cast<int>(table_type.elemtype));
    ++table_index;
  }
}

void Tool::DoMemorySection(LazyMemorySection section) {
  Index memory_index = imported_memory_count;
  for (auto memory : section.sequence) {
    auto memory_type = memory.memory_type;
    Exec("insert into memory values ({}, {}, {});", memory_index,
         memory_type.limits.min, OrNull(memory_type.limits.max));
    ++memory_index;
  }
}

void Tool::DoGlobalSection(LazyGlobalSection section) {
  Index global_index = imported_global_count;
  for (auto global : section.sequence) {
    auto global_type = global.global_type;
    Exec("insert into global values ({}, {}, {});", global_index,
         static_cast<int>(global_type.valtype),
         static_cast<int>(global_type.mut));

    InsertConstantExpression(global.init, "global_init", global_index);
    ++global_index;
  }
}

void Tool::DoExportSection(LazyExportSection section) {
  Index export_index = 0;
  for (auto export_ : section.sequence) {
    Exec("insert into export values ({}, {}, \"{}\", {});", export_index,
         static_cast<int>(export_.kind), export_.name, export_.index);
    ++export_index;
  }
}

void Tool::DoStartSection(StartSection section) {
  if (section) {
    Exec("insert into start values ({});", section->func_index);
  }
}

void Tool::DoElementSection(LazyElementSection section) {
  Index element_index = 0;
  for (auto element : section.sequence) {
    if (element.is_active()) {
      const auto& active = element.active();
      Exec("insert into element values ({}, {}, {});", element_index,
           static_cast<int>(element.segment_type()), active.table_index);

      InsertConstantExpression(active.offset, "element_offset", element_index);
      for (auto index: active.init) {
        Exec("insert into element_init values ({}, null, {});", element_index,
             index);
      }
    } else {
      const auto& passive = element.passive();
      Exec("insert into element values ({}, {}, null);", element_index,
           static_cast<int>(element.segment_type()));
      for (auto expr: passive.init) {
        auto instr = expr.instruction;
        auto opcode_val = static_cast<int>(instr.opcode);
        switch (instr.opcode) {
          case Opcode::RefNull:
            Exec("insert into element_init values ({}, {}, null);",
                 element_index, opcode_val);
            break;

          case Opcode::RefFunc:
            Exec("insert into element_init values ({}, {}, {});", element_index,
                 opcode_val, instr.index_immediate());
            break;

          default:
            break;
        }
      }
    }
    ++element_index;
  }
}

void Tool::DoDataSection(LazyDataSection section) {
}
