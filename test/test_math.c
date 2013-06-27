// RUN: clang -fsyntax-only -Xclang -verify %s

#include <enerc.h>
#include <stdlib.h>
#include <math.h>

int main() {
    APPROX int x = -2;
    APPROX int y = abs(x);

    APPROX double a = 1.0;
    APPROX double b = cos(a);

    double c = cos(a); // expected-error {{precision flow violation}}
}
