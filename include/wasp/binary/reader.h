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

#ifndef WASP_BINARY_READER_H_
#define WASP_BINARY_READER_H_

#include <vector>

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp {
namespace binary {

template <typename T, typename Errors>
optional<T> Read(SpanU8* data, Errors&);

template <typename Errors>
optional<SpanU8> ReadBytes(SpanU8* data, SpanU8::index_type N, Errors&);

template <typename T, typename Errors>
optional<T> ReadVarInt(SpanU8* data, Errors&, string_view desc);

template <typename Errors>
optional<Index> ReadIndex(SpanU8* data, Errors&, string_view desc);

template <typename Errors>
optional<Index> ReadCount(SpanU8* data, Errors&);

template <typename Errors>
optional<string_view> ReadString(SpanU8* data, Errors&, string_view desc);

template <typename T, typename Errors>
optional<std::vector<T>> ReadVector(SpanU8* data, Errors&, string_view desc);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/reader-inl.h"

#endif  // WASP_BINARY_READER_H_
