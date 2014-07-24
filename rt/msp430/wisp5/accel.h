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
/* #include <enerc.h> */

typedef struct {
/* XXX these should all be APPROX, but mspgcc can't understand APPROX and clang
 * can't build the rest of libwisp5.a.  workaround: apps that include libwisp5.a
 * should define their own threeAxis_t with APPROX members. */
#ifdef __clang__
    APPROX int8_t x;
    APPROX int8_t y;
    APPROX int8_t z;
#else
    int8_t x;
    int8_t y;
    int8_t z;
#endif
    int8_t padding;
} threeAxis_t;

BOOL ACCEL_initialize();
BOOL ACCEL_singleSample(threeAxis_t* result);
BOOL ACCEL_readStat(threeAxis_t* result);
BOOL ACCEL_readID(threeAxis_t* result);
BOOL ACCEL_reset();
BOOL ACCEL_range();


#endif /* ACCEL_H_ */
