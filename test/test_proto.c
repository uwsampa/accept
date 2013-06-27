// RUN: clang -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#include <enerc.h>

void f(APPROX int a[5]);
void f(APPROX int a[5]) { }

void g(APPROX int *a);
void g(APPROX int *a) { }

void h(APPROX int a);
void h(APPROX int a) { }
