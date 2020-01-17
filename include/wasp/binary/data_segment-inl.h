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

namespace wasp {
namespace binary {

inline SegmentType DataSegment::segment_type() const {
  if (desc.index() == 0) {
    return SegmentType::Active;
  } else {
    return SegmentType::Passive;
  }
}

inline bool DataSegment::is_active() const {
  return segment_type() == SegmentType::Active;
}

inline bool DataSegment::is_passive() const {
  return segment_type() == SegmentType::Passive;
}

inline DataSegment::Active& DataSegment::active() {
  return get<Active>(desc);
}

inline const DataSegment::Active& DataSegment::active() const {
  return get<Active>(desc);
}

inline DataSegment::Passive& DataSegment::passive() {
  return get<Passive>(desc);
}

inline const DataSegment::Passive& DataSegment::passive() const {
  return get<Passive>(desc);
}

}  // namespace binary
}  // namespace wasp
