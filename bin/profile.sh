#!/bin/sh
here=`dirname $0`

llvmdir=$here/../../../..
builddir=$llvmdir/Debug
bin=$builddir/bin

proflib=$here/profileinst.bc

bcfile=$1
shift
$bin/llvm-link $proflib $bcfile | $bin/opt -strip | $bin/lli - $@
