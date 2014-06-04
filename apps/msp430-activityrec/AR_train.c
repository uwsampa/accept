#ifndef __MSP430__
#include <assert.h>
#else
#include <msp430.h>
#endif

#include <stdlib.h>
#include <math.h>

#ifdef _PERSON_TEST_
#include "Person.h"
#include "PERSON_featurizer.h"
#else
#include "Accelerometer.h"
#include "CUEAR_featurizer.h"
#endif

#include "vec3.h"
#include "NormalDistributionModel.h"
#include "FeatureVector.h"
#include "../../DinoRuntime/dino.h"
#include <stdarg.h>

#define SAMPLE_INTERVAL 1 /*sample every 1000th feature vector*/
#define NUM_SAMPLES 0x0500 /*4096 samples*/
int *whichTrainingExample = (int *)0xD000;
feature *trainingExamples = (feature *)0xD002;

void sampler_init(){

  unsigned int w; 
  unsigned int t;

  *whichTrainingExample = 0;

  /*blink 3 times at startup*/
  for(t = 0; t < 6; t++){
    for(w = 0; w < 10000; w++);
    P1OUT ^= 0x01;
  }
  /*turn light off for a long time*/
  P1OUT |= 0x01;// Toggle P1.0 using exclusive-OR

  /*stay turned on for a while*/
  for(w = 0; w < 0xffff; w++);
  for(w = 0; w < 0xffff; w++);
  for(w = 0; w < 0xffff; w++);

  /*blink 3 times to indicate sampling*/
  for(t = 0; t < 6; t++){
    for(w = 0; w < 10000; w++);
    P1OUT ^= 0x01;
  }
  /*LED on means sampling is happening*/
  P1OUT |= 0x01;// Toggle P1.0 using exclusive-OR

}


int main(int argc, char *argv[]){

  WDTCTL = WDTPW | WDTHOLD;// Stop watchdog timer
  P1DIR |= 0x01;// Set P1.0 to output direction
  P1OUT = 0x01;// Toggle P1.0 using exclusive-OR
  
  if( *whichTrainingExample < 0 || *whichTrainingExample > NUM_SAMPLES * NUM_FEATURES ){
    *whichTrainingExample = 0;
  }
  ACCEL_setup();
  ACCEL_SetReg(0x2D,0x02); 
  
  P4DIR |= BIT5;// Set P4.5 to output direction
  unsigned tmpx = ACCEL_getX();
  unsigned tmpy = ACCEL_getY();
  unsigned tmpz = ACCEL_getZ();
  
  /*newFeaturizableData allocates memory -- assume this is in NV*/
  featurizableData *X = (featurizableData*)newFeaturizableData();//Defined in Accelerometer

  featureVector *features = newFeatureVector(NUM_FEATURES);//Defined in FeatureVector

  createModel(NUM_CLASSES, NUM_FEATURES);//in NormalDistributionModel

  unsigned sampleTimer = SAMPLE_INTERVAL;
  while(1){

    if( P4IN & BIT6 ){
      /*pushbutton on 4.6 is pressed.  This should be interrupt driven, but it's like this for now*/
      P4OUT |= BIT5; 
      sampler_init();
      P4OUT &= ~BIT5;
    } 

    getNextSample(X);//in Accelerometer

    if(samplesReady(X)){

      featurize(X,features); //in CUEAR -- Accelerometer and CUEAR agree on this

      if( --sampleTimer == 0 ){

        sampleTimer = SAMPLE_INTERVAL;
        /*Do this for the first 1000 samples*/
        if( *whichTrainingExample < NUM_SAMPLES * features->numFeatures ){

          int i;
          for(i = 0; i < features->numFeatures; i++){
            trainingExamples[ (*whichTrainingExample) + i ] = features->features[i];
          }
          *whichTrainingExample = *whichTrainingExample + features->numFeatures;

        }else{

          P1OUT &= ~(0x01);//LED off means sampling is done 

        }

      }
    
    }

  }

}
