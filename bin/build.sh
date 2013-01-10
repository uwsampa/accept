#!/bin/sh
destdir=`pwd`/build/built
mkdir -p $destdir

builddir=build
cd $builddir
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ..
make -j9
make install
