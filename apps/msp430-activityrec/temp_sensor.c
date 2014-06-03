#include <msp430.h>
#define TF_TEMP_15_30 2982
#define TF_TEMP_15_85 3515
#define TLVStruct(x)  *(&(((int*)TLV_ADC12_1_TAG_)[x+1]))

#define INTERNAL1V2 (ADC12VRSEL_1 | REFON | REFVSEL_0)
#define REF_MASK 0x31
#define REFV_MASK 0x0F00
unsigned int CAL_ADC_12T30;
unsigned int CAL_ADC_12T85;
int t30;
int t85;
float CAL_TEMP_SCALE_100X;

unsigned char sensor_busy = 0;

void init_sensor() {

  /*made up constants.  these should come from 0x1A1A but my devices is missing calibration constants*/
  t30 = CAL_ADC_12T30 = *((unsigned int *)0x1A1A);
  t85 = CAL_ADC_12T85 = 3515;

  return;

}

unsigned int get_calibrated_adc () {

  volatile unsigned int tmptemp;

  ADC12CTL0 &= ~ADC12ENC;
  ADC12CTL0 = ADC12ON + ADC12SHT0_4;
  ADC12CTL1 = ADC12SSEL_0 | ADC12DIV_4;
  ADC12CTL3 = ADC12TCMAP | ADC12BATMAP;
  ADC12CTL1 = ADC12SHP;
  ADC12CTL2 = ADC12RES_2;
  ADC12IFGR0 = 0;
  ADC12IER0 = ADC12IE0;
  while( REFCTL0 & REFGENBUSY );
  REFCTL0 = INTERNAL1V2 & REF_MASK;
  ADC12MCTL0 = ADC12INCH_30 | (INTERNAL1V2 & REFV_MASK);
  if(REFCTL0 & REFON){
    while(!(REFCTL0 & REFGENRDY));
  }
  ADC12CTL0 |= ADC12ENC | ADC12SC;
  while( ADC12CTL1 & ADC12BUSY );

  ADC12CTL0 &= ~(ADC12ON);
  REFCTL0 &= ~(REFON);

  tmptemp = ADC12MEM0;

  int tempCmy = ((float)tmptemp - (float)t30)  *  (  55. / ((float)(t85-t30))  )  + 30;

  #define ADC_TO_dC(_v) (3000 + (int)((85 - 30) * ((100L * ((int)(_v) - t30)) / (long)(t85 - t30))))
  int tempC = ADC_TO_dC(tmptemp);

  #define dC_TO_dF(_v) (3200 + (9 * (_v)) / 5)
  int tempF = dC_TO_dF(tempC);

  return tempF;

}

void read_sensor(unsigned int volatile *target) {

  unsigned int temp = get_calibrated_adc();

  *target = temp;

  return;

}

