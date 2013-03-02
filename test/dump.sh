#!/bin/sh
base=`dirname $0`/..
$base/bin/enerclang -c -S -O0 -emit-llvm -o - $@
