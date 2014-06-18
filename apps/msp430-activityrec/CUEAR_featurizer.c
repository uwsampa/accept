#include <stdlib.h>
#ifndef __MSP430__
#include <assert.h>
#endif
#include <math.h>
#include "vec3.h"
#include "FeatureVector.h"
#include "Accelerometer.h"
#include <enerc.h>
/*This code deals with features of samples in a rows.
  Features are:
  -magnitude of the mean vector
  -magnitude of the variance vector
  -magnitude of the rms vector
  -magnitude of the kurtosis vector

  Future features will be:
  -magnitude of the median vector
  -magnitude of the skewness vector


*/
vec3 *computeMean(accelWindow *win){

  int i, count;
  APPROX vec3 sum, *mean;
  zeroVec3(&sum);

  accept_roi_begin();
  for(i = 0; i < win->numRows; i++){

    sum.x += win->rows[i]->x;
    sum.y += win->rows[i]->y;
    sum.z += win->rows[i]->z;
    count++;
  }
  accept_roi_end();

  mean = newZeroVec3();
  //original code
  /*
  mean->x = sum.x / win->numRows;
  mean->y = sum.y / win->numRows;
  mean->z = sum.z / win->numRows;
  */
  mean->x = sum.x / count;
  mean->y = sum.y / count;
  mean->z = sum.z / count;
  return mean;

}

vec3 *computeStdDev(accelWindow *win, vec3 *mean){

  int i;

  APPROX vec3 *stddev = newZeroVec3();
  //int n = win->numRows;
  int count = 0;
  for(i = 0; i < win->numRows; i++){

    stddev->x +=
      (win->rows[i]->x > mean->x) ?
      (win->rows[i]->x - mean->x) :
      (mean->x - win->rows[i]->x);

    stddev->y +=
      (win->rows[i]->y > mean->y) ?
      (win->rows[i]->y - mean->y) :
      (mean->y - win->rows[i]->y);

    stddev->z +=
      (win->rows[i]->z > mean->z) ?
      (win->rows[i]->z - mean->z) :
      (mean->z - win->rows[i]->z);

    count++;
  }
  /*
  stddev->x = stddev->x / (n-1);
  stddev->y = stddev->y / (n-1);
  stddev->z = stddev->z / (n-1);
  */
  stddev->x = stddev->x / (count-1);
  stddev->y = stddev->y / (count-1);
  stddev->z = stddev->z / (count-1);
  return stddev;

}

vec3 *computeRms(vec3 *mean, vec3 *variance){

   vec3 *rms = newZeroVec3();
   rms->x = mean->x * mean->x + variance->x;
   rms->y = mean->y * mean->y + variance->y;
   rms->z = mean->z * mean->z + variance->z;

   return rms;

}

vec3 *computeKurtosis(accelWindow *win, vec3 *mean){

  int n;
  int i;
  float oneOverN;
  vec3 *kurtosis;
  vec3 SumOfDiffsTo4th;
  vec3 SumOfDiffsTo2nd;

  SumOfDiffsTo4th.x = 0;
  SumOfDiffsTo4th.y = 0;
  SumOfDiffsTo4th.z = 0;

  SumOfDiffsTo2nd.x = 0;
  SumOfDiffsTo2nd.y = 0;
  SumOfDiffsTo2nd.z = 0;

  n = win->numRows;
  for( i = 0; i < win->numRows; i++){

    SumOfDiffsTo2nd.x += (win->rows[i]->x - mean->x) * (win->rows[i]->x - mean->x);
    SumOfDiffsTo2nd.y += (win->rows[i]->y - mean->y) * (win->rows[i]->y - mean->y);
    SumOfDiffsTo2nd.z += (win->rows[i]->z - mean->z) * (win->rows[i]->z - mean->z);

    SumOfDiffsTo4th.x += SumOfDiffsTo2nd.x * SumOfDiffsTo2nd.x;
    SumOfDiffsTo4th.y += SumOfDiffsTo2nd.y * SumOfDiffsTo2nd.y;
    SumOfDiffsTo4th.z += SumOfDiffsTo2nd.z * SumOfDiffsTo2nd.z;

  }

  oneOverN = 1/n;
  kurtosis = newZeroVec3();
  kurtosis->x =
  (
  ( (oneOverN)*SumOfDiffsTo4th.x ) /
  ( ( (oneOverN)*SumOfDiffsTo2nd.x )*( (oneOverN)*SumOfDiffsTo2nd.x ) )
  ) - 3;

  kurtosis->y =
  (
  ( (oneOverN)*SumOfDiffsTo4th.y ) /
  ( ( (oneOverN)*SumOfDiffsTo2nd.y )*( (oneOverN)*SumOfDiffsTo2nd.y ) )
  ) - 3;

  kurtosis->z =
  (
  ( (oneOverN)*SumOfDiffsTo4th.z ) /
  ( ( (oneOverN)*SumOfDiffsTo2nd.z )*( (oneOverN)*SumOfDiffsTo2nd.z ) )
  ) - 3;

  return kurtosis;

}

/*TODO: This will depend on the problem.  For Accelerometer data we'll want to use m/v/r/k mags. */
void featurize(accelWindow *win,featureVector *fv){

  int i;
  vec3 *mean, *stddev;//, *rms, *kurtosis;
#ifndef __MSP430__
  assert(fv != 0);
  assert(win != 0);
#endif
  mean = computeMean(win);
  stddev = computeStdDev(win,mean);
  //rms = computeRms(mean,variance);
  //kurtosis = computeKurtosis(win, mean);

  i = 0;
  fv->features[i++] = magVec3(mean);
  fv->features[i++] = magVec3(stddev);
  //fv->features[i++] = magVec3(rms);
  //fv->features[i++] = magVec3(kurtosis);

  free(mean);
  free(stddev);
  //free(rms);
  //free(kurtosis);

}
