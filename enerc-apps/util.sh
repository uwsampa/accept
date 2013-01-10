#!/bin/sh
here=`dirname $0`
enercdir=$here/../llvm/tools/clang/enerc
llvmbin=$here/../llvm/Debug/bin
llvmlib=$here/../llvm/Debug/lib
cxx=$enercdir/bin/enerclang++
cc=$enercdir/bin/enerclang
rtdir=$here/../llvm/lib/Transforms/enerc/rt
analyzepy=$here/../llvm/lib/Transforms/enerc/analysis.py
profile=$rtdir/profile.sh
cflags=-g

if [ `uname` = "Darwin" ]; then
    libext=dylib
else
    libext=so
fi
enerclib=$llvmlib/enerc.$libext

usage () {
    echo "usage: $0 ACTION [--cxx] [--runargs ARGS] NAME SRCFILES..." >&2
    echo "actions: build analyze profile danalyze" >&2
    exit 1
}

# Action parsed first.
if [ "$1" = "" ] ; then
    usage
fi
action=$1 ; shift

# Optional arguments.
if [ "$1" = "--cxx" ] ; then
    shift
    compile=$cxx
else
    compile=$cc
fi
if [ "$1" = "--runargs" ] ; then
    shift
    runargs=$1
    shift
else
    runargs=""
fi

# Remaining positional arguments.
if [ "$1" = "" -o "$2" = "" ] ; then
    usage
fi
name=$1 ; shift
srcfiles=$@

if [ "$action" = "build" ] ; then
    rm -f $name.bc $name.ll
    $compile $cflags -c $srcfiles -emit-llvm -o $name.bc
    $llvmbin/llvm-dis $name.bc
elif [ "$action" = "analyze" ] ; then
    $llvmbin/opt -load=$enerclib -enerc -stats $name.bc -o $name.opt.bc
elif [ "$action" = "profile" ] ; then
    $profile $name.opt.bc $runargs
elif [ "$action" = "danalyze" ] ; then
    python $analyzepy
else
    usage
fi
