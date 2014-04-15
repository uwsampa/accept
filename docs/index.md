# An Approximate Compiler

This is the documentation for ACCEPT, an *approximate compiler* for C and C++ programs based on [Clang][]. Think of it as your assistant in breaking your program in small ways to trade off correctness for performance.

[Clang]: http://clang.llvm.org/


## Building

Here's how to build the ACCEPT toolchain in four easy steps.

#### Clone

Clone the repository to your Unix-like system. Use the submodules flag to grab
the project's dependencies:

    $ git clone --recurse-submodules https://github.com/uwsampa/accept.git

#### CMake and Ninja

Install [CMake][] and [Ninja][], which our build scripts use to build to tool. (If you like, you can of course edit the relevant scripts to use make or another build tool instead of Ninja.)

#### make setup

Inside this directory (the repository containing this README file), type `make setup`. This will do several things:

* Download and extract the [LLVM][llvm-dl] source.
* Build LLVM and our modified Clang frontend using CMake and Ninja. (This can take a long time.) The programs and libraries are installed into the `build/built/` subdirectory.
* Build the ACCEPT-specific extensions to LLVM and Clang.
Take a look inside the Makefile if you're curious about how to run any of these steps individually.

You should now be able to use the `bin/enerclang` and `bin/enerclang++` programs to compile EnerC programs. You can type `make test` to make sure everything's working.

#### Python Dependencies

Finally, ensure that you have the dependencies for the Python-based driver component. Use [pip][], the standard Python package installer:

    $ pip install -r requirements.txt

If pip complains about permissions, you may need to use `sudo` with this command.

Now you're ready to approximate! Make sure the driver script is working by typing `./bin/accept help`.

[llvm-dl]: http://llvm.org/releases/index.html
[Ninja]: http://martine.github.com/ninja/
[CMake]: http://www.cmake.org/
[pip]: https://github.com/pypa/pip


## Using

The main entry point to the ACCEPT toolchain is the `bin/accept` script. For
convenience, you can put this on your `$PATH` by running `source activate.sh`.

Follow the [tutorial](tutorial.md) to learn how to use ACCEPT to optimize your favorite program. If you get stuck, check out the [command-line interface reference](cli.md).
