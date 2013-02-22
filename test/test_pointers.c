#include <string.h>
#include <enerc.h>
#include <stdlib.h>

void func(APPROX int *xp) {
    *(xp + 1) = 3;
    APPROX int i = *xp;
    APPROX int *j = 0;
    j = xp;
    memcpy(ENDORSE(xp), ENDORSE(j), 2);
}

int main() {
    APPROX int *xp;
    int *yp;
    yp = (int*)malloc(2);
    xp = (int*)malloc(2);
    free(yp);
    free(ENDORSE(xp));
}
