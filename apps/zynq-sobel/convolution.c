#include "convolution.h"
#include <math.h>
#include <stdio.h>
#include <enerc.h>

static float kx[][3] =
    { { -1, -2, -1},
      {  0, 0,  0},
      {  1, 2,  1}};

static float ky[][3] =
    { { -1, 0,  1},
      { -2, 0,  2},
      { -1, 0,  1}};

float convolve(float w[][3], float k[][3]) {
    int i;
    int j;
    float r;

    r = 0;
    for (j = 0; j < 3; j++)
        for (i = 0; i < 3; i++)
            r += w[i][j] * k[j][i]; //ACCEPT_PERMIT

    return r;
}

APPROX float sobel(float w[][3]) {
    float sx;
    float sy;
    float s;

    sx = convolve(w, ky);
    sy = convolve(w, kx);

    s = sqrt(sx * sx + sy * sy);
    if (s >= (256 / sqrt(256 * 256 + 256 * 256)))
        s = 255 / sqrt(256 * 256 + 256 * 256);

    return s ;
}

