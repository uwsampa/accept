#include "complex.h"

#include <math.h>
#include <enerc.h>

void fftSinCos(float x, APPROX float* s, APPROX float* c) {
    *s = sin(-2 * PI * x);
    *c = cos(-2 * PI * x);
}

