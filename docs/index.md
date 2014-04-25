# An Approximate Compiler

This is the documentation for ACCEPT, an *approximate compiler* for C and C++ programs based on [Clang][]. Think of it as your assistant in breaking your program in small ways to trade off correctness for performance.

[Clang]: http://clang.llvm.org/


## Building

Here's how to build the ACCEPT toolchain in four easy steps.

#### Clone

Clone the repository to your Unix-like system. Use the submodules flag to grab
the project's dependencies:

    $ git clone --recurse-submodules https://github.com/uwsampa/accept.git

#### CMake, Ninja, and virtualenv

There are three dependencies you need to install yourself before getting started. How you install them depends on your OS:

* [CMake][], which the easiest route to building LLVM.
* [Ninja][], a nice companion to CMake.
* [virtualenv][], a Python packaging tool. You can usually get this just by
  typing `pip install virtualenv`.

(If you prefer not to use Ninja, you can fairly easily edit the relevant scripts to have CMake write Makefiles instead.)

#### make setup

Inside this directory (the repository containing this README file), type `make setup`. This will do several things:

* Download and extract the [LLVM][llvm-dl] source.
* Build LLVM and our modified Clang frontend using CMake and Ninja. (This can take a long time.) The programs and libraries are installed into the `build/built/` subdirectory.
* Build the ACCEPT-specific extensions to LLVM and Clang.
* Create a Python [virtual environment][venv] and install the driver's dependencies therein.
Take a look inside the Makefile if you're curious about how to run any of these steps individually.

You should now be able to use the `bin/enerclang` and `bin/enerclang++` programs to compile EnerC programs. You can type `make test` to make sure everything's working.

[llvm-dl]: http://llvm.org/releases/index.html
[Ninja]: http://martine.github.com/ninja/
[CMake]: http://www.cmake.org/
[virtualenv]: http://www.virtualenv.org/


## Using

The main entry point to the ACCEPT toolchain is the `bin/accept` script. For
convenience, you can put this on your `$PATH` by running `source activate.sh`.

Follow the [tutorial](tutorial.md) to learn how to use ACCEPT to optimize your favorite program. If you get stuck, check out the [command-line interface reference](cli.md).
