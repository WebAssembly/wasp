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

#include "src/tools/argparser.h"

#include <algorithm>
#include <iostream>

#include "absl/strings/str_format.h"

namespace wasp::tools {

using absl::StrFormat;
using absl::Format;

ArgParser::Option::Option(LongName long_name, Help help, FlagCallback cb)
    : Option{kInvalidShortName, long_name, help, cb} {}

ArgParser::Option::Option(ShortName short_name,
                          LongName long_name,
                          Help help,
                          FlagCallback cb)
    : short_name{short_name}, long_name{long_name}, help{help}, callback{cb} {}

ArgParser::Option::Option(LongName long_name,
                          Metavar metavar,
                          Help help,
                          ParamCallback cb)
    : Option{kInvalidShortName, long_name, metavar, help, cb} {}

ArgParser::Option::Option(ShortName short_name,
                          LongName long_name,
                          Metavar metavar,
                          Help help,
                          ParamCallback cb)
    : short_name{short_name},
      long_name{long_name},
      metavar{metavar},
      help{help},
      callback{cb} {}

ArgParser::Option::Option(Metavar metavar, Help help, ParamCallback cb)
    : Option{kInvalidShortName, {}, metavar, help, cb} {}

bool ArgParser::Option::is_flag() const {
  return holds_alternative<FlagCallback>(callback);
}

bool ArgParser::Option::is_param() const {
  return holds_alternative<ParamCallback>(callback);
}

bool ArgParser::Option::is_bare() const {
  return short_name == kInvalidShortName && long_name.empty();
}

ArgParser::ArgParser(string_view program) : program_{program} {}

ArgParser& ArgParser::AddRaw(const Option& option) {
  options_.push_back(option);
  return *this;
}

ArgParser& ArgParser::AddFeatureFlags(Features& features) {
#define WASP_V(enum_, variable, flag, default_)                         \
  Add("--disable-" flag, "", [&]() { features.disable_##variable(); }); \
  Add("--enable-" flag, "", [&]() { features.enable_##variable(); });
#include "wasp/base/features.def"
#undef WASP_V
  return *this;
}

void ArgParser::Parse(span<const string_view> args) {
  ArgsGuard guard{*this, args};

  for (index_ = 0; index_ < args.size(); ++index_) {
    string_view arg = args[index_];

    auto call = [&](const Option& option) -> bool {
      if (option.is_param() || option.is_bare()) {
        if (index_ + 1 < args.size()) {
          get<ParamCallback>(option.callback)(args[++index_]);
        } else {
          Format(&std::cerr, "Argument `%s` requires parameter\n.", arg);
        }
        return true;
      } else {
        get<FlagCallback>(option.callback)();
        return false;
      }
    };

    if (starts_with(arg, "--")) {
      if (auto option = FindLongOption(arg)) {
        call(*option);
      } else {
        Format(&std::cerr, "Unknown long argument `%s`.\n", arg);
      }
    } else if (arg[0] == '-') {
      optional<char> prev_arg_with_param;
      for (auto c : arg.substr(1)) {
        if (prev_arg_with_param) {
          Format(&std::cerr,
                 "Argument `-%c` ignored since it follows `-%c` which has a "
                 "parameter.\n",
                 c, *prev_arg_with_param);
          continue;
        }

        if (auto option = FindShortName(c)) {
          if (call(*option)) {
            // When a short argument has a parameter, we don't allow other
            // short args to be grouped after it.
            prev_arg_with_param = c;
          }
        } else {
          Format(&std::cerr, "Unknown short argument `-%c`.\n", c);
        }
      }
    } else {
      if (auto option = FindBare()) {
        --index_;  // Back up so call reads arg as the parameter.
        call(*option);
      } else {
        Format(&std::cerr, "Unexpected bare argument `%s`.\n", arg);
      }
    }
  }
}

span<const string_view> ArgParser::RestOfArgs() {
  return args_.subspan(index_ + 1);
}

std::string ArgParser::GetHelpString() const {
  std::string result;
  result += StrFormat("usage: %s", program_);
  if (!options_.empty()) {
    result += " [options]";
  }
  if (auto bare = FindBare()) {
    result += StrFormat(" %s", bare->metavar);
  }

  if (!options_.empty()) {
    auto option_width = [](const Option& option) {
      // + 1 for the space between name and metavar.
      return option.long_name.size() + 1 + option.metavar.size();
    };

    auto width = option_width(
        *(std::max_element(options_.begin(), options_.end(),
                           [&](const Option& lhs, const Option& rhs) {
                             return option_width(lhs) < option_width(rhs);
                           })));
    result += "\n\noptions:\n";
    for (const auto& option: options_) {
      if (option.is_bare()) {
        continue;
      }

      if (option.short_name != kInvalidShortName) {
        result += StrFormat(" -%c, ", option.short_name);
      } else {
        result += "     ";
      }
      result += StrFormat("%-*s  %s\n", width,
                          StrFormat("%s %s", option.long_name, option.metavar),
                          option.help);
    }

    result += "\npositional:\n";
    if (auto option = FindBare()) {
      result +=
          StrFormat(" %-*s      %s\n", width, option->metavar, option->help);
    }
  }
  return result;
}

void ArgParser::PrintHelpAndExit(int errcode) {
  Format(&std::cerr, "%s", GetHelpString());
  exit(errcode);
}

auto ArgParser::FindShortName(ShortName short_name) const -> optional<Option> {
  auto iter = std::find_if(
      options_.begin(), options_.end(),
      [&](const Option& option) { return option.short_name == short_name; });
  return iter != options_.end() ? optional<Option>{*iter} : nullopt;
}

auto ArgParser::FindLongOption(LongName long_name) const -> optional<Option> {
  auto iter = std::find_if(
      options_.begin(), options_.end(),
      [&](const Option& option) { return option.long_name == long_name; });
  return iter != options_.end() ? optional<Option>{*iter} : nullopt;
}

auto ArgParser::FindBare() const -> optional<Option> {
  auto iter =
      std::find_if(options_.begin(), options_.end(),
                   [&](const Option& option) { return option.is_bare(); });
  return iter != options_.end() ? optional<Option>{*iter} : nullopt;
}

ArgParser::ArgsGuard::ArgsGuard(ArgParser& parser, span<const string_view> args)
    : parser{parser} {
  parser.args_ = args;
  parser.index_ = 0;
}

ArgParser::ArgsGuard::~ArgsGuard() {
  parser.args_ = {};
  parser.index_ = 0;
}

}  // namespace wasp::tools
