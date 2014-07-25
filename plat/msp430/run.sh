#!/bin/sh

WHERE=$(dirname "$0")

if [ ! -f "$WHERE"/mspdebug-wrapper/mspdebug_wrapper.py ]; then
	echo Please 'git submodule init'
	exit 1
fi

if [ $# -lt 1 ]; then
	echo Usage: $(basename "$0") '<filename.elf> [<outfile.txt>] [<bkpt>]' >&2
	exit 1
fi

elf="$1"
shift

if [ -n "$1" ]; then
	outfile="$1"
	shift
else
	outfile="msp430out.txt"
fi

if [ -n "$1" ]; then
	bkpt="$1"
else
	# look for a self-loop like this:
	#   addr: jmp $+0
	bkpt=$(msp430-objdump -d "$elf" | grep ';abs' | \
		awk '{print $1, $7}' | sed -e 's/: 0x/ /' | \
		while read a b; do test "$a" = "$b" && echo "0x$a" && break; done)
	if [ -n "$bkpt" ]; then
		echo "Will break at $bkpt"
	else
		echo "Coult not find infinite loop to break on; call with <bkpt>" >&2
		exit 2
	fi
fi

EXTRAARGS="-H 192.168.60.132 -m /opt/mspdebug/bin/mspdebug" # ransford's VM

"$WHERE"/mspdebug-wrapper/mspdebug_wrapper.py -d -b "$bkpt" \
	-o "$outfile" ${EXTRAARGS} -T accept_time.txt "$elf"
