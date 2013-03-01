// RUN: clang -fsyntax-only -Xclang -verify %s

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

int ppfunc(int *arg) {
    return 2;
}

int pretfunc() {
    APPROX int v = 5;
    return v; // expected-error {{precision flow violation}}
}

int main() {
    APPROX int x;
    x = 5;

    // Flow violation.
    int y;
    y = x; // expected-error {{precision flow violation}}
    x = x; // OK
    y = y; // OK

    // Approximate & precise conditions.
    if (x) {} // expected-error {{approximate condition}}
    while (x) {} // expected-error {{approximate condition}}
    for (int i=2; i<x; ++i) {} // expected-error {{approximate condition}}
    switch (x) {} // expected-error {{approximate condition}}
    if (1) {} // OK
    if (y) {} // OK

    // Pointers and such.
    APPROX int* xp;
    int* yp;
    xp = &x; // OK
    yp = &x; // expected-error {{precision flow violation}}
    xp = &y; // expected-error {{precision flow violation}}
    yp = &y; // OK
    if (*yp) {} // OK
    if (*xp) {} // expected-error {{approximate condition}}
    APPROX int ax[] = {3, 4, 5};
    int ay[] = {3, 4, 5};
    if (ax[0]) {} // expected-error {{approximate condition}}
    if (ay[0]) {} // OK

    // Pointer types are incompatible.
    xp = xp; // OK
    yp = yp; // OK
    xp = yp; // expected-error {{precision flow violation}}
    yp = xp; // expected-error {{precision flow violation}}

    // Function calls.
    afunc(2); // OK
    pfunc(2); // OK
    aafunc(2); // OK
    afunc(x); // OK
    aafunc(x); // OK
    pfunc(x); // expected-error {{precision flow violation}}
    y = aafunc(x); // expected-error {{precision flow violation}}
    x = aafunc(x); // OK
    ppfunc(xp); // expected-error {{precision flow violation}}

    // Variable initialization.
    int pinit = x; // expected-error {{precision flow violation}}

    return 0;
}
