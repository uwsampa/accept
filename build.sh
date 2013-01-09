#!/bin/sh
builddir="llvm-build"
mkdir -p $builddir
cd $builddir
# Setting CMAKE_INSTALL_PREFIX makes it possible to use $LLVM_INCLUDE_DIRS,
# etc., when building "out of source" packages.
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=`pwd` ../llvm
make -j9
# Place all the headers in the build directory.
make install
