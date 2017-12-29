## Spin-2 FS Lib

### Introduction

Quite often, in systems programming environments, it is useful to easily
manipulate  filesystem paths. This library provides a C++17 `Path` class that
attempts to make traversing a directory tree cheap.

Additionally, it provides a `constexpr` function `is_canonical` to make it
possible to write simple `static_assert`s that validate that
compile-time-constant paths are canonical at compile-time. (see `path_test.cc`
for an example)

### Installing

The current version of this library only supports [Bazel](https://bazel.io),
but a future push will likely support either autotools or CMake.

However, this library contains a total of 4 functional files, and is licenced
under the 3-clause BSD license, as long as the license headers are preserved,
the source files can be copied into another repository without issue.

### Naming

This library's name "Spin-2 FS Lib" (and the C++ namespace `spin_2_fs`) because
the author likes [Gravitons](https://en.wikipedia.org/wiki/Graviton) (which
would theoretically have a spin of 2).

"FS Lib" is pretty self explanatory since this is a library containing
filesystem handling functions and classes.


### License

This library is available under the 3-clause BSD license. See the
[COPYING](COPYING) file for a copy of that license.
