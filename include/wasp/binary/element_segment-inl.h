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

#include "wasp/binary/element_segment.h"

namespace wasp {
namespace binary {

inline SegmentType ElementSegment::segment_type() const {
  if (desc.index() == 0) {
    return SegmentType::Active;
  } else {
    return SegmentType::Passive;
  }
}

inline bool ElementSegment::is_active() const {
  return segment_type() == SegmentType::Active;
}

inline bool ElementSegment::is_passive() const {
  return segment_type() == SegmentType::Passive;
}

inline ElementSegment::Active& ElementSegment::active() {
  return get<Active>(desc);
}

inline const ElementSegment::Active& ElementSegment::active() const {
  return get<Active>(desc);
}

inline ElementSegment::Passive& ElementSegment::passive() {
  return get<Passive>(desc);
}

inline const ElementSegment::Passive& ElementSegment::passive() const {
  return get<Passive>(desc);
}

}  // namespace binary
}  // namespace wasp
