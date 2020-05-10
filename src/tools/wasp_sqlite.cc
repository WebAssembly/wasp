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

#include <iostream>

#include "src/tools/argparser.h"
#include "third_party/sqlite/sqlite3.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/errors.h"
#include "wasp/base/errors_nop.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/sections.h"

namespace wasp {

using namespace ::wasp::binary;

struct Options {
  Features features;
};

struct Tool {
  explicit Tool(span<string_view> filenames, Options);
  ~Tool();
  Tool(const Tool&) = delete;
  Tool& operator=(const Tool&) = delete;

  void Run();
  bool OpenDB();
  template <typename... Args>
  bool Exec(string_view sql, const Args&...);
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
  void DoCodeSection(LazyCodeSection);
  void DoDataSection(LazyDataSection);

  void Repl();

  size_t file_offset(SpanU8 data);

  span<string_view> filenames;
  Options options;
  ErrorsNop errors;  // XXX
  LazyModule* module; // HACK: current module
  sqlite3* db = nullptr;
  Index imported_function_count = 0;
  Index imported_table_count = 0;
  Index imported_memory_count = 0;
  Index imported_global_count = 0;
};

}  // namespace wasp

using namespace ::wasp;

int main(int argc, char** argv) {
  std::vector<string_view> args(argc - 1);
  std::copy(&argv[1], &argv[argc], args.begin());

  std::vector<string_view> filenames;
  string_view command;
  Options options;
  options.features.EnableAll();

  wasp::tools::ArgParser parser{"wasp_sqlite"};
  parser
      .Add('h', "--help", "print help and exit", [&]() {
        print(parser.GetHelpString());
        exit(0);
      })
      .Add('c', "--command", "<command>", "command", [&](string_view arg) {
        command = arg;
      })
      .Add("<filenames>", "filenames", [&](string_view arg) {
        filenames.push_back(arg);
      });
  parser.Parse(args);

  if (filenames.empty()) {
    print("No filename given.\n");
    return 1;
  }
  if (filenames.size() > 1) {
    print("Multiple files not yet supported\n");
    return 1;
  }

  Tool tool{filenames, options};
  tool.Run();
  if (!command.empty()) {
    tool.Exec(command);
  } else {
    tool.Repl();
  }
  return 0;
}

Tool::Tool(span<string_view> filenames, Options options)
    : filenames(filenames),
      options{options} {}

Tool::~Tool() {
  sqlite3_close(db);
}

void Tool::Run() {
  if (!OpenDB()) return;
  if (!CreateTables()) return;
  for (auto filename : filenames) {
    auto optbuf = ReadFile(filename);
    if (!optbuf) {
      print("Error reading file {}.\n", filename);
      continue;
    }

    SpanU8 data{*optbuf};
    LazyModule cur_module{ReadModule(data, options.features, errors)};
    if (!(cur_module.magic && cur_module.version)) continue;
    module = &cur_module;

    for (auto section : enumerate(module->sections)) {
      if (section.value->is_known()) {
        auto known = section.value->known();
        auto offset = file_offset(section.value->data());
        auto size = section.value->data().size();

        Exec("insert into section values ({}, {}, {}, {});", section.index,
             static_cast<int>(*known->id), offset, size);

        switch (known->id) {
          case SectionId::Type:
            DoTypeSection(ReadTypeSection(known, module->context));
            break;

          case SectionId::Import:
            DoImportSection(ReadImportSection(known, module->context));
            break;

          case SectionId::Function:
            DoFunctionSection(ReadFunctionSection(known, module->context));
            break;

          case SectionId::Table:
            DoTableSection(ReadTableSection(known, module->context));
            break;

          case SectionId::Memory:
            DoMemorySection(ReadMemorySection(known, module->context));
            break;

          case SectionId::Global:
            DoGlobalSection(ReadGlobalSection(known, module->context));
            break;

          case SectionId::Export:
            DoExportSection(ReadExportSection(known, module->context));
            break;

          case SectionId::Start:
            DoStartSection(ReadStartSection(known, module->context));
            break;

          case SectionId::Element:
            DoElementSection(ReadElementSection(known, module->context));
            break;

          case SectionId::Code:
            DoCodeSection(ReadCodeSection(known, module->context));
            break;

          case SectionId::Data:
            DoDataSection(ReadDataSection(known, module->context));
            break;

          default:
            break;
        }
      }
    }
  }

  print("memory used: {}\n", sqlite3_memory_highwater(0));
}

