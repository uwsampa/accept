#ifndef COMPLEX_H_
#define COMPLEX_H_

#define PI 3.1415926535897931

typedef struct {
   float real;
   float imag;
} Complex;

void fftSinCos(float x, float* s, float* c);

#endif /* COMPLEX_H_ */
