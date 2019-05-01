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

#include <cstdio>

#include "src/tools/argparser.h"

#include "wasp/base/format.h"

namespace wasp {
namespace tools {

ArgParser& ArgParser::Add(string_view long_arg, FlagCallback cb) {
  return Add('\0', long_arg, cb);
}

ArgParser& ArgParser::Add(char short_arg, FlagCallback cb) {
  return Add(short_arg, {}, cb);
}

ArgParser& ArgParser::Add(char short_arg,
                          string_view long_arg,
                          FlagCallback cb) {
  if (short_arg != '\0') {
    short_map_.emplace(short_arg, cb);
  }
  if (!long_arg.empty()) {
    long_map_.emplace(long_arg, cb);
  }
  return *this;
}

ArgParser& ArgParser::Add(string_view long_arg, ParamCallback cb) {
  return Add('\0', long_arg, cb);
}

ArgParser& ArgParser::Add(char short_arg, ParamCallback cb) {
  return Add(short_arg, {}, cb);
}

ArgParser& ArgParser::Add(char short_arg,
                          string_view long_arg,
                          ParamCallback cb) {
  if (short_arg != '\0') {
    short_map_.emplace(short_arg, cb);
  }
  if (!long_arg.empty()) {
    long_map_.emplace(long_arg, cb);
  }
  return *this;
}

ArgParser& ArgParser::Add(ParamCallback cb) {
  bare_ = cb;
  return *this;
}

void ArgParser::Parse(span<string_view> args) {
  ArgsGuard guard{*this, args};

  for (index_ = 0; index_ < args.size(); ++index_) {
    string_view arg = args[index_];

    auto call = [&](Callback cb) -> bool {
      if (holds_alternative<ParamCallback>(cb)) {
        if (index_ + 1 < args.size()) {
          get<ParamCallback>(cb)(args[++index_]);
        } else {
          print(stderr, "Argument `{}` requires parameter\n.", arg);
        }
        return true;
      } else {
        get<FlagCallback>(cb)();
        return false;
      }
    };

    if (arg.starts_with("--")) {
      auto iter = long_map_.find(arg);
      if (iter == long_map_.end()) {
        print(stderr, "Unknown long argument `{}`.\n", arg);
      } else {
        call(iter->second);
      }
    } else if (arg[0] == '-') {
      optional<char> prev_arg_with_param;
      for (auto c : arg.substr(1)) {
        if (prev_arg_with_param) {
          print(stderr,
                "Argument `-{}` ignored since it follows `-{}` which has a "
                "parameter.\n",
                c, *prev_arg_with_param);
          continue;
        }

        auto iter = short_map_.find(c);
        if (iter == short_map_.end()) {
          print(stderr, "Unknown short argument `-{}`.\n", c);
        } else if (call(iter->second)) {
          // When a short argument has a parameter, we don't allow other
          // short args to be grouped after it.
          prev_arg_with_param = c;
        }
      }
    } else {
      if (!holds_alternative<monostate>(bare_)) {
        --index_;  // Back up so call reads arg as the parameter.
        call(bare_);
      } else {
        print(stderr, "Unexpected bare argument `{}`.\n", arg);
      }
    }
  }
}

span<string_view> ArgParser::RestOfArgs() {
  return args_.subspan(index_ + 1);
}

ArgParser::ArgsGuard::ArgsGuard(ArgParser& parser, span<string_view> args)
    : parser{parser} {
  parser.args_ = args;
  parser.index_ = 0;
}

ArgParser::ArgsGuard::~ArgsGuard() {
  parser.args_ = {};
  parser.index_ = 0;
}

}  // namespace tools
}  // namespace wasp
