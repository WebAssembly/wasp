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

#ifndef WASP_BINARY_READ_READ_INSTRUCTION_H_
#define WASP_BINARY_READ_READ_INSTRUCTION_H_

#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/instruction.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_block_type.h"
#include "wasp/binary/read/read_br_table_immediate.h"
#include "wasp/binary/read/read_call_indirect_immediate.h"
#include "wasp/binary/read/read_copy_immediate.h"
#include "wasp/binary/read/read_f32.h"
#include "wasp/binary/read/read_f64.h"
#include "wasp/binary/read/read_index.h"
#include "wasp/binary/read/read_init_immediate.h"
#include "wasp/binary/read/read_mem_arg_immediate.h"
#include "wasp/binary/read/read_opcode.h"
#include "wasp/binary/read/read_reserved.h"
#include "wasp/binary/read/read_s32.h"
#include "wasp/binary/read/read_s64.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<Instruction> Read(SpanU8* data,
                           const Features& features,
                           Errors& errors,
                           Tag<Instruction>) {
  WASP_TRY_READ(opcode, Read<Opcode>(data, features, errors));
  switch (opcode) {
    // No immediates:
    case Opcode::End:
    case Opcode::Unreachable:
    case Opcode::Nop:
    case Opcode::Else:
    case Opcode::Return:
    case Opcode::Drop:
    case Opcode::Select:
    case Opcode::I32Eqz:
    case Opcode::I32Eq:
    case Opcode::I32Ne:
    case Opcode::I32LtS:
    case Opcode::I32LeS:
    case Opcode::I32LtU:
    case Opcode::I32LeU:
    case Opcode::I32GtS:
    case Opcode::I32GeS:
    case Opcode::I32GtU:
    case Opcode::I32GeU:
    case Opcode::I64Eqz:
    case Opcode::I64Eq:
    case Opcode::I64Ne:
    case Opcode::I64LtS:
    case Opcode::I64LeS:
    case Opcode::I64LtU:
    case Opcode::I64LeU:
    case Opcode::I64GtS:
    case Opcode::I64GeS:
    case Opcode::I64GtU:
    case Opcode::I64GeU:
    case Opcode::F32Eq:
    case Opcode::F32Ne:
    case Opcode::F32Lt:
    case Opcode::F32Le:
    case Opcode::F32Gt:
    case Opcode::F32Ge:
    case Opcode::F64Eq:
    case Opcode::F64Ne:
    case Opcode::F64Lt:
    case Opcode::F64Le:
    case Opcode::F64Gt:
    case Opcode::F64Ge:
    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
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
    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
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
    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
    case Opcode::F32Add:
    case Opcode::F32Sub:
    case Opcode::F32Mul:
    case Opcode::F32Div:
    case Opcode::F32Min:
    case Opcode::F32Max:
    case Opcode::F32Copysign:
    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
    case Opcode::F64Add:
    case Opcode::F64Sub:
    case Opcode::F64Mul:
    case Opcode::F64Div:
    case Opcode::F64Min:
    case Opcode::F64Max:
    case Opcode::F64Copysign:
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
    case Opcode::I32TruncSatF32S:
    case Opcode::I32TruncSatF32U:
    case Opcode::I32TruncSatF64S:
    case Opcode::I32TruncSatF64U:
    case Opcode::I64TruncSatF32S:
    case Opcode::I64TruncSatF32U:
    case Opcode::I64TruncSatF64S:
    case Opcode::I64TruncSatF64U:
      return Instruction{Opcode{opcode}};

    // Type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If: {
      WASP_TRY_READ(type, Read<BlockType>(data, features, errors));
      return Instruction{Opcode{opcode}, type};
    }

    // Index immediate.
    case Opcode::Br:
    case Opcode::BrIf:
    case Opcode::Call:
    case Opcode::LocalGet:
    case Opcode::LocalSet:
    case Opcode::LocalTee:
    case Opcode::GlobalGet:
    case Opcode::GlobalSet:
    case Opcode::MemoryDrop:
    case Opcode::TableDrop: {
      WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
      return Instruction{Opcode{opcode}, index};
    }

    // Index* immediates.
    case Opcode::BrTable: {
      WASP_TRY_READ(immediate, Read<BrTableImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, std::move(immediate)};
    }

    // Index, reserved immediates.
    case Opcode::CallIndirect: {
      WASP_TRY_READ(immediate,
                    Read<CallIndirectImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Memarg (alignment, offset) immediates.
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
    case Opcode::I32Store:
    case Opcode::I64Store:
    case Opcode::F32Store:
    case Opcode::F64Store:
    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32: {
      WASP_TRY_READ(memarg, Read<MemArgImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, memarg};
    }

    // Reserved immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow:
    case Opcode::MemoryFill: {
      WASP_TRY_READ(reserved, ReadReserved(data, features, errors));
      return Instruction{Opcode{opcode}, reserved};
    }

    // Const immediates.
    case Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, features, errors),
                            "i32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, features, errors),
                            "i64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, features, errors),
                            "f32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, features, errors),
                            "f64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    // Reserved, Index immediates.
    case Opcode::MemoryInit:
    case Opcode::TableInit: {
      WASP_TRY_READ(immediate, Read<InitImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Reserved, reserved immediates.
    case Opcode::MemoryCopy:
    case Opcode::TableCopy: {
      WASP_TRY_READ(immediate, Read<CopyImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, immediate};
    }
  }
  WASP_UNREACHABLE();
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_INSTRUCTION_H_
