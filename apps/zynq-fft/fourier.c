#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "fourier.h"

// Global variables
// andreolb
// We are not measuring performance, so these are not needed.
/*
long long int t_kernel_precise;
long long int t_kernel_approx;
long long int dynInsn_kernel_approx;
int kernel_invocations;
*/


void calcFftIndices(int K, int* indices) {
    int i, j;
    int N;

    N = (int)log2f(K);

    indices[0] = 0;
    indices[1 << 0] = 1 << (N - (0 + 1));
    for(i = 1; i < N; ++i) {
        for(j = (1 << i); j < (1 << (i + 1)); ++j) {
            indices[j] = indices[j - (1 << i)] + (1 << (N - (i + 1)));
        }
    }
}

// andreolb
// We are not running the NPU code.
/*
void npuFft(int K, int* indices, Complex* x, Complex* f) {

    int i, j, k, l;
    int N;
    int step;
    int eI;
    int oI;
    Complex t;
    float fftSin;
    float fftCos;
    float arg;
    
    // Source and destination buffers
    volatile float* iBuff = (float*) OCM_SRC;
    volatile float* oBuff = (float*) OCM_DST;
    // Invocation count
    int invocations = 0;
    int calls = 0;
    // Buffer for eI and oI and t
    int eI_log[BUFFER_SIZE*NUM_INPUTS];
    int oI_log[BUFFER_SIZE*NUM_INPUTS];
    // Buffer for fftSin and fftCos
    float fft_log[BUFFER_SIZE*NUM_OUTPUTS];

    calcFftIndices(K, indices);

    for(i = 0, N = 1 << (i + 1); N <= K; i++, N = 1 << (i + 1)) {
        for(j = 0; j < K; j += N) {
            step = N >> 1;
            for (k = 0; k < step; k++) {
            
#if PROFILE_MODE == 2
                t_kernel_approx_start();
#elif PROFILE_MODE == 1
                dynInsn_kernel_approx_start();
#endif //PROFILE_MODE
                
                eI_log[invocations] = j + k;
                oI_log[invocations] = j + step + k;
                *(iBuff++) = (float)k / N;
                invocations ++;
                calls ++;
                
                if (invocations == BUFFER_SIZE*NUM_INPUTS) {
                    npu();
                    for (l=0; l<BUFFER_SIZE*NUM_OUTPUTS; l++) {
                        fft_log[l] = *oBuff++;
                    }
                    for (l=0; l<BUFFER_SIZE; l++) {
                        eI = eI_log[l];
                        oI = oI_log[l];
                        t = x[indices[eI]];
                        fftSin = fft_log[2*l+0];
                        fftCos = fft_log[2*l+1];
#if PROFILE_MODE == 2
                        t_kernel_approx_stop();
#elif PROFILE_MODE == 1
                        dynInsn_kernel_approx_stop();
#endif //PROFILE_MODE
                        
                        x[indices[eI]].real = t.real + (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
                        x[indices[eI]].imag = t.imag + (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);
                        
                        x[indices[oI]].real = t.real - (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
                        x[indices[oI]].imag = t.imag - (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);

#if PROFILE_MODE == 2
                        t_kernel_approx_start();
#elif PROFILE_MODE == 1
                        dynInsn_kernel_approx_start();
#endif //PROFILE_MODE

                    }
                    
                    invocations = 0;
                    iBuff = (float*) OCM_SRC;
                    oBuff = (float*) OCM_DST;
                }
                
#if PROFILE_MODE == 2
                t_kernel_approx_stop();
#elif PROFILE_MODE == 1
                dynInsn_kernel_approx_stop();
#endif //PROFILE_MODE

            }
        }
    } 
    
#if PROFILE_MODE == 2
    t_kernel_approx_start();
#elif PROFILE_MODE == 1
    dynInsn_kernel_approx_start();
#endif //PROFILE_MODE

    // Catch the remaining invocations
    if (invocations!=0) {
        npu();
        for (l=0; l<invocations; l++) {
            fft_log[l] = *oBuff++;
        }
        for (l=0; l<invocations; l++) {
            eI = eI_log[l];
            oI = oI_log[l];
            t = x[indices[eI]];
            fftSin = fft_log[2*l+0];
            fftCos = fft_log[2*l+1];
            
#if PROFILE_MODE == 2
            t_kernel_approx_stop();
#elif PROFILE_MODE == 1
            dynInsn_kernel_approx_stop();
#endif //PROFILE_MODE
            
            x[indices[eI]].real = t.real + (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
            x[indices[eI]].imag = t.imag + (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);
            
            x[indices[oI]].real = t.real - (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
            x[indices[oI]].imag = t.imag - (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);

#if PROFILE_MODE == 2
            t_kernel_approx_start();
#elif PROFILE_MODE == 1
            dynInsn_kernel_approx_start();
#endif //PROFILE_MODE

        }
        invocations = 0;
        iBuff = (float*) OCM_SRC;
        oBuff = (float*) OCM_DST;
    }

#if PROFILE_MODE == 2
    t_kernel_approx_stop();
#elif PROFILE_MODE == 1
    dynInsn_kernel_approx_stop();
#endif //PROFILE_MODE

    for(i = 0; i < K; ++i)
        f[i] = x[indices[i]];
    
}
*/

void radix2DitCooleyTukeyFft(int K, int* indices, Complex* x, Complex* f) {

    int i;
    int N;
    int j;
    int k;
    int step;
    int eI;
    int oI;
    Complex t;
    float fftSin;
    float fftCos;
    float arg;

    calcFftIndices(K, indices);
    
    // andreolb: not measuing performance
    // kernel_invocations = 0;

    for(i = 0, N = 1 << (i + 1); N <= K; i++, N = 1 << (i + 1)) {
        for(j = 0; j < K; j += N) {
            step = N >> 1;
            for (k = 0; k < step; k++) {
                
// andreolb: not measuing performance
/*
#if PROFILE_MODE == 2
                t_kernel_precise_start();
                kernel_invocations ++;
#endif //PROFILE_MODE == 2
*/

                arg = (float)k / N;
                eI = j + k;
                oI = j + step + k;

                fftSinCos(arg, &fftSin, &fftCos);

                t = x[indices[eI]];
   
// andreolb: not measuing performance
/*
#if PROFILE_MODE == 2
                t_kernel_precise_stop();
#endif //PROFILE_MODE == 2
*/

                x[indices[eI]].real = t.real + (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
                x[indices[eI]].imag = t.imag + (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);

                x[indices[oI]].real = t.real - (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
                x[indices[oI]].imag = t.imag - (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);
            }
        }
    }

    for(i = 0; i < K; ++i)
        f[i] = x[indices[i]];
        
}


