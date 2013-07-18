/* Read an accelerometer repeatedly in a loop.  Most code adapted from the UMass
 * Moo CRFID's firmware. */

#include <enerc.h>
#include <msp430.h>
#include "dlwisp41.h"

#define NUM_READINGS 1000

unsigned numreadings = 0;

// status register &= ~i
inline void __bic_status_register (unsigned i) {
    asm volatile ("BIC.W %0, R2" ::"m"(i));
}

// status register |= i
inline void __bis_status_register (unsigned i) {
    asm volatile ("BIS.W %0, R2" ::"m"(i));
}

typedef struct {
    APPROX unsigned x;
    APPROX unsigned y;
    APPROX unsigned z;
} accel_reading;

void read_sensor (accel_reading *reading) {
    unsigned char i;

    // turn off comparator
    P1OUT &= ~RX_EN_PIN;

    // slow down clock
    BCSCTL1 = XT2OFF + RSEL1; // select internal resistor (still has effect when
    // DCOR=1)
    DCOCTL = DCO1+DCO0; // set DCO step.

    if(!is_power_good())
        sleep();

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
    for(i = 0; i < 225; i++);
    RECEIVE_CLOCK;

    // x
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 = SREF_0 + ADC10SHT_1 + ADC10ON;
    ADC10CTL1 = ADC10DIV_2 + ADC10SSEL_0 + SHS_0 + CONSEQ_0 + INCH_ACCEL_X;
    ADC10CTL0 |= ENC;
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);    // wait while ADC finishes work
    reading->x = ADC10MEM & 0x3ff;

    // y
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 = SREF_0 + ADC10SHT_1 + ADC10ON;
    ADC10CTL1 = ADC10DIV_2 + ADC10SSEL_0 + SHS_0 + CONSEQ_0 + INCH_ACCEL_Y;
    ADC10CTL0 |= ENC;
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);    // wait while ADC finishes work
    reading->y = ADC10MEM & 0x3ff;

    // z
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 = SREF_0 + ADC10SHT_1 + ADC10ON;
    ADC10CTL1 = ADC10DIV_2 + ADC10SSEL_0 + SHS_0 + CONSEQ_0 + INCH_ACCEL_Z;
    ADC10CTL0 |= ENC;
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);    // wait while ADC finishes work
    reading->z = ADC10MEM & 0x3ff;

    // Power off sensor and adc
    P1DIR &= ~ACCEL_POWER;
    P1OUT &= ~ACCEL_POWER;
    ADC10CTL0 &= ~ENC;
    ADC10CTL1 = 0;       // turn adc off
    ADC10CTL0 = 0;       // turn adc off

    // Store sensor read count
    numreadings++;
}

unsigned short is_power_good (void) {
    return P2IN & VOLTAGE_SV_PIN;
}

inline void sleep (void) {
    P1OUT &= ~RX_EN_PIN;
    // enable port interrupt for voltage supervisor
    P2IES = 0;
    P2IFG = 0;
    P2IE |= VOLTAGE_SV_PIN;
    P1IE = 0;
    P1IFG = 0;
    TACTL = 0;

    __bic_status_register(GIE); // temporarily disable GIE so we can sleep and enable interrupts
    // at the same time
    P2IE |= VOLTAGE_SV_PIN; // Enable Port 2 interrupt

    if (is_power_good())
        P2IFG = VOLTAGE_SV_PIN;

    __bis_status_register(LPM4_bits | GIE);

    return;
}

#ifdef __clang__
// work around a bug wherein clang doesn't know that main() belongs in section
// .init9 or that it has to be aligned in a certain way (mspgcc4 introduced
// these things)
__attribute__((section(".init9"), aligned(2)))
#endif
int main (void) {
    accel_reading cur_reading;
    APPROX unsigned max_x, max_y, max_z;
    APPROX unsigned min_x, min_y, min_z;
    APPROX unsigned avg_x, avg_y, avg_z;
    unsigned i;

    for (i = 0; i < NUM_READINGS; ++i) {
        read_sensor(&cur_reading);

        if (ENDORSE(cur_reading.x > max_x)) max_x = cur_reading.x;
        if (ENDORSE(cur_reading.y > max_y)) max_y = cur_reading.y;
        if (ENDORSE(cur_reading.z > max_z)) max_z = cur_reading.z;

        if (ENDORSE(cur_reading.x < min_x)) min_x = cur_reading.x;
        if (ENDORSE(cur_reading.y < min_y)) min_y = cur_reading.y;
        if (ENDORSE(cur_reading.z < min_z)) min_z = cur_reading.z;

        // TODO: moving average
    }

    return 19; // lucky 0x13
}
