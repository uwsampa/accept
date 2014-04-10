# An Approximate Compiler

The documentation for ACCEPT will appear here. Eventually.


## Building

Here's how to build the ACCEPT toolchain:

1. Clone the repository to your Unix-like system. Use the submodules flag to
   grab the project's dependencies:

        git clone --recurse-submodules https://github.com/uwsampa/accept.git

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


## Using

The main entry point to the ACCEPT toolchain is the `bin/accept` script. For
convenience, you can put this on your `$PATH` by running `source activate.sh`.

Follow the [tutorial](tutorial.md) to learn how to use ACCEPT to optimize your favorite program. If you get stuck, check out the [command-line interface reference](cli.md).
