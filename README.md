EnerC
=====

This repository contains everything you need to get started with the
EnerC approximate compiler project.


Quick Start
-----------

To get all set up, you will need to download and compile LLVM and a
modified version of Clang that supports type qualifiers. This repository
contains a couple of scripts that help with this. To get started, `cd`
to this directory and then run:

    $ ./bin/fetch_llvm.sh

That downloads LLVM 3.2 into a directory called `llvm`. Then it checks
out our modified Clang from git and places it in the LLVM source tree.
To build both LLVM and Clang, run:

    $ ./bin/build_llvm.sh

This uses CMake to build everything and install it under `build/built/`.
You now have a fully-functional Clang compiler at
`build/built/bin/clang`. Next, we need to build the EnerC extensions to
LLVM and Clang. Run:

    $ ./bin/build_enerc.sh

Again using CMake, that builds three things: a frontend (Clang) plugin
for checking types, a backend (LLVM) pass for analysis and
transformation, and a runtime library. Now you can use `bin/enerclang`
and `bin/enerclang++` to compile EnerC programs.


What's Here
-----------

This repository includes lots of stuff:

* `bin/`: Helpful scripts for developing EnerC as well as the front-end
  scripts `enerclang` and `enerclang++`, which act as C and C++ compiler
  executables.
* `checker/`: The Clang plugin that checks the EnerC type system and
  emits annotated LLVM bitcode.
* `pass/`: The LLVM compiler pass that analyzes annotated bitcode and
  performs instrumentation/transformation.
* `include/`: A header file that EnerC programs should use (via
  `#include <enerc.h>`) to get the necessary type qualifiers and
  endorsement macros. This directory is automatically added as an
  include directory when you run the `enerclang` script.
* `rt/`: A runtime library that is linked into EnerC programs for
  dynamic analysis.
* `test/`: Some tests for the frontend (type errors and bitcode
  emission).
* `apps/`: Benchmarks annotated for use with EnerC.
