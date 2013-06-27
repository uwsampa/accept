// RUN: clang -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#include <string.h>
#include <enerc.h>
#include <stdlib.h>

void func(APPROX int *xp) {
    *(xp + 1) = 3;
    APPROX int i = *xp;
    APPROX int *j = 0;
    j = xp;
    memcpy(xp, j, 2);
}

int main() {
    // malloc+free
    APPROX int *xp;
    int *yp;
    yp = (int*)malloc(2);
    xp = (int*)malloc(2);
    free(yp);
    free(xp);

    // propagation through arithmetic/casts
    xp = xp;
    xp = (int*)xp;
    xp = xp + 2;
    xp = (int*)(xp + 2);
    xp = ((int*)0);

    // array-to-pointer demotion
    APPROX int arr[10];
    APPROX int *aptr = arr;

    // unary operators on pointers
    APPROX int v = 0;
    *xp++ = v;
}
