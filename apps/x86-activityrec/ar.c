#include <stdlib.h>
//#include <stdarg.h>
#include <stdio.h>
//#include <wisp5.h>
//#include <accel.h>
//#include <spi.h>
#include <string.h>
#include <math.h>
#include <enerc.h>
//#include <perfctr.h>
#include <unistd.h>

#define MODEL_SIZE 190
#define MODEL_SIZE_PLUS_WARMUP (MODEL_SIZE+10)
#define ACCEL_WINDOW_SIZE 4
#undef FLASH_ON_BOOT

// number of samples until experiment is "done" and "moving" light goes on
#define SAMPLES_TO_COLLECT 2500

// two features: mean & stdev
#define NUM_FEATURES 2

static const uint16_t stationary[MODEL_SIZE] = {
#include "int_wisp5_stationary.h"
};

static const uint16_t moving[MODEL_SIZE] = {
#include "int_wisp5_flapping.h"
};

/* These "pinned" variables are fixed at these addresses. */
APPROX volatile uint16_t __NV_movingCount;
APPROX volatile uint16_t __NV_stationaryCount;
volatile uint16_t __NV_totalCount;
APPROX volatile float __NV_movingPct;
APPROX volatile float __NV_stationaryPct;

APPROX static int32_t aWin[ACCEL_WINDOW_SIZE][3];
static int16_t currSamp = 0;

APPROX static int32_t mean[3];
APPROX static int32_t stddev[3];

#ifdef TRAINING
static int16_t modelEntry = 0;
static int32_t model[MODEL_SIZE_PLUS_WARMUP];
#endif  // TRAINING

APPROX static int32_t meanmag;
APPROX static int32_t stddevmag;

typedef struct {
  APPROX int8_t x;
  APPROX int8_t y;
  APPROX int8_t z;
  uint8_t padding;
} threeAxis_t;

static threeAxis_t accelOut;

/* trace of stored values */
static threeAxis_t trace[SAMPLES_TO_COLLECT];
static unsigned traceIndex = 0;

APPROX void ACCEL_singleSample(threeAxis_t *taxis) {
  memcpy(taxis, &trace[traceIndex++], sizeof(threeAxis_t));
}

void getNextSample() {
  threeAxis_t threeAxis;
  ACCEL_singleSample(&threeAxis);
  aWin[currSamp][0] = (int32_t)threeAxis.x;
  aWin[currSamp][1] = (int32_t)threeAxis.y;
  aWin[currSamp][2] = (int32_t)threeAxis.z;
  currSamp++;

  if (currSamp >= ACCEL_WINDOW_SIZE) {
    currSamp = 0;
  }
}

void featurize() {
  mean[0] = mean[1] = mean[2] = 0;
  stddev[0] = stddev[1] = stddev[2] = 0;
  uint16_t i;

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
  int i;

  /* classify the current sample (stored in meanmag, stddevmag by featurize())
   * as stationary or moving based on its relative similarity to the first
   * MODEL_COMPARISONS entries of the moving and stationary models. */
  for (i = 0; i < MODEL_COMPARISONS; i += NUM_FEATURES) {
    APPROX long int stat_mean_err = (stationary[i] > meanmag)
                                 ? (stationary[i] - meanmag)
                                 : (meanmag - stationary[i]);

    APPROX long int stat_sd_err = (stationary[i + 1] > stddevmag)
                               ? (stationary[i + 1] - stddevmag)
                               : (stddevmag - stationary[i + 1]);

    APPROX long int move_mean_err = (moving[i] > meanmag) ? (moving[i] - meanmag)
                                                    : (meanmag - moving[i]);

    APPROX long int move_sd_err = (moving[i + 1] > stddevmag)
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
  __NV_movingCount = 0;
  __NV_stationaryCount = 0;
  __NV_totalCount = 0;
}

void read_trace (FILE *infile) {
  size_t sofar = 0;
  while (sofar < SAMPLES_TO_COLLECT) {
    sofar += fread(trace, sizeof(threeAxis_t),
                   SAMPLES_TO_COLLECT - sofar, infile);
  }
  /*
  printf("Sanity check: trace[1]{x,y,z] == (%d,%d,%d,%d) (should be 95,-48,-53,0)\n",
      trace[1].x, trace[1].y, trace[1].z, trace[1].padding);
  printf("Sanity check: trace[2]{x,y,z] == (%d,%d,%d,%d) (should be 81,-44,-55,0)\n",
      trace[2].x, trace[2].y, trace[2].z, trace[2].padding);
  */
}

int main(int argc, char *argv[]) {

  initializeNVData();
  // srand(getpid());

  if (argc != 3) {
    fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
    return 1;
  }

  /* read stored accelerometer readings from input file */
  FILE *inf = fopen(argv[1], "r");
  if (!inf) {
    perror("Cannot open input file");
    return 1;
  }
  read_trace(inf);
  fclose(inf);

  FILE *outf = fopen(argv[2], "w");
  if (!outf) {
    perror("Cannot open output file");
    return 1;
  }

  accept_roi_begin();

  while (1) {

    if( __NV_totalCount > SAMPLES_TO_COLLECT ){
      break;
    }
    getNextSample();
    featurize();

    APPROX int class = classify();
    /* __NV_totalCount, __NV_movingCount, and __NV_stationaryCount have an
     * nv-internal consistency requirement.  This code should be atomic. */

    __NV_totalCount++;


    if (ENDORSE(class)) {

      __NV_movingPct =
        ((float)__NV_movingCount / (float)__NV_totalCount) * 100.0f;
      (__NV_movingCount)++;

    } else {

      __NV_stationaryPct =
        ((float)__NV_stationaryCount / (float)__NV_totalCount) * 100.0f;
      (__NV_stationaryCount)++;

    }
  }

  accept_roi_end();
  fprintf(outf, "__NV_movingCount %d\n", __NV_movingCount);
  fprintf(outf, "__NV_stationaryCount %d\n", __NV_stationaryCount);
  fprintf(outf, "__NV_totalCount %d\n", __NV_totalCount);
  fclose(outf);
  return 0;
}

/* vim: set ai et ts=2 sw=2: */
