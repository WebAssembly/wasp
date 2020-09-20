This is a partial copy of the `gdtoa` library, from http://www.netlib.org/fp/.

Enough has been copied only for the following functions:

- `g_ffmt`
- `g_dfmt`
- `strtorf`
- `strtord`

It has been modified as follows:

* The files `arith.h` and `gd_qnan.h` have already been generated
* The `strtof` and `strtod` functions have been renamed so they don't conflict
  with the C stdlib functions
* A `CMakeLists.txt` file has been added
