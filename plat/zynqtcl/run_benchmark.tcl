# Read arguments
if {$argc != 3 && $argc != 4} {
    puts "The script requires one arguments to be inputed."
    puts "\tUsage: xmd -tcl tcl/run_benchmark.tcl [bitfile] [ps7init] [elffile] [datafile]"
    exit 1
} else {
    set bitfile [lindex $argv 0]
    set ps7init [lindex $argv 1]
    set elffile [lindex $argv 2]
    if {$argc == 4} {
        set loaddata 1
        set datafile [lindex $argv 3]
    } else {
        set loaddata 0
    }
}

# Program the FPGA
fpga -cable type xilinx_plugin modulename digilent_plugin modulearg Device=zynq2 -f $bitfile
# Connect to the PS section
connect arm hw -cable type xilinx_plugin modulename digilent_plugin modulearg Device=zynq2
# Reset the system
rst
# Initialize the PS section (Clock PLL, MIO, DDR etc.)
source $ps7init
ps7_init
ps7_post_config
# Load the elf program
dow $elffile
if {$loaddata == 1} {
    # Load the data file at address 0x8000000
    dow -data $datafile 0x8000000
}
# Start execution
con