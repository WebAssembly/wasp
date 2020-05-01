//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_TEXT_WRITE_H_
#define WASP_TEXT_WRITE_H_

#include <algorithm>
#include <string>
#include <type_traits>

#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/types.h"
#include "wasp/base/v128.h"
#include "wasp/text/numeric.h"
#include "wasp/text/types.h"

namespace wasp {
namespace text {

// TODO: Rename to Context and put in write namespace?
struct WriteContext {
  void ClearSeparator() { separator = ""; }
  void Space() { separator = " "; }
  void Newline() { separator = indent; }

  void Indent() { indent += "  "; }
  void Dedent() { indent.erase(indent.size() - 2); }

  std::string separator;
  std::string indent = "\n";
  Base base = Base::Decimal;
};

// WriteRaw
template <typename Iterator>
Iterator WriteRaw(WriteContext& context, Iterator out, char value) {
  *out++ = value;
  return out;
}

template <typename Iterator>
Iterator WriteRaw(WriteContext& context, Iterator out, string_view value) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteRaw(WriteContext& context,
                  Iterator out,
                  const std::string& value) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteSeparator(WriteContext& context, Iterator out) {
  out = WriteRaw(context, out, context.separator);
  context.ClearSeparator();
  return out;
}

// WriteFormat
template <typename Iterator, typename T>
Iterator WriteFormat(WriteContext& context,
                     Iterator out,
                     const T& value,
                     string_view format_string = "{}") {
  out = WriteSeparator(context, out);
  out = WriteRaw(context, out, format(format_string, value));
  context.Space();
  return out;
}

template <typename Iterator>
Iterator WriteLpar(WriteContext& context, Iterator out) {
  out = WriteSeparator(context, out);
  out = WriteRaw(context, out, '(');
  return out;
}

template <typename Iterator>
Iterator WriteLpar(WriteContext& context, Iterator out, string_view name) {
  out = WriteLpar(context, out);
  out = WriteRaw(context, out, name);
  context.Space();
  return out;
}

template <typename Iterator>
Iterator WriteRpar(WriteContext& context, Iterator out) {
  context.ClearSeparator();
  out = WriteRaw(context, out, ')');
  context.Space();
  return out;
}

template <typename Iterator, typename SourceIter>
Iterator WriteRange(WriteContext& context,
                     Iterator out,
                     SourceIter begin,
                     SourceIter end) {
  for (auto iter = begin; iter != end; ++iter) {
    out = Write(context, out, *iter);
  }
  return out;
}

template <typename Iterator, typename T>
Iterator WriteVector(WriteContext& context,
                     Iterator out,
                     const std::vector<T>& values) {
  return WriteRange(context, out, values.begin(), values.end());
}

template <typename Iterator, typename T>
Iterator Write(WriteContext& context,
               Iterator out,
               const optional<T>& value_opt) {
  if (value_opt) {
    out = Write(context, out, *value_opt);
  }
  return out;
}

template <typename Iterator, typename T>
Iterator Write(WriteContext& context,
               Iterator out,
               const At<T>& value) {
  return Write(context, out, *value);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, string_view value) {
  out = WriteSeparator(context, out);
  out = WriteRaw(context, out, value);
  context.Space();
  return out;
}

template <typename Iterator, typename T>
Iterator WriteNat(WriteContext& context, Iterator out, T value) {
  return Write(context, out, string_view{NatToStr<T>(value, context.base)});
}

template <typename Iterator,
          typename T,
          typename std::enable_if_t<std::is_integral_v<T>, int> = 0>
Iterator Write(WriteContext& context, Iterator out, T value) {
  return Write(context, out, string_view{IntToStr<T>(value, context.base)});
}

template <typename Iterator,
          typename T,
          typename std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
Iterator Write(WriteContext& context, Iterator out, T value) {
  return Write(context, out, string_view{FloatToStr<T>(value, context.base)});
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, Var value) {
  if (std::holds_alternative<u32>(value)) {
    return WriteNat(context, out, std::get<u32>(value));
  } else {
    return Write(context, out, std::get<string_view>(value));
  }
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const VarList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Text& value) {
  return Write(context, out, value.text);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const TextList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const ValueType& value) {
  return WriteFormat(context, out, value);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ValueTypeList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ValueTypeList& values,
               string_view name) {
  if (!values.empty()) {
    out = WriteLpar(context, out, name);
    out = Write(context, out, values);
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const FunctionType& value) {
  out = Write(context, out, value.params, "param");
  out = Write(context, out, value.results, "result");
  return out;
}

template <typename Iterator>
Iterator WriteTypeUse(WriteContext& context,
                      Iterator out,
                      const OptAt<Var>& value) {
  if (value) {
    out = WriteLpar(context, out, "type");
    out = Write(context, out, value->value());
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const FunctionTypeUse& value) {
  out = WriteTypeUse(context, out, value.type_use);
  out = Write(context, out, *value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const v128& value) {
  u32x4 immediate = value.as<u32x4>();
  out = Write(context, out, "i32x4"_sv);
  for (auto& lane : immediate) {
    out = Write(context, out, lane);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const BlockImmediate& value) {
  out = Write(context, out, value.label);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const BrOnExnImmediate& value) {
  out = Write(context, out, *value.target);
  out = Write(context, out, *value.event);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const BrTableImmediate& value) {
  out = Write(context, out, value.targets);
  out = Write(context, out, *value.default_target);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const CallIndirectImmediate& value) {
  out = Write(context, out, value.table);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const CopyImmediate& value) {
  out = Write(context, out, value.dst);
  out = Write(context, out, value.src);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const InitImmediate& value) {
  // Write dcontext, out, st first, if it exists.
  if (value.dst) {
    out = Write(context, out, value.dst->value());
  }
  out = Write(context, out, *value.segment);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const MemArgImmediate& value) {
  if (value.offset) {
    out = Write(context, out, "offset="_sv);
    context.ClearSeparator();
    out = Write(context, out, value.offset->value());
  }

  if (value.align) {
    out = Write(context, out, "align="_sv);
    context.ClearSeparator();
    out = Write(context, out, value.align->value());
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ShuffleImmediate& value) {
  return WriteRange(context, out, value.begin(), value.end());
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Opcode& value) {
  return WriteFormat(context, out, value);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Instruction& value) {
  out = Write(context, out, *value.opcode);

  switch (value.immediate.index()) {
    case 0: // monostate
      break;

    case 1: // u32
      out = Write(context, out, std::get<At<u32>>(value.immediate));
      break;

    case 2: // u64
      out = Write(context, out, std::get<At<u64>>(value.immediate));
      break;

    case 3: // f32
      out = Write(context, out, std::get<At<f32>>(value.immediate));
      break;

    case 4: // f64
      out = Write(context, out, std::get<At<f64>>(value.immediate));
      break;

    case 5: // v128
      out = Write(context, out, std::get<At<v128>>(value.immediate));
      break;

    case 6: // BlockImmediate
      out = Write(context, out, std::get<At<BlockImmediate>>(value.immediate));
      break;

    case 7: // BrOnExnImmediate
      out =
          Write(context, out, std::get<At<BrOnExnImmediate>>(value.immediate));
      break;

    case 8: // BrTableImmediate
      out =
          Write(context, out, std::get<At<BrTableImmediate>>(value.immediate));
      break;

    case 9: // CallIndirectImmediate
      out = Write(context, out,
                  std::get<At<CallIndirectImmediate>>(value.immediate));
      break;

    case 10: // CopyImmediate
      out = Write(context, out,
                  std::get<At<CopyImmediate>>(value.immediate));
      break;

    case 11: // InitImmediate
      out = Write(context, out, std::get<At<InitImmediate>>(value.immediate));
      break;

    case 12: // MemArgImmediate
      out = Write(context, out, std::get<At<MemArgImmediate>>(value.immediate));
      break;

    case 13: // SelectImmediate
      out = Write(context, out, std::get<At<SelectImmediate>>(value.immediate));
      break;

    case 14: // ShuffleImmediate
      out =
          Write(context, out, std::get<At<ShuffleImmediate>>(value.immediate));
      break;

    case 15: // Var
      out = Write(context, out, std::get<At<Var>>(value.immediate));
      break;
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const InstructionList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator WriteWithNewlines(WriteContext& context,
                           Iterator out,
                           const InstructionList& values) {
  for (auto& value : values) {
    auto opcode = value->opcode;
    if (opcode == Opcode::End || opcode == Opcode::Else ||
        opcode == Opcode::Catch) {
      context.Dedent();
      context.Newline();
    }

    out = Write(context, out, value);

    if (std::holds_alternative<At<BlockImmediate>>(value->immediate) ||
        opcode == Opcode::Else || opcode == Opcode::Catch) {
      context.Indent();
    }
    context.Newline();
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const BoundValueType& value) {
  out = Write(context, out, value.name);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const BoundValueTypeList& values,
               string_view prefix) {
  bool first = true;
  bool prev_has_name = false;
  for (auto& value : values) {
    bool has_name = value->name.has_value();
    if ((has_name || prev_has_name) && !first) {
      out = WriteRpar(context, out);
    }
    if (has_name || prev_has_name || first) {
      out = WriteLpar(context, out, prefix);
    }
    if (has_name) {
      out = Write(context, out, value->name);
    }
    out = Write(context, out, value->type);
    prev_has_name = has_name;
    first = false;
  }
  if (!values.empty()) {
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const BoundFunctionType& value) {
  out = Write(context, out, value.params, "param");
  out = Write(context, out, value.results, "result");
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const TypeEntry& value) {
  out = WriteLpar(context, out, "type");
  out = WriteLpar(context, out, "func");
  out = Write(context, out, value.bind_var);
  out = Write(context, out, value.type);
  out = WriteRpar(context, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const FunctionDesc& value) {
  out = Write(context, out, "func"_sv);
  out = Write(context, out, value.name);
  out = WriteTypeUse(context, out, value.type_use);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const Limits& value) {
  out = Write(context, out, value.min);
  if (value.max) {
    out = Write(context, out, *value.max);
  }
  if (value.shared == Shared::Yes) {
    out = Write(context, out, "shared"_sv);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, ElementType value) {
  return WriteFormat(context, out, value);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const TableType& value) {
  out = Write(context, out, value.limits);
  out = WriteFormat(context, out, value.elemtype);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const TableDesc& value) {
  out = Write(context, out, "table"_sv);
  out = Write(context, out, value.name);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const MemoryType& value) {
  out = Write(context, out, value.limits);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const MemoryDesc& value) {
  out = Write(context, out, "memory"_sv);
  out = Write(context, out, value.name);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const GlobalType& value) {
  if (value.mut == Mutability::Var) {
    out = WriteLpar(context, out, "mut");
  }
  out = Write(context, out, value.valtype);
  if (value.mut == Mutability::Var) {
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const GlobalDesc& value) {
  out = Write(context, out, "global"_sv);
  out = Write(context, out, value.name);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const EventType& value) {
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const EventDesc& value) {
  out = Write(context, out, "event"_sv);
  out = Write(context, out, value.name);
  out = Write(context, out, value.type);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Import& value) {
  out = WriteLpar(context, out, "import"_sv);
  out = Write(context, out, value.module);
  out = Write(context, out, value.name);
  out = WriteLpar(context, out);
  switch (value.desc.index()) {
    case 0:  // FunctionDesc
      out = Write(context, out, std::get<FunctionDesc>(value.desc));
      break;

    case 1:  // TableDesc
      out = Write(context, out, std::get<TableDesc>(value.desc));
      break;

    case 2:  // MemoryDesc
      out = Write(context, out, std::get<MemoryDesc>(value.desc));
      break;

    case 3:  // GlobalDesc
      out = Write(context, out, std::get<GlobalDesc>(value.desc));
      break;

    case 4:  // EventDesc
      out = Write(context, out, std::get<EventDesc>(value.desc));
      break;
  }
  out = WriteRpar(context, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const InlineImport& value) {
  out = WriteLpar(context, out, "import"_sv);
  out = Write(context, out, value.module);
  out = Write(context, out, value.name);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const InlineExport& value) {
  out = WriteLpar(context, out, "export"_sv);
  out = Write(context, out, value.name);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const InlineExportList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Function& value) {
  out = WriteLpar(context, out, "func");

  // Can't write FunctionDesc directly, since inline imports/exports occur
  // between the bindvar and the type use.
  out = Write(context, out, value.desc.name);
  out = Write(context, out, value.exports);

  if (value.import) {
    out = Write(context, out, *value.import);
  }

  out = WriteTypeUse(context, out, value.desc.type_use);
  out = Write(context, out, value.desc.type);

  if (!value.import) {
    context.Indent();
    context.Newline();
    out = Write(context, out, value.locals, "local");
    context.Newline();
    out = WriteWithNewlines(context, out, value.instructions);
    context.Dedent();
  }

  out = WriteRpar(context, out);
  context.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ElementExpressionList& elem_exprs) {
  // Use spaces instead of newlines for element expressions.
  for (auto& elem_expr : elem_exprs) {
    for (auto& instr : elem_expr) {
      // Expressions need to be wrapped in parens.
      out = WriteLpar(context, out);
      out = Write(context, out, instr);
      out = WriteRpar(context, out);
      context.Space();
    }
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ElementListWithExpressions& value) {
  out = Write(context, out, value.elemtype);
  out = Write(context, out, value.list);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, ExternalKind value) {
  return WriteFormat(context, out, value);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ElementListWithVars& value) {
  out = Write(context, out, value.kind);
  out = Write(context, out, value.list);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const ElementList& value) {
  if (std::holds_alternative<ElementListWithVars>(value)) {
    return Write(context, out, std::get<ElementListWithVars>(value));
  } else {
    return Write(context, out, std::get<ElementListWithExpressions>(value));
  }
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Table& value) {
  out = WriteLpar(context, out, "table");

  // Can't write TableDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, out, value.desc.name);
  out = Write(context, out, value.exports);

  if (value.import) {
    out = Write(context, out, *value.import);
    out = Write(context, out, value.desc.type);
  } else if (value.elements) {
    // Don't write the limits, because they are implicitly defined by the
    // element segment length.
    out = Write(context, out, value.desc.type->elemtype);
    out = WriteLpar(context, out, "elem");
    // Only write the list of elements, without the ExternalKind or ElementType.
    if (std::holds_alternative<ElementListWithVars>(*value.elements)) {
      out = Write(context, out,
                  std::get<ElementListWithVars>(*value.elements).list);
    } else {
      out = Write(context, out,
                  std::get<ElementListWithExpressions>(*value.elements).list);
    }
    out = WriteRpar(context, out);
  } else {
    out = Write(context, out, value.desc.type);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Memory& value) {
  out = WriteLpar(context, out, "memory");

  // Can't write MemoryDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, out, value.desc.name);
  out = Write(context, out, value.exports);

  if (value.import) {
    out = Write(context, out, *value.import);
    out = Write(context, out, value.desc.type);
  } else if (value.data) {
    out = WriteLpar(context, out, "data");
    out = Write(context, out, value.data);
    out = WriteRpar(context, out);
  } else {
    out = Write(context, out, value.desc.type);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Global& value) {
  out = WriteLpar(context, out, "global");

  // Can't write GlobalDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, out, value.desc.name);
  out = Write(context, out, value.exports);

  if (value.import) {
    out = Write(context, out, *value.import);
    out = Write(context, out, value.desc.type);
  } else {
    out = Write(context, out, value.desc.type);
    out = Write(context, out, value.init);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Export& value) {
  out = WriteLpar(context, out, "export");
  out = Write(context, out, value.name);
  out = WriteLpar(context, out);
  out = Write(context, out, value.kind);
  out = Write(context, out, value.var);
  out = WriteRpar(context, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Start& value) {
  out = WriteLpar(context, out, "start");
  out = Write(context, out, value.var);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ElementSegment& value) {
  out = WriteLpar(context, out, "elem");
  out = Write(context, out, value.name);
  switch (value.type) {
    case SegmentType::Active:
      if (value.table) {
        out = WriteLpar(context, out, "table");
        out = Write(context, out, *value.table);
        out = WriteRpar(context, out);
      }
      if (value.offset) {
        out = WriteLpar(context, out, "offset");
        out = Write(context, out, *value.offset);
        out = WriteRpar(context, out);
      }

      // When writing a function var list, we can omit the "func" keyword to
      // remain compatible with the MVP text format.
      if (std::holds_alternative<ElementListWithVars>(value.elements)) {
        auto& element_vars = std::get<ElementListWithVars>(value.elements);
        //  The legacy format which omits the external kind cannot be used with
        //  the "table use" or bind_var syntax.
        if (element_vars.kind != ExternalKind::Function || value.table ||
            value.name) {
          out = Write(context, out, element_vars.kind);
        }
        out = Write(context, out, element_vars.list);
      } else {
        out = Write(context, out,
                    std::get<ElementListWithExpressions>(value.elements));
      }
      break;

    case SegmentType::Passive:
      out = Write(context, out, value.elements);
      break;

    case SegmentType::Declared:
      out = Write(context, out, "declare"_sv);
      out = Write(context, out, value.elements);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const DataSegment& value) {
  out = WriteLpar(context, out, "data");
  out = Write(context, out, value.name);
  if (value.type == SegmentType::Active) {
    if (value.memory) {
      out = WriteLpar(context, out, "memory");
      out = Write(context, out, *value.memory);
      out = WriteRpar(context, out);
    }
    if (value.offset) {
      out = WriteLpar(context, out, "offset");
      out = Write(context, out, *value.offset);
      out = WriteRpar(context, out);
    }
  }

  out = Write(context, out, value.data);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Event& value) {
  out = WriteLpar(context, out, "event");

  // Can't write EventDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, out, value.desc.name);
  out = Write(context, out, value.exports);

  if (value.import) {
    out = Write(context, out, *value.import);
    out = Write(context, out, value.desc.type);
  } else {
    out = Write(context, out, value.desc.type);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const ModuleItem& value) {
  switch (value.index()) {
    case 0: // TypeEntry
      out = Write(context, out, std::get<TypeEntry>(value));
      break;

    case 1: // Import
      out = Write(context, out, std::get<Import>(value));
      break;

    case 2:  // Function
      out = Write(context, out, std::get<Function>(value));
      break;

    case 3:  // Table
      out = Write(context, out, std::get<Table>(value));
      break;

    case 4:  // Memory
      out = Write(context, out, std::get<Memory>(value));
      break;

    case 5:  // Global
      out = Write(context, out, std::get<Global>(value));
      break;

    case 6:  // Export
      out = Write(context, out, std::get<Export>(value));
      break;

    case 7:  // Start
      out = Write(context, out, std::get<Start>(value));
      break;

    case 8:  // ElementSegment
      out = Write(context, out, std::get<ElementSegment>(value));
      break;

    case 9:  // DataSegment
      out = Write(context, out, std::get<DataSegment>(value));
      break;

    case 10:  // Event
      out = Write(context, out, std::get<Event>(value));
      break;
  }
  context.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Module& value) {
  return WriteVector(context, out, value);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const ScriptModule& value) {
  out = WriteLpar(context, out, "module");
  out = Write(context, out, value.name);
  switch (value.kind) {
    case ScriptModuleKind::Text:
      context.Indent();
      context.Newline();
      out = Write(context, out, std::get<Module>(value.module));
      context.Dedent();
      break;

    case ScriptModuleKind::Binary:
      out = Write(context, out, "binary"_sv);
      out = Write(context, out, std::get<TextList>(value.module));
      break;

    case ScriptModuleKind::Quote:
      out = Write(context, out, "quote"_sv);
      out = Write(context, out, std::get<TextList>(value.module));
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Const& value) {
  out = WriteLpar(context, out);
  switch (value.index()) {
    case 0: // u32
      out = Write(context, out, Opcode::I32Const);
      out = Write(context, out, std::get<u32>(value));
      break;

    case 1: // u64
      out = Write(context, out, Opcode::I64Const);
      out = Write(context, out, std::get<u64>(value));
      break;

    case 2: // f32
      out = Write(context, out, Opcode::F32Const);
      out = Write(context, out, std::get<f32>(value));
      break;

    case 3: // f64
      out = Write(context, out, Opcode::F64Const);
      out = Write(context, out, std::get<f64>(value));
      break;

    case 4: // v128
      out = Write(context, out, Opcode::V128Const);
      out = Write(context, out, std::get<v128>(value));
      break;

    case 5: // RefNullConst
      out = Write(context, out, Opcode::RefNull);
      break;

    case 6: // RefHostConst
      out = Write(context, out, "ref.host"_sv);
      out = Write(context, out, std::get<RefHostConst>(value).var);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const ConstList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const InvokeAction& value) {
  out = WriteLpar(context, out, "invoke");
  out = Write(context, out, value.module);
  out = Write(context, out, value.name);
  out = Write(context, out, value.consts);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const GetAction& value) {
  out = WriteLpar(context, out, "get");
  out = Write(context, out, value.module);
  out = Write(context, out, value.name);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Action& value) {
  if (std::holds_alternative<InvokeAction>(value)) {
    out = Write(context, out, std::get<InvokeAction>(value));
  } else if (std::holds_alternative<GetAction>(value)) {
    out = Write(context, out, std::get<GetAction>(value));
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ModuleAssertion& value) {
  out = Write(context, out, value.module);
  context.Newline();
  out = Write(context, out, value.message);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ActionAssertion& value) {
  out = Write(context, out, value.action);
  out = Write(context, out, value.message);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const NanKind& value) {
  if (value == NanKind::Arithmetic) {
    return Write(context, out, "nan:arithmetic"_sv);
  } else {
    assert(value == NanKind::Canonical);
    return Write(context, out, "nan:canonical"_sv);
  }
}

template <typename Iterator, typename T>
Iterator Write(WriteContext& context,
               Iterator out,
               const FloatResult<T>& value) {
  if (std::holds_alternative<T>(value)) {
    return Write(context, out, std::get<T>(value));
  } else {
    return Write(context, out, std::get<NanKind>(value));
  }
}

template <typename Iterator, typename T, size_t N>
Iterator Write(WriteContext& context,
               Iterator out,
               const std::array<FloatResult<T>, N>& value) {
  return WriteRange(context, out, value.begin(), value.end());
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const ReturnResult& value) {
  out = WriteLpar(context, out);
  switch (value.index()) {
    case 0: // u32
      out = Write(context, out, Opcode::I32Const);
      out = Write(context, out, std::get<u32>(value));
      break;

    case 1: // u64
      out = Write(context, out, Opcode::I64Const);
      out = Write(context, out, std::get<u64>(value));
      break;

    case 2: // v128
      out = Write(context, out, Opcode::V128Const);
      out = Write(context, out, std::get<v128>(value));
      break;

    case 3: // F32Result
      out = Write(context, out, Opcode::F32Const);
      out = Write(context, out, std::get<F32Result>(value));
      break;

    case 4: // F64Result
      out = Write(context, out, Opcode::F64Const);
      out = Write(context, out, std::get<F64Result>(value));
      break;

    case 5: // F32x4Result
      out = Write(context, out, Opcode::V128Const);
      out = Write(context, out, "f32x4"_sv);
      out = Write(context, out, std::get<F32x4Result>(value));
      break;

    case 6: // F64x2Result
      out = Write(context, out, Opcode::V128Const);
      out = Write(context, out, "f64x2"_sv);
      out = Write(context, out, std::get<F64x2Result>(value));
      break;

    case 7: // RefNullConst
      out = Write(context, out, Opcode::RefNull);
      break;

    case 8: // RefHostConst
      out = Write(context, out, "ref.host"_sv);
      out = WriteNat(context, out, *std::get<RefHostConst>(value).var);
      break;

    case 9: // RefAnyResult
      out = Write(context, out, "ref.any"_sv);
      break;

    case 10: // RefFuncResult
      out = Write(context, out, "ref.func"_sv);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ReturnResultList& values) {
  return WriteVector(context, out, values);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               Iterator out,
               const ReturnAssertion& value) {
  out = Write(context, out, value.action);
  out = Write(context, out, value.results);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Assertion& value) {
  switch (value.kind) {
    case AssertionKind::Malformed:
      out = WriteLpar(context, out, "assert_malformed");
      context.Indent();
      context.Newline();
      out = Write(context, out, std::get<ModuleAssertion>(value.desc));
      context.Dedent();
      break;

    case AssertionKind::Invalid:
      out = WriteLpar(context, out, "assert_invalid");
      context.Indent();
      context.Newline();
      out = Write(context, out, std::get<ModuleAssertion>(value.desc));
      context.Dedent();
      break;

    case AssertionKind::Unlinkable:
      out = WriteLpar(context, out, "assert_unlinkable");
      context.Indent();
      context.Newline();
      out = Write(context, out, std::get<ModuleAssertion>(value.desc));
      context.Dedent();
      break;

    case AssertionKind::ActionTrap:
      out = WriteLpar(context, out, "assert_trap");
      out = Write(context, out, std::get<ActionAssertion>(value.desc));
      break;

    case AssertionKind::Return:
      out = WriteLpar(context, out, "assert_return");
      out = Write(context, out, std::get<ReturnAssertion>(value.desc));
      break;

    case AssertionKind::ModuleTrap:
      out = WriteLpar(context, out, "assert_trap");
      context.Indent();
      context.Newline();
      out = Write(context, out, std::get<ModuleAssertion>(value.desc));
      context.Dedent();
      break;

    case AssertionKind::Exhaustion:
      out = WriteLpar(context, out, "assert_exhaustion");
      out = Write(context, out, std::get<ActionAssertion>(value.desc));
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Register& value) {
  out = WriteLpar(context, out, "register");
  out = Write(context, out, value.name);
  out = Write(context, out, value.module);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Command& value) {
  switch (value.index()) {
    case 0:  // ScriptModule
      out = Write(context, out, std::get<ScriptModule>(value));
      break;

    case 1:  // Register
      out = Write(context, out, std::get<Register>(value));
      break;

    case 2:  // Action
      out = Write(context, out, std::get<Action>(value));
      break;

    case 3:  // Assertion
      out = Write(context, out, std::get<Assertion>(value));
      break;
  }
  context.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, Iterator out, const Script& values) {
  return WriteVector(context, out, values);
}

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_WRITE_H_
