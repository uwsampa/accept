#include <msp430.h>
#include "perfctr.h"

/*****
 * Simple performance counter for MSP430 that uses TIMER_A0 (with clock
 * reference SMCLK) to count cycles during execution.  Call __perfctr_init() to
 * set up the timer.  Call __perfctr_start() just before the code you want to
 * profile; call __perfctr_stop() afterward and read the value of __perfctr.
 *
 * This mechanism imposes negligible (but nonzero) overhead due to its functions
 * returning and its interrupt sequence which fires once every __PERFCTR_CYCLES
 * cycles of SMCLK.
 */

unsigned long __perfctr;

void __perfctr_init (void) {
    __perfctr = 0ul;
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL__SMCLK;
}

void __perfctr_start (void) {
    /* start at 0, count up to __PERFCTR_CYCLES, and fire an interrupt */
    TA0R = 0;
    TA0CCR0 = __PERFCTR_CYCLES;
    TA0CTL |= MC__UP;
}

void __perfctr_stop (void) {
    __perfctr += TA0R;
    TA0CTL |= TACLR;
}

/*
int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;

  __perfctr_init();
  __no_operation();

  __perfctr_start();
  __delay_cycles(10000);
  __perfctr_stop();
  __no_operation();

  __bis_SR_register(LPM0_bits | GIE);
  __no_operation();
}
*/

void __attribute__ ((interrupt(TIMER0_A0_VECTOR)))
__perfctr_TA0_isr (void)
{
  __perfctr += __PERFCTR_CYCLES + TA0R;
  TA0CCR0 = __PERFCTR_CYCLES;
}
