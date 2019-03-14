[![CircleCI](https://circleci.com/gh/binji/wasp.svg?style=svg)](https://circleci.com/gh/binji/wasp)

# wasp

Wasp is a C++ library designed to make it easy to work with WebAssembly
modules. Unlike tools like [wabt][], it is designed to be used as a library.

It also includes the `wasp` tool, which has the following commands:

* `wasp dump`: Dump the contents of a WebAssembly module
* `wasp callgraph`: Generate a [dot graph][] of the module's callgraph
* `wasp cfg`: Generate a [dot graph][] of a function's [control-flow graph][]
* `wasp dfg`: Generate a [dot graph][] of a function's [data-flow graph][]

## wasp dump examples

Disassemble all functions in a module:

```sh
$ wasp dump -d mod.wasm
```

Display all sections in a module:

```sh
$ wasp dump -h mod.wasm
```

Display the contents of the "import" section:

```sh
$ wasp dump -j import -x mod.wasm
```

## wasp callgraph examples

Write the callgraph as a DOT file to stdout.

```sh
$ wasp callgraph mod.wasm
```

Write the callgraph as a DOT file to `file.dot`.

```sh
$ wasp callgraph mod.wasm -o file.dot
```

## wasp cfg examples

Write the CFG of function 0 as a DOT file to stdout.

```sh
$ wasp cfg -f 0 mod.wasm
```

Write the CFG of function `foo` as a DOT file to `file.dot`.

```sh
$ wasp cfg -f foo mod.wasm -o file.dot
```

## wasp dfg examples

Write the DFG of function 0 as a DOT file to stdout.

```sh
$ wasp dfg -f 0 mod.wasm
```

Write the DFG of function `foo` as a DOT file to `file.dot`.

```sh
$ wasp dfg -f foo mod.wasm -o file.dot
```

[wabt]: https://github.com/WebAssembly/wabt
[dot graph]: http://graphviz.gitlab.io/documentation/
[control-flow graph]: https://en.wikipedia.org/wiki/Control-flow_graph
[data-flow graph]: https://en.wikipedia.org/wiki/Data-flow_analysis
