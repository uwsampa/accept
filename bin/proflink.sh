#!/bin/sh

# This script links an instrumented LLVM bitcode file with the runtime
# profiling library and emits (to standard out) a native object file ready to
# be linked as an executable.

here=`dirname $0`
enercdir=$here/..
bin=$enercdir/build/built/bin
proflib=$enercdir/build/enerc/rt/enercrt.bc

$bin/llvm-link $proflib $@ | $bin/opt -strip | $bin/llc -filetype=obj
