#!/bin/sh
here=`dirname $0`

# A shim for executing binaries on the Zynq hardware.
# Two arguments: the bitstream and the ELF binary.
bit=$1
elf=$2

source /sampa/share/Xilinx/14.6/14.6/ISE_DS/settings64.sh
LOG_FILE = /sampa/share/thierry/minicom.out
RUN_TCL = $here/zynqtcl/run_benchmark.tcl
PS7_INIT = $here/zynqtcl/ps7_init_111.tcl
SWITCH = zynq

# Power-cycle the board.
wemo switch $SWITCH off
sleep 3s
wemo switch $SWITCH on

# Execute the benchmark and collect its log.
truncate $LOG_FILE --size 0
xmd -tcl $RUN_TCL $bit $PS7_INIT $elf
sleep 10s
cp $LOG_FILE zynqlog.txt
