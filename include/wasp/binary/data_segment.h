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

#ifndef WASP_BINARY_DATA_SEGMENT_H_
#define WASP_BINARY_DATA_SEGMENT_H_

#include <functional>

#include "wasp/base/span.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/binary/constant_expression.h"
#include "wasp/binary/segment_type.h"

namespace wasp {
namespace binary {

struct DataSegment {
  struct Active {
    Index memory_index;
    ConstantExpression offset;
  };

  struct Passive {};

  // Active.
  explicit DataSegment(Index memory_index,
                       ConstantExpression offset,
                       SpanU8 init);

  // Passive.
  explicit DataSegment(SpanU8 init);

  SegmentType segment_type() const;
  bool is_active() const;
  bool is_passive() const;

  Active& active();
  const Active& active() const;
  Passive& passive();
  const Passive& passive() const;

  SpanU8 init;
  variant<Active, Passive> desc;
};

bool operator==(const DataSegment&, const DataSegment&);
bool operator!=(const DataSegment&, const DataSegment&);

bool operator==(const DataSegment::Active&, const DataSegment::Active&);
bool operator!=(const DataSegment::Active&, const DataSegment::Active&);

bool operator==(const DataSegment::Passive&, const DataSegment::Passive&);
bool operator!=(const DataSegment::Passive&, const DataSegment::Passive&);

}  // namespace binary
}  // namespace wasp

namespace std {

template <>
struct hash<::wasp::binary::DataSegment> {
  size_t operator()(const ::wasp::binary::DataSegment&) const;
};

template <>
struct hash<::wasp::binary::DataSegment::Active> {
  size_t operator()(const ::wasp::binary::DataSegment::Active&) const;
};

template <>
struct hash<::wasp::binary::DataSegment::Passive> {
  size_t operator()(const ::wasp::binary::DataSegment::Passive&) const;
};

}  // namespace std

#include "wasp/binary/data_segment-inl.h"

#endif // WASP_BINARY_DATA_SEGMENT_H_
