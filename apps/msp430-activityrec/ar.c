#include <msp430.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <wisp5.h>
#include <accel.h>
#include <spi.h>
#include <math.h>
#include <enerc.h>

#define MODEL_SIZE 190
#define MODEL_SIZE_PLUS_WARMUP (MODEL_SIZE+10)
#define ACCEL_WINDOW_SIZE 4
#undef FLASH_ON_BOOT

// number of samples until experiment is "done" and "moving" light goes on
#define SAMPLES_TO_COLLECT 10000

// two features: mean & stdev
#define NUM_FEATURES 2

static const unsigned stationary[MODEL_SIZE] = {
#include "int_wisp5_stationary.h"
};

static const unsigned moving[MODEL_SIZE] = {
#include "int_wisp5_flapping.h"
};

/* These "pinned" variables are fixed at these addresses. */
APPROX extern volatile unsigned int __NV_movingCount     asm("0xD000");
APPROX extern volatile unsigned int __NV_stationaryCount asm("0xD002");
extern volatile unsigned int __NV_totalCount      asm("0xD004");
APPROX extern volatile float __NV_movingPct              asm("0xD006");
APPROX extern volatile float __NV_stationaryPct          asm("0xD00A");

typedef long int accelReading[3];
typedef accelReading accelWindow[ACCEL_WINDOW_SIZE];

static accelWindow aWin;
static int currSamp = 0;

APPROX static accelReading mean;
APPROX static accelReading stddev;

#ifdef TRAINING
static int modelEntry = 0;
static long int model[MODEL_SIZE_PLUS_WARMUP];
#endif  // TRAINING

APPROX static long int meanmag;
APPROX static long int stddevmag;

static int g_x, g_y, g_z;
static threeAxis_t accelOut;

void getOneSample(accelReading tr) {
  threeAxis_t threeAxis;
  ACCEL_singleSample(&threeAxis);

  // HACK: set globals to the accelerometer values
  g_x = threeAxis.x;
  g_y = threeAxis.y;
  g_z = threeAxis.z;

  tr[0] = (long)threeAxis.x;
  tr[1] = (long)threeAxis.y;
  tr[2] = (long)threeAxis.z;
}

void getNextSample() {
  getOneSample(aWin[currSamp++]);
  if (currSamp >= ACCEL_WINDOW_SIZE) {
    currSamp = 0;
  }
}

APPROX void featurize() {
  mean[0] = mean[1] = mean[2] = 0;
  stddev[0] = stddev[1] = stddev[2] = 0;
  int i;
  for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
    mean[0] += aWin[i][0];  // x
    mean[1] += aWin[i][1];  // y
    mean[2] += aWin[i][2];  // z
  }
  /*
  mean[0] = mean[0] / ACCEL_WINDOW_SIZE;
  mean[1] = mean[1] / ACCEL_WINDOW_SIZE;
  mean[2] = mean[2] / ACCEL_WINDOW_SIZE;
  */
  mean[0] >>= 2;
  mean[1] >>= 2;
  mean[2] >>= 2;

  for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
    stddev[0] += aWin[i][0] > mean[0] ? aWin[i][0] - mean[0]
                                      : mean[0] - aWin[i][0];  // x
    stddev[1] += aWin[i][1] > mean[1] ? aWin[i][1] - mean[1]
                                      : mean[1] - aWin[i][1];  // y
    stddev[2] += aWin[i][2] > mean[2] ? aWin[i][2] - mean[2]
                                      : mean[2] - aWin[i][2];  // z
  }
  /*
  stddev[0] = stddev[0] / (ACCEL_WINDOW_SIZE - 1);
  stddev[1] = stddev[1] / (ACCEL_WINDOW_SIZE - 1);
  stddev[2] = stddev[2] / (ACCEL_WINDOW_SIZE - 1);
  */
  stddev[0] >>= 2;
  stddev[1] >>= 2;
  stddev[2] >>= 2;

  float meanmag_f = (float)
    ((mean[0]*mean[0]) + (mean[1]*mean[1]) + (mean[2]*mean[2]));
  float stddevmag_f = (float)
    ((stddev[0]*stddev[0]) + (stddev[1]*stddev[1]) + (stddev[2]*stddev[2]));

  meanmag_f   = sqrtf(meanmag_f);
  stddevmag_f = sqrtf(stddevmag_f);

  meanmag   = (long)meanmag_f;
  stddevmag = (long)stddevmag_f;
}

