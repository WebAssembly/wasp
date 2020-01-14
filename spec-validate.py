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
import json
import os
import subprocess
import sys

class Error(Exception):
    pass


class Runner(object):

    def __init__(self, spec_json, options):
        self.source_filename = spec_json['source_filename']
        self.source_dir = os.path.dirname(options.filename)
        self.commands = spec_json['commands']
        self.options = options

    def Run(self):
        for command in self.commands:
            self._RunCommand(command)


    def _RunCommand(self, command):
        command_funcs = {
            'module': self._ValidModule,
            'assert_malformed': self._InvalidModule,
            'assert_invalid': self._InvalidModule,
            'assert_unlinkable': self._ValidModule,
            'assert_uninstantiable': self._ValidModule,
        }

        func = command_funcs.get(command['type'])
        if func is not None:
            func(command)

    def _ValidModule(self, command):
        self._DoModule(command, True)

    def _InvalidModule(self, command):
        self._DoModule(command, False)

    def _DoModule(self, command, expected):
        filename = os.path.join(self.source_dir, command['filename'])
        if expected or command['module_type'] == 'binary':
            self._Validate(command, filename, expected)

    def _Validate(self, command, filename, expected):
        proc = subprocess.run([self.options.wasp, 'validate', '-v', filename],
                              capture_output=True)
        actual = proc.returncode == 0
        if actual != expected:
            print('[FAIL] %s:%d:' % (self.source_filename, command['line']))
            print('  wasm: %s' % filename)
            if not expected:
                print('  text: %s' % command['text'])
            print('  expected: %s, got %s' % (expected, actual))
            print('  stdout: %s' % proc.stdout)
            print('  stderr: %s' % proc.stderr)
        else:
            if self.options.verbose:
                print('[ OK ] %s' % filename)


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='verbose output')
    parser.add_argument('-w', '--wasp', default='wasp', help='wasp binary to use')
    parser.add_argument('filename', help='wast2json file to read as input')
    options = parser.parse_args(args)

    with open(options.filename) as json_file:
        spec_json = json.load(json_file)

    Runner(spec_json, options).Run()

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except Error as e:
        sys.stderr.write(str(e) + '\n')
        sys.exit(1)
