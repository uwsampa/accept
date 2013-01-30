#include <stdio.h>

FILE *enerc_trace_file = NULL;

void enerc_trace(int instid) {
  if (enerc_trace_file == NULL) {
    enerc_trace_file = fopen("enerc_trace.txt", "w");
  }
  fprintf(enerc_trace_file, "%i\n", instid);
}
