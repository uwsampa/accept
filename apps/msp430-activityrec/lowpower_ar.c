#include <msp430.h>
#include <stdlib.h>
#include <stdarg.h>
#include <adxl362z.h>
#include "../../DinoRuntime/dino.h"

#define DO_RECOVERY
#define LOW_POWER
#define MODEL_SIZE 100
#define NUM_FEATURES 2
#define ACCEL_WINDOW_SIZE 3
static float stationary[MODEL_SIZE] = {
#include "int_stationary_model.h"
};

static float walking[MODEL_SIZE] = {
#include "int_walking_model.h"
};

unsigned int *walkingCount = (unsigned int*)0xD000;
unsigned int *stationaryCount = (unsigned int*)0xD002;
unsigned int *totalCount = (unsigned int*)0xD004;

typedef int accelReading[3];
typedef accelReading accelWindow[ACCEL_WINDOW_SIZE];

static accelWindow aWin;
static int currSamp = 0;

static accelReading mean;
static accelReading stddev;

static int meanmag;
static int stddevmag;

void getOneSample(accelReading tr){

  int x = ACCEL_getX();
  int y = ACCEL_getY();
  int z = ACCEL_getZ();

  tr[0] = x;
  tr[1] = y;
  tr[2] = z;
       
}

void getNextSample(){

  getOneSample(aWin[currSamp]);
  currSamp = (currSamp + 1) % ACCEL_WINDOW_SIZE;

}


void featurize(){

  mean[0] = mean[1] = mean[2] = 0;
  stddev[0] = stddev[1] = stddev[2] = 0;
  int i;
  for(i = 0; i < ACCEL_WINDOW_SIZE; i++){
    mean[0] = mean[0] + aWin[i][0];//x
    mean[1] = mean[1] + aWin[i][1];//y
    mean[2] = mean[2] + aWin[i][2];//z
  }
  mean[0] = mean[0] / ACCEL_WINDOW_SIZE;
  mean[1] = mean[1] / ACCEL_WINDOW_SIZE;
  mean[2] = mean[2] / ACCEL_WINDOW_SIZE;

  for(i = 0; i < ACCEL_WINDOW_SIZE; i++){
    stddev[0] += aWin[i][0] > mean[0] ? aWin[i][0] - mean[0] : mean[0] - aWin[i][0];//x
    stddev[1] += aWin[i][1] > mean[1] ? aWin[i][1] - mean[1] : mean[1] - aWin[i][1];//x
    stddev[2] += aWin[i][2] > mean[2] ? aWin[i][2] - mean[2] : mean[2] - aWin[i][2];//x
  }
  stddev[0] = stddev[0] / (ACCEL_WINDOW_SIZE-1);
  stddev[1] = stddev[1] / (ACCEL_WINDOW_SIZE-1);
  stddev[2] = stddev[2] / (ACCEL_WINDOW_SIZE-1);
 
  meanmag = mean[0] + mean[1] + mean[2] / 3;
  stddevmag = stddev[0] + stddev[1] + stddev[2] / 3;

}

#define MODEL_COMPARISONS 10
int classify(){

  int walk_less_error = 0;
  int stat_less_error = 0;
  int i;
  for( i = 0; i < MODEL_COMPARISONS; i+=NUM_FEATURES ){

    int stat_mean_err = (stationary[i] > meanmag) ?
                          (stationary[i] - meanmag) :
                          (meanmag - stationary[i]);

    int stat_sd_err =   (stationary[i+1] > stddevmag) ?
                          (stationary[i+1] - stddevmag) :
                          (stddevmag - stationary[i+1]);
    
    int walk_mean_err = (walking[i] > meanmag) ?
                          (walking[i] - meanmag) :
                          (meanmag - walking[i]);

    int walk_sd_err =   (walking[i+1] > stddevmag) ?
                          (walking[i+1] - stddevmag) :
                          (stddevmag - walking[i+1]);

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


#define PROCESSING_TASK 0
#define UPDATE_COUNTS_TASK 1
DINO_RECOVERY_ROUTINE_LIST_BEGIN()

  DINO_RECOVERY_RTN_BEGIN(PROCESSING_TASK)


  DINO_RECOVERY_RTN_END()
  
  DINO_RECOVERY_RTN_BEGIN(UPDATE_COUNTS_TASK)

    unsigned int *DINO_VERSION_totalCount = DINO_BIND(unsigned int*);
    unsigned int *DINO_VERSION_statCount = DINO_BIND(unsigned int*);
    unsigned int *DINO_VERSION_walkCount = DINO_BIND(unsigned int*);

    #ifdef DO_RECOVERY
    /*Use DINO versioning support to revert to the 'old' 
      version and take the atomic section from the top

      This versioning prevents atomicity violations due
      to nv-internal inconsistency and repeated updates
      due to nv-external inconsistency
    */

    /*maybe: DINO_NV_REVERT(totalCount)*/
    *totalCount = *DINO_VERSION_totalCount;
    *stationaryCount = *DINO_VERSION_statCount;
    *walkingCount = *DINO_VERSION_walkCount;
  #endif

  DINO_RECOVERY_RTN_END()


DINO_RECOVERY_ROUTINE_LIST_END()


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

  initializeHardware();
  initializeNVData();
  DINO_RESTORE_CHECK()
  while(1){
    

    DINO_TASK_BOUNDARY(PROCESSING_TASK,NULL) 
    while( (*totalCount) >= 100 ){

      if( P4IN & BIT6 ){
        clearExperimentalData();
      }

    }

    if( P4IN & BIT6 ){

      clearExperimentalData();

    }

    getNextSample();
    P1OUT ^= 0x01;

    featurize();

    int class = classify();
    
      
    /*TODO: define WITH_DINO_VERSION_PTR(unsigned int,totalCount);*/
    unsigned int DINO_VERSION_totalCount = *totalCount;
    unsigned int DINO_VERSION_statCount = *stationaryCount;
    unsigned int DINO_VERSION_walkCount = *walkingCount;
    /*TODO: end def'n WITH_DINO_VERSION_PTR(unsigned int,totalCount);*/
   
    DINO_TASK_BOUNDARY(UPDATE_COUNTS_TASK,&DINO_VERSION_totalCount,&DINO_VERSION_statCount,&DINO_VERSION_walkCount) 

    /*totalCount, walkingCount, and stationaryCount 
    have an nv-internal consistency requirement.
    This code should be atomic.
    */

    (*totalCount)++;

    int q;
    for(q = 0; q < 0xFF; q++);

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

