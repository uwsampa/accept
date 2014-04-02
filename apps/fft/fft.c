#include <stdio.h>
#include "fourier.h"

#define  MAX_K      ((1 << 12))

// FFT parameters in fourier.h

// DUMP DATA:
// 0 - No data dump
// 1 - Data dump
#define DUMP_DATA       0
// TIMER:
// 0 - Use ARM performance counters
// 1 - Use FPGA clock counter
#define TIMER           1 
// POWER MODE:
// 0 - Normal operation
// 1 - Loop on precise execution
// 2 - Loop on approximate execution
#define POWER_MODE      0 

static int indices_1[MAX_K];
static int indices_2[MAX_K];
static Complex x_1[MAX_K];
static Complex x_2[MAX_K];
static Complex f_precise[MAX_K];
static Complex f_approx[MAX_K];

// Global variables
// andreolb
// We are not doing measurements.
/*
long long int t_kernel_precise;
long long int t_kernel_approx;
long long int dynInsn_kernel_approx;
int kernel_invocations;
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

    // fft variables
	int i;
	int K = MAX_K;


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
    
    // Init npu:
    // andreolb: not using npu
    //npu();
	
	// Initialize input data
    for(i = 0; i < K; ++i) {
        x_1[i].real = i;
    	x_1[i].imag = 0;
        x_2[i].real = i;
    	x_2[i].imag = 0;
    }
    
#if BUFFER_SIZE == 1
    printf("\n\nRunning fft benchmark on %u inputs (single invocation mode)\n", K);
#else
    printf("\n\nRunning fft benchmark on %u inputs\n", K);
#endif

    
    ///////////////////////////////
    // 2 - Precise execution
    ///////////////////////////////
    
    
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
    
#if POWER_MODE == 1
    while (1) {
#endif //POWER_MODE

        radix2DitCooleyTukeyFft(K, indices_1, x_1, f_precise);
    
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
    // 3 - Approximate execution
    ///////////////////////////////

// andreolb
// No NPU execution.
/*
    
    // TLB page settings
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);
    
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
        npuFft(K, indices_2, x_2, f_approx);
        
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

// andreolb
// No NPU execution, so there's no need to compare the npu results with the precise ones.
/*
    
    double RMSE = 0;
    double NRMSE = 0;
    double diff;
    float min_imag = f_precise[i].imag;
    float max_imag = f_precise[i].imag;
    for (i = 1; i < K; i ++){
        min_imag = (f_precise[i].imag < min_imag) ? f_precise[i].imag : min_imag;
        max_imag = (f_precise[i].imag > max_imag) ? f_precise[i].imag : max_imag;
        diff = f_precise[i].imag - f_approx[i].imag;
        RMSE += (diff*diff);
    }
    RMSE = RMSE/K;
    RMSE = sqrt(RMSE);
    NRMSE = RMSE/(max_imag-min_imag);
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
    printf("Number of kernels:            %d \n", kernel_invocations);
    printf("Precise execution:          %lld cycles spent in kernels\n", t_kernel_precise);
    printf("Approximate execution:      %lld cycles spent in kernels \n", t_kernel_approx);
#endif //PROFILE_MODE
*/
    
// andreolb: no need to dump final data
/*
#if DUMP_DATA==1
    printf("\nFFT precise real data dump...\n");
    for (i = 1; i < K; i ++){
        printf("%.2f,", f_precise[i].real);
    }
    printf("\nFFT precise imaginary data dump...\n");
    for (i = 1; i < K; i ++){
        printf("%.2f,", f_precise[i].imag);
    }
    printf("\nFFT approx real data dump...\n");
    for (i = 1; i < K; i ++){
        printf("%.2f,", f_approx[i].real);
    }
    printf("\nFFT approx imaginary data dump...\n");
    for (i = 1; i < K; i ++){
        printf("%.2f,", f_approx[i].imag);
    }
#endif //DUMP_DATA
*/
    
    return 0;
}



