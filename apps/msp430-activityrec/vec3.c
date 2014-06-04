#include <stdlib.h>
#include <math.h>
#include "vec3.h"
vec3 *newZeroVec3(){
  vec3 *v = (vec3*)malloc(sizeof(float)*3);
  return v;
}

void setVec3(vec3 *v,float x, float y, float z){
  v->x = x;
  v->y = y;
  v->z = z;
}

void zeroVec3(vec3 *v){
  v->x = 0;
  v->y = 0;
  v->z = 0;
}

float magVec3(vec3 *v){

  return sqrtf( v->x*v->x + v->y*v->y + v->z*v->z );

}

matrix3xn *newMatrix3xn(int numRows){

  int i;
  matrix3xn *m = (matrix3xn *)malloc(sizeof(matrix3xn));
  vec3 **rows = (vec3 **)malloc(sizeof(vec3 *)*numRows);

  m->numRows = numRows;
  for(i = 0; i < numRows; i++){

    rows[i] = newZeroVec3();

  }

  m->rows = rows;

  m->next = 0;
  m->full = 0;

  return m;

}

void setMatrixRow(matrix3xn *m, int row, float x, float y, float z){

  setVec3(m->rows[row], x, y, z);

}
