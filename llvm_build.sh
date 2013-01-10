#!/bin/sh

# This is where all build products will be installed.
destdir=`pwd`/built
mkdir -p $destdir

# This is the scratch directory for an out-of-source LLVM build.
builddir="build-llvm"
mkdir -p $builddir

cd $builddir
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ../llvm
make -j9
make install
