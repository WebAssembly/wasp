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

#ifndef WASP_BINARY_INSTRUCTION_H_
#define WASP_BINARY_INSTRUCTION_H_

#include "wasp/base/types.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"
#include "wasp/binary/block_type.h"
#include "wasp/binary/br_table_immediate.h"
#include "wasp/binary/br_on_exn_immediate.h"
#include "wasp/binary/call_indirect_immediate.h"
#include "wasp/binary/copy_immediate.h"
#include "wasp/binary/empty_immediate.h"
#include "wasp/binary/init_immediate.h"
#include "wasp/binary/mem_arg_immediate.h"
#include "wasp/binary/opcode.h"
#include "wasp/binary/shuffle_immediate.h"

namespace wasp {
namespace binary {

struct Instruction {
  explicit Instruction(Opcode opcode);
  explicit Instruction(Opcode opcode, EmptyImmediate);
  explicit Instruction(Opcode opcode, BlockType);
  explicit Instruction(Opcode opcode, Index);
  explicit Instruction(Opcode opcode, CallIndirectImmediate);
  explicit Instruction(Opcode opcode, BrOnExnImmediate);
  explicit Instruction(Opcode opcode, BrTableImmediate);
  explicit Instruction(Opcode opcode, u8);
  explicit Instruction(Opcode opcode, MemArgImmediate);
  explicit Instruction(Opcode opcode, s32);
  explicit Instruction(Opcode opcode, s64);
  explicit Instruction(Opcode opcode, f32);
  explicit Instruction(Opcode opcode, f64);
  explicit Instruction(Opcode opcode, v128);
  explicit Instruction(Opcode opcode, InitImmediate);
  explicit Instruction(Opcode opcode, CopyImmediate);
  explicit Instruction(Opcode opcode, ShuffleImmediate);

  bool has_empty_immediate() const;
  bool has_block_type_immediate() const;
  bool has_index_immediate() const;
  bool has_call_indirect_immediate() const;
  bool has_br_table_immediate() const;
  bool has_br_on_exn_immediate() const;
  bool has_u8_immediate() const;
  bool has_mem_arg_immediate() const;
  bool has_s32_immediate() const;
  bool has_s64_immediate() const;
  bool has_f32_immediate() const;
  bool has_f64_immediate() const;
  bool has_v128_immediate() const;
  bool has_init_immediate() const;
  bool has_copy_immediate() const;
  bool has_shuffle_immediate() const;

  EmptyImmediate& empty_immediate();
  const EmptyImmediate& empty_immediate() const;
  BlockType& block_type_immediate();
  const BlockType& block_type_immediate() const;
  Index& index_immediate();
  const Index& index_immediate() const;
  CallIndirectImmediate& call_indirect_immediate();
  const CallIndirectImmediate& call_indirect_immediate() const;
  BrTableImmediate& br_table_immediate();
  const BrTableImmediate& br_table_immediate() const;
  BrOnExnImmediate& br_on_exn_immediate();
  const BrOnExnImmediate& br_on_exn_immediate() const;
  u8& u8_immediate();
  const u8& u8_immediate() const;
  MemArgImmediate& mem_arg_immediate();
  const MemArgImmediate& mem_arg_immediate() const;
  s32& s32_immediate();
  const s32& s32_immediate() const;
  s64& s64_immediate();
  const s64& s64_immediate() const;
  f32& f32_immediate();
  const f32& f32_immediate() const;
  f64& f64_immediate();
  const f64& f64_immediate() const;
  v128& v128_immediate();
  const v128& v128_immediate() const;
  InitImmediate& init_immediate();
  const InitImmediate& init_immediate() const;
  CopyImmediate& copy_immediate();
  const CopyImmediate& copy_immediate() const;
  ShuffleImmediate& shuffle_immediate();
  const ShuffleImmediate& shuffle_immediate() const;

  Opcode opcode;
  variant<EmptyImmediate,
          BlockType,
          Index,
          CallIndirectImmediate,
          BrTableImmediate,
          BrOnExnImmediate,
          u8,
          MemArgImmediate,
          s32,
          s64,
          f32,
          f64,
          v128,
          InitImmediate,
          CopyImmediate,
          ShuffleImmediate>
      immediate;
};

bool operator==(const Instruction&, const Instruction&);
bool operator!=(const Instruction&, const Instruction&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/instruction-inl.h"

#endif  // WASP_BINARY_INSTRUCTION_H_
