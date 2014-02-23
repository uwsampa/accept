#!/bin/sh
here=$(cd "$(dirname "$0")"; pwd)
base=$here/..

destdir=$base/build/built
mkdir -p $destdir

# Check to make sure LLVM is built where we're expecting it.
if [ ! -e $destdir/share/llvm/cmake/LLVMConfig.cmake ] ; then
    echo "LLVM not present in build/built/. Please run build_llvm.sh first."
    exit 1
fi

builddir=$base/build/enerc
mkdir -p $builddir
cd $builddir
cmake -G Ninja -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ../..
ninja install
