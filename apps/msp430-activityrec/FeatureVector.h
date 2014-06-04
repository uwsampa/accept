#ifndef _FEATURE_VECTOR_H_
#define _FEATURE_VECTOR_H_

#define NUM_CLASSES 2
#define NUM_FEATURES 2

typedef float feature;
typedef struct featureVector{

  int numFeatures;
  feature *features;

} featureVector;
featureVector *newFeatureVector(int numFeatures);
#endif
