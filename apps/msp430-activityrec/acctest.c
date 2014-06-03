#include "adxl362z.h"
#include <msp430.h>

volatile unsigned char vid;
volatile unsigned int x;
volatile unsigned int y;
volatile unsigned int z;
int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

  ACCEL_setup();
 
  ACCEL_SetReg(0x2D,0x02); /*Configure the POWER_CTL to measurement mode*/

  while(1) {
    vid = ACCEL_ReadReg(0x00);
    x = ACCEL_getX();
    y = ACCEL_getY();
    z = ACCEL_getZ();
    int i;
    for(i = 0; i < 2000; i++);
  }
}
