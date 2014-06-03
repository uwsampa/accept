#ifndef _VEC3_H_
#define _VEC3_H_
/*This code deals with vectors, which represent accelerometer readings*/
typedef struct vec3{
  float x;
  float y;
  float z;
} vec3;

/*A matrix is a list of vectors*/
typedef struct matrix3xn{

  int full;
  int next;
  int numRows;
  vec3 **rows; 

} matrix3xn;

vec3 *newZeroVec3();
matrix3xn *newMatrix3xn(int rows);

void zeroVec3(vec3 *v);
float magVec3(vec3 *v);
void setVec3(vec3 *v,float x, float y, float z);
#endif
