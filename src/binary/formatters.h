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

#ifndef WASP_BINARY_FORMATTERS_H_
#define WASP_BINARY_FORMATTERS_H_

#include "src/base/format.h"
#include "src/binary/types.h"

namespace fmt {

template <>
struct formatter<::wasp::binary::ValueType> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::binary::ValueType, Ctx&);
};

template <>
struct formatter<::wasp::binary::BlockType> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::binary::BlockType, Ctx&);
};

template <>
struct formatter<::wasp::binary::ElementType> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::binary::ElementType, Ctx&);
};

template <>
struct formatter<::wasp::binary::ExternalKind> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::binary::ExternalKind, Ctx&);
};

template <>
struct formatter<::wasp::binary::Mutability> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::binary::Mutability, Ctx&);
};

template <>
struct formatter<::wasp::binary::SectionId> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::binary::SectionId, Ctx&);
};

template <>
struct formatter<::wasp::binary::MemArg> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::MemArg&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Limits> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Limits&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Locals> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Locals&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Section> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Section&, Ctx&);
};

template <>
struct formatter<::wasp::binary::KnownSection> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::KnownSection&, Ctx&);
};

template <>
struct formatter<::wasp::binary::CustomSection> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::CustomSection&, Ctx&);
};

template <>
struct formatter<::wasp::binary::TypeEntry> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::TypeEntry&, Ctx&);
};

template <>
struct formatter<::wasp::binary::FunctionType> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::FunctionType&, Ctx&);
};

template <>
struct formatter<::wasp::binary::TableType> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::TableType&, Ctx&);
};

template <>
struct formatter<::wasp::binary::MemoryType> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::MemoryType&, Ctx&);
};

template <>
struct formatter<::wasp::binary::GlobalType> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::GlobalType&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Import> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Import&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Export> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Export&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Expression> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Expression&, Ctx&);
};

template <>
struct formatter<::wasp::binary::ConstantExpression> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::ConstantExpression&,
                                Ctx&);
};

template <>
struct formatter<::wasp::binary::Opcode> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Opcode&, Ctx&);
};

template <>
struct formatter<::wasp::binary::CallIndirectImmediate> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::CallIndirectImmediate&,
                                Ctx&);
};

template <>
struct formatter<::wasp::binary::BrTableImmediate> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::BrTableImmediate&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Instruction> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Instruction&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Function> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Function&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Table> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Table&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Memory> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Memory&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Global> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Global&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Start> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Start&, Ctx&);
};

template <>
struct formatter<::wasp::binary::ElementSegment> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::ElementSegment&, Ctx&);
};

template <>
struct formatter<::wasp::binary::Code> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::Code&, Ctx&);
};

template <>
struct formatter<::wasp::binary::DataSegment> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) {
    return ctx.begin();
  }
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::binary::DataSegment&, Ctx&);
};

}  // namespace fmt

#include "src/binary/formatters-inl.h"

#endif  // WASP_BINARY_FORMATTERS_H_
