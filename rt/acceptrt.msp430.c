#include "msp430/perfctr/perfctr.h"

void accept_roi_begin() {
    __perfctr_init();
    __perfctr_start();
}

void accept_roi_end() {
    __perfctr_stop();
    __accept_roi_result = __perfctr;
    perfctr_lo = (unsigned)(__perfctr & 0x0000ffff);
    perfctr_hi = (unsigned)(__perfctr >> 16);
    asm volatile (
            "MOV %0, R9\n"
            "MOV %1, R10\n"
            "NOP" // this is a nice place to set a breakpoint
            ::"m"(perfctr_hi), "m"(perfctr_lo));
}
