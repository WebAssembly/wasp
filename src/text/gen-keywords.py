#!/usr/bin/env python3
#
# Copyright 2020 WebAssembly Community Group participants
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import collections
import os
import sys

SOURCE_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_INPUT = os.path.join(SOURCE_DIR, 'keywords.txt')
DEFAULT_OUTPUT = os.path.join(SOURCE_DIR, 'keywords-inl.cc')

class Error(Exception):
    pass


class Runner(object):

    def __init__(self, filename, options):
        self.keys = []
        self.values = {}
        self.options = options

        with open(filename) as input_file:
            for line in input_file:
                line = line.strip()
                if line.startswith('#'):
                    continue

                parts = line.split(', ')
                self.keys.append(parts[0])
                self.values[parts[0]] = parts[1:]

    def Run(self):
        if self.options.output:
            with open(self.options.output, 'w') as output_file:
                self.output_file = output_file
                self.Emit(self.keys)
        else:
            self.output_file = sys.stdout
            self.Emit(self.keys)
        self.output_file = None

    def DistinctChars(self, keys, index):
        distinct = collections.defaultdict(list)
        for key in keys:
            assert(index <= len(key))
            if index < len(key):
                distinct[key[index]].append(key)
            else:
                distinct['\0'].append(key)
        return len(distinct), index, distinct

    def Emit(self, keys, indent=''):
        min_length = min(len(key) for key in keys)
        _, index, distinct = max(self.DistinctChars(keys, i)
                                 for i in range(min_length + 1))
        self.Print(indent, 'switch (PeekChar(data, {})) {{'.format(index))
        has_default = False
        for char in sorted(distinct.keys()):
            subkeys = distinct[char]
            if char == '\0':
                has_default = True
                self.Print(indent, "  default:", end='')
            else:
                self.Print(indent, "  case '{}':".format(char), end='')

            if len(subkeys) == 1:
                key = subkeys[0]
                values = tuple(self.values[key])
                if values[0] in ('TokenType::AlignEqNat',
                                 'TokenType::OffsetEqNat'):
                  self.Print('', ' return LexNameEqNum(data, "{}", {});'.format(
                             key, values[0]))
                elif values == ('TokenType::Float', 'LiteralKind::HexNan'):
                  self.Print('', ' return LexNan(data);')
                else:
                  self.Print('', ' return LexKeyword(data, "{}", {});'.format(
                             key, ', '.join(values)))
            else:
                self.Print()
                self.Emit(subkeys, indent + '    ')

        if not has_default:
            self.Print(indent, '  default: break;')
        self.Print(indent, '}')
        self.Print(indent, 'break;')

    def Print(self, indent='', line='', end=None):
        print('{}{}'.format(indent, line), end=end, file=self.output_file)


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--filename', help='file to read as input',
                        default=DEFAULT_INPUT)
    parser.add_argument('-o', '--output', help='file to write as output',
                        default=DEFAULT_OUTPUT)
    options = parser.parse_args(args)

    Runner(options.filename, options).Run()


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except Error as e:
        sys.stderr.write(str(e) + '\n')
        sys.exit(1)
