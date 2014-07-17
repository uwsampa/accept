#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <enerc.h>

#include "convolution.h"
#include "rgb_image.h"

#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_types.h"

// Sobel parameters in convolution.h

// DUMP DATA:
// 0 - No data dump
// 1 - Data dump
#define DUMP_DATA       1
// TIMER:
// 0 - Use ARM performance counters
// 1 - Use FPGA clock counter
#define TIMER           1 
// POWER MODE:
// 0 - Normal operation
// 1 - Loop on precise execution
// 2 - Loop on approximate execution
#define POWER_MODE      0 

// Global variables
long long int t_kernel_precise;
long long int t_kernel_approx;
long long int dynInsn_kernel_approx;
int kernel_invocations;


int main (int argc, const char* argv[]) {
    
    // Sobel variables
    int i;
    int x, y;
    APPROX float s;
    APPROX RgbImage srcImage;
    APPROX RgbImage dstImage;
    APPROX float w[3][3];

    // Init the rgb images
    initRgbImage(&srcImage);
    initRgbImage(&dstImage);
    loadRgbImage("", &srcImage);
    loadRgbImage("", &dstImage);
    makeGrayscale(&srcImage);
    
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);

    accept_roi_begin();
    

        for (y = 0; y < (srcImage.h); y++) {

            for (x = 0; x < srcImage.w; x++) {
              w[0][0] = (x == 0 || y == 0                    ) ? 0 : srcImage.pixels[y - 1][x - 1].r;
              w[0][1] = (y == 0                              ) ? 0 : srcImage.pixels[y - 1][x].r;
              w[0][2] = (x == srcImage.w - 1 || y == 0          ) ? 0 : srcImage.pixels[y - 1][x + 1].r;
              w[1][0] = (x == 0                              ) ? 0 : srcImage.pixels[y][x - 1].r;
              w[1][1] =                                              ENDORSE(srcImage.pixels[y][x].r);
              w[1][2] = (x == srcImage.w - 1                    ) ? 0 : srcImage.pixels[y][x + 1].r;
              w[2][0] = (x == 0 || y == srcImage.h - 1          ) ? 0 : srcImage.pixels[y + 1][x - 1].r;
              w[2][1] = (y == srcImage.h - 1                    ) ? 0 : srcImage.pixels[y + 1][x].r;
              w[2][2] = (x == srcImage.w - 1 || y == srcImage.h - 1) ? 0 : srcImage.pixels[y + 1][x + 1].r;


              s = sobel(w);


              dstImage.pixels[y][x].r = s;
              dstImage.pixels[y][x].g = s;
              dstImage.pixels[y][x].b = s;
            }
            
        }

    accept_roi_end();

    
    printf("256,256\n");
    for (y=0;y<srcImage.h;y++) {
        for (x=0;x<srcImage.w;x++) {
            if (x!=srcImage.w-1) {
                printf("%d,", (int) (dstImage.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage.pixels[y][x].r*256));
            } else {
                printf("%d,", (int) (dstImage.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage.pixels[y][x].r*256));
                printf("%d\n", (int) (dstImage.pixels[y][x].r*256));
            }
        }
    }
    

    ///////////////////////////////
    // 6 - Free memory
    ///////////////////////////////
    
    freeRgbImage(ENDORSE(&srcImage));
    freeRgbImage(ENDORSE(&dstImage));
    
    return 0;
}


