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
    APPROX RgbImage dstImage_precise;
    APPROX RgbImage dstImage_approx;
    APPROX float w[3][3];


    ///////////////////////////////
    // 1 - Initialization
    ///////////////////////////////
    
    // Init performance counters:
    /*
    init_perfcounters (1, 0, 1, evt_counter);
    t_kernel_precise = 0;
    t_kernel_approx = 0;
    dynInsn_kernel_approx = 0;
    */
    
    // Init the rgb images
    initRgbImage(&srcImage);
    initRgbImage(&dstImage_precise);
    initRgbImage(&dstImage_approx);
    loadRgbImage("", &srcImage);
    loadRgbImage("", &dstImage_precise);
    loadRgbImage("", &dstImage_approx);
    makeGrayscale(&srcImage);
    
#if DUMP_DATA == 0
    printf("\n\nRunning sobel benchmark on an %u x %u image\n", srcImage.w, srcImage.h);
#endif


    ///////////////////////////////
    // 2 - Precise execution
    ///////////////////////////////
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);
    

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


              dstImage_precise.pixels[y][x].r = s;
              dstImage_precise.pixels[y][x].g = s;
              dstImage_precise.pixels[y][x].b = s;
            }
            
        }

    


    ///////////////////////////////
    // 3 - Approximate execution 
    ///////////////////////////////
    
    /*
    // Pointers and state for NPU based sobel
    volatile float* iBuff;              // NPU input buffer
    volatile float* oBuff;              // NPU output buffer
    int invoc_index;                    // Current NPU invocation in the batch
    int x_wr;                           // x index for writes to the output image
    int y_wr;                           // y index for writes to the output image
    int im_w;                           // output image width
    float tmp_out;                      // NPU output
    
    
    // Init TLB page settings
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);
    
    // Initialization
    iBuff = (float*) OCM_SRC;
    oBuff = (float*) OCM_DST;
    invoc_index = 0;
    x_wr = 0;
    y_wr = 0;
    im_w = dstImage_approx.w;
    
#if TIMER==0
    t_approx = get_cyclecount();
#else
    t_approx = rd_fpga_clk();
#endif //TIMER

    dynInsn_approx = get_eventcount(0);  
    
        
        for (y = 1; y < (srcImage.h - 1); y++) {
            x = 0;
            NPU_HALF_WINDOW(srcImage, dstImage_approx, x, y, x_wr, y_wr, im_w, iBuff, oBuff, tmp_out, invoc_index, i);

            for (x = 1; x < srcImage.w - 1; x++) {
                NPU_WINDOW(srcImage, dstImage_approx, x, y, x_wr, y_wr, im_w, iBuff, oBuff, tmp_out, invoc_index, i);
            }
            
            x = srcImage.w - 1;
            NPU_HALF_WINDOW(srcImage, dstImage_approx, x, y, x_wr, y_wr, im_w, iBuff, oBuff, tmp_out, invoc_index, i);
        }


   */ 
    
    ///////////////////////////////
    // 4 - Compute RMSE
    ///////////////////////////////
    
    // Compute RMSE 
    double RMSE = 0;
    double diff;
    unsigned int total = srcImage.w*srcImage.h;
    /*
    for (y=0;y<srcImage.h;y++) {
        for (x=0;x<srcImage.w;x++) {
            diff = dstImage_precise.pixels[y][x].r - dstImage_approx.pixels[y][x].r;
            RMSE += (diff*diff);
        }
    }
    RMSE = RMSE/total;
    RMSE = sqrt(RMSE);
    */
    
    
    ///////////////////////////////
    // 5 - Report results
    ///////////////////////////////

    /*
#if PROFILE_MODE != 0
    printf("WARNING: kernel level profiling affects cycle counts of whole application\n");
#endif
    printf("Precise execution took:     %u cycles \n", t_precise);
    printf("                            %u dynamic instructions\n", dynInsn_precise);
    printf("Approximate execution took: %u cycles\n" , t_approx);
    printf("                            %u dynamic instructions\n", dynInsn_approx);
#if PROFILE_MODE == 1
    printf("                            %lld dynamic NPU instructions\n", dynInsn_kernel_approx);
#endif
    printf("==> NPU speedup is %.2fX\n", (float) t_precise/t_approx);
    printf("==> NPU dynamic instruction reduction is %.2fX\n", (float) dynInsn_precise/dynInsn_approx);
    printf("==> RMSE = %.4f\n", (float) RMSE);
#if PROFILE_MODE == 1
    printf("==> Percentage of dynamic NPU instructions %.2f%%\n", (float) dynInsn_kernel_approx/dynInsn_approx*100);
#elif PROFILE_MODE == 2
    printf("\nKERNEL INFO: \n");
    printf("Number of kernels:            %d \n", kernel_invocations);
    printf("Precise execution:          %lld cycles spent in kernels\n", t_kernel_precise);
    printf("Approximate execution:      %lld cycles spent in kernels \n", t_kernel_approx);
#endif //PROFILE_MODE
    */
    

#if DUMP_DATA == 1
    // Print the precise output
    printf("\nPrecise output RGB dump...\n");
    printf("256,256\n");
    for (y=0;y<srcImage.h;y++) {
        for (x=0;x<srcImage.w;x++) {
            if (x!=srcImage.w-1) {
                printf("%d,", (int) (dstImage_precise.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage_precise.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage_precise.pixels[y][x].r*256));
            } else {
                printf("%d,", (int) (dstImage_precise.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage_precise.pixels[y][x].r*256));
                printf("%d\n", (int) (dstImage_precise.pixels[y][x].r*256));
            }
        }
    }
    // printf("\"{\'bitdepth\': 8, \'interlace\': 0, \'planes\': 3, \'greyscale\': False, \'alpha\': False, \'size\': (256, 256)}\"%%\n");
    
    // Print the approximate output
    /*
    printf("\nApproximate output RGB dump...\n");
    for (y=0;y<srcImage.h;y++) {
        for (x=0;x<srcImage.w;x++) {
            if (x!=srcImage.w-1) {
                printf("%d,", (int) (dstImage_approx.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage_approx.pixels[y][x].g*256));
                printf("%d,", (int) (dstImage_approx.pixels[y][x].b*256));
            } else {
                printf("%d,", (int) (dstImage_approx.pixels[y][x].r*256));
                printf("%d,", (int) (dstImage_approx.pixels[y][x].g*256));
                printf("%d\n", (int) (dstImage_approx.pixels[y][x].b*256));
            }
        }
    }
    */
#endif //DUMP_DATA
    
    
    ///////////////////////////////
    // 6 - Free memory
    ///////////////////////////////
    
    freeRgbImage(ENDORSE(&srcImage));
    freeRgbImage(ENDORSE(&dstImage_precise));
    freeRgbImage(ENDORSE(&dstImage_approx));
    
    return 0;
}


