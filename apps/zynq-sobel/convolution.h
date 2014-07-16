/*
 * convolution.h
 *
 *  Created on: May 1, 2012
 *      Author: Hadi Esmaeilzadeh <hadianeh@cs.washington.edu>
 *              Thierry Moreau <moreau@cs.washington.edu>
 */

#ifndef CONVOLUTION_H_
#define CONVOLUTION_H_

#include <assert.h>
#include <enerc.h>

#include "rgb_image.h"

#include "npu.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_types.h"

#define NUM_INPUTS      9
#define NUM_OUTPUTS     1
#define BUFFER_SIZE     32

float sobel(APPROX float w[][3]);
//APPROX float w[3][3];

#define WINDOW(image, x, y, window) {            \
    window[0][0] = image.pixels[y - 1][x - 1].r; \
    window[0][1] = image.pixels[y - 1][x].r;     \
    window[0][2] = image.pixels[y - 1][x + 1].r; \
\
    window[1][0] = image.pixels[y][x - 1].r;     \
    window[1][1] = image.pixels[y][x].r;         \
    window[1][2] = image.pixels[y][x + 1].r;     \
\
    window[2][0] = image.pixels[y + 1][x - 1].r; \
    window[2][1] = image.pixels[y + 1][x].r;     \
    window[2][2] = image.pixels[y + 1][x + 1].r; \
}

#define HALF_WINDOW(image, x, y, window) {                                                    \
    window[0][0] = (x == 0 || y == 0                    ) ? 0 : image.pixels[y - 1][x - 1].r; \
    window[0][1] = (y == 0                              ) ? 0 : image.pixels[y - 1][x].r;     \
    window[0][2] = (x == image.w - 1 || y == 0          ) ? 0 : image.pixels[y - 1][x + 1].r; \
\
    window[1][0] = (x == 0                              ) ? 0 : image.pixels[y][x - 1].r;     \
    window[1][1] =                                              ENDORSE(image.pixels[y][x].r);         \
    window[1][2] = (x == image.w - 1                    ) ? 0 : image.pixels[y][x + 1].r;     \
\
    window[2][0] = (x == 0 || y == image.h - 1          ) ? 0 : image.pixels[y + 1][x - 1].r; \
    window[2][1] = (y == image.h - 1                    ) ? 0 : image.pixels[y + 1][x].r;     \
    window[2][2] = (x == image.w - 1 || y == image.h - 1) ? 0 : image.pixels[y + 1][x + 1].r; \
}

#define NPU_HALF_WINDOW(image, dstImage, x, y, x_wr, y_wr, im_w, iBuff, oBuff, tmp, invoc_index, i) { \
    invoc_index++;                                                                          \
\
    *(iBuff++) = (x == 0 || y == 0                    ) ? 0 : image.pixels[y - 1][x - 1].r; \
    *(iBuff++) = (y == 0                              ) ? 0 : image.pixels[y - 1][x].r;     \
    *(iBuff++) = (x == image.w - 1 || y == 0          ) ? 0 : image.pixels[y - 1][x + 1].r; \
\
    *(iBuff++) = (x == 0                              ) ? 0 : image.pixels[y][x - 1].r;     \
    *(iBuff++) =                                              image.pixels[y][x].r;         \
    *(iBuff++) = (x == image.w - 1                    ) ? 0 : image.pixels[y][x + 1].r;     \
\
    *(iBuff++) = (x == 0 || y == image.h - 1          ) ? 0 : image.pixels[y + 1][x - 1].r; \
    *(iBuff++) = (y == image.h - 1                    ) ? 0 : image.pixels[y + 1][x].r;     \
    *(iBuff++) = (x == image.w - 1 || y == image.h - 1) ? 0 : image.pixels[y + 1][x + 1].r; \
\
    if (invoc_index == BUFFER_SIZE) {                                                       \
        npu();                                                                              \
        for (i=0; i<BUFFER_SIZE*NUM_OUTPUTS; i++) {                                         \
            tmp = *(oBuff++);                                                               \
            dstImage.pixels[y_wr][x_wr].r = tmp;                                            \
            dstImage.pixels[y_wr][x_wr].g = tmp;                                            \
            dstImage.pixels[y_wr][x_wr].b = tmp;                                            \
            if (x_wr==im_w-1) { x_wr = 0; y_wr++; }                                         \
            else { x_wr++; }                                                                \
        }                                                                                   \
        invoc_index = 0;                                                                    \
        iBuff = (float*) OCM_SRC;                                                           \
        oBuff = (float*) OCM_DST;                                                           \
    }                                                                                       \
}

#define NPU_WINDOW(image, dstImage, x, y, x_wr, y_wr, im_w, iBuff, oBuff, tmp, buff_index, i) { \
    invoc_index++;                                                                          \
\
    *(iBuff++) = image.pixels[y - 1][x - 1].r;                                              \
    *(iBuff++) = image.pixels[y - 1][x].r;                                                  \
    *(iBuff++) = image.pixels[y - 1][x + 1].r;                                              \
\
    *(iBuff++) = image.pixels[y][x - 1].r;                                                  \
    *(iBuff++) = image.pixels[y][x].r;                                                      \
    *(iBuff++) = image.pixels[y][x + 1].r;                                                  \
\
    *(iBuff++) = image.pixels[y + 1][x - 1].r;                                              \
    *(iBuff++) = image.pixels[y + 1][x].r;                                                  \
    *(iBuff++) = image.pixels[y + 1][x + 1].r;                                              \
\
    if (invoc_index == BUFFER_SIZE) {                                                       \
        npu();                                                                              \
        for (i=0; i<BUFFER_SIZE*NUM_OUTPUTS; i++) {                                         \
            tmp = *(oBuff++);                                                               \
            dstImage.pixels[y_wr][x_wr].r = tmp;                                            \
            dstImage.pixels[y_wr][x_wr].g = tmp;                                            \
            dstImage.pixels[y_wr][x_wr].b = tmp;                                            \
            if (x_wr==im_w-1) { x_wr = 0; y_wr++; }                                         \
            else { x_wr++; }                                                                \
        }                                                                                   \
        invoc_index = 0;                                                                    \
        iBuff = (float*) OCM_SRC;                                                           \
        oBuff = (float*) OCM_DST;                                                           \
    }                                                                                       \
}

#define NPU_PADDING(image, dstImage, x_wr, y_wr, im_w, iBuff, oBuff, tmp, buff_index, i) { \
\
    npu();                                                                                  \
    for (i=0; i<buff_index*NUM_OUTPUTS; i++) {                                              \
        tmp = *(oBuff++);                                                                   \
        dstImage.pixels[y_wr][x_wr].r = tmp;                                                \
        dstImage.pixels[y_wr][x_wr].g = tmp;                                                \
        dstImage.pixels[y_wr][x_wr].b = tmp;                                                \
        if (x_wr==im_w-1) { x_wr = 0; y_wr++; }                                             \
        else { x_wr++; }                                                                    \
    }                                                                                       \
    invoc_index = 0;                                                                        \
    iBuff = (float*) OCM_SRC;                                                               \
    oBuff = (float*) OCM_DST;                                                               \
    x_wr = 0;                                                                               \
    y_wr = 0;                                                                               \
}

#endif /* CONVOLUTION_H_ */
