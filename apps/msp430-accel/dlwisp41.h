/* See license.txt for license information. */

#ifndef DLWISP41_H
#define DLWISP41_H

// Pin definitions
// WISP 4.1 DL
// "Blue WISP"

// MSP430F2132
#include <msp430x21x2.h>
#define USE_2132  1

// See wisp.wikispaces.com for a schematic.

// Port 1
#define TEMP_POWER     BIT0       // output
#define TX_PIN         BIT1       // output
#define RX_PIN         BIT2       // input
#define RX_EN_PIN      BIT3       // output
#define DEBUG_1_4      BIT4       // output unless externally driven
#define ACCEL_POWER    BIT5       // output
#define LED_POWER      BIT6       // output
#define CAP_SENSE      BIT7       // output/input

// Port 2
#define ACCEL_Z        BIT0       // input
#define ACCEL_Y        BIT1       // input
#define ACCEL_X        BIT2       // input
#define DEBUG_2_3      BIT3       // output unless externally driven
#define VOLTAGE_SV_PIN BIT4       // input
#define DEBUG_2_5      BIT5       // connect to SV_IN by 0 ohm
#define CRYSTAL_IN     BIT6       // input
#define CRYSTAL_OUT    BIT7       // output

// Port 3
#define CLK_A          BIT0       // output unless externally driven
#define SDA_B          BIT1       // input (connected to 10k pullup res)
#define SCL_B          BIT2       // input (connected to 10k pullup res)
#define VSENSE_POWER   BIT3       // output
#define TX_A           BIT4       // output unless externally driven
#define RX_A           BIT5       // output unless externally driven
#define VSENSE_IN      BIT6       // input
#define TEMP_EXT_IN    BIT7       // input

// Analog Inputs (ADC In Channel)
#define INCH_ACCEL_Z     INCH_0
#define INCH_ACCEL_Y     INCH_1
#define INCH_ACCEL_X     INCH_2
#define INCH_DEBUG_2_3   INCH_3
#define INCH_VSENSE_IN   INCH_6
#define INCH_TEMP_EXT_IN INCH_7

//#define INCH_2_4 INCH_4   // not accessible
// #define INCH_3_5 INCH_5  // ??

#define DRIVE_ALL_PINS  \
  P1OUT = 0;  \
  P2OUT = 0;  \
  P3OUT = 0;  \
  P1DIR = TEMP_POWER | TX_PIN | RX_EN_PIN | DEBUG_1_4 | LED_POWER | CAP_SENSE; \
  P2DIR = DEBUG_2_3 | CRYSTAL_OUT; \
  P3DIR = CLK_A | VSENSE_POWER | TX_A | RX_A;

#define SEND_CLOCK  \
  BCSCTL1 = XT2OFF + RSEL3 + RSEL0 ; \
    DCOCTL = DCO2 + DCO1 ;
  //BCSCTL1 = XT2OFF + RSEL3 + RSEL1 ; \
  //DCOCTL = 0;
#define RECEIVE_CLOCK \
  BCSCTL1 = XT2OFF + RSEL3 + RSEL1 + RSEL0; \
  DCOCTL = 0; \
  BCSCTL2 = 0; // Rext = ON

#define STATE_READY               0
#define STATE_ARBITRATE           1
#define STATE_REPLY               2
#define STATE_ACKNOWLEDGED        3
#define STATE_OPEN                4
#define STATE_SECURED             5
#define STATE_KILLED              6
#define STATE_READ_SENSOR         7

#if DEBUG_PINS_ENABLED
#define DEBUG_PIN5_HIGH               P3OUT |= BIT5;
#define DEBUG_PIN5_LOW                P3OUT &= ~BIT5;
#else
#define DEBUG_PIN5_HIGH
#define DEBUG_PIN5_LOW
#endif

#if ENABLE_SESSIONS
void initialize_sessions();
void handle_session_timeout();
int bitCompare(unsigned char *, unsigned short, unsigned char *,
               unsigned short, unsigned short);
#endif // ENABLE_SESSIONS
void setup_to_receive();
void sleep();
unsigned short is_power_good();
#if ENABLE_SLOTS
void lfsr();
void loadRN16(), mixupRN16();
#endif // ENABLE_SLOTS
void crc16_ccitt_readReply(unsigned int);

#endif // DLWISP41_H
