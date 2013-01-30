#!/bin/sh
here=`dirname $0`

enercdir=$here/..
bin=$enercdir/build/built/bin
proflib=$enercdir/build/enerc/rt/enercrt.bc

bcfile=$1
shift
$bin/llvm-link $proflib $bcfile | $bin/opt -strip | $bin/lli - $@
