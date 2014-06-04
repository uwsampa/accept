#include "NormalDistributionModel.h"
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

int classify(featureVector *features){

  float mean = features->features[0];
  float variance = features->features[1]; 

  /*
    When the average variation from the mean
    is greater than 10 percent of the mean,
    we conclude that we're changing contexts
    and say we're in class 1.  Otherwise, we
    say we're in class 0, meaning we're in a
    consistent context.  
  */
  float threshold = mean * .1;
  
  if( sqrt(variance) > threshold ){

    return 1;

  }

  return 0;

}
