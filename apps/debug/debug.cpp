#include <enerc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main() {
  FILE *file;
  char *output_file = "my_output.txt";

  APPROX int a, b;
  int ret_val;
  // for memcpy from approx to approx int pointer
  APPROX int *c = (int *)DEDORSE(malloc(sizeof(int))), *d = DEDORSE(&a);
  // for memcpy from precise to approx int pointer
  int *e = (int *)malloc(sizeof(int));
  APPROX int *f = DEDORSE(&b);

  *c = 21;
  *e = 48;
  accept_roi_begin();
  memcpy(d, c, sizeof(int));
  memcpy(f, e, sizeof(int));
  accept_roi_end();
  printf("a: %d\n", a);
  printf("b: %d\n", b);
  printf("c: %d\n", *c);
  printf("d: %d\n", *d);
  printf("e: %d\n", *e);
  printf("f: %d\n", *f);

  file = fopen(output_file, "w");
  if (file == NULL) {
    perror("fopen for write failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fprintf(file, "%d\n", a);
  if (ret_val < 0) {
    perror("fprintf of int value failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fprintf(file, "%d\n", b);
  if (ret_val < 0) {
    perror("fprintf of int value failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fprintf(file, "%d\n", *c);
  if (ret_val < 0) {
    perror("fprintf of int value failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fprintf(file, "%d\n", *d);
  if (ret_val < 0) {
    perror("fprintf of int value failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fprintf(file, "%d\n", *e);
  if (ret_val < 0) {
    perror("fprintf of int value failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fprintf(file, "%d\n", *f);
  if (ret_val < 0) {
    perror("fprintf of int value failed\n");
    return EXIT_FAILURE;
  }

  ret_val = fclose(file);
  if (ret_val != 0) {
    perror("fclose failed\n");
    return EXIT_FAILURE;
  }

  return 0;
}
