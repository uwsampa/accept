#!/bin/sh

if [ ! -f mspdebug-wrapper/mspdebug_wrapper.py ]; then
	echo Please 'git submodule init'
	exit 1
fi

if [ $# -lt 2 ]; then
	echo Usage: $(basename "$0") '<filename.elf> <outfile.txt> [<bkpt>]' >&2
	exit 1
fi

elf="$1"
outfile="$2"

if [ $# -eq 3 ]; then
	bkpt="$3"
else
	# look for a self-loop like this:
	#   addr: jmp $+0
	bkpt=$(msp430-objdump -d "$elf" | grep ';abs' | \
		awk '{print $1, $7}' | sed -e 's/: 0x/ /' | \
		while read a b; do test "$a" = "$b" && echo "0x$a" && break; done)
	if [ -z "$bkpt" ]; then
		echo "Coult not find infinite loop to break on; call with <bkpt>" >&2
		exit 2
	fi
fi


# TODO: -s mean simulate; don't use for real code on real chips
SIMFLAG=

./mspdebug-wrapper/mspdebug_wrapper.py -d -b "$bkpt" ${SIMFLAG} -o "$outfile" "$elf"
