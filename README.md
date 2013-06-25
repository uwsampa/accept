EnerC
=====

This repository contains everything you need to get started with the
EnerC approximate compiler project.


Quick Start
-----------

The build scripts included use [CMake][] and [Ninja][]. If you don't
have these tools, install them or edit the scripts to use make or
whatever your preferred tool is.

[Ninja]: http://martine.github.com/ninja/
[CMake]: http://www.cmake.org/

To get all set up, you will need to download (vanilla) LLVM and compile it with
our modified version of Clang that supports type qualifiers. This repository
contains a couple of scripts that help with this. To get started, `cd` to this
directory and then run:

    $ ./bin/fetch_llvm.sh

That downloads LLVM 3.2 into a directory called `llvm`. Then it symlinks our
Clang subrepository into `llvm/tools/clang`, which is required for the LLVM
build process. Then, to build both LLVM and Clang, run:

    $ ./bin/build_llvm.sh

This uses CMake and Ninja to build everything and install it under
`build/built/`. You now have a fully-functional Clang compiler at
`build/built/bin/clang`. Next, we need to build the EnerC extensions to LLVM
and Clang. Run:

    $ ./bin/build_enerc.sh

Again using CMake, that builds three things: a frontend (Clang) plugin
for checking types, a backend (LLVM) pass for analysis and
transformation, and a runtime library. Now you can use `bin/enerclang`
and `bin/enerclang++` to compile EnerC programs.

Now, it might be a good idea to `./bin/runtests.sh` to make sure everything's
working.


Using the Tool
--------------

Once you have the compiler set up, you can use the driver logic to
interactively explore relaxations of programs. This stuff is all written in
Python and has a number of dependencies. Install these by typing:

    pip install -r requirements.txt

The main entry point is the `bin/accept` script, which invokes the Python code
in the `accept` directory. For convenience, you can put this on your `$PATH` by
running `source activate.sh`.

Type `accept help` to see the available commands:

* `accept log`: Dump the ACCEPT compiler log when compiling the program in the
  current directory, which can help you get an idea of which approximations are
  available. The output is filtered through `c++filt` to demangle C++ names.
* `accept build`: Dump the Clang output from compiling the program. This can be
  helpful during the annotation process. Shares a memoized build call with the
  `log` command, so after running one, the other will be free.
* `accept precise`: Show the precise output, precise running time, and
  approximation opportunities for the application.
* `accept approx [CONFIGNUM]`: Similarly, show the approximate output and
  approximate running time for either a particular relaxation or all
  configurations if the parameter is omitted. Both this and `precise` share
  memoized calls with the main experiments.

All of the commands use memoization to some degree. Memoized results are stored
in an SQLite database called `memo.db`. Clear this out or pass the
`-f`/`--force` flag to `accept` to throw away the results and recompute.

(If you're hacking on ACCEPT itself, you can also use the `build_enerc.sh` and
`build_llvm.sh` scripts once you've extended your `$PATH`. This avoids the
`cd`-to-build-directory, `cd -` dance that would otherwise be necessary.)

### Running the Experiments

For some of the benchmarks, you will need some large input files that are not included in this repository:

* blackscholes: "simlarge" input from PARSEC (`in_64K.txt`)
* canneal: "native" input (`2500000.nets`)
* fluidanimate: "simlarge" input (`in_300K.fluid`)
* x264: "simmedium" input (`eledream_640x360_32.y4m`)

Run the experiments by typing `accept exp`.

### Running on a Cluster

The above will run all compilations and executions locally and in serial. To
run data collections on a cluster, install and set up the [cluster-workers][]
library. Then pass the `-c`/`--cluster` flag to `accept` to enable cluster
execution.

[cluster-workers]: https://github.com/sampsyo/cluster-workers


What Else is Here?
------------------

This repository includes lots of stuff:

* `bin/`: Helpful scripts for developing EnerC as well as the front-end
  scripts `enerclang` and `enerclang++`, which act as C and C++ compiler
  executables.
* `clang/`: This git subrepository contains our version of Clang, which is
  hacked to support type qualifiers. After running `fetch_llvm.sh`, there will
  be a symlink to this directory at `llvm/tools/clang`.
* `checker/`: The Clang plugin that checks the EnerC type system and
  emits annotated LLVM bitcode.
* `checkerlib/`: This is a subrepository (hosted as a separate git repo
  on BitBucket) that supports the writing of modular type checkers for
  Clang like the one above.
* `pass/`: The LLVM compiler pass that analyzes annotated bitcode and
  performs instrumentation/transformation.
* `include/`: A header file that EnerC programs should use (via
  `#include <enerc.h>`) to get the necessary type qualifiers and
  endorsement macros. This directory is automatically added as an
  include directory when you run the `enerclang` script.
* `rt/`: A runtime library that is linked into EnerC programs for
  dynamic analysis.
* `test/`: Some tests for the frontend (type errors and bitcode
  emission). This uses LLVM's [LIT][] testing infrastructure. To run the
  tests, just type `./bin/runtests.sh`.
* `apps/`: Benchmarks annotated for use with EnerC.
* `accept/`: The high-level profile-guided feedback loop used to drive a full
  compilation. This Python package also scripts the experiments that generate
  the results used in the (eventual) paper.

[LIT]: http://llvm.org/docs/CommandGuide/lit.html
