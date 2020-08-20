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

#ifndef WASP_TEXT_READ_LOCATION_GUARD_H_
#define WASP_TEXT_READ_LOCATION_GUARD_H_

#include "wasp/text/read/tokenizer.h"

namespace wasp::text {

class LocationGuard {
 public:
  explicit LocationGuard(Tokenizer& tokenizer)
      : tokenizer_{tokenizer}, start_{tokenizer.Peek().loc.begin()} {}

  Location loc() const {
    auto* end = tokenizer_.Previous().loc.end();
    return Location{start_, start_ <= end ? end : start_};
  }

 private:
  Tokenizer& tokenizer_;
  const u8* start_;
};

}  // namespace wasp::text

#endif  // WASP_TEXT_READ_LOCATION_GUARD_H_
