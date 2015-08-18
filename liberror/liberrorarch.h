#ifndef _LIBERROR_ARCH_H_
#define _LIBERROR_ARCH_H_

/*
 * This file defines any parameter that should be considered
 * 'architectural', i.e. cpu frequency, cache parameters, etc.
 */

// ARM A9 class processor
const double CPU_FREQ = 2000000000.0; // 2 GHz
const double CPI = 2.2; // average Cycles Per Instruction (CPI)
const int L1D_ASSOC = 4; // 4-way
const int L1D_SIZE = (1<<15); // 32 KB
const int L1D_BLOCKSIZE = 32; // 32 Bytes

#endif /* _LIBERROR_ARCH_H_ */
