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

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "src/tools/argparser.h"
#include "src/tools/binary_errors.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/errors_nop.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/str_to_u32.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_module_utils.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/sections.h"

namespace wasp {
namespace tools {
namespace dfg {

using namespace ::wasp::binary;

struct Options {
  Features features;
  string_view function;
  string_view output_filename;
};

using BBID = u32;
using ValueID = u32;
using VarID = u32;

using ValueIDs = std::vector<ValueID>;

constexpr BBID InvalidBBID = ~0;
constexpr ValueID InvalidValueID = ~0;

struct Value {
  bool is_phi() const { return !instr; }

  BBID block;
  optional<Instruction> instr;
  ValueIDs operands;
};

struct Block {
  std::vector<BBID> preds;
  std::map<VarID, ValueID> incomplete_phis;
  size_t value_count;
  bool is_loop_header;
  bool sealed;
};

struct Label {
  Opcode opcode;
  BBID parent;
  BBID br;
  BBID next;
  size_t value_stack_size;
  bool unreachable;
};

struct Tool {
  explicit Tool(SpanU8 data, Options);

  int Run();
  void DoPrepass();
  optional<Index> GetFunctionIndex();
  optional<FunctionType> GetFunctionType(Index);
  optional<Code> GetCode(Index);
  void CalculateDFG(const FunctionType&, Code);
  void DoInstruction(const Instruction&);
  optional<ValueID> GetTrivialPhiOperand(ValueID);
  void RemoveTrivialPhis();
  void WriteDotFile();

  static size_t BlockTypeToValueCount(BlockType);

  void PushLabel(Opcode, BBID br, BBID next);
  Label PopLabel();

  BBID NewBlock(size_t value_count = 0, bool is_loop_header = false);
  void StartBlock(BBID);
  Block& GetBlock(BBID);
  void MarkUnreachable();
  void AddPred(BBID);
  void AddPred(BBID block, BBID pred);
  void Br(Index);
  void Return();

  ValueID NewValue(const Instruction&, size_t operand_count = 0);
  ValueID NewPhi(BBID);
  ValueID Undef();

  size_t GetStackSize() const;
  Value& GetValue(ValueID);
  void CopyValues(size_t count, ValueIDs& out);
  void ForwardValues(const Label&, BBID target);
  void PushValue(ValueID);
  void PushUndefValues(size_t count);
  ValueID PopValue();
  void PopValues(size_t count);
  void BasicInstruction(const Instruction&,
                        size_t operand_count,
                        size_t result_count);

  void WriteVariable(VarID, BBID, ValueID);
  ValueID ReadVariable(VarID, BBID);
  ValueID ReadVariableRecurse(VarID, BBID);
  ValueID AddPhiOperands(VarID, ValueID);
  void SealBlock(BBID);

  BinaryErrors errors;
  Options options;
  LazyModule module;
  std::vector<TypeEntry> type_entries;
  std::vector<Function> functions;
  std::map<string_view, Index> name_to_function;
  Index imported_function_count = 0;
  std::vector<Label> labels;
  std::vector<Block> bbs;
  std::vector<Value> values;
  std::map<std::pair<VarID, BBID>, ValueID> current_def;
  size_t value_stack_size = 0;
  BBID start_bbid = InvalidBBID;
  BBID current_bbid = InvalidBBID;
  ValueID undef = InvalidValueID;
};

int Main(span<string_view> args) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp dfg"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('o', "--output", "<filename>", "write DOT file output to <filename>",
           [&](string_view arg) { options.output_filename = arg; })
      .Add('f', "--function", "<func>", "generate DFG for <func>",
           [&](string_view arg) { options.function = arg; })
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          print(std::cerr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    print(std::cerr, "No filename given.\n");
    parser.PrintHelpAndExit(1);
  }

