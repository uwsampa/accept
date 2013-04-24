#!/bin/sh

# Get LLVM.
curl -O http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
tar -xf llvm-3.2.src.tar.gz
mv llvm-3.2.src llvm

# Create a symlink to Clang, which is a subrepository.
cd llvm/tools
ln -s ../../clang .
