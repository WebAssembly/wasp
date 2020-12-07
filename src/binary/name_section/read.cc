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

#include "wasp/binary/name_section/read.h"

#include "wasp/base/errors_context_guard.h"
#include "wasp/base/formatters.h"
#include "wasp/binary/name_section/encoding.h"
#include "wasp/binary/read.h"
#include "wasp/binary/read/location_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_ctx.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp::binary {

OptAt<IndirectNameAssoc> Read(SpanU8* data,
                              ReadCtx& ctx,
                              Tag<IndirectNameAssoc>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "indirect name assoc"};
  LocationGuard guard{data};
  WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
  WASP_TRY_READ(name_map, ReadVector<NameAssoc>(data, ctx, "name map"));
  return At{guard.range(data), IndirectNameAssoc{index, std::move(name_map)}};
}

OptAt<NameAssoc> Read(SpanU8* data, ReadCtx& ctx, Tag<NameAssoc>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "name assoc"};
  LocationGuard guard{data};
  WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
  WASP_TRY_READ(name, ReadString(data, ctx, "name"));
  return At{guard.range(data), NameAssoc{index, name}};
}

OptAt<NameSubsection> Read(SpanU8* data, ReadCtx& ctx, Tag<NameSubsection>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "name subsection"};
  LocationGuard guard{data};
  WASP_TRY_READ(id, Read<NameSubsectionId>(data, ctx));
  WASP_TRY_READ(length, ReadLength(data, ctx));
  WASP_TRY_READ(bytes, ReadBytes(data, length, ctx));
  return At{guard.range(data), NameSubsection{id, *bytes}};
}

OptAt<NameSubsectionId> Read(SpanU8* data,
                             ReadCtx& ctx,
                             Tag<NameSubsectionId>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "name subsection id"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, Read<u8>(data, ctx));
  WASP_TRY_DECODE(decoded, val, NameSubsectionId, "name subsection id");
  return At{guard.range(data), decoded};
}

}  // namespace wasp::binary
