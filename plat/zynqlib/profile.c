/*
 * profile.c
 *
 *  Created on: July 7, 2013
 *      Author: Thierry Moreau <moreau@cs.washington.edu>
 */

#include <assert.h>

#include "profile.h"

inline unsigned int get_cyclecount (void)
{
    unsigned int value;
    // read CCNT Register
    asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value)); 
    return value;
}

inline unsigned int get_eventcount (int event)
{
    unsigned int value;
    // select performance register
    asm volatile ("MCR p15, 0, %0, c9, c12, 5\t\n" :: "r"(event));
    // read CCNT Register
    asm volatile ("MRC p15, 0, %0, c9, c13, 2\t\n": "=r"(value)); 
    return value;
}

inline unsigned int get_granularity (void)
{
    unsigned int value;
    asm volatile ("MRC p15, 0, r5, c9, c13, 0\t\n"::: "r5","cc");
    asm volatile ("MRC p15, 0, r6, c9, c13, 0\t\n"::: "r6","cc");
    asm volatile ("SUB %0, r6, r5\t\n": "=r"(value));
    return value;
}

inline unsigned int get_overflow (void)
{
    unsigned int value;
    // Read OVSR Register (PMOVSR - 32 bit)
    asm volatile ("MRC p15, 0, %0, c9, c12, 3\t\n": "=r"(value));
    return value;
}

inline void init_perfcounters (int do_reset, int enable_divider, int num_counters, int* event_types)
{
    int i;
    int value;
    
    assert (num_counters<4 || num_counters>=0);
    
    // enable all counters (including cycle counter)
    value = (1<<0);

    // peform reset: 
    if (do_reset)
    {
        value |= (1<<1);     // reset all counters to zero.
        value |= (1<<2);     // reset cycle counter to zero.
    }

    if (enable_divider)
        value |= (1<<3);     // enable "by 64" divider for CCNT.
    
    value |= (1<<4);
    
    value |= (num_counters)<<11;

    // program the performance-counter control-register:
    asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(value)); 
    
    for (i=0; i<num_counters; i++) {
        // select performance register
        asm volatile ("MCR p15, 0, %0, c9, c12, 5\t\n" :: "r"(i));
        // set the performance register type
        asm volatile ("MCR p15, 0, %0, c9, c13, 1\t\n" :: "r"(event_types[i]));
    }
    
    // enable cycle counter 
    value = (1<<31);
    
    for (i=0; i<num_counters; i++) {
        value |= (1<<i);
    }
    
    // enable counters: 
    asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(value)); 

    // clear overflows:
    asm volatile ("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(value));    
}

