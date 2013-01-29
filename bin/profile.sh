#!/bin/sh
here=`dirname $0`

enercdir=$here/..
bin=$enercdir/build/built/bin

# Runtime library disabled for now since we're using an interpretation (instead
# of instrumentation) approach.
# proflib=$enercdir/build/enerc/rt/enercrt.bc

bcfile=$1
shift
$bin/llvm-link $bcfile | $bin/opt -strip | $bin/enerci - $@
