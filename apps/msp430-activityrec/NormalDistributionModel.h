#include "FeatureVector.h"
typedef struct normalDistribution{

  float mu; //the mean
  float sigma;  //the stdDev


} normalDistribution;


typedef struct normalDistributionModelClass{

  int clazz;
  int numFeatures;
  normalDistribution **featureDistributions;

} normalDistributionModelClass;


typedef struct normalDistributionModel{

  int numClasses;
  normalDistributionModelClass **clazzes;

}normalDistributionModel;


normalDistribution *newNormalDistribution();
double getProbNormalDist1D(float x, float mu, float sigma);
normalDistributionModelClass *newNormalDistributionModelClass(int clazzId, int numFeatures);
void setNormalDistributionParameters(normalDistribution *nd,float mu, float sigma);
void setNormalDistributionModelClassParams(normalDistributionModelClass *c,float mus[], float sigmas[]);
normalDistributionModel *newNormalDistributionModel(int numClasses, int numFeatures);

void createModel(int nClasses, int nFeatures);
int classify(featureVector *features);
