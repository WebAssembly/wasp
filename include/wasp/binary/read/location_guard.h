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

#ifndef WASP_BINARY_READ_LOCATION_GUARD_H_
#define WASP_BINARY_READ_LOCATION_GUARD_H_

#include "wasp/base/at.h"
#include "wasp/base/span.h"

namespace wasp::binary {

class LocationGuard {
 public:
  explicit LocationGuard(SpanU8* data) : start_{data->begin()} {}

  Location range(SpanU8* end) const { return MakeSpan(start_, end->begin()); }

 private:
  const u8* start_;
};

}  // namespace wasp::binary

#endif // WASP_BINARY_READ_LOCATION_GUARD_H_
