#!/bin/sh

if [ $# -ne 2 ]; then
	echo Usage: $(basename "$0") '<filename.elf>' '<breakpoint>' >&2
	exit 1
fi

elf=$1
bkpt=$2
shift 2

if [ ! -f mspdebug-wrapper/mspdebug_wrapper.py ]; then
	echo Please 'git submodule init'
fi

# TODO: -s mean simulate; don't use for real code on real chips
SIMFLAG=-s

./mspdebug-wrapper/mspdebug_wrapper.py -b "$bkpt" ${SIMFLAG} "$elf"
