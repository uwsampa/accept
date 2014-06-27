#ifndef DFT_H_
#define DFT_H_

#include "complex.h"

// andreolb: no NPU for now.
/*
#include "npu.h"
#include "profile.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_types.h"
*/

// FFT parameters
#define NUM_INPUTS      1
#define NUM_OUTPUTS     2
#define BUFFER_SIZE     32

void radix2DitCooleyTukeyFft(int K, int* indices, Complex* x, Complex* f);
void npuFft(int K, int* indices, Complex* x, Complex* f);

#endif /* DFT_H_ */
