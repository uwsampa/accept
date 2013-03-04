#!/bin/sh
here=`dirname $0`
base=$here/..

$base/build/built/bin/llvm-lit -v $base/test
