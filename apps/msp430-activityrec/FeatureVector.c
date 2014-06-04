#include <stdlib.h>
#include "FeatureVector.h"
featureVector *newFeatureVector(int numFeatures){
  
  featureVector *fv = (featureVector *)malloc(sizeof(featureVector));
  fv->numFeatures = numFeatures;
  fv->features = (feature *)malloc(sizeof(feature)*numFeatures);
  return fv;

}
