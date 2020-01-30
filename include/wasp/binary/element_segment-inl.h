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

inline bool ElementSegment::has_indexes() const {
  return desc.index() == 0;
}

inline bool ElementSegment::has_expressions() const {
  return desc.index() == 1;
}

inline ElementSegment::IndexesInit& ElementSegment::indexes() {
  return get<IndexesInit>(desc);
}

inline const ElementSegment::IndexesInit& ElementSegment::indexes() const {
  return get<IndexesInit>(desc);
}

inline ElementSegment::ExpressionsInit& ElementSegment::expressions() {
  return get<ExpressionsInit>(desc);
}

inline const ElementSegment::ExpressionsInit& ElementSegment::expressions()
    const {
  return get<ExpressionsInit>(desc);
}

}  // namespace binary
}  // namespace wasp
