#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <enerc.h>

// andreolb: we are not executing in NPU.

#include "npu.h"
#include "profile.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_types.h"


#define PI 3.14159265

// Inversek2j parameters
#define NUM_INPUTS      2
#define NUM_OUTPUTS     2
#define BUFFER_SIZE     32
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
// andreolb: not measuring anything for now.
/*
long long int t_kernel_precise;
long long int t_kernel_approx;
long long int dynInsn_kernel_approx;
*/

float l1 = 0.5;
float l2 = 0.5;

void forwardk2j(APPROX float theta1, APPROX float theta2, float* x, float* y) {
    *x = l1 * cos(ENDORSE(theta1)) + l2 * cos(ENDORSE(theta1 + theta2));
    *y = l1 * sin(ENDORSE(theta1)) + l2 * sin(ENDORSE(theta1 + theta2));
}

void inversek2j(float x, float y, APPROX float* theta1, APPROX float* theta2) {
    *theta2 = acos(((x * x) + (y * y) - (l1 * l1) - (l2 * l2))/(2 * l1 * l2));
    *theta1 = asin((y * (l1 + l2 * cos(ENDORSE(*theta2))) - x * l2 * sin(ENDORSE(*theta2)))/(x * x + y * y));
}


int main (int argc, const char* argv[]) {

    // Performance counters
    // andreolb: not measuring anything for now.
    /*
    unsigned int t_precise;
    unsigned int t_approx;
    unsigned int dynInsn_precise;
    unsigned int dynInsn_approx;
    unsigned int evt_counter[1] = {0x68};
    */

    // Inversek2j variables
    int i;
    int j;
    int x;
    int n;


    ///////////////////////////////
    // 1 - Initialization
    ///////////////////////////////

    // Init performance counters:
    // andreolb: not measuring anything for now.
    /*
    init_perfcounters (1, 0, 1, evt_counter);
    t_kernel_precise = 0;
    t_kernel_approx = 0;
    dynInsn_kernel_approx = 0;
    */
    
    // Init rand number generator:
    srand (1);
    //npu();
    
    // Set input size to 100000 if not set
    if (argc < 2) {
        n = 32;
    } else {
        n = atoi(argv[1]);
    }
    //assert (n%(BUFFER_SIZE)==0);
    
    // Allocate input and output arrays
    float* xy           = (float*)malloc(n * 2 * sizeof (float));
    float* xy_approx    = (float*)malloc(n * 2 * sizeof (float));
    APPROX float* t1t2_precise = (float*)malloc(n * 2 * sizeof (float));
    APPROX float* t1t2_approx  = (float*)malloc(n * 2 * sizeof (float));
    
    // Ensure memory allocation was successful
    if(t1t2_approx == NULL || t1t2_precise == NULL || xy == NULL) {
        printf("Cannot allocate memory!\n");
        return -1;
    }
    
    // Initialize input data
    for (i = 0; i < n * NUM_INPUTS; i += NUM_INPUTS) {
        x = rand();
        t1t2_precise[i] = (((float)x)/RAND_MAX) * PI / 2;
        x = rand();
        t1t2_precise[i + 1] = (((float)x)/RAND_MAX) *  PI / 2;
        forwardk2j(t1t2_precise[i + 0], t1t2_precise[i + 1], xy + (i + 0), xy + (i + 1));
    }
    
    printf("\n\nRunning inversek2j benchmark on %u inputs\n\n", n);

    
    ///////////////////////////////
    // 2 - Precise execution
    ///////////////////////////////
    
// andreolb: not measuring anything for now.
/*
#if TIMER==0
    t_precise = get_cyclecount();
#else
    t_precise = rd_fpga_clk();
#endif //TIMER

    dynInsn_precise = get_eventcount(0);  
*/

    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);
#if POWER_MODE == 1
    while (1) {
#endif //POWER_MODE

        for (i = 0; i < n * NUM_INPUTS; i += NUM_INPUTS) {
        
// andreolb: not measuring anything for now.
/*
#if PROFILE_MODE == 2
            t_kernel_precise_start();
#endif //PROFILE_MODE == 2
*/

            inversek2j(xy[i + 0], xy[i + 1], t1t2_precise + (i + 0), t1t2_precise + (i + 1));

// andreolb: not measuring anything for now.
/*
#if PROFILE_MODE == 2
            t_kernel_precise_stop();
#endif //PROFILE_MODE == 2
*/

        }

        for (int k = 0; k < n * NUM_INPUTS; k += NUM_INPUTS)
          printf("\n%f\t%f", *(t1t2_precise + (k + 0)), *(t1t2_precise + (k + 1)));
    
#if POWER_MODE == 1
    }
#endif //POWER_MODE

// andreolb: not measuring anything for now.
/*
    dynInsn_precise = get_eventcount(0) - dynInsn_precise; 
    
#if TIMER==0
    t_precise = get_cyclecount() - t_precise;
#else
    t_precise = rd_fpga_clk() - t_precise;
#endif //TIMER
*/


    ///////////////////////////////
    // 3 - Approximate execution
    ///////////////////////////////

