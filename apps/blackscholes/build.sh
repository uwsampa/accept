#!/bin/sh
here=`dirname $0`
args="--runargs '1 in_16.txt output.txt' blackscholes blackscholes.c"
. $here/../build.sh
