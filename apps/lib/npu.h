/*
 * npu.h
 *
 *  Created on: Oct 30, 2013
 *      Author: Thierry Moreau <moreau@cs.washington.edu>
 */

#ifndef NPU_HEADER_
#define NPU_HEADER_

#define OCM_SRC           0xFFFF8000
#define OCM_DST           0xFFFFF000
#define OCM_CHK           OCM_DST + BUFFER_SIZE*NUM_OUTPUTS-1

#define dsb()             __asm__ __volatile__ ("dsb" : : : "memory")
#define sev()             __asm__ __volatile__ ("SEV\n")
#define wfe()             __asm__ __volatile__ ("WFE\n")
#define init()            (*((volatile int *)OCM_CHK)=0)
#define check()           assert(*((volatile int *)OCM_CHK)==0)
#define wait()            while (*((volatile int *)OCM_CHK)==0) {; } 

//#define npu()             init(); sev(); wait();
#define npu()             dsb(); sev(); wfe(); wfe(); 

#endif /* NPU_HEADER_ */
