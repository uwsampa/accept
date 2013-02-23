#include <stdio.h>
#include <stdlib.h>

int enerc_registered_atexit = 0;
unsigned long enerc_approx = 0;
unsigned long enerc_elidable = 0;
unsigned long enerc_total = 0;

void enerc_done() {
  printf("%lu %lu %lu\n", enerc_approx, enerc_elidable, enerc_total);
}

void enerc_trace(int approx, int elidable, int total) {
  enerc_approx += approx;
  enerc_elidable += elidable;
  enerc_total += total;
  
  if (!enerc_registered_atexit) {
    atexit(enerc_done);
    enerc_registered_atexit = 1;
  }
}
