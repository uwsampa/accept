#!/bin/sh
here=`dirname $0`
enercdir=$here/..
builtdir=$enercdir/build/built
cxx=$enercdir/bin/enerclang++
cc=$enercdir/bin/enerclang
proflink=$enercdir/bin/proflink.sh
cflags="-g -fno-use-cxa-atexit"

if [ `uname` = "Darwin" ]; then
    libext=dylib
else
    libext=so
fi
enerclib=$builtdir/lib/enerc.$libext

usage () {
    echo "usage: $0 ACTION [--cxx] [--runargs ARGS] NAME SRCFILES..." >&2
    echo "actions: build profile" >&2
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
    link=c++
else
    compile=$cc
    link=cc
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
    rm -f enerc_static.txt  # Empty out static numbers to build them up again.
    for srcfile in $srcfiles
    do
        $compile $cflags -c $srcfile -emit-llvm -o $srcfile.bc
        bcfiles="$bcfiles $srcfile.bc"
    done
    $builtdir/bin/llvm-link $bcfiles > $name.bc
    $builtdir/bin/llvm-dis $name.bc
elif [ "$action" = "profile" ] ; then
    $proflink $name.opt.bc > $name.o
    $link $name.o -o $name
    ./$name $runargs
else
    usage
fi
