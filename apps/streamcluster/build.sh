#!/bin/sh
here=`dirname $0`
args="--cxx --runargs '3 10 3 16 16 10 none output.txt 1' streamcluster streamcluster.cpp"
. $here/../build.sh

# PARSEC sizes:
# test: 2 5 1 10 10 5
# simdev: 3 10 3 16 16 10
# simsmall: 10 20 32 4096 4096 1000
# simmedium: 10 20 64 8192 8192 1000
# simlarge: 10 20 128 16384 16384 1000
# native: 10 20 128 1000000 200000 5000
