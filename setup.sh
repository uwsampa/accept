#!/bin/sh

# Get LLVM.
curl -O http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
tar -xf llvm-3.2.src.tar.gz
mv llvm-3.2.src llvm

# Get (hacked) Clang.
cd llvm/tools
git clone git@github.com:bholt/clang-annos.git clang  # May relocate this.
 
# Compiler pass.
cd ../lib/Transforms
hg clone ssh://bicycle//projects/sampa/epa/enerc

# Applications.
cd ../../..
hg clone ssh://bicycle//projects/sampa/epa/enerc-apps
