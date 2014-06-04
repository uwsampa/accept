#include "Person.h"
#include "FeatureVector.h"
#include <stdio.h>

void featurize(featurizableData *p,featureVector *fv){

  fv->features[0] = p->height;
  fv->features[1] = p->weight;
  fv->features[2] = p->foot;
  //fprintf(stderr,"%f %f %f\n",p->height,p->weight,p->foot);

}