#define MODEL_COMPARISONS 10
int classify() {
  int move_less_error = 0;
  int stat_less_error = 0;
  int i;

  for (i = 0; i < MODEL_COMPARISONS; i += NUM_FEATURES) {
    long int stat_mean_err = (stationary[i] > meanmag)
                                 ? (stationary[i] - meanmag)
                                 : (meanmag - stationary[i]);

    long int stat_sd_err = (stationary[i + 1] > stddevmag)
                               ? (stationary[i + 1] - stddevmag)
                               : (stddevmag - stationary[i + 1]);

    long int move_mean_err = (moving[i] > meanmag) ? (moving[i] - meanmag)
                                                    : (meanmag - moving[i]);

    long int move_sd_err = (moving[i + 1] > stddevmag)
                               ? (moving[i + 1] - stddevmag)
                               : (stddevmag - moving[i + 1]);

    if (move_mean_err < stat_mean_err) {
      move_less_error++;
    } else {
      stat_less_error++;
    }

    if (move_sd_err < stat_sd_err) {
      move_less_error++;
    } else {
      stat_less_error++;
    }
  }

  if (move_less_error > stat_less_error) {
    return 1;
  } else {
    return 0;
  }
}

void initializeNVData() {
  /*The erase-initial condition*/
  if (ENDORSE((__NV_movingCount == 0xffff && __NV_stationaryCount == 0xffff &&
      __NV_totalCount == 0xffff))) {

    __NV_movingCount = 0;
    __NV_stationaryCount = 0;
    __NV_totalCount = 0;
  }
}

#ifdef __clang__
void __delay_cycles(unsigned long cyc) {
  unsigned i;
  for (i = 0; i < (cyc >> 3); ++i)
    ;
}
#endif

void initializeHardware() {
  setupDflt_IO();

  PRXEOUT |=
      PIN_RX_EN; /** TODO: enable PIN_RX_EN only when needed in the future */

  // set clock speed to 4 MHz
  CSCTL0_H = 0xA5;
  CSCTL1 = DCOFSEL0 | DCOFSEL1;
  CSCTL2 = SELA_0 | SELS_3 | SELM_3;
  CSCTL3 = DIVA_0 | DIVS_0 | DIVM_0;

  /*Before anything else, do the device hardware configuration*/

  P4DIR |= BIT0;
  PJDIR |= BIT6;
#if defined(USE_LEDS) && defined(FLASH_ON_BOOT)
  P4OUT |= BIT0;
  PJOUT |= BIT6;
  int i;
  for (i = 0; i < 0xffff; i++)
    ;
  P4OUT &= ~BIT0;  // Toggle P4.4 using exclusive-OR
  PJOUT &= ~BIT6;  // Toggle P4.5 using exclusive-OR
#endif

  /*
  SPI_initialize();
  ACCEL_initialize();
  */
  // ACCEL_SetReg(0x2D,0x02);

  /* TODO: move the below stuff to accel.c */
  BITSET(PDIR_AUX3, PIN_AUX3);
  __delay_cycles(50);
  BITCLR(P1OUT, PIN_AUX3);
  __delay_cycles(50);
  BITSET(P1OUT, PIN_AUX3);
  __delay_cycles(50);

  BITSET(P4SEL1, PIN_ACCEL_EN);
  BITSET(P4SEL0, PIN_ACCEL_EN);

  accelOut.x = 1;
  accelOut.y = 1;
  accelOut.z = 1;
  BITSET(P2SEL1, PIN_ACCEL_SCLK | PIN_ACCEL_MISO | PIN_ACCEL_MOSI);
  BITCLR(P2SEL0, PIN_ACCEL_SCLK | PIN_ACCEL_MISO | PIN_ACCEL_MOSI);
  __delay_cycles(5);
  SPI_initialize();
  __delay_cycles(5);
  ACCEL_range();
  __delay_cycles(5);
  ACCEL_initialize();
  __delay_cycles(5);
  ACCEL_readID(&accelOut);
}

__attribute__((section(".init9"), aligned(2)))
int main(int argc, char *argv[]) {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;

  initializeHardware();
  initializeNVData();
  while (1) {

    if( __NV_totalCount > SAMPLES_TO_COLLECT ){
      P4OUT |= BIT0;
      PJOUT &= ~BIT6;
      while(1);
    }

    getNextSample();
    featurize();

#ifdef TRAINING
    if (modelEntry < MODEL_SIZE_PLUS_WARMUP) {
      model[modelEntry] = meanmag;
      model[modelEntry + 1] = stddevmag;
      modelEntry += 2;
    } else {
      asm("nop");
    }
#endif

    int class = classify();

    /* __NV_totalCount, __NV_movingCount, and __NV_stationaryCount have an
     * nv-internal consistency requirement.  This code should be atomic. */

    __NV_totalCount++;


    if (class) {

#if defined (USE_LEDS)
      PJOUT &= ~BIT6;
      P4OUT |= BIT0;
#endif //USE_LEDS
      __NV_movingPct =
        ((float)__NV_movingCount / (float)__NV_totalCount) * 100.0f;
      (__NV_movingCount)++;

    } else {

#if defined (USE_LEDS)
      P4OUT &= ~BIT0;  // Toggle P1.0 using exclusive-OR
      PJOUT |= BIT6;
#endif //USE_LEDS
      __NV_stationaryPct =
        ((float)__NV_stationaryCount / (float)__NV_totalCount) * 100.0f;
      (__NV_stationaryCount)++;

    }
  }
}

/* vim: set ai et ts=2 sw=2: */
