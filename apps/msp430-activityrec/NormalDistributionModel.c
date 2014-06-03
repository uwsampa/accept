#include "NormalDistributionModel.h"
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

/*Here's the code for a normal distribution*/
//Given data with mean mu and stddev sigma, the probability of 
//a new piece of data X is:
//p(X) = (1/( sqrt( 2*PI*sigma*sigma ) )) * exp( (-1 * ((X - mu)^2) ) / (2*sigma*sigma) )

static normalDistributionModel *model;
void createModel(int nClasses, int nFeatures){

  model = newNormalDistributionModel(nClasses,nFeatures);

  //TODO: Initialize the model from a file here

  #ifdef _PERSON_TEST_
  model->clazzes[0]->featureDistributions[0]->mu=5.855;
  model->clazzes[0]->featureDistributions[0]->sigma=3.503e-2;
  
  model->clazzes[1]->featureDistributions[0]->mu=5.4175;
  model->clazzes[1]->featureDistributions[0]->sigma=9.723e-2;
  
  model->clazzes[0]->featureDistributions[1]->mu=176.25;
  model->clazzes[0]->featureDistributions[1]->sigma=1.23e2;
  
  model->clazzes[1]->featureDistributions[1]->mu=132.5;
  model->clazzes[1]->featureDistributions[1]->sigma=5.5833e2;
  
  model->clazzes[0]->featureDistributions[2]->mu=11.25;
  model->clazzes[0]->featureDistributions[2]->sigma=9.17e-1;
  
  model->clazzes[1]->featureDistributions[2]->mu=7.5;
  model->clazzes[1]->featureDistributions[2]->sigma=1.667;
  #endif

}


normalDistribution *newNormalDistribution(){
  normalDistribution *nd = (normalDistribution *)malloc(sizeof(normalDistribution));
  nd->mu = 0;
  nd->sigma = 0;
  return nd;
}

double getProbNormalDist1D(float x, float mu, float sigma){

#ifdef __MSP430__
  double p = 1.0;
#else
  double p = (1/( sqrt( 2*M_PI*sigma*sigma ) )) * exp( (-1 * ((x - mu)*(x - mu)) ) / (2*sigma*sigma) );
#endif
  return p;

}




/*Here's the code for clazz that has several normally distributed features and a clazz id*/


normalDistributionModelClass *newNormalDistributionModelClass(int clazzId, int numFeatures){

  int i; 
  normalDistributionModelClass *c = 
    (normalDistributionModelClass *)malloc(sizeof(normalDistributionModelClass));
  c->clazz = clazzId;
  c->numFeatures = numFeatures;
  c->featureDistributions = 
    (normalDistribution **)malloc( sizeof(normalDistribution*) * numFeatures);;

  for(i = 0; i < numFeatures; i++){
    c->featureDistributions[i] = newNormalDistribution();
  }
  return c;
  
}

void setNormalDistributionParameters(normalDistribution *nd,float mu, float sigma){
  
  nd->mu = mu;
 
  nd->sigma = sigma;

}

void setNormalDistributionModelClassParams(normalDistributionModelClass *c,float mus[], float sigmas[]){

  int i;
  for(i = 0; i < c->numFeatures; i++){

    setNormalDistributionParameters(c->featureDistributions[i],mus[i],sigmas[i]);

  }

}



/*Here's the code for a model made up of several clazzes that have normally distributed features*/


normalDistributionModel *newNormalDistributionModel(int numClasses, int numFeatures){

  int i;
  normalDistributionModel *ndm = 
    (normalDistributionModel*)malloc(sizeof(normalDistributionModel));
  ndm->clazzes = 
    (normalDistributionModelClass**)malloc(sizeof(normalDistributionModelClass*)*numClasses);

  ndm->numClasses = numClasses;

  for(i = 0; i < numClasses; i++){
    ndm->clazzes[i] = newNormalDistributionModelClass( i, numFeatures );
  }

  return ndm;

}


int classify(featureVector *features){

  int resultClass = -1;
  float resultBest = -99999999999;
  int i; 
  for( i = 0; i < NUM_CLASSES; i++){
    double prob = 0.5;//TODO: get the prior from somewhere -- 
                      //      should be in the model

    double lprob = log(0.5);

    int j;
    for( j = 0; j < NUM_FEATURES; j++ ){

      double comp = 
        getProbNormalDist1D(features->features[j], 
                            model->clazzes[i]->featureDistributions[j]->mu, 
                            model->clazzes[i]->featureDistributions[j]->sigma);
      //lprob += log(comp);

    }
    if(lprob > resultBest){ resultBest = lprob; resultClass = i; }

  }
  return resultClass;
#ifndef __MSP430__
  //fprintf(stderr,"Example is from class %d (log(p) = %lf)\n",resultClass,resultBest);
#endif

}


