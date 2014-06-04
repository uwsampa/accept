#include <stdlib.h>
#include <limits.h>
#include <msp430.h>
#include "Thermometer.h"



/*An accelTriple a single accelerometer reading with an xy&z component*/
tempWindow *newTempWindow(unsigned size){

  tempWindow *t = (tempWindow *)malloc(sizeof(tempWindow));
  float *win = (float *)malloc(size * sizeof(float));
  t->entries = win;
  t->next = 0;
  t->full = 0;
  t->numEntries = size;
  return t;

}

/*External Interface*/
featurizableData *newFeaturizableData(){
  //init_sensor();
  tempWindow *x = newTempWindow(TEMP_WINDOW_SIZE);
  return (featurizableData *)x;
}

int sampleDifferent(float *x,float *win,float variance){

  return 1;

}

int samplesReady(tempWindow *win){

  if( win->full ){
    return 1;
  }
  return 0;

}

void getOneSample(float *tmp){

  //Read thermometer here
  unsigned int ret = adc12_sample();
  *tmp = (float)ret;
   
}

/*TODO: "Get" the samples -- emulate or connect to a device*/
void getNextSample(tempWindow *win){

  getOneSample( &(win->entries[win->next]) );

  win->next = (win->next + 1) % win->numEntries;

  if( win->next == 0 ){

    win->full = 1;

  }
  
}

