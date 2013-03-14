#ifndef __MSP430__
#include <stdio.h>
#include <stdlib.h>
#endif // __MSP430__

#ifdef __MSP430__
unsigned enerc_approx = 0;
unsigned enerc_elidable = 0;
unsigned enerc_total = 0;
#else
int enerc_registered_atexit = 0;
unsigned long enerc_approx = 0;
unsigned long enerc_elidable = 0;
unsigned long enerc_total = 0;
#endif

#ifndef __MSP430__
void enerc_done() {
  FILE *results_file = fopen("enerc_dynamic.txt", "w");
  fprintf(results_file,
          "%lu %lu %lu\n", enerc_approx, enerc_elidable, enerc_total);
  fclose(results_file);
}
#endif

void enerc_trace(int approx, int elidable, int total) {
  enerc_approx += approx;
  enerc_elidable += elidable;
  enerc_total += total;

#ifndef __MSP430__
  if (!enerc_registered_atexit) {
    atexit(enerc_done);
    enerc_registered_atexit = 1;
  }
#endif // __MSP430__
}
