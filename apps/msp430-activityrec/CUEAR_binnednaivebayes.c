#include "FeatureVector.h"
#define MODEL_SIZE 100

static float stationary[MODEL_SIZE] = {
#include "stationary_model.h"
};

static float walking[MODEL_SIZE] = {
#include "walking_model.h"
};

int classify(featureVector *features){

  int walk_less_error = 0;
  int stat_less_error = 0;
  int i;
  for( i = 0; i < MODEL_SIZE; i+=NUM_FEATURES ){

    float stat_mean_err = (stationary[i] > features->features[0]) ?
                          (stationary[i] - features->features[0]) :
                          (features->features[0] - stationary[i]);
    float stat_sd_err =   (stationary[i+1] > features->features[1]) ?
                          (stationary[i+1] - features->features[1]) :
                          (features->features[1] - stationary[i+1]);
    
    float walk_mean_err = (walking[i] > features->features[0]) ?
                          (walking[i] - features->features[0]) :
                          (features->features[0] - walking[i]);

    float walk_sd_err =   (walking[i+1] > features->features[1]) ?
                          (walking[i+1] - features->features[1]) :
                          (features->features[1] - walking[i+1]);

    if( walk_mean_err < stat_mean_err ){
      walk_less_error ++;
    }else{
      stat_less_error ++;
    }

    if( walk_sd_err < stat_sd_err ){
      walk_less_error ++;
    }else{
      stat_less_error ++;
    }
    
  }
  if(walk_less_error > stat_less_error ){
    return 1;
  }else{
    return 0;
  }

}

void createModel(int nClasses, int nFeatures){

}
