/* See license.txt for license information. */

#ifndef MOO_H
#define MOO_H

// Hardware definitions for Moo 1.0 and 1.1 (MSP430F2618)
// (derived from Intel WISP 4.1 DL ["Blue WISP"] definitions)
// See the schematics/ directory for pinouts and diagrams.

#include <msp430x26x.h>
#define USE_2618  1

// Port 1
#define TEMP_POWER     BIT0       // output
#define TX_PIN         BIT1       // output
#define RX_PIN         BIT2       // input
#define RX_EN_PIN      BIT3       // output
#define ACCEL_POWER    BIT4       // output
#define DEBUG_1_5      BIT5       // output/input
#define DEBUG_1_6      BIT6       // output/input
#define DEBUG_1_7      BIT7       // output/input

// Port 2
//Supervisor in: PCB is designed to P6.7, should wire connect P6.7 and P2.0
#define VOLTAGE_SV_PIN      BIT0       // output/input
#define DEBUG_2_1      BIT1       // output/input
#define DEBUG_2_2      BIT2       // output/input
#define IDLE_2_3       BIT3       // idle pin
#define IDLE_2_4       BIT4       // idle pin
#define IDLE_2_5       BIT5       // idle pin
#define IDLE_2_6       BIT6       // idle pin
#define IDLE_2_7       BIT7       // idle pin

// Port 3
#define CLK_A          BIT0       // output unless externally driven
#define SDA_B          BIT1       // input (connected to 10k pullup res)
#define SCL_B          BIT2       // input (connected to 10k pullup res)
#define CLK_B          BIT3       // output unless externally driven
#define TX_A           BIT4       // output unless externally driven
#define RX_A           BIT5       // output unless externally driven
#define DEBUG_3_6      BIT6       // output/input
#define DEBUG_3_7      BIT7       // output/input

// Port 4
#define DEBUG_4_0      BIT0       // connect to SV_IN by 0 ohm
#define CAP_SENSE      BIT1       // output/input
#define LED_POWER      BIT2       // output
#define VSENSE_POWER   BIT3       // output
#define DEBUG_4_4      BIT4       // output/input
#define DEBUG_4_5      BIT5       // output/input
#define DEBUG_4_6      BIT6       // output/input
#define DEBUG_4_7      BIT7       // output/input

// Port 5
#define FLASH_CE       BIT0       // output
#define FLASH_SIMO     BIT1       // output
#define FLASH_SOMI     BIT2       // input
#define FLASH_SCK      BIT3       // output
#define DEBUG_5_4      BIT4       // output unless externally driven
#define DEBUG_5_5      BIT5       // output unless externally driven
#define IDLE_5_6       BIT6       // idle pin
#define DEBUG_5_7      BIT7       // output unless externally driven

// Port 6
#define ACCEL_Z        BIT0       // input
#define ACCEL_Y        BIT1       // input
#define ACCEL_X        BIT2       // input
#define TEMP_EXT_IN    BIT3       // input
#define VSENSE_IN      BIT4       // input
#define DEBUG_6_5      BIT5       // output unless externally driven
#define DEBUG_6_6      BIT6       // output unless externally driven
#define DEBUG_6_7      BIT7       // input

// Port 8: Zhangh, need to reconfirm
#define CRYSTAL_IN     BIT7       // input
#define CRYSTAL_OUT    BIT6       // output

// Analog Inputs (ADC In Channel)
#define INCH_ACCEL_Z     INCH_0
#define INCH_ACCEL_Y     INCH_1
#define INCH_ACCEL_X     INCH_2
#define INCH_TEMP_EXT_IN INCH_3
#define INCH_VSENSE_IN   INCH_4
#define INCH_DEBUG_6_5   INCH_5
#define INCH_DEBUG_6_6   INCH_6


#define DRIVE_ALL_PINS  \
  P1OUT = 0;  \
  P2OUT = 0;  \
  P3OUT = 0;  \
  P4OUT = 0;  \
  P5OUT = 0;  \
  P6OUT = 0;  \
  P8OUT = 0;  \
  P1DIR = TEMP_POWER | ACCEL_POWER | TX_PIN | RX_EN_PIN; \
  P4DIR = CAP_SENSE | LED_POWER | VSENSE_POWER; \
  P5DIR = FLASH_CE | FLASH_SIMO | FLASH_SCK; \
  P8DIR = CRYSTAL_OUT;

#define SEND_CLOCK  \
  BCSCTL1 = XT2OFF + RSEL3 + RSEL0 ; \
    DCOCTL = DCO2 + DCO1 ;
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

#define DEBUG_PINS_ENABLED            0
#if DEBUG_PINS_ENABLED
#define DEBUG_PIN5_HIGH               P3OUT |= BIT5;
#define DEBUG_PIN5_LOW                P3OUT &= ~BIT5;
#else
#define DEBUG_PIN5_HIGH
#define DEBUG_PIN5_LOW
#endif

void setup_to_receive();
void sleep();
unsigned short is_power_good();
#if ENABLE_SLOTS
void lfsr();
void loadRN16(), mixupRN16();
#endif // ENABLE_SLOTS
void crc16_ccitt_readReply(unsigned int);

#endif // MOO_H
