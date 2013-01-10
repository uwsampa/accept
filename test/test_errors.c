#include <enerc.h>

int afunc(APPROX int arg) {
    return 2;
}

int pfunc(int arg) {
    return 2;
}

APPROX int aafunc(APPROX int arg) {
    return 2;
}

int pretfunc() {
    APPROX int v = 5;
    return v; // error
}

int main() {
    APPROX int x;
    x = 5;

    // Flow violation.
    int y;
    y = x; // error
    x = x; // OK
    y = y; // OK

    // Approximate & precise conditions.
    if (x) {} // error
    while (x) {} // error
    for (int i=2; i<x; ++i) {} // error
    switch (x) {} // error
    if (1) {} // OK
    if (y) {} // OK

    // Pointers and such.
    APPROX int* xp;
    int* yp;
    xp = &x; // OK
    yp = &x; // error
    xp = &y; // OK
    yp = &y; // OK
    if (*yp) {} // OK
    if (*xp) {} // error
    APPROX int ax[] = {3, 4, 5};
    int ay[] = {3, 4, 5};
    if (ax[0]) {} // error
    if (ay[0]) {} // OK

    // Function calls.
    afunc(2); // OK
    pfunc(2); // OK
    aafunc(2); // OK
    afunc(x); // OK
    aafunc(x); // OK
    pfunc(x); // error
    y = aafunc(x); // error
    x = aafunc(x); // OK

    // Variable initialization.
    int pinit = x; // error

    return 0;
}