  if (options.function.empty()) {
    print(std::cerr, "No function given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print(std::cerr, "Error reading file {}.\n", filename);
    return 1;
  }

  SpanU8 data{*optbuf};
  Tool tool{data, options};
  int result = tool.Run();
  tool.errors.PrintTo(std::cerr);
  return result;
}

Tool::Tool(SpanU8 data, Options options)
    : errors{data},
      options{options},
      module{ReadModule(data, options.features, errors)} {}

int Tool::Run() {
  DoPrepass();
  auto index_opt = GetFunctionIndex();
  if (!index_opt) {
    print(std::cerr, "Unknown function {}\n", options.function);
    return 1;
  }
  auto ft_opt = GetFunctionType(*index_opt);
  auto code_opt = GetCode(*index_opt);
  if (!ft_opt || !code_opt) {
    print(std::cerr, "Invalid function index {}\n", *index_opt);
    return 1;
  }
  CalculateDFG(*ft_opt, *code_opt);
  RemoveTrivialPhis();
  WriteDotFile();
  return 0;
}

void Tool::DoPrepass() {
  ForEachFunctionName(module, [this](const IndexNamePair& pair) {
    name_to_function.insert(std::make_pair(pair.second, pair.first));
  });

  for (auto section : module.sections) {
    if (section->is_known()) {
      auto known = section->known();
      switch (known->id) {
        case SectionId::Type: {
          auto seq = ReadTypeSection(known, module.context).sequence;
          std::copy(seq.begin(), seq.end(), std::back_inserter(type_entries));
          break;
        }

        case SectionId::Import:
          for (auto import :
               ReadImportSection(known, module.context).sequence) {
            if (import->kind() == ExternalKind::Function) {
              functions.push_back(Function{import->index()});
            }
          }
          imported_function_count = functions.size();
          break;

        case SectionId::Function: {
          auto seq = ReadFunctionSection(known, module.context).sequence;
          std::copy(seq.begin(), seq.end(), std::back_inserter(functions));
          break;
        }

        default:
          break;
      }
    }
  }
}

// TODO(binji): share code with cfg.cc
optional<Index> Tool::GetFunctionIndex() {
  // Search by name.
  auto iter = name_to_function.find(options.function);
  if (iter != name_to_function.end()) {
    return iter->second;
  }

  // Try to convert the string to an integer and search by index.
  return StrToU32(options.function);
}

optional<FunctionType> Tool::GetFunctionType(Index func_index) {
  if (func_index >= functions.size()) {
    return nullopt;
  }
  Index type_index = functions[func_index].type_index;
  if (type_index >= type_entries.size()) {
    return nullopt;
  }
  return type_entries[type_index].type;
}

optional<Code> Tool::GetCode(Index find_index) {
  for (auto section : module.sections) {
    if (section->is_known()) {
      auto known = section->known();
      if (known->id == SectionId::Code) {
        auto section = ReadCodeSection(known, module.context);
        for (auto code : enumerate(section.sequence, imported_function_count)) {
          if (code.index == find_index) {
            return code.value;
          }
        }
      }
    }
  }
  return nullopt;
}

void Tool::CalculateDFG(const FunctionType& type, Code code) {
  // Create start block and label.
  start_bbid = NewBlock();
  StartBlock(start_bbid);

  // Add params.
  for (u32 i = 0; i < type.param_types.size(); ++i) {
    PushValue(NewValue(Instruction{MakeAt(Opcode::LocalGet), MakeAt(i)}));
  }

  // Add locals, initialized to 0.
  for (const auto& locals : code.locals) {
    for (Index i = 0; i < locals->count; ++i) {
      switch (locals->type) {
        case ValueType::I32:
          PushValue(
              NewValue(Instruction{MakeAt(Opcode::I32Const), MakeAt(s32{0})}));
          break;

        case ValueType::F32:
          PushValue(
              NewValue(Instruction{MakeAt(Opcode::F32Const), MakeAt(f32{0})}));
          break;

        case ValueType::I64:
          PushValue(
              NewValue(Instruction{MakeAt(Opcode::I64Const), MakeAt(s64{0})}));
          break;

        case ValueType::F64:
          PushValue(
              NewValue(Instruction{MakeAt(Opcode::F64Const), MakeAt(f64{0})}));
          break;

        case ValueType::V128:
          PushValue(
              NewValue(Instruction{MakeAt(Opcode::V128Const), MakeAt(v128{})}));
          break;

        case ValueType::Externref:
        case ValueType::Funcref:
        case ValueType::Exnref:
          PushValue(NewValue(Instruction{MakeAt(Opcode::RefNull)}));
          break;
      }
    }
  }

  // Push a dummy label so the return value is still accessible after the final
  // `end` instruction is reached.
  PushLabel(Opcode::End, InvalidBBID, InvalidBBID);

  BBID return_bbid = NewBlock(type.result_types.size());
  PushUndefValues(type.result_types.size());
  PushLabel(Opcode::Return, return_bbid, return_bbid);

  for (const auto& instr : ReadExpression(code.body, module.context)) {
    DoInstruction(instr);
  }

  BasicInstruction(Instruction{MakeAt(Opcode::Return)},
                   type.result_types.size(), 0);
  SealBlock(return_bbid);
}

void Tool::DoInstruction(const Instruction& instr) {
  switch (instr.opcode) {
    case Opcode::Unreachable:
      MarkUnreachable();
      break;

    case Opcode::Block: {
      auto value_count = BlockTypeToValueCount(instr.block_type_immediate());
      auto next = NewBlock(value_count);
      PushUndefValues(value_count);
      PushLabel(instr.opcode, next, next);
      break;
    }

    case Opcode::Loop: {
      auto value_count = BlockTypeToValueCount(instr.block_type_immediate());
      auto loop = NewBlock(0, true);
      auto next = NewBlock(value_count);
      AddPred(loop);
      PushUndefValues(value_count);
      PushLabel(instr.opcode, loop, next);
      StartBlock(loop);
      break;
    }

    case Opcode::If: {
      auto value_count = BlockTypeToValueCount(instr.block_type_immediate());
      auto true_ = NewBlock();
      auto next = NewBlock(value_count);
      AddPred(true_);
      BasicInstruction(instr, 1, 0);
      PushUndefValues(value_count);
      PushLabel(instr.opcode, next, next);
      StartBlock(true_);
      break;
    }

    case Opcode::Else: {
      auto top = PopLabel();
      auto false_ = NewBlock();
      AddPred(false_, top.parent);
      PushLabel(instr.opcode, top.next, top.next);
      StartBlock(false_);
      break;
    }

    case Opcode::End: {
      auto top = PopLabel();
      if (top.opcode == Opcode::If) {
        AddPred(top.next, top.parent);
      }
      StartBlock(top.next);
      break;
    }

    case Opcode::Br:
      Br(instr.index_immediate());
      MarkUnreachable();
      break;

    case Opcode::BrIf: {
      BasicInstruction(instr, 1, 0);
      Br(instr.index_immediate());
      auto next = NewBlock();
      AddPred(next);
      StartBlock(next);
      break;
    }

    case Opcode::BrTable: {
      const auto& immediate = instr.br_table_immediate();
      BasicInstruction(instr, 1, 0);
      for (const auto& target : immediate->targets) {
        Br(target);
      }
      Br(immediate->default_target);
      MarkUnreachable();
      break;
    }

    case Opcode::Return:
      Return();
      MarkUnreachable();
      break;

    case Opcode::Call:
    case Opcode::ReturnCall: {
      auto func_type_opt = GetFunctionType(instr.index_immediate());
      if (func_type_opt) {
        BasicInstruction(instr, func_type_opt->param_types.size(),
                         func_type_opt->result_types.size());
      } else {
        print(std::cerr, "*** Error: `{}` with unknown function\n", instr);
      }
      if (instr.opcode == Opcode::ReturnCall) {
        Return();
        MarkUnreachable();
      }
      break;
    }

    case Opcode::CallIndirect:
    case Opcode::ReturnCallIndirect: {
      auto type_index = instr.call_indirect_immediate()->index;
      if (type_index < type_entries.size()) {
        const auto& func_type = type_entries[type_index].type;
        BasicInstruction(instr, func_type->param_types.size() + 1,
                         func_type->result_types.size());
      } else {
        print(std::cerr, "*** Error: `{}` with unknown type\n", instr);
      }
      if (instr.opcode == Opcode::ReturnCallIndirect) {
        Return();
        MarkUnreachable();
      }
      break;
    }

    case Opcode::LocalGet:
      PushValue(ReadVariable(instr.index_immediate(), current_bbid));
      break;

    case Opcode::LocalSet:
      WriteVariable(instr.index_immediate(), current_bbid, PopValue());
      break;

    case Opcode::LocalTee: {
      auto value = PopValue();
      WriteVariable(instr.index_immediate(), current_bbid, value);
      PushValue(value);
      break;
    }

    case Opcode::Nop:
    case Opcode::DataDrop:
    case Opcode::ElemDrop:
      break;

    case Opcode::Drop:
    case Opcode::GlobalSet:
      BasicInstruction(instr, 1, 0);
      break;

    case Opcode::Select:
    case Opcode::SelectT:
    case Opcode::V128BitSelect:
    case Opcode::MemoryAtomicWait32:
    case Opcode::MemoryAtomicWait64:
    case Opcode::I32AtomicRmwCmpxchg:
    case Opcode::I64AtomicRmwCmpxchg:
    case Opcode::I32AtomicRmw8CmpxchgU:
    case Opcode::I32AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw8CmpxchgU:
    case Opcode::I64AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw32CmpxchgU:
      BasicInstruction(instr, 3, 1);
      break;

    case Opcode::GlobalGet:
    case Opcode::MemorySize:
    case Opcode::I32Const:
    case Opcode::I64Const:
    case Opcode::F32Const:
    case Opcode::F64Const:
    case Opcode::RefNull:
    case Opcode::RefFunc:
    case Opcode::V128Const:
      BasicInstruction(instr, 0, 1);
      break;

    case Opcode::I32Load:
    case Opcode::I64Load:
    case Opcode::F32Load:
    case Opcode::F64Load:
    case Opcode::I32Load8S:
    case Opcode::I32Load8U:
    case Opcode::I32Load16S:
    case Opcode::I32Load16U:
    case Opcode::I64Load8S:
    case Opcode::I64Load8U:
    case Opcode::I64Load16S:
    case Opcode::I64Load16U:
    case Opcode::I64Load32S:
    case Opcode::I64Load32U:
    case Opcode::MemoryGrow:
    case Opcode::I32Eqz:
    case Opcode::I64Eqz:
    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
    case Opcode::I32WrapI64:
    case Opcode::I32TruncF32S:
    case Opcode::I32TruncF32U:
    case Opcode::I32TruncF64S:
    case Opcode::I32TruncF64U:
    case Opcode::I64ExtendI32S:
    case Opcode::I64ExtendI32U:
    case Opcode::I64TruncF32S:
    case Opcode::I64TruncF32U:
    case Opcode::I64TruncF64S:
    case Opcode::I64TruncF64U:
    case Opcode::F32ConvertI32S:
    case Opcode::F32ConvertI32U:
    case Opcode::F32ConvertI64S:
    case Opcode::F32ConvertI64U:
    case Opcode::F32DemoteF64:
    case Opcode::F64ConvertI32S:
    case Opcode::F64ConvertI32U:
    case Opcode::F64ConvertI64S:
    case Opcode::F64ConvertI64U:
    case Opcode::F64PromoteF32:
    case Opcode::I32ReinterpretF32:
    case Opcode::I64ReinterpretF64:
    case Opcode::F32ReinterpretI32:
    case Opcode::F64ReinterpretI64:
    case Opcode::I32Extend8S:
    case Opcode::I32Extend16S:
    case Opcode::I64Extend8S:
    case Opcode::I64Extend16S:
    case Opcode::I64Extend32S:
    case Opcode::RefIsNull:
    case Opcode::I32TruncSatF32S:
    case Opcode::I32TruncSatF32U:
    case Opcode::I32TruncSatF64S:
    case Opcode::I32TruncSatF64U:
    case Opcode::I64TruncSatF32S:
    case Opcode::I64TruncSatF32U:
    case Opcode::I64TruncSatF64S:
    case Opcode::I64TruncSatF64U:
    case Opcode::V128Load:
    case Opcode::I8X16Splat:
    case Opcode::I8X16ExtractLaneS:
    case Opcode::I8X16ExtractLaneU:
    case Opcode::I16X8Splat:
    case Opcode::I16X8ExtractLaneS:
    case Opcode::I16X8ExtractLaneU:
    case Opcode::I32X4Splat:
    case Opcode::I32X4ExtractLane:
    case Opcode::I64X2Splat:
    case Opcode::I64X2ExtractLane:
    case Opcode::F32X4Splat:
    case Opcode::F32X4ExtractLane:
    case Opcode::F64X2Splat:
    case Opcode::F64X2ExtractLane:
    case Opcode::V128Not:
    case Opcode::I8X16Neg:
    case Opcode::I8X16AnyTrue:
    case Opcode::I8X16AllTrue:
    case Opcode::I16X8Neg:
    case Opcode::I16X8AnyTrue:
    case Opcode::I16X8AllTrue:
    case Opcode::I32X4Neg:
    case Opcode::I32X4AnyTrue:
    case Opcode::I32X4AllTrue:
    case Opcode::I64X2Neg:
    case Opcode::F32X4Abs:
    case Opcode::F32X4Neg:
    case Opcode::F32X4Sqrt:
    case Opcode::F64X2Abs:
    case Opcode::F64X2Neg:
    case Opcode::F64X2Sqrt:
    case Opcode::I32X4TruncSatF32X4S:
    case Opcode::I32X4TruncSatF32X4U:
    case Opcode::F32X4ConvertI32X4S:
    case Opcode::F32X4ConvertI32X4U:
    case Opcode::V8X16LoadSplat:
    case Opcode::V16X8LoadSplat:
    case Opcode::V32X4LoadSplat:
    case Opcode::V64X2LoadSplat:
    case Opcode::I16X8WidenLowI8X16S:
    case Opcode::I16X8WidenHighI8X16S:
    case Opcode::I16X8WidenLowI8X16U:
    case Opcode::I16X8WidenHighI8X16U:
    case Opcode::I32X4WidenLowI16X8S:
    case Opcode::I32X4WidenHighI16X8S:
    case Opcode::I32X4WidenLowI16X8U:
    case Opcode::I32X4WidenHighI16X8U:
    case Opcode::I16X8Load8X8S:
    case Opcode::I16X8Load8X8U:
    case Opcode::I32X4Load16X4S:
    case Opcode::I32X4Load16X4U:
    case Opcode::I64X2Load32X2S:
    case Opcode::I64X2Load32X2U:
    case Opcode::I8X16Abs:
    case Opcode::I16X8Abs:
    case Opcode::I32X4Abs:
    case Opcode::I32AtomicLoad:
    case Opcode::I64AtomicLoad:
    case Opcode::I32AtomicLoad8U:
    case Opcode::I32AtomicLoad16U:
    case Opcode::I64AtomicLoad8U:
    case Opcode::I64AtomicLoad16U:
    case Opcode::I64AtomicLoad32U:
      BasicInstruction(instr, 1, 1);
      break;

    case Opcode::I32Store:
    case Opcode::I64Store:
    case Opcode::F32Store:
    case Opcode::F64Store:
    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32:
    case Opcode::V128Store:
    case Opcode::I32AtomicStore:
    case Opcode::I64AtomicStore:
    case Opcode::I32AtomicStore8:
    case Opcode::I32AtomicStore16:
    case Opcode::I64AtomicStore8:
    case Opcode::I64AtomicStore16:
    case Opcode::I64AtomicStore32:
      BasicInstruction(instr, 2, 0);
      break;

    case Opcode::I32Eq:
    case Opcode::I32Ne:
    case Opcode::I32LtS:
    case Opcode::I32LtU:
    case Opcode::I32GtS:
    case Opcode::I32GtU:
    case Opcode::I32LeS:
    case Opcode::I32LeU:
    case Opcode::I32GeS:
    case Opcode::I32GeU:
    case Opcode::I64Eq:
    case Opcode::I64Ne:
    case Opcode::I64LtS:
    case Opcode::I64LtU:
    case Opcode::I64GtS:
    case Opcode::I64GtU:
    case Opcode::I64LeS:
    case Opcode::I64LeU:
    case Opcode::I64GeS:
    case Opcode::I64GeU:
    case Opcode::F32Eq:
    case Opcode::F32Ne:
    case Opcode::F32Lt:
    case Opcode::F32Gt:
    case Opcode::F32Le:
    case Opcode::F32Ge:
    case Opcode::F64Eq:
    case Opcode::F64Ne:
    case Opcode::F64Lt:
    case Opcode::F64Gt:
    case Opcode::F64Le:
    case Opcode::F64Ge:
    case Opcode::I32Add:
    case Opcode::I32Sub:
    case Opcode::I32Mul:
    case Opcode::I32DivS:
    case Opcode::I32DivU:
    case Opcode::I32RemS:
    case Opcode::I32RemU:
    case Opcode::I32And:
    case Opcode::I32Or:
    case Opcode::I32Xor:
    case Opcode::I32Shl:
    case Opcode::I32ShrS:
    case Opcode::I32ShrU:
    case Opcode::I32Rotl:
    case Opcode::I32Rotr:
    case Opcode::I64Add:
    case Opcode::I64Sub:
    case Opcode::I64Mul:
    case Opcode::I64DivS:
    case Opcode::I64DivU:
    case Opcode::I64RemS:
    case Opcode::I64RemU:
    case Opcode::I64And:
    case Opcode::I64Or:
    case Opcode::I64Xor:
    case Opcode::I64Shl:
    case Opcode::I64ShrS:
    case Opcode::I64ShrU:
    case Opcode::I64Rotl:
    case Opcode::I64Rotr:
    case Opcode::F32Add:
    case Opcode::F32Sub:
    case Opcode::F32Mul:
    case Opcode::F32Div:
    case Opcode::F32Min:
    case Opcode::F32Max:
    case Opcode::F32Copysign:
    case Opcode::F64Add:
    case Opcode::F64Sub:
    case Opcode::F64Mul:
    case Opcode::F64Div:
    case Opcode::F64Min:
    case Opcode::F64Max:
    case Opcode::F64Copysign:
    case Opcode::V8X16Shuffle:
    case Opcode::V8X16Swizzle:
    case Opcode::I8X16ReplaceLane:
    case Opcode::I16X8ReplaceLane:
    case Opcode::I32X4ReplaceLane:
    case Opcode::I64X2ReplaceLane:
    case Opcode::F32X4ReplaceLane:
    case Opcode::F64X2ReplaceLane:
    case Opcode::I8X16Eq:
    case Opcode::I8X16Ne:
    case Opcode::I8X16LtS:
    case Opcode::I8X16LtU:
    case Opcode::I8X16GtS:
    case Opcode::I8X16GtU:
    case Opcode::I8X16LeS:
    case Opcode::I8X16LeU:
    case Opcode::I8X16GeS:
    case Opcode::I8X16GeU:
    case Opcode::I16X8Eq:
    case Opcode::I16X8Ne:
    case Opcode::I16X8LtS:
    case Opcode::I16X8LtU:
    case Opcode::I16X8GtS:
    case Opcode::I16X8GtU:
    case Opcode::I16X8LeS:
    case Opcode::I16X8LeU:
    case Opcode::I16X8GeS:
    case Opcode::I16X8GeU:
    case Opcode::I32X4Eq:
    case Opcode::I32X4Ne:
    case Opcode::I32X4LtS:
    case Opcode::I32X4LtU:
    case Opcode::I32X4GtS:
    case Opcode::I32X4GtU:
    case Opcode::I32X4LeS:
    case Opcode::I32X4LeU:
    case Opcode::I32X4GeS:
    case Opcode::I32X4GeU:
    case Opcode::F32X4Eq:
    case Opcode::F32X4Ne:
    case Opcode::F32X4Lt:
    case Opcode::F32X4Gt:
    case Opcode::F32X4Le:
    case Opcode::F32X4Ge:
    case Opcode::F64X2Eq:
    case Opcode::F64X2Ne:
    case Opcode::F64X2Lt:
    case Opcode::F64X2Gt:
    case Opcode::F64X2Le:
    case Opcode::F64X2Ge:
    case Opcode::V128And:
    case Opcode::V128Or:
    case Opcode::V128Xor:
    case Opcode::I8X16Shl:
    case Opcode::I8X16ShrS:
    case Opcode::I8X16ShrU:
    case Opcode::I8X16Add:
    case Opcode::I8X16AddSaturateS:
    case Opcode::I8X16AddSaturateU:
    case Opcode::I8X16Sub:
    case Opcode::I8X16SubSaturateS:
    case Opcode::I8X16SubSaturateU:
    case Opcode::I8X16MinS:
    case Opcode::I8X16MinU:
    case Opcode::I8X16MaxS:
    case Opcode::I8X16MaxU:
    case Opcode::I16X8Shl:
    case Opcode::I16X8ShrS:
    case Opcode::I16X8ShrU:
    case Opcode::I16X8Add:
    case Opcode::I16X8AddSaturateS:
    case Opcode::I16X8AddSaturateU:
    case Opcode::I16X8Sub:
    case Opcode::I16X8SubSaturateS:
    case Opcode::I16X8SubSaturateU:
    case Opcode::I16X8Mul:
    case Opcode::I16X8MinS:
    case Opcode::I16X8MinU:
    case Opcode::I16X8MaxS:
    case Opcode::I16X8MaxU:
    case Opcode::I32X4Shl:
    case Opcode::I32X4ShrS:
    case Opcode::I32X4ShrU:
    case Opcode::I32X4Add:
    case Opcode::I32X4Sub:
    case Opcode::I32X4Mul:
    case Opcode::I32X4MinS:
    case Opcode::I32X4MinU:
    case Opcode::I32X4MaxS:
    case Opcode::I32X4MaxU:
    case Opcode::I64X2Shl:
    case Opcode::I64X2ShrS:
    case Opcode::I64X2ShrU:
    case Opcode::I64X2Add:
    case Opcode::I64X2Sub:
    case Opcode::I64X2Mul:
    case Opcode::F32X4Add:
    case Opcode::F32X4Sub:
    case Opcode::F32X4Mul:
    case Opcode::F32X4Div:
    case Opcode::F32X4Min:
    case Opcode::F32X4Max:
    case Opcode::F64X2Add:
    case Opcode::F64X2Sub:
    case Opcode::F64X2Mul:
    case Opcode::F64X2Div:
    case Opcode::F64X2Min:
    case Opcode::F64X2Max:
    case Opcode::I8X16NarrowI16X8S:
    case Opcode::I8X16NarrowI16X8U:
    case Opcode::I16X8NarrowI32X4S:
    case Opcode::I16X8NarrowI32X4U:
    case Opcode::V128Andnot:
    case Opcode::I8X16AvgrU:
    case Opcode::I16X8AvgrU:
    case Opcode::MemoryAtomicNotify:
    case Opcode::I32AtomicRmwAdd:
    case Opcode::I64AtomicRmwAdd:
    case Opcode::I32AtomicRmw8AddU:
    case Opcode::I32AtomicRmw16AddU:
    case Opcode::I64AtomicRmw8AddU:
    case Opcode::I64AtomicRmw16AddU:
    case Opcode::I64AtomicRmw32AddU:
    case Opcode::I32AtomicRmwSub:
    case Opcode::I64AtomicRmwSub:
    case Opcode::I32AtomicRmw8SubU:
    case Opcode::I32AtomicRmw16SubU:
    case Opcode::I64AtomicRmw8SubU:
    case Opcode::I64AtomicRmw16SubU:
    case Opcode::I64AtomicRmw32SubU:
    case Opcode::I32AtomicRmwAnd:
    case Opcode::I64AtomicRmwAnd:
    case Opcode::I32AtomicRmw8AndU:
    case Opcode::I32AtomicRmw16AndU:
    case Opcode::I64AtomicRmw8AndU:
    case Opcode::I64AtomicRmw16AndU:
    case Opcode::I64AtomicRmw32AndU:
    case Opcode::I32AtomicRmwOr:
    case Opcode::I64AtomicRmwOr:
    case Opcode::I32AtomicRmw8OrU:
    case Opcode::I32AtomicRmw16OrU:
    case Opcode::I64AtomicRmw8OrU:
    case Opcode::I64AtomicRmw16OrU:
    case Opcode::I64AtomicRmw32OrU:
    case Opcode::I32AtomicRmwXor:
    case Opcode::I64AtomicRmwXor:
    case Opcode::I32AtomicRmw8XorU:
    case Opcode::I32AtomicRmw16XorU:
    case Opcode::I64AtomicRmw8XorU:
    case Opcode::I64AtomicRmw16XorU:
    case Opcode::I64AtomicRmw32XorU:
    case Opcode::I32AtomicRmwXchg:
    case Opcode::I64AtomicRmwXchg:
    case Opcode::I32AtomicRmw8XchgU:
    case Opcode::I32AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw8XchgU:
    case Opcode::I64AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw32XchgU:
      BasicInstruction(instr, 2, 1);
      break;

    case Opcode::MemoryInit:
    case Opcode::MemoryCopy:
    case Opcode::MemoryFill:
    case Opcode::TableInit:
    case Opcode::TableCopy:
      BasicInstruction(instr, 3, 0);
      break;

    case Opcode::Try:
    case Opcode::Catch:
    case Opcode::Throw:
    case Opcode::Rethrow:
    case Opcode::BrOnExn:
      // TODO
      assert(false);
      break;

    case Opcode::TableGet:
    case Opcode::TableSet:
    case Opcode::TableGrow:
    case Opcode::TableSize:
    case Opcode::TableFill:
      // TODO
      assert(false);
      break;
  }
}

// static
size_t Tool::BlockTypeToValueCount(BlockType type) {
  return type == BlockType::Void ? 0 : 1;
}

void Tool::PushLabel(Opcode opcode, BBID br, BBID next) {
  labels.push_back({opcode, current_bbid, br, next, value_stack_size, false});
}

Label Tool::PopLabel() {
  auto top = labels.back();
  if (!top.unreachable) {
    ForwardValues(top, top.next);
    AddPred(top.next);
  }
  labels.pop_back();
  value_stack_size = top.value_stack_size;
  if (top.opcode == Opcode::Loop) {
    SealBlock(top.br);
  }
  return top;
}

BBID Tool::NewBlock(size_t value_count, bool is_loop_header) {
  bbs.push_back(Block{{}, {}, value_count, is_loop_header, false});
  return static_cast<BBID>(bbs.size() - 1);
}

void Tool::StartBlock(BBID bbid) {
  if (current_bbid != InvalidBBID &&
      !GetBlock(current_bbid).is_loop_header) {
    SealBlock(current_bbid);
  }
  current_bbid = bbid;
}

Block& Tool::GetBlock(BBID bbid) {
  assert(bbid < bbs.size());
  return bbs[bbid];
}

void Tool::MarkUnreachable() {
  assert(!labels.empty());
  labels.back().unreachable = true;
  StartBlock(NewBlock());
}

void Tool::AddPred(BBID bbid) {
  AddPred(bbid, current_bbid);
}

void Tool::AddPred(BBID bbid, BBID pred) {
  if (bbid != InvalidBBID) {
    GetBlock(bbid).preds.emplace_back(pred);
  }
}

void Tool::Br(Index index) {
  if (index < labels.size()) {
    const auto& label = labels[labels.size() - index - 1];
    auto target = label.br;
    AddPred(target);
    ForwardValues(label, target);
  } else {
    print(std::cerr, "*** Error: Invalid br depth {}\n", index);
  }
}

void Tool::Return() {
  Br(labels.size() - 2);
}

ValueID Tool::NewValue(const Instruction& instr, size_t operand_count) {
  values.push_back(Value{current_bbid, instr, {}});
  auto value = static_cast<ValueID>(values.size() - 1);
  ValueIDs operands;
  CopyValues(operand_count, operands);
  GetValue(value).operands = std::move(operands);
  return value;
}

ValueID Tool::NewPhi(BBID bbid) {
  values.push_back(Value{bbid, nullopt, {}});
  auto value = static_cast<ValueID>(values.size() - 1);
  return value;
}

ValueID Tool::Undef() {
  if (undef == InvalidValueID) {
    undef = NewValue(Instruction{MakeAt(Opcode::Unreachable)});
  }
  return undef;
}

size_t Tool::GetStackSize() const {
  if (labels.empty()) {
    return 0;
  }
  return value_stack_size - labels.back().value_stack_size;
}

Value& Tool::GetValue(ValueID id) {
  assert(id < values.size());
  return values[id];
}

void Tool::CopyValues(size_t count, ValueIDs& out) {
  if (count <= GetStackSize()) {
    out.resize(count);
    for (size_t i = 0; i < count; ++i) {
      out[i] = ReadVariable(value_stack_size - count + i, current_bbid);
    }
  } else {
    print(std::cerr, "*** Error: CopyValues({}) past bottom of stack {}\n",
          count, GetStackSize());
  }
}

void Tool::ForwardValues(const Label& label, BBID bbid) {
  const auto& block = GetBlock(bbid);
  for (size_t i = 0; i < block.value_count; ++i) {
    auto value =
        ReadVariable(value_stack_size - block.value_count + i, current_bbid);
    WriteVariable(label.value_stack_size - block.value_count + i, current_bbid,
                  value);
  }
}

void Tool::PushValue(ValueID value) {
  WriteVariable(value_stack_size++, current_bbid, value);
}

void Tool::PushUndefValues(size_t count) {
  auto undef = Undef();
  for (size_t i = 0; i < count; ++i) {
    WriteVariable(value_stack_size++, current_bbid, undef);
  }
}

ValueID Tool::PopValue() {
  if (GetStackSize() == 0) {
    return InvalidValueID;
  }

  return ReadVariable(--value_stack_size, current_bbid);
}

void Tool::PopValues(size_t count) {
  auto stack_size = GetStackSize();
  if (count <= stack_size) {
    value_stack_size -= count;
  } else {
    print(std::cerr, "*** Error: PopValues({}) past bottom of stack {}\n",
          count, GetStackSize());
    value_stack_size -= stack_size;
  }
}

void Tool::BasicInstruction(const Instruction& instr,
                            size_t operand_count,
                            size_t result_count) {
  assert(result_count <= 1);  // TODO support multi-value
  auto value = NewValue(instr, operand_count);
  PopValues(operand_count);
  if (result_count > 0) {
    PushValue(value);
  }
}

// Implementation of SSA construction from
// https://pp.info.uni-karlsruhe.de/uploads/publikationen/braun13cc.pdf

void Tool::WriteVariable(VarID var, BBID bbid, ValueID value) {
  assert(value != InvalidValueID);
  current_def[std::make_pair(var, bbid)] = value;
}

ValueID Tool::ReadVariable(VarID var, BBID bbid) {
  auto iter = current_def.find(std::make_pair(var, bbid));
  if (iter != current_def.end()) {
    return iter->second;
  }
  auto value = ReadVariableRecurse(var, bbid);
  return value;
}

ValueID Tool::ReadVariableRecurse(VarID var, BBID bbid) {
  auto& block = GetBlock(bbid);
  ValueID value;
  if (!block.sealed) {
    // Incomplete CFG.
    value = NewPhi(bbid);
    block.incomplete_phis[var] = value;
  } else {
    if (block.preds.size() == 1) {
      // Optimize the common case of one predecessor: no phi needed.
      value = ReadVariable(var, block.preds[0]);
    } else {
      // Break potential cycles with a operandless phi.
      value = NewPhi(bbid);
      WriteVariable(var, bbid, value);
      value = AddPhiOperands(var, value);
      assert(value != InvalidValueID);
    }
  }
  WriteVariable(var, bbid, value);
  assert(value != InvalidValueID);
  return value;
}

ValueID Tool::AddPhiOperands(VarID var, ValueID phi) {
  // Determine operands from predecessors.
  auto preds = GetBlock(GetValue(phi).block).preds;
  for (BBID pred: preds) {
    auto value = ReadVariable(var, pred);
    GetValue(phi).operands.push_back(value);
  }
  return phi;
}

void Tool::SealBlock(BBID bbid) {
  auto& block = GetBlock(bbid);
  assert(!block.sealed);
  for (auto pair : block.incomplete_phis) {
    AddPhiOperands(pair.first, pair.second);
  }
  block.incomplete_phis.clear();
  block.sealed = true;
}

optional<ValueID> Tool::GetTrivialPhiOperand(ValueID vid) {
  auto& value = GetValue(vid);
  if (value.is_phi()) {
    optional<ValueID> same;
    for (auto op : value.operands) {
      if (op == same || op == vid) {
        continue;  // Unique value of self-reference.
      }
      if (same) {
        return nullopt;  // The phi merges at least two values: not trivial.
      }
      same = op;
    }
    // This phi is trivial and can be replaced by same.
    return same;
  }
  return nullopt;
}

void Tool::RemoveTrivialPhis() {
  using UserMap = std::multimap<ValueID, ValueID>;
  std::set<ValueID> trivial_phis;
  UserMap users;
  ValueIDs phis;
  ValueID vid = 0;
  for (const auto& value : values) {
    if (value.is_phi()) {
      phis.emplace_back(vid);
    }
    for (auto op : value.operands) {
      users.emplace(op, vid);
    }
    ++vid;
  }

  while (!phis.empty()) {
    ValueIDs new_phis;
    for (auto phi : phis) {
      auto same = GetTrivialPhiOperand(phi);
      if (same) {
        auto& phi_value = GetValue(phi);
        // For all operands of this phi: replace any users that point to this
        // phi with same.
        for (auto op : phi_value.operands) {
          auto range = users.equal_range(op);
          for (auto it = range.first; it != range.second; ++it) {
            if (it->second == phi) {
              it->second = *same;
            }
          }
        }
        phi_value.operands.clear();

        // For all users of this phi: replace any operands that point to this
        // phi with same.
        std::vector<UserMap::value_type> new_user_pairs;
        auto range = users.equal_range(phi);
        for (auto it = range.first; it != range.second; ++it) {
          auto user = it->second;
          if (user != phi) {
            auto& operands = GetValue(user).operands;
            std::replace(operands.begin(), operands.end(), phi, *same);
            new_user_pairs.emplace_back(*same, user);
            if (GetValue(user).is_phi()) {
              // Perform another pass with any users that may have become
              // trivial by the removal of phi.
              new_phis.push_back(user);
            }
          }
        }
        users.erase(range.first, range.second);
        users.insert(new_user_pairs.begin(), new_user_pairs.end());

        trivial_phis.insert(phi);
      }
    }
    auto&& is_trivial = [&](ValueID x) { return trivial_phis.count(x) != 0; };
    auto new_end = std::remove_if(new_phis.begin(), new_phis.end(), is_trivial);
    std::sort(new_phis.begin(), new_end);
    new_end = std::unique(new_phis.begin(), new_end);
    new_phis.erase(new_end, new_phis.end());
    std::swap(phis, new_phis);
  }
}

namespace {

std::string EscapeString(string_view s) {
  std::string result;
  for (char c: s) {
    switch (c) {
      case '{':
      case '}':
        result += '\\';
        // Fallthrough.

      default:
        result += c;
        break;
    }
  }
  return result;
}

}  // namespace

void Tool::WriteDotFile() {
  std::ofstream fstream;
  std::ostream* stream = &std::cout;
  if (!options.output_filename.empty()) {
    fstream = std::ofstream{std::string{options.output_filename}};
    if (fstream) {
      stream = &fstream;
    }
  }

  // Collect values for each basic block, and users of each value.
  std::multimap<ValueID, ValueID> users;
  std::map<BBID, std::vector<ValueID>> blocks;
  ValueID vid = 0;
  for (const auto& value : values) {
    blocks[value.block].push_back(vid);
    for (auto op : value.operands) {
      users.emplace(op, vid);
    }
    vid++;
  }

  auto&& should_display = [&](ValueID vid) {
    auto& value = GetValue(vid);
    return !value.operands.empty() || users.count(vid) != 0;
  };

  std::vector<std::pair<ValueID, ValueID>> interblock_edges;

  print(*stream, "strict digraph {{\n");

  // Write clusters.
  for (const auto& pair : blocks) {
    auto bbid = pair.first;
    const auto& block_vids = pair.second;

    print(*stream, "  subgraph cluster_{} {{\n", bbid);

    // Write nodes.
    for (const auto& vid : block_vids) {
      if (should_display(vid)) {
        const auto& value = GetValue(vid);
        print(*stream, "    {} [shape=box;label=\"", vid);
        if (value.is_phi()) {
          print(*stream, "phi");
        } else {
          print(*stream, "{}", EscapeString(format("{}", *value.instr)));
        }
        print(*stream, "\"]\n");
      }
    }

    // Write edges that exist completely within this block.
    for (const auto& vid : block_vids) {
      const auto& value = GetValue(vid);
      for (const auto& op : value.operands) {
        if (GetValue(op).block == bbid) {
          print(*stream, "    {} -> {}\n", op, vid);
        } else {
          interblock_edges.push_back(std::make_pair(op, vid));
        }
      }
    }

    print(*stream, "  }}\n");
  }

  // Write edges that span between blocks.
  for (const auto& pair : interblock_edges) {
    print(*stream, "  {} -> {}\n", pair.first, pair.second);
  }

  print(*stream, "}}\n");
  stream->flush();
}

}  // namespace dfg
}  // namespace tools
}  // namespace wasp