bool Tool::OpenDB() {
  sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);
  if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
    print("Unable to open database.\n");
    return false;
  }
  return true;
}

template <typename... Args>
bool Tool::Exec(string_view sql, const Args&... args) {
  auto fmt_sql = vformat(sql, make_format_args(args...));
  // print(">> Executing \"{}\"\n", fmt_sql);

  auto cb = [](void*, int columns, char** values, char** names) {
    string_view first = "";
    for (int i = 0; i < columns; ++i) {
      print("{}{} = {}", first, names[i], values[i] ? values[i] : "(null)");
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
  CHECK(Exec("create table element_init (element_idx int, idx int, opcode, value);"));
  CHECK(Exec("create table data (idx int primary key, kind int, memory_idx, data);"));
  CHECK(Exec("create table data_offset (data_idx int, opcode int, value);"));
  CHECK(Exec("create table code (idx int primary key, offset int, size int);"));
  CHECK(Exec("create table locals (code_idx int, idx int, count int, type int);"));
  CHECK(Exec("create table instruction (code_idx int, idx int, offset int, size int, opcode int, immediate);"));

  // Name tables.
  CHECK(Exec("create table section_name (code int primary key, name text);"));
  CHECK(Exec("create table value_type_name (code int primary key, name text);"));
  CHECK(Exec("create table opcode_name (opcode int primary key, name text);"));

  // Views using names.
  CHECK(Exec("create view section_n as select section.*, name from section, section_name using (code);"));
  CHECK(Exec("create view param_type_n as select param_type.*, name from param_type, value_type_name using (code);"));
  CHECK(Exec("create view result_type_n as select result_type.*, name from result_type, value_type_name using (code);"));
  CHECK(Exec("create view global_import_n as select global_import.*, name from global_import, value_type_name on valtype = code;"));
  CHECK(Exec("create view global_n as select global.*, name from global, value_type_name on valtype = code;"));
  CHECK(Exec("create view locals_n as select locals.*, name from locals, value_type_name on type = code;"));
  CHECK(Exec("create view element_offset_n as select element_offset.*, name from element_offset, opcode_name using (opcode);"));
  CHECK(Exec("create view data_offset_n as select data_offset.*, name from data_offset, opcode_name using (opcode);"));
  CHECK(Exec("create view instruction_n as select instruction.*, name from instruction, opcode_name using (opcode);"));

  // Convenience views.
  CHECK(Exec(R"(create view ftype as
    with
      pt as (
        select ft.idx, group_concat(name) as names
        from function_type as ft
        left join param_type_n on ft.idx = type_idx
        group by ft.idx
      ),
      rt as (
        select ft.idx, group_concat(name) as names
        from function_type as ft
        left join result_type_n on ft.idx = type_idx
        group by ft.idx
      )
    select ft.idx, pt.names as params, rt.names as results
    from function_type as ft, pt, rt
    where ft.idx = pt.idx and ft.idx = rt.idx;)"));

  CHECK(Exec("create view function_ft as select f.idx, ftype.idx as type_idx, ftype.params, ftype.results from function as f, ftype on type=ftype.idx;"));

  int count = 0;
#define WASP_V(prefix, val, Name, str, ...) \
  CHECK(Exec("insert into opcode_name values ({}, \"{}\");", count++, str));
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V

  count = 0;
#define WASP_V(val, Name, str) \
  CHECK(Exec("insert into value_type_name values ({}, \"{}\");", count++, str));
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/def/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V

  count = 0;
#define WASP_V(val, Name, str) \
  CHECK(Exec("insert into section_name values ({}, \"{}\");", count++, str));
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/def/section_id.def"
#undef WASP_V
#undef WASP_FEATURE_V

  return true;
}

bool Tool::InsertConstantExpression(const ConstantExpression& expr,
                                    string_view table, Index index) {
  auto instr = expr.instruction;
  auto opcode_val = static_cast<int>(*instr->opcode);
  switch (instr->opcode) {
    case Opcode::I32Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr->s32_immediate());
      break;

    case Opcode::I64Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr->s64_immediate());
      break;

    case Opcode::F32Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr->f32_immediate());
      break;

    case Opcode::F64Const:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr->f64_immediate());
      break;

    case Opcode::GlobalGet:
      Exec("insert into {} values ({}, {}, {});", table, index, opcode_val,
           instr->index_immediate());
      break;

    default:
      return false;
  }

  return true;
}

