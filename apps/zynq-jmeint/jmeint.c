#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tritri.h"

// andreolb: we are not executing in NPU.
#include "npu.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_types.h"

// Jmeint parameters
#define NUM_INPUTS      18
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
// andreolb
// We are not doing measurements.
/*
long long int t_kernel_precise;
long long int t_kernel_approx;
long long int dynInsn_kernel_approx;
*/


int main(int argc, char* argv[]) {

    // Performance counters
    // andreolb
    // We are not doing measurements.
    /*
    unsigned int t_precise;
    unsigned int t_approx;
    unsigned int dynInsn_precise;
    unsigned int dynInsn_approx;
    unsigned int evt_counter[1] = {0x68};
    */
    
    // Jmeint variables & pointers
    int i, j;
    int x;
    int n;
    float* xyz;
    //APPROX int* x_precise;
    APPROX float* x_precise;
    int* x_approx;
    float* x_approx_raw;
    
    
    ///////////////////////////////
    // 1 - Initialization
    ///////////////////////////////
        
    // Init performance counters:
    // andreolb
    // We are not doing measurements.
    /*
    init_perfcounters (1, 0, 1, evt_counter);
    t_kernel_precise = 0;
    t_kernel_approx = 0;
    dynInsn_kernel_approx = 0;
    */
    
    // Init rand number generator:
    srand (1);
    
    // Set input size to 10016 if not set
    if (argc < 2) {
        n = 1152; //10000
    } else {
        n = atoi(argv[1]);
    }
    //assert (n%(BUFFER_SIZE)==0);
    
    // Allocate memory for input and output arrays
    xyz = (float*)malloc(n * 6 * 3 * sizeof (float));
    x_precise = (float*)malloc(n * sizeof (float));
    x_approx = (float*)malloc(n * sizeof (float));
    
    // Ensure memory allocation was successful
    if(xyz == NULL || x_precise == NULL) {
        printf("Cannot allocate memory for the triangle coordinates!");
        return -1;
    }
    
    // Initialize input data
    for (i = 0; i < n * 6 * 3; ++i) {
        x = rand();
        xyz[i] = ((float)x)/RAND_MAX;
    }
    
    printf("\n\nRunning jmeint benchmark on %u inputs\n", n);


    ///////////////////////////////
    // 2 - Precise Execution
    ///////////////////////////////
    
    APPROX float * x_precise_ptr = x_precise;
    
// andreolb
// We are not doing measurements.
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

        for (i = 0; i < (n * 6 * 3); i += 6 * 3) {
            
// andreolb
// We are not doing measurements.
/*
#if PROFILE_MODE == 2
        t_kernel_precise_start();
#endif //PROFILE_MODE == 2
*/
//printf("before i: %d\n", i);


            *(x_precise_ptr++) = tri_tri_intersect(
                xyz + i + 0 * 3, xyz + i + 1 * 3, xyz + i + 2 * 3,
                xyz + i + 3 * 3, xyz + i + 4 * 3, xyz + i + 5 * 3
            );
            
//printf("after i: %d\n", i);
// andreolb
// We are not doing measurements.
/*
#if PROFILE_MODE == 2
        t_kernel_precise_stop();
#endif //PROFILE_MODE == 2
*/

        }

        for (int k = 0; k < n; ++k)
          printf("\n%f", x_precise[k]);

#if POWER_MODE == 1
    }
#endif //POWER_MODE

// andreolb
// We are not doing measurements.
/*
    dynInsn_precise = get_eventcount(0) - dynInsn_precise; 
    
#if TIMER==0
    t_precise = get_cyclecount() - t_precise;
#else
    t_precise = rd_fpga_clk() - t_precise;
#endif //TIMER
*/
    
    
    ///////////////////////////////
    // 3 - Approximate Execution
    ///////////////////////////////
    
// andreolb
// No NPU execution.
/*

    // Pointers for NPU based jmeint
    float * srcData = xyz;
    int * dstData = x_approx;
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
        for (i = 0; i < (n * 6 * 3); i += 6 * 3 * BUFFER_SIZE) {
            
#if PROFILE_MODE == 2
            t_kernel_approx_start();
#elif PROFILE_MODE == 1
            dynInsn_kernel_approx_start();
#endif //PROFILE_MODE

            iBuff = (float*) OCM_SRC;
            oBuff = (float*) OCM_DST;
            for (j = 0; j < NUM_INPUTS*BUFFER_SIZE; j++) {
                *(iBuff++) = *(srcData ++);
            }
            npu();
            for (j = 0; j < NUM_OUTPUTS*BUFFER_SIZE/2; j++, oBuff+=2) {
                *(dstData++) = (*(oBuff+0)>*(oBuff+1)) ? 1 : 0;
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
    // 4 - Now compute miss rate
    ///////////////////////////////
    
// andreolb
// No NPU execution, so there's no need to compare the npu results with the precise ones.
/*

    float miss_rate;
    int hits = 0;
    for (i = 0; i < n; i ++) { 
        hits += (x_precise[i] == x_approx[i]) ? 1 : 0;
    }
    miss_rate = ((float)(n-hits)/n)*100.;
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
    printf("==> Miss Rate = %.2f%%\n", miss_rate);
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
    
    free(xyz);

    return 0;
}

