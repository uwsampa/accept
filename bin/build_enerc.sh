#!/bin/sh
base=`dirname $0`/..

destdir=$base/build/built
mkdir -p $destdir

builddir=$base/build/enerc
mkdir -p $builddir
cd $builddir
cmake -G Ninja -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ../..
ninja install
