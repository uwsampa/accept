// RUN: clang -fsyntax-only -Xclang -verify %s

#include <enerc.h>

// OK: matching approximate param.
void func1(APPROX int p);
void func1(APPROX int p) {
}

// OK: matching precise param.
void func2(int p);
void func2(int p) {
}

// OK: matching approximate return value.
APPROX int func3();
APPROX int func3() {
    return 1;
}

// OK: matching precise return.
int func4();
int func4() {
    return 1;
}

// param mismatch: approx then precise
void func5(APPROX int p);
void func5(int p) { // expected-error {{redeclaration}}
}

// param mismatch: precise then approx
void func6(int p);
void func6(APPROX int p) { // expected-error {{redeclaration}}
}

// return mismatch: approx then precise
APPROX int func7();
int func7() { // expected-error {{redeclaration}}
    return 1;
}

// return mismatch: precise then approx
int func8();
APPROX int func8() { // expected-error {{redeclaration}}
    return 1;
}
