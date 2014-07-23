#include <msp430.h>
#include <perfctr.h>

void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) __perfctr_TA0_isr (void)
{
    asm volatile("nop");
    __perfctr += __PERFCTR_CYCLES + TA0R;
    TA0CCR0 = __PERFCTR_CYCLES;
}