void Tool::DoTypeSection(LazyTypeSection section) {
  for (auto entry : enumerate(section.sequence)) {
    Exec("insert into function_type values ({});", entry.index);
    for (auto param : enumerate(entry.value->type->param_types)) {
      Exec("insert into param_type values ({}, {}, {});", entry.index,
           param.index, static_cast<int>(*param.value));
    }
    for (auto result : enumerate(entry.value->type->result_types)) {
      Exec("insert into result_type values ({}, {}, {});", entry.index,
           result.index, static_cast<int>(*result.value));
    }
  }
}

namespace {

template <typename T>
std::string OrNull(optional<T> x) {
  return x ? format("{}", *x) : "null";
}

}  // namespace

void Tool::DoImportSection(LazyImportSection section) {
  for (auto import : enumerate(section.sequence)) {
    Exec("insert into import values ({}, \"{}\", \"{}\");", import.index,
         import.value->module, import.value->name);
    switch (import.value->kind()) {
      case ExternalKind::Function:
        Exec("insert into function_import values ({}, {}, {});",
             imported_function_count++, import.index, import.value->index());
        break;

      case ExternalKind::Table: {
        auto table_type = import.value->table_type();
        Exec("insert into table_import values ({}, {}, {}, {}, {});",
             imported_table_count++, import.index, table_type->limits->min,
             OrNull(table_type->limits->max),
             static_cast<int>(*table_type->elemtype));
        break;
      }

      case ExternalKind::Memory: {
        auto memory_type = import.value->memory_type();
        Exec("insert into memory_import values ({}, {}, {}, {});",
             imported_memory_count++, import.index, memory_type->limits->min,
             OrNull(memory_type->limits->max));
        break;
      }

      case ExternalKind::Global: {
        auto global_type = import.value->global_type();
        Exec("insert into global_import values ({}, {}, {}, {});",
             imported_global_count++, import.index,
             static_cast<int>(*global_type->valtype),
             static_cast<int>(*global_type->mut));
        break;
      }

      case ExternalKind::Event:
        // TODO(binji): implement
        break;
    }
  }
}

void Tool::DoFunctionSection(LazyFunctionSection section) {
  for (auto func : enumerate(section.sequence, imported_function_count)) {
    Exec("insert into function values ({}, {});", func.index,
         func.value->type_index);
  }
}

void Tool::DoTableSection(LazyTableSection section) {
  for (auto table : enumerate(section.sequence, imported_table_count)) {
    Exec("insert into table_ values ({}, {}, {}, {});", table.index,
         table.value->table_type->limits->min,
         OrNull(table.value->table_type->limits->max),
         static_cast<int>(*table.value->table_type->elemtype));
  }
}

void Tool::DoMemorySection(LazyMemorySection section) {
  for (auto memory : enumerate(section.sequence, imported_memory_count)) {
    Exec("insert into memory values ({}, {}, {});", memory.index,
         memory.value->memory_type->limits->min,
         OrNull(memory.value->memory_type->limits->max));
  }
}

void Tool::DoGlobalSection(LazyGlobalSection section) {
  for (auto global : enumerate(section.sequence, imported_global_count)) {
    Exec("insert into global values ({}, {}, {});", global.index,
         static_cast<int>(*global.value->global_type->valtype),
         static_cast<int>(*global.value->global_type->mut));

    InsertConstantExpression(global.value->init, "global_init", global.index);
  }
}

void Tool::DoExportSection(LazyExportSection section) {
  for (auto export_ : enumerate(section.sequence)) {
    Exec("insert into export values ({}, {}, \"{}\", {});", export_.index,
         static_cast<int>(*export_.value->kind), export_.value->name,
         export_.value->index);
  }
}

void Tool::DoStartSection(StartSection section) {
  if (section) {
    Exec("insert into start values ({});", (*section)->func_index);
  }
}

