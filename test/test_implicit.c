// RUN: clang -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#include <enerc.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // malloc
    APPROX int *xp;
    int *yp;
    yp = (int*)malloc(2);
    xp = (int*)malloc(2);

    // free
    free(yp);
    free(xp);

    // memcpy
    memcpy(yp, yp, 1);
    memcpy(xp, xp, 1);

    // memset
    memset(xp, 0, 1);
    memset(yp, 0, 1);

    // crashy: const approx
    APPROX const void *data;
    memcpy(0, data, 0);
}
