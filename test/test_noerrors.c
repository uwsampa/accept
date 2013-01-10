#include <enerc.h>

int main() {
    APPROX int x;
    x = 5;

    int y;
    x = x; // OK
    y = y; // OK

    if (1) {} // OK
    if (y) {} // OK

    APPROX int* xp;
    int* yp;
    xp = &x; // OK
    xp = &y; // OK
    yp = &y; // OK
    if (*yp) {} // OK
    APPROX int ax[] = {3, 4, 5};
    int ay[] = {3, 4, 5};
    if (ay[0]) {} // OK

    ax[0] = 2;
    ax[1] = ax[2];
    ay[0] = 3;
    ax[1] = ay[2];

    return 0;
}
