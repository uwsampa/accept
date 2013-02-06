#!/bin/sh
here=`dirname $0`
args="--runargs '-DN=960 -DNCO=4' blackscholes blackscholes.c"
. $here/../build.sh
