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

#include "wasp/binary/linking_section/read.h"

#include "wasp/base/errors_context_guard.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/binary/linking_section/encoding.h"
#include "wasp/binary/read.h"
#include "wasp/binary/read/context.h"
#include "wasp/binary/read/location_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp::binary {

OptAt<Comdat> Read(SpanU8* data, Context& context, Tag<Comdat>) {
  ErrorsContextGuard error_guard{context.errors, *data, "comdat"};
  LocationGuard guard{data};
  WASP_TRY_READ(name, ReadString(data, context, "name"));
  WASP_TRY_READ(flags, Read<u32>(data, context));
  WASP_TRY_READ(symbols, ReadVector<ComdatSymbol>(data, context,
                                                  "comdat symbols vector"));
  return At{guard.range(data), Comdat{name, flags, std::move(symbols)}};
}

OptAt<ComdatSymbol> Read(SpanU8* data, Context& context, Tag<ComdatSymbol>) {
  ErrorsContextGuard error_guard{context.errors, *data, "comdat symbol"};
  LocationGuard guard{data};
  WASP_TRY_READ(kind, Read<ComdatSymbolKind>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  return At{guard.range(data), ComdatSymbol{kind, index}};
}

OptAt<ComdatSymbolKind> Read(SpanU8* data,
                             Context& context,
                             Tag<ComdatSymbolKind>) {
  ErrorsContextGuard error_guard{context.errors, *data, "comdat symbol kind"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, ComdatSymbolKind, "comdat symbol kind");
  return decoded;
}

OptAt<InitFunction> Read(SpanU8* data, Context& context, Tag<InitFunction>) {
  ErrorsContextGuard error_guard{context.errors, *data, "init function"};
  LocationGuard guard{data};
  WASP_TRY_READ(priority, Read<u32>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "function index"));
  return At{guard.range(data), InitFunction{priority, index}};
}

OptAt<LinkingSubsection> Read(SpanU8* data,
                              Context& context,
                              Tag<LinkingSubsection>) {
  ErrorsContextGuard error_guard{context.errors, *data, "linking subsection"};
  LocationGuard guard{data};
  WASP_TRY_READ(id, Read<LinkingSubsectionId>(data, context));
  WASP_TRY_READ(length, ReadLength(data, context));
  auto bytes = *ReadBytes(data, length, context);
  return At{guard.range(data), LinkingSubsection{id, bytes}};
}

OptAt<LinkingSubsectionId> Read(SpanU8* data,
                                Context& context,
                                Tag<LinkingSubsectionId>) {
  ErrorsContextGuard error_guard{context.errors, *data, "linking subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, LinkingSubsectionId, "linking subsection id");
  return decoded;
}

OptAt<RelocationEntry> Read(SpanU8* data,
                            Context& context,
                            Tag<RelocationEntry>) {
  ErrorsContextGuard error_guard{context.errors, *data, "relocation entry"};
  LocationGuard guard{data};
  WASP_TRY_READ(type, Read<RelocationType>(data, context));
  WASP_TRY_READ(offset, Read<u32>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  switch (type) {
    case RelocationType::MemoryAddressLEB:
    case RelocationType::MemoryAddressSLEB:
    case RelocationType::MemoryAddressI32:
    case RelocationType::FunctionOffsetI32:
    case RelocationType::SectionOffsetI32: {
      WASP_TRY_READ(addend, Read<s32>(data, context));
      return At{guard.range(data),
                RelocationEntry{type, offset, index, addend}};
    }

    default:
      return At{guard.range(data),
                RelocationEntry{type, offset, index, nullopt}};
  }
}

OptAt<RelocationType> Read(SpanU8* data,
                           Context& context,
                           Tag<RelocationType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "relocation type"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, RelocationType, "relocation type");
  return decoded;
}

OptAt<SegmentInfo> Read(SpanU8* data, Context& context, Tag<SegmentInfo>) {
  ErrorsContextGuard error_guard{context.errors, *data, "segment info"};
  LocationGuard guard{data};
  WASP_TRY_READ(name, ReadString(data, context, "name"));
  WASP_TRY_READ(align_log2, Read<u32>(data, context));
  WASP_TRY_READ(flags, Read<u32>(data, context));
  return At{guard.range(data), SegmentInfo{name, align_log2, flags}};
}

OptAt<SymbolInfo> Read(SpanU8* data, Context& context, Tag<SymbolInfo>) {
  ErrorsContextGuard error_guard{context.errors, *data, "symbol info"};
  LocationGuard guard{data};
  WASP_TRY_READ(kind, Read<SymbolInfoKind>(data, context));
  WASP_TRY_READ(encoded_flags, Read<u32>(data, context));
  WASP_TRY_DECODE(flags, encoded_flags, SymbolInfoFlags, "symbol info flags");
  switch (kind) {
    case SymbolInfoKind::Function:
    case SymbolInfoKind::Global:
    case SymbolInfoKind::Event: {
      WASP_TRY_READ(index, ReadIndex(data, context, "index"));
      optional<At<string_view>> name;
      if (flags->undefined == SymbolInfo::Flags::Undefined::No ||
          flags->explicit_name == SymbolInfo::Flags::ExplicitName::Yes) {
        WASP_TRY_READ(name_, ReadString(data, context, "name"));
        name = name_;
      }
      return At{guard.range(data),
                SymbolInfo{flags, SymbolInfo::Base{kind, index, name}}};
    }

    case SymbolInfoKind::Data: {
      WASP_TRY_READ(name, ReadString(data, context, "name"));
      optional<SymbolInfo::Data::Defined> defined;
      if (flags->undefined == SymbolInfo::Flags::Undefined::No) {
        WASP_TRY_READ(index, ReadIndex(data, context, "segment index"));
        WASP_TRY_READ(offset, Read<u32>(data, context));
        WASP_TRY_READ(size, Read<u32>(data, context));
        defined = SymbolInfo::Data::Defined{index, offset, size};
      }
      return At{guard.range(data),
                SymbolInfo{flags, SymbolInfo::Data{name, defined}}};
    }

    case SymbolInfoKind::Section:
      WASP_TRY_READ(section, Read<u32>(data, context));
      return At{guard.range(data),
                SymbolInfo{flags, SymbolInfo::Section{section}}};
  }
  WASP_UNREACHABLE();
}

OptAt<SymbolInfoKind> Read(SpanU8* data,
                           Context& context,
                           Tag<SymbolInfoKind>) {
  ErrorsContextGuard error_guard{context.errors, *data, "symbol info kind"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, SymbolInfoKind, "symbol info kind");
  return decoded;
}

}  // namespace wasp::binary
