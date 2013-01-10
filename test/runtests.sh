#!/bin/sh
bindir=`dirname $0`/../bin
cc=$bindir/enerclang
testc="$cc -emit-llvm -S -o -"

cd `dirname $0`
$testc test_errors.c
$testc test_noerrors.c
$testc test_cxx.cpp
$testc test_cxx2.cpp
