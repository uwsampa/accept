/* Read an accelerometer repeatedly in a loop.  Most code adapted from the
 * dlwisp41 firmware. */

#include <enerc.h>
#include <msp430.h>
#include "dlwisp41.h"

#define NUM_READINGS 250u
#define NUM_AVG 16
#define DUMMY_AVG 0xBEEFu

unsigned int numreadings = 0;
unsigned int iters = 0;

// status register &= ~i
inline void __bic_status_register (unsigned i) {
    asm volatile ("BIC.W %0, R2" ::"m"(i));
}

// status register |= i
inline void __bis_status_register (unsigned i) {
    asm volatile ("BIS.W %0, R2" ::"m"(i));
}

APPROX unsigned read_accel() {
    APPROX unsigned char i;
    APPROX unsigned reading;

    // slow down clock
    BCSCTL1 = XT2OFF + RSEL1; // select internal resistor (still has effect when
    // DCOR=1)
    DCOCTL = DCO1+DCO0; // set DCO step.

    // Clear out any lingering voltage on the accelerometer outputs
    ADC10AE0 = 0;

    P2OUT &= ~(ACCEL_X | ACCEL_Y | ACCEL_Z);
    P2DIR |=   ACCEL_X | ACCEL_Y | ACCEL_Z;
    P2DIR &= ~(ACCEL_X | ACCEL_Y | ACCEL_Z);

    P1DIR |= ACCEL_POWER;
    P1OUT |= ACCEL_POWER;
    ADC10AE0 |= ACCEL_X | ACCEL_Y | ACCEL_Z;

    // a little time for regulator to stabilize active mode current AND
    // filter caps to settle.
    for(i = 0; ENDORSE(i < 225); i++);
    RECEIVE_CLOCK;

    ADC10CTL0 &= ~ENC;
    ADC10CTL0 = SREF_0 + ADC10SHT_1 + ADC10ON;
    ADC10CTL1 = ADC10DIV_2 + ADC10SSEL_0 + SHS_0 + CONSEQ_0 + INCH_ACCEL_X;
    ADC10CTL0 |= ENC;
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);    // wait while ADC finishes work
    reading = ADC10MEM & 0x3ff;

    // Power off sensor and adc
    P1DIR &= ~ACCEL_POWER;
    P1OUT &= ~ACCEL_POWER;
    ADC10CTL0 &= ~ENC;
    ADC10CTL1 = 0;       // turn adc off
    ADC10CTL0 = 0;       // turn adc off

    ++iters;
    return reading;
}

#ifdef __clang__
// work around a bug wherein clang doesn't know that main() belongs in section
// .init9 or that it has to be aligned in a certain way (mspgcc4 introduced
// these things)
__attribute__((section(".init9"), aligned(2)))
#endif
int main (void) {
    APPROX unsigned cur_reading = 0;
    APPROX unsigned max = 0;
    APPROX unsigned min = 0xffffu;
    APPROX signed int avg = DUMMY_AVG;
    unsigned int sum = 0, summed = 0;

    WDTCTL = WDTPW + WDTHOLD; // stop watchdog timer to avoid untimely death

    for (; numreadings != NUM_READINGS; ++numreadings) {
        cur_reading = read_accel(); // ACCEPT_PERMIT

        if (ENDORSE(cur_reading > max))
            max = cur_reading;

        if (ENDORSE(cur_reading < min))
            min = cur_reading;

        // weighted average of last NUM_AVG readings
        if (ENDORSE(avg == DUMMY_AVG)) {
            sum += cur_reading;
            if (++summed == NUM_AVG)
                avg = sum / NUM_AVG;
            continue;
        }

        avg = avg + (((signed int)cur_reading - avg) / NUM_AVG);
    }

    asm volatile ("MOV.W %0, R12" ::"m"(min));
    asm volatile ("MOV.W %0, R13" ::"m"(max));
    asm volatile ("MOV.W %0, R14" ::"m"(avg));
    return 19; // lucky 0x13
}
