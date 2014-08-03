#include <msp430.h>
#include <stdlib.h>
#include <enerc.h>
#include <wisp5.h>
#include <accel.h>
#include <spi.h>
#include <math.h>
#include <perfctr.h>

#define MODEL_SIZE 190
#define MODEL_SIZE_PLUS_WARMUP (MODEL_SIZE+10)
#define ACCEL_WINDOW_SIZE 4
#undef FLASH_ON_BOOT

// number of samples until experiment is "done" and "moving" light goes on
#define SAMPLES_TO_COLLECT 5000

// two features: mean & stdev
#define NUM_FEATURES 2

static const unsigned stationary[MODEL_SIZE] = {
#include "int_wisp5_stationary.h"
};

static const unsigned moving[MODEL_SIZE] = {
#include "int_wisp5_flapping.h"
};

#ifdef USE_TRACE
static const __fram threeAxis_t trace[SAMPLES_TO_COLLECT] = {
#include "readings5k.h"
};
static unsigned traceIndex = 0;
#endif // USE_TRACE

/* These "pinned" variables are fixed at these addresses. */
APPROX volatile __fram unsigned int __NV_movingCount;
APPROX volatile __fram unsigned int __NV_stationaryCount;
volatile __fram unsigned int __NV_totalCount;
APPROX volatile __fram float __NV_movingPct;
APPROX volatile __fram float __NV_stationaryPct;

APPROX static long int aWin[ACCEL_WINDOW_SIZE][3];
static unsigned currSamp = 0;

APPROX static long int mean[3];
APPROX static long int stddev[3];

#ifdef TRAINING
static int modelEntry = 0;
static long int model[MODEL_SIZE_PLUS_WARMUP];
#endif  // TRAINING

APPROX static long int meanmag;
APPROX static long int stddevmag;

void getNextSample() {
#ifdef USE_TRACE
  const threeAxis_t *threeAxis = &trace[traceIndex++];
  aWin[currSamp][0] = (long)threeAxis->x;
  aWin[currSamp][1] = (long)threeAxis->y;
  aWin[currSamp][2] = (long)threeAxis->z;
  if (traceIndex > SAMPLES_TO_COLLECT) {
    traceIndex = 0;
  }
  __delay_cycles(500);
#else
  threeAxis_t threeAxis;
  ACCEL_singleSample(&threeAxis);  // ACCEPT_PERMIT

  aWin[currSamp][0] = (long)threeAxis.x;
  aWin[currSamp][1] = (long)threeAxis.y;
  aWin[currSamp][2] = (long)threeAxis.z;
#endif // USE_TRACE

  if (++currSamp >= ACCEL_WINDOW_SIZE) {
    currSamp = 0;
  }
}

void featurize() {
  mean[0] = mean[1] = mean[2] = 0;
  stddev[0] = stddev[1] = stddev[2] = 0;
  unsigned i;

  /* compute the average accel value in the window of accel values.  use right
   * shift by 2 instead of division by 4 because mspgcc is too dumb to convert
   * the latter to the former. */
  for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
    mean[0] += aWin[i][0];  // x
    mean[1] += aWin[i][1];  // y
    mean[2] += aWin[i][2];  // z
  }
  mean[0] >>= 2;
  mean[1] >>= 2;
  mean[2] >>= 2;

  /* compute the (nonstandard) deviation of the values in the window versus the
   * mean computed above.  this is actually the mean distance from the
   * population mean. */
  for (i = 0; i < ACCEL_WINDOW_SIZE; i++) {
    stddev[0] += aWin[i][0] > mean[0] ? aWin[i][0] - mean[0]
                                      : mean[0] - aWin[i][0];  // x
    stddev[1] += aWin[i][1] > mean[1] ? aWin[i][1] - mean[1]
                                      : mean[1] - aWin[i][1];  // y
    stddev[2] += aWin[i][2] > mean[2] ? aWin[i][2] - mean[2]
                                      : mean[2] - aWin[i][2];  // z
  }
  stddev[0] >>= 2;
  stddev[1] >>= 2;
  stddev[2] >>= 2;

  /* compute the magnitude of each feature vector. */
  APPROX float meanmag_f = (float)
    ((mean[0]*mean[0]) + (mean[1]*mean[1]) + (mean[2]*mean[2]));
  APPROX float stddevmag_f = (float)
    ((stddev[0]*stddev[0]) + (stddev[1]*stddev[1]) + (stddev[2]*stddev[2]));

  meanmag_f   = sqrtf(meanmag_f);
  stddevmag_f = sqrtf(stddevmag_f);

  meanmag   = (long)meanmag_f;
  stddevmag = (long)stddevmag_f;
}

#define MODEL_COMPARISONS 20
APPROX int classify() {
  APPROX int move_less_error = 0;
  APPROX int stat_less_error = 0;
  APPROX long int stat_mean_err, stat_sd_err, move_mean_err, move_sd_err;
  int i;

  /* classify the current sample (stored in meanmag, stddevmag by featurize())
   * as stationary or moving based on its relative similarity to the first
   * MODEL_COMPARISONS entries of the moving and stationary models. */
  for (i = 0; ENDORSE(i < MODEL_COMPARISONS); i += NUM_FEATURES) {
    stat_mean_err = (stationary[i] > meanmag)
                      ? (stationary[i] - meanmag)
                      : (meanmag - stationary[i]);

    stat_sd_err = (stationary[i + 1] > stddevmag)
                    ? (stationary[i + 1] - stddevmag)
                    : (stddevmag - stationary[i + 1]);

    move_mean_err = (moving[i] > meanmag)
                      ? (moving[i] - meanmag)
                      : (meanmag - moving[i]);

    move_sd_err = (moving[i + 1] > stddevmag)
                     ? (moving[i + 1] - stddevmag)
                     : (stddevmag - moving[i + 1]);

    if (ENDORSE(move_mean_err < stat_mean_err)) {
      move_less_error++;
    } else {
      stat_less_error++;
    }

    if (ENDORSE(move_sd_err < stat_sd_err)) {
      move_less_error++;
    } else {
      stat_less_error++;
    }
  }

  if (ENDORSE(move_less_error > stat_less_error)) {
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

  /* TODO: move the below stuff to accel.c */
  BITSET(PDIR_AUX3, PIN_AUX3);
  __delay_cycles(50);
  BITCLR(P1OUT, PIN_AUX3);
  __delay_cycles(50);
  BITSET(P1OUT, PIN_AUX3);
  __delay_cycles(50);

  BITSET(P4SEL1, PIN_ACCEL_EN);
  BITSET(P4SEL0, PIN_ACCEL_EN);

#ifndef USE_TRACE
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
#endif // USE_TRACE
}

__attribute__((section(".init9"), aligned(2)))
int main(void) {
  WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;

  initializeHardware();
  initializeNVData();

  accept_roi_begin();

  while (1) {

    if( __NV_totalCount > SAMPLES_TO_COLLECT ){
      break;
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

    APPROX int class = classify();
    /* __NV_totalCount, __NV_movingCount, and __NV_stationaryCount have an
     * nv-internal consistency requirement.  This code should be atomic. */

    __NV_totalCount++;


    if (!ENDORSE(class)) {

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

  accept_roi_end();

  /* Light LEDs and loop forever */
  P4OUT |= BIT0;
  PJOUT &= ~BIT6;
  asm volatile ("MOV %0, R8\n"
                "MOV %1, R7" ::"m"(__NV_movingCount), "m"(__NV_totalCount));
  while(1);
}

/* vim: set ai et ts=2 sw=2: */
