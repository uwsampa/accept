#include <stdlib.h>
#include <limits.h>
#include "Accelerometer.h"

vec3 accelWindowRows[ACCEL_WINDOW_SIZE];
matrix3xn accelWindowMatrix;

featurizableData *initFeaturizableData(){
  accelWindowMatrix.numRows = ACCEL_WINDOW_SIZE;
  accelWindowMatrix.rows = &accelWindowRows;
  accelWindowMatrix.next = 0;
  accelWindowMatrix.full = 0;
  return (featurizableData*) &accelWindowMatrix;
}

/*An accelTriple a single accelerometer reading with an xy&z component*/
accelTriple *newAccelTriple(float x, float y, float z){

  accelTriple *trip = (accelTriple*)malloc(sizeof(accelTriple));
  trip->x = x;
  trip->y = y;
  trip->z = z;

  return trip;

}

accelWindow *newAccelWindow(int winSize){

  matrix3xn *m = newMatrix3xn(winSize);
  return (accelWindow *)m;

}

/*External Interface*/
featurizableData *newFeaturizableData(){
  accelWindow *x = newAccelWindow(ACCEL_WINDOW_SIZE);
  return (featurizableData *)x;

}

int sampleDifferent(accelTriple *x,accelWindow *win,float variance){

  vec3 *diff = newZeroVec3();
  int i;
  for(i = 0; i < win->numRows; i++){

    diff->x += (win->rows[i]->x - x->x) * (win->rows[i]->x - x->x);
    diff->y += (win->rows[i]->y - x->y) * (win->rows[i]->y - x->y);
    diff->z += (win->rows[i]->z - x->z) * (win->rows[i]->z - x->z);

  }
  diff->x /= win->numRows;
  diff->y /= win->numRows;
  diff->z /= win->numRows;

  if( magVec3(diff) > variance ){
    return 1;
  } 

}

int samplesReady(accelWindow *win){

  if( win->full ){
    return 1;
  }
  return 0;

}

void getOneSample(accelTriple *tr){

  int x = ACCEL_getX();
  int y = ACCEL_getY();
  int z = ACCEL_getZ();

  if(x > 1200 || x < -1200){ x = 0;}
  if(y > 1200 || y < -1200){ y = 0;}
  if(z > 1200 || z < -1200){ z = 0;}

  tr->x = x;
  tr->y = y;
  tr->z = z;
       
}

/*TODO: "Get" the samples -- emulate or connect to a device*/
void getNextSample(accelWindow *win){

  getOneSample(win->rows[win->next]);

  win->next = (win->next + 1) % win->numRows;

  if( win->next == 0 ){

    win->full = 1;

  }

}

void reportLastSample(accelWindow *win,void *buf){

  unsigned int which = (win->next > 0) ? (win->next - 1) : (win->numRows - 1);
  
  unsigned int *fbuf = (unsigned int*)buf; 

  fbuf[0] = (unsigned)win->rows[which]->x;
  fbuf[1] = (unsigned)win->rows[which]->y;
  fbuf[2] = (unsigned)win->rows[which]->z;

}

