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

#ifndef WASP_BINARY_ELEMENT_SEGMENT_H_
#define WASP_BINARY_ELEMENT_SEGMENT_H_

#include <vector>

#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/binary/constant_expression.h"
#include "wasp/binary/element_type.h"
#include "wasp/binary/segment_type.h"

namespace wasp {
namespace binary {

struct ElementSegment {
  struct Active {
    Index table_index;
    ConstantExpression offset;
  };

  struct Passive {
    ElementType element_type;
  };

  // Active.
  explicit ElementSegment(Index table_index,
                          ConstantExpression offset,
                          const std::vector<Index>& init);

  // Passive.
  explicit ElementSegment(ElementType element_type,
                          const std::vector<Index>& init);

  SegmentType segment_type() const;
  bool is_active() const;
  bool is_passive() const;

  Active& active();
  const Active& active() const;
  Passive& passive();
  const Passive& passive() const;

  std::vector<Index> init;
  variant<Active, Passive> desc;
};

bool operator==(const ElementSegment&, const ElementSegment&);
bool operator!=(const ElementSegment&, const ElementSegment&);

bool operator==(const ElementSegment::Active&, const ElementSegment::Active&);
bool operator!=(const ElementSegment::Active&, const ElementSegment::Active&);

bool operator==(const ElementSegment::Passive&, const ElementSegment::Passive&);
bool operator!=(const ElementSegment::Passive&, const ElementSegment::Passive&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/element_segment-inl.h"

#endif // WASP_BINARY_ELEMENT_SEGMENT_H_
