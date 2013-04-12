#!/bin/sh
here=$(cd "$(dirname "$0")"; pwd)
base=$here/..

destdir=$base/build/built
mkdir -p $destdir

builddir=$base/build/enerc
mkdir -p $builddir
cd $builddir
cmake -G Ninja -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ../..
ninja install
