//                   MSP430FR5969
//                 -----------------
//             /|\|                 |
//              | |                 |  
//              --|RST              |
//                |                 |
//                |             P2.0|-> Data Out to Slave (UCA0SIMO)
//                |                 |
//                |             P2.1|<- Data In from Slave (UCA0SOMI)
//                |                 |
//                |             P1.5|-> Serial Clock Out to Slave (UCA0CLK)
//                |             P4.0|-> Slave Active when Low  
#include <msp430.h>

static volatile unsigned char vid = 0;
static volatile unsigned char power_ctl = 0;
static volatile unsigned char XDataH = 0;
static volatile unsigned char XDataL = 0;
static volatile unsigned char YDataH = 0;
static volatile unsigned char YDataL = 0;
static volatile unsigned char ZDataH = 0;
static volatile unsigned char ZDataL = 0;

inline void ACCEL_SoftReset(){
   
    volatile unsigned char data;
    P4OUT &= ~BIT0; // set CS to low 
    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = 0x0A;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;

    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = 0x1F;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;

    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = 0x52;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;
    P4OUT |= BIT0; // set CS to low 

}

inline void ACCEL_SetReg(unsigned char reg, unsigned char val){

    /*write the value of reg 2d to 2*/    
    volatile unsigned char data;
    P4OUT &= ~BIT0; // set CS to low 
    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = 0x0A;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;

    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = reg;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;

    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = val;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;
    P4OUT |= BIT0; // set CS to low 

}

inline unsigned char ACCEL_ReadReg(unsigned char reg){

    /*read the value of reg 2d to 2*/    
    volatile unsigned char data;
    P4OUT &= ~BIT0; // set CS to low 
    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = 0x0B;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;

    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = reg;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;

    while(UCA0STATW & UCBUSY);
    UCA0TXBUF = 0x00;               // Transmit characters
    while(UCA0STATW & UCBUSY);
    data = UCA0RXBUF;
    P4OUT |= BIT0; // set CS to low 
 
    return data;

}

int ACCEL_getX(){
  XDataL = ACCEL_ReadReg(0x0E);
  XDataH = ACCEL_ReadReg(0x0F);
   
  return (int)((XDataH << 8)|XDataL);
}

int ACCEL_getY(){
  YDataL = ACCEL_ReadReg(0x10);
  YDataH = ACCEL_ReadReg(0x11);
  return (int)((YDataH << 8)|YDataL);
}

int ACCEL_getZ(){
  ZDataL = ACCEL_ReadReg(0x12);
  ZDataH = ACCEL_ReadReg(0x13);
  return (int)((ZDataH << 8)|ZDataL);
}

void ACCEL_setup(){

  /*The slave select bit will run out of P4.0*/ 
  /*Set P4.0 to the output direction*/
  P4DIR  |= BIT0; // Configure CS on P4.0 
  /*Set P4.0 to high, meaning the slave is inactive*/
  P4OUT  |= BIT0; // set CS to high for now 

  /*
    This sets pins' module selectors to they're connected
    to the SPI-related modules internally.  Module selector bits come 
    from the device specific datasheet
  */
  /*Pins P1.4 and P1.5  are the CS and CLK for the SPI*/
  P1SEL1 |= BIT5 | BIT4;                    
  /*Pins P2.0 and P2.1  are the SOMI and SIMO for the SPI*/
  P2SEL1 |= BIT0 | BIT1;                    

  // Disable the GPIO power-on default high-impedance mode to activate
  // previously configured port settings
  PM5CTL0 &= ~LOCKLPM5;

  

  /*Unlock the clock configuration registers*/
  CSCTL0_H = CSKEY >> 8;                   

  /*These two lines set the digital oscilator to 1MHz.
    The 1MHz frequency is listed in the device specific datasheet */
  CSCTL1 = DCOFSEL_0;    
  CSCTL1 &= ~DCORSEL;    

  /*The next line sets the clock sources.  
    Aux clock is set to VLO - ~10kHz
    Sub-module clock is set to DCO - 1MHz
    Master clock is set to DCO - 1MHz
    It's probably adequate to just set the SM here because
    that's all the SPI uses, but I've set them all so the 
    config register is an assignment not an OR/Assignment
  */
  CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK; 

  /*Set the clock dividers to 1 so the clocks are as set above*/
  CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     

  /*Re-lock the clock configuration registers*/
  CSCTL0_H = 0;                             

  // Configure USCI_A0 for SPI operation
  UCA0CTLW0 = UCSWRST;                      // **Put state machine in reset**
  
  /*
    UCMST -> this is the master
    UCSYNC -> Synchronous communication with slave 
    UCCKPL -> clock polarity is 'low' -- unsure what this one means
    UCMSB -> send most significant bits first
    UCMODE_2 -> 4 pin master mode with slave select low while slave active
    UCSTEM -> UCxxSTE pin generates slave select signal
  */
  UCA0CTLW0 |= UCMST | UCSYNC | UCCKPL | UCMSB | UCMODE_2 | UCSTEM;
                                            // Clock polarity high, MSB
  UCA0CTLW0 |= UCSSEL_2;                    // SMCLK
  UCA0BR0 = 104;                            // 1MHz / 104 = 9615.4 ~= 9600bps 
  UCA0BR1 = 0;                              //
  UCA0MCTLW = 0;                            // No modulation (?)
  UCA0CTLW0 &= ~UCSWRST;                    // **Initialize USCI state machine**

  /*
    This uses SPI to reset the accelerometer, which is good to do as
    the msp as just reset itself.
  */  
  ACCEL_SoftReset();

}

