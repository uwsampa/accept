// RUN: clang++ -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

#include <vector>
int main() {
    int x = 2;
    const int &y = x;
}
