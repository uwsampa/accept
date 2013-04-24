#!/bin/sh
here=$(cd "$(dirname "$0")"; pwd)
base=$here/..
modified_llvm_files=$base/modified_LLVM_files

# This is where all build products will be installed.
destdir=$base/build/built
mkdir -p $destdir

# Build LLVM "out-of-source" in a scratch directory.
builddir=$base/build/llvm
mkdir -p $builddir
cp $modified_llvm_files/BasicAliasAnalysis.cpp $base/llvm/lib/Analysis
cp $modified_llvm_files/TypeBasedAliasAnalysis.cpp $base/llvm/lib/Analysis
cd $builddir
cmake -G Ninja -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:PATH=$destdir ../../llvm
ninja install