// andreolb: No NPU execution for now.
/*
    
    // Pointers NPU based inversek2j
    float * srcData;
    float * dstData;
    volatile float * iBuff;
    volatile float * oBuff;
    
    // TLB page settings
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);
    
    iBuff = (float*) OCM_SRC;
    oBuff = (float*) OCM_DST;
    
#if TIMER==0
    t_approx = get_cyclecount();
#else
    t_approx = rd_fpga_clk();
#endif //TIMER

    dynInsn_approx = get_eventcount(0);  
    
#if POWER_MODE == 2
    while (1) {
#endif //POWER_MODE
        
        // NPU OFFLOADING
        for (i = 0; i < n * NUM_INPUTS; i += NUM_INPUTS * BUFFER_SIZE){
        
#if PROFILE_MODE == 2
            t_kernel_approx_start();
#elif PROFILE_MODE == 1
            dynInsn_kernel_approx_start();
#endif //PROFILE_MODE
            
            srcData = (xy + i);
            dstData = (t1t2_approx + i);
            iBuff = (float*) OCM_SRC;
            oBuff = (float*) OCM_DST;
            for(j = 0; j < NUM_INPUTS * BUFFER_SIZE; j++) {
                *(iBuff++) = *(srcData ++);
            }
            npu();
            for(j = 0; j < NUM_OUTPUTS * BUFFER_SIZE; j++) {
                *(dstData++) = *(oBuff ++);
            }
            
#if PROFILE_MODE == 2
            t_kernel_approx_stop();
#elif PROFILE_MODE == 1
            dynInsn_kernel_approx_stop();
#endif //PROFILE_MODE

        }
        
#if POWER_MODE == 2
    }
#endif //POWER_MODE

    dynInsn_approx = get_eventcount(0) - dynInsn_approx; 
    
#if TIMER==0
    t_approx = get_cyclecount() - t_approx;
#else
    t_approx = rd_fpga_clk() - t_approx;
#endif //TIMER
*/
    
    
    ///////////////////////////////
    // 4 - Compute RMSE
    ///////////////////////////////
    //
// andreolb
// No NPU execution, so there's no need to compare the npu results with the precise ones.
/*
    // Perform forward kinematics on approx thetas
    for (i = 0; i < n * NUM_INPUTS; i += NUM_INPUTS) {
        forwardk2j(t1t2_approx[i + 0], t1t2_approx[i + 1], xy_approx + (i + 0), xy_approx + (i + 1));
    }
    
    // Compute RMSE and NRMSE
    double RMSE = 0;
    double NRMSE = 0;
    double diff0, diff1;
    float min0 = xy[0];
    float min1 = xy[1];
    float max0 = xy[0];
    float max1 = xy[1];
    unsigned int total = n;
    for (i = 0; i < n * NUM_OUTPUTS; i += NUM_OUTPUTS){
        // Check for nan errors
        if (isnan(t1t2_approx[i+0])||isnan(t1t2_approx[i+1])) {
            printf("ERROR: NaN @ address 0x%X\n", (unsigned int) (t1t2_approx+i));
            exit(0);
        }
        if (isnan(t1t2_precise[i+0])||isnan(t1t2_precise[i+1])) {
            total --;
        } else {
            min0 = (xy[i+0] < min0) ? xy[i+0] : min0;
            min1 = (xy[i+1] < min1) ? xy[i+1] : min1;
            max0 = (xy[i+0] > max0) ? xy[i+0] : max0;
            max1 = (xy[i+1] > max1) ? xy[i+1] : max1;
            diff0 = xy[i+0] - xy_approx[i+0];
            diff1 = xy[i+1] - xy_approx[i+1];
            RMSE += (diff0*diff0+diff1*diff1);
        }
    }
    RMSE = RMSE/total;
    diff0 = max0 - min0;
    diff1 = max1 - min1;
    RMSE = sqrt(RMSE);
    NRMSE = RMSE/(sqrt(diff0+diff1));
*/
    
    
    ///////////////////////////////
    // 5 - Report results
    ///////////////////////////////

// andreolb: no need to report results.
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
    printf("==> RMSE = %.4f (NRMSE = %.2f%%)\n", (float) RMSE, (float) ((100.*NRMSE)));
#if PROFILE_MODE == 1
    printf("==> Percentage of dynamic NPU instructions %.2f%%\n", (float) dynInsn_kernel_approx/dynInsn_approx*100);
#elif PROFILE_MODE == 2
    printf("\nKERNEL INFO: \n");
    printf("Number of kernels:            %d \n", n);
    printf("Precise execution:          %lld cycles spent in kernels\n", t_kernel_precise);
    printf("Approximate execution:      %lld cycles spent in kernels \n", t_kernel_approx);
#endif //PROFILE_MODE
*/
    

    ///////////////////////////////
    // 6 - Free memory
    ///////////////////////////////
    
    free(t1t2_precise);
    free(t1t2_approx);
    free(xy);
    free(xy_approx);

    return 0;
}


