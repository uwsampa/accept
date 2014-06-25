/*
 * profile.h
 *
 *  Created on: July 7, 2013
 *      Author: Thierry Moreau <moreau@cs.washington.edu>
 */

#ifndef PROFILE_H_
#define PROFILE_H_

#include <stdint.h>

// Profile mode:
//      0: Profiling on entire application
//      1: Kernel level profiling for instruction count
//      2: Kernel level profiling for cycle count
#define PROFILE_MODE 0

// Memory mapped hardware counter
#define GPIO_ADDR       0xE000A048
#define rd_fpga_clk()   *((volatile unsigned int*)GPIO_ADDR)

// kernel timers
#define t_kernel_precise_start()            dsb(); t_kernel_precise = t_kernel_precise - get_cyclecount();
#define t_kernel_precise_stop()             t_kernel_precise = t_kernel_precise + get_cyclecount(); dsb(); 
#define t_kernel_approx_start()             dsb(); t_kernel_approx = t_kernel_precise - get_cyclecount();
#define t_kernel_approx_stop()              t_kernel_approx = t_kernel_precise + get_cyclecount(); dsb(); 
#define dynInsn_kernel_precise_start()      dsb(); dynInsn_kernel_precise = dynInsn_kernel_precise - get_eventcount(0);
#define dynInsn_kernel_precise_stop()       dynInsn_kernel_precise = dynInsn_kernel_precise + get_eventcount(0); dsb(); 
#define dynInsn_kernel_approx_start()       dsb(); dynInsn_kernel_approx = dynInsn_kernel_approx - get_eventcount(0);
#define dynInsn_kernel_approx_stop()        dynInsn_kernel_approx = dynInsn_kernel_approx + get_eventcount(0); dsb();

extern long long int t_kernel_precise;
extern long long int t_kernel_approx;
extern long long int dynInsn_kernel_precise;
extern long long int dynInsn_kernel_approx;
extern int kernel_invocations;

extern inline unsigned int get_cyclecount ();
extern inline unsigned int get_eventcount (int event);
extern inline unsigned int get_granularity ();
extern inline unsigned int get_overflow ();
extern inline void init_perfcounters (int do_reset, int enable_divider, int num_counters, int* event_types);

#endif //PROFILE_H_
