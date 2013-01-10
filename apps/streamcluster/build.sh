#!/bin/sh
here=`dirname $0`
args="--cxx --runargs '2 5 1 10 10 5 none output.txt 1' streamcluster streamcluster.cpp"
. $here/../build.sh
