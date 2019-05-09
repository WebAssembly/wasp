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
#include <string>
#include <vector>

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/variant.h"

namespace wasp {
namespace tools {

class ArgParser {
 public:
  using ShortName = char;
  using LongName = string_view;
  using Help = string_view;
  using Metavar = string_view;
  using FlagCallback = std::function<void()>;
  using ParamCallback = std::function<void(string_view)>;

  struct Option {
    // Flag callbacks: `-f`, `--foo`
    explicit Option(LongName, Help, FlagCallback);
    explicit Option(ShortName, LongName, Help, FlagCallback);

    // Param callbacks: `-f 3`, `--foo 3`
    explicit Option(LongName, Metavar, Help, ParamCallback);
    explicit Option(ShortName, LongName, Metavar, Help, ParamCallback);

    // Bare callback: `foo`, `bar`
    explicit Option(Metavar, Help, ParamCallback);

    bool is_flag() const;
    bool is_param() const;
    bool is_bare() const;

    ShortName short_name;
    LongName long_name;
    string_view metavar;
    string_view help;
    variant<FlagCallback, ParamCallback> callback;
  };

  explicit ArgParser(string_view program);

  template <typename... Ts>
  ArgParser& Add(Ts&&... ts) { return AddRaw(Option{std::forward<Ts>(ts)...}); }
  ArgParser& AddRaw(const Option&);

  void Parse(span<string_view>);
  span<string_view> RestOfArgs();

  std::string GetHelpString() const;
  void PrintHelpAndExit(int errcode);

 private:
  static const ShortName kInvalidShortName = '\0';

  optional<Option> FindShortName(ShortName) const;
  optional<Option> FindLongOption(LongName) const;
  optional<Option> FindBare() const;

  string_view program_;
  std::vector<Option> options_;

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
