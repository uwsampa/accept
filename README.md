EnerC
=====

This repository contains everything you need to get started with the
EnerC approximate compiler project.


Quick Start
-----------

Here's the easiest way to get started:

1. Clone this repository to your Unix-like system.

2. Install [CMake][] and [Ninja][], which our build scripts use to build to tool. (If you like, you can of course edit the relevant scripts to use make or another build tool instead of Ninja.)

3. Inside this directory (the repository containing this README file), type `make setup`. This will do several things:

    * Download and extract [LLVM 3.2][llvm-dl].
    * Build LLVM and our modified Clang frontend using CMake and Ninja. (This can take a long time.) The programs and libraries are installed into the `build/built/` subdirectory.
    * Build the ACCEPT-specific extensions to LLVM and Clang.
Take a look inside the Makefile if you're curious about how to run any of these steps individually.

    You should now be able to use the `bin/enerclang` and `bin/enerclang++` programs to compile EnerC programs. You can type `make test` to make sure everything's working.

4. Finally, ensure that you have the dependencies for the Python-based driver component. Use [pip][], the standard Python package installer:

        pip install -r requirements.txt

    Make sure the driver script is working by typing `./bin/accept help`.

[llvm-dl]: http://llvm.org/releases/index.html
[Ninja]: http://martine.github.com/ninja/
[CMake]: http://www.cmake.org/
[pip]: https://github.com/pypa/pip


Using the Tool
--------------

The main entry point to the ACCEPT toolchain is the `bin/accept` script. For
convenience, you can put this on your `$PATH` by running `source activate.sh`.

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

### Running the Included Experiments

For some of the benchmarks, you will need some large input files that are not included in this repository:

* blackscholes: "simlarge" input from PARSEC (`in_64K.txt`)
* canneal: "native" input (`2500000.nets`)
* fluidanimate: "simlarge" input (`in_300K.fluid`)
* x264: "simmedium" input (`eledream_640x360_32.y4m`)

Run the experiments by typing `accept exp`.

### Writing Your Own Experiment

To add your own app `foo`, put your C/C++ sources in `apps/foo`.  (The build
system will assume all `.c` and `.cpp` files are to be built and linked
together.)

Add `#include <enerc.h>` to files where you plan to add `APPROX` type
qualifiers.

Insert `APPROX` type qualifiers into your code as appropriate.  (The ACCEPT paper contains details on the annotation language.)
You can run `make build_orig` and `make build_opt` to build precise and
approximate versions of your app, respectively.  Use `accept -f log` during
development to preview relaxation opportunity sites.

Insert a call to `accept_roi_begin()` immediately before the program's main
chunk of work, and insert a call to `accept_roi_end()` immediately afterward.

Finally, run your app:

    $ accept exp foo

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
