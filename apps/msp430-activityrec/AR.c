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
#include "CUEAR_binnednaivebayes.h"
#include "FeatureVector.h"
#include <stdarg.h>

#define LOW_POWER
#undef DO_RECOVERY 

//fwd decl
void settleAccelerometer();
unsigned int *accelVals = (unsigned int*)0xE000;
/*Explicit non-volatile state*/
unsigned int *walkingCount = (unsigned int*)0xD000;
unsigned int *stationaryCount = (unsigned int*)0xD002;
unsigned int *totalCount = (unsigned int*)0xD004;
#define PROCESSING_TASK 0
#define UPDATE_COUNTS_TASK 1

void settleAccelerometer(){
  int x = 0;
  int y = 0;
  int z = 0;
  while( x == 0 || y == 0 || z == 0 ){
    x = ACCEL_getX();
    y = ACCEL_getY();
    z = ACCEL_getZ();
  }
}

void initializeNVData(){
  
  /*The erase-initial condition*/
  if(*walkingCount == 0xffff &&
     *stationaryCount == 0xffff &&
     *totalCount == 0xffff){
    
    *walkingCount = 0;
    *stationaryCount = 0;
    *totalCount = 0;

  }

}

void initializeHardware(){

  /*Before anything else, do the device hardware configuration*/

  WDTCTL = WDTPW | WDTHOLD;// Stop watchdog timer
  
  P1DIR |= 0x01;// Set P1.0 to output direction
#ifndef LOW_POWER
  P1OUT = 0x01;// Toggle P1.0 using exclusive-OR
#endif
  
#ifndef LOW_POWER
  P4DIR |= BIT4 | BIT5;// Set P4.4/5 to output direction
  P4OUT |= BIT4 | BIT5;// Toggle P4.4/5 using exclusive-OR
  int i;
  for(i = 0; i < 0xffff; i++);
  P4OUT &= ~BIT4;// Toggle P4.4 using exclusive-OR
  P4OUT &= ~BIT5;// Toggle P4.5 using exclusive-OR
#endif

  ACCEL_setup();
  ACCEL_SetReg(0x2D,0x02); 

}

void clearExperimentalData(){
   
#ifndef LOW_POWER
  P4OUT |= BIT4 | BIT5;// Toggle P4.4/5 using exclusive-OR
#endif
  accelVals = (unsigned int*)0xE000; 
  *walkingCount = 0;
  *stationaryCount = 0;
  *totalCount = 0;
#ifndef LOW_POWER
  int i;
  for(i = 0; i < 0xffff; i++);
  P4OUT &= ~BIT4;// Toggle P4.4 using exclusive-OR
  P4OUT &= ~BIT5;// Toggle P4.5 using exclusive-OR
#endif
  
}

int main(int argc, char *argv[]){

  int classifications = 0;
  initializeHardware();
  initializeNVData();

  /*newFeaturizableData allocates memory -- assume this is in NV*/
  featurizableData *X = (featurizableData*)initFeaturizableData();//Defined in Accelerometer

  featureVector *features = newFeatureVector(NUM_FEATURES);//Defined in FeatureVector

  createModel(NUM_CLASSES, NUM_FEATURES);//in NormalDistributionModel

  while(1){
    

    while( (*totalCount) >= 250 ){

      if( P4IN & BIT6 ){
        clearExperimentalData();
      }

    }

    if( P4IN & BIT6 ){

      clearExperimentalData();

    }

    getNextSample(X);//in Accelerometer
    P1OUT ^= 0x01;

    if(samplesReady(X)){

      /*reportLastSample(X,accelVals);
      accelVals = accelVals + 3;*/

      featurize(X,features); //in CUEAR -- Accelerometer and CUEAR agree on this

      int class = classify(features); 
   

      /*totalCount, walkingCount, and stationaryCount 
        have an nv-internal consistency requirement.
        This code should be atomic.
       */

      classifications++;

      (*totalCount)++;

      if(class){
#ifndef LOW_POWER
        P4OUT &= ~BIT4;
        P4OUT |= BIT5;
#endif
        (*walkingCount)++;

      }else{

#ifndef LOW_POWER
        P4OUT &= ~BIT5;// Toggle P1.0 using exclusive-OR
        P4OUT |= BIT4;
#endif
        (*stationaryCount)++;

      }

    }
  
 

  }

}
