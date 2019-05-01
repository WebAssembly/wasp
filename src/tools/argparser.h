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

#ifndef WASP_TOOLS_ARGPARSER_H_
#define WASP_TOOLS_ARGPARSER_H_

#include <functional>
#include <map>

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/variant.h"

namespace wasp {
namespace tools {

class ArgParser {
 public:
  using FlagCallback = std::function<void()>;
  using ParamCallback = std::function<void(string_view)>;
  using Callback = variant<monostate, FlagCallback, ParamCallback>;

  // Flag callbacks: `-f`, `--foo`
  ArgParser& Add(string_view long_arg, FlagCallback);
  ArgParser& Add(char short_arg, FlagCallback);
  ArgParser& Add(char short_arg, string_view long_arg, FlagCallback);

  // Param callbacks: `-f 3`, `--foo 3`
  ArgParser& Add(string_view long_arg, ParamCallback);
  ArgParser& Add(char short_arg, ParamCallback);
  ArgParser& Add(char short_arg, string_view long_arg, ParamCallback);

  // Bare callback: `foo`, `bar`
  ArgParser& Add(ParamCallback);
  void Parse(span<string_view>);

  span<string_view> RestOfArgs();

 private:
  std::map<char, Callback> short_map_;
  std::map<string_view, Callback> long_map_;
  Callback bare_;

  struct ArgsGuard {
    explicit ArgsGuard(ArgParser& parser, span<string_view> args);
    ~ArgsGuard();

    ArgParser& parser;
  };

  span<string_view> args_;
  int index_ = 0;
};

}  // namespace tools
}  // namespace wasp

#endif  // WASP_TOOLS_ARGPARSER_H_
