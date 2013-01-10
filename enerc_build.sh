#!/bin/sh

# This is where all build products will be installed.
destdir=`pwd`/built
mkdir -p $destdir

builddir="build-enerc"
mkdir -p $builddir
cd $builddir
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ..
make -j9
make install
