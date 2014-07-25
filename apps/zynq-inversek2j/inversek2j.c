#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <enerc.h>

// andreolb: we are not executing in NPU.

#include "npu.h"
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
    int i;
    int j;
    int x;
    int n;
    
    // Init rand number generator:
    srand (1);
    
    // Set input size if not set.
    if (argc < 2) {
        n = 4096;
    } else {
        n = atoi(argv[1]);
    }
    
    // Allocate input and output arrays
    float* xy           = (float*)malloc(n * 2 * sizeof (float));
    float* t1t2 = (float*)malloc(n * 2 * sizeof (float));
    APPROX float *t1t2out = (float*)malloc(n * 2 * sizeof (float));
    
    // Ensure memory allocation was successful
    if(t1t2 == NULL || xy == NULL) {
        printf("Cannot allocate memory!\n");
        return -1;
    }
    
    // Initialize input data
    for (i = 0; i < n * NUM_INPUTS; i += NUM_INPUTS) {
        x = rand();
        t1t2[i] = (((float)x)/RAND_MAX) * PI / 2;
        x = rand();
        t1t2[i + 1] = (((float)x)/RAND_MAX) *  PI / 2;
        forwardk2j(t1t2[i + 0], t1t2[i + 1], xy + (i + 0), xy + (i + 1));
    }
    
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);

    accept_roi_begin();

    for (i = 0; i < n * NUM_INPUTS; i += NUM_INPUTS) {
    
        inversek2j(xy[i + 0], xy[i + 1], t1t2out + (i + 0), t1t2out + (i + 1));

    }

    accept_roi_end();

    // Output results.
    for (int k = 0; k < n * NUM_INPUTS; k += NUM_INPUTS)
      printf("\n%f\t%f", *(t1t2out + (k + 0)), *(t1t2out + (k + 1)));
    
    free(t1t2);
    free(t1t2out);
    free(xy);

    return 0;
}


