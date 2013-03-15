/* Read an accelerometer repeatedly in a loop.  Most code adapted from the UMass
 * Moo CRFID's firmware. */

#include <enerc.h>
#include <msp430.h>
#include "moo.h"

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

inline unsigned read_accel_channel (unsigned channel) {
    ADC12CTL0 &= ~ENC;
    ADC12CTL0 = ADC12ON + SHT0_1;
    ADC12CTL1 = SHP;
    ADC12MCTL0 = channel + SREF_0; // Vr+ = AVcc = Vreg (= 1.8V)
    ADC12CTL0 |= ENC;
    ADC12CTL0 |= ADC12SC; // take a sample
    while (ADC12CTL1 & ADC12BUSY);
    return ADC12MEM0;
}

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
    P6SEL = 0;
    P6OUT &= ~(ACCEL_X | ACCEL_Y | ACCEL_Z);
    P6DIR |=   ACCEL_X | ACCEL_Y | ACCEL_Z;
    P6DIR &= ~(ACCEL_X | ACCEL_Y | ACCEL_Z);

    P1DIR |= ACCEL_POWER;
    P1OUT |= ACCEL_POWER;
    P6SEL |= ACCEL_X | ACCEL_Y | ACCEL_Z;

    // a little time for regulator to stabilize active mode current AND
    // filter caps to settle.
    for(i = 0; i < 225; i++);
    RECEIVE_CLOCK;

    // GRAB DATA
    ADC12CTL0 &= ~ENC; // make sure this is off otherwise settings are locked.
    ADC12CTL0 = ADC12ON + SHT0_1;                     // Turn on and set up ADC12
    ADC12CTL1 = SHP;                                  // Use sampling timer
    ADC12MCTL0 = INCH_ACCEL_X + SREF_0;               // Vr+=AVcc=Vreg=1.8V
    // ADC12CTL1 =  + ADC12SSEL_0 + SHS_0 + CONSEQ_0;
    ADC12CTL0 |= ENC;
    ADC12CTL0 |= ADC12SC;
    while (ADC12CTL1 & ADC12BUSY);    // wait while ADC finishes work

    reading->x = read_accel_channel(INCH_ACCEL_X);
    reading->y = read_accel_channel(INCH_ACCEL_Y);
    reading->z = read_accel_channel(INCH_ACCEL_Z);

    // Power off sensor and adc
    P1DIR &= ~ACCEL_POWER;
    P1OUT &= ~ACCEL_POWER;
    ADC12CTL0 &= ~ENC;
    ADC12CTL1 = 0;       // turn adc off
    ADC12CTL0 = 0;       // turn adc off

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

int main (void) {
    accel_reading cur_reading;
    APPROX unsigned max_x, max_y, max_z;
    APPROX unsigned min_x, min_y, min_z;
    APPROX unsigned avg_x, avg_y, avg_z;
    unsigned i;

    for (i = 0; i < NUM_READINGS; ++i) {
        read_sensor(&cur_reading);

        if (ENDORSE(cur_reading).x > max_x) max_x = cur_reading.x;
        if (ENDORSE(cur_reading).y > max_y) max_y = cur_reading.y;
        if (ENDORSE(cur_reading).z > max_z) max_z = cur_reading.z;

        if (ENDORSE(cur_reading).x < min_x) min_x = cur_reading.x;
        if (ENDORSE(cur_reading).y < min_y) min_y = cur_reading.y;
        if (ENDORSE(cur_reading).z < min_z) min_z = cur_reading.z;

        // TODO: moving average
    }

    return 19; // lucky 0x13
}
