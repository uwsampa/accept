// Zynq platform: use FPGA cycle timer and print to stdout.
#include <stdio.h>

// Memory mapped hardware counter.
#define gpio_addr       0xe000a048
#define rd_fpga_clk()   *((volatile unsigned int*)gpio_addr)

unsigned int accept_clock_begin;

void accept_roi_begin() {
    accept_clock_begin = rd_fpga_clk();
}

void accept_roi_end() {
    unsigned int clock_end = rd_fpga_clk();
    unsigned int elapsed = accept_clock_begin - clock_end;
    printf("ACCEPT TIME: %u\n", elapsed);
}
