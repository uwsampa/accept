#include <stdlib.h>
#ifndef __MSP430__
#include <assert.h>
#endif
#include "FeatureVector.h"
#include "Thermometer.h"
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

void createModel(unsigned numclasses, unsigned numfeatures){
}

float computeMean(tempWindow *win){

  int i;
  float sum, mean;

  for(i = 0; i < win->numEntries; i++){

    sum += win->entries[i];      

  }
 
  mean = sum / win->numEntries; 

  return mean;

}

float computeVariance(tempWindow *win, float mean){

  int i;
  float variance = 0;
  int n = win->numEntries;
  for(i = 0; i < win->numEntries; i++){

    variance += (win->entries[i] - mean) * (win->entries[i] - mean);

  }
  variance = variance / (n - 1); 

  return variance;

}


/*TODO: This will depend on the problem.  For Accelerometer data we'll want to use m/v/r/k mags. */
void featurize(tempWindow *win,featureVector *fv){

  int i;
  float mean, variance;
  mean = computeMean(win);
  variance = computeVariance(win,mean);

  i = 0;
  fv->features[i++] = mean;
  fv->features[i++] = variance;

}