void Tool::DoElementSection(LazyElementSection section) {
  for (auto segment : enumerate(section.sequence)) {
    if (segment.value->table_index) {
      Exec("insert into element values ({}, {}, {});", segment.index,
           static_cast<int>(segment.value->type), *segment.value->table_index);
    } else {
      Exec("insert into element values ({}, {}, null);", segment.index,
           static_cast<int>(segment.value->type));
    }
    if (segment.value->offset) {
      InsertConstantExpression(*segment.value->offset, "element_offset",
                               segment.index);
    }
    if (segment.value->has_indexes()) {
      for (auto item : enumerate(segment.value->indexes().list)) {
        Exec("insert into element_init values ({}, {}, null, {});",
             segment.index, item.index, item.value);
      }
    } else if (segment.value->has_expressions()) {
      for (auto expr : enumerate(segment.value->expressions().list)) {
        auto instr = expr.value->instruction;
        auto opcode_val = static_cast<int>(*instr->opcode);
        switch (instr->opcode) {
          case Opcode::RefNull:
            Exec("insert into element_init values ({}, {}, {}, null);",
                 segment.index, expr.index, opcode_val);
            break;

          case Opcode::RefFunc:
            Exec("insert into element_init values ({}, {}, {}, {});",
                 segment.index, expr.index, opcode_val,
                 instr->index_immediate());
            break;

          default:
            break;
        }
      }
    }
  }
}

void Tool::DoCodeSection(LazyCodeSection section) {
  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(
          db, "insert into instruction values (?1, ?2, ?3, ?4, ?5, ?6);", -1,
          &stmt, nullptr) != SQLITE_OK) {
    print("Error: {}\n", sqlite3_errmsg(db));
    return;
  }

  for (auto code : enumerate(section.sequence, imported_function_count)) {
    Exec("insert into code values ({}, {}, {});", code.index,
         file_offset(code.value->body->data), code.value->body->data.size());

    for (const auto& locals : enumerate(code.value->locals)) {
      Exec("insert into locals values ({}, {}, {}, {});", code.index,
           locals.index, locals.value->count,
           static_cast<int>(*locals.value->type));
    }

    SpanU8 last_data = code.value->body->data;
    Index index = 0;
    auto instrs = ReadExpression(code.value->body, module->context);
    for (auto it = instrs.begin(), end = instrs.end(); it != end;
         ++it, ++index) {
      const auto& instr = *it;
      auto opcode_val = static_cast<int>(*instr->opcode);

      if (instr->has_no_immediate()) {
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_block_type_immediate()) {
        sqlite3_bind_int(stmt, 6,
                         static_cast<int>(*instr->block_type_immediate()));
      } else if (instr->has_index_immediate()) {
        sqlite3_bind_int(stmt, 6, static_cast<int>(instr->index_immediate()));
      } else if (instr->has_call_indirect_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_br_table_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_br_on_exn_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_u8_immediate()) {
        sqlite3_bind_int(stmt, 6, instr->u8_immediate());
      } else if (instr->has_mem_arg_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_s32_immediate()) {
        sqlite3_bind_int(stmt, 6, instr->s32_immediate());
      } else if (instr->has_s64_immediate()) {
        sqlite3_bind_int64(stmt, 6, instr->s64_immediate());
      } else if (instr->has_f32_immediate()) {
        sqlite3_bind_double(stmt, 6, instr->f32_immediate());
      } else if (instr->has_f64_immediate()) {
        sqlite3_bind_double(stmt, 6, instr->f64_immediate());
      } else if (instr->has_v128_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_init_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_copy_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else if (instr->has_shuffle_immediate()) {
        // TODO: immediate
        sqlite3_bind_null(stmt, 6);
      } else {
        assert(false);
      }

      auto offset = file_offset(it.data());
      auto size = it.data().begin() - last_data.begin();
      last_data = it.data();

      sqlite3_bind_int(stmt, 1, code.index);
      sqlite3_bind_int(stmt, 2, index);
      sqlite3_bind_int(stmt, 3, offset);
      sqlite3_bind_int(stmt, 4, size);
      sqlite3_bind_int(stmt, 5, opcode_val);

      int result = sqlite3_step(stmt);
      if (result != SQLITE_DONE) {
        print("Error: {}\n", sqlite3_errmsg(db));
      }

      sqlite3_reset(stmt);
    }
  }

  sqlite3_finalize(stmt);
}

void Tool::DoDataSection(LazyDataSection section) {
  // TODO
}

size_t Tool::file_offset(SpanU8 data) {
  return data.begin() - module->data.begin();
}

void Tool::Repl() {
  std::string input;
  while (true) {
    std::cout << "> ";
    if (!getline(std::cin, input)) {
      break;
    }
    if (input == "help" || input == "h" || input == "" || input == "?") {
      print("type a sql command, or \"quit\" to exit.\n");
    } else if (input == "quit") {
      break;
    } else {
      Exec(input.c_str());
    }
  }
}
