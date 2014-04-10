ACCEPT
======

This is the ACCEPT approximate compiler project from the [University of
Washington][uw].

[uw]: http://www.cs.washington.edu


Documentation
-------------

The project's [documentation][docs] includes instructions for building the tool and using it to optimize approximate programs.

[docs]: https://sampa.cs.washington.edu/accept/


What Is All This?
-----------------

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
* `docs/`: The Markdown-formatted documentation. This can be built with the
  [MkDocs][] tool.

[MkDocs]: http://www.mkdocs.org/
[LIT]: http://llvm.org/docs/CommandGuide/lit.html


Building the Docs
-----------------

The documentation can be translated into a nice HTML representation using [MkDocs][]. Install it and the SmartyPants Markdown extension with:

    $ pip install mkdocs
    $ pip install mdx_smartypants

To view the docs locally, type `mkdocs serve` and go to the indicated URL.

If you have the appropriate permissions, type `make deploydocs` to upload the HTML files to [our servers][docs].


License
-------

We haven't settled on a license yet! We should do that at some point.
