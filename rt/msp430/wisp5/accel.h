/**
 * @file accel.h
 *
 * Interface to the ADXL362 accelerometer driver
 *
 * @author Aaron Parks
 */

#ifndef ACCEL_H_
#define ACCEL_H_

#include "wisp5.h"

typedef struct {
    int8_t x;
    int8_t y;
    int8_t z;
} threeAxis_t;

BOOL ACCEL_initialize();
BOOL ACCEL_singleSample(threeAxis_t* result);
BOOL ACCEL_readStat(threeAxis_t* result);
BOOL ACCEL_readID(threeAxis_t* result);
BOOL ACCEL_reset();
BOOL ACCEL_range();


#endif /* ACCEL_H_ */
